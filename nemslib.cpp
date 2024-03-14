/*
lib compile:

    g++ -c -fPIC /Volumes/RONNIE_JI/MacBk/pytest/NLP_test/src/lib/nemslib.cpp -o /Volumes/RONNIE_JI/MacBk/pytest/NLP_test/src/lib/nemslib.o -I/usr/local/include/eigen3/ -I/usr/local/boost/include/ -I/opt/homebrew/Cellar/icu4c/73.2/include/ -L/opt/homebrew/Cellar/icu4c/73.2/lib -licuuc -licudata /usr/local/boost/lib/libboost_system.a /usr/local/boost/lib/libboost_filesystem.a /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/lib/nemslib.a -I/Users/dengfengji/miniconda3/pkgs/sqlite-3.41.2-h80987f9_0 -lsqlite3 -I/Volumes/WorkDisk/cppLibs/curl/include/ -lcurl -I/opt/homebrew/Cellar/gumbo-parser/0.10.1/include /opt/homebrew/Cellar/gumbo-parser/0.10.1/lib/libgumbo.a -std=c++20
    g++ -shared -o /Volumes/WorkDisk/cpp_test/db_tools/lib/nemslib.a /Volumes/RONNIE_JI/MacBk/pytest/NLP_test/src/lib/nemslib.o -I/usr/local/include/eigen3/ -I/usr/local/boost/include/ -I/opt/homebrew/Cellar/icu4c/73.2/include/ -L/opt/homebrew/Cellar/icu4c/73.2/lib -licuuc -licudata /usr/local/boost/lib/libboost_system.a /usr/local/boost/lib/libboost_filesystem.a /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/lib/nemslib.a -I/Users/dengfengji/miniconda3/pkgs/sqlite-3.41.2-h80987f9_0 -lsqlite3 -I/Volumes/WorkDisk/cppLibs/curl/include/ -lcurl -I/opt/homebrew/Cellar/gumbo-parser/0.10.1/include /opt/homebrew/Cellar/gumbo-parser/0.10.1/lib/libgumbo.a -std=c++20

*/
/*
	Ronnie Ji
	version 0.1
	updated on Feb 21, 2024 : changed struct datatype -> struct Mdatatype
    version 0.2
    updated on Feb 25, 2024 : changed tokenized_en -> works both for English & Chinese/ add get_topics_freq which shows the frequency of the topic;
    version 0.3
    Add lots of features from file_pro.cpp and others 
*/
#include "nemslib.h"
#include "authorinfo/author_info.h"
#include <sqlite3.h>
#include <fstream>
#include <vector>
#include <string>
#include <random>
#include <set>
#include <algorithm>
#include <numeric>
#include <cmath>
#include <thread>
#include <iomanip>
#include <filesystem>
#include <chrono>
#include <regex>
#include <cctype>//std::isdigit()
#include <unistd.h>
#include <unicode/unistr.h>  // ICU
#include <unicode/uchar.h>
#include <unordered_set> // ICU
#include <atomic>//guarantee to be executed as a single ,indivisible operation without interruption.
#include <gumbo.h>
#include <iostream>
#include <sstream>
#include <ranges> //std::ranges::split(tokens,input,std::isspace);
#include <map>
#include <unordered_map>

//#define DB_PATH std::filesystem::absolute(std::filesystem::path("db/catalogdb.db")).string();
/*
    std::filesystem::path currentPath = std::filesystem::current_path();

    std::cout << "Current working directory: " << currentPath << std::endl;

    std::filesystem::path absolutePath = std::filesystem::absolute(currentPath);

    std::cout << "Absolute path: " << absolutePath << std::endl;
*/

std::atomic<bool> result_found(false); // Shared flag variable for database

/*
    store the stopword list
*/
static std::unordered_set<std::string> stopwordSet;

/*-Start Json Library-*/
std::string Jsonlib::trim(const std::string& str_input) {
    auto is_not_space = [](unsigned char ch){return !std::isspace(ch);};
    auto start = std::ranges::find_if(str_input,is_not_space);
    auto end = std::ranges::find_if(str_input | std::views::reverse, is_not_space).base();
    if (start < end) {
        return {start, end};
    } else {
        return {};
    }  
}
/*
    Get the strings between [BOF] and [EOF]-special case for article topics seperator
*/
std::string_view Jsonlib::extractString(std::string_view str,const std::string& str_start, const std::string& str_end){
    if(str.empty() || str_start.empty() || str_end.empty()){
        std::cerr << "Jsonlib::extractString input empty" << '\n';
        return {};
    }
    // Find the position of [BOF]
    auto start = str.find(str_start);//"[BOF]");
    if (start == std::string_view::npos) {
        return {};  // [BOF] not found
    }
    // Find the position of [EOF]
	size_t pos = str_start.size();//the real start positon
    auto end = str.find(str_end,start + pos); //"[EOF]", start + 5);
    if (end == std::string_view::npos) {
        return {};  // [EOF] not found
    }
    // Return the substring between [BOF] and [EOF]
    return str.substr(start + pos, end - (start + pos));
}
/*
    split a string by a delimiter
*/
std::vector<std::string> Jsonlib::splitString(const std::string& input, char delimiter){
    std::vector<std::string> result;
    if(input.empty() || delimiter == '\0'){
        std::cerr << "Jsonlib::splitString input empty" << '\n';
    	return result;
    }
    std::stringstream ss(input);
    std::string token;
    while(std::getline(ss,token,delimiter)){
        result.push_back(token);
    }
    return result;
}
/*
    split a string by a string
*/
std::vector<std::string> Jsonlib::splitString_bystring(const std::string& input, const std::string& delimiter){
    std::vector<std::string> tokens;
    if(input.empty() || delimiter.empty()){
        std::cerr << "Jsonlib::splitString_bystring input empty" << '\n';
    	return tokens;
    }
    size_t start = 0;
    size_t end = input.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(input.substr(start, end - start));
        start = end + delimiter.length();
        end = input.find(delimiter, start);
    }
    tokens.push_back(input.substr(start, end));
    return tokens;
}
bool Jsonlib::isDomainExtension(const std::string& word){
	static const std::vector<std::string> domainExtensions = {
		".com", ".net", ".org", ".edu", ".gov", ".io", ".co", // Add more as needed
	};
	for (const auto& ext : domainExtensions) {
		if (word.size() >= ext.size() && word.substr(word.size() - ext.size()) == ext) {
			return true;
		}
	}
	return false;
}
std::vector<std::string> Jsonlib::split_sentences(const std::string& text){
	std::vector<std::string> sentences;
	std::istringstream iss(text);
	std::string token;
	std::string sentence;
	while (iss >> token) {
		sentence += token + " ";
		// Check if the token ends with a sentence terminator
		char lastChar = token.back();
		if (lastChar == '.' || lastChar == '?' || lastChar == '!' || lastChar == ';') {
			// Check if the token is a domain extension
			if (this->isDomainExtension(token)) {
				continue;
			}
			// Trim the sentence and add it to the list if it's not empty
			sentence = sentence.substr(0, sentence.size() - 1); // Remove trailing space
			if (!sentence.empty()) {
				sentences.push_back(sentence);
				sentence.clear();
			}
		}
	}
	// Add any remaining text as the last sentence
	if (!sentence.empty()) {
		sentences.push_back(sentence);
	}
	return sentences;
}
void Jsonlib::toLower(std::string& str){
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::tolower(c); });
}
void Jsonlib::toUpper(std::string& str){
    std::transform(str.begin(), str.end(), str.begin(), [](unsigned char c){ return std::toupper(c); });
}
void Jsonlib::remove_numbers(std::string& str){
    str.erase(std::remove_if(str.begin(), str.end(), [](char c) { return std::isdigit(c); }), str.end());
}
std::string Jsonlib::str_replace(std::string& originalString, std::string& string_to_replace, const std::string& replacement){
	if(string_to_replace.empty()){
		std::cerr << "Jsonlib::str_replace input empty" << '\n';
		return "";
	}
    size_t startPos = originalString.find(string_to_replace);
    while (startPos != std::string::npos) {
        originalString.replace(startPos, string_to_replace.length(), replacement);
        startPos = originalString.find(string_to_replace, startPos + replacement.length());
    }
    return originalString;
}
std::vector<std::string> Jsonlib::read_single_col_list(const std::string& file_path){
    std::vector<std::string> url_list;
    if(file_path.empty()){
      std::cerr << "Jsonlib::read_single_col_list: input is empty!" << '\n';
      return url_list;
    }
    std::ifstream stopwordFile(file_path);
      if (!stopwordFile.is_open()) {
          std::cerr << "read_stopword_list::Error opening stopword file-nemslib" << '\n';
          return url_list;
      }
      std::string line;
      Jsonlib jl_j;
      while(std::getline(stopwordFile, line)){
          line = jl_j.trim(line);
          url_list.push_back(line);
      }
      stopwordFile.close();
      return url_list;
}
void Jsonlib::removeDuplicates(std::vector<std::string>& vec){
    if(!vec.empty()){
        try{
            std::sort(vec.begin(), vec.end());
            auto it = std::unique(vec.begin(), vec.end());
            vec.erase(it, vec.end());
        }
        catch(const std::exception& e){
            std::cerr << e.what() << '\n';
        }
    }
}
std::string Jsonlib::remove_last_char_in_a_string(const std::string& str){
    return str.substr(0,str.size()-1);
}
/*-End Json Library-*/
/*-start Sqlite3 Library-*/
SQLite3Library::SQLite3Library(const std::string& db_file) : db_file(db_file), connection(nullptr) {}
void SQLite3Library::connect() {
    int result = sqlite3_open(db_file.c_str(), &connection);
    if (result != SQLITE_OK) {
        std::cerr << "Error opening database: " << sqlite3_errmsg(connection) << std::endl;
    }
}
void SQLite3Library::disconnect(){
    sqlite3_close(connection);
}
template<typename T>
void SQLite3Library::executeQuery(const std::string& query, T& returnValue) {
    sqlite3_stmt* statement = nullptr;
    int result = sqlite3_prepare_v2(connection, query.c_str(), -1, &statement, nullptr);
    if (result != SQLITE_OK) {
        std::cerr << "Error preparing query: " << sqlite3_errmsg(connection) << std::endl;
        return;
    }
    while ((result = sqlite3_step(statement)) == SQLITE_ROW) {
        std::vector<std::string> row;
        int numColumns = sqlite3_column_count(statement);
        for (int i = 0; i < numColumns; i++) {
            const char* value = reinterpret_cast<const char*>(sqlite3_column_text(statement, i));
            row.push_back(value ? value : "NULL");
        }
        returnValue.push_back(row);
    }
    if (result != SQLITE_DONE) {
        std::cerr << "Error executing query: " << sqlite3_errmsg(connection) << std::endl;
    }
    sqlite3_finalize(statement);
}

