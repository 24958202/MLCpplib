/*
	g++ -c -fPIC /Volumes/WorkDisk/cpp_test/db_tools/lib/mysqldatabase.cpp -o /Volumes/WorkDisk/cpp_test/db_tools/lib/mysqldatabase.o -I/opt/homebrew/opt/mysql/include -L/opt/homebrew/opt/mysql/lib -lmysqlclient-std=c++20
	g++ -shared -o /Volumes/WorkDisk/cpp_test/db_tools/lib/mysqldatabase.a /Volumes/WorkDisk/cpp_test/db_tools/lib/mysqldatabase.o -I/opt/homebrew/opt/mysql/include -L/opt/homebrew/opt/mysql/lib -lmysqlclient -std=c++20

    g++ -c -fPIC '/home/ronnieji/lib/lib/mysqldatabase.cpp' -o '/home/ronnieji/lib/lib/mysqldatabase.o' -I/usr/include/ -lmysqlclient -std=c++20
    g++ -shared -o '/home/ronnieji/lib/lib/mysqldatabase.so' '/home/ronnieji/lib/lib/mysqldatabase.o' -I/usr/include/ -lmysqlclient -std=c++20

    g++ -c -fPIC '/home/ronnieji/lib/lib/mysqldatabase.cpp' -o '/home/ronnieji/lib/lib/mysqldatabase.o' -I/usr/include/ -lmysqlclient -std=c++20
    g++ -shared -o '/home/ronnieji/lib/lib/mysqldatabase.so' '/home/ronnieji/lib/lib/mysqldatabase.o' -I/usr/include/ -lmysqlclient -std=c++20
	
*/
// MySQLDatabase.cpp
#include "mysqldatabase.h"
#include <iostream>

MySQLDatabase::MySQLDatabase() : connection(nullptr), isConnected(false) {
    connection = mysql_init(nullptr);
    if (connection == nullptr) {
        std::cerr << "MySQL initialization failed." << std::endl;
    }
}

MySQLDatabase::~MySQLDatabase() {
    closeConnection();
}

bool MySQLDatabase::connect(const char* host, const char* user, const char* passwd, const char* dbname, unsigned int port) {
    if (!mysql_real_connect(connection, host, user, passwd, dbname, port, nullptr, 0)) {
        std::cerr << "Database connection failed: " << mysql_error(connection) << std::endl;
        return false;
    }
    isConnected = true;
    return true;
}

bool MySQLDatabase::executeQuery(const char* query) {
    if (!isConnected) {
        std::cerr << "Execute query attempted without an active connection." << std::endl;
        return false;
    }
    if (mysql_query(connection, query)) {
        std::cerr << "MySQL query failed: " << mysql_error(connection) << std::endl;
        return false;
    }
    return true;
}

MYSQL_RES* MySQLDatabase::fetchResult() {
    if (!isConnected) {
        std::cerr << "Fetch result attempted without an active connection." << std::endl;
        return nullptr;
    }
    return mysql_store_result(connection);
}

void MySQLDatabase::closeConnection() {
    if (connection != nullptr) {
        mysql_close(connection);
        connection = nullptr;
        isConnected = false;
    }
}

MYSQL* MySQLDatabase::getConnection() const {
    return connection;
}

// Uncommented and fixed the executeNoneReturns function
bool MySQLDatabase::executeNoneReturns(const std::string& query) {
    if (!isConnected) {
        std::cerr << "Execute query attempted without an active connection." << std::endl;
        return false;
    }
    if (mysql_query(connection, query.c_str())) {
        std::cerr << "MySQL query error: " << mysql_error(connection) << std::endl;
        return false;
    }
    my_ulonglong num_affected_rows = mysql_affected_rows(connection);
    if (num_affected_rows == (my_ulonglong)-1) {
        std::cerr << "MySQL error in fetching affected rows: " << mysql_error(connection) << std::endl;
        return false;
    }
    std::cout << "Number of affected rows: " << num_affected_rows << std::endl;
    return true;
}

