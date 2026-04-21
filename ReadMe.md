<div align="center">

<h1>🛡 Sentinel</h1>
<p><strong>Intelligent Driver Safety System</strong></p>
<p>Real-time drowsiness detection and crash SOS on edge hardware — no cloud dependency for inference.</p>

<p>
  <img src="https://img.shields.io/badge/C%2B%2B-17-blue?logo=c%2B%2B" alt="C++17"/>
  <img src="https://img.shields.io/badge/TensorFlow_Lite-2.x-orange?logo=tensorflow" alt="TFLite"/>
  <img src="https://img.shields.io/badge/OpenCV-4.x-green?logo=opencv" alt="OpenCV"/>
  <img src="https://img.shields.io/badge/Platform-Raspberry_Pi_4-red?logo=raspberrypi" alt="RPi4"/>
  <img src="https://img.shields.io/badge/License-MIT-lightgrey" alt="MIT"/>
</p>

</div>

---

## What It Does

Sentinel runs entirely on a Raspberry Pi 4 mounted inside a vehicle. It watches the driver's face through a forward-facing USB or CSI camera and simultaneously polls an MPU-6050 accelerometer for sudden G-force spikes.

| Event | Response |
|-------|----------|
| Drowsiness detected | On-screen warning → escalates to full alert after 1.5 s → posts snapshot to cloud dashboard |
| Crash detected (≥ 3.5 g) | Immediately POSTs snapshot to cloud + fires Twilio SMS SOS to emergency contact |

All ML inference runs locally. No camera frames leave the device unless an incident occurs.

---

## Architecture Overview

```
Camera ──► Face Detection ──► TFLite Inference ──► Alert Logic ──► AlertManager
                                                                         │
MPU-6050 (I²C) ──► CrashDetector ──────────────────────────────────────►│
                                                                         │
                                                              ┌──────────┴──────────┐
                                                              │  Cloud Dashboard    │
                                                              │  (Flask + WS)       │
                                                              └─────────────────────┘
                                                                         │
                                                                  Twilio SMS ──► Emergency Contact
```

See [`ARCHITECTURE.docx`](./ARCHITECTURE.docx) for the full system design, circuit diagrams, and data flow breakdown.

---

## Hardware Requirements

| Component | Purpose | Notes |
|-----------|---------|-------|
| Raspberry Pi 4 (2 GB+) | Main compute | Also works on Jetson Nano |
| USB / CSI Camera | Driver-facing video stream | 720p+ recommended |
| MPU-6050 breakout | 3-axis accelerometer | ~$2 on most electronics sites |
| 4× female–female jumper wires | RPi GPIO ↔ MPU-6050 | — |

**MPU-6050 wiring:**

```
MPU-6050    Raspberry Pi 4
─────────   ───────────────
VCC     →   Pin 1  (3.3 V)
GND     →   Pin 6  (GND)
SDA     →   GPIO 2 (Pin 3)
SCL     →   GPIO 3 (Pin 5)
AD0     →   GND         ← sets I²C address to 0x68
```

Enable I²C: `sudo raspi-config` → Interface Options → I2C → Enable
Verify: `sudo i2cdetect -y 1` — should show `68` at address 0x68.

---

## Project Structure

```
sentinel/
├── edge_device/
│   ├── include/
│   │   ├── Config.h            # All tuneable constants — edit here first
│   │   ├── InferenceEngine.h   # TFLite model wrapper
│   │   ├── CrashDetector.h     # MPU-6050 I²C + crash-event logic
│   │   └── AlertManager.h      # Cloud POST + Twilio SMS
│   ├── src/
│   │   ├── main.cpp            # Vision loop orchestration
│   │   ├── InferenceEngine.cpp
│   │   ├── CrashDetector.cpp
│   │   └── AlertManager.cpp
│   ├── assets/                 # .tflite model + haar cascade XML
│   └── CMakeLists.txt
├── cloud_dashboard/
│   ├── app.py                  # Flask + Socket.IO REST + WebSocket backend
│   └── templates/index.html    # Live dashboard UI (no framework)
├── data/dataset/
│   ├── alert/                  # Training images — alert driver
│   └── drowsy/                 # Training images — drowsy driver
└── scripts/
    ├── train_model.py          # MobileNetV2 fine-tune → TFLite export
    └── setup.sh                # One-shot dependency installer + CMake build
```

