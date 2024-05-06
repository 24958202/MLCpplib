#include <iostream>
#include <string>
#include <fstream>
#include "../lib/nemslib.h"
void exportToCSV(const std::vector<Mdatatype>& data, const std::string& filename) {
    std::ofstream file(filename,std::ios::out);
    if(!file.is_open()){
        file.open(filename,std::ios::out);
    }
    else{
        file << "word,word_type,meaning_en,meaning_zh\n";
        for (const auto& entry : data) {
            file << entry.word << "," << entry.word_type << "," << entry.meaning_en << "," << entry.meaning_zh << "\n";
        }
        file.close();
        std::cout << "CSV file exported successfully.\n";
    }
}
int main(){
    nlp_lib nem_j;
    Jsonlib jsl_j;
    std::vector<Mdatatype> getEnglish_voc_clean;
    std::vector<Mdatatype> getEnglish_voc = nem_j.readBinaryFile("/home/ronnieji/lib/bk_0506_2024/english_voc.bin");
    unsigned int i = 0;
    for(auto& item : getEnglish_voc){
        std::string str_en = jsl_j.trim(item.meaning_en);
        std::cout << "index: " << ++i << "   ***  " << item.word << " >>> " << item.word_type << " >>> " << item.meaning_en << " >>> " << item.meaning_zh << '\n';
        if(!str_en.empty()){
            getEnglish_voc_clean.push_back(item);
        }
    }
    if(!getEnglish_voc_clean.empty()){
         exportToCSV(getEnglish_voc_clean,"/home/ronnieji/lib/bk_0506_2024/english_voc.csv");
    }
    return 0;
}