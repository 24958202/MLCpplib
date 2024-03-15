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
    g++ /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/webcrawler_past_participle.cpp -o /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/webcrawler_past_participle -I/usr/local/include/eigen3/ -I/usr/local/boost/include/ -I/opt/homebrew/Cellar/icu4c/73.2/include/ -L/opt/homebrew/Cellar/icu4c/73.2/lib -licuuc -licudata /usr/local/boost/lib/libboost_system.a /usr/local/boost/lib/libboost_filesystem.a /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/lib/nemslib.a -I/Users/dengfengji/miniconda3/pkgs/sqlite-3.41.2-h80987f9_0 -lsqlite3 -I/Volumes/WorkDisk/cppLibs/curl/include/ -lcurl -I/opt/homebrew/Cellar/gumbo-parser/0.10.1/include /opt/homebrew/Cellar/gumbo-parser/0.10.1/lib/libgumbo.a -std=c++20
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
#include <cctype>
#include <algorithm>
#include <thread>
#include <chrono>
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
std::string getPastParticiple(const std::string& strurl){
	WebSpiderLib wSpider_j;
	std::string htmlContent = wSpider_j.GetURLContent(strurl);
	std::string queryResult = wSpider_j.findWordBehindSpan(htmlContent,"<span class=\"vg-ins\">(\\w+)</span>");	//(\\w+)(.*?)
	std::cout << queryResult << std::endl;
	//size_t pos_front = queryResult.find(strWord);
	//if(pos_front != std::string::npos){
	std::vector<std::string> get_final_result = wSpider_j.findAllSpans(queryResult, "<span class=\"if\">(\\w+)</span>");
	for(const auto& get_f : get_final_result){
		std::cout << get_f << std::endl;
	}
	//return queryResult;
	//std::vector<std::string> removeHtmlTags_findStr(const std::string&);
	return "";
}
void look_for_past_participle_of_word(){
	SysLogLib sys_j;
	Jsonlib jsonl_j;
	std::vector<std::vector<std::string>> dbresult;
	size_t td = 5000;
	writeLog("Start looking for words past tenses...");
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
				std::string strUrl_past_participle = "https://www.merriam-webster.com/dictionary/" + strWord;
				
				/*
					get past participle
				*/
				std::string get_result_past_participle = getPastParticiple(strUrl_past_participle);
				sys_j.sys_timedelay(td);
				std::cout << get_result_past_participle << std::endl;

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
    std::string htmlContent = weblib_j.GetURLContent("https://dictionary.cambridge.org/dictionary/english/" + inputStr);
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
        std::vector<std::vector<std::string>> dbDelete;
        db.executeQuery("delete from english_voc where word='" + result[0] + "'", dbDelete);
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
size_t start_looking_for_word(const std::string& inputWord, const std::unordered_map<std::string,std::string>& wordtypes){
    CurrentDateTime current_date_time;
    current_date_time = getCurrentDateTime();
    std::cout << " current time: " <<  current_date_time.current_date + " " + current_date_time.current_time << std::endl;
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
/*
    add on Nov 8, 2023
*/
void getNonAntPastParticiple(std::string& strurl, std::string strWord){
	std::vector<std::vector<std::string>> dbresult;
	std::vector<std::vector<std::string>> dbDelete;
	WebSpiderLib wSpider_j;
	Jsonlib jsl_j;
	jsl_j.toLower(strWord);
	std::string strWord_lower = strWord;
	std::cout << "String input: --->" << strWord_lower << std::endl;
	std::string htmlContent = wSpider_j.GetURLContent(strurl);
    /*
        time_delay 
    */
    std::this_thread::sleep_for(std::chrono::milliseconds(300));
	// Find the position of the content attribute
    size_t contentPos = htmlContent.find("<meta name=\"description\" content=\"");
    if(contentPos != std::string::npos) {
        // Find the end of the content attribute
        size_t endQuotePos = htmlContent.find("\"", contentPos + 36); // 9 is the length of "content=\""
        if(endQuotePos != std::string::npos) {
            // Extract the content
            std::string content = htmlContent.substr(contentPos + 36, endQuotePos - contentPos - 36);
            std::string strUPP = strWord;
            std::transform(strUPP.begin(), strUPP.end(), strUPP.begin(),
                    [](unsigned char c) { return std::toupper(c); });
            std::vector<std::string > non_ant = jsl_j.splitString_bystring(content,"Antonyms of "+ strWord +":");
            std::string strNon;
            std::string strAnt;
            if(!non_ant.empty()){
                std::vector<std::string> strNons = jsl_j.splitString(non_ant[0],':');
                strNon = strNons[1];
                strNon = std::string(jsl_j.trim(strNon));
                //strNon = strNon.substr(0,strNon.size()-1);
                strAnt = std::string(jsl_j.trim(non_ant[1]));
            }
            /*
                has to remove "; Antonyms of WA"
            */
            std::string substr = "; Antonyms of " + strUPP;
            std::string result;
            size_t pos = strNon.find(substr);
            if (pos != std::string::npos) {
                result = strNon.substr(0, pos);
                std::cout << "string to replace:>>>>>>>>>>>>" << result << std::endl;
            } else {
                std::cout << "Sub string not found in: " << strNon << std::endl;
            }
            std::cout << "Successfully get: " << strWord_lower << "-> "<< result << "| Ant: " << strAnt << std::endl;
            /*
                save to database
            */
			SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
			db.connect();
			db.executeQuery("delete from raw_stories where story_txt='" + strWord_lower + "'", dbDelete);
			db.executeQuery("insert into raw_stories(story_txt,story_topics,Resv)values('" + strWord_lower + "','" + result + "','" + strAnt + "')", dbresult);
			db.disconnect();
			writeLog("Successfully saved: " + result + " | " + strAnt +  "into database!");
        }
    }
}
/*
	get already check 
*/
std::vector<std::string> get_english_voc_already_checked_in_db(){
	Jsonlib json_j;
	std::vector<std::vector<std::string>> dbresult;
	std::vector<std::string> words;
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
	db.connect();
	db.executeQuery("select word from english_voc order by id asc",dbresult);
	db.disconnect();
	if(!dbresult.empty()){
		for(auto& rw : dbresult){
			std::string strword = std::string(json_j.trim(rw[0]));
			words.push_back(strword);
		}
	}
	return words;
}
std::vector<std::string> get_non_ant_already_checked_in_db(){
	Jsonlib json_j;
	std::vector<std::vector<std::string>> dbresult;
	std::vector<std::string> words;
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
	db.connect();
	db.executeQuery("select story_txt from raw_stories order by id asc",dbresult);
	db.disconnect();
	if(!dbresult.empty()){
		for(auto& rw : dbresult){
			std::string strword = std::string(json_j.trim(rw[0]));
			words.push_back(strword);
		}
	}
	return words;
}
bool isNumeric(const std::string& str) {
    for (char c : str) {
        if (!std::isdigit(c)) {
            return false;
        }
    }
    return true;
}
void expend_english_voc(){
	/*
		open the english txt folder
	*/
	nemslib nems_j;
	Jsonlib json_j;
	std::vector<std::string> words_checked;
	std::vector<std::string> words_story_checked;
	/*
		ini word type list from db
	*/
	words_checked = get_english_voc_already_checked_in_db();
	words_story_checked = get_non_ant_already_checked_in_db();
	
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
                    std::vector<std::string> str_tokenized = nems_j.tokenize_en(line);
                    std::string strcheck;
                    if(!str_tokenized.empty()){
                        for(const std::string& st : str_tokenized){
                        	strcheck = st;
                        	if(nems_j.isNonAlphabetic(strcheck) || isNumeric(strcheck)){
                            	continue;
                            }
                        	json_j.trim(strcheck);
            				strcheck = nems_j.removeEnglishPunctuation_training(strcheck);
                            json_j.toLower(strcheck);
                            writeLog("The word we are about to look up: ->");
                            /*
                                Find the word's Non & Ant first
                            */
                            /*
                                check if the word has already look up
                            */
                            if(!words_story_checked.empty()){
                                auto it = std::find(words_story_checked.begin(), words_story_checked.end(), strcheck);
                                if (it == words_story_checked.end()) {
                                    std::string strUrl = "https://www.merriam-webster.com/thesaurus/" + strcheck;
                                    getNonAntPastParticiple(strUrl, strcheck);
                                    words_story_checked.push_back(strcheck);
                                }
                            }
                            else{
                                std::string strUrl = "https://www.merriam-webster.com/thesaurus/" + strcheck;
                                getNonAntPastParticiple(strUrl, strcheck);
                                words_story_checked.push_back(strcheck);
                            }                         
                            if (if_already_checked(words_checked, strcheck) == 0){
								SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
								db.connect();
								std::vector<std::vector<std::string>> dbresult;
								db.executeQuery("select * from english_voc where word='" + strcheck + "'",dbresult);
								//db.executeQuery("insert into english_voc(word,word_type,meaning_en,meaning_zh)values('" + result[0] + "','" + result[1] + "','" + result[2] + "','" + result[3]  + "')", dbresult);
								size_t word_found = 0;
								if(dbresult.empty()){
									word_found = start_looking_for_word(strcheck, iniWordType);
									std::this_thread::sleep_for(std::chrono::seconds(7));//time dealy
								}
								words_checked.push_back(strcheck);
								db.disconnect();
                            }
                        }
                        
                    }
                    else{
                        continue;
                    }
                    //insert here
                    /*
                        tokenize the line
                    */
                   
                    //insert here
                    
                }
                file.close();
            } else {
                std::cerr << "Failed to open file: " << entry.path() << std::endl;
            }
        }
    }

}
void check_pass_for_the_word(const std::string& s_word, const std::string& meaning_e, const std::string& meaning_z){
	writeLog("The word we are about to look up:-> " + s_word);
	WebSpiderLib weblib_j;
	Jsonlib json_j;
	std::vector<std::vector<std::string>> delresult;
	std::vector<std::vector<std::string>> updresult;
	/*
		move the cursor to the first place
	*/
	// Read the HTML file
	//std::ifstream file("example.html");
	//std::string htmlContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
	std::string pass1;
	std::string htmlContent;
	std::vector<std::string> html_str;
	try{
		htmlContent = weblib_j.GetURLContent("https://www.merriam-webster.com/dictionary/" + s_word);
		std::this_thread::sleep_for(std::chrono::seconds(3));//seconds
		html_str =  weblib_j.findAllWordsBehindSpans(htmlContent,"<span class=\"if\">(.*?)</span>");
	}
	catch(const std::exception& e){
		std::cerr << "Exception caught: " << e.what() << std::endl;
	}
	if(!html_str.empty()){
		SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
		db.connect();
		for(const auto& hs : html_str){
			if(!hs.empty()){
				pass1 = std::string(json_j.trim(hs));
				db.executeQuery("delete from english_voc where word_type='VB' and word='" + pass1 + "'", delresult);
				std::this_thread::sleep_for(std::chrono::milliseconds(500));
				db.executeQuery("insert into english_voc(word,word_type,meaning_en,meaning_zh)values('" + pass1 + "','VB','" + meaning_e + "','" + meaning_z + "')", updresult);
				writeLog("Successfully write :" + pass1 + " -> " + meaning_e + " -> " + meaning_z + " to the db!");
			}
		}	
		db.disconnect();
	}
	else{
		/*
			saved the word into /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/res/Rules/missed_words.txt
			start from index of 60 was added by the program
		*/
		std::ofstream file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/res/Rules/missed_words.txt", std::ios::app);
		if (file.is_open()) {
			file << s_word << std::endl;
		} else {
			std::cout << "Unable to open the missed_words file." << std::endl;
		}
		file.close();
	}
}
void add_pass_terms_to_words_db(){
	Jsonlib json_j;
	std::vector<std::vector<std::string>> dbresult;
	std::vector<std::string> words;
	std::vector<std::string> meaning_en;
	std::vector<std::string> meaning_zh;
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
	db.connect();
	db.executeQuery("select word,meaning_en,meaning_zh from english_voc where word_type='VB'",dbresult);
	db.disconnect();
	if(!dbresult.empty()){
		for(auto& rw : dbresult){
			std::string strword = std::string(json_j.trim(rw[0]));
			words.push_back(strword);
			meaning_en.push_back(rw[1]+" ");
			meaning_zh.push_back(rw[2]+" ");
		}
	}
	if(words.size()!=0){
		for(int i = 0; i < words.size(); i++){
			check_pass_for_the_word(words[i],meaning_en[i],meaning_zh[i]);
			std::this_thread::sleep_for(std::chrono::seconds(7));//seconds
		}
	}
}
void check_missed_words_pass(){
	Jsonlib json_j;
	std::vector<std::string> wordList;
    std::ifstream file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/res/Rules/missed_words.txt");
    std::string line;
    std::vector<std::string> words;
    std::vector<std::string> meaning_ens;
    std::vector<std::string> meaning_zhs;
    while (getline(file, line)) {
        wordList.push_back(line);
    }
    /*
		check for the meaning_en, meaning_zh of the word
	*/
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db/catalogdb.db");
	db.connect();
	std::vector<std::vector<std::string>> dbresult;
    for(auto& wl : wordList){
    	std::string str_word = std::string(json_j.trim(wl));
    	std::string str_meaning_en;
    	std::string str_meaning_zh;
		db.executeQuery("select meaning_en,meaning_zh from english_voc where word='" + str_word + "'",dbresult);
		if(!dbresult.empty()){
			for(const auto& rw : dbresult){
				str_meaning_en = rw[0]+" ";
				str_meaning_zh = rw[1]+" ";
			}
			words.push_back(str_word);
			meaning_ens.push_back(str_meaning_en);
			meaning_zhs.push_back(str_meaning_zh);
		}
    }
    db.disconnect();
    for(int i =0; i< words.size(); i++){
    	//std::cout << words[i] << " " << meaning_ens[i] << " " << meaning_zhs[i] << std::endl;		
    	check_pass_for_the_word(words[i],meaning_ens[i],meaning_zhs[i]);	
		std::this_thread::sleep_for(std::chrono::seconds(7));//seconds
    }
}
int main() {
	/*
		read words from english books and check online for the english meaning and chinese meaning.
	*/
	//expend_english_voc();//->add_pass_terms_to_words_db()->check_missed_words_pass()
	/*
		read the word from db and look for its pass terms, and insert pass terms with the same meaning.
	*/
	add_pass_terms_to_words_db();
	//writeLog("The jos is finished!");
	/*
		look for the missed words
	*/
	writeLog("Start looking for the missed words...");
	check_missed_words_pass();
	writeLog("All jobs are done!");
    return 0;
}