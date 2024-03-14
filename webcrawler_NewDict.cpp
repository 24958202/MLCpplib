/*
    webcrawler to crawle the online dictionary to check the input word type:

    Noun(NN): A word that represents a person, place, thing, or idea. Examples: cat, city, love.

    Verb(VB): A word that describes an action, occurrence, or state of being. Examples: run, eat, sleep.

    Adjective(AJ): A word that describes or modifies a noun. Examples: happy, tall, beautiful.

    Adverb(AV): A word that describes or modifies a verb, adjective, or other adverb. Examples: quickly, very, well.

    Pronoun(PN): A word that takes the place of a noun. Examples: he, she, they.

    Preposition(PP): A word that shows the relationship between a noun/pronoun and other words in a sentence. Examples: in, on, at.

    Conjunction(CN): A word that connects words, phrases, or clauses. Examples: and, but, or.

    Interjection(IT): A word or phrase that expresses strong emotion. Examples: wow, oh, hooray.

    Determiner(DT): A word that provides information about a noun. Examples: the, a, this.

    Article(AT): A type of determiner used to specify or generalize a noun. Examples: a, an, the.

    ---------------------------------------------------------------------------------------------------------------------------------------------------------
    
    Compile:
    g++ /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/webcrawler_NewDict.cpp -o /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/webcrawler_NewDict -I/usr/local/include/eigen3/ -I/usr/local/boost/include/ -I/opt/homebrew/Cellar/icu4c/73.2/include/ -L/opt/homebrew/Cellar/icu4c/73.2/lib -licuuc -licudata /usr/local/boost/lib/libboost_system.a /usr/local/boost/lib/libboost_filesystem.a /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/lib/nemslib.a -I/Users/dengfengji/miniconda3/pkgs/sqlite-3.41.2-h80987f9_0 -lsqlite3 -I/Volumes/WorkDisk/cppLibs/curl/include/ -lcurl -I/opt/homebrew/Cellar/gumbo-parser/0.10.1/include /opt/homebrew/Cellar/gumbo-parser/0.10.1/lib/libgumbo.a -std=c++20
*/
#include <iostream>
#include <string>
#include <curl/curl.h>
#include <regex>
#include <sqlite3.h>
#include <vector>
#include <fstream>
#include <chrono>
#include <thread>
#include <functional>
#include <locale>
#include <codecvt>
#include <string_view>
#include <iomanip>
#include <unordered_map>
#include <gumbo.h>
#include <cctype>
#include "lib/nemslib.h"

struct CurrentDateTime{
    std::string current_date;
    std::string current_time;
};
/*
	Define the return value type
*/
struct return_str_value{
	std::string str_past;
	std::string str_past_perfect;
};

/*
	Abbrevation of word type transfer
*/
std::unordered_map<std::string, std::string> get_word_type_abv(){
	std::unordered_map<std::string, std::string> get_wordtypes;
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
	db.connect();
	std::vector<std::vector<std::string>> dbresult;
	db.executeQuery("select type_name, type_abv from type_table",dbresult);
	db.disconnect();
	if(!dbresult.empty()){
		for(const auto& res : dbresult){
			get_wordtypes[res[0]] = res[1];
		}
	}
	return get_wordtypes;
}

/*
    get current date and time
*/
CurrentDateTime getCurrentDateTime() {
    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::tm* current_time_tm = std::localtime(&current_time_t);
    CurrentDateTime currentDateTime;
    currentDateTime.current_date = std::to_string(current_time_tm->tm_year + 1900) + "-" + std::to_string(current_time_tm->tm_mon + 1) + "-" + std::to_string(current_time_tm->tm_mday);
    currentDateTime.current_time = std::to_string(current_time_tm->tm_hour) + ":" + std::to_string(current_time_tm->tm_min) + ":" + std::to_string(current_time_tm->tm_sec);
    return currentDateTime;
}

