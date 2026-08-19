#include "rsqlite.h"

// ==========================================
// Constructors & Destructors
// ==========================================

rsqlite::rsqlite(const std::string& path) : db(nullptr), db_path(path) {
    // sqlite3_open_v2 is safer and more configurable than sqlite3_open
    int flags = SQLITE_OPEN_READWRITE | SQLITE_OPEN_CREATE | SQLITE_OPEN_FULLMUTEX;
    if (sqlite3_open_v2(db_path.c_str(), &db, flags, nullptr) != SQLITE_OK) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_close(db); // Always close to prevent leaks even on failure
        throw std::runtime_error("Can't open database: " + error);
    }
}

rsqlite::~rsqlite() {
    // sqlite3_close_v2 gracefully waits if there are unfinalized statements
    if (db) {
        sqlite3_close_v2(db);
        db = nullptr;
    }
}

// Move constructor
rsqlite::rsqlite(rsqlite&& other) noexcept : db(other.db), db_path(std::move(other.db_path)) {
    other.db = nullptr;
}

// Move assignment
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
// Callbacks
// ==========================================

int rsqlite::callback(void* data, int argc, char** argv, char** col_name) {
    auto* results = static_cast<nlohmann::json*>(data);
    nlohmann::json row;
    for (int i = 0; i < argc; i++) {
        row[col_name[i]] = argv[i] ? argv[i] : nullptr; // Use JSON null instead of empty string
    }
    results->push_back(row);
    return 0;
}

int rsqlite::single_callback(void* data, int argc, char** argv, char** col_name) {
    auto* result = static_cast<nlohmann::json*>(data);
    for (int i = 0; i < argc; i++) {
        (*result)[col_name[i]] = argv[i] ? argv[i] : nullptr;
    }
    return 0;
}

// ==========================================
// Core Execution
// ==========================================

void rsqlite::execute(const std::string& sql) {
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), nullptr, nullptr, &err_msg) != SQLITE_OK) {
        std::string error = "SQL error: " + std::string(err_msg ? err_msg : "");
        sqlite3_free(err_msg);
        throw std::runtime_error(error + "\nSQL was: " + sql);
    }
}

nlohmann::json rsqlite::query(const std::string& sql) {
    nlohmann::json results = nlohmann::json::array();
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), callback, &results, &err_msg) != SQLITE_OK) {
        std::string error = "SQL query error: " + std::string(err_msg ? err_msg : "");
        sqlite3_free(err_msg);
        throw std::runtime_error(error + "\nSQL was: " + sql);
    }
    return results;
}

nlohmann::json rsqlite::query_single(const std::string& sql) {
    nlohmann::json result = nlohmann::json::object();
    char* err_msg = nullptr;
    if (sqlite3_exec(db, sql.c_str(), single_callback, &result, &err_msg) != SQLITE_OK) {
        std::string error = "SQL query error: " + std::string(err_msg ? err_msg : "");
        sqlite3_free(err_msg);
        throw std::runtime_error(error + "\nSQL was: " + sql);
    }
    return result;
}

// ==========================================
// Prepared Statements (Anti-Injection)
// ==========================================

void rsqlite::execute_prepared(const std::string& sql, const std::vector<std::string>& params) {
    sqlite3_stmt* stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql.c_str(), -1, &stmt, nullptr) != SQLITE_OK) {
        throw std::runtime_error("Failed to prepare statement: " + std::string(sqlite3_errmsg(db)));
    }

    // Bind parameters (SQLite index starts at 1)
    for (size_t i = 0; i < params.size(); ++i) {
        sqlite3_bind_text(stmt, i + 1, params[i].c_str(), -1, SQLITE_TRANSIENT);
    }

    if (sqlite3_step(stmt) != SQLITE_DONE) {
        std::string error = sqlite3_errmsg(db);
        sqlite3_finalize(stmt);
        throw std::runtime_error("Failed to execute prepared statement: " + error);
    }

    sqlite3_finalize(stmt);
}

// ==========================================
// Transactions
// ==========================================

void rsqlite::begin_transaction() { execute("BEGIN TRANSACTION;"); }
void rsqlite::commit() { execute("COMMIT;"); }
void rsqlite::rollback() { execute("ROLLBACK;"); }

// ==========================================
// Utility & Metadata
// ==========================================

bool rsqlite::table_exists(const std::string& table) {
    std::string sql = "SELECT name FROM sqlite_master WHERE type='table' AND name='" + table + "';";
    nlohmann::json result = query_single(sql);
    return !result.empty();
}

bool rsqlite::column_exists(const std::string& table, const std::string& column) {
    std::string sql = "PRAGMA table_info(" + table + ");";
    nlohmann::json result = query(sql);
    
    for (const auto& row : result) {
        if (row["name"] == column) return true;
    }
    return false;
}

int64_t rsqlite::get_last_insert_rowid() {
    return sqlite3_last_insert_rowid(db);
}

int rsqlite::get_rows_affected() {
    return sqlite3_changes(db);
}