# Camera Module (ESP32-CAM)

## Overview

The Camera module captures images and transmits them to Sub-Receivers via ESP-NOW protocol.

**Communication:** ESP-NOW only (direct to Sub-Receiver)

## Hardware

- **Device:** ESP32-CAM (AI-THINKER module recommended)
- **Camera:** OV2640 or compatible
- **Flash LED:** Built-in on GPIO 4
- **Power:** 5V 2A minimum (important!)

## Device Configuration

You have 4 camera modules. Each must be configured individually:

| Device | MAC Address | Target Sub-Receiver | Config |
|--------|-------------|---------------------|--------|
| A_T1 | A4:CF:12:3C:AF:10 | A_SR | `DEVICE_ID = "A_T1"`, `subReceiverMAC = A_SR` |
| A_T2 | A4:CF:12:E7:42:55 | A_SR | `DEVICE_ID = "A_T2"`, `subReceiverMAC = A_SR` |
| B_T1 | A1:DF:12:9B:7E:23 | B_SR | `DEVICE_ID = "B_T1"`, `subReceiverMAC = B_SR` |
| B_T2 | A4:CF:12:2B:7G:23 | B_SR | `DEVICE_ID = "B_T2"`, `subReceiverMAC = B_SR` |

## Configuration Steps

### 1. Open the Code

Open `camera_transmitter/camera_transmitter.ino` in Arduino IDE.

### 2. Set Device ID

Find line ~17:
```cpp
#define DEVICE_ID "A_T1"  // OPTIONS: A_T1, A_T2, B_T1, B_T2
```

Change to match your device: `"A_T1"`, `"A_T2"`, `"B_T1"`, or `"B_T2"`

### 3. Set Sub-Receiver MAC Address

Find line ~22:
```cpp
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x12, 0x58, 0xD1, 0x9A}; // A_SR
```

**For A_T1 and A_T2 (targeting A_SR):**
```cpp
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x12, 0x58, 0xD1, 0x9A}; // A_SR
```

**For B_T1 and B_T2 (targeting B_SR):**
```cpp
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x11, 0x9B, 0x7E, 0x24}; // B_SR
```

### 4. Optional: Adjust Settings

**Image Quality** (line ~76):
```cpp
config.frame_size = FRAMESIZE_VGA;  // QVGA, VGA, SVGA, XGA
config.jpeg_quality = 12;            // 0-63 (lower = better quality)
```

**Capture Interval** (line ~31):
```cpp
#define CAPTURE_INTERVAL 10000   // milliseconds (10 seconds)
```

## Upload Instructions

### 1. Arduino IDE Settings

- **Board:** "AI Thinker ESP32-CAM"
- **Upload Speed:** 115200
- **Flash Frequency:** 80MHz
- **Flash Mode:** QIO
- **Partition Scheme:** "Huge APP (3MB No OTA/1MB SPIFFS)"
- **Core Debug Level:** "None"

### 2. Wiring for Programming

Connect FTDI programmer to ESP32-CAM:

```
FTDI    ESP32-CAM
-----   ---------
5V   →  5V
GND  →  GND
TX   →  U0R
RX   →  U0T

For Programming Mode:
GPIO 0 → GND (connect jumper wire)
```

### 3. Upload Process

1. Connect GPIO 0 to GND
2. Connect FTDI to computer
3. Select correct COM port
4. Click Upload in Arduino IDE
5. Wait for "Done uploading"
6. **Disconnect GPIO 0 from GND**
7. Press RESET button on ESP32-CAM

### 4. Verify Operation

Open Serial Monitor (115200 baud). You should see:

```
[A_T1] ESP32-CAM Transmitter
========================================
[A_T1] Camera initialized
[A_T1] MAC Address: A4:CF:12:3C:AF:10
[A_T1] ESP-NOW initialized
[A_T1] Sub-receiver registered
[A_T1] Target: A4:CF:12:58:D1:9A
========================================

[A_T1] === Capturing Image ===
[A_T1] Image captured: 24567 bytes
[A_T1] Sending 107 packets to sub-receiver...
[A_T1] Progress: 20/107 packets
[A_T1] Progress: 40/107 packets
...
[A_T1] Transfer complete: 107/107 packets successful
```

## Operation

### Normal Operation

After successful upload and reset:

1. Camera initializes (2 seconds)
2. Takes first photo automatically
3. Sends to configured sub-receiver via ESP-NOW
4. Waits for configured interval
5. Repeats capture and send

