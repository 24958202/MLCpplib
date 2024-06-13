#include <iostream>
#include <string>
#include <filesystem>
#include <fstream>
#include <set>
#include <ranges>
#include <thread>
#include <chrono>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "../lib/nemslib.h"
#include "../lib/libdict.h"

void write_to_file(const std::string& f_path, const std::pair<std::string,std::string>& f_content){
	nlp_lib nl_j;
	std::string str_bin_output = f_path + "_.bin";
	if(f_path.empty()){
		std::cout << "Please input the file path!" << std::endl;
		return;
	}
	std::ofstream ofile(f_path, std::ios::app);
	if(!ofile.is_open()){
		ofile.open(f_path);
	}
	if(!f_content.first.empty()){
		ofile << f_content.first << "\n";
	}
	ofile.close();
	std::string str_line = f_content.first + "^~&" + f_content.second;
	nl_j.AppendBinaryOne(str_bin_output,str_line);
}
bool containsOnlyOneWord(const std::string& str) {
    if(str.find(' ') != std::string::npos){
        return false;//has space,there are more than one words
	}
    else{
        return true;
	}
}
void get_index_sense(const std::string& strInput){
	if(strInput.empty()){
		return;
	}
	std::pair<std::string,std::string> final_res;
	Jsonlib jsl_j;
	std::vector<std::string> result;
	boost::split(result, strInput, boost::is_any_of("%"));
	if(!result.empty()){
		boost::algorithm::trim(result[0]);
		final_res = std::make_pair(result[0],"NF");
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_index_sense.txt", final_res);
		std::cout << "Adding the word: " << result[0] << " explain: " << '\n';
	}
}
void get_data_with_type(std::string& str_line, const std::string& f_name){
	if(str_line.empty()){
		std::cout << "str_line is empty!" << "\n";
		return;
	}
	nemslib nem_j;
	Jsonlib jsl_j;
	std::pair<std::string,std::string> word_plus_explaination;
	if(f_name.find("index") != std::string::npos){
		std::vector<std::string> res;
		boost::algorithm::split(res, str_line, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
		std::string str_i_adj = res[0];
		boost::algorithm::trim(str_i_adj);
		std::cout << str_i_adj << std::endl;
		if(str_i_adj.find('_') != std::string::npos){
			boost::algorithm::replace_all(str_i_adj, "_", " ");
		}
		word_plus_explaination = std::make_pair(str_i_adj,"NF");
	}
	else{
		std::vector<std::string> result; 
		boost::split(result, str_line, boost::is_any_of("|"));
		if(!result.empty()){
			std::string str_word;
			std::string str_explaination;
			std::string str_n = result[0];
			str_n = str_n.substr(17);
			std::vector<std::string> res;
			boost::algorithm::split(res, str_n, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
			if(!res.empty() && res.size()>=2){
				str_word = res[0];
				boost::algorithm::trim(str_word);
				str_explaination = res[1];
			}
			boost::algorithm::trim(result[1]);
			str_explaination.append(" ");
			str_explaination.append(result[1]);
			if(result.size()>2){
				str_explaination.append(" ");
				str_explaination.append(result[2]);
			}
			std::vector<std::string> tok_strexp;
			boost::algorithm::split(tok_strexp, str_explaination, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
			tok_strexp.erase(std::remove_if(tok_strexp.begin(), tok_strexp.end(),
                                [&nem_j](const auto& tk) { return nem_j.isNumeric(tk); }),tok_strexp.end());
			str_explaination="";
			for(const auto& ts : tok_strexp){
				str_explaination += ts + " ";
			}
			str_explaination.pop_back();
			if(str_word.find('_') != std::string::npos){
				boost::algorithm::replace_all(str_word, "_", " ");
			}
			word_plus_explaination = std::make_pair(str_word,str_explaination);
			std::cout << "Adding the word: " << str_word << " explain: " << str_explaination << '\n';
		}
 	}
    /*
    	write the collection into the related file.
    */
    if(f_name == "data_adj.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_data_adj.txt",word_plus_explaination);
	}
	else if(f_name == "data_adv.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_data_adv.txt",word_plus_explaination);
	}
	else if(f_name == "data_noun.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_data_noun.txt",word_plus_explaination);
	}
	else if(f_name == "data_verb.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_data_verb.txt",word_plus_explaination);
	}
	else if(f_name == "index_adj.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_index_adj.txt",word_plus_explaination);
	}
	else if(f_name == "index_adv.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_index_adv.txt",word_plus_explaination);
	}
	else if(f_name == "index_noun.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_index_noun.txt",word_plus_explaination);
	}
	else if(f_name == "index_verb.txt"){
		write_to_file("/home/ronnieji/lib/EnglishWords/dict/output/word_index_verb.txt",word_plus_explaination);
	}
}
void readFolder(const std::string& folder_name){
    /*
        read the existing word list
    */
    for(const auto& entry: std::filesystem::directory_iterator(folder_name)){
		if(entry.is_regular_file() && entry.path().extension() == ".txt"){
			std::ifstream file(entry.path());
			if(file.is_open()){
				std::string line;
                std::string f_name = entry.path().filename();
				while (std::getline(file,line)){
                    boost::algorithm::trim(line);
                    if(f_name != "index_sense.txt"){
						get_data_with_type(line, f_name);
					}
					else{
						get_index_sense(line);
					}
                }
            }
        }
    }
}
void merge_one(){
	nemslib nem_j;
    std::vector<std::string> voc_download = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/words/output.txt");//
	std::vector<std::string> word_data_adv = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_data_adv.txt");//
	std::vector<std::string> word_index_adv = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_index_adv.txt");
	std::vector<std::string> word_data_noun = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_data_noun.txt");//
	std::vector<std::string> word_index_noun = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_index_noun.txt");
	std::vector<std::string> word_data_verb = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_data_verb.txt");//
	std::vector<std::string> word_index_verb = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_index_verb.txt");
	std::vector<std::string> word_index_adj = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_index_adj.txt");//
	std::cout << "/* word_data_adv + word_index_adv  */" << '\n';
	std::set<std::string> existing_words(word_data_adv.begin(), word_data_adv.end());
	for (const auto& word : word_index_adv) {
		if (existing_words.find(word) == existing_words.end()) {
			word_data_adv.push_back(word);
		}
	}
	std::cout << "/* word_data_noun + word_index_noun */" << '\n';
	std::set<std::string> existing_words_noun(word_data_noun.begin(), word_data_noun.end());
	for (const auto& word : word_index_noun) {
		if (existing_words_noun.find(word) == existing_words_noun.end()) {
			word_data_noun.push_back(word);
		}
	}
	std::cout << "/* word_data_verb +  word_index_verb */ " << '\n';
	std::set<std::string> existing_words_verb(word_data_verb.begin(), word_data_verb.end());
	for (const auto& word : word_index_verb) {
		if (existing_words_verb.find(word) == existing_words_verb.end()) {
			word_data_verb.push_back(word);
		}
	}
	std::cout << "/* make one file */" << '\n';
	std::set<std::string> voc(voc_download.begin(),voc_download.end());
	for(const auto& wd : word_data_adv){
		if(voc.find(wd) == voc.end()){
			voc_download.push_back(wd);
		}
	}
	voc.clear();
	voc.insert(voc_download.begin(),voc_download.end());
	for(const auto& wd : word_data_noun){
		if(voc.find(wd) == voc.end()){
			voc_download.push_back(wd);
		}
	}
	voc.clear();
	voc.insert(voc_download.begin(),voc_download.end());
	for(const auto& wd : word_data_verb){
		if(voc.find(wd) == voc.end()){
			voc_download.push_back(wd);
		}
	}
	voc.clear();
	voc.insert(voc_download.begin(),voc_download.end());
	for(const auto& wd : word_index_adj){
		if(voc.find(wd) == voc.end()){
			voc_download.push_back(wd);
		}
	}
	if(!voc_download.empty()){
		std::sort(voc_download.begin(), voc_download.end());
		std::ofstream ofile("/home/ronnieji/lib/EnglishWords/dict/output/total_voc.txt", std::ios::out);
		if(!ofile.is_open()){
			ofile.open("/home/ronnieji/lib/EnglishWords/dict/output/total_voc.txt", std::ios::out);
		}
		for(const auto& item : voc_download){
			ofile << item << '\n';
		}
		ofile.close();
	}
	std::cout << "File was saved at /home/ronnieji/lib/EnglishWords/dict/output/total_voc.txt " << '\n';
}
void verb_search(){
	nemslib nem_j;
	libdict lib_j;
	std::vector<std::string> get_verbs = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_data_verb.txt");//
	std::vector<std::string> word_index_verb = nem_j.readTextFile("/home/ronnieji/lib/EnglishWords/dict/output/word_index_verb.txt");
	std::set<std::string> no_repeated_verbs(get_verbs.begin(),get_verbs.end());
	for (const auto& word : word_index_verb) {
		if (no_repeated_verbs.find(word) == no_repeated_verbs.end()) {
			get_verbs.push_back(word);
		}
	}
	if(!get_verbs.empty()){
		for(const auto& gv : get_verbs){
			std::string str_verb = gv;
			std::string str_url;
			std::vector<std::string> phrases_to_check;//more than one word
			std::string str_phrase;
			std::string str_word;
			std::vector<std::string> getPasAnt;
			if(containsOnlyOneWord(str_verb)){
				str_url = "https://www.merriam-webster.com/dictionary/" + str_verb;
				str_word = str_verb;
				getPasAnt = lib_j.getPastParticiple(str_url, str_word,"/home/ronnieji/lib/db_tools/log");
				if(!getPasAnt.empty()){
					for(const auto& gp : getPasAnt){
						if(no_repeated_verbs.find(gp) == no_repeated_verbs.end()){
							get_verbs.push_back(gp);
						}
					}
				}
			}
			else{//has more than one word
				boost::algorithm::split(phrases_to_check, str_verb, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
				if(!phrases_to_check.empty()){
					str_phrase = phrases_to_check[0];//check the first word
					str_url = "https://www.merriam-webster.com/dictionary/" + str_phrase;
					str_word = str_phrase;
					getPasAnt = lib_j.getPastParticiple(str_url, str_word,"/home/ronnieji/lib/db_tools/log");
					if(!getPasAnt.empty()){
						for(const auto& gp : getPasAnt){
							std::string word_to_lookup;
							word_to_lookup.append(gp);
							word_to_lookup.append(" ");
							for(unsigned int i = 0; i < phrases_to_check.size(); i++){
								if(i !=0){
									word_to_lookup += phrases_to_check[i] + " ";
								}
							}
							word_to_lookup.pop_back();
							if(no_repeated_verbs.find(word_to_lookup) == no_repeated_verbs.end()){
								get_verbs.push_back(word_to_lookup);
							}
						}
					}
				}
			}
			no_repeated_verbs.clear();
			no_repeated_verbs.insert(get_verbs.begin(),get_verbs.end());
			std::this_thread::sleep_for(std::chrono::seconds(1));//seconds 
		}
		std::sort(get_verbs.begin(),get_verbs.end());
		std::ofstream ofile("/home/ronnieji/lib/EnglishWords/dict/output/total_verb.txt", std::ios::out);
		if(!ofile.is_open()){
			ofile.open("/home/ronnieji/lib/EnglishWords/dict/output/total_verb.txt", std::ios::out);
		}
		for(const auto& item : get_verbs){
			ofile << item << '\n';
		}
		ofile.close();
	}
	std::cout << "https://www.merriam-webster.com/dictionary/ word checking is finished." << '\n'; 
}
int main(){
    //readFolder("/home/ronnieji/lib/EnglishWords/dict");
	//merge_one();
	verb_search();
	std::cout << "All jobs are done!" << '\n';
    return 0;
}