#include <iostream>
#include <string>
#include <vector>
#include <unordered_map>
#include <regex>
#include "nemslib.h"
#include "libdict.h"
using CallbackFunction = std::function<void(const std::vector<std::string>&, const std::unordered_map<std::string,std::string>&)>;
std::vector<std::string> libdict::get_english_voc_already_checked_in_db(const std::string& english_voc_path){
	Jsonlib json_j;
	nlp_lib nl_j;
	std::vector<std::string> words;
	std::vector<Mdatatype> read_english_voc = nl_j.readBinaryFile(english_voc_path);//"home/ronnieji/lib/db_tools/webUrls/english_voc.bin");
	if(!read_english_voc.empty()){
		for(const auto& rev : read_english_voc){
			words.push_back(rev.word);
		}
	}
	else{
		std::cerr << "Could not find the english_voc.bin file!" << '\n';
	}
	return words;
}
size_t libdict::if_already_checked(const std::vector<std::string>& words_checked, const std::string& word_to_find){
    auto it = std::find(words_checked.begin(), words_checked.end(), word_to_find);
    if (it != words_checked.end()) {
        return 1;
    }
    return 0;
}
void libdict::callback_Function(const std::vector<std::string>& result, const std::unordered_map<std::string,std::string>& word_types,const std::string& english_voc_path, const std::string& log_folder_path){
    SysLogLib sys_j;
	nlp_lib nl_j;
    if(!result.empty() && !english_voc_path.empty() && !log_folder_path.empty()){
        sys_j.writeLog(log_folder_path,"Callback function called with result: ----------------------");
        sys_j.writeLog(log_folder_path,"word: " + result[0]);
        sys_j.writeLog(log_folder_path,"word type: " + result[1]);
        sys_j.writeLog(log_folder_path,"English: " + result[2]);
        sys_j.writeLog(log_folder_path,"Chinese: " + result[3]);
        sys_j.writeLog(log_folder_path,"-------------------------------------------------------------");
        std::string word_type_abv;
        if(!word_types.empty()){
			for(const auto& wt : word_types){
				if(wt.first == result[1]){
					word_type_abv = wt.second;
				}
			}
			if(word_type_abv.empty()){
				sys_j.writeLog(log_folder_path,"Word type: " + result[1] + " did not find in type_table db." );
            	word_type_abv = "NF";
			}
        }
        else{
            sys_j.writeLog(log_folder_path,"Word type: " + result[1] + " did not find in type_table db." );
            word_type_abv = "NF";
        }
		
		Mdatatype mt;
		mt.word = result[0];
		mt.word_type = word_type_abv;
		mt.meaning_en = result[2];
		mt.meaning_zh = result[3];
		/*
			overwrite the word
		*/
		std::vector<Mdatatype> read_english_voc;
		read_english_voc = nl_j.readBinaryFile(english_voc_path);//"home/ronnieji/lib/db_tools/webUrls/english_voc.bin");
		if(!read_english_voc.empty()){
			for(auto it = read_english_voc.begin(); it != read_english_voc.end();){
				if(it->word == result[0]){
					it = read_english_voc.erase(it);
				}
				else{
					++it;
				}
			}
		}
		
		/*
			add the new one
		*/
		read_english_voc.push_back(mt);
		nlp_lib nl_j;
		nl_j.writeBinaryFile(read_english_voc,english_voc_path);//"home/ronnieji/lib/db_tools/webUrls/english_voc.bin");
        sys_j.writeLog(log_folder_path,"Successfully saved the word: " + result[0]);
    }
}
 /*
	para1:input word
	para2:type_table.bin file path
	para3: english_voc file path
	para4:log files folder path
	//"https://dictionary.cambridge.org/dictionary/essential-american-english/" + word
	return 1 = successed
    return 0 = failed
*/
size_t libdict::check_word_onlineDictionary(const std::string& inputStr, const std::unordered_map<std::string,std::string>& wordtypes, const std::string& english_voc_path, const std::string& log_folder_path){
	/*
        if a word was successfully crawled, then return 1
    */
    size_t word_was_found = 0;
    WebSpiderLib weblib_j;
	SysLogLib syslog_j;
    std::string word_type;
    std::string meaning_en;
    std::string meaning_zh;
    std::vector<std::string> pass_value_to_callback(4);
	
	std::string strurl_word_to_lookup = "https://dictionary.cambridge.org/dictionary/essential-american-english/";
	strurl_word_to_lookup.append(inputStr);
    std::string htmlContent = weblib_j.GetURLContent(strurl_word_to_lookup);
    std::this_thread::sleep_for(std::chrono::seconds(5));//seconds 
    if(!htmlContent.empty()){
        word_was_found = 1;
    }else{
        return 0;
    }
	//find all english meaning span
	//"<div class=\"def ddef_d db\">(\\w+)</div>"
	//"<div class=\"def ddef_d db\">(.*?)</div>"
	//R"(<div[^>]*>([^<]*)</div>)"
	//std::vector<std::string> english_meanings =  findAllWordsBehindSpans(htmlContent,R"(<div[^>]*>([^<]*)</div>)");
	std::vector<std::string> english_meanings =  weblib_j.findAllWordsBehindSpans(htmlContent,"<div class=\"def ddef_d db\">(.*?)</div>");
	for(std::string& em : english_meanings){
		//em = findWordBehindSpan(em,"<div class=\"def ddef_d db\">(.*?)</div>");
		//std::cout << "findWordBehindSpan: " << em << std::endl;
		em = weblib_j.G_removehtmltags(em);
        meaning_en += em + "\n";
	}
	meaning_en.append(" ");
	std::string word_front = "<div class=\"tc-bb tb lpb-25 break-cj\" lang=\"zh-Hans\">";
	std::string word_after = "</div>";
	size_t pos_front = htmlContent.find(word_front);
	if (pos_front != std::string::npos) {
		size_t pos_after = htmlContent.find(word_after, pos_front);
		if (pos_after != std::string::npos) {								
			std::string divContent = htmlContent.substr(pos_front + word_front.length(), pos_after - pos_front - word_front.length());
			std::regex tagRegex("<[^>]+>");
			std::string Chinese_result = std::regex_replace(divContent, tagRegex, "");
			Chinese_result = std::regex_replace(Chinese_result, std::regex("&hellip;"), "");
			meaning_zh = Chinese_result + " ";
		}
	}
    // Find all word_type elements
    std::vector<std::string> spans = weblib_j.findAllSpans(htmlContent, "<span class=\"pos dpos\" title=\"[^\"]+\">(\\w+)</span>");
    // Print the found word_type elements
    for (const std::string& span : spans) {
        word_type = weblib_j.findWordBehindSpan(span,"<span class=\"pos dpos\" title=\"[^\"]+\">(\\w+)</span>");
		syslog_j.writeLog(log_folder_path,"The word behind the span: " + word_type);
        //save everything into database
        pass_value_to_callback.resize(4);  // Resize the vector to have a size of 4
        pass_value_to_callback[0] = inputStr;
		word_type = std::string(weblib_j.str_trim(word_type));
		if(!wordtypes.empty()){
			for(const auto& wt : wordtypes){
				if(wt.first == word_type){
					word_type = wt.second;
					break;
				}
			}
		}
        pass_value_to_callback[1] = word_type;
        pass_value_to_callback[2] = std::string(weblib_j.str_trim(meaning_en));
        pass_value_to_callback[3] = std::string(weblib_j.str_trim(meaning_zh));
        //return word_type;
		//const std::vector<std::string>& result, const std::unordered_map<std::string,std::string>& word_types,const std::string& english_voc_path, const std::string& log_folder_path
        this->callback_Function(pass_value_to_callback,wordtypes,english_voc_path,log_folder_path);
		syslog_j.writeLog(log_folder_path,"Successfully got the result form the online dictionary!");
        return 1;
    }
    return 1;
}
/*
	if the word was not found on "https://dictionary.cambridge.org/dictionary/essential-american-english/" + inputStr,
	check the word on "https://www.merriam-webster.com/dictionary/" + strWord;
	para1:input word
	para2:type_table.bin file path
	para3: english_voc file path
	para4:callback function: callback_Function
	para5:log files folder path
	"https://www.merriam-webster.com/dictionary/" + strWord;
	return 1 = successed
    return 0 = failed
*/
size_t libdict::check_word_onMerriamWebsterDictionary(const std::string& inputStr, const std::unordered_map<std::string,std::string>& wordtypes, const std::string& english_voc_path, const std::string& log_folder_path){
	/*
        if a word was successfully crawled, then return 1
    */
    size_t word_was_found = 0;
    WebSpiderLib weblib_j;
	Jsonlib jsl_j;
	SysLogLib syslog_j;
    std::vector<std::string> pass_value_to_callback(4);
    std::string str_url_word_to_lookup = "https://www.merriam-webster.com/dictionary/";
	str_url_word_to_lookup.append(inputStr);
    std::string htmlContent = weblib_j.GetURLContent(str_url_word_to_lookup);
    std::this_thread::sleep_for(std::chrono::seconds(5));//seconds 
    if(!htmlContent.empty()){
        word_was_found = 1;
    }else{
        return 0;
    }
	std::vector<std::string> split_the_whole_page_by_the_word = jsl_j.splitString_bystring(htmlContent,inputStr);
	if(!split_the_whole_page_by_the_word.empty()){
		for(const auto& split_word : split_the_whole_page_by_the_word){
			/*
				fetch the dictionary return from every split html
			*/
			std::vector<std::string> word_types = weblib_j.findAllWordsBehindSpans(split_word,"<a class=\"important-blue-link\" href=\"/dictionary/[^>]*\">(.*?)</a>");
			std::vector<std::string> english_meanings =  weblib_j.findAllWordsBehindSpans(split_word,"<span class=\"dtText\">(.*?)</span>");
			if(!word_types.empty() && !english_meanings.empty()){
				for(const auto& wt : word_types){
					std::string word_type;
					std::string meaning_en;
					std::string meaning_zh;
					pass_value_to_callback.resize(4);  // Resize the vector to have a size of 4
					word_type = std::string(weblib_j.str_trim(wt));
					for(const auto& wt : wordtypes){
						if(wt.first == word_type){
							word_type = wt.second;
							break;
						}
					}
					pass_value_to_callback[0] = inputStr;
					pass_value_to_callback[1] = word_type;
					std::string clean_en_meaning;
					for(auto& em : english_meanings){
						em = weblib_j.G_removehtmltags(em);
						em = std::string(weblib_j.str_trim(em));
						clean_en_meaning += em + '\n';
					}
					pass_value_to_callback[2] = clean_en_meaning;
					pass_value_to_callback[3] = "";
					//return word_type;
					//const std::vector<std::string>& result, const std::unordered_map<std::string,std::string>& word_types,const std::string& english_voc_path, const std::string& log_folder_path
					this->callback_Function(pass_value_to_callback, wordtypes,english_voc_path,log_folder_path);
					syslog_j.writeLog(log_folder_path,"Successfully got the result form the online dictionary!");
					return 1;
				}
			}
		}
	}
	return 0;
}
/*
	para1: input string url: "https://www.merriam-webster.com/dictionary/" + strWord;
*/
std::vector<std::string> libdict::getPastParticiple(const std::string& strurl,const std::string& log_folder_path){
	WebSpiderLib wSpider_j;
	SysLogLib syslog_j;
	std::vector<std::string> get_final_result;
	if(strurl.empty() || log_folder_path.empty()){
		return get_final_result;
	}
	std::string htmlContent = wSpider_j.GetURLContent(strurl);
	std::string queryResult = wSpider_j.findWordBehindSpan(htmlContent,"<span class=\"vg-ins\">(\\w+)</span>");	//(\\w+)(.*?)
	if(!queryResult.empty()){
		syslog_j.writeLog(log_folder_path,queryResult);
		get_final_result = wSpider_j.findAllSpans(queryResult, "<span class=\"if\">(\\w+)</span>");
		return get_final_result;
	}
	return get_final_result;
}
void libdict::look_for_past_participle_of_word(const std::string& english_voc_path,const std::string& log_folder_path){
    SysLogLib sys_j;
	Jsonlib jsonl_j;
	nlp_lib nl_j;
	std::vector<std::vector<std::string>> dbresult;
	size_t td = 2000;
    if(english_voc_path.empty() || log_folder_path.empty()){
        std::cerr << "libdict::look_for_past_participle_of_word input empty!" << '\n';
        return;
    }
	sys_j.writeLog(log_folder_path,"Start looking for words past tenses...");
	std::vector<Mdatatype> read_english_voc = nl_j.readBinaryFile(english_voc_path);//"home/ronnieji/lib/db_tools/webUrls/english_voc.bin");
	if(!read_english_voc.empty()){
		for(const auto& re : read_english_voc){
			/*
				rw[0]=id,
				rw[1]=word,
				rw[2]=word_type,
				rw[3]=meaning_en,
				rw[4]=meaning_zh
				
				check the word's past participle and past perfect participle
			*/
			std::string str_word_type = jsonl_j.trim(re.word_type);
			if(str_word_type=="VB"){
				std::string strWord = std::string(jsonl_j.trim(re.word));
				std::string strUrl_past_participle = "https://www.merriam-webster.com/dictionary/" + strWord;
				/*
					get past participle
				*/
				std::vector<std::string> get_result_past_participle = this->getPastParticiple(strUrl_past_participle,log_folder_path);
				sys_j.sys_timedelay(td);
				if(!get_result_past_participle.empty()){
					for(const auto& gr : get_result_past_participle){
						Mdatatype new_word;
						new_word.word = gr;
						new_word.word_type="VB";
						new_word.meaning_en = re.meaning_en;
						new_word.meaning_zh = re.meaning_zh;
						read_english_voc.push_back(new_word);
					}
					nl_j.writeBinaryFile(read_english_voc,english_voc_path);//"home/ronnieji/lib/db_tools/webUrls/english_voc.bin");
				}
			}
		}
	}
	sys_j.writeLog(log_folder_path,"Done looking for words past tenses...");
}