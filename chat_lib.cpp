#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <set>
#include "nemslib.h"
#include "chat_lib.h"
class binary_file_lib{
    public:
        size_t write_all_voc(const std::string& all_voc, const std::string& str_to_write);
        void write_binary_file(const std::string& file_path, const std::string& str_to_write);
};
size_t binary_file_lib::write_all_voc(const std::string& all_voc, const std::string& str_to_write){
    if(all_voc.empty() || str_to_write.empty()){
        return -1;
    }
    nemslib nem_j;
    size_t pos = -1;
    std::vector<std::string> get_content_from_all_voc;
    if(std::filesystem::exists(all_voc)){
        get_content_from_all_voc = nem_j.readTextFile(all_voc);
        if(!get_content_from_all_voc.empty()){
            auto it = std::find(get_content_from_all_voc.begin(),get_content_from_all_voc.end(),str_to_write);
            if(it != get_content_from_all_voc.end()){
                pos = std::distance(get_content_from_all_voc.begin(), it);
                return pos;
            }
        }
    }
    get_content_from_all_voc.push_back(str_to_write);
    /*
        write to file
    */
    std::ofstream out_file(all_voc,std::ios::app);
    if(!out_file.is_open()){
        out_file.open(all_voc,std::ios::app);
    }
    out_file << str_to_write << '\n';
    out_file.close();
    pos = get_content_from_all_voc.size();
    return pos;
}
void binary_file_lib::write_binary_file(const std::string& file_path, const std::string& str_to_write){
    if(str_to_write.empty()){
        return;
    }
    /*
        check existance str_to_write
    */
    nemslib nem_j;
    if(std::filesystem::exists(file_path)){
       std::vector<std::string> get_content_from_all_voc = nem_j.readTextFile(file_path);
       if(!get_content_from_all_voc.empty()){
            auto it = std::find(get_content_from_all_voc.begin(),get_content_from_all_voc.end(),str_to_write);
            if(it != get_content_from_all_voc.end()){
                return;
            }
       }
    }
    std::ofstream out_file(file_path,std::ios::app);
    if(!out_file.is_open()){
        out_file.open(file_path,std::ios::app);
    }
    out_file << str_to_write << '\n';
    out_file.close();
}
/*
    para1:input_folder_path, para2:stopwordListPath, para3:output_log_path,para4: trained_all_voc.bin path, para5:all_voc.bin path, para6:book_x_y.bin path
*/
void chat_lib::write_books_mysql(const std::string& input_folder_path,const std::string& stopwordListPath,const std::string& output_log_path,const std::string& trained_all_voc, const std::string& all_voc, const std::string& book_x_y_path){
    if(input_folder_path.empty() || stopwordListPath.empty() || output_log_path.empty() || trained_all_voc.empty() || all_voc.empty() || book_x_y_path.empty()){
        return;
    }
    WebSpiderLib jsl_j;
    nemslib nems_j;/* initialize stop word*/
    binary_file_lib bflib;
    SysLogLib syslog_j;
    syslog_j.writeLog(output_log_path,"Initialize stopword list...");
    nems_j.set_stop_word_file_path(stopwordListPath);
    for (const auto& entry : std::filesystem::directory_iterator(input_folder_path)) {
        if (entry.is_regular_file() && entry.path().extension() == ".txt") {
            std::ifstream in_file(entry.path());
            syslog_j.writeLog(output_log_path,"Reading book: ");
            syslog_j.writeLog(output_log_path,entry.path());
            if (in_file.is_open()) {
                std::string line;
                std::string str_book;
                std::vector<std::string> book_token;
                std::vector<std::string> book_token_xy;
                std::vector<std::unordered_map<std::string,unsigned int>> get_topics_withf;
                std::string st_book_temp;
                std::string str_book_saved;
                std::string str_topics_without_freq;
                std::string str_topics_with_freq;
                std::string book_y;//y-coordinate
                std::string book_x_y;
                while (std::getline(in_file, line)) {
                    line = std::string(jsl_j.str_trim(line));
                    str_book += line + " ";
                }
                syslog_j.writeLog(output_log_path,"Getting the book's topic...");
                /*
                    get the topics of the article
                */
                get_topics_withf = nems_j.get_topics_freq(str_book);
                unsigned int topic_count = 0;
                for(const auto& gtw : get_topics_withf){
                    topic_count++;
                    if(topic_count>48){
                        break;
                    }
                    for(const auto& gt : gtw){
                        std::stringstream sss;
                        sss << gt.first << "-" << gt.second << ",";
                        str_topics_with_freq += sss.str();
                        str_topics_without_freq += gt.first + ",";
                    }
                }
                str_topics_without_freq = str_topics_without_freq.substr(0,str_topics_without_freq.size()-1);
                str_topics_with_freq = str_topics_with_freq.substr(0,str_topics_with_freq.size()-1);
                /*
                    start saving the binary
                */
                //syslog_j.writeLog(output_log_path,"Tokenizing the book...");
                book_token = nems_j.tokenize_en(str_book);
                if(!book_token.empty()){
                    //syslog_j.writeLog(output_log_path,"Adding the book to the library...");
                    for(const auto& bt : book_token){
                        std::cout << bt << '\n';
                        try{
                            /*
                                check the existance of bt
                            */
                            size_t b_pos = bflib.write_all_voc(all_voc,bt);
                            if(b_pos != -1){
                                book_token_xy.push_back(std::to_string(b_pos));
                            }
                        }
                        catch(const std::exception& e){
                            std::cerr << e.what() << '\n';
                        }
                    }
                    book_x_y="";
                    book_y = "";
                    unsigned int i = 0;
                    if(!book_token_xy.empty()){
                        for(const auto& bt : book_token_xy){
                            book_y += bt + ",";
                            i++;
                            int bt_y = std::stoi(bt) - i;
                            book_x_y += bt + "," + std::to_string(bt_y) + "^~&";
                        }
                        book_y = book_y.substr(0,book_y.size()-1);
                        book_x_y = book_x_y.substr(0,book_x_y.size()-3);
                    }
                }
                bflib.write_binary_file(trained_all_voc,str_topics_without_freq + "^~&" + book_y);
                bflib.write_binary_file(book_x_y_path,book_x_y);
                std::stringstream ss;
                ss << "Book: " << entry.path().filename().string() << " was successfully saved!" << '\n';
                syslog_j.writeLog(output_log_path,ss.str());
            }
            in_file.close();
        }
    }
    syslog_j.writeLog(output_log_path,"All jobs are done!");
}