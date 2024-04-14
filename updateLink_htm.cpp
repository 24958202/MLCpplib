#include <iostream>
#include <vector>
#include <string>
#include <fstream>
#include <chrono>
#include <thread>
#include "../lib/nemslib.h"

void download_files(const std::string& strUrl,const std::string& out_filename){
    WebSpiderLib web_j;
    SysLogLib syslog_j;
    nlp_lib nl_j;
    syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Downloading >> ");
    syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", strUrl);
    std::string str_file_path = "/home/ronnieji/corpus/ebooks_new/";
    std::string strBook = web_j.GetURLContent(strUrl);
    std::this_thread::sleep_for(std::chrono::seconds(3));//seconds
    try{
        if(!strBook.empty()){
            auto it = strBook.find("* START");
            if(it != std::string::npos){
                strBook = strBook.substr(it);
                auto start_pos = strBook.find("***");
                if(start_pos != std::string::npos){
                    strBook = strBook.substr(start_pos + 1);
                }
            }
            auto end_pos = strBook.find("*** END OF THE PROJECT");
            strBook = strBook.substr(0,end_pos);
        }
    }
    catch(std::exception& e){
        std::cerr << e.what() << '\n';
    }
    str_file_path.append(out_filename);
    std::ofstream file(str_file_path,std::ios::out);
    if(!file.is_open()){
        file.open(str_file_path,std::ios::out);
    }
    file << strBook << '\n';
    file.close();
    nl_j.AppendBinaryOne(str_file_path,strBook);
    syslog_j.writeLog("/home/ronnieji/lib/db_tools/eBooks/wikiLog", "Successfully saved the file!");
}
void u_link(){
    nlp_lib nl_j;
    Jsonlib jsl_j;
    std::vector<std::string> links;
    std::vector<std::string> getAllLinks = nl_j.ReadBinaryOne("/home/ronnieji/lib/db_tools/updateLink/booklist.bin");
    if(!getAllLinks.empty()){
        for(const auto& ga : getAllLinks){
            //https://www.gutenberg.org/ebooks/32292.txt.utf-8
            //new address: http://www.gutenberg.org/cache/epub/32292/pg32292.txt       
            std::cout << "Checking the link: " << ga << '\n';
            std::vector<std::string> split_link = jsl_j.splitString(ga,'.');
            if(!split_link.empty()) {
                std::vector<std::string> get_num = jsl_j.splitString(split_link[2],'/');
                if(!get_num.empty() && get_num.size() > 1){
                   std::string f_num =  get_num[2]; 
                   f_num = jsl_j.trim(f_num);    
                   std::string f_link =  "http://www.gutenberg.org/cache/epub/";
                   f_link.append(f_num);
                   f_link.append("/");
                   f_link.append("pg");
                   f_link.append(f_num);
                   f_link.append("-images");
                   f_link.append(".html");
                   /*
                        download the file
                   */
                    download_files(f_link,f_num + ".txt");
                   /*
                        save the file
                   */
                   links.push_back(f_link); 
                   std::this_thread::sleep_for(std::chrono::seconds(5));//seconds
                }     
            }
        }
        if(!links.empty()){
            std::ofstream file("/home/ronnieji/lib/db_tools/updateLink/u_booklist.txt",std::ios::app);
            if(!file.is_open()){
                file.open("/home/ronnieji/lib/db_tools/updateLink/u_booklist.txt",std::ios::app);            
            }     
            for(const auto& ls : links){
                file << ls << '\n';   
                std::cout << ls << '\n';         
            }   
            file.close();
            std::cout << "All jobs are done!" << '\n';
        }
    }
}

int main(){
    u_link();
    return 0;
}
