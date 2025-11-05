# Version 2: Hybrid ESP-NOW + Mesh Image Transfer System

## System Architecture

```
Cameras (ESP-NOW) → Sub-Receivers (ESP-NOW + Mesh) → Main Receiver (Mesh) → Raspberry Pi (Serial)

A_T1 ──┐
       ├─→ A_SR ──┐
A_T2 ──┘          │
                  ├─→ R1 ──→ [Serial] ──→ Raspberry Pi
B_T1 ──┐          │
       ├─→ B_SR ──┘
B_T2 ──┘
```

## Overview

This system implements a hierarchical image capture and transfer network:

1. **Camera Modules (ESP32-CAM):** Capture images and send via ESP-NOW to their sub-receiver
2. **Sub-Receivers (ESP32):** Receive from cameras via ESP-NOW, forward via Mesh to main receiver
3. **Main Receiver (ESP32):** Receive from sub-receivers via Mesh, output to serial for Raspberry Pi

## Device Configuration

| Device | Type | MAC Address | Protocol | Role |
|--------|------|-------------|----------|------|
| A_T1 | ESP32-CAM | A4:CF:12:3C:AF:10 | ESP-NOW TX | Camera → A_SR |
| A_T2 | ESP32-CAM | A4:CF:12:E7:42:55 | ESP-NOW TX | Camera → A_SR |
| B_T1 | ESP32-CAM | A1:DF:12:9B:7E:23 | ESP-NOW TX | Camera → B_SR |
| B_T2 | ESP32-CAM | A4:CF:12:2B:7G:23 | ESP-NOW TX | Camera → B_SR |
| A_SR | ESP32 | A4:CF:12:58:D1:9A | ESP-NOW RX + Mesh TX | Relay A cameras |
| B_SR | ESP32 | A4:CF:11:9B:7E:24 | ESP-NOW RX + Mesh TX | Relay B cameras |
| R1 | ESP32 | A4:CF:12:9B:7E:23 | Mesh RX | Final receiver |

## Quick Start

### 1. Prerequisites

**Software:**
- Arduino IDE with ESP32 board support version **2.0.x** (NOT 3.x)
- Libraries: painlessMesh, ArduinoJson, TaskScheduler

**Hardware:**
- 4x ESP32-CAM modules (cameras)
- 3x ESP32 Dev Modules (sub-receivers + main receiver)
- Power supplies (5V 2A for cameras, 5V 1A for receivers)

### 2. Upload Sequence

**Step 1: Main Receiver (R1)**
- Upload `Reciever/main_receiver/main_receiver.ino`
- No configuration needed
- Verify mesh initialized

**Step 2: Sub-Receivers (A_SR, B_SR)**
- Upload `Subreciever/subreciever_relay/subreciever_relay.ino`
- Configure `SUBRECEIVER_ID` for each ("A_SR" or "B_SR")
- Verify mesh connection to R1

**Step 3: Cameras (A_T1, A_T2, B_T1, B_T2)**
- Upload `Camera/camera_transmitter/camera_transmitter.ino`
- Configure `DEVICE_ID` and `subReceiverMAC` for each
- Verify images sending to sub-receivers

### 3. Verification

Monitor serial output on all devices:

**Cameras:** Should show image capture and ESP-NOW transmission  
**Sub-Receivers:** Should show ESP-NOW reception and Mesh forwarding  
**Main Receiver:** Should show Mesh reception and serial output

## Data Flow

### Complete Image Journey

1. **Camera captures** (200ms) → JPEG image ~15-30 KB
2. **ESP-NOW transmission** (3-4s) → Camera to Sub-Receiver
3. **Mesh forwarding** (4-6s) → Sub-Receiver to Main Receiver  
4. **Serial output** (immediate) → Main Receiver to Raspberry Pi

**Total latency:** ~7-10 seconds from capture to Raspberry Pi

## Configuration Details

### Cameras (see Camera/README.md)

