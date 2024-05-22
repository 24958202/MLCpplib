#include <iostream>
#include <string>
#include <fstream>
#include "../lib/mysqldatabase.h"
#include "../lib/nemslib.h"
void exportToCSV(const std::vector<Mdatatype>& data, const std::string& filename) {
    std::ofstream file(filename,std::ios::out);
    if(!file.is_open()){
        file.open(filename,std::ios::out);
    }
    else{
        file << "word,word_type,meaning_en,meaning_zh\n";
        for (const auto& entry : data) {
            file << entry.word << "," << entry.word_type << "," << entry.meaning_en << "," << entry.meaning_zh << "\n";
        }
        file.close();
        std::cout << "CSV file exported successfully.\n";
    }
}
std::vector<Mdatatype> getEng_voc(){
    MySQLDatabase db;   
    const char* host = "localhost";
    const char* user = "24958202";
    const char* passwd = "7122759";
    const char* dbname = "nlp_db";  
    std::vector<Mdatatype> vocs;  
    if (!db.connect(host, user, passwd, dbname)) {
    	std::cout << "Mysql connection error!" << '\n';
        return vocs;
    }  
    const char* query = "SELECT * FROM english_voc";   
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
                    Mdatatype mt;
					//std::cout << "row[1]: " << row[1] << " row[2]: " << row[2] << " row[3]: " << row[3] << " row[4]: " << row[4] << '\n';
                    mt.word = row[1];
                    mt.word_type = row[2];
                    mt.meaning_en = row[3];
                    mt.meaning_zh = row[4];
                    vocs.push_back(mt);
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
    return vocs;
}
int main(){
    nlp_lib nl_j;
    std::vector<Mdatatype> getEnglish_voc_clean;
    std::vector<Mdatatype> getEnglish_voc = getEng_voc();//nem_j.readBinaryFile("/home/ronnieji/lib/bk_0506_2024/english_voc.bin"); /home/ronnieji/watchdog/db
    /*
        output the database to *.csv file
    */
    // unsigned int i = 0;
    // for(auto& item : getEnglish_voc){
    //     std::string str_en = jsl_j.trim(item.meaning_en);
    //     std::cout << "index: " << ++i << "   ***  " << item.word << " >>> " << item.word_type << " >>> " << item.meaning_en << " >>> " << item.meaning_zh << '\n';
    //     if(!str_en.empty()){
    //         getEnglish_voc_clean.push_back(item);
    //     }
    // }
    // if(!getEnglish_voc_clean.empty()){
    //      exportToCSV(getEnglish_voc_clean,"/home/ronnieji/watchdog/db/english_voc.csv");
    // }
    /*
        create a binary file
    */
    // if(!getEnglish_voc.empty()){
    //     nl_j.writeBinaryFile(getEnglish_voc,"/home/ronnieji/watchdog/db/english_voc.bin");
    //     std::cout << "Successfully created the english_voc.bin file! at: /home/ronnieji/watchdog/db/english_voc.bin" << '\n';
    // }
    // else{
    //     std::cout << "getEnglish_voc is empty!" << '\n';
    // }
    /*
        read the english_voc.bin file
    */
    getEnglish_voc = nl_j.readBinaryFile("/home/ronnieji/watchdog/db/english_voc.bin"); 
    std::cout << getEnglish_voc.size() << '\n';
    return 0;
}
