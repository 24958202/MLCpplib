/*
    
*/
#include <iostream>
#include <string>
#include <vector>
#include <fstream>
#include <filesystem>
#include <sstream>
#include <cstdint>  // For uint32_t
#include <stdexcept>
#include <algorithm>
#include <ranges>
#include <thread>
#include <chrono>
#include <ctime>
#include <regex>
#include <cctype>//std::isdigit()
#include <unistd.h>
#include <unicode/unistr.h>  // ICU
#include <unicode/uchar.h>
#include <unordered_set> // ICU
#include <atomic>//guarantee to be executed as a single ,indivisible operation without interruption.
#include <gumbo.h>
#include <map>
#include <unordered_map>
#include <locale>
#include <codecvt>
#include <utility>
#include <curl/curl.h>

#if defined(_WIN32) || defined(_WIN64)
    #include <windows.h>
#elif defined(__linux__)
    #include <unistd.h>
    #include <limits.h> // For PATH_MAX
#elif defined(__APPLE__)
    #include <mach-o/dyld.h> // For _NSGetExecutablePath
    #include <limits.h>      // For PATH_MAX
#endif
class nemslib_webcam{
public:
    std::string getExecutablePath(){
    #if defined(_WIN32) || defined(_WIN64)
            // On Windows: Use GetModuleFileName
            char buffer[MAX_PATH];
            DWORD length = GetModuleFileNameA(NULL, buffer, sizeof(buffer));
            if (length == 0 || length == sizeof(buffer)) {
                throw std::runtime_error("Failed to retrieve executable path on Windows!");
            }
            return std::string(buffer);
        #elif defined(__linux__)
            // On Linux: Use readlink to get the executable path
            char buffer[PATH_MAX];
            size_t count = readlink("/proc/self/exe", buffer, sizeof(buffer) - 1);
            if (count == -1) {
                throw std::runtime_error("Failed to retrieve executable path on Linux!");
            }
            buffer[count] = '\0'; // Null-terminate the result
            return std::string(buffer);
        #elif defined(__APPLE__)
            // On macOS: Use _NSGetExecutablePath to get path
            uint32_t size = PATH_MAX;
            char buffer[PATH_MAX];
            // Dynamically resize the buffer if PATH_MAX is too small
            int result = _NSGetExecutablePath(buffer, &size);
            if (result == -1) {
                // If buffer is too small, dynamically allocate the correct size
                char* dynamic_buffer = new char[size];
                if (_NSGetExecutablePath(dynamic_buffer, &size) != 0) {
                    delete[] dynamic_buffer;
                    throw std::runtime_error("Failed to retrieve executable path on macOS!");
                }
                std::string executablePath(dynamic_buffer);
                delete[] dynamic_buffer;
                return executablePath;
            }
            return std::string(buffer);
        #else
            throw std::runtime_error("Unsupported platform!");
        #endif
    }
    std::string getExecutableDirectory(){
        return std::filesystem::path(getExecutablePath()).parent_path().string();
    }
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
    /*
        write system log
        example path: "/Volumes/WorkDisk/MacBk/pytest/NLP_test/src/log/"
    */
    public:
        struct CurrentDateTime{
            std::string current_date;
            std::string current_time;
        };
        CurrentDateTime getCurrentDateTime();
        std::string get_current_date_as_string();
        void sys_timedelay(size_t&);//3000 = 3 seconds
        void writeLog(const std::string&, const std::string&);
        void chatCout(const std::string&);

};
/*
    start SysLogLib
*/
/*
    get current date and time
*/
SysLogLib::CurrentDateTime SysLogLib::getCurrentDateTime() {
    std::chrono::system_clock::time_point current_time = std::chrono::system_clock::now();
    std::time_t current_time_t = std::chrono::system_clock::to_time_t(current_time);
    std::tm* current_time_tm = std::localtime(&current_time_t);
    SysLogLib::CurrentDateTime currentDateTime;
    currentDateTime.current_date = std::to_string(current_time_tm->tm_year + 1900) + "-" + std::to_string(current_time_tm->tm_mon + 1) + "-" + std::to_string(current_time_tm->tm_mday);
    currentDateTime.current_time = std::to_string(current_time_tm->tm_hour) + ":" + std::to_string(current_time_tm->tm_min) + ":" + std::to_string(current_time_tm->tm_sec);
    return currentDateTime;
}
/*
    write system log
*/
std::string SysLogLib::get_current_date_as_string(){
    SysLogLib::CurrentDateTime str_date = getCurrentDateTime();
    return str_date.current_date;
}
void SysLogLib::sys_timedelay(size_t& mini_sec){
    std::this_thread::sleep_for(std::chrono::milliseconds(mini_sec));
}
void SysLogLib::writeLog(const std::string& logpath, const std::string& log_message) {
    if(logpath.empty() || log_message.empty()){
        std::cerr << "SysLogLib::writeLog input empty!" << '\n'; 
        return;
    }
    std::string strLog = logpath;
    if(strLog.back() != '/'){
        strLog.append("/");
    }
    if (!std::filesystem::exists(strLog)) {
        try {
            std::filesystem::create_directory(strLog);
        } catch (const std::exception& e) {
            std::cout << "Error creating log folder: " << e.what() << std::endl;
            return;
        }
    }
    SysLogLib::CurrentDateTime currentDateTime = getCurrentDateTime();
    strLog.append(currentDateTime.current_date);
    strLog.append(".txt");
    std::ofstream file(strLog, std::ios::app);
    if (!file.is_open()) {
        file.open(strLog, std::ios::app);
    }
    file << currentDateTime.current_time + " : " + log_message << std::endl;
    file.close();
    //std::cout << currentDateTime.current_date << " " << currentDateTime.current_time << " : " << log_message << std::endl;
}
void SysLogLib::chatCout(const std::string& chatTxt){
    size_t t_delay = 23;
    for (char c : chatTxt) {
        std::cout << c << std::flush;
        SysLogLib::sys_timedelay(t_delay);
    }
    std::cout << std::endl;
}
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
        static size_t WriteCallback(void*, size_t, size_t, void*);//std::string*
        std::vector<std::string> tokenize_en(const std::string&);
        std::string_view str_trim(std::string_view);
        std::string str_replace(std::string&, std::string&, const std::string&);
        std::vector<std::string> splitString_bystring(const std::string&, const std::string&);
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
    start WebSpiderLib
