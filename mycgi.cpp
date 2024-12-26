/*
 *	sudo apt-get install libcgicc-dev
	g++ -o your_script.cgi mycgi.cpp -lcgicc
	chmod +x /path/to/cgi-bin/your_script.cgi
 */
#include <iostream>  
#include <string>  
#include <vector>  
#include <cgicc/Cgicc.h>  
#include <cgicc/HTTPHTMLHeader.h>  
#include <cgicc/HTMLClasses.h>  

int main() {  
    try {  
        // Create an instance of Cgicc to handle form data  
        cgicc::Cgicc formData;  

        // Output HTTP header  
        std::cout << cgicc::HTTPHTMLHeader() << std::endl;  

        // Start HTML response  
        std::cout << "<html><head><title>Form Submission</title></head><body>" << std::endl;  
        std::cout << "<h2>Form Submission Results</h2>" << std::endl;  

        // Retrieve and display the username  
        cgicc::form_iterator username = formData.getElement("username");  
        if (username != formData.getElements().end() && !username->isEmpty()) {  
            std::cout << "<p>Username: " << **username << "</p>" << std::endl;  
        } else {  
            std::cout << "<p>Username: Not provided</p>" << std::endl;  
        }  

        // Retrieve and display the password (not recommended to display in real applications)  
        cgicc::form_iterator password = formData.getElement("password");  
        if (password != formData.getElements().end() && !password->isEmpty()) {  
            std::cout << "<p>Password: " << **password << "</p>" << std::endl;  
        } else {  
            std::cout << "<p>Password: Not provided</p>" << std::endl;  
        }  

        // Retrieve and display the selected country  
        cgicc::form_iterator country = formData.getElement("country");  
        if (country != formData.getElements().end() && !country->isEmpty()) {  
            std::cout << "<p>Country: " << **country << "</p>" << std::endl;  
        } else {  
            std::cout << "<p>Country: Not selected</p>" << std::endl;  
        }  

        // Retrieve and display the selected gender  
        cgicc::form_iterator gender = formData.getElement("gender");  
        if (gender != formData.getElements().end() && !gender->isEmpty()) {  
            std::cout << "<p>Gender: " << **gender << "</p>" << std::endl;  
        } else {  
            std::cout << "<p>Gender: Not selected</p>" << std::endl;  
        }  

        // Retrieve and display the selected interests (checkboxes)  
        std::vector<cgicc::form_iterator> interests = formData.getElements("interests");  
        if (!interests.empty()) {  
            std::cout << "<p>Interests: ";  
            for (auto& interest : interests) {  
                std::cout << **interest << " ";  
            }  
            std::cout << "</p>" << std::endl;  
        } else {  
            std::cout << "<p>Interests: None selected</p>" << std::endl;  
        }  

        // Handle file upload for profile picture  
        cgicc::file_iterator profilePic = formData.getFile("profile_pic");  
        if (profilePic != formData.getFiles().end()) {  
            std::cout << "<p>Profile Picture Uploaded: " << profilePic->getFilename() << "</p>" << std::endl;  
        } else {  
            std::cout << "<p>Profile Picture: Not uploaded</p>" << std::endl;  
        }  

        // Handle multiple file uploads for documents  
        std::vector<cgicc::file_iterator> documents = formData.getFiles("documents");  
        if (!documents.empty()) {  
            std::cout << "<p>Documents Uploaded:</p><ul>" << std::endl;  
            for (auto& doc : documents) {  
                std::cout << "<li>" << doc->getFilename() << "</li>" << std::endl;  
            }  
            std::cout << "</ul>" << std::endl;  
        } else {  
            std::cout << "<p>Documents: None uploaded</p>" << std::endl;  
        }  

        // End HTML response  
        std::cout << "</body></html>" << std::endl;  

    } catch (const std::exception& e) {  
        // Handle any exceptions  
        std::cout << "Content-Type: text/html\r\n\r\n";  
        std::cout << "<html><body>";  
        std::cout << "<h2>Error processing form</h2>";  
        std::cout << "<p>" << e.what() << "</p>";  
        std::cout << "</body></html>";  
    }  

    return 0;  
}
