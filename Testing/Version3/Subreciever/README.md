# Sub-Receiver Module (ESP32)

## Overview

The Sub-Receiver acts as a relay between Camera modules and the Main Receiver:
- **Receives** images from Camera modules via ESP-NOW
- **Forwards** images to Main Receiver via painlessMesh

**Communication:** ESP-NOW (receive) + painlessMesh (send)

## Hardware

- **Device:** ESP32 Dev Module (standard ESP32, NOT ESP32-CAM)
- **Power:** 5V 1A minimum
- **Note:** No camera needed

## Device Configuration

You have 2 sub-receiver modules:

| Device | MAC Address | Receives From | Forwards To |
|--------|-------------|---------------|-------------|
| A_SR | A4:CF:12:58:D1:9A | A_T1, A_T2 | R1 (via Mesh) |
| B_SR | A4:CF:11:9B:7E:24 | B_T1, B_T2 | R1 (via Mesh) |

## Software Requirements

### ESP32 Board Version

**CRITICAL:** Must use ESP32 board version **2.0.x** (NOT 3.x)

painlessMesh is not compatible with version 3.x.

To install correct version:
1. Tools → Board → Boards Manager
2. Search "ESP32"
3. If version 3.x installed: Click "Remove"
4. Select version 2.0.17 from dropdown
5. Click "Install"

### Required Libraries

Install via Arduino IDE Library Manager:

1. **painlessMesh** (v1.5.0 or later)
   - Tools → Manage Libraries
   - Search "painlessMesh"
   - Click Install

2. **ArduinoJson** (v6.x)
   - Usually auto-installed with painlessMesh
   - If not: Search "ArduinoJson" and install

3. **TaskScheduler** (v3.x)
   - Usually auto-installed with painlessMesh
   - If not: Search "TaskScheduler" and install

## Configuration Steps

### 1. Open the Code

Open `subreciever_relay/subreciever_relay.ino` in Arduino IDE.

### 2. Set Sub-Receiver ID

Find line ~24:
```cpp
#define SUBRECEIVER_ID "A_SR"  // OPTIONS: A_SR, B_SR
```

Change to match your device:
- For A_SR: `"A_SR"`
- For B_SR: `"B_SR"`

### 3. Verify Mesh Credentials

Lines ~27-29 (should match all devices):
```cpp
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555
```

**Do not change these** unless you change them on all devices.

## Upload Instructions

### 1. Arduino IDE Settings

- **Board:** "ESP32 Dev Module"
- **Upload Speed:** 115200
- **Flash Frequency:** 80MHz
- **Flash Mode:** DIO or QIO
- **Partition Scheme:** "Default 4MB with spiffs"
- **Core Debug Level:** "None"

### 2. Upload Process

