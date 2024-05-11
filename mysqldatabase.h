#ifndef MYSQLDATABASE_H
#define MYSQLDATABASE_H

#include <mysql/mysql.h>
#include <string>

class MySQLDatabase {
public:
    MySQLDatabase();
    ~MySQLDatabase();

    bool connect(const char* host, const char* user, const char* passwd, const char* dbname, unsigned int port = 3306);
    bool executeQuery(const char* query);
    MYSQL_RES* fetchResult();
    void closeConnection();
    MYSQL* getConnection() const;
    bool executeNoneReturns(const std::string& query);

private:
    MYSQL *connection;
    bool isConnected;
};

#endif // MYSQLDATABASE_H
