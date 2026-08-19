#include "rmysql.h"

// ==========================================
// Constructors & Memory Management
// ==========================================
rmysql::rmysql(const std::string& host, const std::string& user, const std::string& pass, const std::string& db, unsigned int port) {
    conn = mysql_init(nullptr);
    if (!conn) {
        throw std::runtime_error("MySQL Init failed (Out of memory)");
    }
    
    // Enable multi-statements and standard configs
    if (!mysql_real_connect(conn, host.c_str(), user.c_str(), pass.c_str(), db.c_str(), port, nullptr, 0)) {
        std::string err = mysql_error(conn);
        mysql_close(conn);
        throw std::runtime_error("Connection failed: " + err);
    }
}

rmysql::~rmysql() {
    if (conn) {
        mysql_close(conn);
        conn = nullptr;
    }
}

rmysql::rmysql(rmysql&& other) noexcept : conn(other.conn) {
    other.conn = nullptr;
}

rmysql& rmysql::operator=(rmysql&& other) noexcept {
    if (this != &other) {
        if (conn) mysql_close(conn);
        conn = other.conn;
        other.conn = nullptr;
    }
    return *this;
}

// ==========================================
// Core Execution (Fast Direct Querying)
// ==========================================
void rmysql::execute(const std::string& sql) {
    if (mysql_real_query(conn, sql.c_str(), sql.length()) != 0) {
        throw std::runtime_error("SQL error: " + std::string(mysql_error(conn)) + "\nSQL: " + sql);
    }
}

nlohmann::json rmysql::query(const std::string& sql) {
    execute(sql);
    MYSQL_RES* res = mysql_store_result(conn);
    if (!res) {
        if (mysql_field_count(conn) == 0) return nlohmann::json::array(); // It was an UPDATE/INSERT
        throw std::runtime_error("Result fetch error: " + std::string(mysql_error(conn)));
    }

    nlohmann::json results = nlohmann::json::array();
    int num_fields = mysql_num_fields(res);
    MYSQL_FIELD* fields = mysql_fetch_fields(res);

    MYSQL_ROW row;
    while ((row = mysql_fetch_row(res))) {
        nlohmann::json j_row = nlohmann::json::object();
        for (int i = 0; i < num_fields; i++) {
            //j_row[fields[i].name] = row[i] ? std::string(row[i]) : nlohmann::json(nullptr);
			if (row[i] != nullptr) {
				j_row[fields[i].name] = std::string(row[i]);
			} else {
				j_row[fields[i].name] = nullptr;
			}
        }
        results.push_back(j_row);
    }

    mysql_free_result(res);
    return results;
}

nlohmann::json rmysql::query_single(const std::string& sql) {
    // Add LIMIT 1 for efficiency
    nlohmann::json results = query(sql + " LIMIT 1");
    if (results.empty()) return nlohmann::json::object();
    return results[0];
}

// ==========================================
// Transactions
// ==========================================
void rmysql::begin_transaction() { execute("START TRANSACTION;"); }
void rmysql::commit() { execute("COMMIT;"); }
void rmysql::rollback() { execute("ROLLBACK;"); }

// ==========================================
// Metadata (Bypasses JSON for max speed)
// ==========================================
bool rmysql::table_exists(const std::string& table) {
    nlohmann::json result = query_single("SELECT 1 FROM information_schema.tables WHERE table_schema = DATABASE() AND table_name = '" + table + "'");
    return !result.empty();
}

bool rmysql::column_exists(const std::string& table, const std::string& column) {
    nlohmann::json result = query_single("SELECT 1 FROM information_schema.columns WHERE table_schema = DATABASE() AND table_name = '" + table + "' AND column_name = '" + column + "'");
    return !result.empty();
}

uint64_t rmysql::get_last_insert_rowid() {
    return mysql_insert_id(conn);
}

uint64_t rmysql::get_rows_affected() {
    return mysql_affected_rows(conn);
}