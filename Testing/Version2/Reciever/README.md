# Main Receiver Module (ESP32)

## Overview

The Main Receiver is the final destination in the image transfer chain:
- **Receives** images from Sub-Receivers via painlessMesh
- **Outputs** structured data to Serial for Raspberry Pi processing

**Communication:** painlessMesh only (no ESP-NOW)

## Hardware

- **Device:** ESP32 Dev Module (standard ESP32)
- **Power:** 5V 1A minimum (or USB from Raspberry Pi)
- **Connection to Raspberry Pi:** USB Serial

## Device Information

| Device | MAC Address | Receives From | Outputs To |
|--------|-------------|---------------|------------|
| R1 | A4:CF:12:9B:7E:23 | A_SR, B_SR (via Mesh) | Raspberry Pi (Serial) |

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

Install via Arduino IDE Library Manager (same as Sub-Receiver):

1. **painlessMesh** (v1.5.0 or later)
2. **ArduinoJson** (v6.x)
3. **TaskScheduler** (v3.x)

## Configuration

### No Device-Specific Configuration Required!

The main receiver code works as-is. No need to change device IDs or MAC addresses.

Just verify mesh credentials (lines ~13-15):
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
5. Press RESET button (if needed)

### 3. Verify Operation

Open Serial Monitor (115200 baud). You should see:

```
[R1] Main Receiver
========================================
[R1] Mesh initialized
[R1] Node ID: 987654321
========================================
[R1] Waiting for images from sub-receivers...
========================================
```

## Operation

### Mesh Reception

The main receiver waits for mesh messages from sub-receivers:

```
[R1] Mesh: New connection 123456789  (A_SR connected)
[R1] Mesh: New connection 234567890  (B_SR connected)
[R1] Mesh: Connections changed
[R1] Mesh: 2 nodes connected
```

### Image Reception

When a sub-receiver sends an image:

```
[R1] === Starting Reception ===
[R1] Camera: A_T1
[R1] Sub-receiver: A_SR
[R1] Size: 24567 bytes
[R1] Chunks: 25

[R1] A_T1: 5/25 chunks (20.0%)
[R1] A_T1: 10/25 chunks (40.0%)
[R1] A_T1: 15/25 chunks (60.0%)
[R1] A_T1: 20/25 chunks (80.0%)
[R1] A_T1: 25/25 chunks (100.0%)

[R1] === Reception Complete ===
[R1] Camera: A_T1
[R1] Sub-receiver: A_SR
[R1] Received: 25/25 chunks
[R1] Transfer time: 5432 ms
[R1] All chunks received!
[R1] Valid JPEG confirmed
```

### Serial Output Format

After successful reception, the image is output in a structured format for the Raspberry Pi:

```
===IMAGE_START===
CAMERA: A_T1
SUBRECEIVER: A_SR
SIZE: 24567
TIMESTAMP: 1234567890
===DATA===
FFD8FFE000104A46494600010100000100010000FFDB004300080606...
(hex-encoded image data, 64 bytes per line)
...
===IMAGE_END===

[R1] Image data sent to serial (24567 bytes)
```

### Multiple Simultaneous Images

The receiver can handle up to 4 images simultaneously (one from each camera):

```
[R1] === Starting Reception ===
[R1] Camera: A_T1
[R1] Sub-receiver: A_SR

[R1] === Starting Reception ===
[R1] Camera: B_T1
[R1] Sub-receiver: B_SR

[R1] A_T1: 5/25 chunks (20.0%)
[R1] B_T1: 5/23 chunks (21.7%)
...
```

## Serial Output Details

### Output Structure

The serial output is designed for easy parsing by Raspberry Pi:

1. **Start Marker:** `===IMAGE_START===`
2. **Metadata:**
   - `CAMERA:` Which camera took the photo (A_T1, A_T2, B_T1, B_T2)
   - `SUBRECEIVER:` Which sub-receiver forwarded it (A_SR, B_SR)
   - `SIZE:` Image size in bytes
   - `TIMESTAMP:` Milliseconds since ESP32 boot
3. **Data Marker:** `===DATA===`
4. **Image Data:** Hex-encoded JPEG (2 hex chars per byte, 64 bytes per line)
5. **End Marker:** `===IMAGE_END===`

### Raspberry Pi Integration

The Raspberry Pi should:
1. Monitor serial port (115200 baud)
2. Wait for `===IMAGE_START===`
3. Parse metadata
4. Collect hex data between `===DATA===` and `===IMAGE_END===`
5. Convert hex to binary
6. Save as JPEG file

Example filename: `A_T1_from_A_SR_1234567890.jpg`

## Troubleshooting

### Mesh Not Connecting

**Symptoms:**
```
No "Mesh: New connection" messages
```

**Solutions:**
- **Verify ESP32 board version 2.0.x** (most common issue!)
- Check mesh credentials match all devices
- Ensure Sub-Receivers are running
- Wait 20-30 seconds for mesh formation
- Power cycle all devices in order: R1, sub-receivers, cameras

### No Images Received

**Symptoms:**
```
Mesh connected but no "Starting Reception" messages
```

