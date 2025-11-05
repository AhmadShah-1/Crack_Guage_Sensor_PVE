# ESP-NOW Image Transfer

## Quick Start Guide

### 1. Find MAC Addresses
Upload `find_MAC-address.ino` from `Pre-release/find_MAC-address/` to both devices and note the MAC addresses.

### 2. Configure Transmitter
Edit `send_one_picture.ino`:
```cpp
// Line 18: Replace with your receiver's MAC address
uint8_t receiverAddress[] = {0xA4, 0xCF, 0x12, 0x9B, 0x7E, 0x23};
```

### 3. Upload Code
1. Upload `receive_one_picture.ino` to receiver (regular ESP32)
2. Upload `send_one_picture.ino` to transmitter (ESP32-CAM)

### 4. Monitor Serial Output
Open Serial Monitor at 115200 baud for both devices.

## Configuration Options

### Image Quality
In `send_one_picture.ino`:
```cpp
config.frame_size = FRAMESIZE_VGA;   // Image resolution
config.jpeg_quality = 12;             // 0-63 (lower = better)
```

Available frame sizes:
- `FRAMESIZE_QVGA` - 320x240 (~8-15 KB)
- `FRAMESIZE_VGA` - 640x480 (~15-30 KB)
- `FRAMESIZE_SVGA` - 800x600 (~20-50 KB)
- `FRAMESIZE_XGA` - 1024x768 (~30-80 KB)

### Capture Interval
In `send_one_picture.ino`, `loop()` function:
```cpp
delay(10000);  // 10 seconds between captures
```

### Packet Size
```cpp
#define MAX_PACKET_SIZE 240  // Do not exceed 250
```

## Wiring (ESP32-CAM)

### Programming Mode
Connect FTDI programmer:
- 5V → 5V
- GND → GND
- U0R → TX
- U0T → RX
- IO0 → GND (for programming only)

### Running Mode
Remove IO0 → GND connection and press RESET.

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Camera init failed | Check board selection: "AI Thinker ESP32-CAM" |
| Brownout detector | Use 5V 2A power supply, not USB power |
| Delivery Fail | Verify MAC address, check distance < 20m |
| Packets lost | Reduce image size or quality |
| Out of memory | Use smaller frame size |

## Performance

### Typical Transfer Times (VGA quality)
- Image capture: ~200ms
- Transfer time: ~2-3 seconds
- Total cycle: ~3 seconds per image

### Range
- Indoor: 10-30 meters
- Outdoor: 50-100 meters (with clear line of sight)
- Use external antenna for extended range

## Extending Functionality

### Save to SD Card
Add SD card module to receiver and modify `processImage()` in receiver code.

### Multiple Transmitters
Each transmitter sends to the same receiver. Receiver handles images from multiple sources sequentially.

### Image Processing
Process `imageBuffer` in receiver's `processImage()` function for:
- Edge detection
- Object recognition
- Crack detection
- Color analysis

## MAC Address Reference
From `assests/MACaddress.csv`:
```
R1:   A4:CF:12:9B:7E:23  (Receiver)
A_T1: A4:CF:12:3C:AF:10  (Transmitter CAM)
A_T2: A4:CF:12:E7:42:55  (Transmitter CAM)
```

