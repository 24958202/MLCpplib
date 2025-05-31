
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sqlite3.h>

// Helper function to display query results
void display_results(sqlite3_stmt *stmt) {
    int col_count = sqlite3_column_count(stmt);

    // Print column headers
    for (int i = 0; i < col_count; ++i) {
        printf("%-15s | ", sqlite3_column_name(stmt, i));
    }
    printf("\n");
    for (int i = 0; i < col_count * 17; ++i) printf("-");
    printf("\n");

    // Print rows
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        for (int i = 0; i < col_count; ++i) {
            const char *val = (const char *)sqlite3_column_text(stmt, i);
            printf("%-15s | ", val ? val : "NULL");
        }
        printf("\n");
    }
    printf("\n");
    // Reset statement for possible reuse
    sqlite3_reset(stmt);
}

int main() {
    sqlite3 *db;
    char *errmsg = NULL;
    int rc;
	
    // 1. Open database
    rc = sqlite3_open("/home/ronnieji/ronnieji/lib/MLCpplib-main/example.db", &db);
    if (rc) {
        fprintf(stderr, "Can't open database: %s\n", sqlite3_errmsg(db));
        return 1;
    }
    printf("SQLite database opened/created successfully\n");
	
    // 2. Create users table if not exists
    const char *create_sql =
        "CREATE TABLE IF NOT EXISTS users ("
        "Name TEXT NOT NULL,"
        "Age INTEGER,"
        "Address TEXT,"
        "PhoneNumber TEXT"
        ");";
    rc = sqlite3_exec(db, create_sql, 0, 0, &errmsg);
    if (rc != SQLITE_OK) {
        fprintf(stderr, "SQL error: %s\n", errmsg);
        sqlite3_free(errmsg);
        sqlite3_close(db);
        return 1;
    }
	
    // 3. SELECT * FROM users
    printf("\nAll users:\n");
    sqlite3_stmt *stmt;
    rc = sqlite3_prepare_v2(db, "SELECT * FROM users", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        display_results(stmt);
    }
    sqlite3_finalize(stmt);

    // 4. SELECT Name, PhoneNumber FROM users WHERE Age > 20
    printf("Users over 20 (Name and PhoneNumber only):\n");
    rc = sqlite3_prepare_v2(db, "SELECT Name, PhoneNumber FROM users WHERE Age > 20", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        display_results(stmt);
    }
    sqlite3_finalize(stmt);

    // 5. UPDATE users SET Age = 20 WHERE PhoneNumber = ?
    rc = sqlite3_prepare_v2(db, "UPDATE users SET Age = 20 WHERE PhoneNumber = ?", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, "54321", -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        int changes = sqlite3_changes(db);
        printf("UPDATE: %d rows affected\n", changes);
    }
    sqlite3_finalize(stmt);

    // 6. DELETE FROM users WHERE Name = ? AND PhoneNumber = ?
    rc = sqlite3_prepare_v2(db, "DELETE FROM users WHERE Name = ? AND PhoneNumber = ?", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, "Jack", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 2, "1234", -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        int changes = sqlite3_changes(db);
        printf("DELETE (specific user): %d rows affected\n", changes);
    }
    sqlite3_finalize(stmt);

    // 7. DELETE FROM users WHERE Age > ?
    rc = sqlite3_prepare_v2(db, "DELETE FROM users WHERE Age > ?", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_int(stmt, 1, 12);
        rc = sqlite3_step(stmt);
        int changes = sqlite3_changes(db);
        printf("DELETE (age > 30): %d rows affected\n", changes);
    }
    sqlite3_finalize(stmt);

    // 8. INSERT INTO users
    rc = sqlite3_prepare_v2(db, "INSERT INTO users(Name, Age, Address, PhoneNumber) VALUES(?, ?, ?, ?)", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        sqlite3_bind_text(stmt, 1, "Ron", -1, SQLITE_STATIC);
        sqlite3_bind_int(stmt, 2, 12);
        sqlite3_bind_text(stmt, 3, "57 Main str.", -1, SQLITE_STATIC);
        sqlite3_bind_text(stmt, 4, "54321", -1, SQLITE_STATIC);
        rc = sqlite3_step(stmt);
        int changes = sqlite3_changes(db);
        printf("INSERT: %d rows affected\n", changes);
    }
    sqlite3_finalize(stmt);

    // 9. Final SELECT * FROM users
    printf("\nFinal users list:\n");
    rc = sqlite3_prepare_v2(db, "SELECT * FROM users", -1, &stmt, NULL);
    if (rc == SQLITE_OK) {
        display_results(stmt);
    }
    sqlite3_finalize(stmt);

    sqlite3_close(db);
    return 0;
}
