#include <SQLiteCpp/SQLiteCpp.h>
#include <iostream>
#include <string>
#include <vector>
#include <format>  // C++20 format library

// Helper function to display query results
void displayResults(const SQLite::Statement& query) {
    // Get column count
    const int columnCount = query.getColumnCount();
    
    // Display column headers
    for (int i = 0; i < columnCount; ++i) {
        std::cout << std::format("{:<15}", query.getColumnName(i)) << " | ";
    }
    std::cout << "\n" << std::string(columnCount * 17, '-') << "\n";
    
    // Display rows
    while (query.executeStep()) {
        for (int i = 0; i < columnCount; ++i) {
            std::cout << std::format("{:<15}", query.getColumn(i).getString()) << " | ";
        }
        std::cout << "\n";
    }
    std::cout << "\n";
}

int main() {
    try {
        // 1. Connect to database (creates if it doesn't exist)
        SQLite::Database db("/home/ronnieji/ronnieji/lib/MLCpplib-main/example.db", SQLite::OPEN_READWRITE | SQLite::OPEN_CREATE);
        std::cout << "SQLite database opened/created successfully\n";
        
        // Create users table if it doesn't exist
        db.exec(R"(
            CREATE TABLE IF NOT EXISTS users (
                Name TEXT NOT NULL,
                Age INTEGER,
                Address TEXT,
                PhoneNumber TEXT
            )
        )");
        
        // 2. SELECT queries
        std::cout << "\nAll users:\n";
        SQLite::Statement query1(db, "SELECT * FROM users");
        displayResults(query1);
        
        std::cout << "Users over 20 (Name and PhoneNumber only):\n";
        SQLite::Statement query2(db, "SELECT Name, PhoneNumber FROM users WHERE Age > 20");
        displayResults(query2);
        
        // 3. UPDATE query
        {
            SQLite::Statement update(db, "UPDATE users SET Age = 20 WHERE PhoneNumber = ?");
            update.bind(1, "1234");
            int changes = update.exec();
            std::cout << std::format("UPDATE: {} rows affected\n", changes);
        }
        
        // 4. DELETE query with multiple conditions
        {
            SQLite::Statement del(db, "DELETE FROM users WHERE Name = ? AND PhoneNumber = ?");
            del.bind(1, "Jack");
            del.bind(2, "1234");
            int changes = del.exec();
            std::cout << std::format("DELETE (specific user): {} rows affected\n", changes);
        }
        
        // 5. DELETE query with condition
        {
            SQLite::Statement del(db, "DELETE FROM users WHERE Age > ?");
            del.bind(1, 30);
            int changes = del.exec();
            std::cout << std::format("DELETE (age > 30): {} rows affected\n", changes);
        }
        
        // 6. INSERT query
        {
            SQLite::Statement insert(db, 
                "INSERT INTO users(Name, Age, Address, PhoneNumber) VALUES(?, ?, ?, ?)");
            insert.bind(1, "Ron");
            insert.bind(2, 12);
            insert.bind(3, "57 Main str.");
            insert.bind(4, "54321");
            int changes = insert.exec();
            std::cout << std::format("INSERT: {} rows affected\n", changes);
        }
        
        // Display final state
        std::cout << "\nFinal users list:\n";
        SQLite::Statement finalQuery(db, "SELECT * FROM users");
        displayResults(finalQuery);
        
    } catch (const SQLite::Exception& e) {
        std::cerr << "SQLite exception: " << e.what() << "\n";
        return 1;
    } catch (const std::exception& e) {
        std::cerr << "General exception: " << e.what() << "\n";
        return 1;
    }
    
    return 0;
}