*/
size_t WebSpiderLib::WriteCallback(void* contents, size_t size, size_t nmemb, void* output) {  
    ((std::string*)output)->append((char*)contents, size * nmemb);  
    return size * nmemb;  
}  
std::vector<std::string> WebSpiderLib::tokenize_en(const std::string& str_line) {
    std::vector<std::string> result;
    if(str_line.empty()){
        std::cerr << "nemslib::tokenize_en input empty!" << '\n';
        return result;
    }
    std::stringstream ss(str_line);
    for(const auto& token : std::ranges::istream_view<std::string>(ss)){
        result.push_back(token);
    }
    return result;
}
std::string_view WebSpiderLib::str_trim(std::string_view str){
    // Find the first non-whitespace character from the beginning
    auto start = str.find_first_not_of(" \t\n\r\f\v");
    // If the string is empty or contains only whitespace, return an empty string
    if (start == std::string_view::npos) {
        return {};
    }
    // Find the first non-whitespace character from the end
    auto end = str.find_last_not_of(" \t\n\r\f\v");
    // Return the trimmed substring
    return str.substr(start, end - start + 1);
}
std::string WebSpiderLib::str_replace(std::string& originalString, std::string& string_to_replace, const std::string& replacement){
	if(string_to_replace.empty()){
		std::cerr << "Jsonlib::str_replace input empty" << '\n';
		return "";
	}
    size_t startPos = originalString.find(string_to_replace);
    try{
        while (startPos != std::string::npos) {
            originalString.replace(startPos, string_to_replace.length(), replacement);
            startPos = originalString.find(string_to_replace, startPos + replacement.length());
        }
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }
    return originalString;
}
std::vector<std::string> WebSpiderLib::splitString_bystring(const std::string& input, const std::string& delimiter){
    std::vector<std::string> tokens;
    if(input.empty() || delimiter.empty()){
        std::cerr << "WebSpiderLib::splitString_bystring input empty" << '\n';
    	return tokens;
    }
    size_t start = 0;
    size_t end = input.find(delimiter);
    while (end != std::string::npos) {
        tokens.push_back(input.substr(start, end - start));
        start = end + delimiter.length();
        end = input.find(delimiter, start);
    }
    tokens.push_back(input.substr(start, end));
    return tokens;
}
std::string WebSpiderLib::removeHtmlTags(const std::string& input){
    // Regular expression to match HTML tags
    std::regex htmlRegex("<[^>]*>");
    // Replace HTML tags with an empty string
    std::string result = std::regex_replace(input, htmlRegex, "");
    return result;
}
std::vector<std::string> WebSpiderLib::findAllWordsBehindSpans(const std::string& input, const std::string& strreg){
    std::regex regexPattern(strreg);
    std::smatch match;
    std::vector<std::string> words;
    std::sregex_iterator it(input.begin(), input.end(), regexPattern);
    std::sregex_iterator end;
    while (it != end) {
        words.push_back((*it)[1].str());
        ++it;
    }
    return words;
}
std::vector<std::string> WebSpiderLib::findAllSpans(const std::string& input, const std::string& strreg){
    std::vector<std::string> spans;
    std::regex regexPattern(strreg);
    std::smatch match;
    std::string::const_iterator searchStart(input.cbegin());
    while (std::regex_search(searchStart, input.cend(), match, regexPattern)) {
    	if(!match.empty()){
    		spans.push_back(match[0].str());
    	}
    	searchStart = match.suffix().first;
    }
    return spans;
}
std::string WebSpiderLib::findWordBehindSpan(const std::string& input, const std::string& strreg){
    std::regex regexPattern(strreg);
    std::smatch match;
    if (std::regex_search(input, match, regexPattern)) {
    	if(match.size() > 1){
    		return match[1].str();
    	}
    }
    return "";
}
std::string WebSpiderLib::GetURLContent(const std::string& url) {  
    CURL* curl = curl_easy_init();  
    std::string output;  
    try{
        if (curl) {  
            curl_easy_setopt(curl, CURLOPT_URL, url.c_str());  
            curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, WriteCallback);  
            curl_easy_setopt(curl, CURLOPT_WRITEDATA, &output);  
            // Optional settings  
            curl_easy_setopt(curl, CURLOPT_ACCEPT_ENCODING, ""); // Enable gzip/deflate  
            curl_easy_setopt(curl, CURLOPT_FOLLOWLOCATION, 1L);  
            curl_easy_setopt(curl, CURLOPT_AUTOREFERER, 1L);  
            curl_easy_setopt(curl, CURLOPT_MAXREDIRS, 10L);  
            curl_easy_setopt(curl, CURLOPT_TIMEOUT_MS, 20000L); // 20 seconds timeout  
            curl_easy_setopt(curl, CURLOPT_CONNECTTIMEOUT_MS, 2000L); // 2 seconds connect timeout  
            curl_easy_setopt(curl, CURLOPT_MAXFILESIZE_LARGE, (curl_off_t)1024 * 1024 * 1024); // 1 GB max file size  
            // Set the User-Agent header  
            curl_easy_setopt(curl, CURLOPT_USERAGENT, "Mozilla/5.0 (X11; Linux x86_64) AppleWebKit/537.36 (KHTML, like Gecko) Chrome/58.0.3029.110 Safari/537.3");  
            // SSL verification (for production, enable these)  
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYPEER, 1L);  
            curl_easy_setopt(curl, CURLOPT_SSL_VERIFYHOST, 2L);  
            // Perform the request  
            CURLcode res = curl_easy_perform(curl);  
            if (res != CURLE_OK) {  
                std::cerr << "Error: " << curl_easy_strerror(res) << std::endl;  
            }  
            // Cleanup  
            curl_easy_cleanup(curl);  
        } else {  
            std::cerr << "Error: Failed to initialize CURL." << std::endl;  
        }  
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }
    return output;  
}
std::string WebSpiderLib::GetURLContentAndSave(const std::string& url, const std::string& strReg){
    /*
        if a word was successfully crawled, then return 1
    */
    size_t word_was_found = 0;
    /*
        move the cursor to the first place
    */
    // Read the HTML file
    //std::ifstream file("example.html");
    //std::string htmlContent((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    std::vector<std::string> web_content;
    if(url.empty() || strReg.empty()){
        std::cerr << "WebSpiderLib::GetURLContentAndSave input empty!" << '\n';
        return "";
    }
    std::string htmlContent = this->GetURLContent(url);
    if(htmlContent.length() > 0){
        word_was_found = 1;
    }else{
        return "";
    }
    web_content =  this->findAllWordsBehindSpans(htmlContent,strReg);
	std::string web_content_preprocess;
	for(std::string& em : web_content){
		em = this->removeHtmlTags(em);
        web_content_preprocess += em + "\n";
	}
    return web_content_preprocess;
}
std::string WebSpiderLib::removeHtmlTags_google(GumboNode* node){
    if (node->type == GUMBO_NODE_TEXT) {
        return std::string(node->v.text.text);
    } else if (node->type == GUMBO_NODE_ELEMENT &&
                node->v.element.tag != GUMBO_TAG_SCRIPT &&
                node->v.element.tag != GUMBO_TAG_STYLE) {
        std::string contents;
        GumboVector* children = &node->v.element.children;
        for (unsigned int i = 0; i < children->length; ++i) {
            const std::string text = WebSpiderLib::removeHtmlTags_google(static_cast<GumboNode*>(children->data[i]));
            if (i != 0 && !text.empty()) {
                contents.append(" ");
            }
            contents.append(text);
        }
        return contents;
    } else {
        return "";
    }
}
std::string WebSpiderLib::G_removehtmltags(std::string& HTML_DOC){
	WebSpiderLib websl_j;
	GumboOutput* output = gumbo_parse(HTML_DOC.c_str());
	std::string strOut =  websl_j.removeHtmlTags_google(output->root);
	gumbo_destroy_output(&kGumboDefaultOptions, output);
	return strOut;
}
std::string WebSpiderLib::extractDomainFromUrl(const std::string& url){
    std::regex domainRegex(R"(?:https?:\/\/)?(?:www\.)?([^\/]+)\.([a-zA-Z]{2,}(?=/))");
    std::smatch match;
    if (std::regex_search(url, match, domainRegex)) {
    	if(match.size()>1){
    		std::string fullDomain = match[1];
			size_t pos = fullDomain.find("://");
			if(pos != std::string::npos){
				return fullDomain.substr(pos+3);
			}
    	}
    } 
	return "";
}
/*
    end WebSpiderLib
*/
std::string tech_web_template = R"(
<!DOCTYPE html>
<html lang="en">
<head>
    <meta charset="UTF-8">
    <meta name="viewport" content="width=device-width, initial-scale=1.0">
    <title>News Viewer</title>
    <style>
        body {
            display: flex;
            margin: 0;
            font-family: Arial, sans-serif;
        }
        .news-list {
            width: 30%;
            border-right: 1px solid #ccc;
            padding: 10px;
            overflow-y: auto;
            height: 100vh;
        }
        .news-list h2 {
            margin-top: 0;
        }
        .news-list ul {
            list-style-type: none;
            padding: 0;
        }
        .news-list li {
            margin: 5px 0;
            cursor: pointer;
            color: blue;
            text-decoration: underline;
        }
        .news-list li:hover {
            color: darkblue;
        }
        .news-content {
            width: 70%;
            padding: 10px;
            overflow-y: auto;
            height: 100vh;
        }
        iframe {
            width: 100%;
            height: 100%;
            border: none;
        }
    </style>
