#include "AlertManager.h"
#include <curl/curl.h>
#include <iostream>
#include <thread>
#include <ctime>
#include <sstream>

// ─── Constructor ─────────────────────────────────────────────────────────────

AlertManager::AlertManager(const std::string& api_base_url,
                           const std::string& twilio_sid,
                           const std::string& twilio_token,
                           const std::string& from_phone,
                           const std::string& to_phone)
    : api_base_url_(api_base_url),
      twilio_sid_(twilio_sid),
      twilio_token_(twilio_token),
      from_phone_(from_phone),
      to_phone_(to_phone)
{}

// ─── Public API ──────────────────────────────────────────────────────────────

void AlertManager::sendDrowsinessAlert(const cv::Mat& frame) {
    // Detach: never block the vision loop
    std::thread([this, f = frame.clone()]() mutable {
        postImageToCloud(f, AlertType::DROWSINESS, "");
    }).detach();
}

void AlertManager::sendSOS(const cv::Mat& frame, float gforce,
                           const std::string& location) {
    std::thread([this, f = frame.clone(), gforce, location]() mutable {
        // 1) Post snapshot to dashboard
        std::ostringstream meta;
        meta << "gforce=" << gforce;
        if (!location.empty()) meta << "&location=" << location;
        postImageToCloud(f, AlertType::CRASH_SOS, meta.str());

        // 2) Send SMS
        std::ostringstream msg;
        msg << "[SENTINEL SOS] Vehicle crash detected!\n"
            << "Impact: " << gforce << "g\n";
        if (!location.empty())
            msg << "Location: https://maps.google.com/?q=" << location << "\n";
        else
            msg << "Location: unavailable\n";
        msg << "Emergency services may be needed.";

        sendSMS(msg.str());
    }).detach();
}

// ─── Private helpers ─────────────────────────────────────────────────────────

void AlertManager::postImageToCloud(const cv::Mat& frame,
                                    AlertType type,
                                    const std::string& metadata) {
    // Persist frame to a uniquely-named temp file
    std::string tmp = "/tmp/sentinel_" + std::to_string(std::time(nullptr)) + ".jpg";
    if (!cv::imwrite(tmp, frame)) {
        std::cerr << "[AlertManager] Could not write temp image: " << tmp << "\n";
        return;
    }

    const std::string typeStr  = (type == AlertType::CRASH_SOS) ? "crash" : "drowsiness";
    const std::string endpoint = api_base_url_ + "/api/report-incident";

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    if (curl) {
        std::string response_body;

        curl_mime*     form  = curl_mime_init(curl);
        curl_mimepart* field;

        // image
        field = curl_mime_addpart(form);
        curl_mime_name(field, "image");
        curl_mime_filedata(field, tmp.c_str());

        // type
        field = curl_mime_addpart(form);
        curl_mime_name(field, "type");
        curl_mime_data(field, typeStr.c_str(), CURL_ZERO_TERMINATED);

        // metadata (optional)
        if (!metadata.empty()) {
            field = curl_mime_addpart(form);
            curl_mime_name(field, "metadata");
            curl_mime_data(field, metadata.c_str(), CURL_ZERO_TERMINATED);
        }

        curl_easy_setopt(curl, CURLOPT_URL,           endpoint.c_str());
        curl_easy_setopt(curl, CURLOPT_MIMEPOST,      form);
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION, curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,     &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,       10L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            std::cout << "[AlertManager] " << typeStr
                      << " incident logged. Server: " << response_body << "\n";
        } else {
            std::cerr << "[AlertManager] POST failed: "
                      << curl_easy_strerror(res) << "\n";
        }

        curl_easy_cleanup(curl);
        curl_mime_free(form);
    }

    curl_global_cleanup();
    std::remove(tmp.c_str());
}

void AlertManager::sendSMS(const std::string& message) const {
    if (twilio_sid_.empty() || twilio_token_.empty()) {
        std::cerr << "[AlertManager] Twilio credentials not set — SMS skipped.\n";
        return;
    }

    const std::string url = "https://api.twilio.com/2010-04-01/Accounts/"
                            + twilio_sid_ + "/Messages.json";

    curl_global_init(CURL_GLOBAL_ALL);
    CURL* curl = curl_easy_init();

    if (curl) {
        std::string response_body;
        std::string postFields =
            "To="   + to_phone_   +
            "&From=" + from_phone_ +
            "&Body=" + curl_easy_escape(curl, message.c_str(),
                                        static_cast<int>(message.size()));

        curl_easy_setopt(curl, CURLOPT_URL,            url.c_str());
        curl_easy_setopt(curl, CURLOPT_POSTFIELDS,     postFields.c_str());
        curl_easy_setopt(curl, CURLOPT_USERNAME,       twilio_sid_.c_str());
        curl_easy_setopt(curl, CURLOPT_PASSWORD,       twilio_token_.c_str());
        curl_easy_setopt(curl, CURLOPT_WRITEFUNCTION,  curlWriteCallback);
        curl_easy_setopt(curl, CURLOPT_WRITEDATA,      &response_body);
        curl_easy_setopt(curl, CURLOPT_TIMEOUT,        15L);

        CURLcode res = curl_easy_perform(curl);
        if (res == CURLE_OK) {
            std::cout << "[AlertManager] SOS SMS sent successfully.\n";
        } else {
            std::cerr << "[AlertManager] SMS failed: "
                      << curl_easy_strerror(res) << "\n";
        }
        curl_easy_cleanup(curl);
    }
    curl_global_cleanup();
}

size_t AlertManager::curlWriteCallback(void* ptr, size_t size,
                                       size_t nmemb, std::string* out) {
    out->append(static_cast<char*>(ptr), size * nmemb);
    return size * nmemb;
}
