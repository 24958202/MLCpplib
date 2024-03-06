/*
	g++ /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db_tools/wordnet.cpp -o /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db_tools/wordnet -I/usr/local/include/eigen3/ -I/usr/local/boost/include/ -I/opt/homebrew/Cellar/icu4c/73.2/include/ -L/opt/homebrew/Cellar/icu4c/73.2/lib -licuuc -licudata /usr/local/boost/lib/libboost_system.a /usr/local/boost/lib/libboost_filesystem.a -I/Users/dengfengji/miniconda3/pkgs/sqlite-3.41.2-h80987f9_0 -lsqlite3 -std=c++20
	/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/db_tools/wordnet /Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/dict_d
*/

#include <iostream>
#include <fstream>
#include <algorithm>
#include <boost/algorithm/string.hpp>
#include <boost/algorithm/string/split.hpp>
#include <boost/algorithm/string/classification.hpp>
#include <sqlite3.h>
#include <filesystem>
#include <thread>
#include <chrono>
#include <unicode/unistr.h>  // ICU
#include <unicode/uchar.h>
#include <iomanip>
#include <cctype>
#include <atomic>
#include <regex>
#include <string>
#include <string_view>
#include <sstream>



std::vector<std::string> totalVoc;

/*
	Begin SQLite3Library
*/
class SQLite3Library {
public:
    SQLite3Library(const std::string& db_file) : db_file(db_file), connection(nullptr) {}
    void connect();
    void disconnect();
    template<typename T>
    void executeQuery(const std::string&, T&);
    template<typename T>
	static int callback(void*, int, char**, char**);
private:
    sqlite3* connection;
    std::string db_file;
};
/*-start Sqlite3 Library-*/
void SQLite3Library::connect() {
        int result = sqlite3_open(db_file.c_str(), &connection);
        if (result != SQLITE_OK) {
            std::cerr << "Error opening database: " << sqlite3_errmsg(connection) << std::endl;
        }
}
void SQLite3Library::disconnect(){
    sqlite3_close(connection);
}
template<typename T>
void SQLite3Library::executeQuery(const std::string& query, T& returnValue) {
    char* errorMessage = nullptr;
    int result = sqlite3_exec(connection, query.c_str(), SQLite3Library::callback<T>, &returnValue, &errorMessage);
    if (result != SQLITE_OK) {
        std::cerr << "Error executing query: " << errorMessage << std::endl;
        sqlite3_free(errorMessage);
    }
}
// Explicit template instantiation for vector<vector<string>>
template void SQLite3Library::executeQuery(std::string const&, std::vector<std::vector<std::string>>&);

template<typename T>
int SQLite3Library::callback(void* data, int argc, char** argv, char** columnNames) {
    T* returnValue = static_cast<T*>(data);
    // Create a new row to store the values
    std::vector<std::string> row;
    // Iterate over the columns and store the values in the row
    for (int i = 0; i < argc; i++) {
        if (argv[i]) {
            row.push_back(argv[i]);
        } else {
            row.push_back("NULL");
        }
    }
    // Add the row to the result vector
    returnValue->push_back(row);
    return 0;
}
/*-End Sqlite3 Library-*/