</head>
<body>
    <div class="news-list">
        <h2>News List</h2>
        !@#
    </div>
    <div class="news-content">
        <iframe id="news-frame" src=""></iframe>
    </div>
    <script>
        function showNews(url) {
            const iframe = document.getElementById('news-frame');
            iframe.src = url;
        }
    </script>
</body>
</html>
)";
/*
    tech news spider definition 
*/
struct web_return{
  std::string web_title;
  std::string img_thumb_path;
  std::string img_large_path;
  std::string webcontent;
};
std::string CurrDir;//current work directory
std::vector<web_return> unit_web_return;
std::map<std::string, std::string> main_url_titles;
std::vector<std::string> main_url_list; //main url list
std::unordered_map<std::string, std::vector<std::string>> str_urls;//sub url list
std::unordered_map<std::string, std::vector<std::string>> str_broken_urls;//sub broken url list
std::unordered_map<std::string, std::vector<std::string>> str_crawled_urls;//sub successfully crawlled url list
std::unordered_map<std::string, std::vector<std::string>> web_templates;
SysLogLib syslog_j;
WebSpiderLib web_j;
/*
    end definition
*/
bool isDomainExtension(const std::string& word){
    static const std::vector<std::string> domainExtensions = {
        // Generic Top-Level Domains (gTLDs):
        ".com", ".net", ".org", ".edu", ".gov", ".io", ".co", ".mil", ".int",
        ".info", ".biz", ".name", ".pro", ".aero", ".coop", ".museum",
        // Country Code Top-Level Domains (ccTLDs):
        ".us", ".uk", ".ca", ".de", ".fr", ".jp", ".cn", ".in", ".au", ".br",
        ".ru", ".za", ".mx", ".es", ".it", ".nl", ".se", ".no", ".dk", ".fi", 
        ".ie", ".nz", ".ch", ".at", ".be", ".gr", ".hu", ".pl", ".pt", ".tr",
        ".ar", ".cl", ".co", ".my", ".sg", ".hk", ".tw", ".kr", ".ph", ".id",
        // New gTLDs:
        ".app", ".blog", ".shop", ".online", ".site", ".tech", ".xyz", ".club",
        ".design", ".news", ".store", ".website", ".guru", ".space", ".today",
        ".solutions", ".agency", ".center", ".company", ".life", ".world",
        ".health", ".community", ".support", ".email", ".photography", ".media",
        // Sponsored Top-Level Domains (sTLDs):
        ".asia", ".cat", ".jobs", ".mobi", ".tel", ".travel", ".post", ".gov",
        // Infrastructure Top-Level Domain:
        ".arpa",
        // Additional new gTLDs and popular ccTLDs:
        ".bio", ".cloud", ".digital", ".family", ".expert", ".global", ".law",
        ".properties", ".report", ".reviews", ".show", ".tools", ".work", 
        ".academy", ".capital", ".care", ".dating", ".education", ".events",
        ".fund", ".host", ".institute", ".land", ".lease", ".loans", ".network",
        ".partners", ".software", ".vacations", ".ventures"
    };
    for (const auto& ext : domainExtensions) {
        if (word.size() >= ext.size() && word.substr(word.size() - ext.size()) == ext) {
            return true;
        }
    }
    return false;
}
const std::vector<std::string> domain_extensions = {
    // Generic Top-Level Domains (gTLDs):
    ".com", ".net", ".org", ".edu", ".gov", ".io", ".co", ".mil", ".int",
    ".info", ".biz", ".name", ".pro", ".aero", ".coop", ".museum",
    // Country Code Top-Level Domains (ccTLDs):
    ".us", ".uk", ".ca", ".de", ".fr", ".jp", ".cn", ".in", ".au", ".br",
    ".ru", ".za", ".mx", ".es", ".it", ".nl", ".se", ".no", ".dk", ".fi", 
    ".ie", ".nz", ".ch", ".at", ".be", ".gr", ".hu", ".pl", ".pt", ".tr",
    ".ar", ".cl", ".co", ".my", ".sg", ".hk", ".tw", ".kr", ".ph", ".id",
    // New gTLDs:
    ".app", ".blog", ".shop", ".online", ".site", ".tech", ".xyz", ".club",
    ".design", ".news", ".store", ".website", ".guru", ".space", ".today",
    ".solutions", ".agency", ".center", ".company", ".life", ".world",
    ".health", ".community", ".support", ".email", ".photography", ".media",
    // Sponsored Top-Level Domains (sTLDs):
    ".asia", ".cat", ".jobs", ".mobi", ".tel", ".travel", ".post", ".gov",
    // Infrastructure Top-Level Domain:
    ".arpa",
    // Additional new gTLDs and popular ccTLDs:
    ".bio", ".cloud", ".digital", ".family", ".expert", ".global", ".law",
    ".properties", ".report", ".reviews", ".show", ".tools", ".work", 
    ".academy", ".capital", ".care", ".dating", ".education", ".events",
    ".fund", ".host", ".institute", ".land", ".lease", ".loans", ".network",
    ".partners", ".software", ".vacations", ".ventures"
};
/*
    remove repeated items in std::vector<std::string>
*/
void addUniqueItems(std::vector<std::string>& old_data, const std::vector<std::string>& new_data) {  
    if(new_data.empty()){
        return;
    }
    try{
        // Create an unordered_set to track items in old_data for fast lookup  
        std::unordered_set<std::string> existing_items(old_data.begin(), old_data.end());  
        // Iterate through new_data and add only unique items to old_data  
        for (const auto& item : new_data) {  
            std::string trim_item = std::string(web_j.str_trim(item));
            if (existing_items.insert(trim_item).second) { // insert returns {iterator, bool}, second is true if inserted  
                old_data.push_back(trim_item);  
            }  
        } 
    } 
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl; 
    }
}
void write_url_to_file(const std::string& file_path, const std::string& url){
	if(file_path.empty() || url.empty()){
        return;
    }
    try{
        std::ofstream ofile(file_path, std::ios::app);
        if(!ofile.is_open()){
            ofile.open(file_path, std::ios::app);
        }
        ofile << url << '\n';
        ofile.close();
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl; 
    }
}
std::string getRawHtml(const std::string& strurl){
    std::string str_content;
    if(strurl.empty()){
        return str_content;
    }
	str_content = web_j.GetURLContent(strurl);
	return str_content;
}
std::string get_title_content(const std::string& raw_str){
	if(raw_str.empty()){
		std::cout << "get_title_content: input empty!" << '\n';
		return "";
	}
	return web_j.findWordBehindSpan(raw_str,"<title>(.*?)</title>");
}
std::string get_domain_string(std::string& cleanUrl){
    std::string str_domain;
    if(cleanUrl.empty()){
        return str_domain;
    }
    if (cleanUrl.find("http://") == 0) {  
        cleanUrl = cleanUrl.substr(7); // Remove "http://"  
    } else if (cleanUrl.find("https://") == 0) {  
        cleanUrl = cleanUrl.substr(8); // Remove "https://"  
    }  
    // Find the domain extension from the list  
    for (const auto& ext : domain_extensions) {  
        size_t extPos = cleanUrl.rfind(ext); // Search for the extension from the end  
        if (extPos != std::string::npos) {  
            // Find the position of the last '.' before the extension  
            size_t dotPos = cleanUrl.rfind('.', extPos - 1);  
            if (dotPos != std::string::npos) {  
                // Extract the substring between the last '.' and the extension  
                return cleanUrl.substr(dotPos + 1, extPos - dotPos - 1);  
            }  
        }  
    }  
    return str_domain;
}
/*
    Ensure urls does not contain other domain's urls
*/
void current_domain_urls_only(std::vector<std::string>& urls, const std::string& str_domain){
    if(urls.empty() || str_domain.empty()){
        return;
    }
    for(auto it = urls.begin(); it != urls.end();){
        if(it->find(str_domain) == std::string::npos 
            || it->find(".jpg") != std::string::npos || it->find("?") != std::string::npos
            || it->find(".png") != std::string::npos || it->find(".css") != std::string::npos
            || it->find(".js") != std::string::npos
            || it->find(".JPG") != std::string::npos
            || it->find(".jpeg") != std::string::npos
            || it->find(".JPEG") != std::string::npos
            || it->find(".gif") != std::string::npos
            || it->find(".PNG") != std::string::npos
            || it->find(".GIF") != std::string::npos
            ){
            it = urls.erase(it);
        }
        else{
            ++it;
        }
    }
}
/*
    Get url list from a txt file
*/
std::vector<std::string> get_url_list(const std::string& file_path){
	std::vector<std::string> url_list;
	if(file_path.empty()){
		std::cerr << "url list is empty!" << '\n';
		return url_list;
	}
    try{
        std::ifstream stopwordFile(file_path);
        if (!stopwordFile.is_open()) {
            std::cerr << "get_url_list::Error opening: " << file_path << '\n';
            return url_list;
        }
        std::string line;
        while(std::getline(stopwordFile, line)){
            line = std::string(web_j.str_trim(line));
            url_list.push_back(line);
        }
        stopwordFile.close();
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }
    return url_list;
}
//remove broken urls from std::unordered_map<std::string, std::vector<std::string>> str_urls;
//para1: webSite title, para2: url to remove
void remove_str_stored_urls(std::unordered_map<std::string,std::vector<std::string>>& str_urls, const std::string& webSiteTitle, const std::string& strurl_to_remove){  
    if (str_urls.empty() || webSiteTitle.empty() || strurl_to_remove.empty()) {  
        return; // Exit if either input is empty  
    }  
    auto it = str_urls.find(webSiteTitle);
    if (it != str_urls.end()) {  
        auto& url_list = it->second; // Reference to the vector of URLs  
        for (auto it = url_list.begin(); it != url_list.end(); ) {  
            if (*it == strurl_to_remove) {  
                it = url_list.erase(it); // Erase the URL and update the iterator  
            } else {  
                ++it; // Move to the next element  
            }  
        }  
    }  
}
std::string generateOutput(const std::string& str1, const std::string& str2) {  
    std::istringstream stream1(str1), stream2(str2);  
    std::vector<std::string> words1, words2;  
    // Split both strings into words  
    std::string word;  
    while (stream1 >> word) {  
        words1.push_back(word);  
    }  
    while (stream2 >> word) {  
        words2.push_back(word);  
    }  
    // Iterate through both word vectors and compare  
    std::string result;  
    auto it1 = words1.begin();  
    auto it2 = words2.begin();  
    while (it1 != words1.end() && it2 != words2.end()) {  
        if (*it1 == *it2) {  
            // If words are the same, add them to the result  
            result += *it1 + " ";  
        } else {  
            // If words differ, add the separator  
            result += "!@# ";  
            // Skip differing words in both strings  
            while (it1 != words1.end() && it2 != words2.end() && *it1 != *it2) {  
                ++it1;  
                ++it2;  
            }  
            continue;  
        }  
        ++it1;  
        ++it2;  
    }  
    // Handle remaining words (if any)  
    while (it1 != words1.end()) {  
        result += "!@# ";  
        ++it1;  
    }  
    while (it2 != words2.end()) {  
        result += "!@# ";  
        ++it2;  
    }  
    // Remove trailing space  
    if (!result.empty() && result.back() == ' ') {  
        result.pop_back();  
    }  
    return result;  
}
// Function to extract only web page URLs  
std::vector<std::string> extractWebPageUrls(const std::string& html) {  
    std::vector<std::string> urls;  
    // Regular expression to match URLs in href or src attributes  
    // Only include URLs ending with .html, .asp, .jsp, .php, etc.  
    std::regex urlRegex(R"((href|src)\s*=\s*["']([^"']+\.(html|asp|jsp|php|aspx|htm))["'])");  
    std::smatch match;  
    // Use regex_iterator to find all matches  
    auto begin = std::sregex_iterator(html.begin(), html.end(), urlRegex);  
    auto end = std::sregex_iterator();  
    for (auto it = begin; it != end; ++it) {  
        // Extract the URL (second capture group in the regex)  
        urls.push_back((*it)[2].str());  
    }  
    return urls;  
}   
// BFS-based function to get all URLs within the same domain  
std::vector<std::string> get_all_urls(const std::string& start_url, const std::string& domain) {  
    std::vector<std::string> all_urls; // Final list of all URLs  
    if (start_url.empty() || domain.empty()) {  
        return all_urls;  
    }  
    std::unordered_set<std::string> visited; // Track visited URLs  
    std::queue<std::string> to_visit;       // Queue for BFS traversal  
    // Start with the initial URL  
    to_visit.push(start_url);  
    visited.insert(start_url);  
    while (!to_visit.empty()) {  
        std::string current_url = to_visit.front();  
        to_visit.pop();  
        std::cout << "Fetching URL: " << current_url << std::endl;  
        // Fetch the raw HTML content of the current URL  
        std::string html_content = getRawHtml(current_url);  
        std::this_thread::sleep_for(std::chrono::seconds(2)); // Throttle requests to avoid overloading the server  
        if (!html_content.empty()) {  
            // Extract URLs from the HTML content  
            std::vector<std::string> extracted_urls = extractWebPageUrls(html_content);  
            // Filter URLs to include only those within the same domain  
            current_domain_urls_only(extracted_urls, domain);  
            for (const auto& url : extracted_urls) {  
                // If the URL hasn't been visited, add it to the queue and mark it as visited  
                if (visited.find(url) == visited.end()) {  
                    to_visit.push(url);  
                    visited.insert(url);  
                    all_urls.push_back(url); // Add to the final list of URLs  
                }  
            }  
        }  
        std::cout << "Finished processing: " << current_url << std::endl;  
    }  
    return all_urls;  
}
/*
    check broken url list file, and 
*/
void read_other_txt_files(const std::vector<std::string>& url_list){
    if(main_url_titles.empty() || url_list.empty()){
        return;
    }
    if(!CurrDir.empty()){
        try{
            for(const auto& main_url : url_list){
                std::string web_title = main_url_titles[main_url];
                std::string urls_path = CurrDir + "/web_temp/" + web_title + "_urls.txt";
                if(std::filesystem::exists(urls_path)){
                    std::cout << "Previous configurations found, reading " << urls_path << std::endl;
                    std::vector<std::string> get_urls = get_url_list(urls_path);
                    if(!get_urls.empty()){
                        str_urls[web_title] = get_urls;
                    }
                }
                std::string crawled_path = CurrDir + "/web_temp/" + web_title + "_crawled.txt";
                if(std::filesystem::exists(crawled_path)){
                    std::cout << "Previous configurations found, reading " << crawled_path << std::endl;
                    std::vector<std::string> get_crawled = get_url_list(crawled_path);
                    if(!get_crawled.empty()){
                        str_crawled_urls[web_title] = get_crawled;
                    }
                }
                std::string broken_path = CurrDir + "/web_temp/" + web_title + "_broken.txt";
                if(std::filesystem::exists(broken_path)){
                    std::cout << "Previous configurations found, reading " << broken_path << std::endl;
                    std::vector<std::string> get_broken = get_url_list(broken_path);
                    if(!get_broken.empty()){
                        str_broken_urls[web_title] = get_broken;
                    }
                }
                std::string template_path = CurrDir + "/web_temp/" + web_title + "_template.txt";
                if(std::filesystem::exists(template_path)){
                    std::cout << "Previous configurations found, reading " << template_path << std::endl;
                    std::vector<std::string> get_temp = get_url_list(template_path);
                    if(!get_temp.empty()){
                        web_templates[web_title] = get_temp;
                    }
                }
            }
        }
        catch(const std::exception& ex){
            std::cerr << ex.what() << std::endl;
        }
        catch(...){
            std::cerr << "Unknown errors." << std::endl; 
        }
    }
}
/*
    Save url list to file_path (txt file)
*/
void save_url_list_to_txt_file(const std::vector<std::string>& url_list, const std::string& file_path){
    if(url_list.empty()){
        return;
    }
    try{
        std::ofstream ofile(file_path, std::ios::out);
        if(!ofile.is_open()){
            ofile.open(file_path, std::ios::out);
        }
        for(const auto& url_item : url_list){
            ofile << url_item << '\n';
        }
        ofile.close();
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }
    
}
/*
    Get domain's template 
*/
void get_domain_web_templates(const std::string& strDomain, const std::vector<std::string>& str_url){
    if(str_url.empty()){
        return;
    }
    try{
        std::cout << "Creating web template for the domain: " << strDomain << std::endl;
        std::vector<std::string> old_web_template;
        for(const auto& url_item : str_url){
            std::string str_web_content = getRawHtml(url_item);
            std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
            for(const auto& url_compare : str_url){
                if(url_compare != url_item){
                    std::string web_content_compare = getRawHtml(url_compare);
                    std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
                    std::string web_compare_str = generateOutput(str_web_content,web_content_compare);
                    std::vector<std::string> web_temp_t = web_j.splitString_bystring(web_compare_str, "!@#");
                    addUniqueItems(old_web_template, web_temp_t);
                }
            }
        }
        if(!old_web_template.empty()){
            web_templates[strDomain] = old_web_template;
        }
        //save to file
        std::string str_file_path = CurrDir + "/web_temp/" + strDomain + "_template.txt";
        std::ofstream ofile(str_file_path, std::ios::out);
        if(!ofile.is_open()){
            ofile.open(str_file_path, std::ios::out);
        }
        for(const auto& item : old_web_template){
            ofile << item << '\n';
        }
        ofile.close();
        std::cout << "Done creating web template the file was successfully saved at: " << str_file_path << std::endl;
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }
}
void generate_large_html_page(
    const std::string& output_html_path, 
    std::string& current_raw_html_content
    ){
    if(output_html_path.empty() || current_raw_html_content.empty()){
        return;
    }
    /*
        get all images
    */
    std::string strMainHtml="<html><head><title>";
    strMainHtml.append("</title></head><body>");
    std::vector<std::string> image_urls = web_j.findAllWordsBehindSpans(current_raw_html_content,"<img src=\"(.*?)\">");
    std::vector<std::string> image_urls_objs = web_j.findAllWordsBehindSpans(current_raw_html_content,"<object data=\"(.*?)\" type=\"image/jpeg\">");
    std::vector<std::string> image_urls_embed = web_j.findAllWordsBehindSpans(current_raw_html_content,"<embed src=\"(.*?)\" type=\"image/jpeg\">");
    std::vector<std::string> image_urls_canvas = web_j.findAllWordsBehindSpans(current_raw_html_content,"img.src='(.*?)'");
    if(!image_urls.empty()){
        for(const auto& img_url : image_urls){
            strMainHtml.append("<img src=\"" + img_url + "\" width=\"640\" height=\"320\">");
        }
    }
    if(!image_urls_objs.empty()){
        for(const auto& img_url : image_urls_objs){
            strMainHtml.append("<img src=\"" + img_url + "\" width=\"640\" height=\"320\">");
        }
    }
    if(!image_urls_embed.empty()){
        for(const auto& img_url : image_urls_embed){
            strMainHtml.append("<img src=\"" + img_url + "\" width=\"640\" height=\"320\">");
        }
    }
    if(!image_urls_canvas.empty()){
        for(const auto& img_url : image_urls_canvas){
            strMainHtml.append("<img src=\"" + img_url + "\" width=\"640\" height=\"320\">");
        }
    }
    /*
        main content - remove all html tags
    */
    try{
        std::string str_main_body = web_j.G_removehtmltags(current_raw_html_content);
        strMainHtml.append(str_main_body);
        //get large html content
        strMainHtml.append("</body></html>");
        //save to file
        std::ofstream ofile(output_html_path, std::ios::out);
        if(!ofile.is_open()){
            ofile.open(output_html_path, std::ios::out);
        }
        ofile << strMainHtml << '\n';
        ofile.close();
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }
}
void generate_html_for_return_data(
    const std::string& strDomain, 
    const std::vector<std::string>& url_list, 
    std::string& out_content_html, 
    const std::string& output_folder){
    if(strDomain.empty() || url_list.empty()){
        return;
    }
    try{
        for(const auto& str_url : url_list){
            if(!str_broken_urls.empty()){
                const auto& urls = str_broken_urls[strDomain];  
                if (std::find(urls.begin(), urls.end(), str_url) != urls.end()) {  
                    continue; // Skip if str_url is already in the vector  
                }
            }
            std::string strWebContent = getRawHtml(str_url);
            std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
            if(strWebContent.empty()){
                auto& broken_url_list = str_broken_urls[strDomain];
                broken_url_list.push_back(str_url);
                //save broken url list to the disk
                //CurrDir + "/web_temp/" + web_title + "_broken.txt";
                save_url_list_to_txt_file(broken_url_list, CurrDir + "/web_temp/" + strDomain + "_broken.txt");
            }
            if(!web_templates.empty()){
                auto currentWebTemplate = web_templates[strDomain];
                for(auto& web_t : currentWebTemplate){
                    strWebContent = web_j.str_replace(strWebContent, web_t, "@#$");
                }
                std::vector<std::string> return_web_final_content = web_j.splitString_bystring(strWebContent, "@#$");
                /*
                    get html elements
                */
                /*
                    Start creating html webpage
                */
                out_content_html += "<div><h3>";
                out_content_html.append(strDomain);
                out_content_html.append("</h3>");
                if(!return_web_final_content.empty()){
                    for(auto& item : return_web_final_content){
                        /*
                            Headlines
                        */
                        out_content_html.append("ul");
                        /*
                            Get href titles
                        */
                        // Regular expression to match <a> tags and extract href and text  
                        std::regex pattern(R"(<a\s+href=\"([^"]+)\">([^<]+)</a>)");  
                        // Create a container to store matches  
                        std::smatch match;  
                        // Iterator to search through the HTML string  
                        std::string::const_iterator search_start(item.cbegin());  
                        // Loop through all matches  
                        while (std::regex_search(search_start, item.cend(), match, pattern)) {  
                            // match[1] contains the href value  
                            std::string url = match[1];  
                            // match[2] contains the text between the <a> tags  
                            std::string headline = match[2];  
                            //generate large html page
                            //get large html raw content
                            std::string largeHtml = getRawHtml(url);
                            std::this_thread::sleep_for(std::chrono::seconds(2));//seconds
                            if(largeHtml.empty()){
                               std::cerr << "Failed to open url: " <<  url << std::endl;
                               continue;
                            }
                            generate_large_html_page(output_folder + "/mainHtml/" + headline + ".html", largeHtml);
                            // Print the results  
                            //std::cout << "URL: " << url << "\n";  
                            //std::cout << "Headline: " << headline << "\n"; 
                            //<li onclick="showNews('https://example.com/abc-headline1')">Headline 1</li>
                            out_content_html.append("<li onclick=\"showNews('\"");
                            out_content_html.append("mainHtml/" + headline + ".html");
                            out_content_html.append("\"')\">");
                            out_content_html.append(headline);
                            out_content_html.append("</li>");
                            // Move the search start position forward  
                            search_start = match.suffix().first;  
                        }  
                        out_content_html.append("</ul></div>");
                    }
                }
            }
            /*
                Add to successfully crawled link list
            */
            auto& crawledLinks = str_crawled_urls[strDomain];
            crawledLinks.push_back(str_url);
            save_url_list_to_txt_file(crawledLinks, CurrDir + "/web_temp/" + strDomain + "_crawled.txt");
            std::cout << "Successfully saved crawled link: " << str_url << std::endl;
        }
        /*
            main html page
        */
        std::string final_main_index_page = tech_web_template;
        std::string str_to_replace_html = "!@#";
        final_main_index_page = web_j.str_replace(final_main_index_page, str_to_replace_html, out_content_html);
        //save to disk
        std::ofstream ofile(output_folder + "/index.html", std::ios::out);
        if(!ofile.is_open()){
            ofile.open(output_folder + "/index.html", std::ios::out);
        }
        ofile << final_main_index_page << '\n';
        ofile.close();
        std::cout << "Successfully generated index.html page at: " << output_folder << std::endl;
        std::cout << "All jobs are done!" << std::endl;
    }
    catch(const std::exception& ex){
        std::cerr << ex.what() << std::endl;
    }
    catch(...){
        std::cerr << "Unknown errors." << std::endl;
    }
}
void start_crawling(const std::string& txt_list_path, std::string& output_folder_path){
    if(txt_list_path.empty() || output_folder_path.empty()){
        std::cerr << "List text file path is empty or outupt folder path is empty." << std::endl;
        return;
    }
    if(!std::filesystem::exists(txt_list_path)){
        std::cerr << "List text file: " << txt_list_path << " does not exists." << std::endl;
        return;
    }
    std::ifstream ifile(txt_list_path);
    if(!ifile.is_open()){
        std::cerr << "Failed to open file for reading: " << txt_list_path << std::endl;
    }
    if(output_folder_path.back() != '/'){
        output_folder_path.append("/");
    }
    main_url_list.clear();
    main_url_list = get_url_list(txt_list_path);
    //start processing
    if(!main_url_list.empty()){
        main_url_titles.clear();
        //get web titles for all main url list
        std::cout << "Main url list: " << '\n';
        for(auto& item : main_url_list){
            std::string web_title = get_domain_string(item);
            web_title = std::string(web_j.str_trim(web_title));
            if(!web_title.empty()){
                main_url_titles[item] = web_title;
                std::cout << "Domain: " << web_title << " Url: " << item << '\n';
            }
        }
        std::cout << std::endl;
        /*
            check temp files for previous crawling data
        */
        if(std::filesystem::exists(CurrDir + "/web_temp")){
            read_other_txt_files(main_url_list);
        }
        else{
            /*
                create web_temp folder
            */
            std::filesystem::create_directory(CurrDir + "/web_temp");
            std::cout << "Start a new crawling..." << std::endl;
            if(!main_url_list.empty()){
                //start to create a new urls for all websites
                for(const auto& item : main_url_list){
                    std::cout << "Getting all urls at domain: " << item << std::endl; 
                    std::vector<std::string> domain_all_urls = get_all_urls(item, main_url_titles[item]);
                    str_urls[item] = domain_all_urls;
                    //save to file
                    std::string str_file_name = CurrDir + "/web_temp/" + main_url_titles[item] + "_urls.txt";
                    save_url_list_to_txt_file(domain_all_urls, str_file_name);
                    std::cout << "Done! Urls list has been saved to: " << str_file_name << std::endl;
                    /*
                     * get_domain_web_templates 
                    */
                    if(web_templates.empty()){//create webtemplate for the domain
                        get_domain_web_templates(main_url_titles[item], domain_all_urls);
                    }
                }
            }
            else{
                std::cerr << "main_url_list or main_url_titles is empty!" << std::endl;
            }
        }
        //std::unordered_map<std::string, std::vector<std::string>> str_urls;//sub url list
        //std::unordered_map<std::string, std::vector<std::string>> str_broken_urls;//sub broken url list
        //std::unordered_map<std::string, std::vector<std::string>> str_crawled_urls;//sub successfully crawlled url list
        /*
            start crawling
        */
        std::string str_date_folder_name = syslog_j.get_current_date_as_string();
        std::string output_folder = output_folder_path; 
        output_folder.append(str_date_folder_name);
        if(!std::filesystem::exists(output_folder)){
            std::filesystem::create_directory(output_folder);
        }
        if(!str_urls.empty()){
            for(const auto& domain_item : str_urls){
                std::string str_web_domain = domain_item.first;
                std::string outContentHtml;
                generate_html_for_return_data(str_web_domain, domain_item.second, outContentHtml, output_folder);
                std::cout << "Getting html content: " << outContentHtml << std::endl;
            }
        }
    }
}
void showUsage(const std::string& program_name){
    std::cout << "Usage: " << program_name << " [space]\n";
    std::cout << "parameter 1: websites url list text file path.\n";
    std::cout << "[space]\n";
    std::cout << "parameter 2: output folder path, path/to/output" << std::endl;
}
int main(int argc, char* argv[]){
    if(argc < 2){
        showUsage("tech_news");
        return -1;
    }
    nemslib_webcam nems_j;
    CurrDir = nems_j.getExecutableDirectory();
    if(CurrDir.empty()){
       std::cerr << "Failed to get current directory!" << std::endl; 
       return -1;
    }
    std::string para1 = argv[1];
    std::string para2 = argv[2];
    if(para1.empty() || para2.empty()){
        return -1;
    }
    start_crawling(para1, para2);
    return 0;
}