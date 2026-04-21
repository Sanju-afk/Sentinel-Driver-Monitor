#!/usr/bin/env bash
# ─────────────────────────────────────────────────────────────────────────────
# Sentinel — Edge Device Setup Script
# Tested on: Raspberry Pi OS (Bullseye/Bookworm), Ubuntu 22.04
# ─────────────────────────────────────────────────────────────────────────────
set -euo pipefail

echo "=== Sentinel Setup ==="

# ── System packages ───────────────────────────────────────────────────────────
sudo apt-get update -qq
sudo apt-get install -y \
    build-essential cmake git \
    libopencv-dev \
    libcurl4-openssl-dev \
    libi2c-dev i2c-tools \
    python3-pip python3-venv

# ── Enable I²C (Raspberry Pi only) ───────────────────────────────────────────
if command -v raspi-config &>/dev/null; then
    echo "[setup] Enabling I²C via raspi-config…"
    sudo raspi-config nonint do_i2c 0
    echo "[setup] I²C enabled. Reboot may be required."
    echo "[setup] Verify MPU-6050 with: sudo i2cdetect -y 1  (should show 0x68)"
fi

# ── TensorFlow Lite C++ library ───────────────────────────────────────────────
# We download the pre-built shared library for the host arch.
ARCH=$(uname -m)
TFLITE_VERSION="2.14.0"

case "${ARCH}" in
  aarch64) TFLITE_PKG="tflite_runtime-${TFLITE_VERSION}-cp311-cp311-linux_aarch64.whl" ;;
  armv7l)  TFLITE_PKG="tflite_runtime-${TFLITE_VERSION}-cp311-cp311-linux_armv7l.whl"  ;;
  x86_64)  TFLITE_PKG="tflite_runtime-${TFLITE_VERSION}-cp311-cp311-linux_x86_64.whl"  ;;
  *)       echo "[setup] Unknown arch: ${ARCH}. Install TFLite manually."; exit 1 ;;
esac

echo "[setup] Downloading TFLite C++ headers from tensorflow/tensorflow…"
# Clone just the lite headers (shallow, no full repo)
if [ ! -d "/tmp/tflite_headers" ]; then
    git clone --depth 1 --filter=blob:none --sparse \
        https://github.com/tensorflow/tensorflow.git /tmp/tflite_headers
    (cd /tmp/tflite_headers && git sparse-checkout set tensorflow/lite)
fi
sudo cp -r /tmp/tflite_headers/tensorflow /usr/local/include/ 2>/dev/null || true

echo "[setup] Building TFLite flat-source library (this takes ~10 min on RPi 4)…"
if [ ! -f "/usr/local/lib/libtensorflowlite.so" ]; then
    pip3 install cmake --break-system-packages 2>/dev/null || pip3 install cmake
    git clone --depth 1 https://github.com/tensorflow/tensorflow.git /tmp/tensorflow_src || true
    mkdir -p /tmp/tflite_build && cd /tmp/tflite_build
    
 	# ---- FIX neon2sse BEFORE CMake runs ----
	sed -i 's/cmake_minimum_required.*/cmake_minimum_required(VERSION 3.5)/' \
	/tmp/tensorflow_src/tensorflow/lite/tools/cmake/modules/neon2sse/CMakeLists.txt 2>/dev/null || true

    cmake /tmp/tensorflow_src/tensorflow/lite \
    -DCMAKE_BUILD_TYPE=Release \
    -DTFLITE_ENABLE_GPU=OFF \
    -DTFLITE_ENABLE_XNNPACK=OFF
    make -j"$(nproc)"
    sudo cp /tmp/tflite_build/libtensorflowlite.so /usr/local/lib/
    sudo ldconfig
fi

# ── Python (cloud dashboard) ──────────────────────────────────────────────────
SCRIPT_DIR="$(cd "$(dirname "${BASH_SOURCE[0]}")" && pwd)"
DASHBOARD_DIR="${SCRIPT_DIR}/../cloud_dashboard"

if [ -d "${DASHBOARD_DIR}" ]; then
    echo "[setup] Creating Python venv for cloud dashboard…"
    python3 -m venv "${DASHBOARD_DIR}/.venv"
    # shellcheck disable=SC1091
    source "${DASHBOARD_DIR}/.venv/bin/activate"
    pip install --upgrade pip
    pip install flask flask-socketio pillow eventlet
    deactivate
    echo "[setup] Dashboard deps installed. To run:"
    echo "        source ${DASHBOARD_DIR}/.venv/bin/activate && python ${DASHBOARD_DIR}/app.py"
fi

# ── Twilio credentials reminder ───────────────────────────────────────────────
echo ""
echo "=== Twilio SOS Setup ==="
echo "  Export these before running Sentinel:"
echo "    export TWILIO_SID=<your_account_sid>"
echo "    export TWILIO_TOKEN=<your_auth_token>"
echo "    export TWILIO_FROM=<your_twilio_number>     # E.164 e.g. +14155551234"
echo "    export EMERGENCY_TO=<emergency_contact>     # E.164 e.g. +14155550000"
echo ""

# ── Build the C++ project ─────────────────────────────────────────────────────
BUILD_DIR="${SCRIPT_DIR}/../edge_device/build"
mkdir -p "${BUILD_DIR}"
cd "${BUILD_DIR}"

sed -i '1,10s/cmake_minimum_required(VERSION [0-9.]\+)/cmake_minimum_required(VERSION 3.5)/' \
/tmp/tflite_build/neon2sse/CMakeLists.txt 2>/dev/null || true
cmake .. -DCMAKE_BUILD_TYPE=Release
make -j"$(nproc)"

echo ""
echo "=== Build complete! ==="
echo "  Binary: ${BUILD_DIR}/sentinel"
echo "  Run:    ${BUILD_DIR}/sentinel             # webcam"
echo "          ${BUILD_DIR}/sentinel video.mp4   # video file"