/*
    write system log
*/
void writeLog(const std::string& log_message) {
    std::string folderPath = "/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/log";
    if(!std::filesystem::exists(folderPath)){
        std::ofstream file(folderPath, std::ios::app);
    }
    CurrentDateTime currentDateTime = getCurrentDateTime();
    std::ofstream file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/log/" + currentDateTime.current_date + ".txt", std::ios::app);
    if (file.is_open()) {
        file << currentDateTime.current_time + " : " + log_message << std::endl;
    } else {
        std::cout << "Unable to open the log file." << std::endl;
    }
    file.close();
    std::cout << currentDateTime.current_date << " " << currentDateTime.current_time << " : " << log_message << std::endl;
}
/*
    callback function
*/
using CallbackFunction = std::function<void(const std::vector<std::string>&, const std::unordered_map<std::string,std::string>&)>;
/*
	Start crawling 
*/
void getPastParticiple(const std::string& strurl, std::string& strWord){
	std::vector<std::vector<std::string>> dbresult;
	WebSpiderLib wSpider_j;
	Jsonlib jsl_j;
	jsl_j.toLower(strWord);
	std::string strWord_lower = strWord;
	std::cout << "String input: --->" << strWord_lower << std::endl;
	std::string htmlContent = wSpider_j.GetURLContent(strurl);
	// Find the position of the content attribute
    size_t contentPos = htmlContent.find("<meta name=\"description\" content=\"");
    if(contentPos != std::string::npos) {
        // Find the end of the content attribute
        size_t endQuotePos = htmlContent.find("\"", contentPos + 36); // 9 is the length of "content=\""
        if(endQuotePos != std::string::npos) {
            // Extract the content
            std::string content = htmlContent.substr(contentPos + 36, endQuotePos - contentPos - 36);
            std::ranges::transform(strWord, strWord.begin(), [](char c) {
				return std::toupper(c);
			});
            std::vector<std::string > non_ant = jsl_j.splitString_bystring(content,"Antonyms of "+ strWord +":");
            std::string strNon;
            std::string strAnt;
          	if(!non_ant.empty()){
          		std::vector<std::string> strNons = jsl_j.splitString(non_ant[0],':');
          		strNon = strNons[1];
          		strNon = jsl_j.trim(strNon);
          		strNon = strNon.substr(0,strNon.size()-1);
          		strAnt = jsl_j.trim(non_ant[1]);
          	}
            std::cout << "Successfully get: " << strWord_lower << "-> "<< strNon << "| Ant: " << strAnt << std::endl;
            /*
            	save to database
            */
			// SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
// 			db.connect();
// 			db.executeQuery("insert into raw_stories(story_txt,story_topics,Resv)values('" + strWord_lower + "','" + strNon + "','" + strAnt + "')", dbresult);
// 			db.disconnect();
// 			std::cout << "Successfully saved: " << strNon << " | " << strAnt <<  "into database!" << std::endl;   
        }
    }
}
void look_for_past_participle_of_word(){
	SysLogLib sys_j;
	Jsonlib jsonl_j;
	std::vector<std::vector<std::string>> dbresult;
	size_t td = 5000;
	writeLog("Start looking for words...");
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
	db.connect();
	db.executeQuery("select * from english_voc where word_type='VB' order by id asc",dbresult);
	db.disconnect();
	if(!dbresult.empty()){
		for(const auto& rw : dbresult){
			/*
				rw[0]=id,
				rw[1]=word,
				rw[2]=word_type,
				rw[3]=meaning_en,
				rw[4]=meaning_zh
				
				check the word's past participle and past perfect participle
			*/
				std::string strWord = std::string(jsonl_j.trim(rw[1]));
				std::string strUrl_past_participle = "https://www.merriam-webster.com/thesaurus/" + strWord;
				
				/*
					get past participle
				*/
				getPastParticiple(strUrl_past_participle, strWord);
				sys_j.sys_timedelay(td);
		}
	}
	writeLog("Done looking for words past tenses...");
}
void NewDic_look_for_past_participle_of_word(){
	SysLogLib sys_j;
	Jsonlib jsonl_j;
	std::vector<std::vector<std::string>> dbresult;
	size_t td = 5000;
	writeLog("Start looking for words...");
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
	db.connect();
	db.executeQuery("select * from english_voc order by id asc",dbresult);
	db.disconnect();
	if(!dbresult.empty()){
		for(const auto& rw : dbresult){
			/*
				rw[0]=id,
				rw[1]=word,
				rw[2]=word_type,
				rw[3]=meaning_en,
				rw[4]=meaning_zh
				
				check the word's past participle and past perfect participle
			*/
				std::string strWord = std::string(jsonl_j.trim(rw[1]));
				std::string strUrl_past_participle = "https://www.merriam-webster.com/thesaurus/" + strWord;
				
				/*
					get past participle
				*/
				getPastParticiple(strUrl_past_participle, strWord);
				sys_j.sys_timedelay(td);
		}
	}
	writeLog("Done looking for words past tenses...");
}
size_t get_word_type(const std::string& inputStr, const std::unordered_map<std::string,std::string>& wordtypes, CallbackFunction fncallback){
    /*
        if a word was successfully crawled, then return 1
    */
    size_t word_was_found = 0;
    WebSpiderLib weblib_j;
    
    /*
        move the cursor to the first place
    */
    // Read the HTML file
    //std::ifstream file("example.html");
    //std::string htmlContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::string htmlContent = weblib_j.GetURLContent("https://www.merriam-webster.com/dictionary/" + inputStr);
    std::string word_type;
    std::string meaning_en;
    std::string meaning_zh;
    std::vector<std::string> pass_value_to_callback(4);
    if(htmlContent.length() > 0){
        word_was_found = 1;
    }else{
        return 0;
    }
	//find all english meaning span
	//"<div class=\"def ddef_d db\">(\\w+)</div>"
	//"<div class=\"def ddef_d db\">(.*?)</div>"
	//R"(<div[^>]*>([^<]*)</div>)"
	//std::vector<std::string> english_meanings =  findAllWordsBehindSpans(htmlContent,R"(<div[^>]*>([^<]*)</div>)");
	std::vector<std::string> english_meanings =  weblib_j.findAllWordsBehindSpans(htmlContent,"<div class=\"def ddef_d db\">(.*?)</div>");
	std::string english_meanings_db;
	for(std::string& em : english_meanings){
		//em = findWordBehindSpan(em,"<div class=\"def ddef_d db\">(.*?)</div>");
		//std::cout << "findWordBehindSpan: " << em << std::endl;
		em = weblib_j.removeHtmlTags(em);
        english_meanings_db += em + "\n";
	}
	meaning_en = english_meanings_db;//save the english translation into a string
	std::string word_front = "<div class=\"tc-bb tb lpb-25 break-cj\" lang=\"zh-Hans\">";
	std::string word_after = "</div>";
	size_t pos_front = htmlContent.find(word_front);
	if (pos_front != std::string::npos) {
		size_t pos_after = htmlContent.find(word_after, pos_front);
		if (pos_after != std::string::npos) {
			std::string divContent = htmlContent.substr(pos_front + word_front.length(), pos_after - pos_front - word_front.length());
			std::regex tagRegex("<[^>]+>");
			std::string Chinese_result = std::regex_replace(divContent, tagRegex, "");
			Chinese_result = std::regex_replace(Chinese_result, std::regex("&hellip;"), "");
			meaning_zh = Chinese_result;
		}
	}
    // Find all word_type elements
    std::vector<std::string> spans = weblib_j.findAllSpans(htmlContent, "<span class=\"pos dpos\" title=\"[^\"]+\">(\\w+)</span>");
    // Print the found word_type elements
    for (const std::string& span : spans) {
        word_type = weblib_j.findWordBehindSpan(span,"<span class=\"pos dpos\" title=\"[^\"]+\">(\\w+)</span>");
        std::cout << "The word behind the span: " << word_type << std::endl;
        //save everything into database
        pass_value_to_callback.resize(4);  // Resize the vector to have a size of 4
        pass_value_to_callback[0] = inputStr;
        pass_value_to_callback[1] = word_type;
        pass_value_to_callback[2] = weblib_j.str_trim(meaning_en);
        pass_value_to_callback[3] = weblib_j.str_trim(meaning_zh);
        //return word_type;
        fncallback(pass_value_to_callback, wordtypes);
        std::this_thread::sleep_for(std::chrono::seconds(1));//time dealy
        return 1;
    }
    return 1;
}
/*
    Callback function implementation
    result[0]: word
    result[1]: word type
    result[2]: english meaning
    result[3]: simplified chinese meaning
*/
void callback_Function(const std::vector<std::string>& result, const std::unordered_map<std::string,std::string>& word_types) {
    //sys_log("Callback function called with result: " + std::to_string(result));
	if(!result.empty()){
        writeLog("Callback function called with result: ----------------------");
        writeLog("word: " + result[0]);
        writeLog("word type: " + result[1]);
        writeLog("English: " + result[2]);
        writeLog("Chinese: " + result[3]);
        writeLog("-------------------------------------------------------------");
        std::string word_type_abv;
        if(word_types.count(result[1]) > 0){
        	word_type_abv = word_types.at(result[1]);
        }
        else{
        	writeLog("Word type: " + result[1] + " did not find in type_table db." );
        	word_type_abv = "NF";
        }
        /*
            open database
        */
        SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
        db.connect();
        std::vector<std::vector<std::string>> dbresult;
        db.executeQuery("insert into english_voc(word,word_type,meaning_en,meaning_zh)values('" + result[0] + "','" + word_type_abv + "','" + result[2] + "','" + result[3]  + "')", dbresult);
        db.disconnect();
    }
}