**Solutions:**
- Verify sub-receivers are running and connected to mesh
- Check cameras are sending to sub-receivers
- Monitor sub-receiver serial output
- Ensure complete chain is operational

### Incomplete Images

**Symptoms:**
```
[R1] WARNING: 3 chunks missing
```

**Solutions:**
- Check mesh network stability
- Reduce camera image size
- Increase delays in sub-receiver forwarding
- Check for WiFi interference
- Reduce distance between devices

### Memory Errors

**Symptoms:**
```
[R1] ERROR: Failed to allocate memory for A_T1 from A_SR
```

**Solutions:**
- Reduce camera image size (use VGA or QVGA)
- Increase camera JPEG quality number
- Power cycle main receiver
- Check if too many images being processed simultaneously

### Timeout Errors

**Symptoms:**
```
[R1] Timeout: A_T1 from A_SR (15/25 chunks)
```

**Solutions:**
- Check sub-receiver is still running
- Verify mesh connectivity
- Check sub-receiver serial for errors
- Reduce network traffic

## Performance

**Typical Values:**
- Mesh reception time: 4-6 seconds (VGA image)
- Total system latency: 7-10 seconds (camera capture to serial output)
- Can handle 4 concurrent image receptions
- Memory usage: ~30-50 KB per image buffer

## Testing Checklist

Before deploying:

- [ ] ESP32 board version 2.0.x installed
- [ ] All required libraries installed
- [ ] Serial Monitor shows Mesh initialized
- [ ] Mesh connections to sub-receivers established
- [ ] Can receive images from sub-receivers
- [ ] Serial output format is correct
- [ ] No memory allocation errors
- [ ] Raspberry Pi can parse serial output

## System Integration

### Complete Data Flow

```
Cameras → [ESP-NOW] → Sub-Receivers → [Mesh] → Main Receiver → [Serial] → Raspberry Pi

A_T1 ──┐
       ├─→ A_SR ──┐
A_T2 ──┘          │
                  ├─→ R1 ──→ RaspberryPi
B_T1 ──┐          │
       ├─→ B_SR ──┘
B_T2 ──┘
```

### Startup Sequence

1. **Start R1 (Main Receiver)** - Let it initialize mesh
2. **Start A_SR and B_SR** - Wait for mesh connections to R1
3. **Start Cameras** - They will begin capturing and sending

Monitor R1 serial output to verify complete flow.

## Raspberry Pi Integration

### Python Serial Reading Example

```python
import serial
import binascii
from datetime import datetime

ser = serial.Serial('/dev/ttyUSB0', 115200, timeout=1)

def read_image():
    while True:
        line = ser.readline().decode('utf-8').strip()
        
        if line == "===IMAGE_START===":
            metadata = {}
            hex_data = ""
            
            # Read metadata
            while True:
                line = ser.readline().decode('utf-8').strip()
                if line == "===DATA===":
                    break
                if ':' in line:
                    key, value = line.split(':', 1)
                    metadata[key.strip()] = value.strip()
            
            # Read hex data
            while True:
                line = ser.readline().decode('utf-8').strip()
                if line == "===IMAGE_END===":
                    break
                hex_data += line
            
            # Convert hex to binary
            image_data = binascii.unhexlify(hex_data)
            
            # Save image
            filename = f"{metadata['CAMERA']}_from_{metadata['SUBRECEIVER']}_{metadata['TIMESTAMP']}.jpg"
            with open(filename, 'wb') as f:
                f.write(image_data)
            
            print(f"Saved {filename} ({len(image_data)} bytes)")

while True:
    read_image()
```

### Testing Serial Output

You can test serial output without Raspberry Pi:

1. Open Serial Monitor in Arduino IDE
2. Set to 115200 baud
3. Watch for structured output
4. Copy hex data from `===DATA===` to `===IMAGE_END===`
5. Use online hex-to-file converter to verify JPEG

## Monitoring

### Key Messages to Watch

**Good:**
- "Mesh initialized"
- "Mesh: New connection"
- "=== Starting Reception ==="
- "All chunks received!"
- "Valid JPEG confirmed"
- "===IMAGE_START===" ... "===IMAGE_END==="

**Warnings:**
- "WARNING: X chunks missing" (incomplete image)
- "Timeout: ..." (reception timeout)

**Errors:**
- "ERROR: Failed to allocate memory"
- "ERROR: Invalid JPEG header"

## Advanced Configuration

### Maximum Concurrent Images

Line ~17:
```cpp
#define MAX_IMAGES 4
```

With 4 cameras, set to at least 4. Increase if you add more cameras.

### Reception Timeout

Line ~71:
```cpp
#define TIMEOUT_MS 10000  // 10 seconds
```

Increase if using large images or slow mesh network.

## Next Steps

After main receiver is working:

1. Verify mesh connections to all sub-receivers
2. Test image reception from each camera
3. Verify serial output format
4. Integrate with Raspberry Pi
5. Test complete system end-to-end
6. Monitor for stability over time

## Support

For issues:
1. **First:** Verify ESP32 board version 2.0.x
2. Check all required libraries installed
3. Verify mesh connections established
4. Check sub-receivers are forwarding images
5. Monitor serial output for specific errors
6. Test with single camera first, then scale up

