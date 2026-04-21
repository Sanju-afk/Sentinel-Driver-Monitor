#pragma once
#include <atomic>
#include <thread>
#include <functional>
#include <cmath>

/**
 * @brief Raw 3-axis accelerometer reading in g-force units.
 */
struct AccelData {
    float x = 0.f, y = 0.f, z = 0.f;

    float magnitude() const {
        return std::sqrt(x * x + y * y + z * z);
    }
};

/**
 * @brief Monitors an MPU-6050 IMU over Linux I²C and fires a callback
 *        when a sudden-impact G-force spike exceeds the configured threshold.
 *
 *  Hardware wiring (Raspberry Pi 4 / Jetson Nano):
 *    VCC → 3.3 V    GND → GND
 *    SDA → GPIO 2   SCL → GPIO 3   (I²C bus 1)
 *    AD0 → GND  →  I²C address 0x68
 *
 *  Enable I²C on Raspberry Pi:
 *    sudo raspi-config → Interface Options → I2C → Enable
 *    sudo apt install i2c-tools && sudo i2cdetect -y 1   (should show 0x68)
 */
class CrashDetector {
public:
    /**
     * @param threshold_g  G-force magnitude that triggers a crash event (default 3.5g).
     * @param i2c_bus      Linux I²C bus number (1 = /dev/i2c-1).
     */
    explicit CrashDetector(float threshold_g = 3.5f, int i2c_bus = 1);
    ~CrashDetector();

    // Non-copyable — owns OS file descriptor
    CrashDetector(const CrashDetector&)            = delete;
    CrashDetector& operator=(const CrashDetector&) = delete;

    using CrashCallback = std::function<void(AccelData)>;

    /**
     * @brief Open I²C device and wake the MPU-6050.
     * @return true on success.
     */
    bool initialize();

    /**
     * @brief Start the background polling thread.
     * @param callback  Invoked (from the polling thread) on crash detection.
     */
    void start(CrashCallback callback);

    /** @brief Signal the polling thread to stop and join it. */
    void stop();

    /** @brief Perform a single synchronous accelerometer read (all three axes). */
    AccelData readAccel() const;

    bool isRunning() const { return running_.load(); }

private:
    // ── MPU-6050 register map ───────────────────────────────────────────────
    static constexpr uint8_t MPU6050_ADDR  = 0x68;
    static constexpr uint8_t REG_PWR_MGMT  = 0x6B;  // Power management
    static constexpr uint8_t REG_ACCEL_CFG = 0x1C;  // Accelerometer config
    static constexpr uint8_t REG_ACCEL_OUT = 0x3B;  // First output register
    // ±2 g range → 1 g = 16 384 LSB
    static constexpr float   ACCEL_SCALE   = 16384.0f;

    int   i2c_fd_      = -1;
    int   i2c_bus_;
    float threshold_g_;

    std::atomic<bool> running_{false};
    std::thread       monitor_thread_;
    CrashCallback     callback_;

    bool openI2C();
    void writeRegister(uint8_t reg, uint8_t value) const;
    void monitorLoop();
};
