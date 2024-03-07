#include <iostream>
#include <vector>
#include <string>
#include <filesystem>
#include <algorithm>
#include <fstream>
#include <thread>
#include <chrono>
#include <condition_variable>
#include <regex>
#include <cmath>
#include <unordered_map>
#include <numeric>
#include "nemslib.h"
#include "webcrawlerlib.h"
#include "authorinfo/author_info.h"
#define APP_VERSION "0.3"

std::string webcrawlerlib::extractDomainFromUrl(const std::string& url){
    std::regex domainRegex(R"(?:https?:\/\/)?(?:www\.)?([^\/]+)\.([a-zA-Z]{2,})");
    std::smatch match;
    if (std::regex_search(url, match, domainRegex)) {
		std::string fullDomain = match[1];
		size_t pos = fullDomain.find("://");
		if(pos != std::string::npos){
			return fullDomain.substr(pos+3);
		}
    } 
	return "";
}
std::string webcrawlerlib::getRawHtml(const std::string& strurl){
	WebSpiderLib wSpider_j;
	std::string str_content;
	try{
		str_content = wSpider_j.GetURLContent(strurl);
		return str_content;
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
	}
	return "";
}
std::string webcrawlerlib::get_title_content(std::string& raw_str){
	WebSpiderLib wSpider_j;
	return wSpider_j.findWordBehindSpan(raw_str,"<title>(.*?)</title>");
}
void webcrawlerlib::remove_str_is_crawling(const std::string& strurl){
	if(!strurl.empty()){
		if(!str_is_crawling.empty()){
			str_is_crawling.erase(std::remove(str_is_crawling.begin(),str_is_crawling.end(), strurl),str_is_crawling.end());
			std::cout << "Successfully removed crawled url:" << strurl << '\n';
		}
	}
}
void webcrawlerlib::write_url_to_binaryfile(const url_type& ult){
	nlp_lib nl_j;
	if(ult == url_type::url_broken && !str_broken.empty()){
		nl_j.WriteBinaryOne_from_std(str_broken,file_broken_link_path);
	}
	else if(ult == url_type::url_crawled && !str_crawled.empty()){
		nl_j.WriteBinaryOne_from_std(str_crawled,file_crawled_link_path);
	}
	else if(ult == url_type::url_crawling && !str_is_crawling.empty()){
		nl_j.WriteBinaryOne_from_std(str_is_crawling,file_crawling_link_path);
	}
	std::cout << "Successfully write the file to binary!" << '\n';
}
void webcrawlerlib::OutputHTMLContent(const std::string& str_title, const std::string& str_content){
	/*
		write the *.txt file
	*/
	if(str_title.empty() || str_content.empty()){
		std::cerr << "OutputHTMLContent: input empty!" << '\n';
		return;
	}
	std::string str_txt_path;
	std::string str_outpath;
	try{
		str_outpath = file_crawled_output_folder_path;
		if(!str_outpath.empty()){
			if(str_outpath.back()!='/'){
				str_outpath.append("/");
			}
			str_outpath.append(str_title);
		}
		else{
			std::cerr << "Output folder path is empty!" << '\n';
			return;
		}
		str_txt_path = str_outpath;
		str_txt_path.append(".txt");
		std::ofstream ofile(str_txt_path,std::ios::out);
		if(!ofile.is_open()){
			ofile.open(str_txt_path,std::ios::out);
		}
		ofile << str_content << '\n';
		ofile.close();
	}
	catch(const std::exception& e){
		std::cerr << "OutputHTMLContent: " << e.what() << '\n';
	}
	std::this_thread::sleep_for(std::chrono::milliseconds(2000));//seconds
	/*
		write the binary file to destination folder
	*/
	if(!str_txt_path.empty()){
		nlp_lib nl_j;
		nl_j.WriteBinaryOne_from_txt(str_txt_path);
	}
}
/*
	add std::condition_variable& cv to the parameter
	std::condition_variable& cv, 
*/
void webcrawlerlib::get_one_page_urls(const std::string& url){
	//get all urls of the site
	Jsonlib jsl_j;
	WebSpiderLib wSpider_j;
	nemslib nems_j;
	std::vector<std::string> urls;
	std::vector<std::string> fetched_template;
	std::string str_clean_web_content;
	std::vector<std::string> the_right_template;
	std::string str_domain;
	std::vector<unsigned int> int_url_length;
	/*
		start crawling
	*/
	std::cout << "Url to crawl: " << '\n';
	std::cout << url << '\n';
	/*
		check if it's a qualified url
	*/
	if(url.find("http") == std::string::npos
				&& url.find(".jpg") != std::string::npos && url.find(".jpeg") != std::string::npos
				&& url.find(".gif") != std::string::npos && url.find(".png") != std::string::npos 
				&& url.find(".css") != std::string::npos
				&& url.find(".js") != std::string::npos){
		std::cout << "Bad url, move next... " << '\n';
		str_broken.push_back(url);//add to broken link list
		write_url_to_binaryfile(url_type::url_broken);//write to binary file
		return;
	}

	std::string htmlContent = getRawHtml(url);
	std::this_thread::sleep_for(std::chrono::seconds(5));//seconds
	try{
		if(htmlContent.empty()){
			str_broken.push_back(url);//add to broken link list
			write_url_to_binaryfile(url_type::url_broken);//write to binary file
			return;
		}
		std::string str_title = get_title_content(htmlContent);
		if(str_title.empty()){
			str_broken.push_back(url);//add to broken link list
			write_url_to_binaryfile(url_type::url_broken);//write to binary file
			std::cout << "The title is empty, bad url, move next..." << '\n';
			return;
		}
		str_domain = extractDomainFromUrl(url);
		std::string url_rule = "(https?:\\/\\/)?(www\\.)?([a-zA-Z0-9-]+\\.){1,}[a-zA-Z]{2,}(\\/[a-zA-Z0-9\\/.-]*)?";
		std::vector<std::pair<size_t, std::string>> indexedNames;
		std::regex pattern = std::regex(url_rule);
        std::smatch match;
        std::string::const_iterator searchStart(htmlContent.cbegin());
        std::string strReturn;
        while (std::regex_search(searchStart, htmlContent.cend(), match, pattern)) {
            strReturn = match[0].str();
            strReturn = std::string(wSpider_j.str_trim(strReturn));
            if(!strReturn.empty()){
				std::cout << "match.position() :" << match.position() << '\n';
                indexedNames.push_back({match.position(), strReturn});
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
				std::string str_url_fetch = indexedName.second;
				str_url_fetch = std::string(wSpider_j.str_trim(str_url_fetch));
				urls.push_back(str_url_fetch);
				int_url_length.push_back(str_url_fetch.size());
				std::cout << str_url_fetch << std::endl;
			}
        }
		else{
			std::cout << "Regex raw urls dataset is empty!" << std::endl;
		}
		/*
			remove unnecessary links, images links etc.
		*/
		if(!urls.empty()){
			std::cout << "verifing url link list..." << '\n';
			for(const auto& url : urls){
				std::cout << "Checking link: >> " << url << '\n'; 
				/*
					save urls to txt files
				*/
				if(url.find("http") != std::string::npos
				&& url.find(".jpg") == std::string::npos && url.find(".jpeg") == std::string::npos
				&& url.find(".gif") == std::string::npos && url.find(".png") == std::string::npos 
				&& url.find(".css") == std::string::npos && url.find(".js") == std::string::npos){
					std::cout << "url saved: " << url << std::endl;
				}
				else{
					//std::cout << "url not related to the website: " <<  url << std::endl;
					//erase items gn from std::vector<std::string>persons
					urls.erase(std::remove(urls.begin(), urls.end(), url), urls.end());//remove the item from the list
					std::cout << "It's a bad link >> " << url << '\n';
				}
			}
		}
		/*
			remove url's length is a lot of different from others (< max_length/2)
		*/
		if(!urls.empty()){
			auto max_length = std::ranges::max_element(urls, [](const std::string& a, const std::string& b){
				return a.size() < b.size();
			})->size();
			for(auto it = urls.begin(); it != urls.end();){
				if((*it).size() < max_length/2){
					it = urls.erase(it);
				}
				else{
					++it;
				}
			}
		}
		
		//for(auto it=)
		/*
			remove the links exists in str_is_crawling, and save all urls into str_is_crawling
		*/
		for(const auto& ul : urls){
			for(auto it=str_broken.begin(); it != str_broken.end();){
				if(*it == ul){
					urls.erase(std::remove(urls.begin(), urls.end(), ul), urls.end());
				}
				else{
					++it;
				}
			}
			for(auto it=str_crawled.begin(); it != str_crawled.end();){
				if(*it == ul){
					urls.erase(std::remove(urls.begin(), urls.end(), ul), urls.end());
				}
				else{
					++it;
				}
			}
			for(auto it=str_is_crawling.begin(); it != str_is_crawling.end();){
				if(*it == ul){
					urls.erase(std::remove(urls.begin(), urls.end(), ul), urls.end());
				}
				else{
					++it;
				}
			}
		}
		if(!urls.empty()){
			str_is_crawling.insert(str_is_crawling.end(),urls.begin(),urls.end());
		}
		/*
			start analying htmlContent
		*/
		std::string web_content_without_html_tags = wSpider_j.G_removehtmltags(htmlContent);
		std::cout << "********************Content**********************" << '\n';
		std::cout << web_content_without_html_tags << '\n';
		std::cout << "*************************************************" << '\n';
		if(!web_content_without_html_tags.empty()){
			fetched_template = nems_j.tokenize_en(web_content_without_html_tags);
			if(!fetched_template.empty()){
				if(website_template.size()>0){//already has webpage saved for this site
					for(const auto& wt : website_template){
						if(wt.first == str_domain){
							for(const auto& ft : fetched_template){
								if(ft != wt.second){
									str_clean_web_content += ft + " ";
								}
								else{
									the_right_template.push_back(ft);
								}
							}
						}
					}
					if(!the_right_template.empty()){
						if (!str_last_url.empty() && str_last_url == str_domain) {
							// if it's an adjusted template by removing the old template
							std::erase_if(website_template, [&str_domain](const auto& pair) { return pair.first == str_domain; });	
							for (const auto& wt : the_right_template) {
								website_template[str_domain] = wt;
							}
							str_last_url = str_domain;
						}
						the_right_template.clear();
					}
				}
				else{
					for(const auto& ft : fetched_template){
						website_template[str_domain] = ft;
						str_clean_web_content += ft + " ";
					}
					str_last_url = str_domain;
				}
			}
		}
		else{
			std::cerr << "web_content_without_html_tags is empty!" << '\n';
		}
		/*
			save the content
			output one *.txt file, one binary file
			str_clean_web_content
		*/
		str_clean_web_content = std::string(wSpider_j.str_trim(str_clean_web_content));

		OutputHTMLContent(str_title,str_clean_web_content);
		/*
			remove the link from str_to_crawle when the content was saved.
		*/
		str_is_crawling.erase(std::remove(str_is_crawling.begin(), str_is_crawling.end(), url), str_is_crawling.end());
		write_url_to_binaryfile(url_type::url_crawling);
		std::this_thread::sleep_for(std::chrono::milliseconds(500));//seconds
		/*
			update the crawled list and crawled binary file
		*/
		str_crawled.push_back(url);
		write_url_to_binaryfile(url_type::url_crawled);
	}
	catch(const std::exception& e){
		std::cerr << e.what() << '\n';
	}
	/*
		notify the main thread if exceed the specific time
	*/
	//cv.notify_one();
}
void webcrawlerlib::crawling_the_www(const std::string& the_link_spider_to_crawl){
	/*
		check if the link is in the broken url list or crawled url list
	*/
	if(!str_broken.empty()){
		auto it = std::find(str_broken.begin(),str_broken.end(),the_link_spider_to_crawl);
		if(it != str_broken.end()){
			str_is_crawling.erase(std::remove(str_is_crawling.begin(), str_is_crawling.end(), the_link_spider_to_crawl), str_is_crawling.end());
			write_url_to_binaryfile(url_type::url_crawling);
			if(str_is_crawling.size()>0){
				crawling_the_www(str_is_crawling[0]);
				return;
			}
		}
	}
	if(!str_crawled.empty()){
		auto it = std::find(str_crawled.begin(),str_crawled.end(),the_link_spider_to_crawl);
		if(it != str_crawled.end()){
			str_is_crawling.erase(std::remove(str_is_crawling.begin(), str_is_crawling.end(), the_link_spider_to_crawl), str_is_crawling.end());
			write_url_to_binaryfile(url_type::url_crawling);
			if(str_is_crawling.size()>0){
				crawling_the_www(str_is_crawling[0]);
				return;
			}
		}
	}
	get_one_page_urls(the_link_spider_to_crawl);
	if(str_is_crawling.size()>0){
		crawling_the_www(str_is_crawling[0]);
	}
}
void webcrawlerlib::start_crawling(){
   if(!str_is_crawling.empty()){//continue on-going task
		crawling_the_www(str_is_crawling[0]);
   }
   else{
		/*
			a new crawl task
		*/
		if(!str_to_crawle.empty()){
			//put the str_to_crawle to str_is_crawling
			str_is_crawling.insert(str_is_crawling.end(),str_to_crawle.begin(),str_to_crawle.end());
			crawling_the_www(str_is_crawling[0]);
		}
		else{
			std::cerr << "start_crawling: main list file is empty!" << '\n';
			std::string strInput;
			std::cout << "Please enter a url address to crawl: " << '\n';
			std::getline(std::cin,strInput);
			if(!strInput.empty()){
				str_is_crawling.push_back(strInput);
				crawling_the_www(str_is_crawling[0]);
			}
		}
   }
   std::cout << "All jobs are done!" << '\n';
}
void webcrawlerlib::start_working(const std::string& url_list){
    nlp_lib nl_j;
    /*
        check unfinished jobs if exists
    */
    if(std::filesystem::exists(file_broken_link_path)){
		str_broken = nl_j.ReadBinaryOne(file_broken_link_path);
	}
    if(std::filesystem::exists(file_crawled_link_path)){
		str_crawled = nl_j.ReadBinaryOne(file_crawled_link_path);
	}
    if(std::filesystem::exists(file_crawling_link_path)){
		str_is_crawling = nl_j.ReadBinaryOne(file_crawling_link_path);
	}
	/*
        check broken links list, if exists in str_is_crawling, remove it
   */
  	/*
		remove broken link list from str_is_crawling
	*/
  	if(!str_broken.empty() && !str_is_crawling.empty()){
		for(const auto& item : str_broken){
			for(auto it=str_is_crawling.begin(); it != str_is_crawling.end();){
				if(*it == item){
					it = str_is_crawling.erase(it);
				}
				else{
					++it;
				}
			}
		}
	}
	/*
		remove crawled link list from str_is_crawling
	*/
	if(!str_crawled.empty() && !str_is_crawling.empty()){
		for(const auto& item : str_crawled){
			for(auto it=str_is_crawling.begin(); it != str_is_crawling.end();){
				if(*it == item){
					it = str_is_crawling.erase(it);
				}
				else{
					++it;
				}
			}
		}
	}
    if(str_is_crawling.empty()){
		if(std::filesystem::exists(url_list)){
			str_to_crawle = nl_j.ReadBinaryOne(url_list);
		}
        else{
			std::cerr << "Could not find the main url_list file!" << '\n';
		}
    }
	start_crawling();
}
