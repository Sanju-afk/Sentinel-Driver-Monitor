# 👁️ Sentinel: AI Driver Drowsiness Monitor

**Sentinel** is a high-performance, real-time edge AI system designed to detect driver drowsiness and distraction. Built with **C++**, **OpenCV**, and **TensorFlow Lite**, it is optimized for low-latency inference on edge devices (Linux/Raspberry Pi).

![C++](https://img.shields.io/badge/C++-17-blue.svg) ![OpenCV](https://img.shields.io/badge/OpenCV-4.x-green.svg) ![TFLite](https://img.shields.io/badge/TensorFlow%20Lite-2.x-orange.svg)

## 🚀 Features
* **Real-time Inference:** Uses optimized TFLite models for face tracking and state classification.
* **Dual Input Modes:** Supports live webcam feed or pre-recorded video files for testing.
* **Edge Optimized:** Built with XNNPACK and AVX optimizations for CPU efficiency.
* **Robust Detection:** Monitors eye closure (EAR) and yawning to trigger alerts (Normal vs. Drowsy).

## 🛠️ Prerequisites
Before building, ensure you have the following installed on your Linux machine:
* **C++ Compiler** (GCC/G++ supporting C++17)
* **CMake** (v3.10+)
* **OpenCV** (v4.0+ with GStreamer support recommended)
* **TensorFlow Lite** (Static libraries linked via CMake)

## 🏗️ Build Instructions

1.  **Clone the repository:**
    ```bash
    git clone [https://github.com/YOUR_USERNAME/Sentinel-Driver-Monitor.git](https://github.com/YOUR_USERNAME/Sentinel-Driver-Monitor.git)
    cd Sentinel-Driver-Monitor
    ```

2.  **Create a build directory:**
    ```bash
    cd edge_device
    mkdir build && cd build
    ```

3.  **Compile with CMake:**
    ```bash
    cmake ..
    make -j$(nproc)
    ```

## 💻 Usage

Run the compiled executable from the `build` directory.

### 1. Live Webcam Mode
By default, Sentinel will attempt to open the primary webcam (Index 0).
```bash
./Sentinel

2. Video File Mode

To test with a dataset or video file, pass the file path as an argument:
Bash

./Sentinel ../../data/test_video.mp4


📂 Project Structure
Plaintext

Sentinel-Driver-Monitor/
├── edge_device/
│   ├── src/           # C++ Source code (main.cpp, InferenceEngine.cpp)
│   ├── include/       # Header files
│   ├── CMakeLists.txt # Build configuration
│   └── build/         # Compiled binaries (ignored by Git)
├── scripts/           # Python utilities (e.g., image-to-video stitching)
├── models/            # TFLite models (.tflite)
└── README.md          # Project documentation


🔧 Troubleshooting

    "Undefined reference to ruy or pthreadpool": Ensure you have linked all TFLite dependencies in CMakeLists.txt (XNNPACK, cpuinfo, ruy).

    Camera not opening: Check your video device index (/dev/video0 vs /dev/video2) or ensure no other process is using the webcam.

    "Failed to allocate required memory": If using GStreamer/OpenCV, try restarting the camera or killing background Python processes (sudo killall python3).

🤝 Contributing

Contributions are welcome! Please fork the repository and submit a pull request.

    Fork the Project

    Create your Feature Branch (git checkout -b feature/AmazingFeature)

    Commit your Changes (git commit -m 'Add some AmazingFeature')

    Push to the Branch (git push origin feature/AmazingFeature)

    Open a Pull Request