### LED Indicators

- **Flash LED (bright):** Turns on briefly during capture
- **Red LED (small):** May blink during transmission

## Troubleshooting

### Camera Init Failed

**Symptoms:**
```
[A_T1] Camera init failed: 0x105
```

**Solutions:**
- Verify board selection: "AI Thinker ESP32-CAM"
- Check partition scheme: "Huge APP (3MB)"
- Power cycle the device
- Use external 5V 2A power supply

### Brownout Detector

**Symptoms:**
```
Brownout detector was triggered
```

**Solutions:**
- **CRITICAL:** Use 5V 2A power supply
- Do NOT power from USB-serial adapter
- Use shorter, thicker power cables
- Check power supply quality

### Upload Failed

**Symptoms:**
```
Failed to connect to ESP32
```

**Solutions:**
- Ensure GPIO 0 connected to GND before upload
- Press and hold RESET, click Upload, release RESET
- Try lower upload speed (115200)
- Verify correct COM port
- Check USB cable (must support data, not just charging)

### Delivery Fail Messages

**Symptoms:**
```
[A_T1] Packet failed: X
```

**Solutions:**
- Verify sub-receiver is powered and running
- Check sub-receiver MAC address configuration
- Reduce distance between camera and sub-receiver (< 10m for testing)
- Ensure sub-receiver has ESP-NOW initialized

### Memory Issues

**Symptoms:**
```
Camera capture failed
```

**Solutions:**
- Reduce image size: Change to `FRAMESIZE_QVGA` or `FRAMESIZE_VGA`
- Increase JPEG quality number: Try `15` or `18`
- Power cycle device

## Configuration Reference

### Quick Configuration Table

Copy and paste these configurations for each device:

**A_T1:**
```cpp
#define DEVICE_ID "A_T1"
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x12, 0x58, 0xD1, 0x9A}; // A_SR
```

**A_T2:**
```cpp
#define DEVICE_ID "A_T2"
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x12, 0x58, 0xD1, 0x9A}; // A_SR
```

**B_T1:**
```cpp
#define DEVICE_ID "B_T1"
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x11, 0x9B, 0x7E, 0x24}; // B_SR
```

**B_T2:**
```cpp
#define DEVICE_ID "B_T2"
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x11, 0x9B, 0x7E, 0x24}; // B_SR
```

## Image Quality Settings

### Recommended Settings

**High Quality (slower transfer):**
```cpp
config.frame_size = FRAMESIZE_SVGA;  // 800x600
config.jpeg_quality = 10;             // High quality
// ~30-50 KB images, ~5-7 seconds transfer
```

**Balanced (recommended):**
```cpp
config.frame_size = FRAMESIZE_VGA;   // 640x480
config.jpeg_quality = 12;             // Good quality
// ~15-30 KB images, ~3-4 seconds transfer
```

**Fast (lower quality):**
```cpp
config.frame_size = FRAMESIZE_QVGA;  // 320x240
config.jpeg_quality = 15;             // Acceptable quality
// ~8-15 KB images, ~1-2 seconds transfer
```

## Testing Checklist

Before deploying:

- [ ] Correct DEVICE_ID configured
- [ ] Correct subReceiverMAC configured
- [ ] Board selection: "AI Thinker ESP32-CAM"
- [ ] Partition scheme: "Huge APP (3MB)"
- [ ] Serial Monitor shows successful camera init
- [ ] Serial Monitor shows ESP-NOW initialized
- [ ] Images capturing successfully
- [ ] Packets sending without errors
- [ ] Sub-receiver receiving images (check sub-receiver serial output)

## Performance

**Typical Values (VGA, Quality 12):**
- Image size: 15-30 KB
- Capture time: ~200 ms
- Transfer time: ~3-4 seconds
- Total cycle: ~4 seconds per image
- Range: 30-100 meters (depending on environment)

## Next Steps

After camera is working:

1. Configure and upload to all 4 cameras (A_T1, A_T2, B_T1, B_T2)
2. Ensure corresponding sub-receivers (A_SR, B_SR) are running
3. Verify images reaching sub-receivers via their serial output
4. Monitor complete system: Camera → Sub-Receiver → Main Receiver

## Support

For issues:
1. Check Serial Monitor output for specific errors
2. Verify power supply is adequate (5V 2A minimum)
3. Ensure correct board and partition settings
4. Test with smaller image size first (QVGA)
5. Confirm sub-receiver is running and accessible

