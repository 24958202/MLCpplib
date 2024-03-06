#ifndef SPIDERLIB_H
#define SPIDERLIB_H
#include <iostream>
#include <vector>
#include <string>
#include <unordered_map>
class spiderlib{
    public:
        static std::string file_broken_link_path;
        static std::string file_crawled_link_path;
        static std::string file_crawling_link_path;
        static std::string file_crawled_output_folder_path;
        static std::string str_last_url;
        static std::unordered_map<std::string,std::string> website_template;
        static std::vector<std::string> str_crawled;
        static std::vector<std::string> str_to_crawle;
        static std::vector<std::string> str_is_crawling;
        static std::vector<std::string> str_broken; 
        enum url_type{
            url_broken,
            url_crawled,
            url_crawling
        };
        /*
            para1: pass the main url list (*.bin) file
        */
        static std::string extractDomainFromUrl(const std::string&);
        /*
            input a main url list *.txt file, each url per line
            initialize the file location:
            spiderlib::file_broken_link_path = 
        */
        static void start_working(const std::string&);
};
#endif