// Explicit template instantiation for vector<vector<string>>
template void SQLite3Library::executeQuery(std::string const& query, std::vector<std::vector<std::string>>& returnValue);

template<typename T>
int SQLite3Library::callback(void* data, int argc, char** argv, char** columnNames) {
    T* returnValue = static_cast<T*>(data);
    // Create a new row to store the values
    std::vector<std::string> row;
    // Iterate over the columns and store the values in the row
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            row.push_back(argv[i]);
        } else {
            row.push_back("NULL");
        }
    }
    // Add the row to the result vector
    returnValue->push_back(row);
    return 1;
}
/*-End Sqlite3 Library-*/
/*
    Start nemslib library----
*/
void nemslib::set_stop_word_file_path(const std::string& stop_word_file_path){
    stop_word_list_path = stop_word_file_path;
}
std::string nemslib::get_current_dir(){
    char cwd[PATH_MAX];
    if(getcwd(cwd,sizeof(cwd))!= nullptr){
        return cwd;
    }else{
        return "";
    }
}
bool nemslib::isVector(const std::string& str) {
    std::regex vectorPattern(R"((-?\d+(\.\d+)?\s*){5})");//
    return std::regex_match(str, vectorPattern);
}
bool nemslib::isNonAlphabetic(const std::string& word) {
    for (const auto& ch : word) {
        if (!std::isalpha(ch)) {
            return true;
        }
    }
    return false;
}
bool nemslib::isNumeric(const std::string& str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}
int nemslib::en_string_word_count(const std::string& sentence){
    if(sentence.empty()){
        std::cerr << "nemslib::en_string_word_count input sentence empty!" << '\n';
        return -1;
    }
    int count = std::ranges::distance(sentence | std::views::split(' '));
    return count;
}
/*
	remove all the char_list's char in input_str
*/
std::string nemslib::remove_chars_from_str(const std::string& input_str, const std::vector<std::string>& char_list_to_remove){
    std::string result = input_str;
    if(input_str.empty() || char_list_to_remove.empty()){
    	std::cerr << "nemslib::remove_chars_from_str input empty" << '\n';
    	return "";
    }
    for(const auto& replacement : char_list_to_remove){
        std::size_t pos_r = result.find(replacement + " ");//at space on Oct23,2023
		while (pos_r != std::string::npos && pos_r < result.length()) {
			result.erase(pos_r, replacement.length()); // Remove "'s"
			pos_r = result.find(replacement, pos_r);
		}
    }
    for(const auto& replacement : char_list_to_remove){
        std::size_t pos_l = result.find(" " + replacement);//at space on Oct23,2023
		while (pos_l != std::string::npos && pos_l < result.length()) {
			result.erase(pos_l, replacement.length()); // Remove "'s"
			pos_l = result.find(replacement, pos_l);
		}
    }
    //double check
    result = this->removeEnglishPunctuation_training(result);
    return result;
}
std::string nemslib::removeEnglishPunctuation_training(const std::string& str) {
    std::string result = str;
    if(str.empty()){
    	std::cerr << "nemslib::removeEnglishPunctuation_training input empty" << '\n';
    	return "";
    }
    std::vector<std::string> replacements = {"'em","'cause","'m","'s","'re","'ll","'ve","'d","'t",",",".",",","!","?",":",";","/","\\","|","[","]","@","#","$","%","^","&","*","(",")", "\"",",","“","”","!",".","--","?","*","?\"",".\"",",\""};
    for(const auto& replacement : replacements){
        std::size_t pos_r = result.find(replacement + " ");//added space on Oct23,2023
		while (pos_r != std::string::npos) {
			result.erase(pos_r, replacement.length()); // Remove "'s"
			pos_r = result.find(replacement, pos_r);
		}
    }
    for(const auto& replacement : replacements){
        std::size_t pos_l = result.find(" " + replacement);//added space on Oct23,2023
		while (pos_l != std::string::npos) {
			result.erase(pos_l, replacement.length()); // Remove "'s"
			pos_l = result.find(replacement, pos_l);
		}
    }
    std::vector<std::string> replacements_punctuation_only = {",",".",",","!","?",":",";","/","\\","|","[","]","@","#","$","%","^","&","*","(",")", "\"",",","“","”","!",".","--","?","*","?\"",".\"",",\""};
    for(const auto& replacement : replacements_punctuation_only){
        std::size_t pos_Non = result.find(replacement);//added space on Oct23,2023
        while (pos_Non != std::string::npos) {
			result.erase(pos_Non, replacement.length()); // Remove "'s"
			pos_Non = result.find(replacement, pos_Non);
		}
    }
    result.erase(std::remove_if(result.begin(), result.end(),[](char c) { return std::ispunct(static_cast<unsigned char>(c)); }),result.end());
    return result;
}
std::string nemslib::removeChinesePunctuation_training(const std::string& input) {
    icu::UnicodeString unicodeInput = icu::UnicodeString::fromUTF8(input);
    icu::UnicodeString output;
    for (int32_t i = 0; i < unicodeInput.length(); ++i) {
        UChar32 character = unicodeInput.char32At(i);
        if (!u_ispunct(character)) {
            output.append(character);
        }
        if (U_IS_SUPPLEMENTARY(character)) {
            i++; // Skip the next code unit of a supplementary character
        }
    }
    std::string result;
    output.toUTF8String(result);
    return result;
}
/*
    old code
*/
// std::vector<std::string> result;
// std::istringstream iss(line);
// std::string token;
// while (std::getline(iss, token, ' ')) {
//     result.push_back(token);
// }
// return result;
/*
    new
*/
std::vector<std::string> nemslib::tokenize_en(const std::string& str_line) {
    std::vector<std::string> result;
    if(str_line.empty()){
        std::cerr << "nemslib::tokenize_en input empty!" << '\n';
        return result;
    }
    std::stringstream ss(str_line);
    for(const auto& token : std::ranges::istream_view<std::string>(ss)){
        result.push_back(token);
    }
    return result;
}
/*
    tokenize a string and extract Chinese characters as separate tokens.
*/
std::vector<std::string> nemslib::tokenize_zh(const std::string& line) {
    std::vector<std::string> tokens;
    std::string token;
    std::regex re("[\u4e00-\u9fa5]"); // regular expression to match Chinese characters
    for (char c : line) {
        if(std::isspace(c)) {
            if(!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
        } else if(std::regex_match(std::string(1, c), re)) {
            if (!token.empty()) {
                tokens.push_back(token);
                token.clear();
            }
            tokens.push_back(std::string(1, c));
        } else {
            token.push_back(c);
        }
    }
    if (!token.empty()) {
        tokens.push_back(token);
    }
    return tokens;
}
/*
    tokenize a string and extract individual Unicode characters as separate tokens.
*/
std::vector<std::string> nemslib::tokenize_zh_unicode(const std::string& text) {
    std::vector<std::string> characters;
    icu::UnicodeString unicodeText = icu::UnicodeString::fromUTF8(text);
    for (int32_t i = 0; i < unicodeText.length(); i++) {
        UChar32 character = unicodeText.char32At(i);
        icu::UnicodeString unicodeCharacter(character);
        std::string utf8Character;
        unicodeCharacter.toUTF8String(utf8Character);
        characters.push_back(utf8Character);
    }
    return characters;
}
bool nemslib::is_stopword_en(const std::string& word){
    if(word.empty()){
        std::cerr << "nemslib::is_stopword_en input word is empty." << '\n';
        return false;
    }
    //static std::unordered_set<std::string> stopwordSet;
    if(stopwordSet.empty()){
        std::vector<std::string> stopword_en = this->read_stopword_list();
        if(!stopword_en.empty()){
            for(const auto& stopword : stopword_en){
                stopwordSet.insert(stopword);
            }
        }
    }
    return stopwordSet.find(word) != stopwordSet.end();
}
std::vector<std::pair<std::string, int>> nemslib::calculateTermFrequency(const std::string& text){
    std::map<std::string, int> termFreq;
    std::vector<std::string> words;
    if(text.empty()){
        std::cerr << "nemslib::calculateTermFrequency input empty!" << '\n';
        return {};
    }
    if(this->isNonAlphabetic(text)){
        words = this->tokenize_zh(text);
    }
    else{
        words = this->tokenize_en(text);
    }
    /*
        cout the frequency of each word
    */
    for (const std::string& word : words) {
        termFreq[word]++;
    }
    /*
    sort the t_fre order from big to small
    Sort the map based on the values in descending order
    */
    std::vector<std::pair<std::string, int>> sortedFreq(termFreq.begin(), termFreq.end());
    std::sort(sortedFreq.begin(), sortedFreq.end(), [](const auto& a, const auto& b) {
        return a.second > b.second;
    });
    return sortedFreq;
}
std::string nemslib::get_topics(const std::string& str_in){
    Jsonlib jsl_j;
    std::string strNoStopwords;
    // Tokenization
    std::vector<std::string> result;
    if(str_in.empty()){
    	std::cerr << "nemslib::get_topics input empty" << '\n';
    	return "";
    }
    if(this->isNonAlphabetic(str_in)){
        result = this->tokenize_zh(str_in);
    }
    else{
        result = this->tokenize_en(str_in);
    }
	for(const auto& rs : result){
		if(!this->is_stopword_en(rs)){
			strNoStopwords += rs + " ";
		}
	}
    strNoStopwords = jsl_j.trim(strNoStopwords);
    std::string topic_string;
    if(!str_in.empty() && str_in.length() > 0){
        std::vector<std::pair<std::string, int>> sorted_txt = this->calculateTermFrequency(strNoStopwords);
        for(const auto& item : sorted_txt){
            topic_string += item.first + " ";
        }
    }
    topic_string = jsl_j.trim(topic_string);
    return topic_string;
}
std::vector<std::unordered_map<std::string,unsigned int>> nemslib::get_topics_freq(const std::string& str_in){
    Jsonlib jsl_j;
    std::string strNoStopwords;
    // Tokenization
    std::vector<std::string> result;
    std::vector<std::unordered_map<std::string,unsigned int>> final_result;
    if(str_in.empty()){
    	std::cerr << "nemslib::get_topics input empty" << '\n';
    	return final_result;
    }
    if(this->isNonAlphabetic(str_in)){
        result = this->tokenize_zh(str_in);
    }
    else{
        result = this->tokenize_en(str_in);
    }
	for(const auto& rs : result){
		if(!this->is_stopword_en(rs)){
			strNoStopwords += rs + " ";
		}
	}
    strNoStopwords = jsl_j.trim(strNoStopwords);
    std::string topic_string;
    if(!str_in.empty() && str_in.length() > 0){
        std::vector<std::pair<std::string, int>> sorted_txt = this->calculateTermFrequency(strNoStopwords);
        for(const auto& item : sorted_txt){
            std::unordered_map<std::string,unsigned int> uitem;
            uitem[item.first] = item.second;
            final_result.push_back(uitem);
        }
    }
    return final_result;
}
std::vector<std::string> nemslib::read_stopword_list(){
    std::vector<std::string> stopwords;
    if(nemslib::stop_word_list_path.empty()){
        std::cerr << "nemslib::read_stopword_list stopword file path stop_word_list_path empty" << '\n';
    	return stopwords;
    }
    std::ifstream stopwordFile(this->stop_word_list_path);
    if (!stopwordFile.is_open()) {
        std::cerr << "read_stopword_list::Error opening stopword file-nemslib" << '\n';
        return stopwords;
    }
    std::string line;
    Jsonlib jl_j;
    while(std::getline(stopwordFile, line)){
        line = jl_j.trim(line);
        //stopwords.push_back(line + " "); removed on Nov 11,2023
        stopwords.push_back(line);
    }
    stopwordFile.close();
    return stopwords;
}
std::string nemslib::remove_en_stopwords_from_string_for_gpt(const std::string& str){
	if(str.empty()){
		std::cerr << "nemslib::remove_en_stopwords_from_string_for_gpt input empty" << '\n';
		return "";
	}
    std::string userInput = this->removeEnglishPunctuation_training(str);
    std::vector<std::string> userInput_tokenized = this->tokenize_en(userInput);
    std::string userInput_no_stopwords;
    for(const auto& userInput_not_stop : userInput_tokenized){
        if(!this->is_stopword_en(userInput_not_stop)){
            userInput_no_stopwords += userInput_not_stop + " ";
        }
    }
    Jsonlib jl_j;
    userInput_no_stopwords = jl_j.trim(userInput_no_stopwords);
    std::string q_topic = this->get_topics(userInput_no_stopwords);
    q_topic = jl_j.trim(q_topic);
    jl_j.toLower(q_topic);
    return q_topic;
}
nemslib::return_topic_match nemslib::if_topic_match_the_content(const std::string& str_topic, const std::string& str_content, float& matching_rate){
    /*
        define return dataset
    */
    nemslib::return_topic_match r_m_topic;
    if(str_topic.empty() || str_content.empty()){
    	return r_m_topic;
    }
    std::vector<std::string> s_topic = this->tokenize_en(str_topic);
    float predict_total = this->en_string_word_count(str_content);
    float predict_cout = 0;
    float predict_perc = 0;
    for(const auto& st : s_topic){
        if(str_content.find(st) != std::string::npos){
            predict_cout ++;
            /*
                test print
            */
            predict_perc = predict_cout / predict_total;
            //std::cout << "predict_cout: " << predict_cout << " predict_total:" << predict_total << " matching rate: " << predict_perc << std::endl;
            if (predict_perc > matching_rate){
                r_m_topic.topic_match_content = true;
                r_m_topic.matching_percentage = predict_perc;
                return r_m_topic;
            }
        }
    }
    r_m_topic.topic_match_content = false;
    r_m_topic.matching_percentage = predict_perc;
    return r_m_topic;
}
std::vector<std::string> nemslib::get_language_exp(const std::string& sqlite3_db_file_path){
    std::vector<std::vector<std::string>> result;
    std::vector<std::string> str_exp;
    if(sqlite3_db_file_path.empty()){
        std::cerr << "nemslib::get_language_exp input empty!" << '\n';
        return str_exp;
    }
    SQLite3Library db(sqlite3_db_file_path);//"/home/ronnieji/lib/db/catalogdb.db");
    db.connect();
    db.executeQuery("select exp from language_exp",result);
    db.disconnect();
    if(!result.empty()){
        for(const auto&row : result){
            str_exp.push_back(row[0]);
        }
    }
    return str_exp;
}
std::vector<std::string> nemslib::readTextFile(const std::string& filename) {
    std::vector<std::string> lines;
	if(filename.empty()){
		std::cerr << "nemslib::readTextFile : input empty!" << '\n';
		return lines;
	}
    std::ifstream inFile(filename);
    if (!inFile.is_open()) {
        std::cout << "Failed to open file: " << filename << std::endl;
        return lines;
    }
    std::string line;
	Jsonlib jl_j;
    while (std::getline(inFile, line)) {
		line = jl_j.trim(line);
        lines.push_back(line);
    }
    inFile.close();
    return lines;
}
std::string nemslib::read_rawtxts(const std::string& input_file_path,const std::string& english_stopwords_file_path,const std::string& sqlite3_db_file_path){
    //classfied txt test
    std::string rawdata_txt;
    std::vector<std::string> rawdata_lines;
    if(input_file_path.empty() || english_stopwords_file_path.empty() || sqlite3_db_file_path.empty()){
    	std::cerr << "nemslib::read_rawtxts input empty" << '\n';
    	return "";
    }
    std::ifstream inFile(input_file_path);
    if(!inFile.is_open()){
        std::cout << "File open failed" << std::endl;
        return "";
    }
    //phrase the category name from the text file name
    std::string str_category;
    std::string str_filename;
    std::vector<std::string> str_filename_split;
    Jsonlib json_r;
    /*
        initialize stopword in nemslib
    */
    nemslib nems_j;
    nems_j.set_stop_word_file_path(english_stopwords_file_path);//"/home/ronnieji/lib/lib/res/english_stopwords");

    str_filename = input_file_path.substr(input_file_path.find_last_of("/")+1,input_file_path.length());
    str_filename = json_r.splitString(str_filename,'.')[0];
    str_filename_split = json_r.splitString(str_filename,'-');
    str_category = "[" + str_filename_split[0] + "][" + str_filename_split[1] + "]";//[med][diabetes]
    //std::filesystem::path cwd = std::filesystem::current_path();
    // std::filesystem::path stopwordlist_path = "/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/res/english_stopwords"; //cwd.relative_path() / "res/english_stopwords";
    // std::cout << std::string(stopwordlist_path) << std::endl;
    std::string rawdata_line;
    while (std::getline(inFile, rawdata_line)){
        rawdata_line = this->removeEnglishPunctuation_training(rawdata_line);
        rawdata_line = this->removeChinesePunctuation_training(rawdata_line);
        json_r.toLower(rawdata_line);
        json_r.remove_numbers(rawdata_line);
        std::istringstream iss(rawdata_line);
        std::string word;
        while (iss >> word){
            if (!nemslib::is_stopword_en(word)) {
                rawdata_txt += word + " ";
            }
        }
    }
    inFile.close();
    /*
        lower-case all the text
    */
    std::string topic_string;
    topic_string = this->get_topics(rawdata_txt);
    if(topic_string.empty()){
        std::cout << "No topics found!" << std::endl;
        return topic_string;
    }
    std::string rawdata_output = json_r.trim(topic_string);
    //rawdata_output = rawdata_output + "[EOT]" + rawdata_txt;//have to vector this later, should not include rawdata_txt
    nemslib::savedb(rawdata_output,str_category,sqlite3_db_file_path);//"/home/ronnieji/lib/db/catalogdb.db");
    return rawdata_output;
}
void nemslib::savedb(const std::string& strinput,const std::string&str_category, const std::string& dbstring){
	if(strinput.empty() || str_category.empty() || dbstring.empty()){
		std::cerr << "nemslib::savedb input empty" << '\n';
		return;
	}
    SQLite3Library db(dbstring);
    db.connect();
    std::vector<std::vector<std::string>> result;
    db.executeQuery("delete FROM tracorp where cat = '" + str_category + "'", result);
    db.executeQuery("insert into tracorp(cat,sortedtop)values('" + str_category + "','" + strinput + "')", result);
    db.disconnect();
}
/*
    score_v / predict_txt_total >= *CATAG_RATE
    -> if the topics found is greater than *CATAG_RATE, match the topic
*/
int nemslib::predict_sorted_Text_match_the_topic(const std::vector<std::string>& predict_sorted_txt,const std::string& topic_string, float& matching_rate){
    double score_v;
    if(predict_sorted_txt.empty() || topic_string.empty()){
    	std::cerr << "predict_sorted_Text_match_the_topic input empty" << '\n';
    	return 0;
    }
    const double predict_txt_total = predict_sorted_txt.size();
    for(const auto& item : predict_sorted_txt){
            /*
            predict_topic_string += item.first + " ";
            str_sortednum += std::to_string(item.second) + " ";
            std::cout << "Word: " << item.first << " frequency: "<<  item.second << std::endl;
            */
            if (topic_string.find(item) != std::string::npos){
                score_v +=1;
            }
			if (score_v / predict_txt_total >= matching_rate){ //0.5){
		            return 1;
		    }
    }
    return 0;
}
/*
    classfied txt test-prediction
*/
std::string nemslib::read_predicttxts(const std::string& input_file_path,const std::string& sqlite3_db_file_path,float& matching_rate){
    std::string predictxt_line;
    std::string predicttxt_final_string;
    if(input_file_path.empty() || sqlite3_db_file_path.empty()){
    	std::cerr << "nemslib::read_predicttxts input empty" << '\n';
    	return "";
    }
    std::ifstream inFile(input_file_path);
    if(!inFile.is_open()){
        std::cout << "File open failed" << std::endl;
        return "";
    }
    /*
        lower-case all the text
    */
    result_found = false;
    std::cout << "start reading:" + input_file_path << std::endl;
    Jsonlib json_r;
    while (std::getline(inFile, predictxt_line)){
        predictxt_line = this->removeEnglishPunctuation_training(predictxt_line);
        predictxt_line = this->removeChinesePunctuation_training(predictxt_line);
        json_r.toLower(predictxt_line);
        json_r.remove_numbers(predictxt_line);
        std::istringstream iss(predictxt_line);
        std::string word;
        while (iss >> word){
            if (!this->is_stopword_en(word)) {
                predicttxt_final_string += word + " ";
            }
        }
    }
    inFile.close();
    std::string predict_topic_string;
    std::string str_sortednum;
    std::vector<std::pair<std::string, int>> predict_sorted_txt = this->calculateTermFrequency(predicttxt_final_string);
    /*
        the raw data of predict txt
        multi-thread
    */
    SQLite3Library db(sqlite3_db_file_path);//"/home/ronnieji/lib/db/catalogdb.db");
    db.connect();
    std::vector<std::vector<std::string>> result;
    std::string sql_query = "SELECT * FROM tracorp";
    db.executeQuery(sql_query, result);
    db.disconnect();

    /*
        catalog the predict txt
    */
    std::string uncatelog = "[UNC]";
    std::string& predict_result = uncatelog;
    int checkMatchTopics = 0;
    std::vector<std::string> checkMatchTopicsInputStrList;
    for(const auto& row : result){
        /*
            row[0] = [MED][dia], row[1] = "[SOT] data [EOT]"
        */
        std::cout << row[0] << " " << row[1] << std::endl;
        for(const auto& item : predict_sorted_txt){
            checkMatchTopicsInputStrList.push_back(item.first);
        }
        checkMatchTopics = predict_sorted_Text_match_the_topic(checkMatchTopicsInputStrList,row[1],matching_rate);
        if(checkMatchTopics == 1){
            predict_result = row[0]; //catalog the predict txt-final result
            std::cout << "Done catalog the input, the result is:" + predict_result << std::endl;
            return predict_result;
        }
    }
    return "";
}
std::string nemslib::preprocess_words(const std::string& str_in){
    std::string temp;
    std::string str_temp = str_in;
    str_temp = this->removeEnglishPunctuation_training(str_temp);//remove punctuations-en
    str_temp = this->removeChinesePunctuation_training(str_temp);//remove punctuations-zh
    std::string str_output;
    for (const auto& ch : str_temp){
        if(ch == ' ' || (ch >= 0 && ch <= 127)){
            if (!temp.empty()) {
                str_output += temp;
                temp.clear();
            }
            if(ch != ' ') temp += ch;
        } else {
            temp += ch;
        }
    }
    if (!temp.empty()) str_output += temp;
    return str_output;
}
std::string nemslib::preprocess_words_training(const std::string& str_in){
    std::string temp;
    std::string str_temp = str_in;
    if(this->isNonAlphabetic(str_temp)){
        str_temp = this->removeChinesePunctuation_training(str_temp);//remove punctuations-zh
    }
    else{
        str_temp = this->removeEnglishPunctuation_training(str_temp);//remove punctuations-en
    }
    std::string str_output;
    for (const auto& ch : str_temp){
        if(ch == ' ' || (ch >= 0 && ch <= 127)){
            if (!temp.empty()) {
                str_output += temp;
                temp.clear();
            }
            if(ch != ' ') temp += ch;
        } else {
            temp += ch;
        }
    }
    if (!temp.empty()) str_output += temp;
    return str_output;
}
/*
    process user input from the terminal
*/
/*
    Check the answer in sqlite
*/
std::vector<std::vector<std::string>> nemslib::check_sql_return_result(const std::string& sqlite3_db_file_path,const std::string& strSQL){
	std::vector<std::vector<std::string>> result;
	if(sqlite3_db_file_path.empty() || strSQL.empty()){
		std::cerr << "nemslib::check_sql_return_result: input empty!" << '\n';
		return result;
	}
    SQLite3Library db(sqlite3_db_file_path);//"/home/ronnieji/lib/db/catalogdb.db");
    db.connect();
    db.executeQuery(strSQL, result);
    db.disconnect();
    return result;
}
/*
    Check user's input in the preload data
*/
void nemslib::process_user_input(const std::unordered_map<std::string, std::string>& stateMap, const std::string& user_input, const std::vector<std::string>& stopwords){
    // Process the user input
    //std::cout << "You entered: " << userInput << std::endl;
    /*
        pre-processing userInput,1.get_topics,2.tokenize_en
        use: topic
    */
    nemslib nems_j;
    Jsonlib jslib_j;
    SysLogLib sys_log;
    std::string q_topic = nems_j.remove_chars_from_str(user_input, stopwords);
    jslib_j.toLower(q_topic);//to lower case

    bool answer_is_found = false;
    std::unordered_map<float, std::string> store_secondary_answers;
    nemslib::return_topic_match r_t_m;
    for(const auto& row : stateMap){
        std::string str_state = row.first;
        float matching_rate = 0.5;
        r_t_m = nems_j.if_topic_match_the_content(q_topic, str_state, matching_rate);
        bool topic_match = r_t_m.topic_match_content;
        float predict_percent = r_t_m.matching_percentage;
        std::string str_answer;
        for(const auto& rw : row.second){
            str_answer += rw;
        }
        /*
            store the secondary data...
        */
        /*
            if there is multiple random answer mark #*
        */
        if(str_answer.find("#*") != std::string::npos){
            str_answer = this->get_random_answer_if_exists(str_answer);
        }
        store_secondary_answers[predict_percent] = str_answer;
        std::string strMatchAnswer = str_answer;
        str_answer = "";
        /*
            until it found the right answer
        */
        if(topic_match){
            sys_log.chatCout(strMatchAnswer);
            answer_is_found = true;
            break;
        }
        if(answer_is_found){
            break;
        }
    }
    /*
        look up the seconary q_topic in the corpus
    */
    if(!answer_is_found && store_secondary_answers.size()!=0){
        auto maxElement = std::max_element(store_secondary_answers.begin(), store_secondary_answers.end(),
        [](const auto& pair1, const auto& pair2) {
            return pair1.first < pair2.first;
        });
        if (maxElement != store_secondary_answers.end()) {
            sys_log.chatCout(maxElement->second);
            store_secondary_answers.clear();
        }
    }
}
void nemslib::process_user_multiple_input(const std::unordered_map<std::string, std::string>& stateMap, const std::string& userInput, const std::vector<std::string>& stopwords){
    nemslib nems_j;
    // Define the regular expression pattern to match punctuation marks, except for 's, 've, and 't
    std::regex punctuation("(?!('s|'ve|'d|'t))[[:punct:]]");

    // Use std::sregex_token_iterator to split the string by punctuation marks
    std::vector<std::string> tokens(std::sregex_token_iterator(userInput.begin(), userInput.end(), punctuation, -1),
                                    std::sregex_token_iterator());
    // Print the resulting tokens
    for (const std::string& token : tokens) {
        nems_j.process_user_input(stateMap,token,stopwords);
    }
}
std::string nemslib::get_random_answer_if_exists(const std::string& strAnswers){
    Jsonlib jl_j;
    std::vector<std::string> str_answers = jl_j.splitString_bystring(strAnswers,"#*");
    std::random_device rd; // obtain a random number from hardware
    std::mt19937 eng(rd()); // seed the generator
    std::uniform_int_distribution<> distr(0, str_answers.size() - 1); // define the range
    std::string random_choice = str_answers[distr(eng)]; // generate numbers
    return random_choice;
}
/*
    end nemsLib--------------------------------------------------------------------------------------------------------
*/
/*
    Start CallbackExp library
*/
void CallbackExp::callbackFunction(const std::vector<std::string>& condition) {
	for(const auto& con : condition){
		std::cout << con << std::endl;
	}
}
void CallbackExp::initializeCallbacks(std::unordered_map<std::string, std::vector<std::string>>& states) {
    if (!states.empty()) {
        for (const auto& state : states) {
            CallbackExp::callbacks[state.first] = std::bind(&CallbackExp::callbackFunction, this, std::cref(state.second));
        }
    }
}
void CallbackExp::processState(const std::string& state, const std::vector<std::string>& condition) {
    auto it = CallbackExp::callbacks.find(state);
    if (it != CallbackExp::callbacks.end()) {
        CallbackFunction callback = it->second;
        callback(condition);
    } else {
        std::cout << "No callback defined for the current state" << std::endl;
    }
}
/*
    End CallbackExp library
*/
/*
    start SysLogLib
*/
/*
    get current date and time
*/
SysLogLib::CurrentDateTime SysLogLib::getCurrentDateTime() {
    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::tm* current_time_tm = std::localtime(&current_time_t);
    SysLogLib::CurrentDateTime currentDateTime;
    currentDateTime.current_date = std::to_string(current_time_tm->tm_year + 1900) + "-" + std::to_string(current_time_tm->tm_mon + 1) + "-" + std::to_string(current_time_tm->tm_mday);
    currentDateTime.current_time = std::to_string(current_time_tm->tm_hour) + ":" + std::to_string(current_time_tm->tm_min) + ":" + std::to_string(current_time_tm->tm_sec);
    return currentDateTime;
}
/*
    write system log
*/
void SysLogLib::sys_timedelay(size_t& mini_sec){
    std::this_thread::sleep_for(std::chrono::milliseconds(mini_sec));
}
void SysLogLib::writeLog(const std::string& logpath, const std::string& log_message) {
    if(logpath.empty() || log_message.empty()){
        std::cerr << "SysLogLib::writeLog input empty!" << '\n'; 
        return;
    }
    std::string strLog = logpath;
    if(strLog.back() != '\n'){
        strLog.append("/");
    }
    if (!std::filesystem::exists(strLog)) {
        try {
            std::filesystem::create_directory(strLog);
        } catch (const std::exception& e) {
            std::cout << "Error creating log folder: " << e.what() << std::endl;
            return;
        }
    }
    SysLogLib::CurrentDateTime currentDateTime = SysLogLib::getCurrentDateTime();
    strLog.append(currentDateTime.current_date);
    strLog.append(".txt");
    std::ofstream file(strLog, std::ios::app);
    if (!file.is_open()) {
        file.open(strLog, std::ios::app);
    }
    file << currentDateTime.current_time + " : " + log_message << std::endl;
    file.close();
    std::cout << currentDateTime.current_date << " " << currentDateTime.current_time << " : " << log_message << std::endl;
}
void SysLogLib::chatCout(const std::string& chatTxt){
    size_t t_delay = 23;
    for (char c : chatTxt) {
        std::cout << c << std::flush;
        SysLogLib::sys_timedelay(t_delay);
    }
    std::cout << std::endl;
}
/*
    end SysLogLib
*/
/*
    start WebSpiderLib
*/
size_t WebSpiderLib::WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output){
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}
std::string_view WebSpiderLib::str_trim(std::string_view str){
    // Find the first non-whitespace character from the beginning
    auto start = str.find_first_not_of(" \t\n\r\f\v");
    // If the string is empty or contains only whitespace, return an empty string
    if (start == std::string_view::npos) {
        return {};
    }
    // Find the first non-whitespace character from the end
    auto end = str.find_last_not_of(" \t\n\r\f\v");
    // Return the trimmed substring
    return str.substr(start, end - start + 1);
}
std::string WebSpiderLib::removeHtmlTags(const std::string& input){
    // Regular expression to match HTML tags
    std::regex htmlRegex("<[^>]*>");
    // Replace HTML tags with an empty string
    std::string result = std::regex_replace(input, htmlRegex, "");
    return result;
}
std::vector<std::string> WebSpiderLib::findAllWordsBehindSpans(const std::string& input, const std::string& strreg){
    std::regex regexPattern(strreg);
    std::smatch match;
    std::vector<std::string> words;
    std::sregex_iterator it(input.begin(), input.end(), regexPattern);
    std::sregex_iterator end;
    while (it != end) {
        words.push_back((*it)[1].str());
        ++it;
    }
    return words;
}
std::vector<std::string> WebSpiderLib::findAllSpans(const std::string& input, const std::string& strreg){
    std::vector<std::string> spans;
    std::regex regexPattern(strreg);
    std::smatch match;
    std::string::const_iterator searchStart(input.cbegin());
    while (std::regex_search(searchStart, input.cend(), match, regexPattern)) {
        spans.push_back(match[0].str());
        searchStart = match.suffix().first;
    }
    return spans;
}
std::string WebSpiderLib::findWordBehindSpan(const std::string& input, const std::string& strreg){
    std::regex regexPattern(strreg);
    std::smatch match;
    if (std::regex_search(input, match, regexPattern)) {
        return match[1].str();
    }
    return "";
}
std::string WebSpiderLib::GetURLContent(const std::string& url){
    curl_global_init(CURL_GLOBAL_DEFAULT);
	CURL* curl = curl_easy_init();
	std::string output;
	if (curl) {
		curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, &WebSpiderLib::WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
        // Set the allowed redirection protocols to HTTP and HTTPS
        curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, CURLPROTO_HTTP | CURLPROTO_HTTPS);
        // For completeness
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 20000L); // Using milliseconds for timeout
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
        curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)1024*1024*1024);
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt"); // Use a cookie file if needed
        curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
        // Set a proper User-Agent header
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3");
        // Authentication settings
        curl_easy_setopt(curl, CURLOPT_HTTPAUTH, CURLAUTH_ANY);
        curl_easy_setopt(curl, CURLOPT_UNRESTRICTED_AUTH, 1L);
        curl_easy_setopt(curl, CURLOPT_PROXYAUTH, CURLAUTH_ANY);
        curl_easy_setopt(curl, CURLOPT_EXPECT_100_TIMEOUT_MS, 0L);
        // Disable SSL verification (not recommended for production)
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 0L);
        curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 0L);
		CURLcode res = curl_easy_perform(curl);
		if (res != CURLE_OK) {
			std::cerr << "Error: " << curl_easy_strerror(res) << std::endl;
		}
		curl_easy_cleanup(curl);
	}
	curl_global_cleanup();
	return output;
}
std::string WebSpiderLib::GetURLContentAndSave(const std::string& url, const std::string& strReg){
    /*
        if a word was successfully crawled, then return 1
    */
    size_t word_was_found = 0;
    /*
        move the cursor to the first place
    */
    // Read the HTML file
    //std::ifstream file("example.html");
    //std::string htmlContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<std::string> web_content;
    if(url.empty() || strReg.empty()){
        std::cerr << "WebSpiderLib::GetURLContentAndSave input empty!" << '\n';
        return "";
    }
    std::string htmlContent = this->GetURLContent(url);
    if(htmlContent.length() > 0){
        word_was_found = 1;
    }else{
        return "";
    }
    web_content =  this->findAllWordsBehindSpans(htmlContent,strReg);
	std::string web_content_preprocess;
	for(std::string& em : web_content){
		em = this->removeHtmlTags(em);
        web_content_preprocess += em + "\n";
	}
    return web_content_preprocess;
}
std::string WebSpiderLib::removeHtmlTags_google(GumboNode* node){
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT &&
                node->v.element.tag != GUMBO_TAG_SCRIPT &&
                node->v.element.tag != GUMBO_TAG_STYLE) {
        std::string contents;
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            const std::string text = WebSpiderLib::removeHtmlTags_google(static_cast<GumboNode*>(children->data[i]));
            if (i != 0 && !text.empty()) {
                contents.append(" ");
            }
            contents.append(text);
        }
        return contents;
    } else {
        return "";
    }
}
std::string WebSpiderLib::G_removehtmltags(std::string& HTML_DOC){
	WebSpiderLib websl_j;
	GumboOutput* output = gumbo_parse(HTML_DOC.c_str());
	std::string strOut =  websl_j.removeHtmlTags_google(output->root);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	return strOut;
}
std::string WebSpiderLib::extractDomainFromUrl(const std::string& url){
    std::regex domainRegex(R"(?:https?:\/\/)?(?:www\.)?([^\/]+)\.([a-zA-Z]{2,}(?=/))");
    std::smatch match;
    if (std::regex_search(url, match, domainRegex)) {
		std::string fullDomain = match[1];
		size_t pos = fullDomain.find("://");
		if(pos != std::string::npos){
			return fullDomain.substr(pos+3);
		}
    } 
	return "";
}
/*
    end WebSpiderLib
*/
/*
    processing time spend counter
*/
std::chrono::milliseconds ProcessTimeSpent::time_start() {
    auto timestart = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(timestart.time_since_epoch());
}

