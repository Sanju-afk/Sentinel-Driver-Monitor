#pragma once
#include <string>
#include <opencv2/opencv.hpp>

enum class AlertType {
    DROWSINESS,
    CRASH_SOS
};

/**
 * @brief Centralises all outbound alerts.
 *
 *  Drowsiness alert  →  multipart POST (image) to cloud dashboard REST API.
 *  Crash SOS         →  same image POST **plus** Twilio SMS to the emergency
 *                        contact with G-force reading and GPS co-ordinates
 *                        (if gpsd is running).
 *
 *  Both network calls run in detached threads so they never block the
 *  main vision loop.
 */
class AlertManager {
public:
    /**
     * @param api_base_url  Root URL of the cloud dashboard, e.g. "http://192.168.1.10:5000".
     * @param twilio_sid    Twilio Account SID.
     * @param twilio_token  Twilio Auth Token.
     * @param from_phone    Twilio source phone number  (E.164 format).
     * @param to_phone      Emergency contact phone number (E.164 format).
     */
    AlertManager(const std::string& api_base_url,
                 const std::string& twilio_sid,
                 const std::string& twilio_token,
                 const std::string& from_phone,
                 const std::string& to_phone);

    /**
     * @brief Fire-and-forget: POST frame to /api/report-incident as DROWSINESS.
     */
    void sendDrowsinessAlert(const cv::Mat& frame);

    /**
     * @brief Fire-and-forget: POST frame to /api/report-incident as CRASH,
     *        then send an SMS SOS via Twilio.
     * @param gforce    Peak G-force magnitude that triggered the crash.
     * @param location  Optional "lat,lon" string (empty → omitted from SMS).
     */
    void sendSOS(const cv::Mat& frame, float gforce,
                 const std::string& location = "");

private:
    std::string api_base_url_;
    std::string twilio_sid_;
    std::string twilio_token_;
    std::string from_phone_;
    std::string to_phone_;

    // ── Internal helpers (run inside detached threads) ───────────────────
    void postImageToCloud(const cv::Mat& frame, AlertType type,
                          const std::string& metadata);
    void sendSMS(const std::string& message) const;

    // libcurl write-callback for response body capture
    static size_t curlWriteCallback(void* ptr, size_t size,
                                    size_t nmemb, std::string* out);
};
