#include <iostream>
#include <string>
#include <string_view>
#include <cstdlib>
#include <optional>
#include <vector>
#include <map>
#include <sstream>
#include <fstream>
#include <stdexcept>
#include <iomanip>

// Include SQLite3 C API header
#include <sqlite3.h>

// Function to safe-guard against null pointers from database
std::string safe_text(const unsigned char* text) {
    return text ? std::string(reinterpret_cast<const char*>(text)) : "";
}
// --- Database Wrapper Class ---
class Database {
private:
    sqlite3* db = nullptr;
public:
    Database(const std::string& db_file) {
        if (sqlite3_open(db_file.c_str(), &db) != SQLITE_OK) {
            throw std::runtime_error("Can't open database: " + std::string(sqlite3_errmsg(db)));
        }
        // Enable foreign keys
        exec("PRAGMA foreign_keys = ON;");
    }
    ~Database() {
        if (db) {
            sqlite3_close(db);
        }
    }
    // Execute a simple SQL statement
    void exec(const std::string& sql) {
        char* err_msg = nullptr;
        if (sqlite3_exec(db, sql.c_str(), 0, 0, &err_msg) != SQLITE_OK) {
            std::string err = "SQL error: " + std::string(err_msg);
            sqlite3_free(err_msg);
            throw std::runtime_error(err);
        }
    }
    // Prepare a statement
    sqlite3_stmt* prepare(const std::string& sql) {
        sqlite3_stmt* stmt = nullptr;
        if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, 0) != SQLITE_OK) {
            throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
        }
        return stmt;
    }
    sqlite3* get_native_handle() { return db; }
};
void send_error(const std::string& strErr){
    std::cout << strErr << std::endl;
}
void send_success(const std::string& strSuccess){
    std::cout << strSuccess << std::endl;
}

// --- Configuration ---
const std::string DB_FILE = "/home/ronnieji/db/test.db";
Database db(DB_FILE);

void selectDB(){
    std::string sql = "SELECT * FROM Employees";
    sqlite3_stmt* stmt = db.prepare(sql);
    // sqlite3_step returns SQLITE_ROW for each row found, and SQLITE_DONE when finished
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // 4. Extract column data
        // Note: Column indices start at 0 matching the SELECT order (or table structure)
        // id, name, email, phone, start_date, end_date, remind_email, remind_sms, remind_phone
        int id = sqlite3_column_int(stmt, 0);
        std::string name = safe_text(sqlite3_column_text(stmt, 1));
        std::string phonenum = safe_text(sqlite3_column_text(stmt, 2));
        int age = sqlite3_column_int(stmt, 3);
        // 5. Print using std::cout
        std::cout << "ID: " << id << "\n"
                  << "  Name:  " << name << "\n"
                  << "  Phone: " << phonenum << "\n"
                  << "  Age: " << age << "\n"
                  << "\n" << std::string(40, '-') << std::endl;
    }
    // 6. Cleanup
    sqlite3_finalize(stmt);
}
void insertDB(){
    std::string sql = "INSERT INTO Employees (Name, PhoneNum, Age) VALUES (?, ?, ?);";
    sqlite3_stmt* stmt = db.prepare(sql);
    sqlite3_bind_text(stmt, 1, "Jack", -1, SQLITE_STATIC);
    sqlite3_bind_text(stmt, 2, "13333333333", -1, SQLITE_STATIC);
    sqlite3_bind_int(stmt, 3, 12);
//    sqlite3_bind_text(stmt, 4, start_date->c_str(), -1, SQLITE_STATIC);
//    sqlite3_bind_text(stmt, 5, end_date->c_str(), -1, SQLITE_STATIC);
//    sqlite3_bind_int(stmt, 6, *remind_email);
//    sqlite3_bind_int(stmt, 7, *remind_sms);
//    sqlite3_bind_int(stmt, 8, *remind_phone);

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string err = "Failed to add member: " + std::string(sqlite3_errmsg(db.get_native_handle()));
        sqlite3_finalize(stmt);
        send_error(err);
    } else {
        sqlite3_finalize(stmt);
        send_success("Successfully saved the employee to db!");
    }
}
void updateDB(){
    std::string sql = "update Employees set Age = ? where Name = ?;";
    sqlite3_stmt* stmt = db.prepare(sql);
    sqlite3_bind_int(stmt, 1, 22);
    sqlite3_bind_text(stmt, 2, "Jack", -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string err = "Failed to update the employee: " + std::string(sqlite3_errmsg(db.get_native_handle()));
        sqlite3_finalize(stmt);
        send_error(err);
    } else {
        sqlite3_finalize(stmt);
        send_success("Successfully updated the employee's info!");
    }
}
void deleteDB(){
    std::string sql = "delete from Employees where Name = ?;";
    sqlite3_stmt* stmt = db.prepare(sql);
    sqlite3_bind_text(stmt, 1, "Jack", -1, SQLITE_STATIC);
    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string err = "Failed to delete the employee: " + std::string(sqlite3_errmsg(db.get_native_handle()));
        sqlite3_finalize(stmt);
        send_error(err);
    } else {
        sqlite3_finalize(stmt);
        send_success("Successfully updated the employee's info!");
    }
}
int main(){
    try{
        // Initialize database
        selectDB();
        insertDB();
        updateDB();
        deleteDB();
    } catch (const std::exception& e) {
        send_error("An unexpected server error occurred: " + std::string(e.what()));
    } catch (...) {
        send_error("An unknown server error occurred.");
    }
}