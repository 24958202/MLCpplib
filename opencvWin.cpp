#include <opencv2/opencv.hpp>
#include <iostream>
#include <string>
#include <ctime>
#include <chrono>
#include <sstream>
#include <iomanip>
#include <cstdlib>
#include <array>
#include <stdexcept>
#include <functional>

// Button properties
static cv::Rect buttonArea(250, 300, 100, 40);
static bool buttonHover = false;
static bool okClicked = false;

// Wrapper for mouse callback
void mouseHandler(int event, int x, int y, int flags, void* userdata) {
    auto* handler = static_cast<std::function<void(int, int, int, int)>*>(userdata);
    (*handler)(event, x, y, flags);
}

// Improved command execution with error handling
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    return result;
}

// Validate date/time input
bool isValidDateTime(const std::tm& time) {
    // Basic validation
    if (time.tm_year < 0 || 
        time.tm_mon < 0 || time.tm_mon > 11 || 
        time.tm_mday < 1 || time.tm_mday > 31 ||
        time.tm_hour < 0 || time.tm_hour > 23 ||
        time.tm_min < 0 || time.tm_min > 59 ||
        time.tm_sec < 0 || time.tm_sec > 59) {
        return false;
    }

    // Convert to time_t to let the system validate
    std::time_t test = std::mktime(const_cast<std::tm*>(&time));
    if (test == -1) return false;

    // Compare with converted back tm to catch invalid dates
    std::tm* verified = std::localtime(&test);
    return (verified->tm_year == time.tm_year &&
            verified->tm_mon == time.tm_mon &&
            verified->tm_mday == time.tm_mday);
}

// Enhanced system time setting with better error reporting
bool setSystemTime(const std::tm& newTime) {
    if (!isValidDateTime(newTime)) {
        std::cerr << "Error: Invalid date/time specified" << std::endl;
        return false;
    }

    std::time_t new_time = std::mktime(const_cast<std::tm*>(&newTime));
    if (new_time == -1) {
        std::cerr << "Error: Failed to convert time specification" << std::endl;
        return false;
    }

    std::stringstream ss;
    ss << "sudo date -s @" << new_time << " 2>&1";
    std::string command = ss.str();
    
    std::string output = exec(command.c_str());
    if (output.find("date: ") != std::string::npos) {
        std::cerr << "Error setting time: " << output << std::endl;
        return false;
    }

    // Try to sync hardware clock, but don't fail if hwclock isn't available
    int hwclockResult = system("which hwclock > /dev/null 2>&1");
    if (hwclockResult == 0) {
        output = exec("sudo hwclock --systohc 2>&1");
        if (!output.empty() && output.find("hwclock: ") != std::string::npos) {
            std::cerr << "Warning: Could not sync hardware clock: " << output << std::endl;
            // Don't return false here - system time was still set
        }
    } else {
        std::cerr << "Warning: hwclock not found - system time was set but hardware clock was not synced" << std::endl;
    }

    return true;
}

// Create a better button with hover effect
void drawButton(cv::Mat& img, const std::string& text, const cv::Rect& area, bool hover = false) {
    cv::Scalar color = hover ? cv::Scalar(0, 200, 0) : cv::Scalar(0, 255, 0);
    cv::rectangle(img, area, color, cv::FILLED);
    cv::rectangle(img, area, cv::Scalar(0, 0, 0), 1);
    
    int baseline = 0;
    cv::Size textSize = cv::getTextSize(text, cv::FONT_HERSHEY_SIMPLEX, 0.7, 2, &baseline);
    cv::Point textOrg((area.width - textSize.width)/2 + area.x, 
                     (area.height + textSize.height)/2 + area.y);
    
    cv::putText(img, text, textOrg, cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);
}

// Trackbar callback functions
void onYearChange(int value, void* userdata) { *static_cast<int*>(userdata) = value; }
void onMonthChange(int value, void* userdata) { *static_cast<int*>(userdata) = value; }
void onDayChange(int value, void* userdata) { *static_cast<int*>(userdata) = value; }
void onHourChange(int value, void* userdata) { *static_cast<int*>(userdata) = value; }
void onMinuteChange(int value, void* userdata) { *static_cast<int*>(userdata) = value; }
void onSecondChange(int value, void* userdata) { *static_cast<int*>(userdata) = value; }

