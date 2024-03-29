
#include <iostream>
#include <string>
#include <fstream>
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
#include <filesystem>
//#include <boost/algorithm/string.hpp>
#include "../lib/nemslib.h"

std::vector<std::string> str_broken_urls;
std::vector<std::string> str_urls;
std::vector<std::string> str_crawelled_urls;
std::vector<std::string> str_stored_urls;
std::vector<std::string> str_stored_urls_get_urls;

std::vector<std::string> get_url_list(const std::string& file_path){
	std::vector<std::string> url_list;
	if(file_path.empty()){
		std::cerr << "url list is empty!" << '\n';
		return url_list;
	}
	std::ifstream stopwordFile(file_path);
    if (!stopwordFile.is_open()) {
        std::cerr << "read_stopword_list::Error opening stopword file-nemslib" << '\n';
        return url_list;
    }
    std::string line;
    WebSpiderLib jl_j;
    while(std::getline(stopwordFile, line)){
        line = std::string(jl_j.str_trim(line));
        url_list.push_back(line);
    }
    stopwordFile.close();
    return url_list;
}
bool isDomainExtension(const std::string& word){
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
std::vector<std::string> split_sentences(const std::string& text){
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
            if (isDomainExtension(token)) {
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
std::string getRawHtml(const std::string& strurl){
	WebSpiderLib wSpider_j;
	std::string str_content;
	str_content = wSpider_j.GetURLContent(strurl);
	return str_content;
}
std::string get_title_content(std::string& raw_str){
	if(raw_str.empty()){
		std::cout << "get_title_content: input empty!" << '\n';
		return "";
	}
	WebSpiderLib wSpider_j;
	return wSpider_j.findWordBehindSpan(raw_str,"<h1 class=\"content-title mb-5\">(.*?)</h1>");
}
std::string get_topic_content(std::string& raw_str){
	size_t pos_start = 0;
    size_t pos_end = 0;
    std::string str_phrased_content;
    std::string str_start = "<!--[BP1]--><p class=\"topic-paragraph\">"; //"<!--[BEFORE-ARTICLE]--><span class=\"marker before-article\"></span><section id=\"ref1\" data-level=\"1\"><p class=\"topic-paragraph\"><strong><span id=\"ref883486\"></span>";
    std::string str_end = "</p><!--[P9]-->"; //"</p><!--[P10]--><span class=\"marker p10\"></span><!--[AM10]--><span class=\"marker AM10 am-inline\"></span>";
    pos_start = raw_str.find(str_start);
    if (pos_start != std::string::npos) {
        pos_end = raw_str.find(str_end, pos_start);
        if (pos_end != std::string::npos) {
            size_t content_length = pos_end - pos_start + str_end.length(); // Calculate the length of the content
            str_phrased_content = raw_str.substr(pos_start, content_length);
        } else {
            std::cout << "End marker not found!" << std::endl;
        }
    } else {
        std::cout << "Start marker not found!" << std::endl;
		return "";
    }
	std::this_thread::sleep_for(std::chrono::milliseconds(3));
	WebSpiderLib wSpider_j;
	str_phrased_content = wSpider_j.removeHtmlTags(str_phrased_content);
	return str_phrased_content;
}
std::string get_content_updated(std::string& raw_str){
	//<p class="topic-paragraph"></p>
	if(raw_str.empty()){
		std::cout << "get_content_updated: input empty!" << '\n';
		return "";
	}
	Jsonlib jsl_j;
	WebSpiderLib weblib_j;
	std::vector<std::string> main_content;
	main_content = weblib_j.findAllWordsBehindSpans(raw_str,"<p class=\"topic-paragraph\">(.*?)</p>");
	std::string str_content;
	if(!main_content.empty()){
		for(auto& mc : main_content){
			mc = std::string(weblib_j.str_trim(mc));
			mc = weblib_j.removeHtmlTags(mc);
			str_content += mc + '\n';
		}
	}
	return str_content;
}
void write_url_to_file(const std::string& file_path, const std::string& url){
	try{
		std::ofstream file(file_path,std::ios::app);
		if(!file.is_open()){
			file.open(file_path,std::ios::app);
		}
		file << url << '\n';
		file.close();
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
	}
}
void save_main_url_list(const std::string&file_path,const std::vector<std::string> urls){
	if(urls.size()==0){
		std::cerr << "save_main_url_list: input empty!" << '\n';
		return;
	}
	std::ofstream file(file_path,std::ios::out);
	if(!file.is_open()){
		file.open(file_path,std::ios::out);
	}
	for(const auto& ul : urls){
			file << ul << '\n';
	}
	file.close();
}
void get_one_page_urls(const std::string& url){
	//get all urls of the site
	Jsonlib jsl_j;
	WebSpiderLib wSpider_j;
	std::vector<std::string> urls;
	std::string htmlContent = getRawHtml(url);
	std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
	try{
		/*
			check if the url already crawlled
		*/
		auto it = std::find_if(str_crawelled_urls.begin(), str_crawelled_urls.end(), [url](const std::string& s) {
            return s == url;
		  });
		  if(it != str_crawelled_urls.end()){
			//found
			for(const auto& str_stored : str_stored_urls){
				auto it = std::find_if(str_stored_urls_get_urls.begin(), str_stored_urls_get_urls.end(), [str_stored](const std::string& s) {
					return s == str_stored;
				  });
				  if(it != str_stored_urls_get_urls.end()){
					//found
					continue;
				}
				str_stored_urls_get_urls.push_back(str_stored);
				return;
			}
		}
		/*
			check if the link is the broken link
		*/
		auto it_b = std::find_if(str_broken_urls.begin(), str_broken_urls.end(), [url](const std::string& s) {
            return s == url;
		});
		if(it_b != str_broken_urls.end()){
			for(const auto& str_stored : str_stored_urls){
				auto it = std::find_if(str_stored_urls_get_urls.begin(), str_stored_urls_get_urls.end(), [str_stored](const std::string& s) {
					return s == str_stored;
				  });
				  if(it != str_stored_urls_get_urls.end()){
					//found
					continue;
				}
				str_stored_urls_get_urls.push_back(str_stored);
				return;
			}
		}
		if(htmlContent.empty()){
			write_url_to_file("/home/working/lib/db_tools/webUrls/broken_urls.txt",url);
			str_broken_urls.push_back(url);
		}
		std::string url_rule = "(https?:\\/\\/)?(www\\.)?([a-zA-Z0-9-]+\\.){1,}[a-zA-Z]{2,}(\\/[a-zA-Z0-9\\/\\.-]*)?";
		std::vector<std::pair<size_t, std::string>> indexedNames;
		std::regex pattern = std::regex(url_rule);
        std::smatch match;
        std::string::const_iterator searchStart(htmlContent.cbegin());
        std::string strReturn;
        while (std::regex_search(searchStart, htmlContent.cend(), match, pattern)) {
            strReturn = match[0].str();
            strReturn = std::string(wSpider_j.str_trim(strReturn));
            if(!strReturn.empty()){
                indexedNames.push_back({match.position(0), strReturn});
                searchStart = match.suffix().first;
            }
            else{
            	std::cout << "strReturn is empty!" << std::endl;
            }
        }
        if(!indexedNames.empty()){
        	 // Sort names by their index of appearance
			std::sort(indexedNames.begin(), indexedNames.end());
			// Extract sorted names
			for (const auto& indexedName : indexedNames) {
				urls.push_back(indexedName.second);
				std::cout << indexedName.second << std::endl;
			}
        }
		else{
			std::cout << "Regex raw urls dataset is empty!" << std::endl;
		}
		/*
			start crawlling using the urls
		*/
		for(const auto& url : urls){
			/*
				save urls to txt files
			*/
			if(url.find("https://www.britannica.com/topic/") != std::string::npos && url.find("http") != std::string::npos
			&& url.find(".jpg") == std::string::npos && url.find("?") == std::string::npos
			&& url.find(".png") == std::string::npos && url.find(".css") == std::string::npos
			&& url.find(".js") == std::string::npos){
				write_url_to_file("/home/working/lib/db_tools/webUrls/urls.txt",url);
				str_stored_urls.push_back(url);
				std::cout << "url saved: " << url << std::endl;
			}
			else{
				//std::cout << "url not related to the website: " <<  url << std::endl;
				//erase items gn from std::vector<std::string>persons
				urls.erase(std::remove(urls.begin(), urls.end(), url), urls.end());//remove the item from the list
			}
		}
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
	}
	/*

	*/
	for(const auto& str_stored : str_stored_urls){
		auto it = std::find_if(str_stored_urls_get_urls.begin(), str_stored_urls_get_urls.end(), [str_stored](const std::string& s) {
            return s == str_stored;
		  });
		  if(it != str_stored_urls_get_urls.end()){
			//found
			continue;
		}
		str_stored_urls_get_urls.push_back(str_stored);
		break;
	}
}
void remove_str_stored_urls(const std::string& strurl){
	if(!strurl.empty()){
		str_stored_urls.erase(std::remove(str_stored_urls.begin(),str_stored_urls.end(), strurl),str_stored_urls.end());
		std::cout << "Successfully removed crawlled url:" << strurl << '\n';
	}
}
void start_crawlling(const std::string& strurl){
	std::cout << "Start getting the whole sites urls..." << '\n';
	get_one_page_urls(strurl);
	std::cout << "Finished getting the site's urls and saved the urls to the file." << '\n';
	/*
		start crawlling using the whole site's url list
	*/
	if(str_stored_urls.size()>0){
		std::string gpr = str_stored_urls[0];
		/*
			check if the url already crawlled
		*/
		auto it = std::find_if(str_crawelled_urls.begin(), str_crawelled_urls.end(), [gpr](const std::string& s) {
            return s == gpr;
		  });
		if(it != str_crawelled_urls.end()){
			//if already crawelled, remove from the list and move next
			remove_str_stored_urls(gpr);
			if(str_stored_urls.size()>0){
				start_crawlling(str_stored_urls[0]);
			}
		}
		/*
			check if the link is the broken link
		*/
		auto it_b = std::find_if(str_broken_urls.begin(), str_broken_urls.end(), [gpr](const std::string& s) {
            return s == gpr;
		  });
		if(it_b != str_broken_urls.end()){
			//if already crawelled, remove from the list and move next
			remove_str_stored_urls(gpr);
			if(str_stored_urls.size()>0){
				start_crawlling(str_stored_urls[0]);
			}
		}
		std::cout << "Start getting the content of " << gpr << std::endl;
		std::string htmlContent = getRawHtml(gpr);
		std::string getContent = get_content_updated(htmlContent);
		if(!getContent.empty()){
			std::vector<std::string> content_tokenized_by_sentences = split_sentences(getContent);
			if(content_tokenized_by_sentences.size()>0){
				std::string get_title = content_tokenized_by_sentences[0];
				std::ofstream file("/home/working/corpus/wiki/" + get_title + ".txt",std::ios::out);
				if (!file.is_open()) {
					file.open("/home/working/corpus/wiki/" + get_title + ".txt",std::ios::out);
				}
				file << getContent << '\n';
				file.close();
				write_url_to_file("/home/working/lib/db_tools/webUrls/crawlled_urls.txt",gpr);
				str_crawelled_urls.push_back(gpr);
			}
		}
		remove_str_stored_urls(gpr);
		if(str_stored_urls.size()>0){
			save_main_url_list("/home/working/lib/db_tools/webUrls/str_stored_urls.txt",str_stored_urls);
			start_crawlling(str_stored_urls[0]);
		}
	}
}
int main() {
	std::string str_file_path;
	
	/*
		broken link
	*/
	str_file_path = "/home/working/lib/db_tools/webUrls/broken_urls.txt";
	if(std::filesystem::exists(str_file_path)){
		str_broken_urls = get_url_list(str_file_path);
	}
	/*
		on-going links
	*/
	str_file_path = "/home/working/lib/db_tools/webUrls/str_stored_urls.txt";
	if(std::filesystem::exists(str_file_path)){
		str_stored_urls = get_url_list(str_file_path);
	}
	/*
		get waiting url list
	*/
	str_file_path = "/home/working/lib/db_tools/webUrls/urls.txt";
	if(std::filesystem::exists(str_file_path)){
		str_urls = get_url_list(str_file_path);
	}
	
	/*
		crawled urls
	*/
	str_file_path = "/home/working/lib/db_tools/webUrls/crawlled_urls.txt";
	if(std::filesystem::exists(str_file_path)){
		str_crawelled_urls = get_url_list(str_file_path);
	}
	start_crawlling("https://www.britannica.com/topic/Wikipedia");
	std::cout << "All jobs are done!" << std::endl;
    return 0;
}