void write_to_file(const std::string& f_path, const std::vector<std::string>& f_content){
	if(f_path.empty()){
		std::cout << "Please input the file path!" << std::endl;
		return;
	}
	std::ofstream ofile(f_path, std::ios::app);
	if(!ofile.is_open()){
		ofile.open(f_path);
	}
	for(const auto& ct : f_content){
		ofile << ct << "\n";
	}
	ofile.close();
}
bool containsOnlyOneWord(const std::string& str) {
    if(str.find(' ') != std::string::npos){
        return false;//has space,there are more than one words
    else{
        return true;
    }
}

std::vector<std::string> remove_repeated_items_vs(const std::vector<std::string>& vec){
    std::vector<std::string> const_vec(vec.begin(), vec.end());
    try{
        // Create a non-const copy of the vect
        // Sort the vector
        std::sort(const_vec.begin(), const_vec.end());
        // Use std::unique to remove adjacent duplicates
        auto it = std::unique(const_vec.begin(), const_vec.end());
        // Erase the elements after the unique ones
        const_vec.erase(it, const_vec.end());
    }
    catch(const std::exception& e){
        std::cerr << "Function:remove_repeated_items_vsError(line:339): " << e.what() << std::endl;
    }
    return const_vec;
}
/*
	split the line if there are two words and put the word into a same collection
*/
std::vector<std::string> get_adj(std::string& input_string){
	std::vector<std::string> str_temp;
	if(input_string.empty()){
		return str_temp;
	}
	if(containsOnlyOneWord(input_string)){
    	str_temp.push_back(input_string);
    	return str_temp;
    }
	boost::algorithm::split(str_temp, input_string, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
	std::this_thread::sleep_for(std::chrono::milliseconds(1));//milliseconds
	for(auto& st : str_temp){
		boost::algorithm::trim(st);
	}
	return str_temp;
}
/*
	remove punctuations, numbers, spaces = the word
*/
std::vector<std::string> get_cnlist(std::string& input_string){
	std::vector<std::string> str_temp;
	if(input_string.empty()){
		return str_temp;
	}
	/*
		remove punctuations
	*/
	input_string.erase(std::remove_if(input_string.begin(), input_string.end(),
                                [](char c) { return std::ispunct(static_cast<unsigned char>(c)); }), input_string.end());
    /*
    	remove numbers
    */
    input_string.erase(std::remove_if(input_string.begin(), input_string.end(),
                                [](char c) { return std::isdigit(static_cast<unsigned char>(c)); }), input_string.end());
    /*
    	remove spaces
    */
    //boost::algorithm::trim(input_string);
    if(containsOnlyOneWord(input_string)){
    	boost::algorithm::trim(input_string);
    	str_temp.push_back(input_string);
    	return str_temp;
    }
    boost::algorithm::split(str_temp, input_string, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
	for(auto& st : str_temp){
		boost::algorithm::trim(st);
	}
	return str_temp;
}
std::vector<std::string> get_cnlist2(std::string& input_string){
	std::vector<std::string> str_temp;
	if(input_string.empty()){
		return str_temp;
	}
	for(auto& c : input_string){
		if(std::ispunct(static_cast<unsigned char>(c)) || std::isdigit(static_cast<unsigned char>(c))){
			c = ' ';
		}
	}
	if(containsOnlyOneWord(input_string)){
    	boost::algorithm::trim(input_string);
    	str_temp.push_back(input_string);
    	return str_temp;
    }
    boost::algorithm::split(str_temp, input_string, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
	for(auto& st : str_temp){
		boost::algorithm::trim(st);
	}
	return str_temp;
}

void get_o_version_to_txt(const std::string& folder_name){
	std::cout << "Start processing..." << "\n";
	for(const auto& entry: std::filesystem::directory_iterator(folder_name)){
		if(entry.is_regular_file() && entry.path().extension() == ".txt"){
			std::ifstream file(entry.path());
			if(file.is_open()){
				std::string line;
				while (std::getline(file,line)){
					boost::algorithm::trim(line);
					std::string f_name = entry.path().filename();
					if(f_name == "adj.txt"){
						std::vector<std::string> re_temp = get_adj(line);
						if(!re_temp.empty()){
							totalVoc.insert(totalVoc.end(),re_temp.begin(),re_temp.end());
						}
					}
					else if(f_name == "cntlist.txt"){
						std::vector<std::string> str_temp = get_cnlist2(line);
						if(!str_temp.empty()){
							totalVoc.insert(totalVoc.end(),str_temp.begin(),str_temp.end());
						}
					}
					else if(f_name == "index.txt"){
						std::vector<std::string> str_temp_index = get_cnlist2(line);
						if(!str_temp_index.empty()){
							totalVoc.insert(totalVoc.end(),str_temp_index.begin(),str_temp_index.end());
						}
					}
					else if(f_name == "noun.txt"){
						std::vector<std::string> re_temp_n = get_adj(line);
						if(!re_temp_n.empty()){
							totalVoc.insert(totalVoc.end(),re_temp_n.begin(),re_temp_n.end());
						}
					}
					else if(f_name == "sentidx.txt"){
						std::vector<std::string> str_temp_sentidx = get_cnlist2(line);
						if(!str_temp_sentidx.empty()){
							totalVoc.insert(totalVoc.end(),str_temp_sentidx.begin(),str_temp_sentidx.end());
						}
					}
				}//while
			}//if(file.is_open())
		}//if
	}//for
	/*
		remove repeated items in totalVoc
	*/
	//remove empty items
	for(auto& tv : totalVoc){
		if(tv.empty()){
			continue;
		}
		std::vector<std::string> v_temp;
		boost::algorithm::split(v_temp, tv, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
		/*
			remove current item
		*/
		
		if(!v_temp.empty()){
			totalVoc.insert(totalVoc.end(),v_temp.begin(),v_temp.end());
			tv = v_temp[0];
		}
	}
	totalVoc.erase(std::remove(totalVoc.begin(), totalVoc.end(), ""), totalVoc.end());
	totalVoc = remove_repeated_items_vs(totalVoc);
	/*
		write into a txt file
	*/
	write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/totalVoc.txt",totalVoc);
	std::cout << "All jobs are done!" << "\n";
}
//------------------------------- from dict txt to db ------------------------------------------------
void save_to_db(const std::string& str_word, const std::string& str_wordtype, const std::string& str_content){
	if(str_word.empty()){
		std::cout << "save_to_db" << str_word << "\n";
		return;
	}
	std::vector<std::vector<std::string>> dbresult;
	std::vector<std::vector<std::string>> dbre;
	SQLite3Library db("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/rules/catalogdb.db");
	db.connect();
	db.executeQuery("select word,meaning_en,meaning_zh from english_voc where word='" + str_word + "'",dbresult);
	if(dbresult.empty()){//the word does not exist
		db.executeQuery("insert into english_voc(word,word_type,meaning_en)values('" + str_word + "','" + str_wordtype + "','" + str_content + "')",dbre);
	}
	db.disconnect();
	std::cout << "Successfully saved " << str_word << std::endl;
}
void check_non_con_for_vb(const std::string& str_file){
	if(str_file.empty()){
		std::cout << "the vb list is empty!" << "\n";
		return;
	}
	std::ifstream ifile(str_file);
	if(ifile.is_open()){
		std::string line;
		while (std::getline(ifile,line)){
			boost::algorithm::trim(line);
			std::vector<std::string> vb_phrase;
			std::string vb_word_to_loop_up;
			auto it = std::ranges::find(line, ' ');
			if(it != line.end()){
				boost::algorithm::split(vb_phrase, line, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
				vb_word_to_loop_up = vb_phrase[0];
			}
			else{//single word
				vb_word_to_loop_up = line;
			}
		}
	}
	ifile.close();
}
void get_data_with_type(std::string& str_line, const std::string& f_name){
	if(str_line.empty()){
		std::cout << "str_line is empty!" << "\n";
		return;
	}
	std::vector<std::string> d_wordtype;//store the word at positon 17
	if(f_name.find("index") != std::string::npos){
		std::vector<std::string> res;
		boost::algorithm::split(res, str_line, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
		std::string str_i_adj = res[0];
		boost::algorithm::trim(str_i_adj);
		std::cout << str_i_adj << std::endl;
		if(str_i_adj.find('_') != std::string::npos){
			boost::algorithm::replace_all(str_i_adj, "_", " ");
		}
		d_wordtype.push_back(str_i_adj);
	}
	else{
		str_line = str_line.substr(17, str_line.size());
		const char* c_str_line = str_line.c_str();
		using split_iter_t = boost::split_iterator<const char*>;
		split_iter_t sentences = boost::make_split_iterator(c_str_line, boost::algorithm::token_finder(boost::is_any_of("|")));
		for(unsigned int i=1; !sentences.eof(); ++sentences, ++i) {
			auto range = *sentences;    
			if(i==1){
				std::vector<std::string> result;
				boost::algorithm::split(result, range, boost::algorithm::is_space(), boost::algorithm::token_compress_off);
				std::string str_word = result[0];
				boost::algorithm::trim(str_word);
				if(str_word.find('_') != std::string::npos){
					boost::algorithm::replace_all(str_word, "_", " ");
				}
				d_wordtype.push_back(str_word);//put the word at positon 17 into a collection
				std::cout << str_word << std::endl;
				std::stringstream ss;
				ss << *std::next(sentences,1);
				std::string meaning_en = ss.str();
				if(f_name == "data_adj.txt"){
					save_to_db(str_word,"AJ",meaning_en);
				}
				else if(f_name == "data_adv.txt"){
					save_to_db(str_word,"AV",meaning_en);
				}
				else if(f_name == "data_noun.txt"){
					save_to_db(str_word,"NN",meaning_en);
				}
				else if(f_name == "data_verb.txt"){
					save_to_db(str_word,"VB",meaning_en);
				}
				/*
					save the word and the meaning into db
				*/
			}//if
			//std::cout << range << "\n";
		}//for
 	}
    /*
    	write the collection into the related file.
    */
    if(f_name == "data_adj.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_data_adj.txt",d_wordtype);
	}
	else if(f_name == "data_adv.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_data_adv.txt",d_wordtype);
	}
	else if(f_name == "data_noun.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_data_noun.txt",d_wordtype);
	}
	else if(f_name == "data_verb.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_data_verb.txt",d_wordtype);
	}
	else if(f_name == "index_adj.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_index_adj.txt",d_wordtype);
	}
	else if(f_name == "index_adv.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_index_adv.txt",d_wordtype);
	}
	else if(f_name == "index_noun.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_index_noun.txt",d_wordtype);
	}
	else if(f_name == "index_sense.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_index_sense.txt",d_wordtype);
	}
	else if(f_name == "index_verb.txt"){
		write_to_file("/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/wordnet/word_index_verb.txt",d_wordtype);
	}
	// str_line.erase(std::remove_if(str_line.begin(), str_line.end(),
