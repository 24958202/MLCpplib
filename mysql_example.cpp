
/*

input sql string contains digit variables:

std::stringstream ddsql;
ddsql << "delete from nlp_db.bag_of_words where corpus_id=" << max_doc_id;
if (db.executeQuery(ddsql.str().c_str())){
       std::cout << "Successfully remove the data into db!" << '\n';
 }

*/
/*
	g++ '/home/ronnieji/lib/db_tools/mysql_test.cpp' -o '/home/ronnieji/lib/db_tools/mysql_test' -I/usr/local/include/eigen3 -I/usr/local/include -lcurl -lsqlite3 -I/usr/include -DBOOST_BIND_GLOBAL_PLACEHOLDERS /usr/local/lib/libgumbo.so -I/usr/include/ -lmysqlclient /home/ronnieji/lib/lib/mysqldatabase.so /home/ronnieji/lib/lib/nlplib.so -std=c++20
*/
// main.cpp
#include <iostream>
#include <vector>
#include <string>
#include <algorithm>
#include "../lib/mysqldatabase.h"
#include "../lib/nlplib.h"

void insertData(){
	MySQLDatabase db;   
    const char* host = "localhost";
    const char* user = "24958202@qq.com";
    const char* passwd = "7122759";
    const char* dbname = "nlp_db";    
    if (!db.connect(host, user, passwd, dbname)) {
        std::cout << "Error mysql connection" << '\n';
        return;
    }  
	if (db.executeQuery("insert into nlp_db.english_voc(word,word_type,meaning_en,meaning_zh)values('test1','BB','this is a test','test test')")){
		std::cout << "Successfully saved the data into db!" << '\n';
    }
    db.closeConnection();
}
void updateData(){
	MySQLDatabase db;   
    const char* host = "localhost";
    const char* user = "24958202@qq.com";
    const char* passwd = "7122759";
    const char* dbname = "nlp_db";    
    if (!db.connect(host, user, passwd, dbname)) {
        std::cout << "Error mysql connection" << '\n';
        return;
    }  
	if (db.executeQuery("update nlp_db.english_voc set word_type='VV' where word='test1'")){
		std::cout << "Successfully update the data into db!" << '\n';
    }
    db.closeConnection();
}
void deleteData(){
	MySQLDatabase db;   
    const char* host = "localhost";
    const char* user = "24958202@qq.com";
    const char* passwd = "7122759";
    const char* dbname = "nlp_db";    
    if (!db.connect(host, user, passwd, dbname)) {
        std::cout << "Error mysql connection" << '\n';
        return;
    }  
	if (db.executeQuery("delete from nlp_db.english_voc where word='test1'")){
		std::cout << "Successfully delete the data into db!" << '\n';
    }
    db.closeConnection();
}
void selectData(){
	MySQLDatabase db;   
    const char* host = "localhost";
    const char* user = "24958202@qq.com";
    const char* passwd = "7122759";
    const char* dbname = "nlp_db";    
    if (!db.connect(host, user, passwd, dbname)) {
    	std::cout << "Mysql connection error!" << '\n';
        return;
    }  
    const char* query = "SELECT * FROM english_voc where word='test1'";   
    if (db.executeQuery(query)) {
        MYSQL_RES *result = db.fetchResult();
        if (result) {
            MYSQL_ROW row;
            unsigned int num_fields = mysql_num_fields(result);
            std::vector<std::vector<std::string>> data; // Vector to hold the result           
            while ((row = mysql_fetch_row(result))) {
                std::vector<std::string> rowData;
                for (unsigned int i = 0; i < num_fields; i++) {
                    // Add each field in the row to rowData
                    rowData.push_back(row[i] ? row[i] : "");
                }
                // Add the populated rowData to the main data vector
                data.push_back(rowData);
            }  
            // Now you can work with the data vector as needed
            // For example, to print the data:
            if(!data.empty()){
            	for (const auto& row : data) {
					std::cout << "row[1]: " << row[1] << " row[2]: " << row[2] << " row[3]: " << row[3] << " row[4]: " << row[4] << '\n';
				}
            }
            mysql_free_result(result);
        } else {
            std::cerr << "Failed to retrieve result set: " << mysql_error(db.getConnection()) << std::endl;
        }
    } else {
        std::cerr << "Query execution failed." << std::endl;
    }  
    db.closeConnection();
}