/*
	g++ -std=c++20 '/home/ronnieji/ronnieji/lib/MLCpplib-main/utf_8_convert.cpp' \
    -o '/home/ronnieji/ronnieji/lib/MLCpplib-main/utf_8_convert' \
    -I/usr/include/unicode -licuuc -licudata 
 */
#include <string>  
#include <stdexcept>  
#include <iostream>   
#include <unicode/unistr.h>  
#include <unicode/ucnv.h>  
#include <unicode/ustream.h>  
#include <iconv.h> //The iconv library is another option for encoding conversion. It is available on most Unix-like systems.
#include <cstring>  //for iconv

/*
 *Function to check if a string is utf-8 format 
 */
bool isValidUTF8(const std::string& str) {  
    size_t i = 0;  
    while (i < str.size()) {  
        unsigned char c = str[i];  
        if ((c & 0x80) == 0) { // 1-byte character (ASCII)  
            ++i;  
        } else if ((c & 0xE0) == 0xC0) { // 2-byte character  
            if (i + 1 >= str.size() || (str[i + 1] & 0xC0) != 0x80) return false;  
            i += 2;  
        } else if ((c & 0xF0) == 0xE0) { // 3-byte character  
            if (i + 2 >= str.size() || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80) return false;  
            i += 3;  
        } else if ((c & 0xF8) == 0xF0) { // 4-byte character  
            if (i + 3 >= str.size() || (str[i + 1] & 0xC0) != 0x80 || (str[i + 2] & 0xC0) != 0x80 || (str[i + 3] & 0xC0) != 0x80) return false;  
            i += 4;  
        } else {  
            return false; // Invalid UTF-8 byte  
        }  
    }  
    return true;  
}
// Convert a string from a given encoding to UTF-8  
std::string convertToUTF8(const std::string& input, const std::string& encoding) {  
    UErrorCode errorCode = U_ZERO_ERROR;  
    // Create a converter for the given encoding  
    UConverter* converter = ucnv_open(encoding.c_str(), &errorCode);  
    if (U_FAILURE(errorCode)) {  
        throw std::runtime_error("Failed to open converter for encoding: " + encoding);  
    }  
    // Convert the input string to UTF-8  
    int32_t utf8Length = ucnv_toAlgorithmic(UCNV_UTF8, converter, nullptr, 0, input.c_str(), input.size(), &errorCode);  
    if (errorCode != U_BUFFER_OVERFLOW_ERROR && U_FAILURE(errorCode)) {  
        ucnv_close(converter);  
        throw std::runtime_error("Failed to calculate UTF-8 length");  
    }  
    std::string utf8Str(utf8Length, '\0');  
    errorCode = U_ZERO_ERROR;  
    ucnv_toAlgorithmic(UCNV_UTF8, converter, &utf8Str[0], utf8Length, input.c_str(), input.size(), &errorCode);  
    ucnv_close(converter);  
    if (U_FAILURE(errorCode)) {  
        throw std::runtime_error("Failed to convert string to UTF-8");  
    }  
    return utf8Str;  
}  
std::string convertToUTF8_iconv(const std::string& input, const std::string& fromEncoding) {  
    iconv_t cd = iconv_open("UTF-8", fromEncoding.c_str());  
    if (cd == (iconv_t)-1) {  
        throw std::runtime_error("Failed to open iconv for encoding: " + fromEncoding);  
    }  
    size_t inBytesLeft = input.size();  
    size_t outBytesLeft = input.size() * 4; // UTF-8 may require up to 4 bytes per character  
    std::string output(outBytesLeft, '\0');  
    char* inBuf = const_cast<char*>(input.data());  
    char* outBuf = &output[0];  
    if (iconv(cd, &inBuf, &inBytesLeft, &outBuf, &outBytesLeft) == (size_t)-1) {  
        iconv_close(cd);  
        throw std::runtime_error("Failed to convert string to UTF-8");  
    }  
    iconv_close(cd);  
    output.resize(output.size() - outBytesLeft); // Resize to actual output size  
    return output;  
}  
int main() {  
	// Example input string (unknown encoding)  
    std::string input = "\xD6\xD0\xCE\xC4"; // "中文" in GBK  
	std::string encoding = "GBK"; 
    // Step 1: Validate UTF-8  
    if (isValidUTF8(input)) {  
        std::cout << "Input is valid UTF-8: " << input << '\n';  
    } else {  
        std::cout << "Input is not UTF-8. Converting...\n";  
        // Step 2: Convert to UTF-8 (assuming GBK encoding for this example)  
        try {  
            std::string utf8Str = convertToUTF8(input, encoding);  
            std::cout << "Converted to UTF-8: " << utf8Str << '\n';  
        } catch (const std::exception& e) {  
            std::cerr << "Error: " << e.what() << '\n';  
        }  
		try {  
			std::string utf8Str = convertToUTF8_iconv(input, encoding);  
			std::cout << "Converted to UTF-8: " << utf8Str << '\n';  
		} catch (const std::exception& e) {  
			std::cerr << "Error: " << e.what() << '\n';  
		}  
    }  
    return 0;  
} 