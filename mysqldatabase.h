// MySQLDatabase.h
#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H

#include <mysql/mysql.h>
#include <string>
class MySQLDatabase {
public:
    MySQLDatabase();
    ~MySQLDatabase();
    MYSQL *connection;
    bool isConnected;
    bool connect(const char* host, const char* user, const char* passwd, const char* dbname);
    bool executeQuery(const char* query);
    MYSQL_RES* fetchResult();
    void closeConnection();
    MYSQL* getConnection() const;
};
#endif // MYSQLDATABASE_H
