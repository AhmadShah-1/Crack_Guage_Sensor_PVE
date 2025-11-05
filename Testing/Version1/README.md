# ESP32-CAM Image Transfer Project

This project implements image capture and transmission using ESP32-CAM modules with two different communication protocols:
1. **ESP-NOW** - Direct peer-to-peer communication
2. **painlessMesh** - Mesh network communication

## Hardware Requirements

- ESP32-CAM AI-THINKER modules (or compatible)
- FTDI programmer or ESP32-CAM-MB programmer board
- Sufficient power supply (5V 2A recommended)

## Software Requirements

### Arduino IDE Setup
1. Install **ESP32 board support** version 2.0.x (NOT 3.x for painlessMesh)
   - File → Preferences → Additional Board Manager URLs
   - Add: `https://raw.githubusercontent.com/espressif/arduino-esp32/gh-pages/package_esp32_index.json`
   - Tools → Board → Boards Manager → Search "ESP32" → Install version 2.0.x

2. Install required libraries via Library Manager:
   - **painlessMesh** (for Mesh network version)
   - **ArduinoJson** (dependency for painlessMesh)
   - **TaskScheduler** (dependency for painlessMesh)

### Board Configuration
- **Board**: "AI Thinker ESP32-CAM"
- **Upload Speed**: 115200
- **Flash Frequency**: 80MHz
- **Flash Mode**: QIO
- **Partition Scheme**: "Huge APP (3MB No OTA/1MB SPIFFS)"
- **Core Debug Level**: "None" or "Error" for production

## Project Structure

```
Testing/
├── NOW/
│   ├── send_one_picture/          # ESP-NOW transmitter (ESP32-CAM)
│   └── receive_one_picture/       # ESP-NOW receiver (ESP32)
├── Mesh/
│   ├── send_one_picture/          # Mesh transmitter (ESP32-CAM)
│   └── receive_one_picture/       # Mesh receiver (ESP32)
└── README.md
```

## 1. ESP-NOW Implementation

### Features
- Fast, low-latency communication
- Direct peer-to-peer connection
- Suitable for point-to-point image transfer
- Maximum packet size: 250 bytes
- Automatic chunking and reassembly

### Setup Instructions

#### Step 1: Find MAC Addresses
1. Upload the `find_MAC-address.ino` to both ESP32 devices
2. Note down the MAC addresses from Serial Monitor
3. Update `receiverAddress` in `NOW/send_one_picture/send_one_picture.ino`

#### Step 2: Upload Code
1. **Receiver First**: Upload `receive_one_picture.ino` to the receiver ESP32
2. **Transmitter**: Upload `send_one_picture.ino` to the ESP32-CAM

#### Step 3: Configuration
Edit these parameters in the transmitter code if needed:
```cpp
uint8_t receiverAddress[] = {0xA4, 0xCF, 0x12, 0x9B, 0x7E, 0x23}; // Your receiver MAC
```

Adjust image quality in transmitter:
```cpp
config.frame_size = FRAMESIZE_VGA;  // Options: QVGA, VGA, SVGA, XGA
config.jpeg_quality = 12;            // 0-63 (lower = better quality)
```

### Expected Output

**Transmitter:**
```
ESP32-CAM Image Transmitter
============================
Camera initialized successfully
Transmitter MAC Address: A4:CF:12:3C:AF:10
ESP-NOW initialized
Peer registered successfully

--- Taking picture ---
Image captured: 23456 bytes
Sending 98 packets...
Progress: 20/98 packets sent
Progress: 40/98 packets sent
...
Transfer complete: 98/98 packets successful
```

**Receiver:**
```
ESP32 Image Receiver
====================
Receiver MAC Address: A4:CF:12:9B:7E:23

--- Starting image reception ---
Expected packets: 98
Estimated size: ~23520 bytes
Received: 20/98 packets (20.4%)
Received: 40/98 packets (40.8%)
...
=== Image Reception Complete ===
Total packets received: 98
Image size: 23456 bytes
Valid JPEG image received!
```

## 2. Mesh Network Implementation

### Features
- Multi-hop mesh network
- Self-healing topology
- Broadcast to all mesh nodes
- No need to specify MAC addresses
- Automatic routing

### Setup Instructions

#### Step 1: Upload Code
1. **Receiver(s)**: Upload `receive_one_picture.ino` to receiver ESP32(s)
2. **Transmitter**: Upload `send_one_picture.ino` to ESP32-CAM

#### Step 2: Configuration
Both transmitter and receiver must use the same mesh credentials:
```cpp
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555
```

Adjust capture interval in transmitter:
```cpp
#define CAPTURE_INTERVAL 30000   // 30 seconds between captures
```

Adjust image parameters:
```cpp
config.frame_size = FRAMESIZE_SVGA;  // Smaller for mesh network
config.jpeg_quality = 15;             // Higher number = lower quality
```

### Expected Output

