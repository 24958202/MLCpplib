
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
#include "../lib/nemslib.h"
#include "../lib/libdict.h"

std::vector<std::string> all_voc;
std::unordered_set<std::string> ProcessFileNames;
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
				add_missed_words("/home/ronnieji/lib/db_tools/web/words_not_found.bin",av);
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
	all_voc = nl_j.ReadBinaryOne("/home/ronnieji/lib/db_tools/web/all_voc.bin");
	for (const auto& entry : std::filesystem::directory_iterator(str_folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
					if(!line.empty()){
						line = jsl_j.trim(line);
						line = nem_j.removeEnglishPunctuation_training(line);
						jsl_j.toLower(line);
						std::vector<std::string> line_split = nem_j.tokenize_en(line);
						if(!line_split.empty()){
							for(auto& ls : line_split){
								ls = jsl_j.trim(ls);
								if(!all_voc.empty()){
									auto it = std::find_if(all_voc.begin(),all_voc.end(),[&ls](const std::string& s){
										return s == ls;
									});
									if(it != all_voc.end()){
										continue;
									}
								}
								all_voc.push_back(ls);
								nl_j.AppendBinaryOne("/home/ronnieji/lib/db_tools/web/all_voc.bin",ls);
								strMsg = "Adding the word: ";
								strMsg.append(ls);
								syslog_j.writeLog("/home/ronnieji/lib/db_tools/log",strMsg);
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
int main(int argc, char* argv[]) {
	/*
		read words from english books and check online for the english meaning and chinese meaning.
	*/
	if(argc != 2){
		std::cerr << "You should input book's folder path as a parameter!" << '\n';
        return 1;
    }
    std::string folder_name = argv[1];
    //std::string folderPath = "/Volumes/WorkDisk/MacBk/pytest/NLP_test/corpus/english_ebooks"; // Open the english book folder path
	get_all_voc(folder_name);
	expend_english_voc(folder_name);//->add_pass_terms_to_words_db()->check_missed_words_pass()
	SysLogLib syslog_j;
	libdict dic_j;
	dic_j.look_for_past_participle_of_word("/home/ronnieji/lib/db_tools/web/english_voc.bin","/home/ronnieji/lib/db_tools/log");
	syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","All jobs are done!");
    return 0;
}
