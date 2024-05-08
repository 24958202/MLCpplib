#include <iostream>
#include <string>
#include <cgicc/Cgicc.h>
#include <cgicc/HTTPHTMLHeader.h>
#include <cgicc/HTMLClasses.h>

int main() {
  // Read input from request
  cgicc::Cgicc formData;
  std::string userInput;
  cgicc::form_iterator strUserInput = formData.getElement("user-input");
  if (!strUserInput->isEmpty() && strUserInput!= (*formData).end()) {
    userInput = **strUserInput;
  }

  // Create HTML response
  std::cout << "Content-Type: text/html\r\n\r\n";
  std::cout << "<html><body>";
  std::cout << "<p>Hello " << userInput << "!</p>";
  std::cout << "</body></html>";

  return 0;
}