**Transmitter:**
```
ESP32-CAM Mesh Transmitter
===========================
Camera initialized
Node ID: 3654789120
Mesh network started

=== Capturing Image ===
Image captured: 18234 bytes
Will send in 19 chunks
Sent start message
Sent chunk 5/19 (26.3%)
Sent chunk 10/19 (52.6%)
Sent chunk 15/19 (78.9%)
Sent chunk 19/19 (100.0%)
All chunks sent!
Sent end message
```

**Receiver:**
```
ESP32 Mesh Image Receiver
==========================
Node ID: 2198765432

=== Starting Image Reception ===
From Node: 3654789120
Image size: 18234 bytes
Total chunks: 19
Chunk size: 1000 bytes
Received: 5/19 chunks (26.3%)
Received: 10/19 chunks (52.6%)
Received: 15/19 chunks (78.9%)
Received: 19/19 chunks (100.0%)

=== Image Reception Complete ===
Received 19/19 chunks
All chunks received successfully!
Valid JPEG image received!
```

## Comparison: ESP-NOW vs Mesh

| Feature | ESP-NOW | Mesh |
|---------|---------|------|
| **Setup Complexity** | Medium (need MAC addresses) | Easy (auto-discovery) |
| **Speed** | Very Fast | Moderate |
| **Range** | Direct only | Multi-hop possible |
| **Scalability** | Limited pairs | Many nodes |
| **Reliability** | High (direct) | Good (redundant paths) |
| **Use Case** | Fixed sensors | Dynamic networks |

## Device Mapping (from MACaddress.csv)

| Device | MAC Address | Role | Compatible Protocol |
|--------|-------------|------|---------------------|
| R1 | A4:CF:12:9B:7E:23 | Receiver | ESP-NOW, Mesh |
| A_SR | A4:CF:12:58:D1:9A | Sub-Receiver | Mesh |
| A_T1 | A4:CF:12:3C:AF:10 | Transmitter (CAM) | ESP-NOW, Mesh |
| A_T2 | A4:CF:12:E7:42:55 | Transmitter (CAM) | ESP-NOW, Mesh |

## Troubleshooting

### Camera Issues
- **"Camera init failed"**: Check board selection and partition scheme
- **"PSRAM not found"**: Use smaller frame size (QVGA or VGA)
- **Brownout detected**: Use adequate power supply (2A minimum)

### ESP-NOW Issues
- **"Failed to add peer"**: Verify MAC address format
- **"Delivery Fail"**: Check receiver is powered and in range
- **Packet loss**: Reduce distance, avoid WiFi interference

### Mesh Issues
- **Nodes not connecting**: Verify MESH_PREFIX and MESH_PASSWORD match
- **"Failed to allocate memory"**: Reduce CHUNK_SIZE or image quality
- **Dropped chunks**: Reduce capture frequency, increase CHUNK_SIZE

### General Tips
- Use Serial Monitor at **115200 baud**
- Reset both devices after upload
- Keep devices within 10-20 meters for testing
- Use external antenna for longer range
- Avoid obstacles and interference

## Advanced Modifications

### Save Image to SD Card (Receiver)
Add SD card code in `processImage()`:
```cpp
#include "SD_MMC.h"

void processImage() {
  File file = SD_MMC.open("/image.jpg", FILE_WRITE);
  if (file) {
    file.write(imageBuffer, imageSize);
    file.close();
    Serial.println("Image saved to SD card!");
  }
}
```

### Send Image to Web Server
Use HTTPClient to POST image data to a server.

### Adjust Capture Timing
Change intervals in transmitter code:
- ESP-NOW: Modify `delay(10000)` in `loop()`
- Mesh: Change `CAPTURE_INTERVAL` constant

### Multiple Transmitters
Both protocols support multiple transmitters:
- **ESP-NOW**: Each transmitter needs receiver's MAC
- **Mesh**: All nodes automatically communicate

## Performance Notes

### Typical Transfer Times
- **ESP-NOW**: 
  - VGA (640x480): ~2-3 seconds
  - SVGA (800x600): ~4-6 seconds
  
- **Mesh**:
  - VGA: ~5-8 seconds
  - SVGA: ~10-15 seconds

### Memory Usage
- ESP32-CAM has 4MB PSRAM
- VGA JPEG: 15-30 KB
- SVGA JPEG: 20-50 KB
- Image buffer dynamically allocated

## License
This code is based on examples from:
- ESP32 Camera examples
- DroneBot Workshop ESP-NOW tutorials
- RandomNerdTutorials painlessMesh examples

## Support
For issues related to:
- **Hardware**: Check ESP32-CAM datasheet
- **ESP-NOW**: Review ESP-IDF documentation
- **painlessMesh**: Visit https://gitlab.com/painlessMesh/painlessMesh

---
**Last Updated**: November 2025
**ESP32 Board Version**: 2.0.x
**painlessMesh Version**: 1.5.x

