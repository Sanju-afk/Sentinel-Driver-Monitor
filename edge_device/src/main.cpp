#include <opencv2/opencv.hpp>
#include <iostream>
#include <chrono>
#include <thread>
#include <curl/curl.h> // Fixed include
#include "InferenceEngine.h"

// CONFIG
const float DROWSY_THRESHOLD = 0.70f;
const double TIME_LIMIT_SECONDS = 1.5;
const std::string MODEL_PATH = "../assets/drowsiness_model.tflite";
const std::string API_URL = "http://localhost:5000/api/report-incident";

bool alarmActive = false;
auto startDrowsyTime = std::chrono::steady_clock::now();
bool timerRunning = false;

void sendAlert(cv::Mat img) {
    // Clone the image so the background thread has its own private copy
    std::thread([img = img.clone()]() {
        CURL* curl;
        CURLcode res;
        
        // Save to a unique filename to avoid collisions
        std::string filename = "/tmp/alert_" + std::to_string(time(0)) + ".jpg";
        cv::imwrite(filename, img);
        
        curl_global_init(CURL_GLOBAL_ALL);
        curl = curl_easy_init();
        
        if (curl) {
            curl_mime* form = curl_mime_init(curl);
            curl_mimepart* field = curl_mime_addpart(form);
            curl_mime_name(field, "image");
            curl_mime_filedata(field, filename.c_str());

            curl_easy_setopt(curl, CURLOPT_URL, API_URL.c_str());
            curl_easy_setopt(curl, CURLOPT_MIMEPOST, form);
            
            res = curl_easy_perform(curl);
            
            if (res == CURLE_OK) std::cout << "[CLOUD] Incident logged!" << std::endl;
            else std::cout << "[CLOUD] Failed: " << curl_easy_strerror(res) << std::endl;
            
            curl_easy_cleanup(curl);
            curl_mime_free(form);
        }
        curl_global_cleanup();
    }).detach();
}

int main(int argc, char* argv[]) {
    InferenceEngine engine;
    if (engine.loadModel(MODEL_PATH) != 0){
    	std::cout << "MODEL LOADING FAILED!"<<std::endl;
    	return -1;
    }

    cv::VideoCapture cap; 
    if (argc > 1) cap.open(argv[1]); //opens the video file
    else cap.open(0); //opens webcam

    if (!cap.isOpened()) {
        std::cerr << "Could not open source" << std::endl;
        return -1;
    }

    cv::CascadeClassifier faceCascade;
    if(!faceCascade.load("../assets/haarcascade_frontalface_default.xml")) {
        std::cerr << "Cascade load failed!" << std::endl;
        return -1;
    }

    cv::Mat frame;
    std::cout << "Sentinel SYSTEM ACTIVE" << std::endl;

    while (true) {
        cap >> frame;
        if (frame.empty()){
        	cap.set(cv::CAP_PROP_POS_FRAMES, 0); // Loop video
            	continue;
        }

        std::vector<cv::Rect> faces; 
        cv::Mat gray;
        cv::cvtColor(frame, gray, cv::COLOR_BGR2GRAY); // Fixed: COLOR_BGR2GRAY
        faceCascade.detectMultiScale(gray, faces, 1.1, 4);

        bool frameHasDrowsyFace = false;

        for (auto& face : faces) {
            cv::Mat faceROI = frame(face);
            std::vector<float> results = engine.runInference(faceROI);
            
            if(results.empty()) continue;
            float drowsyScore = results[1];

            if (drowsyScore > DROWSY_THRESHOLD) {
                frameHasDrowsyFace = true;
                if (!timerRunning) {
                    startDrowsyTime = std::chrono::steady_clock::now();
                    timerRunning = true;
                }

                auto now = std::chrono::steady_clock::now();
                std::chrono::duration<double> elapsed = now - startDrowsyTime;

                if (elapsed.count() > TIME_LIMIT_SECONDS) {
                    //exceeded safety, sending ALERT
                    cv::rectangle(frame, face, cv::Scalar(0,0,255), 3);
                    cv::putText(frame, "DROWSY!!", cv::Point(face.x, face.y - 10), 
                                cv::FONT_HERSHEY_SIMPLEX, 1, cv::Scalar(0,0,255), 3);
                    
                    if (!alarmActive) {
                        sendAlert(frame); 
                        alarmActive = true;
                    }
                } else {
                    //warning phase
                    cv::rectangle(frame, face, cv::Scalar(0,255,255), 2);
                    cv::putText(frame, "WARNING...", cv::Point(face.x, face.y - 10), 
                                cv::FONT_HERSHEY_SIMPLEX, 0.8, cv::Scalar(0,255,255), 2);
                }
            }
        }

        // Reset timer if eyes are open (no drowsy face found in this frame)
        if (!frameHasDrowsyFace) {
            timerRunning = false;
            alarmActive = false; 
        }

        cv::imshow("Sentinel Driver Monitor", frame);
        if (cv::waitKey(1) == 27) break; 
    }
    return 0;
}
