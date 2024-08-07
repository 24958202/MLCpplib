
#include <iostream>
#include <string>
#include <fstream>
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
#include <unordered_map>
#include <cctype>
#include <algorithm>
#include <thread>
#include <chrono>
#include <filesystem>
#include <cstdlib>
//#include <boost/algorithm/string.hpp>
#include "../lib/nemslib.h"

std::vector<std::string> str_broken_urls;
std::vector<std::string> str_urls;
std::vector<std::string> str_crawelled_urls;
std::vector<std::string> str_stored_urls;//under processing urls
std::vector<std::string> str_catalogs_wiki;
std::vector<std::string> str_books_txt;

std::string getCurrentDateTimeForTitle() {
    auto now = std::chrono::system_clock::now();
    auto now_time_t = std::chrono::system_clock::to_time_t(now);
    std::stringstream ss;
    ss << std::put_time(std::localtime(&now_time_t), "%Y%m%d_%H%M%S");
    return ss.str();
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
	if(raw_str.find("<title>")!=std::string::npos){
		return wSpider_j.findWordBehindSpan(raw_str,"<title>(.*?)</title>");
	}
	else{
		return "No title " + getCurrentDateTimeForTitle();
	}
}

std::string get_download_link_from_webpage(std::string& raw_str){
	//<p class="topic-paragraph"></p>
	if(raw_str.empty()){
		std::cout << "get_download_link_from_webpage: input empty!" << '\n';
		return "";
	}
	Jsonlib jsl_j;
	WebSpiderLib weblib_j;
	std::vector<std::string> main_content = weblib_j.findAllWordsBehindSpans(raw_str,"href=\"(.*?)\"");//"<tr class=\"even\" about=\"(.*?)\" typeof=\"pgterms:file\">");//"<td class=\"noscreen\">(.*?)</td>");
	std::string str_booklink = "https://www.gutenberg.org";
	std::string str_link_from_webpage;
	if(!main_content.empty()){
		for(const auto& mc : main_content){
			if(mc.find(".txt.utf-8") != std::string::npos){
				str_link_from_webpage = mc;
				break;
			}
			else{
				continue;
			}
		}
	}
	if(!str_link_from_webpage.empty()){
		if(str_link_from_webpage.find("http")==std::string::npos){
			if(str_link_from_webpage.substr(0,1) != "/"){
				str_link_from_webpage = "/" + str_link_from_webpage;
			}
			str_booklink.append(str_link_from_webpage);
		}
		else{
			str_booklink = str_link_from_webpage;
		}
		// std::string str_to_replace = ".utf-8";
		// str_booklink = jsl_j.str_replace(str_booklink,str_to_replace,"");
		str_booklink = std::string(weblib_j.str_trim(str_booklink));
		return str_booklink;
	}
	else{
		return "";
	}
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
void remove_str_stored_urls(const std::string& strurl){
	if(!strurl.empty()){
		str_stored_urls.erase(std::remove(str_stored_urls.begin(),str_stored_urls.end(), strurl),str_stored_urls.end());
		std::cout << "Successfully removed crawlled url:" << strurl << '\n';
	}
	save_main_url_list("/home/ronnieji/lib/db_tools/eBooks/webUrls/str_stored_urls.bin",str_stored_urls);
}
void get_one_page_urls(const std::string& url){
	//get all urls of the site
	Jsonlib jsl_j;
	WebSpiderLib wSpider_j;
	SysLogLib syslog_j;
	std::vector<std::string> urls;
	if(url.find(".images") != std::string::npos || url.find("copy.pglaf")!=std::string::npos 
			|| url.find("facebook") != std::string::npos || url.find("twitter") != std::string::npos 
			|| url.find("mastodon") != std::string::npos || url.find("policy") != std::string::npos 
			|| url.find("about") != std::string::npos || url.find("help") != std::string::npos
			|| url.find("donate") != std::string::npos || url.find("aka") != std::string::npos
			|| url.find("attic") != std::string::npos || url.find("copyleft") != std::string::npos
			|| url.find("wikipedia") != std::string::npos){
		remove_str_stored_urls(url);
		if(!str_stored_urls.empty()){
			get_one_page_urls(str_stored_urls[0]);
			return;
		}
	}
	std::string htmlContent = getRawHtml(url);
	std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
	/*
		if it's from outer wikipedia, move next
	*/
	if(!htmlContent.empty()){
		std::string ss_title = get_title_content(htmlContent); //wSpider_j.findWordBehindSpan(htmlContent,"<title>(.*?)</title>");
		std::cout << "The web page title: " << ss_title << '\n';
		if(ss_title.find("Gutenberg Copyright Clearance") != std::string::npos || url.find(".images") != std::string::npos || url.find("copy.pglaf")!=std::string::npos){
			syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog","Wrong content...");
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
    	syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog","Start clawlering >> " + url);
		if(htmlContent.empty()){
			str_broken_urls.push_back(url);
			write_url_to_file("/home/ronnieji/lib/db_tools/eBooks/webUrls/broken_urls.bin",url);
			syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", url + " >> The page is emtpy or it's an index");
			if(!str_stored_urls.empty()){
				remove_str_stored_urls(url);
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
			std::cout << strReturn << '\n';
			strReturn = std::string(wSpider_j.str_trim(strReturn));
			if(!strReturn.empty()){
				std::string str_booklink;
				if(strReturn.find("http") == std::string::npos){
					str_booklink = "https://www.gutenberg.org";
					if(strReturn.substr(0,1) != "/"){
						strReturn = "/" + strReturn;
					}
					str_booklink.append(strReturn);
				}
				else{
					str_booklink = strReturn;
				}
				if(str_booklink.back() == '.'){
					str_booklink = str_booklink.substr(0,str_booklink.size()-1);
				}
				indexedNames.push_back({match.position(0), str_booklink});
				searchStart = match.suffix().first;
				syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", str_booklink);
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
	WebSpiderLib web_j;
	if(strurl.find(".images") != std::string::npos || strurl.find("copy.pglaf")!=std::string::npos){
		remove_str_stored_urls(strurl);
		if(!str_stored_urls.empty()){
			start_crawlling(str_stored_urls[0]);
			return;
		}
	}
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
		std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
		std::string str_url_to_download;
		if(htmlContent.empty() || htmlContent.find("txt.utf-8") == std::string::npos || htmlContent.find("<td>English</td>") == std::string::npos){
			remove_str_stored_urls(gpr);
			if(!str_stored_urls.empty()){
				start_crawlling(str_stored_urls[0]);
				return;
			}
		}
		str_url_to_download = get_download_link_from_webpage(htmlContent);
		if(!str_url_to_download.empty()){
			/*
				add to str_books_txt
			*/
			std::cout << "Download link: " << str_url_to_download << '\n';
			str_books_txt.push_back(str_url_to_download);
			std::ofstream txtOut("/home/ronnieji/lib/db_tools/eBooks/booklist.txt",std::ios::app);
			if(!txtOut.is_open()){
				txtOut.open("/home/ronnieji/lib/db_tools/eBooks/booklist.txt",std::ios::app);
			}
			txtOut << str_url_to_download << '\n';
			txtOut.close();
			nl_j.AppendBinaryOne("/home/ronnieji/lib/db_tools/eBooks/booklist.bin",str_url_to_download);

			
			// syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Downloading >> ");
			// syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", str_url_to_download);
			// std::string str_file_path = "/home/ronnieji/corpus/ebooks_new/";
			// std::string str_title = get_title_content(htmlContent);
			// str_file_path.append(str_title);
			// str_file_path.append(".txt");
			// std::string strBook = web_j.GetURLContent(str_url_to_download);
			// std::this_thread::sleep_for(std::chrono::seconds(3));//seconds
			// try{
			// 	if(strBook.empty() || strBook.find("Language: English")==std::string::npos){
			// 		remove_str_stored_urls(gpr);
			// 		if(!str_stored_urls.empty()){
			// 			start_crawlling(str_stored_urls[0]);
			// 			return;
			// 		}
			// 	}
			// 	if(!strBook.empty()){
			// 		auto it = strBook.find("* START");
			// 		if(it != std::string::npos){
			// 			strBook = strBook.substr(it);
			// 			auto start_pos = strBook.find("***");
			// 			if(start_pos != std::string::npos){
			// 				strBook = strBook.substr(start_pos + 1);
			// 			}
			// 		}
			// 	}
			// }
			// catch(std::exception& e){
			// 	std::cerr << e.what() << '\n';
			// }
			// std::ofstream file(str_file_path,std::ios::out);
			// if(!file.is_open()){
			// 	file.open(str_file_path,std::ios::out);
			// }
			// file << strBook << '\n';
			// file.close();
			// nl_j.AppendBinaryOne(str_file_path,strBook);
			syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Successfully saved the file!");
			/*
				update the binary file 
			*/
			write_url_to_file("/home/ronnieji/lib/db_tools/eBooks/webUrls/crawlled_urls.bin",gpr);
			str_crawelled_urls.push_back(gpr);
		}
		remove_str_stored_urls(gpr);
		if(!str_stored_urls.empty()){
			/*
				update the binary file 
			*/
			save_main_url_list("/home/ronnieji/lib/db_tools/eBooks/webUrls/str_stored_urls.bin",str_stored_urls);
			start_crawlling(str_stored_urls[0]);
		}
		//std::this_thread::sleep_for(std::chrono::seconds(3));//seconds
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
	start_crawlling("https://www.gutenberg.org/ebooks/bookshelf/");
	std::cout << "All jobs are done!" << std::endl;
    return 0;
}
