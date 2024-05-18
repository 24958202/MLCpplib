#include <iostream>
#include <string>
#include <vector>
#include <filesystem>
#include <fstream>
#include <set>
#include <sstream>
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
    std::vector<std::vector<std::string>> dbresult;
	SQLite3Library db(all_voc);
	db.connect();
	db.executeQuery("select * from english_all_voc where w='" + str_to_write + "'",dbresult);
	size_t w_id = -1;
	if(!dbresult.empty()){
		for(const auto& rw : dbresult){
			/*
				rw[0]=id,
				rw[1]=word,
				rw[2]=word_type,
				rw[3]=meaning_en,
				rw[4]=meaning_zh
				
				check the word's past participle and past perfect participle
			*/
            std::cout << "rw: " << rw[1] << '\n';
			w_id = std::stoi(rw[1]);
            std::cout << "w_id: " << w_id << '\n';
		}
        db.disconnect();
        return w_id;
	}
    else{
        /*
            select max id
        */
        std::vector<std::vector<std::string>> dbmax;
        db.executeQuery("select max(w_id) as maxid from english_all_voc",dbmax);
        if(!dbmax.empty()){
            for(const auto& rw : dbmax){
                w_id = std::stoi(rw[0]);
            }
        }
        w_id = w_id + 1;
        std::stringstream ss;
        ss << "insert into english_all_voc(w_id,w)values(" << w_id << ",'" << str_to_write << "')";
        std::string s_q = ss.str();
        db.executeQuery(s_q.c_str(),dbmax);
        db.disconnect();
        return w_id;
    }
    return w_id;
}
void binary_file_lib::write_binary_file(const std::string& file_path, const std::string& str_to_write){
    if(str_to_write.empty()){
        return;
    }
    /*
        check existance str_to_write
    */
    size_t t_id = -1;
    std::vector<std::vector<std::string>> dbresult;
    std::vector<std::vector<std::string>> dbmax;
    Jsonlib jsl_j;
	SQLite3Library db(file_path);
	db.connect();
    db.executeQuery("delete from trained_all_voc where topic='" + str_to_write + "'",dbresult);
	db.executeQuery("select max(topic_id) as maxid from trained_all_voc",dbmax);
	if(!dbmax.empty()){
		for(const auto& rw : dbmax){
			t_id = std::stoi(rw[0]);
		}
        t_id = t_id + 1;
	}
    std::vector<std::string> get_topics = jsl_j.splitString_bystring(str_to_write,"^~&");
    if(!get_topics.empty()){
        std::stringstream ss;
        ss << "insert into trained_all_voc(topic_id,topic,str_book)values(" << t_id << ",'" << get_topics[0] << "','" << get_topics[1] << "')";
        std::string s_query = ss.str();
        db.executeQuery(s_query.c_str(),dbresult);
    }
    db.disconnect();
}
/*
    para1:input_folder_path, para2:stopwordListPath, para3:output_log_path,para4: trained_all_voc.bin path, para5:all_voc.bin path, para6:book_x_y.bin path
*/
void chat_lib::write_books_mysql(const std::string& input_folder_path,const std::string& stopwordListPath,const std::string& output_log_path,const std::string& db_path){
    if(input_folder_path.empty() || stopwordListPath.empty() || output_log_path.empty() || db_path.empty()){
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
                        try{
                            /*
                                check the existance of bt
                            */
                            size_t b_pos = bflib.write_all_voc(db_path,bt);
                            if(b_pos != -1){
                                book_token_xy.push_back(std::to_string(b_pos));
                            }
                        }
                        catch(const std::exception& e){
                            std::cerr << e.what() << '\n';
                        }
                    }
                    book_y = "";
                    if(!book_token_xy.empty()){
                        for(const auto& bt : book_token_xy){
                            book_y += bt + ",";
                        }
                        book_y = book_y.substr(0,book_y.size()-1);
                    }
                }
                bflib.write_binary_file(db_path,str_topics_without_freq + "^~&" + book_y);
                std::vector<std::vector<std::string>> dbresult;
                SQLite3Library db(db_path);
                db.connect();
                db.executeQuery("select topic_id from trained_all_voc where topic='"+ str_topics_without_freq +"'",dbresult);
                if(!dbresult.empty()){
                    size_t t_id = -1;
                    for(const auto& rw : dbresult){
                        std::cout << "rw[0]: " << rw[0] << '\n';
                        t_id = std::stoi(rw[0]);
		            }
                    if(!book_token_xy.empty()){
                        for(unsigned int i=0; i < book_token_xy.size(); ++i){
                            int bt_y = std::stoi(book_token_xy[i]) - i;
                            std::stringstream ss;
                            ss << "insert into CBow(topic_id,dt,dt_y)values(" << t_id << ",'" << book_token_xy[i] << "','" << std::to_string(bt_y) << "')";
                            std::string ss_q = ss.str();
                            db.executeQuery(ss_q.c_str(),dbresult);
                        }
                    }
                }
                db.disconnect();
                std::stringstream ss;
                ss << "Book: " << entry.path().filename().string() << " was successfully saved!" << '\n';
                syslog_j.writeLog(output_log_path,ss.str());
            }
            in_file.close();
        }
    }
    syslog_j.writeLog(output_log_path,"All jobs are done!");
}