#ifndef LIBDICT_H
#define LIBDICT_H
#include <vector>
#include <string>
#include <unordered_map>
#include <functional>
class libdict{
    public:
        /*
            put all the word in english_voc into a dataset
            para1: input english_voc.bin file path;
        */
        std::vector<std::string> get_english_voc_already_checked_in_db(const std::string&);
        /*
            para1:english_voc words,
            para2: the word to look up
        */
        size_t if_already_checked(const std::vector<std::string>&, const std::string&);
        /*
            callback function
        */
        using CallbackFunction = std::function<void(const std::vector<std::string>&, const std::unordered_map<std::string,std::string>&,const std::string&,const std::string&)>;
        /*
            callback_Function for get_word_type function
            para1: the result from url return
            para2: type_table.bin file path
            para3: english_voc file path
            para4: log file folder path
            
            result[0]: word
            result[1]: word type
            result[2]: english meaning
            result[3]: simplified chinese meaning
        */
        void callback_Function(const std::vector<std::string>&, const std::unordered_map<std::string,std::string>&,const std::string&, const std::string&);
        /*
            para1:input word
            para2:type_table.bin file path
            para3: english_voc file path
            para4:log files folder path
            //"https://dictionary.cambridge.org/dictionary/essential-american-english/" + word
            return 1 = successed
            return 0 = failed
        */
        size_t check_word_onlineDictionary(const std::string&, const std::unordered_map<std::string,std::string>&, const std::string&, const std::string&);
        /*
            check the word in merriam-webster.com/dictionary if the word was not found in dictionary.cambridge.org
        */
        size_t check_word_onMerriamWebsterDictionary(const std::string&, const std::unordered_map<std::string,std::string>&, const std::string&, const std::string&);
        /*
            para1: input string url: "https://www.merriam-webster.com/dictionary/" + strWord;
            para2:log files folder path 
        */
        std::vector<std::string> getPastParticiple(const std::string&,const std::string&);
        /*
            use the following function right after finding a new word from cam dictionary online.
            para1:input english_voc.bin file path
            para2:log files folder path
        */
        void look_for_past_participle_of_word(const std::string&, const std::string&);

};
#endif