int main() {
    // Get current time
    std::time_t now = std::time(nullptr);
    std::tm currentTime = *std::localtime(&now);
    
    // Create window
    const std::string winName = "Set Date and Time";
    cv::namedWindow(winName, cv::WINDOW_AUTOSIZE);
    cv::resizeWindow(winName, 600, 400);
    
    // Trackbar values with reasonable limits
    int year = currentTime.tm_year + 1900;
    int month = currentTime.tm_mon + 1;
    int day = currentTime.tm_mday;
    int hour = currentTime.tm_hour;
    int minute = currentTime.tm_min;
    int second = currentTime.tm_sec;
    
    // Create trackbars with proper ranges and callbacks
    cv::createTrackbar("Year (1900-2100)", winName, nullptr, 2100, onYearChange, &year);
    cv::setTrackbarPos("Year (1900-2100)", winName, year);
    cv::setTrackbarMin("Year (1900-2100)", winName, 1900);
    
    cv::createTrackbar("Month (1-12)", winName, nullptr, 12, onMonthChange, &month);
    cv::setTrackbarPos("Month (1-12)", winName, month);
    cv::setTrackbarMin("Month (1-12)", winName, 1);
    
    cv::createTrackbar("Day (1-31)", winName, nullptr, 31, onDayChange, &day);
    cv::setTrackbarPos("Day (1-31)", winName, day);
    cv::setTrackbarMin("Day (1-31)", winName, 1);
    
    cv::createTrackbar("Hour (0-23)", winName, nullptr, 23, onHourChange, &hour);
    cv::setTrackbarPos("Hour (0-23)", winName, hour);
    
    cv::createTrackbar("Minute (0-59)", winName, nullptr, 59, onMinuteChange, &minute);
    cv::setTrackbarPos("Minute (0-59)", winName, minute);
    
    cv::createTrackbar("Second (0-59)", winName, nullptr, 59, onSecondChange, &second);
    cv::setTrackbarPos("Second (0-59)", winName, second);
    
    // Mouse callback using lambda with wrapper
    auto mouseCallback = [&](int event, int x, int y, int) {
        if (buttonArea.contains(cv::Point(x, y))) {
            if (event == cv::EVENT_LBUTTONDOWN) {
                okClicked = true;
            }
            buttonHover = (event == cv::EVENT_MOUSEMOVE);
        } else {
            buttonHover = false;
        }
    };
    
    // Store the callback
    static std::function<void(int, int, int, int)> func = mouseCallback;
    
    // Set the callback
    cv::setMouseCallback(winName, mouseHandler, &func);
    
    // Main loop
    cv::Mat display(400, 600, CV_8UC3, cv::Scalar(240, 240, 240));
    while (true) {
        display.setTo(cv::Scalar(240, 240, 240));
        
        // Display current selection
        std::stringstream timeText;
        timeText << "Selected Time: " << std::setfill('0') 
                 << std::setw(4) << year << "-"
                 << std::setw(2) << month << "-"
                 << std::setw(2) << day << " "
                 << std::setw(2) << hour << ":"
                 << std::setw(2) << minute << ":"
                 << std::setw(2) << second;
        
        cv::putText(display, timeText.str(), cv::Point(50, 50), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 0), 2);
        
        // Draw button with hover effect
        drawButton(display, "SET TIME", buttonArea, buttonHover);
        
        // Display current system time
        std::time_t current = std::time(nullptr);
        std::tm* local = std::localtime(&current);
        std::stringstream currentTimeText;
        currentTimeText << "Current System Time: " << std::put_time(local, "%Y-%m-%d %H:%M:%S");
        cv::putText(display, currentTimeText.str(), cv::Point(50, 80), 
                   cv::FONT_HERSHEY_SIMPLEX, 0.7, cv::Scalar(0, 0, 255), 2);
        
        cv::imshow(winName, display);
        
        if (okClicked) {
            // Prepare new time
            std::tm newTime = {0};
            newTime.tm_year = year - 1900;
            newTime.tm_mon = month - 1;
            newTime.tm_mday = day;
            newTime.tm_hour = hour;
            newTime.tm_min = minute;
            newTime.tm_sec = second;
            newTime.tm_isdst = -1;
            
            bool timeSetSuccess = setSystemTime(newTime);
            
            // Show appropriate message
            cv::Mat message(150, 400, CV_8UC3, timeSetSuccess ? cv::Scalar(200, 255, 200) : cv::Scalar(200, 200, 255));
            std::string statusText = timeSetSuccess ? "Time Set Successfully!" : "Failed to Set Time!";
            cv::Scalar textColor = timeSetSuccess ? cv::Scalar(0, 100, 0) : cv::Scalar(0, 0, 100);
            
            std::stringstream msg;
            msg << (timeSetSuccess ? "System time updated to: " : "Error: ") 
                << std::put_time(&newTime, "%Y-%m-%d %H:%M:%S");
            
            cv::putText(message, statusText, cv::Point(50, 50), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.7, textColor, 2);
            cv::putText(message, msg.str(), cv::Point(20, 90), 
                       cv::FONT_HERSHEY_SIMPLEX, 0.5, textColor, 1);
            
            cv::imshow(timeSetSuccess ? "Success" : "Error", message);
            cv::waitKey(2000);
            cv::destroyWindow(timeSetSuccess ? "Success" : "Error");
            
            okClicked = false;
        }
        
        if (cv::waitKey(30) == 27) { // ESC key
            break;
        }
    }
    
    cv::destroyAllWindows();
    return 0;
}