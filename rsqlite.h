#ifndef RSQLITE_H
#define RSQLITE_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sqlite3.h>
#include <nlohmann/json.hpp>

class rsqlite {
private:
    sqlite3* db;
    std::string db_path;

    // Static callbacks for standard query()
    static int callback(void* data, int argc, char** argv, char** col_name);
    static int single_callback(void* data, int argc, char** argv, char** col_name);

    // --- TEMPLATE BINDING HELPERS ---
    void bind_value(sqlite3_stmt* stmt, int index, int value) {
        sqlite3_bind_int(stmt, index, value);
    }
    void bind_value(sqlite3_stmt* stmt, int index, double value) {
        sqlite3_bind_double(stmt, index, value);
    }
    void bind_value(sqlite3_stmt* stmt, int index, const std::string& value) {
        sqlite3_bind_text(stmt, index, value.c_str(), -1, SQLITE_TRANSIENT);
    }
    void bind_value(sqlite3_stmt* stmt, int index, const char* value) {
        sqlite3_bind_text(stmt, index, value, -1, SQLITE_TRANSIENT);
    }
    void bind_value(sqlite3_stmt* stmt, int index, std::nullptr_t) {
        sqlite3_bind_null(stmt, index);
    }

    // Base case for template recursion
    void bind_all(sqlite3_stmt* stmt, int index) {}

    // Variadic template to unpack arguments safely
    template<typename T, typename... Args>
    void bind_all(sqlite3_stmt* stmt, int index, T first, Args... rest) {
        bind_value(stmt, index, first);     
        bind_all(stmt, index + 1, rest...); 
    }

public:
    // Constructors & Rule of 5
    explicit rsqlite(const std::string& path);
    ~rsqlite();
    rsqlite(const rsqlite&) = delete;
    rsqlite& operator=(const rsqlite&) = delete;
    rsqlite(rsqlite&& other) noexcept;
    rsqlite& operator=(rsqlite&& other) noexcept;

    // Core Execution
    void execute(const std::string& sql);
    nlohmann::json query(const std::string& sql);
    nlohmann::json query_single(const std::string& sql);

    // Transactions
    void begin_transaction();
    void commit();
    void rollback();

    // Fast Metadata (Bypasses JSON for max speed)
    bool table_exists(const std::string& table);
    bool column_exists(const std::string& table, const std::string& column);
    int64_t get_last_insert_rowid();
    int get_rows_affected();

    // --- VARIADIC TEMPLATE PREPARED STATEMENTS ---
    template<typename... Args>
    void execute_prepared(const std::string& sql, Args... args) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
        }

        bind_all(stmt, 1, args...);

        if (sqlite3_step(stmt) != SQLITE_DONE) {
            std::string error = sqlite3_errmsg(db);
            sqlite3_finalize(stmt);
            throw std::runtime_error("Failed to execute prepared statement: " + error);
        }
        sqlite3_finalize(stmt);
    }

    template<typename... Args>
    nlohmann::json query_prepared(const std::string& sql, Args... args) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
        }

        bind_all(stmt, 1, args...);

        nlohmann::json results = nlohmann::json::array();
        
        while (sqlite3_step(stmt) == SQLITE_ROW) {
            nlohmann::json row = nlohmann::json::object();
            int cols = sqlite3_column_count(stmt);
            for (int i = 0; i < cols; i++) {
                const char* col_name = sqlite3_column_name(stmt, i);
                const char* val = reinterpret_cast<const char*>(sqlite3_column_text(stmt, i));
                row[col_name] = val ? std::string(val) : nullptr;
            }
            results.push_back(row);
        }
        
        sqlite3_finalize(stmt);
        return results;
    }
};

#endif

/*
 How to use it:

 #include <iostream>
#include <string>
#include <vector>
#include "rsqlite.h"

// Note: It's better practice to pass the database by reference 
// rather than re-opening it inside every function.
std::vector<std::string> getName(rsqlite& db) {
    std::vector<std::string> names;
    
    // Use the class's query() method. It handles the callbacks automatically!
    std::string sql = "SELECT name FROM employee";
    nlohmann::json results = db.query(sql);
    
    for (const auto& row : results) {
        // Since we mapped empty SQLite values to JSON nulls in our refactor, 
        // we should check for null before converting to string.
        if (!row["name"].is_null()) {
            names.push_back(row["name"].get<std::string>());
        }
    }
    return names;
}

int main() {
    try {
        // 1. Initialize Database
        std::cout << "Opening database...\n";
        rsqlite db("/Users/jidengfeng/empty.db");

        // 2. execute(): Create a table
        db.execute("CREATE TABLE IF NOT EXISTS employee ("
                   "id INTEGER PRIMARY KEY AUTOINCREMENT, "
                   "name TEXT, "
                   "department TEXT, "
                   "salary REAL);");

        // 3. Metadata: Check tables and columns
        if (db.table_exists("employee")) {
            std::cout << "Success: Table 'employee' exists.\n";
        }
        
        // This is safe to uncomment now! The new C-API implementation prevents crashes.
        if (db.column_exists("employee", "salary")) {
            std::cout << "Success: Column 'salary' exists.\n";
        }

        // 4. Transactions & Prepared Statements (Safe Inserts)
        std::cout << "\nInserting data safely...\n";
        db.begin_transaction(); 
        
        // We use ? placeholders. This protects against SQL injection!
        std::string insert_sql = "INSERT INTO employee (name, department, salary) VALUES (?, ?, ?)";
        
        // NO CURLY BRACES! Pass arguments directly. Notice we can pass numbers safely now.
        db.execute_prepared(insert_sql, "Alice", "Engineering", 85000.50);
        
        // 5. get_last_insert_rowid(): Get Alice's new ID
        std::cout << "Inserted Alice. Her ID is: " << db.get_last_insert_rowid() << "\n";
        
        db.execute_prepared(insert_sql, "Bob", "Marketing", 75000.0);
        db.execute_prepared(insert_sql, "Charlie", "Engineering", 92000.0);
        
        db.commit(); // Save all inserts at once (much faster!)

        // 6. query(): Fetching multiple rows
        std::cout << "\nFetching all employee names:\n";
        auto strNames = getName(db);
        for(const auto& name : strNames) {
            std::cout << "- " << name << "\n";
        }

        // 7. query_single(): Fetching exactly one row
        std::cout << "\nFetching Bob's details:\n";
        nlohmann::json bob = db.query_single("SELECT * FROM employee WHERE name = 'Bob'");
        if (!bob.empty()) {
            std::cout << "Bob works in " << bob["department"].get<std::string>() 
                      << " and makes $" << bob["salary"].get<std::string>() << "\n";
        }

        // 8. get_rows_affected(): Updating data
        db.execute("UPDATE employee SET salary = 95000 WHERE department = 'Engineering'");
        std::cout << "\nUpdated salaries. Rows affected: " << db.get_rows_affected() << "\n";

        // Clean up data for the next run (Optional)
        db.execute("DROP TABLE employee");

    } 
    catch (const std::exception& e) {
        // If anything fails (bad SQL, file permissions), it safely lands here
        std::cerr << "\nDATABASE ERROR: " << e.what() << std::endl;
    }

    return 0;
}
  
 
*/