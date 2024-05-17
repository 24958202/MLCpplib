#ifndef CHAT_LIB_H
#define CHAT_LIB_H
#include <string>
enum binary_file_type{
    trained_all_voc,
    all_voc,
    all_voc_x_y
};
class chat_lib{
    public:
        /*
            para1:input_folder_path, para2:stopwordListPath, para3:output_log_path,para4: trained_all_voc.bin path, para5:all_voc.bin path, para6:book_x_y.bin path
        */
        void write_books_mysql(const std::string&,const std::string&,const std::string&,const std::string&, const std::string&, const std::string&);
};
#endif