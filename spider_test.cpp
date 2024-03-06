#include <iostream>
#include "../lib/spiderlib.h"

int main(){
    spiderlib spd_j;
    spiderlib::file_broken_link_path = "/home/ronnieji/lib/db_tools/webUrls/broken_urls.bin";
    spiderlib::file_crawled_link_path = "/home/ronnieji/lib/db_tools/webUrls/crawlled_urls.bin";
    spiderlib::file_crawling_link_path = "/home/ronnieji/lib/db_tools/webUrls/str_stored_urls.bin";
    spiderlib::file_crawled_output_folder_path = "/home/ronnieji/corpus/wiki_new/";
    spiderlib::start_working("/home/ronnieji/lib/db_tools/webUrls/url_list.bin");
    
}