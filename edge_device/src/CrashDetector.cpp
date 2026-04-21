#include "CrashDetector.h"
#include "Config.h"

#include <iostream>
#include <stdexcept>
#include <thread>
#include <chrono>
#include <cstring>

// Linux I²C headers
#include <fcntl.h>
#include <unistd.h>
#include <sys/ioctl.h>
#include <linux/i2c-dev.h>
#include <linux/i2c.h>

// ─── Constructor / Destructor ────────────────────────────────────────────────

CrashDetector::CrashDetector(float threshold_g, int i2c_bus)
    : threshold_g_(threshold_g), i2c_bus_(i2c_bus) {}

CrashDetector::~CrashDetector() {
    stop();
    if (i2c_fd_ >= 0) {
        ::close(i2c_fd_);
        i2c_fd_ = -1;
    }
}

// ─── Public API ──────────────────────────────────────────────────────────────

bool CrashDetector::initialize() {
    if (!openI2C()) return false;

    // Wake MPU-6050: clear SLEEP bit in PWR_MGMT_1
    writeRegister(REG_PWR_MGMT, 0x00);

    // Set accelerometer range to ±2g (ACCEL_CFG bits [4:3] = 00)
    writeRegister(REG_ACCEL_CFG, 0x00);

    std::cout << "[CrashDetector] MPU-6050 initialised on /dev/i2c-"
              << i2c_bus_ << " (threshold: " << threshold_g_ << "g)\n";
    return true;
}

void CrashDetector::start(CrashCallback callback) {
    if (running_.load()) return;
    callback_ = std::move(callback);
    running_  = true;
    monitor_thread_ = std::thread(&CrashDetector::monitorLoop, this);
    std::cout << "[CrashDetector] Monitoring thread started.\n";
}

void CrashDetector::stop() {
    running_ = false;
    if (monitor_thread_.joinable()) monitor_thread_.join();
}

// ─── Raw accelerometer read ───────────────────────────────────────────────────

AccelData CrashDetector::readAccel() const {
    if (i2c_fd_ < 0) return {};

    // MPU-6050 outputs 6 bytes starting at 0x3B (big-endian, two-byte pairs)
    uint8_t buf[6] = {};
    uint8_t reg = REG_ACCEL_OUT;

    // Write register address, then read 6 bytes
    struct i2c_msg msgs[2] = {
        { MPU6050_ADDR, 0,        1, &reg  },  // write reg pointer
        { MPU6050_ADDR, I2C_M_RD, 6, buf   }   // read 6 bytes
    };
    struct i2c_rdwr_ioctl_data packet = { msgs, 2 };

    if (::ioctl(i2c_fd_, I2C_RDWR, &packet) < 0) {
        // Transient read failures are non-fatal; return zeros
        return {};
    }

    auto to_int16 = [](uint8_t hi, uint8_t lo) -> int16_t {
        return static_cast<int16_t>((hi << 8) | lo);
    };

    AccelData d;
    d.x = static_cast<float>(to_int16(buf[0], buf[1])) / ACCEL_SCALE;
    d.y = static_cast<float>(to_int16(buf[2], buf[3])) / ACCEL_SCALE;
    d.z = static_cast<float>(to_int16(buf[4], buf[5])) / ACCEL_SCALE;
    return d;
}

// ─── Private helpers ─────────────────────────────────────────────────────────

bool CrashDetector::openI2C() {
    std::string path = "/dev/i2c-" + std::to_string(i2c_bus_);
    i2c_fd_ = ::open(path.c_str(), O_RDWR);
    if (i2c_fd_ < 0) {
        std::cerr << "[CrashDetector] Cannot open " << path
                  << ": " << std::strerror(errno) << "\n";
        return false;
    }
    if (::ioctl(i2c_fd_, I2C_SLAVE, MPU6050_ADDR) < 0) {
        std::cerr << "[CrashDetector] Cannot set I2C slave address 0x"
                  << std::hex << static_cast<int>(MPU6050_ADDR) << "\n";
        ::close(i2c_fd_);
        i2c_fd_ = -1;
        return false;
    }
    return true;
}

void CrashDetector::writeRegister(uint8_t reg, uint8_t value) const {
    uint8_t buf[2] = { reg, value };
    if (::write(i2c_fd_, buf, 2) != 2) {
        std::cerr << "[CrashDetector] Register write failed (reg=0x"
                  << std::hex << static_cast<int>(reg) << ")\n";
    }
}

void CrashDetector::monitorLoop() {
    constexpr auto POLL_INTERVAL =
        std::chrono::milliseconds(Config::IMU_POLL_RATE_MS);

    // Simple peak-detector with a one-sample cooldown to avoid duplicate events
    bool inCrashEvent = false;

    while (running_.load()) {
        AccelData d = readAccel();
        float mag   = d.magnitude();

        if (mag >= threshold_g_ && !inCrashEvent) {
            inCrashEvent = true;
            std::cout << "[CrashDetector] CRASH DETECTED! G = " << mag << "\n";
            if (callback_) callback_(d);
        } else if (mag < threshold_g_ * 0.5f) {
            // Hysteresis: only reset once forces drop well below threshold
            inCrashEvent = false;
        }

        std::this_thread::sleep_for(POLL_INTERVAL);
    }

    std::cout << "[CrashDetector] Monitoring thread stopped.\n";
}