Each camera needs unique configuration:
```cpp
#define DEVICE_ID "A_T1"  // Change for each camera
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x12, 0x58, 0xD1, 0x9A}; // A_SR or B_SR
```

### Sub-Receivers (see Subreciever/README.md)

Each sub-receiver needs ID:
```cpp
#define SUBRECEIVER_ID "A_SR"  // A_SR or B_SR
```

### Main Receiver (see Reciever/README.md)

No device-specific configuration required.

## Troubleshooting

### Common Issues

**1. Mesh not connecting**
- CRITICAL: Verify ESP32 board version 2.0.x (not 3.x)
- Check mesh credentials match all devices
- Wait 20-30 seconds for mesh formation

**2. Cameras not sending**
- Verify correct sub-receiver MAC address
- Check power supply (5V 2A for cameras)
- Reduce distance for testing (< 10m)

**3. Images not reaching R1**
- Check sub-receivers connected to mesh
- Monitor sub-receiver serial output
- Verify complete chain operational

**4. Memory errors**
- Reduce camera image size (use VGA or QVGA)
- Increase JPEG quality number (lower quality)

## Performance

**Typical Values:**
- Image size: 15-30 KB (VGA, Quality 12)
- Camera to Sub-Receiver: 3-4 seconds (ESP-NOW)
- Sub-Receiver to Main Receiver: 4-6 seconds (Mesh)
- Total system latency: 7-10 seconds
- Concurrent images: 4 (all cameras can operate simultaneously)

## Serial Output Format

Main Receiver outputs structured data for Raspberry Pi:

```
===IMAGE_START===
CAMERA: A_T1
SUBRECEIVER: A_SR
SIZE: 24567
TIMESTAMP: 1234567890
===DATA===
FFD8FFE000104A46494600... (hex-encoded JPEG)
===IMAGE_END===
```

## Raspberry Pi Integration

The Raspberry Pi should:
1. Monitor R1's serial port (115200 baud)
2. Parse structured output
3. Convert hex to binary
4. Save JPEG files

See `Reciever/README.md` for Python example code.

## File Structure

```
Version2/
├── Camera/
│   ├── camera_transmitter/
│   │   ├── camera_transmitter.ino
│   │   └── camera_pins.h
│   └── README.md (Configuration guide)
│
├── Subreciever/
│   ├── subreciever_relay/
│   │   └── subreciever_relay.ino
│   └── README.md (Setup instructions)
│
├── Reciever/
│   ├── main_receiver/
│   │   └── main_receiver.ino
│   └── README.md (Usage instructions)
│
└── README.md (This file)
```

## Testing Checklist

### Before Deployment

- [ ] ESP32 board version 2.0.x installed
- [ ] painlessMesh, ArduinoJson, TaskScheduler libraries installed
- [ ] All devices configured with correct IDs/MACs
- [ ] R1 shows mesh initialized
- [ ] Sub-receivers show mesh connected to R1
- [ ] Cameras show images sending
- [ ] Complete image flow verified
- [ ] Serial output format correct

### Deployment Order

1. Start R1 (main receiver)
2. Start A_SR and B_SR (wait for mesh connection)
3. Start cameras (A_T1, A_T2, B_T1, B_T2)
4. Monitor all serial outputs
5. Verify images reaching R1

## Next Steps

1. Deploy system as described above
2. Test with single camera first, then scale up
3. Integrate Raspberry Pi for image storage
4. Implement image processing/analysis
5. Add crack detection algorithms
6. Scale to additional cameras as needed

## Support

Detailed instructions in component README files:
- **Camera/README.md** - Camera configuration and troubleshooting
- **Subreciever/README.md** - Sub-receiver setup and mesh integration
- **Reciever/README.md** - Main receiver and Raspberry Pi integration

For system-wide issues, verify complete chain step by step from camera to main receiver.

---

**System Status:** Ready for deployment  
**Total Devices:** 7 (4 cameras, 2 sub-receivers, 1 main receiver)  
**Protocols:** ESP-NOW + painlessMesh  
**Target:** Raspberry Pi integration via serial