1. Connect ESP32 to computer via USB
2. Select correct COM port
3. Click Upload
4. Wait for "Done uploading"
5. Press RESET button (if auto-reset doesn't work)

### 3. Verify Operation

Open Serial Monitor (115200 baud). You should see:

```
[A_SR] Sub-Receiver Relay
========================================
[A_SR] MAC Address: A4:CF:12:58:D1:9A
[A_SR] ESP-NOW initialized
[A_SR] Mesh initialized
[A_SR] Mesh Node ID: 123456789
========================================
[A_SR] Ready to receive from cameras and forward to mesh
========================================
```

## Operation

### Dual Protocol Operation

The sub-receiver runs both protocols simultaneously:

**ESP-NOW Reception:**
- Listens for packets from cameras
- Can receive from 2 cameras simultaneously
- Assembles packets into complete images
- Stores in temporary memory buffers

**Mesh Forwarding:**
- Converts complete images to mesh messages
- Sends in chunks via mesh network
- Frees memory after successful forward

### When Receiving Image

```
[A_SR] Started receiving from A_T1 (107 packets)
[A_SR] A_T1: 20/107 packets (18.7%)
[A_SR] A_T1: 40/107 packets (37.4%)
...
[A_SR] A_T1: 107/107 packets (100.0%)
[A_SR] Image complete from A_T1: 24567 bytes
```

### When Forwarding to Mesh

```
[A_SR] Forwarding A_T1 image to mesh...
[A_SR] Mesh forward: 5/25 chunks (20.0%)
[A_SR] Mesh forward: 10/25 chunks (40.0%)
...
[A_SR] Mesh forward: 25/25 chunks (100.0%)
[A_SR] Forward complete for A_T1
```

### Simultaneous Camera Handling

The sub-receiver can handle 2 cameras at once:

```
[A_SR] Started receiving from A_T1 (107 packets)
[A_SR] Started receiving from A_T2 (98 packets)
[A_SR] A_T1: 20/107 packets (18.7%)
[A_SR] A_T2: 20/98 packets (20.4%)
[A_SR] A_T1: 40/107 packets (37.4%)
[A_SR] A_T2: 40/98 packets (40.8%)
...
```

## Troubleshooting

### ESP-NOW Not Receiving

**Symptoms:**
- No "Started receiving" messages
- Cameras show "Delivery Fail"

**Solutions:**
- Verify ESP-NOW initialized successfully
- Check camera is using correct sub-receiver MAC
- Ensure both devices powered and in range
- Power cycle both camera and sub-receiver

### Mesh Not Connecting

**Symptoms:**
```
No "Mesh: New connection" messages
```

**Solutions:**
- **Verify ESP32 board version 2.0.x** (most common issue!)
- Check mesh credentials match all devices
- Ensure Main Receiver (R1) is running
- Wait 20-30 seconds for mesh formation
- Check Serial Monitor for "Mesh initialized"

### painlessMesh Errors

**Symptoms:**
```
Compilation error: painlessMesh.h not found
```

**Solutions:**
- Install painlessMesh library via Library Manager
- Install ArduinoJson library
- Install TaskScheduler library
- Restart Arduino IDE

### Out of Memory

**Symptoms:**
```
[A_SR] ERROR: Failed to allocate memory for A_T1
```

**Solutions:**
- Reduce camera image size (use VGA or QVGA)
- Increase camera JPEG quality number (lower quality)
- Power cycle sub-receiver
- Check for memory leaks (shouldn't happen with this code)

### Timeout Errors

**Symptoms:**
```
[A_SR] Timeout receiving from A_T1 (45/107 packets)
```

**Solutions:**
- Check camera-to-sub-receiver distance (keep < 10m for testing)
- Ensure stable power to camera
- Reduce image size for faster transfer
- Check for WiFi interference

## Mesh Network Formation

### Connection Process

1. Sub-receiver starts mesh
2. Searches for other mesh nodes
3. Connects to Main Receiver (R1)
4. May also connect to other sub-receivers

Expected timeline:
- 0-5 seconds: Initialization
- 5-15 seconds: Discovery
- 15-30 seconds: Stable mesh formed

### Verifying Mesh

Look for these messages:
```
[A_SR] Mesh initialized
[A_SR] Mesh Node ID: 123456789
[A_SR] Mesh: New connection 987654321
[A_SR] Mesh: Connection changed
```

The Node ID should be unique for each device.

## Configuration Reference

### Quick Configuration

**For A_SR:**
```cpp
#define SUBRECEIVER_ID "A_SR"
```
Receives from: A_T1 (A4:CF:12:3C:AF:10), A_T2 (A4:CF:12:E7:42:55)

**For B_SR:**
```cpp
#define SUBRECEIVER_ID "B_SR"
```
Receives from: B_T1 (A1:DF:12:9B:7E:23), B_T2 (A4:CF:12:2B:7G:23)

## Performance

**Typical Values:**
- ESP-NOW receive: 3-4 seconds (VGA image)
- Mesh forward: 4-6 seconds (VGA image)
- Total latency: 7-10 seconds (camera to main receiver)
- Memory usage: ~60-80 KB per image buffer
- Can handle 2 cameras simultaneously

## Testing Checklist

Before deploying:

- [ ] Correct SUBRECEIVER_ID configured
- [ ] ESP32 board version 2.0.x installed
- [ ] All required libraries installed
- [ ] Serial Monitor shows ESP-NOW initialized
- [ ] Serial Monitor shows Mesh initialized
- [ ] Mesh connection to R1 established
- [ ] Cameras configured to send to this sub-receiver
- [ ] Can receive images from cameras
- [ ] Can forward images to mesh
- [ ] No memory allocation errors

## System Integration

### Data Flow

```
Camera → [ESP-NOW] → Sub-Receiver → [Mesh] → Main Receiver
A_T1  ────────────→  A_SR  ──────────────────→  R1
A_T2  ────────────→   ↑
```

### Verification Steps

1. **Start Main Receiver (R1)** first
2. **Start Sub-Receivers (A_SR, B_SR)** second
   - Wait for mesh connection to R1
3. **Start Cameras (A_T1, A_T2, B_T1, B_T2)** last
   - Should immediately start capturing and sending

Monitor all serial outputs to verify complete flow.

## Advanced Configuration

### Mesh Chunk Size

Line ~34:
```cpp
#define MESH_CHUNK_SIZE 1000
```

- Smaller values: More packets, slower, more reliable
- Larger values: Fewer packets, faster, may drop
- Recommended: 800-1200 bytes

### Reception Timeout

Line ~123:
```cpp
#define RECEPTION_TIMEOUT 3000  // 3 seconds
```

Adjust if cameras are slow or far away.

## Monitoring

### Key Messages to Watch

**Good:**
- "ESP-NOW initialized"
- "Mesh initialized"
- "Mesh: New connection"
- "Started receiving from..."
- "Image complete from..."
- "Forward complete for..."

**Warnings:**
- "WARNING: No free buffer" (3rd camera trying to send)
- "Timeout receiving from..." (incomplete image)

**Errors:**
- "ERROR: Failed to allocate memory" (out of memory)
- "FATAL: ESP-NOW init failed"
- "FATAL: Mesh init failed"

## Next Steps

After sub-receiver is working:

1. Configure and upload both sub-receivers (A_SR, B_SR)
2. Ensure Main Receiver (R1) is running
3. Start cameras and verify images flowing through system
4. Monitor complete chain: Camera → Sub-Receiver → Main Receiver
5. Check Main Receiver serial output for complete images

## Support

For issues:
1. **First:** Verify ESP32 board version 2.0.x
2. Check all required libraries installed
3. Verify Serial Monitor output for specific errors
4. Ensure mesh credentials match all devices
5. Test mesh connection to R1 before adding cameras
6. Check power supplies are adequate