std::chrono::milliseconds ProcessTimeSpent::time_end() {
    auto timeend = std::chrono::high_resolution_clock::now();
    return std::chrono::duration_cast<std::chrono::milliseconds>(timeend.time_since_epoch());
}
/*
    nlp_lib--start
*/
/*
    read vocabulary from db and write into a binary file
*/
void nlp_lib::writeBinaryFile(const std::vector<Mdatatype>& data, const std::string& filename){
    std::ofstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        file.open(filename, std::ios::binary);
    }
    uint32_t size = static_cast<uint32_t>(data.size());
    file.write(reinterpret_cast<const char*>(&size), sizeof(uint32_t));
    for (const Mdatatype& dt : data) {
        uint32_t wordSize = static_cast<uint32_t>(dt.word.size());
        file.write(reinterpret_cast<const char*>(&wordSize), sizeof(uint32_t));
        file.write(dt.word.c_str(), wordSize);

        uint32_t wordTypeSize = static_cast<uint32_t>(dt.word_type.size());
        file.write(reinterpret_cast<const char*>(&wordTypeSize), sizeof(uint32_t));
        file.write(dt.word_type.c_str(), wordTypeSize);

        uint32_t meaningEnSize = static_cast<uint32_t>(dt.meaning_en.size());
        file.write(reinterpret_cast<const char*>(&meaningEnSize), sizeof(uint32_t));
        file.write(dt.meaning_en.c_str(), meaningEnSize);

        uint32_t meaningZhSize = static_cast<uint32_t>(dt.meaning_zh.size());
        file.write(reinterpret_cast<const char*>(&meaningZhSize), sizeof(uint32_t));
        file.write(dt.meaning_zh.c_str(), meaningZhSize);
    }
    file.close();
    std::cout << "Successfully created the binary file: " + filename;
}
std::optional<Mdatatype> nlp_lib::readValueFromBinaryFile(const std::string& filename, const std::string& key) {
	if(filename.empty() || key.empty()){
		std::cerr << "nlp_lib::readValueFromBinaryFile: input empty!" << '\n';
		return std::nullopt;
	}
    std::ifstream file(filename, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "readValueFromBinaryFile::Failed to open file for reading: " << filename << std::endl;
        return std::nullopt;
    }
    uint32_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        uint32_t wordSize;
        file.read(reinterpret_cast<char*>(&wordSize), sizeof(uint32_t));
        std::string word(wordSize, '\0');
        file.read(&word[0], wordSize);

        uint32_t wordTypeSize;
        file.read(reinterpret_cast<char*>(&wordTypeSize), sizeof(uint32_t));
        std::string wordType(wordTypeSize, '\0');
        file.read(&wordType[0], wordTypeSize);

        uint32_t meaningEnSize;
        file.read(reinterpret_cast<char*>(&meaningEnSize), sizeof(uint32_t));
        std::string meaningEn(meaningEnSize, '\0');
        file.read(&meaningEn[0], meaningEnSize);

        uint32_t meaningZhSize;
        file.read(reinterpret_cast<char*>(&meaningZhSize), sizeof(uint32_t));
        std::string meaningZh(meaningZhSize, '\0');
        file.read(&meaningZh[0], meaningZhSize);
        if (word == key) {
            Mdatatype dt;
            dt.word = word;
            dt.word_type = wordType;
            dt.meaning_en = meaningEn;
            dt.meaning_zh = meaningZh;
            std::optional<Mdatatype> result;
            result = dt;
            return result;
        }
    }
    file.close();
    return std::nullopt;
}
std::vector<Mdatatype> nlp_lib::readBinaryFile(const std::string& binaryPath){
    std::vector<Mdatatype> exp_list;
    if(binaryPath.empty()){
        std::cerr << "readBinaryFile::input empty: " << binaryPath << std::endl;
        return exp_list;
    }
    std::ifstream file(binaryPath, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "readBinaryFile::Failed to open file for reading: " << binaryPath << std::endl;
        return exp_list;
    }
    uint32_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        uint32_t wordSize;
        file.read(reinterpret_cast<char*>(&wordSize), sizeof(uint32_t));
        std::string word(wordSize, '\0');
        file.read(&word[0], wordSize);

        uint32_t wordTypeSize;
        file.read(reinterpret_cast<char*>(&wordTypeSize), sizeof(uint32_t));
        std::string wordType(wordTypeSize, '\0');
        file.read(&wordType[0], wordTypeSize);

        uint32_t meaningEnSize;
        file.read(reinterpret_cast<char*>(&meaningEnSize), sizeof(uint32_t));
        std::string meaningEn(meaningEnSize, '\0');
        file.read(&meaningEn[0], meaningEnSize);

        uint32_t meaningZhSize;
        file.read(reinterpret_cast<char*>(&meaningZhSize), sizeof(uint32_t));
        std::string meaningZh(meaningZhSize, '\0');
        file.read(&meaningZh[0], meaningZhSize);
        Mdatatype dt;
        dt.word = word;
        dt.word_type = wordType;
        dt.meaning_en = meaningEn;
        dt.meaning_zh = meaningZh;
        exp_list.push_back(dt);
    }
    file.close();
    return exp_list;
}
void nlp_lib::get_english_voc_and_create_bin(const std::string& sqlite3_db_file_path){
    std::cout << "start reading data from db..." << std::endl;
    std::vector<std::vector<std::string>> result;
    std::vector<Mdatatype> str_exp;
    if(sqlite3_db_file_path.empty()){
        std::cerr << "get_english_voc_and_create_bin: input empty!" << '\n';
        return;
    }
    SQLite3Library db(sqlite3_db_file_path);//"/home/ronnieji/lib/res/catalogdb.db");
    db.connect();
    db.executeQuery("SELECT word, word_type, meaning_en, meaning_zh FROM english_voc", result);
    db.disconnect();
    if (!result.empty()) {
        for (const auto& row : result) {
            Mdatatype db_data;
            db_data.word = row[0];
            db_data.word_type = row[1];
            db_data.meaning_en = row[2];
            db_data.meaning_zh = row[3];
            str_exp.push_back(db_data);
            std::cout << "Appending data: " << row[0] << std::endl;
        }
        std::cout << "start writing data into binary file..." << std::endl;
        /*
            change the file extension to *.bin
        */
        std::string f_output = sqlite3_db_file_path + "_.bin";
        nlp_lib::writeBinaryFile(str_exp, f_output); //"/home/ronnieji/lib/res/catalogdb.bin");
    }
}
/*
    write nlp exp binary file
*/
void nlp_lib::writeBinaryFileForNLP(const std::string& txt_list_path){
    Jsonlib jl_j;
    if(txt_list_path.empty()){
        std::cerr << "writeBinaryFileForNLP:Input string is empty!" << '\n';
        return;
    }
    std::string output_file_path = txt_list_path;
    std::vector<std::string> split_names;
    std::string input_file_name;
    split_names = jl_j.splitString(output_file_path,'/');
    if(!split_names.empty()){
        input_file_name = split_names.back();//get the last part
        std::vector<std::string> get_file_name;
        get_file_name = jl_j.splitString(input_file_name,'.');
        if(!get_file_name.empty()){
            std::string str_replacement = get_file_name[0];
            str_replacement.append(".bin");
            output_file_path = jl_j.str_replace(output_file_path,input_file_name,str_replacement);
        }
    }
    std::ifstream inFile(txt_list_path);
    if(!inFile.is_open()){
        std::cerr << "writeBinaryFileForNLP:Failed to open file: " + txt_list_path << '\n';
        return;
    }
    /*
        read the text content from the file
    */
    std::string line;
    std::vector<std::string> raw_txt_content;
    std::vector<exp_data2> str_exp;
    std::vector<std::string> trans_set;
    std::unordered_map<std::string,std::string> trans_vec;
    while(std::getline(inFile, line)){
        raw_txt_content.push_back(line); //+ "^~&";
        trans_set = jl_j.splitString_bystring(line,"^~&");
    }
    if(!trans_set.empty()){
        trans_vec[trans_set[0]] = trans_set[1];
    }
    inFile.close();
    for(const auto& rt : raw_txt_content){
        std::vector<std::string> get_line_exp = jl_j.splitString_bystring(rt,"^~&");
        if(!get_line_exp.empty()){
            exp_data2 str_bi;
            str_bi.exp_key = get_line_exp[0];
            str_bi.exp_value = get_line_exp[1];
            str_exp.push_back(str_bi);
        }
    }
    /*
        start creating binary file and output
    */
    std::ofstream file(output_file_path, std::ios::binary);
    if (!file.is_open()) {
        file.open(output_file_path,std::ios::binary);
    }
    uint32_t total_size = static_cast<uint32_t>(str_exp.size());
    file.write(reinterpret_cast<const char*>(&total_size), sizeof(uint32_t));
    for (const auto& dt : str_exp) {

        uint32_t exp_keySize = static_cast<uint32_t>(dt.exp_key.size());
        file.write(reinterpret_cast<const char*>(&exp_keySize), sizeof(uint32_t));
        file.write(dt.exp_key.c_str(), exp_keySize);

        uint32_t exp_valueSize = static_cast<uint32_t>(dt.exp_value.size());
        file.write(reinterpret_cast<const char*>(&exp_valueSize), sizeof(uint32_t));
        file.write(dt.exp_value.c_str(), exp_valueSize);

    }
    file.close();
    std::cout << "Successfully created the binary file in: " + output_file_path << '\n';
}
std::unordered_map<std::string,std::string> nlp_lib::get_binary_file_for_nlp(const std::string& binary_file_path){
    std::unordered_map<std::string,std::string> exp_list;
    if(binary_file_path.empty()){
        std::cerr << "get_binary_file_for_nlp::input empty: " << binary_file_path << std::endl;
        return exp_list;
    }
    std::ifstream file(binary_file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "get_binary_file_for_nlp::Failed to open file for reading: " << binary_file_path << std::endl;
        return exp_list;
    }
    uint32_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        uint32_t exp_keySize;
        file.read(reinterpret_cast<char*>(&exp_keySize), sizeof(uint32_t));
        std::string exp_key(exp_keySize, '\0');
        file.read(&exp_key[0], exp_keySize);

        uint32_t exp_valueSize;
        file.read(reinterpret_cast<char*>(&exp_valueSize), sizeof(uint32_t));
        std::string exp_value(exp_valueSize, '\0');
        file.read(&exp_value[0], exp_valueSize);

        exp_list[exp_key] = exp_value;
    }
    file.close();
    return exp_list;
}
/*
    read lines(1) from a txt file and create a binary file
*/
void nlp_lib::WriteBinaryOne_from_txt(const std::string& str_txt_file_path){
    if(str_txt_file_path.empty()){
        std::cerr << "nlp_lib::WriteBinaryOne_from_txt input empty!" << '\n';
        return;
    }
    std::ifstream file(str_txt_file_path);
    if (!file.is_open()) {
        std::cerr << "nlp_lib::WriteBinaryOne_from_txt Failed to open file for reading: " << str_txt_file_path << std::endl;
        return;
    }
    std::string line;
    std::vector<std::string> getTxtLines;
    while(std::getline(file,line)){
        getTxtLines.push_back(line);
    }
    file.close();
    /*
        start creating binary file and output
    */
    std::string output_file_path = str_txt_file_path;
    output_file_path.append("_.bin");
    std::ofstream outfile(output_file_path, std::ios::binary);
    if (!outfile.is_open()) {
        outfile.open(output_file_path,std::ios::binary);
    }
    uint32_t total_size = static_cast<uint32_t>(getTxtLines.size());
    outfile.write(reinterpret_cast<const char*>(&total_size), sizeof(uint32_t));
    for (const auto& dt : getTxtLines) {
        uint32_t exp_valueSize = static_cast<uint32_t>(dt.size());
        outfile.write(reinterpret_cast<const char*>(&exp_valueSize), sizeof(uint32_t));
        outfile.write(dt.c_str(), exp_valueSize);
    }
    outfile.close();
    std::cout << "Successfully created the binary file in: " + output_file_path << '\n';
}
/*
    output a data set to a binary file
*/
void nlp_lib::WriteBinaryOne_from_std(const std::vector<std::string>& dataList, const std::string& output_file_path){
    if(dataList.empty() || output_file_path.empty()){
        std::cerr << "nlp_lib::WriteBinaryOne_from_std input empty!" << '\n';
        return;
    }
    /*
        start creating binary file and output
    */
    std::ofstream outfile(output_file_path, std::ios::binary);
    if (!outfile.is_open()) {
        outfile.open(output_file_path,std::ios::binary);
    }
    uint32_t total_size = static_cast<uint32_t>(dataList.size());
    outfile.write(reinterpret_cast<const char*>(&total_size), sizeof(uint32_t));
    for (const auto& dt : dataList) {
        uint32_t exp_valueSize = static_cast<uint32_t>(dt.size());
        outfile.write(reinterpret_cast<const char*>(&exp_valueSize), sizeof(uint32_t));
        outfile.write(dt.c_str(), exp_valueSize);
    }
    outfile.close();
    std::cout << "Successfully created the binary file in: " + output_file_path << '\n';
}
std::vector<std::string> nlp_lib::ReadBinaryOne(const std::string& str_txt_file_path){
    std::vector<std::string> exp_list;
    if(str_txt_file_path.empty()){
        std::cerr << "nlp_lib::ReadBinaryOne input empty!" << '\n';
        return exp_list;
    }
    std::ifstream file(str_txt_file_path, std::ios::binary);
    if (!file.is_open()) {
        std::cerr << "nlp_lib::ReadBinaryOne Failed to open file for reading: " << str_txt_file_path << std::endl;
        return exp_list;
    }
    uint32_t size;
    file.read(reinterpret_cast<char*>(&size), sizeof(uint32_t));
    for (uint32_t i = 0; i < size; i++) {
        uint32_t exp_valueSize;
        file.read(reinterpret_cast<char*>(&exp_valueSize), sizeof(uint32_t));
        std::string exp_value(exp_valueSize, '\0');
        file.read(&exp_value[0], exp_valueSize);
        exp_list.push_back(exp_value);
    }
    file.close();
    return exp_list;
}
void nlp_lib::AppendBinaryOne(const std::string& str_txt_file_path,const std::string& strContent){
    if(str_txt_file_path.empty() || strContent.empty()){
        std::cerr << "nlp_lib::AppendBinaryOne input empty!" << '\n';
        return;
    }
    std::vector<std::string> exp_list;
    exp_list = this->ReadBinaryOne(str_txt_file_path);
    if(!exp_list.empty()){
        auto it = std::find_if(exp_list.begin(),exp_list.end(),[strContent](const std::string& s){ return s == strContent;});
        if(it != exp_list.end()){
            /*
                the record has already existed.
            */
            return;
        }
        exp_list.push_back(strContent);
        this->WriteBinaryOne_from_std(exp_list,str_txt_file_path);
    }
    else{
        /*
            The binary file does not exists, you have to create one
        */
        exp_list.push_back(strContent);
        this->WriteBinaryOne_from_std(exp_list,str_txt_file_path);
    }
}
void nlp_lib::AppendBinaryOne_allow_repeated_items(const std::string& str_txt_file_path,const std::string& strContent){
    if(str_txt_file_path.empty() || strContent.empty()){
        std::cerr << "nlp_lib::AppendBinaryOne input empty!" << '\n';
        return;
    }
    std::vector<std::string> exp_list;
    exp_list = this->ReadBinaryOne(str_txt_file_path);
    if(!exp_list.empty()){
        exp_list.push_back(strContent);
        this->WriteBinaryOne_from_std(exp_list,str_txt_file_path);
    }
    else{
        /*
            The binary file does not exists, you have to create one
        */
        exp_list.push_back(strContent);
        this->WriteBinaryOne_from_std(exp_list,str_txt_file_path);
    }
}
std::vector<std::string> nlp_lib::remove_repeated_items_vs(const std::vector<std::string>& vec){
    std::vector<std::string> const_vec(vec.begin(), vec.end());
    try{
        // Create a non-const copy of the vect
        // Sort the vector
        std::sort(const_vec.begin(), const_vec.end());
        // Use std::unique to remove adjacent duplicates
        auto it = std::unique(const_vec.begin(), const_vec.end());
        // Erase the elements after the unique ones
        const_vec.erase(it, const_vec.end());
    }
    catch(const std::exception& e){
        std::cerr << "Function:remove_repeated_items_vsError: " << e.what() << std::endl;
    }
    return const_vec;
}
/*
    count the frequency of a string in a string
    para1: main string 
    para2: query string (string to search in main string)
*/
unsigned int nlp_lib::get_number_of_substr_in_str(const std::string& mainStr, const std::string& subStr){
    if(mainStr.empty() || subStr.empty()){
        std::cerr << "nlp_lib::get_number_of_substr_in_str input empty!" << '\n';
        return 0;
    }
    unsigned int count = 0;
    size_t pos = 0;
    while ((pos = mainStr.find(subStr, pos)) != std::string::npos) {
        pos += subStr.length();
        count++;
    }
    return count;
}
/*
    get numbers, prices etc.in a string
*/
std::vector<std::string> nlp_lib::get_numbers(const std::string& str_in){
    /*
        get all digits in str_in
    */
    std::vector<std::string> str_nums;
    std::string str_num_temp = "";
    for(const auto& c : str_in){
        if(c != ' ' && c != ':' && c != '.'){// get the numbers from price format .99, 99.99 etc.
            if(std::isdigit(c)){
                str_num_temp += c;
            }
        }
        else{//it's a space
            if(!str_num_temp.empty()){
                str_nums.push_back(str_num_temp);
                str_num_temp = "";
            }
        }
    }
    return str_nums;
}
/*

*/
void nlp_lib::write_books(const std::string& input_folder_path,const std::string& bookbin_path, 
const std::string& binaryOnePath,const std::string& stopwordListPath,const std::string& output_log_path){
    if(input_folder_path.empty() || bookbin_path.empty() || stopwordListPath.empty() || output_log_path.empty()){
        std::cerr << "read_books input empty!" << '\n';
        return;
    }
    std::unordered_set<std::string> ProcessFileNames;
    WebSpiderLib jsl_j;
    nemslib nems_j;/* initialize stop word*/
    SysLogLib syslog_j;
    syslog_j.writeLog(output_log_path,"Initialize stopword list...");
    nems_j.set_stop_word_file_path(stopwordListPath);
    for (const auto& entry : std::filesystem::directory_iterator(input_folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt" && ProcessFileNames.find(entry.path()) == ProcessFileNames.end()) {
            std::ifstream file(entry.path());
            syslog_j.writeLog(output_log_path,"Reading book: ");
            syslog_j.writeLog(output_log_path,entry.path());
            if (file.is_open()) {
                std::string line;
                std::string str_book;
                std::vector<std::string> book_token;
                std::vector<std::string> book_token_xy;
                std::vector<std::unordered_map<std::string,unsigned int>> get_topics_withf;
                std::string st_book_temp;
                std::string str_book_saved;
                std::string str_topics_without_freq;
                std::string str_topics_with_freq;
                std::string book_y;//y-coordinate
                while (std::getline(file, line)) {
                    line = std::string(jsl_j.str_trim(line));
                    str_book += line + '\n';
                }
                syslog_j.writeLog(output_log_path,"Getting the book's topic...");
                /*
                    check tagging
                */
                str_book_saved = entry.path().stem().string();//.filename().string();
                str_book_saved.append("^~&");
                /*
                    get the topics of the article
                */
                get_topics_withf = nems_j.get_topics_freq(str_book);
                syslog_j.writeLog(output_log_path,"Printing out the book's topic...");
                for(const auto& gtw : get_topics_withf){
                    for(const auto& gt : gtw){
                        std::stringstream sss;
                        sss << gt.first << "-" << gt.second << ",";
                        str_topics_with_freq += sss.str();
                        str_topics_without_freq += gt.first + ",";
                        std::stringstream ss;
                        ss << "Topic: " << gt.first << " Freq: " << gt.second << '\n';
                        syslog_j.writeLog(output_log_path,ss.str());
                        std::cout << ss.str() << '\n';
                    }
                }
                str_topics_without_freq = str_topics_without_freq.substr(0,str_topics_without_freq.size()-1);
                str_topics_with_freq = str_topics_with_freq.substr(0,str_topics_with_freq.size()-1);
                str_book_saved.append(str_topics_without_freq);
                str_book_saved.append("^~&");
                str_book_saved.append(str_topics_with_freq);
                str_book_saved.append("^~&");
                /*
                    start saving the binary
                */
                syslog_j.writeLog(output_log_path,"Tokenizing the book...");
                book_token = nems_j.tokenize_en(str_book);
                if(!book_token.empty()){
                    syslog_j.writeLog(output_log_path,"Adding the book to the library...");
                    for(const auto& bt : book_token){
                        try{
                            /*
                                check binary one
                            */
                            std::vector<std::string> binary_one = this->ReadBinaryOne(binaryOnePath);
                            if(!binary_one.empty()){
                                auto it = std::find_if(binary_one.begin(),binary_one.end(),[bt](const std::string& s){ return s == bt;});
                                if(it != binary_one.end()){
                                    unsigned int pos = std::distance(binary_one.begin(), it);
                                    st_book_temp = std::to_string(pos);
                                    book_token_xy.push_back(st_book_temp);
                                    std::stringstream ss;
                                    ss << "st_book_temp>> " << st_book_temp << '\n';
                                    syslog_j.writeLog(output_log_path,ss.str());
                                }
                                else{/*it's a new word*/
                                    /*
                                        save it in binary one
                                    */
                                    binary_one.push_back(bt);
                                    this->WriteBinaryOne_from_std(binary_one,binaryOnePath);
                                    unsigned int pos = binary_one.size()-1;
                                    st_book_temp = std::to_string(pos);
                                    book_token_xy.push_back(st_book_temp);
                                    std::stringstream ss;
                                    ss << "st_book_temp>> " << st_book_temp << '\n';
                                    syslog_j.writeLog(output_log_path,ss.str());
                                }
                                /*
                                    output the file
                                    output_folder_path + entry.path().filename()
                                */
                                if(!book_token_xy.empty()){
                                    for(const auto& bt : book_token_xy){
                                        book_y += bt + ",";
                                    }
                                }
                                book_y = book_y.substr(0,book_y.size()-1);
                            }
                            else{/* start creating a new binary*/
                                book_y = bt;
                                binary_one.push_back(bt);
                                this->WriteBinaryOne_from_std(binary_one,binaryOnePath);
                            }
                        }
                        catch(const std::exception& e){
                            std::cerr << e.what() << '\n';
                        }
                    }
                }
                str_book_saved.append(book_y);
                this->AppendBinaryOne(bookbin_path,str_book_saved);
                std::stringstream ss;
                ss << "Book: " << entry.path().filename().string() << " was successfully saved!" << '\n';
                syslog_j.writeLog(output_log_path,ss.str());
                /*
                    add the file to Processed list
                */
                ProcessFileNames.insert(entry.path());
            }
            file.close();
        }
    }
    /*
        Check new files, and process the new file
    */
    for (const auto& entry : std::filesystem::directory_iterator(input_folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt" && ProcessFileNames.find(entry.path()) == ProcessFileNames.end()){
            this->write_books(input_folder_path,bookbin_path,binaryOnePath,stopwordListPath,output_log_path);
            return;
        }
    }
}
std::vector<std::string> nlp_lib::read_books(const std::string& books_bin_path){
    std::vector<std::string> str_book;
    if(books_bin_path.empty()){
        std::cerr << "nlp_lib::read_books input empty!" << '\n';
        return str_book;
    }
    str_book = this->ReadBinaryOne(books_bin_path);
    return str_book;
}
/*
    nlp_lib--end;
*/
