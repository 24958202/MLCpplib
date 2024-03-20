
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

std::unordered_set<std::string> ProcessFileNames;
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
	if(folder_path.empty()){
		return;
	}
	/*
		ini 
		put the word checked in the checked list
	*/
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/webUrls/english_voc.bin")){
		words_checked = dic_j.get_english_voc_already_checked_in_db("/home/ronnieji/lib/db_tools/webUrls/english_voc.bin");
	}
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/webUrls/missed_words.bin")){
		words_missed = get_missed_words("/home/ronnieji/lib/db_tools/webUrls/missed_words.bin");
	}
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/webUrls/words_not_found.bin")){
		words_not_found = get_missed_words("/home/ronnieji/lib/db_tools/webUrls/words_not_found.bin");
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
    for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt" && ProcessFileNames.find(entry.path()) == ProcessFileNames.end()) {
            std::ifstream file(entry.path());
            if (file.is_open()) {
                std::string line;
                while (std::getline(file, line)) {
                	line = std::string(json_j.trim(line));
					json_j.toLower(line);
					syslog_j.writeLog("/home/ronnieji/lib/db_tools/log",line);
                    /*
                        Remove the punctuation in the line
                    */
                    //std::vector<std::string> str_tokenized = nems_j.tokenize_en(line);
                    std::vector<std::string> str_tokenized = nems_j.tokenize_en(line);
					//boost::algorithm::split(str_tokenized, line, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
                    std::string strcheck;
                    if(!str_tokenized.empty()){
                        for(const std::string& st : str_tokenized){
                        	strcheck = st;
                        	if(nems_j.isNonAlphabetic(strcheck) || nems_j.isNumeric(strcheck)){
                            	continue;
                            }
                        	//boost::algorithm::trim(strcheck);
                        	strcheck = std::string(json_j.trim(strcheck));
            				strcheck = nems_j.removeEnglishPunctuation_training(strcheck);
                            json_j.toLower(strcheck);
							std::string str_msg = "The word we are about to look up: -> ";
							str_msg.append(strcheck);
							syslog_j.writeLog("/home/ronnieji/lib/db_tools/log",str_msg);
                            /*
                                Find the word's Non & Ant first
                            */
                            /*
                                check if the word has already look up
                            */
                            if(!words_checked.empty()){
                                auto it = std::find(words_checked.begin(), words_checked.end(), strcheck);
                                if (it == words_checked.end()) {
									if(!words_not_found.empty()){
										auto word_not_found_it = std::find(words_not_found.begin(),words_not_found.end(),strcheck);
										if(word_not_found_it != words_not_found.end()){//if it's in words_not_found list, jump out
											continue;
										}
									}
                                    std::string strUrl = "https://dictionary.cambridge.org/dictionary/essential-american-english/";
									strUrl.append(strcheck);
									if(dic_j.check_word_onlineDictionary(
									strcheck,
									iniWordType,
									"/home/ronnieji/lib/db_tools/webUrls/english_voc.bin",
									"/home/ronnieji/lib/db_tools/log"
									)==1){
										words_checked.push_back(strcheck);
									}
									else{//try to look it up at the "https://www.merriam-webster.com/"
										if(dic_j.check_word_onMerriamWebsterDictionary(
											strcheck,
											iniWordType,
											"/home/ronnieji/lib/db_tools/webUrls/english_voc.bin",
											"/home/ronnieji/lib/db_tools/log"
										)==1){
											words_checked.push_back(strcheck);
										}
										else{//this word can not be found online
											add_missed_words("/home/ronnieji/lib/db_tools/webUrls/words_not_found.bin",strcheck);
										}
									}
                                }
                            }
							else{
								std::string strUrl = "https://dictionary.cambridge.org/dictionary/essential-american-english/";
								strUrl.append(strcheck);
								if(dic_j.check_word_onlineDictionary(
								strcheck,
								iniWordType,
								"/home/ronnieji/lib/db_tools/webUrls/english_voc.bin",
								"/home/ronnieji/lib/db_tools/log"
								)==1){
									words_checked.push_back(strcheck);
								}
								else{//try to look it up at the "https://www.merriam-webster.com/"
									if(dic_j.check_word_onMerriamWebsterDictionary(
										strcheck,
										iniWordType,
										"/home/ronnieji/lib/db_tools/webUrls/english_voc.bin",
										"/home/ronnieji/lib/db_tools/log"
									)==1){
										words_checked.push_back(strcheck);
									}
									else{//this word can not be found online
										add_missed_words("/home/ronnieji/lib/db_tools/webUrls/words_not_found.bin",strcheck);
									}
								}
							}
							//dic_j.look_for_past_participle_of_word("home/ronnieji/lib/db_tools/webUrls/english_voc.bin","/home/ronnieji/lib/db_tools/log");
							std::this_thread::sleep_for(std::chrono::seconds(3));//seconds 
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
                    ProcessFileNames.insert(entry.path());
                }
                file.close();
            } else {
                std::cerr << "Failed to open file: " << entry.path() << std::endl;
            }
        }
    }
	 /*
        Check new files, and process the new file
    */
    for (const auto& entry : std::filesystem::directory_iterator(folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt" && ProcessFileNames.find(entry.path()) == ProcessFileNames.end()){
            expend_english_voc(folder_path);
            return;
        }
    }
    syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","All jobs are done!");
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
	expend_english_voc(folder_name);//->add_pass_terms_to_words_db()->check_missed_words_pass()
	SysLogLib syslog_j;
	libdict dic_j;
	dic_j.look_for_past_participle_of_word("/home/ronnieji/lib/db_tools/webUrls/english_voc.bin","/home/ronnieji/lib/db_tools/log");
	syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","All jobs are done!");
    return 0;
}
