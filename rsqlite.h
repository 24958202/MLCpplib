#ifndef RSQLITE_H
#define RSQLITE_H

#include <string>
#include <vector>
#include <stdexcept>
#include <iostream>
#include <sqlite3.h>
#include <nlohmann/json.hpp> // Assuming you are using nlohmann/json

class rsqlite {
private:
    sqlite3* db;
    std::string db_path;

    // Callbacks MUST be static to interface with the SQLite C-API
    static int callback(void* data, int argc, char** argv, char** col_name);
    static int single_callback(void* data, int argc, char** argv, char** col_name);

public:
    explicit rsqlite(const std::string& path);
    ~rsqlite();

    // Prevent copying (Crucial for database connections to avoid double-free crashes)
    rsqlite(const rsqlite&) = delete;
    rsqlite& operator=(const rsqlite&) = delete;

    // Allow moving
    rsqlite(rsqlite&& other) noexcept;
    rsqlite& operator=(rsqlite&& other) noexcept;

    // --- Core Execution ---
    void execute(const std::string& sql);
    nlohmann::json query(const std::string& sql);
    nlohmann::json query_single(const std::string& sql);

    // --- Real-World Additions: Prepared Statements (Anti-SQL Injection) ---
    // Safely bind parameters instead of concatenating strings
    void execute_prepared(const std::string& sql, const std::vector<std::string>& params);
    nlohmann::json query_prepared(const std::string& sql, const std::vector<std::string>& params);

    // --- Real-World Additions: Transactions ---
    void begin_transaction();
    void commit();
    void rollback();

    // --- Real-World Additions: Metadata & Utility ---
    bool table_exists(const std::string& table);
    bool column_exists(const std::string& table, const std::string& column);
    int64_t get_last_insert_rowid();
    int get_rows_affected();
};

#endif