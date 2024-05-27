
#include <iostream>
#include <string>
#include <regex>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <functional>
#include <locale>
#include <codecvt>
#include <string_view>
#include <iomanip>
#include <unordered_set>
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <thread>
#include <chrono>
#include <filesystem>
#include <sched.h>
#include <condition_variable>
#include <mutex>
#include <mysql/mysql.h>
#include "../lib/nemslib.h"
#include "../lib/libdict.h"

std::vector<std::string> all_voc;
std::mutex mtx;
std::condition_variable cv;
std::unordered_set<std::string> ProcessFileNames;
// Function to connect to MySQL
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
bool connect_to_mysql(MYSQL* conn) {
    mysql_init(conn);
    if (!mysql_real_connect(conn, "localhost", "24958202", "7122759", "nlp_db", 0, NULL, 0)) {
        std::cerr << "Error connecting to MySQL database: " << mysql_error(conn) << std::endl;
        return false;
    }
    std::cout << "Connected to MySQL database" << std::endl;
    return true;
}
void removeItems(std::vector<std::string>& a, std::vector<std::string>& b) {
    b.erase(std::remove_if(b.begin(), b.end(), [&](const std::string& item) {
        return std::find(a.begin(), a.end(), item) != a.end();
    }), b.end());
}
void add_missed_words(const std::string& word_missed_file_path,const std::string& str_word_missed){
	nlp_lib nl_j;
	if(word_missed_file_path.empty() || str_word_missed.empty()){
		return;
	}
	nl_j.AppendBinaryOne(word_missed_file_path,str_word_missed);
}
std::vector<std::string> get_missed_words(const std::string& word_missed_file_path){
	std::vector<std::string> missed_words;
	nlp_lib nl_j;
	if(word_missed_file_path.empty()){
		return missed_words;
	}
	missed_words = nl_j.ReadBinaryOne(word_missed_file_path);
	return missed_words;
}
void expend_english_voc(const std::string& folder_path){
	/*
		open the english txt folder
	*/
	nemslib nems_j;
	Jsonlib json_j;
	libdict dic_j;
	nlp_lib nl_j;
	SysLogLib syslog_j;
	std::vector<std::string> words_checked;
	std::vector<std::string> words_missed;
	std::vector<std::string> words_not_found;
	//std::vector<std::string> words_story_checked;
	if(folder_path.empty() && all_voc.empty()){
		return;
	}
	/*
		ini s
		put the word checked in the checked list
		remove words_checked,words_missed,words_not_found;
	*/
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/web/english_voc.bin")){
		words_checked = dic_j.get_english_voc_already_checked_in_db("/home/ronnieji/lib/db_tools/web/english_voc.bin");
		removeItems(words_checked,all_voc);
	}
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/web/missed_words.bin")){
		words_missed = get_missed_words("/home/ronnieji/lib/db_tools/web/missed_words.bin");
	}
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/web/words_not_found.bin")){
		words_not_found = get_missed_words("/home/ronnieji/lib/db_tools/web/words_not_found.bin");
		removeItems(words_not_found,all_voc);
	}
	/*
		put the word missed during the disconnection of the network
	*/
	//words_story_checked = get_non_ant_already_checked_in_db();
	std::unordered_map<std::string,std::string> iniWordType;
	if(std::filesystem::exists("/home/ronnieji/lib/lib/res/type_table.bin")){
		std::vector<std::string> word_types_dic = nl_j.ReadBinaryOne("/home/ronnieji/lib/lib/res/type_table.bin");
		if(!word_types_dic.empty()){
			for(const auto& wtd : word_types_dic){
				std::vector<std::string> get_wt = json_j.splitString_bystring(wtd,"^~&");
				if(!get_wt.empty()){
					iniWordType[get_wt[0]] = get_wt[1];
				}
			}
		}
	}
	/*
		read e-books from harddisk
	*/
    for (const auto& av : all_voc) {
        std::string str_msg = "The word we are about to look up: -> ";
		str_msg.append(av);
		syslog_j.writeLog("/home/ronnieji/lib/db_tools/log",str_msg);
		std::string strUrl = "https://dictionary.cambridge.org/dictionary/essential-american-english/";
		strUrl.append(av);
		if(dic_j.check_word_onlineDictionary(
		av,
		iniWordType,
		"/home/ronnieji/lib/db_tools/web/english_voc.bin",
		"/home/ronnieji/lib/db_tools/log"
		)==1){
			words_checked.push_back(av);
		}
		else{//try to look it up at the "https://www.merriam-webster.com/"
			if(dic_j.check_word_onMerriamWebsterDictionary(
				av,
				iniWordType,
				"/home/ronnieji/lib/db_tools/web/english_voc.bin",
				"/home/ronnieji/lib/db_tools/log"
			)==1){
				words_checked.push_back(av);
			}
			else{//this word can not be found online
				add_missed_words("/home/ronnieji/lib/db_tools/web/missed_words.bin",av);
			}
		}
		//dic_j.look_for_past_participle_of_word("home/ronnieji/lib/db_tools/web/english_voc.bin","/home/ronnieji/lib/db_tools/log");
		std::this_thread::sleep_for(std::chrono::seconds(3));//seconds 
	}
	syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","Finish looking up all the words in the dictionary, start checking the parases!");
}
void get_all_voc(const std::string& str_folder_path){
	Jsonlib jsl_j;
	nemslib nem_j;
	SysLogLib syslog_j;
	nlp_lib nl_j;
	std::string strMsg;
	if(str_folder_path.empty()){
		std::cerr << "get_all_voc input empty!" << '\n';
	}
	/*
		get the existing all_voc.bin file
	*/
	std::string str_all_voc_path = "/home/ronnieji/lib/db_tools/web/all_voc.bin";
	if(std::filesystem::exists(str_all_voc_path)){
		all_voc = nl_j.ReadBinaryOne(str_all_voc_path);
	}
	for (const auto& entry : std::filesystem::directory_iterator(str_folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::lock_guard<std::mutex> lock(mtx);
                std::string line;
                while (std::getline(file, line)) {
					if(!line.empty()){
						line = jsl_j.trim(line);
						jsl_j.toLower(line);
						line = nem_j.removeEnglishPunctuation_training(line);
						std::vector<std::string> line_split = nem_j.tokenize_en(line);
						if(!line_split.empty()){
							for(auto& ls : line_split){
								ls = jsl_j.trim(ls);
								if(nem_j.isNumeric(ls) || nem_j.isNonAlphabetic(ls)){
                                    cv.notify_one();
									continue;
								}
								std::cout << "Checking the existance of : " << ls << '\n';
								if(!all_voc.empty()){
									auto it = std::find_if(all_voc.begin(),all_voc.end(),[&ls](const std::string& s){
										return s == ls;
									});
									if(it != all_voc.end()){
										syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","The word's already in the binary file,look for the next...");
                                        cv.notify_one();
										continue;
									}
								}
								all_voc.push_back(ls);
								nl_j.AppendBinaryOne(str_all_voc_path,ls);
								strMsg = "Adding the word: ";
								strMsg.append(ls);
								syslog_j.writeLog("/home/ronnieji/lib/db_tools/log",strMsg);
                                cv.notify_one();                
							}
						}
					}
				}
			}
			else{
				std::cerr << "Failed to open the file!" << '\n';
			}
		}
	 }
	 strMsg = "All words in the binary file! start checking ...";
	 syslog_j.writeLog("/home/ronnieji/lib/db_tools/log",strMsg);
}
void add_wordnet(MYSQL* conn){
	nlp_lib nem_j;
	Jsonlib jsl_j;
	std::cout << "Adding word net into database..." << '\n';
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/webUrls/core-wordnet.bin")){
		std::vector<Mdatatype> word_voc = nem_j.readBinaryFile("/home/ronnieji/lib/db_tools/webUrls/core-wordnet.bin");
		if(!word_voc.empty()){
			for(const auto it = word_voc.begin(); it != word_voc.end();){
				std::string query = "SELECT 1 FROM english_voc WHERE word='" + it->word + "'";
				if (mysql_query(conn, query.c_str()) != 0) {
					std::cerr << "Error checking if word exists: " << mysql_error(conn) << std::endl;
				} else {
					MYSQL_RES* res;
					res = mysql_store_result(conn);
					if (res) {
						int num_rows = mysql_num_rows(res);
						if (num_rows > 0) {
							//std::cout << "Word '" << word << "' already exists in the table." << std::endl;
							// You can choose to skip inserting the word or update the existing row
							continue;
						} else {
							//std::cout << "Word '" << word << "' does not exist in the table. Inserting..." << std::endl;
							// Insert the word into the table
							std::string word = escape_special_characters(it->word);
							std::string word_type = escape_special_characters(it->word_type);
							std::string english = escape_special_characters(it->meaning_en);
							std::string zh = escape_special_characters(it->meaning_zh);
							zh = jsl_j.trim(zh);
							if(zh.empty()){
								zh = "";
							}
							std::string insert_query = "INSERT INTO english_voc(word, word_type, meaning_en, meaning_zh)VALUES('" + word + "', '" + word_type + "', '" + english + "', '" + zh + "')";
							if (mysql_query(conn, insert_query.c_str())) {
								std::cerr << "Error inserting data into MySQL database: " << mysql_error(conn) << std::endl;
							}
							std::cout << "Successfully saved the word: " << word << '\n';
						}
						mysql_free_result(res);
					} else {
						std::cerr << "Error storing result: " << mysql_error(conn) << std::endl;
					}
				}
			}
		}
		else{
			std::cerr << "core-wordnet.bin is empty!" << '\n';
		}
	}
	else{
		std::cerr << "Binary file core-wordnet.bin or english_voc.bin is missing" << '\n';
	}
	std::cout << "Done adding word net into database..." << '\n';
}
void write_checkedTXT(const std::string& str_word){
	std::ofstream file("/home/ronnieji/lib/MLCpplib-main/words_checked",std::ios::app);
	if(!file.is_open()){
		file.open("/home/ronnieji/lib/MLCpplib-main/words_checked",std::ios::app);
	}
	file << str_word << '\n';
	file.close();
}
std::vector<std::string> get_checkedTXT(){
	std::vector<std::string> words_checked;
	Jsonlib jsl;
	std::ifstream file("/home/ronnieji/lib/MLCpplib-main/words_checked");
	if(file.is_open()){
		std::string line;
		while(std::getline(file,line)){
			line = jsl.trim(line);
			words_checked.push_back(line);
		}
	}
	file.close();
	return words_checked;
}
void local_look_for_past_participle_of_word(MYSQL* conn){
	SysLogLib sys_j;
	Jsonlib jsonl_j;
	nlp_lib nl_j;
    nemslib nem_j;
	libdict libd_j;
	std::vector<std::string> words_checked = get_checkedTXT();
	std::vector<Mdatatype> read_english_voc;
	size_t td = 2000;
	sys_j.writeLog("/home/ronnieji/lib/db_tools/log","Start looking for words past tenses...");
	std::string query = "SELECT word,word_type,meaning_en,meaning_zh FROM nlp_db.english_voc order by word asc";
    if (mysql_query(conn, query.c_str())==0) {
		std::vector<Mdatatype> Data;
		MYSQL_RES* res;
		MYSQL_ROW row;
		res = mysql_store_result(conn);
		while ((row = mysql_fetch_row(res)) != nullptr) {
			Mdatatype data;
			data.word = row[0];
			data.word_type = row[1];
			data.meaning_en = row[2];
			data.meaning_zh = row[3];
			/*
				check if the word has already checked
			*/
			std::string wChecked = data.word;
			std::cout << "Start checking..." << wChecked << '\n';
			auto word_c = std::find_if(words_checked.begin(),words_checked.end(),[wChecked](const std::string& s){
				return wChecked == s;
			});
			if(word_c!= words_checked.end()){
				/*
					already check, move next
				*/
				continue;
			}
			/*
				check the online dictionary
			*/
			std::string str_word_type = jsonl_j.trim(data.word_type);
			if(str_word_type=="VB"){
				wChecked = jsonl_j.trim(wChecked);
				std::string strUrl_past_participle;
				/*
					word net's vacabulary has more than one words
				*/
				int wordCount = nem_j.en_string_word_count(wChecked);
				if(wordCount > 1){
					/*
						There are more than one words
					*/
					std::vector<std::string> strWord_split = nem_j.tokenize_en(wChecked);
					if(!strWord_split.empty()){
						std::string str_word_lookup = strWord_split[0];
						std::string str_rest_words;
						if(strWord_split.size() > 1){
							for(unsigned int i =1; i < strWord_split.size(); i++){
								str_rest_words += strWord_split[i] + " ";
							}
							str_rest_words = jsonl_j.trim(str_rest_words);
						}
						else{
							str_rest_words = strWord_split[1];
						}
						strUrl_past_participle = "https://www.merriam-webster.com/dictionary/" + str_word_lookup;
						/*
							get past participle
						*/
						std::vector<std::string> get_result_past_participle = libd_j.getPastParticiple(strUrl_past_participle,"/home/ronnieji/lib/db_tools/log");
						sys_j.sys_timedelay(td);
						if(!get_result_past_participle.empty()){
							for(const auto& gr : get_result_past_participle){
								Mdatatype new_word;
								new_word.word = gr + " " + str_rest_words;
								new_word.word_type="VB";
								new_word.meaning_en = data.meaning_en + " ";
								new_word.meaning_zh = data.meaning_zh + " ";
								std::cout << "Checking the word: " << new_word.word << std::endl;
								/*
									delete the repeated word in english_voc
								*/
								std::string query = "DELETE FROM english_voc where word='" + new_word.word + "'";
								if (mysql_query(conn, query.c_str())) {
									std::cerr << "Error DELETE FROM english_voc: " << new_word.word << " " << mysql_error(conn) << std::endl;
								}
								/*
									insert the new word into english_voc;
								*/
								std::string insert_query = "INSERT INTO english_voc(word, word_type, meaning_en, meaning_zh)VALUES('" + new_word.word + "', '" + new_word.word_type + "', '" + new_word.meaning_en + "', '" + new_word.meaning_zh + "')";
								if (mysql_query(conn, insert_query.c_str())) {
									std::cerr << "Error inserting data into MySQL database: " << mysql_error(conn) << std::endl;
								}
								/*
									add to the check list
								*/
								write_checkedTXT(new_word.word);
							}
						}
					}
					
				}
				else{
					strUrl_past_participle = "https://www.merriam-webster.com/dictionary/" + wChecked;
					/*
						get past participle
					*/
					std::vector<std::string> get_result_past_participle = libd_j.getPastParticiple(strUrl_past_participle,"/home/ronnieji/lib/db_tools/log");
					sys_j.sys_timedelay(td);
					if(!get_result_past_participle.empty()){
						for(const auto& gr : get_result_past_participle){
							Mdatatype new_word;
							new_word.word = gr;
							new_word.word_type="VB";
							new_word.meaning_en = data.meaning_en + " ";
							new_word.meaning_zh = data.meaning_zh + " ";
							/*
								delete the repeated word in english_voc
							*/
							std::cout << "Checking the word: " << new_word.word << std::endl;
							std::string query = "DELETE FROM english_voc where word='" + new_word.word + "'";
							if (mysql_query(conn, query.c_str())) {
								std::cerr << "Error DELETE FROM english_voc: " << new_word.word << " " << mysql_error(conn) << std::endl;
							}
							/*
								insert the new word into english_voc;
							*/
							std::string insert_query = "INSERT INTO english_voc(word, word_type, meaning_en, meaning_zh)VALUES('" + new_word.word + "', '" + new_word.word_type + "', '" + new_word.meaning_en + "', '" + new_word.meaning_zh + "')";
							if (mysql_query(conn, insert_query.c_str())) {
								std::cerr << "Error inserting data into MySQL database: " << mysql_error(conn) << std::endl;
							}
							/*
								add to the check list
							*/
							write_checkedTXT(new_word.word);
						}
					}
				}
			}
			std::this_thread::sleep_for(std::chrono::seconds(5));//seconds
		}
		mysql_free_result(res);
	}
	sys_j.writeLog("/home/ronnieji/lib/db_tools/log","Done looking for words past tenses...");
}
int main(int argc, char* argv[]) {
	/*
		add word net vocabulary into english_voc
	*/
	SysLogLib syslog_j;
	MYSQL conn;
    if (connect_to_mysql(&conn)) {
		//add_wordnet(&conn);
		local_look_for_past_participle_of_word(&conn);
	}
	syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","All jobs are done!");
    return 0;
}
