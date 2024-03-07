#ifndef WEBCRAWERLIB_H
#define WEBCRAWERLIB_H
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
#include <condition_variable>
class webcrawlerlib{
    private:
        std::unordered_map<std::string,std::string> website_template;
        std::vector<std::string> str_crawled;
        std::vector<std::string> str_to_crawle;
        std::vector<std::string> str_is_crawling;
        std::vector<std::string> str_broken; 
        enum url_type{
            url_broken,
            url_crawled,
            url_crawling
        };
        std::string extractDomainFromUrl(const std::string&);
        std::string getRawHtml(const std::string&);
        std::string get_title_content(std::string&);
        void remove_str_is_crawling(const std::string&);
        void write_url_to_binaryfile(const url_type&);
        void OutputHTMLContent(const std::string&, const std::string&);
        void get_one_page_urls(const std::string&);
        void crawling_the_www(const std::string&);
        void start_crawling();
    public:
        std::string file_broken_link_path;
        std::string file_crawled_link_path;
        std::string file_crawling_link_path;
        std::string file_crawled_output_folder_path;
        std::string str_last_url;
        /*
            input a main url list *.txt file, each url per line
            initialize the file location:
            spiderlib::file_broken_link_path = 
        */
        void start_working(const std::string&);
};
#endif
