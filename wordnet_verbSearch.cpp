#include <iostream>
#include <vector>
#include <string>
#include <string>
#include <filesystem>
#include <fstream>
#include <set>
#include <ranges>
#include <thread>
#include <chrono>
#include <regex>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include "../lib/nemslib.h"
std::vector<std::string> findAllWordsBehindSpans(const std::string& input, const std::string& strreg){
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
size_t WriteCallback(void* contents, size_t size, size_t nmemb, std::string* output){
    size_t totalSize = size * nmemb;
    output->append((char*)contents, totalSize);
    return totalSize;
}
std::string GetURLContent(const std::string& url){
    curl_global_init(CURL_GLOBAL_DEFAULT);
    CURL* curl = curl_easy_init();
    std::string output;
    if (curl) {
        curl_easy_setopt(curl, CURLOPT_URL, url.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);
        /* For completeness */
        curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, "");
        curl_easy_setopt(curl, CURLOPT_TIMEOUT, 5L);
        curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);
        /* only allow redirects to HTTP and HTTPS URLs */
        //curl_easy_setopt(curl, CURLOPT_REDIR_PROTOCOLS, "http,https");
        curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);
        curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);
        /* each transfer needs to be done within 20 seconds! */
        curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 20000L);
        /* connect fast or fail */
        curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000L);
        /* skip files larger than a gigabyte */
        curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE,
                        (curl_off_t)1024*1024*1024);
        //curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "cookies.txt"); // Use a cookie file if needed
        curl_easy_setopt(curl, CURLOPT_COOKIEFILE, "");
        curl_easy_setopt(curl, CURLOPT_FILETIME, 1L);
        // Set the User-Agent header to simulate a browser
        curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3"); //"Mozilla/5.0 (Windows NT 10.0; Win64; x64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3");
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
std::vector<std::string> getPastParticiple(const std::string& strurl, const std::string& strWord){
    std::vector<std::string> dbresult;
	WebSpiderLib wSpider_j;
	Jsonlib jsl_j;
	std::string strWord_lower = strWord;
    boost::algorithm::to_lower(strWord_lower);
	std::cout << "String input: --->" << strWord_lower << std::endl;
	std::string htmlContent = GetURLContent(strurl);
    dbresult = findAllWordsBehindSpans(htmlContent,"<span class=\"if\">(.*?)</span>");
    for(auto& db : dbresult){
        boost::algorithm::trim(db);
        std::cout << strWord << " >>word found: >>>" << db << '\n';
    }
    return dbresult;
}
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
                boost::algorithm::replace_all(str_word, "_", " ");
				getPasAnt = getPastParticiple(str_url, str_word);
				if(!getPasAnt.empty()){
					for(const auto& gp : getPasAnt){
						if(no_repeated_verbs.find(gp) == no_repeated_verbs.end()){
							get_verbs.push_back(gp);
						}
					}
				}
			}
			else{//has more than one word
                boost::algorithm::replace_all(str_verb, "_", " ");
				boost::algorithm::split(phrases_to_check, str_verb, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
				if(!phrases_to_check.empty()){
					str_phrase = phrases_to_check[0];//check the first word
                    std::cout << str_verb << " | to check -> " << str_phrase << '\n';
					str_url = "https://www.merriam-webster.com/dictionary/" + str_phrase;
					str_word = str_phrase;
					getPasAnt = getPastParticiple(str_url, str_word);
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
            if(!get_verbs.empty()){
                std::ofstream oofile("/home/ronnieji/lib/EnglishWords/dict/output/total_verb_temp.txt", std::ios::app);
		        if(!oofile.is_open()){
			        oofile.open("/home/ronnieji/lib/EnglishWords/dict/output/total_verb_temp.txt", std::ios::app);
		        }
		        for(auto& gv : get_verbs){
                    boost::algorithm::trim(gv);
			        oofile << gv << '\n';
		        }
		        oofile.close();                            
            }
			no_repeated_verbs.clear();
			no_repeated_verbs.insert(get_verbs.begin(),get_verbs.end());
			std::this_thread::sleep_for(std::chrono::seconds(5));//seconds 
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
