#ifndef RMYSQL_H
#define RMYSQL_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <cstring>
#include <mysql/mysql.h>
#include <nlohmann/json.hpp>

class rmysql {
private:
    MYSQL* conn;

    // --- TEMPLATE BINDING HELPERS FOR MYSQL ---
    static void setup_bind(MYSQL_BIND& b, const int& val) {
        b.buffer_type = MYSQL_TYPE_LONG;
        b.buffer = (void*)&val;
    }
    static void setup_bind(MYSQL_BIND& b, const double& val) {
        b.buffer_type = MYSQL_TYPE_DOUBLE;
        b.buffer = (void*)&val;
    }
    static void setup_bind(MYSQL_BIND& b, const std::string& val) {
        b.buffer_type = MYSQL_TYPE_STRING;
        b.buffer = (void*)val.c_str();
        b.buffer_length = val.length();
    }
    static void setup_bind(MYSQL_BIND& b, const char* val) {
        b.buffer_type = MYSQL_TYPE_STRING;
        b.buffer = (void*)val;
        b.buffer_length = strlen(val);
    }
    static void setup_bind(MYSQL_BIND& b, std::nullptr_t) {
        b.buffer_type = MYSQL_TYPE_NULL;
    }

public:
    // Requires Connection Credentials instead of a File Path
    rmysql(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, unsigned int port = 3306);
    ~rmysql();

    rmysql(const rmysql&) = delete;
    rmysql& operator=(const rmysql&) = delete;
    rmysql(rmysql&& other) noexcept;
    rmysql& operator=(rmysql&& other) noexcept;

    // Core Execution
    void execute(const std::string& sql);
    nlohmann::json query(const std::string& sql);
    nlohmann::json query_single(const std::string& sql);

    // Transactions
    void begin_transaction();
    void commit();
    void rollback();

    // Fast Metadata
    bool table_exists(const std::string& table);
    bool column_exists(const std::string& table, const std::string& column);
    uint64_t get_last_insert_rowid(); // MySQL uses uint64_t for IDs
    uint64_t get_rows_affected();

    // --- VARIADIC TEMPLATE PREPARED STATEMENTS ---
    template<typename... Args>
    void execute_prepared(const std::string& sql, const Args&... args) {
        MYSQL_STMT* stmt = mysql_stmt_init(conn);
        if (!stmt) throw std::runtime_error("mysql_stmt_init failed");

        if (mysql_stmt_prepare(stmt, sql.c_str(), sql.length()) != 0) {
            std::string err = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            throw std::runtime_error("Prepare failed: " + err);
        }

        constexpr size_t num_args = sizeof...(Args);
        if constexpr (num_args > 0) {
            MYSQL_BIND binds[num_args];
            memset(binds, 0, sizeof(binds));

            int i = 0;
            // C++20 Lambda to populate bindings
            auto bind_helper = [&](const auto& val) { setup_bind(binds[i++], val); };
            
            // C++17 Fold Expression unrolls the variadic arguments safely onto the stack
            (bind_helper(args), ...); 

            if (mysql_stmt_bind_param(stmt, binds) != 0) {
                std::string err = mysql_stmt_error(stmt);
                mysql_stmt_close(stmt);
                throw std::runtime_error("Bind param failed: " + err);
            }
        }

        if (mysql_stmt_execute(stmt) != 0) {
            std::string err = mysql_stmt_error(stmt);
            mysql_stmt_close(stmt);
            throw std::runtime_error("Execute failed: " + err);
        }
        mysql_stmt_close(stmt);
    }
};

#endif


/*
 * 
 How to use it:

#include <iostream>
#include <string>
#include <vector>
#include "rmysql.h"

std::vector<std::string> getName(rmysql& db) {
    std::vector<std::string> names;
    
    std::string sql = "SELECT name FROM employee";
    nlohmann::json results = db.query(sql);
    
    for (const auto& row : results) {
        if (!row["name"].is_null()) {
            names.push_back(row["name"].get<std::string>());
        }
    }
    return names;
}

int main() {
    try {
        std::cout << "Connecting to MySQL server...\n";
        // HOST, USER, PASSWORD, DATABASE_NAME, PORT
        // Update these credentials for your local MySQL server!
        rmysql db("127.0.0.1", "root", "password", "dbname", 3306);

        // MySQL Syntax for Auto Increment
        db.execute("CREATE TABLE IF NOT EXISTS employee ("
                   "id INT AUTO_INCREMENT PRIMARY KEY, "
                   "name VARCHAR(255), "
                   "department VARCHAR(255), "
                   "salary DECIMAL(10,2))");

        if (db.table_exists("employee")) {
            std::cout << "Success: Table 'employee' exists.\n";
        }
        
        if (db.column_exists("employee", "salary")) {
            std::cout << "Success: Column 'salary' exists.\n";
        }

        std::cout << "\nInserting data safely (MySQL Prepared Statements)...\n";
        db.begin_transaction(); 
        
        std::string insert_sql = "INSERT INTO employee (name, department, salary) VALUES (?, ?, ?)";
        
        db.execute_prepared(insert_sql, "Alice", "Engineering", "85000.50");
        std::cout << "Inserted Alice. Her ID is: " << db.get_last_insert_rowid() << "\n";
        
        db.execute_prepared(insert_sql, "Bob", "Marketing", 75000.0);
        db.execute_prepared(insert_sql, "Charlie", "Engineering", 92000.0);
        db.execute_prepared(insert_sql, "Dave", "Sales", nullptr); // Null test
        
        db.commit(); 

        std::cout << "\nFetching all employee names:\n";
        auto strNames = getName(db);
        for(const auto& name : strNames) {
            std::cout << "- " << name << "\n";
        }

        std::cout << "\nFetching Bob's details:\n";
        nlohmann::json bob = db.query_single("SELECT * FROM employee WHERE name = 'Bob'");
        if (!bob.empty()) {
            std::cout << "Bob works in " << bob["department"].get<std::string>() 
                      << " and makes $" << bob["salary"].get<std::string>() << "\n";
        }

        db.execute("UPDATE employee SET salary = 95000 WHERE department = 'Engineering'");
        std::cout << "\nUpdated salaries. Rows affected: " << db.get_rows_affected() << "\n";

        //db.execute("DROP TABLE employee");

    } 
    catch (const std::exception& e) {
        std::cerr << "\nDATABASE ERROR: " << e.what() << std::endl;
    }

    return 0;
}
 
 * 
 * */