/*
    return the size of the html page
*/
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output) {
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}
/*
    start looking for word, word type, english meaning, simplified chinese meaning
*/
size_t start_looking_for_word(const std::string& inputWord, const int& currentWordCout, const std::unordered_map<std::string,std::string>& wordtypes){
    CurrentDateTime current_date_time;
    current_date_time = getCurrentDateTime();
    std::cout << "word counting: " << std::to_string(currentWordCout) << " current time: " <<  current_date_time.current_date + " " + current_date_time.current_time << std::endl;
    writeLog("Start checking-> " + inputWord);
    size_t word_found = 0;
    word_found = get_word_type(inputWord, wordtypes, callback_Function);//callback_Function
    return word_found;
}
size_t if_already_checked(const std::vector<std::string>& words_checked, const std::string& word_to_find){
	auto it = std::find(words_checked.begin(), words_checked.end(), word_to_find);
    if (it != words_checked.end()) {
       return 1;
    }
    return 0;
}
void expend_english_voc(){
	/*
		open the english txt folder
	*/
	nemslib nems_j;
	Jsonlib json_j;
	std::vector<std::string> words_checked;
	/*
		ini word type list from db
	*/
	std::unordered_map<std::string,std::string> iniWordType = get_word_type_abv();
	/*
		read e-books from harddisk
	*/
	std::string folderPath = "/Volumes/WorkDisk/MacBk/pytest/NLP_test/corpus/english_ebooks"; // Open the english book folder path
    for (const auto& entry : std::filesystem::directory_iterator(folderPath)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                    std::cout << line << std::endl; // Process the content as needed
                    /*
                    	Remove the punctuation in the line
                    */
                    line = std::string(json_j.trim(line));
                    line = nems_j.removeEnglishPunctuation_training(line);
                    json_j.toLower(line);
                    std::cout << line << std::endl;
                    //insert here
                    /*
                    	tokenize the line
                    */
                    std::vector<std::string> words_to_check = nems_j.tokenize_en(line);
                    size_t i =0;
                    for(const std::string& wtc : words_to_check){
                    	/*
                    		check if it's already exists in db
                    	*/
                    	if (if_already_checked(words_checked, wtc) == 0){
                    		SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
							db.connect();
							std::vector<std::vector<std::string>> dbresult;
							db.executeQuery("select * from english_voc where word='" + wtc + "'",dbresult);
							//db.executeQuery("insert into english_voc(word,word_type,meaning_en,meaning_zh)values('" + result[0] + "','" + result[1] + "','" + result[2] + "','" + result[3]  + "')", dbresult);
							size_t word_found = 0;
							if(dbresult.empty()){
								i++;
								word_found = start_looking_for_word(wtc, i, iniWordType);
								std::this_thread::sleep_for(std::chrono::seconds(7));//time dealy
							}
							words_checked.push_back(wtc);
							db.disconnect();
						}
					}
                    //insert here
                    
                }
                file.close();
            } else {
                std::cerr << "Failed to open file: " << entry.path() << std::endl;
            }
        }
    }

}
int main() {
	//look_for_past_participle_of_word();
	//expend_english_voc();
	NewDic_look_for_past_participle_of_word();
    return 0;
}