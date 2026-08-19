#include "rsqlite.h"

// ==========================================
// Constructors & Memory Management
// ==========================================
rsqlite::rsqlite(const std::string& path) : db(nullptr), db_path(path) {
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(db_path.c_str(), &db, flags, nullptr) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db); 
        throw std::runtime_error("Can't open database: " + error);
    }
}

rsqlite::~rsqlite() {
    if (db) {
        sqlite3_close_v2(db);
        db = nullptr;
    }
}

rsqlite::rsqlite(rsqlite&& other) noexcept : db(other.db), db_path(std::move(other.db_path)) {
    other.db = nullptr;
}

rsqlite& rsqlite::operator=(rsqlite&& other) noexcept {
    if (this != &other) {
        if (db) sqlite3_close_v2(db);
        db = other.db;
        db_path = std::move(other.db_path);
        other.db = nullptr;
    }
    return *this;
}

// ==========================================
// C-Style Callbacks with Exception Safety
// ==========================================
// ==========================================
// C-Style Callbacks with Exception Safety
// ==========================================
int rsqlite::callback(void* data, int argc, char** argv, char** col_name) {
    try {
        auto* results = static_cast<nlohmann::json*>(data);
        nlohmann::json row = nlohmann::json::object();
        for (int i = 0; i < argc; i++) {
            if (argv[i] != nullptr) {
                row[col_name[i]] = std::string(argv[i]);
            } else {
                row[col_name[i]] = nullptr;
            }
        }
        results->push_back(row);
        return 0;
    } 
    catch (...) {
        return 1; // Return non-zero to safely abort SQLite if JSON fails
    }
}

int rsqlite::single_callback(void* data, int argc, char** argv, char** col_name) {
    try {
        auto* result = static_cast<nlohmann::json*>(data);
        for (int i = 0; i < argc; i++) {
            if (argv[i] != nullptr) {
                (*result)[col_name[i]] = std::string(argv[i]);
            } else {
                (*result)[col_name[i]] = nullptr;
            }
        }
        return 0;
    } 
    catch (...) {
        return 1;
    }
}

// ==========================================
// Core Execution
// ==========================================
void rsqlite::execute(const std::string& sql) {
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string error = "SQL error: " + std::string(err_msg ? err_msg : "");
        sqlite3_free(err_msg);
        throw std::runtime_error(error + "\nSQL: " + sql);
    }
}

nlohmann::json rsqlite::query(const std::string& sql) {
    nlohmann::json results = nlohmann::json::array();
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), callback, &results, &err_msg) != SQLITE_OK) {
        std::string error = "SQL query error: " + std::string(err_msg ? err_msg : "");
        sqlite3_free(err_msg);
        throw std::runtime_error(error + "\nSQL: " + sql);
    }
    return results;
}

nlohmann::json rsqlite::query_single(const std::string& sql) {
    nlohmann::json result = nlohmann::json::object();
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), single_callback, &result, &err_msg) != SQLITE_OK) {
        std::string error = "SQL query error: " + std::string(err_msg ? err_msg : "");
        sqlite3_free(err_msg);
        throw std::runtime_error(error + "\nSQL: " + sql);
    }
    return result;
}

// ==========================================
// Transactions
// ==========================================
void rsqlite::begin_transaction() { execute("BEGIN TRANSACTION;"); }
void rsqlite::commit() { execute("COMMIT;"); }
void rsqlite::rollback() { execute("ROLLBACK;"); }

// ==========================================
// High-Efficiency Metadata (No JSON Overhead)
// ==========================================
bool rsqlite::table_exists(const std::string& table) {
    std::string sql = "SELECT 1 FROM sqlite_master WHERE type='table' AND name=?;";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;
    
    sqlite3_bind_text(stmt, 1, table.c_str(), -1, SQLITE_TRANSIENT);
    bool exists = (sqlite3_step(stmt) == SQLITE_ROW);
    
    sqlite3_finalize(stmt);
    return exists;
}

bool rsqlite::column_exists(const std::string& table, const std::string& column) {
    // PRAGMA statements cannot use parameterized bindings for the table name
    std::string sql = "PRAGMA table_info(" + table + ");";
    sqlite3_stmt* stmt = nullptr;
    
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) return false;

    bool found = false;
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        // SQLite PRAGMA table_info returns column name at index 1
        const unsigned char* col_name = sqlite3_column_text(stmt, 1); 
        if (col_name && column == reinterpret_cast<const char*>(col_name)) {
            found = true;
            break;
        }
    }
    
    sqlite3_finalize(stmt);
    return found;
}

int64_t rsqlite::get_last_insert_rowid() {
    return sqlite3_last_insert_rowid(db);
}

int rsqlite::get_rows_affected() {
    return sqlite3_changes(db);
}