---

## Quickstart

### 1. Install dependencies and build

```bash
chmod +x scripts/setup.sh
./scripts/setup.sh
```

This installs OpenCV, libcurl, libi2c, builds TFLite, and compiles the C++ binary.

### 2. Train the model

Populate `data/dataset/alert/` and `data/dataset/drowsy/` with face-crop images, then:

```bash
# Float32 export (default)
python scripts/train_model.py --epochs 20

# + INT8 quantisation — smaller and faster on RPi
python scripts/train_model.py --epochs 20 --quant
```

The trained model is written directly to `edge_device/assets/drowsiness_model.tflite`.

### 3. Start the cloud dashboard

```bash
cd cloud_dashboard
source .venv/bin/activate
python app.py
# Open http://<your-pi-ip>:5000 in a browser
```

### 4. Configure Twilio SOS credentials

```bash
export TWILIO_SID=ACxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxxx
export TWILIO_TOKEN=your_auth_token
export TWILIO_FROM=+14155551234     # your Twilio number
export EMERGENCY_TO=+14155550000   # emergency contact
```

### 5. Run Sentinel

```bash
cd edge_device/build
./sentinel                  # live webcam (default device 0)
./sentinel /dev/video1      # specific camera
./sentinel dashcam.mp4      # video file (loops for testing)
```

Press `ESC` to quit.

---

## Configuration

All tunable parameters live in `include/Config.h`. No external config files — edit and `make`.

| Parameter | Default | Description |
|-----------|---------|-------------|
| `DROWSY_THRESHOLD` | `0.70` | Minimum confidence score to classify a face as drowsy |
| `WARNING_TIME_SECONDS` | `1.5` | Grace period before escalating to a cloud alert |
| `CRASH_G_THRESHOLD` | `3.5` | G-force magnitude (all axes) that declares a crash |
| `IMU_POLL_RATE_MS` | `10` | Accelerometer sampling interval in milliseconds |
| `IMU_I2C_BUS` | `1` | Linux I²C bus number (`/dev/i2c-1`) |
| `INPUT_WIDTH / HEIGHT` | `224` | TFLite model input resolution |
| `API_BASE_URL` | `http://localhost:5000` | Cloud dashboard root URL |

---

## Tech Stack

| Layer | Technology |
|-------|-----------|
| Computer Vision | OpenCV 4 |
| ML Inference | TensorFlow Lite 2.x |
| Model Architecture | MobileNetV2 (transfer learning, fine-tuned) |
| Crash Sensing | MPU-6050 via Linux `i2c-dev` kernel interface |
| HTTP / Multipart Upload | libcurl |
| SMS SOS | Twilio REST API |
| Cloud Backend | Python 3 · Flask · Flask-SocketIO |
| Dashboard Frontend | Vanilla JS + CSS (no framework, Socket.IO) |
| Build System | CMake 3.16+ |
| Language Standard | C++17 |

---

## How the ML Model Works

A **MobileNetV2** backbone (pre-trained on ImageNet) is fine-tuned in two phases:

1. **Feature extraction** — backbone frozen, only the classification head trains (Adam 1e-3, early stopping).
2. **Fine-tuning** — top 30 layers of the backbone unfrozen (Adam 1e-5, lower learning rate to avoid catastrophic forgetting).

Data augmentation (horizontal flip, rotation ±10°, zoom ±10%, brightness ±15%) is applied online during training. The exported `.tflite` model takes a `224×224×3` float32 RGB tensor and returns `[alert_score, drowsy_score]` (softmax).

---

## License

MIT — see `LICENSE`.