//                                 [](char c) { return std::ispunct(static_cast<unsigned char>(c)); }),str_line.end());
// 
 	
}
//----------------------------------------------------------------------------------------------------
void get_dict_d_to_db(const std::string& str_folder){
	std::cout << "Start processing..." << "\n";
	for(const auto& entry: std::filesystem::directory_iterator(str_folder)){
		if(entry.is_regular_file() && entry.path().extension() == ".txt"){
			std::ifstream file(entry.path());
			if(file.is_open()){
				std::string line;
				std::string f_name = entry.path().filename();
				while (std::getline(file,line)){
					boost::algorithm::trim(line);
					if(f_name != "index_sense.txt"){
						get_data_with_type(line, f_name);
					}
				}//while
			}//if(file.is_open())
		}//if
	}//for
}
int main(int argc, char* argv[]){
	if(argc != 2){
		return 1;
	}
	std::string folder_name = argv[1];
	if(folder_name.empty()){
		std::cout << "Please select a folder..." << "\n";
		return 1;
	}
	/*
		get the words from dict folder and save them into totalVoc.txt file.
	*/
	//get_o_version_to_txt(folder_name);
	/*
		get the words and meaning from dict_d folder and save them into db
	*/
	get_dict_d_to_db(folder_name);
	return 0;
}
