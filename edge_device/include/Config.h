#pragma once
#include <string>

// ─── Model & Assets ────────────────────────────────────────────────────────────
namespace Config {

    // Paths
    inline constexpr const char* MODEL_PATH        = "../assets/drowsiness_model.tflite";
    inline constexpr const char* CASCADE_PATH      = "../assets/haarcascade_frontalface_default.xml";

    // Drowsiness Detection
    inline constexpr float  DROWSY_THRESHOLD       = 0.70f;   // confidence to flag as drowsy
    inline constexpr double WARNING_TIME_SECONDS   = 1.5;     // grace period before alert
    inline constexpr int    INPUT_WIDTH             = 224;
    inline constexpr int    INPUT_HEIGHT            = 224;

    // Crash Detection (MPU6050)
    inline constexpr int   IMU_I2C_BUS             = 1;       // /dev/i2c-1 on Raspberry Pi
    inline constexpr float CRASH_G_THRESHOLD       = 3.5f;   // sudden deceleration in g's
    inline constexpr int   IMU_POLL_RATE_MS        = 10;     // sample every 10 ms

    // Cloud API
    inline constexpr const char* API_BASE_URL      = "http://localhost:5000";
    inline constexpr const char* INCIDENT_ENDPOINT = "/api/report-incident";

    // Twilio SOS (set via env or replace here)
    inline const std::string TWILIO_SID    = std::getenv("TWILIO_SID")    ? std::getenv("TWILIO_SID")    : "";
    inline const std::string TWILIO_TOKEN  = std::getenv("TWILIO_TOKEN")  ? std::getenv("TWILIO_TOKEN")  : "";
    inline const std::string TWILIO_FROM   = std::getenv("TWILIO_FROM")   ? std::getenv("TWILIO_FROM")   : "+10000000000";
    inline const std::string EMERGENCY_TO  = std::getenv("EMERGENCY_TO")  ? std::getenv("EMERGENCY_TO")  : "+10000000001";

    // GPS (gpsd) — optional; leave empty to omit from SOS
    inline constexpr const char* GPSD_HOST = "localhost";
    inline constexpr int         GPSD_PORT = 2947;
}
