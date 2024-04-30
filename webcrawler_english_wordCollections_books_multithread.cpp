
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
#include "../lib/nemslib.h"
#include "../lib/libdict.h"

std::vector<std::string> all_voc;
std::mutex mtx;
std::condition_variable cv;

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
    std::lock_guard<std::mutex> lock(mtx);
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
									continue;
								}
								std::cout << "Checking the existance of : " << ls << '\n';
								if(!all_voc.empty()){
									auto it = std::find_if(all_voc.begin(),all_voc.end(),[&ls](const std::string& s){
										return s == ls;
									});
									if(it != all_voc.end()){
										syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","The word's already in the binary file,look for the next...");
										continue;
									}
								}
								all_voc.push_back(ls);
								nl_j.AppendBinaryOne(str_all_voc_path,ls);
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
     cv.notify_one();
}
void add_wordnet(){
	nlp_lib nem_j;
	if(std::filesystem::exists("/home/ronnieji/lib/db_tools/webUrls/core-wordnet.bin") && std::filesystem::exists("/home/ronnieji/lib/db_tools/webUrls/english_voc.bin")){
		std::vector<Mdatatype> eng_voc = nem_j.readBinaryFile("/home/ronnieji/lib/db_tools/webUrls/english_voc.bin");
		std::vector<Mdatatype> word_voc = nem_j.readBinaryFile("/home/ronnieji/lib/db_tools/webUrls/core-wordnet.bin");
		if(!eng_voc.empty()){
			if(!word_voc.empty()){
				eng_voc.insert(eng_voc.end(),word_voc.begin(),word_voc.end());
			}
		}
	}
	else{
		std::cerr << "Binary file core-wordnet.bin or english_voc.bin is missing" << '\n';
	}
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
    /*
        Use sched_setaffinity() to bind the process to specific CPU cores, ensuring that it's not competing with other processes for the same cores.
    */
    cpu_set_t cpuset;
    CPU_ZERO(&cpuset);
    CPU_SET(0, &cpuset);  // Bind to CPU 0
    sched_setaffinity(0, sizeof(cpu_set_t), &cpuset);
    /*
        get the nums of cpu
    */
    int num_threads = std::thread::hardware_concurrency();

    std::vector<std::thread> threads;
    for (int i = 0; i < num_threads; ++i) {
        threads.push_back(std::thread(get_all_voc, folder_name));
    }
    for (auto& thread : threads) {
        thread.join();
    }
	//get_all_voc(folder_name);
	expend_english_voc(folder_name);//->add_pass_terms_to_words_db()->check_missed_words_pass()
	/*
		add word net vocabulary into english_voc
	*/
	add_wordnet();
	SysLogLib syslog_j;
	libdict dic_j;
	dic_j.look_for_past_participle_of_word("/home/ronnieji/lib/db_tools/web/english_voc.bin","/home/ronnieji/lib/db_tools/log");
	syslog_j.writeLog("/home/ronnieji/lib/db_tools/log","All jobs are done!");
    return 0;
}
