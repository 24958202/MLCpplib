#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <vector>
#include <mysql/mysql.h>

// Function to escape special characters
std::string escape_special_characters(const std::string& input_string) {
    std::string escaped_string;
    for (char c : input_string) {
        switch (c) {
            case '\'':
                escaped_string += "\\'";
                break;
            case '"':
                escaped_string += "\\\"";
                break;
            case '\\':
                escaped_string += "\\\\";
                break;
            case '\n':
                escaped_string += "\\n";
                break;
            case '\r':
                escaped_string += "\\r";
                break;
            case '\t':
                escaped_string += "\\t";
                break;
            case '\b':
                escaped_string += "\\b";
                break;
            case '\0':
                escaped_string += "\\0";
                break;
            default:
                escaped_string += c;
                break;
        }
    }
    return escaped_string;
}

// Function to connect to MySQL
bool connect_to_mysql(MYSQL* conn) {
    mysql_init(conn);
    if (!mysql_real_connect(conn, "localhost", "24958202", "7122759", "nlp_db", 0, NULL, 0)) {
        std::cerr << "Error connecting to MySQL database: " << mysql_error(conn) << std::endl;
        return false;
    }
    std::cout << "Connected to MySQL database" << std::endl;
    return true;
}

// Function to read CSV file
std::vector<std::vector<std::string>> read_csv_file(const std::string& filename) {
    std::ifstream file(filename);
    std::vector<std::vector<std::string>> data;
    std::string line;
    while (std::getline(file, line)) {
        std::istringstream iss(line);
        std::vector<std::string> row;
        std::string cell;
        while (std::getline(iss, cell, ',')) {
            row.push_back(cell);
        }
        data.push_back(row);
    }
    return data;
}
// Function to insert data into MySQL
void insert_data_into_mysql(MYSQL* conn, const std::vector<std::vector<std::string>>& data) {
    for (size_t i = 1; i < data.size(); ++i) {  // Skip header row
        if (data[i].size() < 3) {
            std::cerr << "Skipping row due to insufficient columns: ";
            for (const std::string& cell : data[i]) {
                std::cerr << cell << " ";
            }
            std::cerr << std::endl;
            continue;
        }
        std::string word = data[i][0];
        std::string word_type = data[i][1];
        std::string english = data[i][2];
        std::string zh;
        if (data[i].size() >= 4) {
            zh = data[i][3];
        } else {
            zh = "";  // Set default value for 'meaning_zh' if it's empty
        }
        word = escape_special_characters(word);
        word_type = escape_special_characters(word_type);
        english = escape_special_characters(english);
        zh = escape_special_characters(zh);

        std::string query = "SELECT 1 FROM english_voc WHERE word='" + word + "'";
        if (mysql_query(conn, query.c_str()) != 0) {
            std::cerr << "Error checking if word exists: " << mysql_error(conn) << std::endl;
        } else {
            MYSQL_RES* res;
            res = mysql_store_result(conn);
            if (res) {
                int num_rows = mysql_num_rows(res);
                if (num_rows > 0) {
                    std::cout << "Word '" << word << "' already exists in the table." << std::endl;
                    // You can choose to skip inserting the word or update the existing row
                    continue;
                } else {
                    std::cout << "Word '" << word << "' does not exist in the table. Inserting..." << std::endl;
                    // Insert the word into the table
                    std::string insert_query = "INSERT INTO english_voc (word, word_type, meaning_en, meaning_zh) VALUES ('" + word + "', '" + word_type + "', '" + english + "', '" + zh + "')";
                    if (mysql_query(conn, insert_query.c_str())) {
                        std::cerr << "Error inserting data into MySQL database: " << mysql_error(conn) << std::endl;
                    }
                }
                mysql_free_result(res);
            } else {
                std::cerr << "Error storing result: " << mysql_error(conn) << std::endl;
            }
        }
    }
    mysql_close(conn);
    std::cout << "Data successfully inserted into MySQL database!" << std::endl;
}
int main() {
    std::string filename = "/home/ronnieji/watchdog/db/complete_db_export.csv";
    MYSQL conn;
    if (connect_to_mysql(&conn)) {
        std::vector<std::vector<std::string>> data = read_csv_file(filename);
        insert_data_into_mysql(&conn, data);
    }
    return 0;
}