#ifndef NEMSLIB_H
#define NEMSLIB_H

#include <iostream>
#include <string>
#include <vector>
#include <cstring>
#include <unordered_set>
#include <map>
#include <unordered_map>
#include <sqlite3.h>
#include <string_view>
#include <curl/curl.h>
#include <gumbo.h>
#include <chrono>
#include <thread>
#include <optional>
#include <functional>

/*
    CallbackExp cbe_j;
    std::unordered_map<std::string, std::vector<std::string>> stateMap = {{"STATE_1", {"2 > 1","2 + 2"}}, {"STATE_2", {"1 < 3","7-3"}}, {"STATE_3", {"5 + 1 = 6","3-2"}}};
    cbe_j.initializeCallbacks(stateMap);

    cbe_j.processState("STATE_1",stateMap["STATE_1"]);
    output: "2 > 1","2 + 2"
*/
class CallbackExp{
    public:
        typedef std::function<void(const std::vector<std::string>&)> CallbackFunction;
        std::unordered_map<std::string, CallbackFunction> callbacks;
        void callbackFunction(const std::vector<std::string>&);
        void initializeCallbacks(std::unordered_map<std::string, std::vector<std::string>>&);
        void processState(const std::string&, const std::vector<std::string>&);
};
class Jsonlib{
    public:
        std::string trim(const std::string&);
		/*
			std::string_view extractString
			para1 : original strings; para2: start positon string, like "[BOT]"; para3 end position string ,like "[EOT]";
		*/
        std::string_view extractString(std::string_view, const std::string&, const std::string&);//get the text between [BOT]-[EOT]
        std::vector<std::string> splitString(const std::string&, char);//split a string by a delimiter
        std::vector<std::string> splitString_bystring(const std::string&, const std::string&);//split a string by a string
		bool isDomainExtension(const std::string&);
		std::vector<std::string> split_sentences(const std::string&);
        void toLower(std::string&);
		void toUpper(std::string&);
        void remove_numbers(std::string&);
        std::string str_replace(std::string&, std::string&, const std::string&);
        std::vector<std::string> read_single_col_list(const std::string&);
        void removeDuplicates(std::vector<std::string>&);
        /*
            remove last char in a string
        */
        std::string remove_last_char_in_a_string(const std::string&);
};
/*-Begin SQLite3 library*/
class SQLite3Library {
public:
    SQLite3Library(const std::string&);
    void connect();
    void disconnect();
    template<typename T>
    void executeQuery(const std::string&, T&);
    template<typename T>
	int callback(void*, int, char**, char**);
private:
    sqlite3* connection;
    std::string db_file;
};
/*-End SQLite3 library*/
/*
    start nemslib
    You have to initialize stopword list before using some features in the library
    nemslib nems_j;
    nems_j.set_stop_word_file_path("/stopword file path");

*/
class nemslib{
    private:
        std::string stop_word_list_path;
        std::unordered_map<std::string,std::string> nlp_exps;
    public:
        void set_stop_word_file_path(const std::string&);
        std::string get_current_dir();
        bool isVector(const std::string&);
        bool isNonAlphabetic(const std::string&);
		bool isNumeric(const std::string&);
        int en_string_word_count(const std::string&);
        std::string remove_chars_from_str(const std::string&, const std::vector<std::string>&);
        std::string removeEnglishPunctuation_training(const std::string&);
        std::string removeChinesePunctuation_training(const std::string&);
        std::vector<std::string> tokenize_en(const std::string&);
        std::vector<std::string> tokenize_zh(const std::string&);//tokenize a string and extract Chinese characters as separate tokens.
        std::vector<std::string> tokenize_zh_unicode(const std::string&);//tokenize a string and extract individual Unicode characters as separate tokens.
		bool is_stopword_en(const std::string&);//check if a word is a stopword
		std::vector<std::pair<std::string, int>> calculateTermFrequency(const std::string&);
		/*
			Before you using get_topics, you need to initialize stopwords list by calling set_stop_word_file_path(const std::string& stopword_file_path);
			nemslib nems_j;
			nems_j.set_stop_word_file_path("/home/ronnieji/lib/lib/res/english_stopwords");
			nems_j.get_topics(strInput);
		*/
        std::string get_topics(const std::string&);
		std::vector<std::unordered_map<std::string,unsigned int>> get_topics_freq(const std::string&);
        std::vector<std::string> readTextFile(const std::string&);
		/*
			read_rawtxts(const std::string&,const std::string&):
			para1:input_file_path;
			para2:english_stopwords file path
			para3:sqlite3 db file path
		*/
        std::string read_rawtxts(const std::string&,const std::string&,const std::string&);
        void savedb(const std::string&,const std::string&, const std::string&);
		/*
			read_predicttxts
			para1: input topic list
			para2: candidate string
			para3: matching rate
		*/
		int predict_sorted_Text_match_the_topic(const std::vector<std::string>&, const std::string&, float&);
        std::string read_predicttxts(const std::string&,const std::string&,float&);
        std::string preprocess_words(const std::string&);
        /*
            remove words punctuation,tokenize the string
        */
        std::string preprocess_words_training(const std::string&);
        /*
            remove stopwords and get the topic of the text
        */
        std::string remove_en_stopwords_from_string_for_gpt(const std::string&);
		/*
			para1: stopword path; para2:catalogdb file path;
		*/
        std::unordered_map<std::string, std::string> ini_basic_exp(const std::vector<std::string>&, const std::string&);
        /*
            nemslib nl_j;
            std::vector<std::string> test = nl_j.get_language_exp(SQLITE3 db file path);
            for(const auto& te : test){
                std::cout << te << std::endl;
            }
        */
        std::vector<std::string> get_language_exp(const std::string&);
        /*
            remove 's in the string
        */
        std::string remove_s_in_string(const std::string&);
        /*
            default matching_rate: 0.5
			if_topic_match_the_content para1 ususally it's from the loop to check if the para1 matches para2 with para3 matching rate
        */
        struct return_topic_match{
            bool topic_match_content;
            float matching_percentage;
        };
        return_topic_match if_topic_match_the_content(const std::string&, const std::string&, float&);
        std::vector<std::string> read_stopword_list();
        void process_user_input(const std::unordered_map<std::string, std::string>&, const std::string&, const std::vector<std::string>&);
		/*
			You need to input a sqlite3 database path as the first parameter, and the query string as the second parameter
			select query only
		*/
        std::vector<std::vector<std::string>> check_sql_return_result(const std::string&,const std::string&);
        void process_user_multiple_input(const std::unordered_map<std::string, std::string>&, const std::string&, const std::vector<std::string>&);
        std::string get_random_answer_if_exists(const std::string&);
};
/*
    start SysLogLib

    SysLogLib syslog_j;
    syslog_j.writeLog("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/log","this is a test log without");
*/
class SysLogLib{
    /*
        get current date and time
    */
    private:
        struct CurrentDateTime{
            std::string current_date;
            std::string current_time;
        };
        CurrentDateTime getCurrentDateTime();
    /*
        write system log
        example path: "/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/log/"
    */
    public:
        void sys_timedelay(size_t&);//3000 = 3 seconds
        void writeLog(const std::string&, const std::string&);
        void chatCout(const std::string&);

};
/*
    end SysLogLib
*/
/*
    start WebSpiderLib
    WebSpiderLib webspider_j;
    //std::string webContent = webspider_j.GetURLContentAndSave("https://dictionary.cambridge.org/dictionary/essential-american-english/cactus","<div class=\"def ddef_d db\">(.*?)</div>");
    std::string webContent = webspider_j.GetURLContentAndSave("https://cn.bing.com/search?q=face","<i data-bm=\"77\">(.*?)</i>");
    syslog_j.chatCout(webContent);

*/
class WebSpiderLib{
    public:
        static size_t WriteCallback(void*, size_t, size_t, std::string*);
        std::string_view str_trim(std::string_view);
        /*
            parase html page
        */
        std::string removeHtmlTags(const std::string&);
        std::vector<std::string> findAllWordsBehindSpans(const std::string&, const std::string&);
        std::vector<std::string> findAllSpans(const std::string&, const std::string&);
        std::string findWordBehindSpan(const std::string&, const std::string&);
        std::string GetURLContent(const std::string&);
        std::string GetURLContentAndSave(const std::string&, const std::string&);
        /*
            const std::string HTML_DOC = readTextFile_en("/Volumes/WorkDisk/MacBk/pytest/ML_pdf/bow_test/htmlTags_to_removed.txt");
            WebSpiderLib websl_j;
            GumboOutput* output = gumbo_parse(HTML_DOC.c_str());
            std::cout << websl_j.removeHtmlTags_google(output->root) << std::endl;
            gumbo_destroy_output(&kGumboDefaultOptions, output);

            Or:
            const std::string HTML_DOC = readTextFile_en("/Volumes/WorkDisk/MacBk/pytest/ML_pdf/bow_test/htmlTags_to_removed.txt");
            WebSpiderLib websl_j;
            std::string strOutput = G_removehtmltags(HTML_DOC);
            extractDomainFromUrl can get the www.domain.com from url
        */
        std::string removeHtmlTags_google(GumboNode* node);
        std::string G_removehtmltags(std::string&);
        std::string extractDomainFromUrl(const std::string&);
};
/*
    to record how much time a procedure spent
    ProcessTimeSpent process;
    // Start the process
    auto startTime = process.time_start();

    // End the process
    auto endTime = process.time_end();

    // Print the time spent
    std::cout << "Time spent: " << (endTime - startTime).count() << " milliseconds" << std::endl;
*/
class ProcessTimeSpent{
    public:
        std::chrono::milliseconds time_start();
        std::chrono::milliseconds time_end();
};
/*
    nlp_lib--start
*/
struct node_data{
    std::string node_type;
    std::string topics;
    std::string n_value;
    std::string n_left;
    std::string n_right;
    std::string status;
};
struct Mdatatype {
    std::string word;
    std::string word_type;
    std::string meaning_en;
    std::string meaning_zh;
};
struct exp_data2{
    std::string exp_key;
    std::string exp_value;
};
class nlp_lib{
    private:
 	std::unordered_set<std::string> ProcessFileNames;
    public:
        /*
            language pre-processing tools
        */
	/*
            std::vector<std::string> vocA, std::vector<std::string> vocB;
            Remove vocA from vocB
            para1: vocA
            para2: vocB
        */
        void remove_vocA_from_vocB(std::vector<std::string>&, std::vector<std::string>&);
        void writeBinaryFile(const std::vector<Mdatatype>&, const std::string&);// write the binaryfile
        std::optional<Mdatatype> readValueFromBinaryFile(const std::string&, const std::string&);//read the binary file-(key-value)
	/*
			get_english_voc_and_create_bin : input parameter sqlite3 db file path
            binary file creater can use: exp_input.cpp
        */
        void get_english_voc_and_create_bin(const std::string&);//put the db data into binary file
        std::vector<Mdatatype> readBinaryFile(const std::string&);
        void writeBinaryFileForNLP(const std::string&);
        std::unordered_map<std::string,std::string> get_binary_file_for_nlp(const std::string&);
        /*
            write/read single column binary data to file
        */
        void WriteBinaryOne_from_txt(const std::string&);
        /*
            write a dataset to a binary file; para1: dataset, para2: output file path /*.bin
        */
        void WriteBinaryOne_from_std(const std::vector<std::string>&, const std::string&);
        /*
            read the binary file and put the result into a dataset
        */
        std::vector<std::string> ReadBinaryOne(const std::string&);
        /*
            append a record to binary file(One column)\
            para1: input the binary file path + file name
            para2: string to append
        */
        void AppendBinaryOne(const std::string&,const std::string&);
        void AppendBinaryOne_allow_repeated_items(const std::string&,const std::string&);
        /*
            remove repeated items in std::vector<std::string>
        */
        std::vector<std::string> remove_repeated_items_vs(const std::vector<std::string>&);
        /*
            count the frequency of a string in a string
            para1: main string 
            para2: query string (string to search in main string)
        */
        unsigned int get_number_of_substr_in_str(const std::string&, const std::string&);
        /*
            get numbers, prices etc.in a string
        */
        std::vector<std::string> get_numbers(const std::string&);
        /*
            read an article and save into a binary file
            para1: input folder path
            para2: output file path : books.bin
            para3: binaryOne path
            para4: stopword list Path
            para5: log file folder path (to get working status of the function)
        */
        void write_books(const std::string&,const std::string&,const std::string&,const std::string&,const std::string&);
        /*
            read the books.bin file,para1: the books.bin file path
        */
        std::vector<std::string> read_books(const std::string&);
};
/*
    nlp_lib--end
*/
#endif
