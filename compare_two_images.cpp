/*
 * motion detection 
*/
#include <opencv2/opencv.hpp>   
#include <vector>  
#include <iostream>  

cv::VideoCapture cap;  
cv::Mat previous_frame;  
cv::Mat result;  
const int cvMatlevels = 4;  
class ImageComparator {  
    cv::Mat base;  
    cv::Mat target;  
    int thresh;  // Renamed from threshold to avoid conflict with OpenCV function  
    void findBoundingBoxes(cv::Mat& diff, std::vector<cv::Rect>& boxes) {  
        cv::Mat thresholded;  
        cvtColor(diff, diff, cv::COLOR_BGR2GRAY);  // Convert to grayscale  
        cv::threshold(diff, thresholded, thresh, 255, cv::THRESH_BINARY);  // Apply binary threshold  
        // Find contours of the thresholded image  
        std::vector<std::vector<cv::Point>> contours;  
        findContours(thresholded, contours, cv::RETR_EXTERNAL, cv::CHAIN_APPROX_SIMPLE);  
        // Get bounding boxes for each contour  
        for (const auto& contour : contours) {  
            cv::Rect box = boundingRect(contour);  
            if (box.area() > 10) {  // Filter out small noise  
                boxes.push_back(box);  
            }  
        }  
    }  
public:  
    ImageComparator(const cv::Mat& base, const cv::Mat& target, int threshold)  
        : base(base.clone()), target(target.clone()), thresh(threshold) {  
        CV_Assert(base.size() == target.size() && base.type() == target.type());  
    }  
    cv::Mat findChanges() {  
        // Compute absolute difference between the two images  
        cv::Mat diff;  
        cv::absdiff(base, target, diff);  
        // Find bounding boxes around changed regions  
        std::vector<cv::Rect> boxes;  
        findBoundingBoxes(diff, boxes);  
        // Draw rectangles on the target image  
        cv::Mat result = target.clone();  
        for (const auto& box : boxes) {  
            rectangle(result, box, cv::Scalar(0, 255, 0), 2);  // Green rectangle  
        }  
        return result;  
    }  
};  
void start_webcam(){  
    if (!cap.isOpened()) {  
        std::cerr << "Error: Could not open video stream." << std::endl;  
        return;  
    }  
    // Create a named window and set it to full screen  
    cv::namedWindow("Face Detection", cv::WINDOW_NORMAL);  
    //cv::setWindowProperty("Face Detection", cv::WND_PROP_FULLSCREEN, cv::WINDOW_FULLSCREEN);  

    cv::Mat frame;  
    try {  
        while (true) {  
            cap >> frame;  
            if (frame.empty()) {  
                std::cerr << "Error: Empty frame." << std::endl;  
                continue;  
            }  
            cv::Mat low_quality_frame = frame / (256 / cvMatlevels) * (256 / cvMatlevels);  
            // Resize the frame to fit the wxStaticBitmap  
            cv::Mat resized_frame;  
            cv::resize(low_quality_frame, resized_frame, cv::Size(640, 320));//0.5, 0.5  
            if(!previous_frame.empty()){  
                ImageComparator comparator(previous_frame, resized_frame, 30);   
                result = comparator.findChanges();  
            }  
            else{
                result = resized_frame.clone();
            }
            previous_frame = resized_frame.clone();  
            cv::imshow("Face Detection", result);  
            char key = cv::waitKey(30);  
            if (key == 'q') {  
                break;  
            }  
        }  
        cap.release();  
        cv::destroyAllWindows();  
    } catch (const std::exception& ex) {  
        std::cerr << ex.what() << std::endl;  
    } catch (...) {  
        std::cerr << "Unknown errors" << std::endl;  
    }  
}  
int main() {  
    int webcamID = 1;  
    if (!cap.open(webcamID)) {  
        std::cerr << "Error: Unable to open the webcam!" << std::endl;  
        return -1;  
    }  
    std::cout << "Webcam opened successfully. Starting webcam feed..." << std::endl;  
    start_webcam();  
    return 0;  
}
