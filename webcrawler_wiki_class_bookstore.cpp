
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
std::vector<std::string> str_stored_urls;//under processing urls
std::vector<std::string> str_catalogs_wiki;

void remove_str_stored_urls(const std::string& strurl){
	if(!strurl.empty()){
		str_stored_urls.erase(std::remove(str_stored_urls.begin(),str_stored_urls.end(), strurl),str_stored_urls.end());
		std::cout << "Successfully removed crawlled url:" << strurl << '\n';
	}
}
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
	return wSpider_j.findWordBehindSpan(raw_str,"<title>(.*?)</title>");
}
std::vector<std::string> get_key_words(std::string& raw_str){
	std::vector<std::string> keywords;
	Jsonlib nem_j;
	if(raw_str.empty()){
		std::cout << "get_key_words: input empty!" << '\n';
		return keywords;
	}
	WebSpiderLib wSpider_j;
	std::string str_keyword = wSpider_j.findWordBehindSpan(raw_str,"<meta name=\"keywords\" content=\"(.*?)\" />");
	if(!str_keyword.empty()){
		keywords = nem_j.splitString(str_keyword,',');
	}
	return keywords;
}
std::string get_topic_content(std::string& raw_str){
	size_t pos_start = 0;
    size_t pos_end = 0;
    std::string str_phrased_content;
    std::string str_start = "<!--[BP]--><p class=\"topic-paragraph\">"; //"<!--[BEFORE-ARTICLE]--><span class=\"marker before-article\"></span><section id=\"ref\" data-level=\"\"><p class=\"topic-paragraph\"><strong><span id=\"ref883486\"></span>";
    std::string str_end = "</p><!--[P9]-->"; //"</p><!--[P0]--><span class=\"marker p0\"></span><!--[AM0]--><span class=\"marker AM0 am-inline\"></span>";
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
std::string get_download_link_from_webpage(std::string& raw_str){
	//<p class="topic-paragraph"></p>
	if(raw_str.empty()){
		std::cout << "get_download_link_from_webpage: input empty!" << '\n';
		return "";
	}
	Jsonlib jsl_j;
	WebSpiderLib weblib_j;
	std::string main_content = weblib_j.findWordBehindSpan(raw_str,"<tr class=\"even\" about=\"(.*?)\" typeof=\"pgterms:file\">");//"<td class=\"noscreen\">(.*?)</td>");
	std::string str_to_replace = ".utf-8";
	main_content = jsl_j.str_replace(main_content,str_to_replace,"");
	main_content = std::string(weblib_j.str_trim(main_content));
	return main_content;
}
void write_url_to_file(const std::string& file_path, const std::string& url){
	nlp_lib nl_j;
	nl_j.AppendBinaryOne(file_path,url);
}
void save_main_url_list(const std::string&file_path,const std::vector<std::string> urls){
	if(urls.size()==0){
		std::cerr << "save_main_url_list: input empty!" << '\n';
		return;
	}
	nlp_lib nl_j;
	nl_j.WriteBinaryOne_from_std(urls,file_path);
}
void get_one_page_urls(const std::string& url){
	//get all urls of the site
	Jsonlib jsl_j;
	WebSpiderLib wSpider_j;
	SysLogLib syslog_j;
	std::vector<std::string> urls;
	std::string htmlContent = getRawHtml(url);
	std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
	/*
		if it's from outer wikipedia, move next
	*/
	if(!htmlContent.empty()){
		std::string ss_title = get_title_content(htmlContent); //wSpider_j.findWordBehindSpan(htmlContent,"<title>(.*?)</title>");
		std::cout << "The web page title: " << ss_title << '\n';
		if(ss_title.find("Gutenberg") == std::string::npos){
			syslog_j.writeLog("/home/ronnieji/lib/db_tools/wikiLog","This site does not belong to britannica.");
			/*
				erase from str_stored_urls
			*/
			remove_str_stored_urls(url);
			if(!str_stored_urls.empty()){
				get_one_page_urls(str_stored_urls[0]);
				return;
			}
		}
	}
	try{
    	syslog_j.writeLog("/home/ronnieji/lib/db_tools/wikiLog","Start clawlering >> " + url);
		if(htmlContent.empty()){
			str_broken_urls.push_back(url);
			write_url_to_file("/home/ronnieji/lib/db_tools/webUrls/broken_urls.bin",url);
			if(!str_stored_urls.empty()){
				syslog_j.writeLog("/home/ronnieji/lib/db_tools/wikiLog", url + " >> The page was empty!");
				get_one_page_urls(str_stored_urls[0]);
				return;
			}
		}
		std::string url_rule = "href=\"(.*?)\""; //"(https?:\\/\\/)?(www\\.)?([a-zA-Z0-9-]+\\.){1,}[a-zA-Z]{2,}(\\/[a-zA-Z0-9\\/\\.-]*)?"; // "(https?:\\/\\/)?(www\\.)?([a-zA-Z0-9-]+\\.)+[a-zA-Z]{2,}(:\\d+)?(\\/[a-zA-Z0-9\\/.-]*)?(\?[a-zA-Z0-9=&.]+)?(#.*)?"; //"(https?:\\/\\/)?(www\\.)?([a-zA-Z0-9-]+\\.){1,}[a-zA-Z]{2,}(\\/[a-zA-Z0-9\\/\\.-]*)?";
		std::vector<std::pair<size_t, std::string>> indexedNames;
		std::regex pattern = std::regex(url_rule);
        std::smatch match;
        std::string::const_iterator searchStart(htmlContent.cbegin());
        std::string strReturn;
        while (std::regex_search(searchStart, htmlContent.cend(), match, pattern)) {
            strReturn = match.str(1);
            strReturn = std::string(wSpider_j.str_trim(strReturn));
            if(!strReturn.empty()){
				std::string str_booklink = "https://www.gutenberg.org";
				if(strReturn.substr(0,1) != "/"){
					strReturn = "/" + strReturn;
				}
				str_booklink.append(strReturn);
                indexedNames.push_back({match.position(0), str_booklink});
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
				save candidated urls to txt files
			*/
			if(url.find("https://www.gutenberg.org") != std::string::npos && url.find("http") != std::string::npos
			&& url.find(".jpg") == std::string::npos && url.find("?") == std::string::npos
			&& url.find(".png") == std::string::npos && url.find(".css") == std::string::npos
			&& url.find(".js") == std::string::npos){
				write_url_to_file("/home/ronnieji/lib/db_tools/eBooks/webUrls/urls.bin",url);
				str_stored_urls.push_back(url);
				std::cout << "url saved: " << url << std::endl;
			}
			else{
				//std::cout << "url not related to the website: " <<  url << std::endl;
				//erase items gn from std::vector<std::string>persons
				urls.erase(std::remove(urls.begin(), urls.end(), url), urls.end());//remove the bad item from the list
			}
			/*
				Erase the bad urls
			*/
			/*
				check if the url already crawlled
			*/
			if(!str_crawelled_urls.empty()){
				auto it = std::find_if(str_crawelled_urls.begin(), str_crawelled_urls.end(), [url](const std::string& s) {
					return s == url;
				});
				if(it != str_crawelled_urls.end()){
					//found
					if(!str_stored_urls.empty()){
						remove_str_stored_urls(url);
					}
				}
			}
			/*
				check if the link is the broken link
			*/
			if(!str_broken_urls.empty()){
				auto it_b = std::find_if(str_broken_urls.begin(), str_broken_urls.end(), [url](const std::string& s) {
					return s == url;
				});
				if(it_b != str_broken_urls.end()){
					//found
					if(!str_stored_urls.empty()){
						remove_str_stored_urls(url);
					}
				}
			}	
		}
		/*
			saved str_stored_urls
		*/
		save_main_url_list("/home/ronnieji/lib/db_tools/eBooks/webUrls/str_stored_urls.bin",str_stored_urls);
		
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
	}
}
void start_crawlling(const std::string& strurl){
	SysLogLib syslog_j;
	Jsonlib jsl_j;
	nlp_lib nl_j;
	nemslib nem_j;
	syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Start getting the whole sites urls...");
	get_one_page_urls(strurl);
	syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Finished getting the site's urls and saved the urls to the file.");
	/*
		start crawlling using the whole site's url list
	*/
	if(!str_stored_urls.empty()){
		for(const auto& ss : str_stored_urls){
			std::cout << ss << '\n';
		}
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
			if(!str_stored_urls.empty()){
				start_crawlling(str_stored_urls[0]);
				return;
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
			if(!str_stored_urls.empty()){
				start_crawlling(str_stored_urls[0]);
				return;
			}
		}
		std::cout << "Start getting the content of " << gpr << std::endl;
		std::string htmlContent = getRawHtml(gpr);
		std::string str_url_to_download;
		if(!htmlContent.empty()){
			str_url_to_download = get_download_link_from_webpage(htmlContent);
		}
		syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Downloading >> " + str_url_to_download);
		if(!str_url_to_download.empty()){
			std::string str_book_content = getRawHtml(str_url_to_download);
			if(!str_book_content.empty()){
				auto title_pos_last = str_book_content.find("This ebook is for the use of anyone anywhere");
				std::string str_title = str_book_content.substr(0,title_pos_last);
				std::cout << "The book title is: >> " << str_title << '\n';
				if(str_book_content.find("Language: English") != std::string::npos){
					auto it = str_book_content.find("*** START OF THE PROJECT GUTENBERG EBOOK MOBY DICK; OR, THE WHALE ***");
					std::string final_book_content = str_book_content.substr(it);
					if(!str_title.empty() && !final_book_content.empty()){
						std::string str_file_path = "/home/ronnieji/corpus/ebooks_new/";
						str_file_path.append(str_title);
						str_file_path.append(".txt");
						/*
							save *.txt file
						*/
						std::ofstream file(str_file_path,std::ios::out);
						if(!file.is_open()){
							file.open(str_file_path,std::ios::out);
						}
						file << final_book_content << '\n';
						file.close();
						/*
							save *.bin file
						*/
						std::string str_txt = ".txt";
						str_file_path = jsl_j.str_replace(str_file_path,str_txt,".bin");
						nl_j.AppendBinaryOne(str_file_path,final_book_content);
						syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Successfully saved the file!");
						/*
							update the binary file 
						*/
						write_url_to_file("/home/ronnieji/lib/db_tools/eBooks/webUrls/crawlled_urls.bin",gpr);
						str_crawelled_urls.push_back(gpr);
					}
				}
			}
		}
		remove_str_stored_urls(gpr);
		if(!str_stored_urls.empty()){
			/*
				update the binary file 
			*/
			save_main_url_list("/home/ronnieji/lib/db_tools/eBooks/webUrls/str_stored_urls.bin",str_stored_urls);
			start_crawlling(str_stored_urls[0]);
		}
	}
}
int main() {
	std::string str_file_path;
	/*
		broken link
	*/
	str_file_path = "/home/ronnieji/lib/db_tools/eBooks/webUrls/broken_urls.bin";
	if(std::filesystem::exists(str_file_path)){
		str_broken_urls = get_url_list(str_file_path);
	}
	/*
		on-going links
	*/
	str_file_path = "/home/ronnieji/lib/db_tools/eBooks/webUrls/str_stored_urls.bin";
	if(std::filesystem::exists(str_file_path)){
		str_stored_urls = get_url_list(str_file_path);
	}
	/*
		get waiting url list
	*/
	str_file_path = "/home/ronnieji/lib/db_tools/eBooks/webUrls/urls.bin";
	if(std::filesystem::exists(str_file_path)){
		str_urls = get_url_list(str_file_path);
	}
	/*
		get catalogs for wiki
	*/
	str_file_path = "/home/ronnieji/lib/db_tools/eBooks/webUrls/catalogs_wiki.bin";
	if(std::filesystem::exists(str_file_path)){
		str_catalogs_wiki = get_url_list(str_file_path);
	}
	/*
		crawled urls
	*/
	str_file_path = "/home/ronnieji/lib/db_tools/eBooks/webUrls/crawlled_urls.bin";
	if(std::filesystem::exists(str_file_path)){
		str_crawelled_urls = get_url_list(str_file_path);
	}
	start_crawlling("https://www.gutenberg.org/");
	std::cout << "All jobs are done!" << std::endl;
    return 0;
}
