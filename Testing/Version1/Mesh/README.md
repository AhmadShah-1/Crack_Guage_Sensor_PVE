# Mesh Network Image Transfer

## Quick Start Guide

### 1. Install Required Libraries
In Arduino IDE → Tools → Manage Libraries, install:
- painlessMesh
- ArduinoJson
- TaskScheduler

### 2. Set ESP32 Board Version
**IMPORTANT**: painlessMesh requires ESP32 board support version 2.0.x (NOT 3.x)
- Tools → Board → Boards Manager → ESP32
- If version 3.x is installed, uninstall and install 2.0.x

### 3. Configure Mesh Network
Both transmitter and receiver must have matching credentials:
```cpp
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555
```

### 4. Upload Code
1. Upload `receive_one_picture.ino` to receiver(s)
2. Upload `send_one_picture.ino` to ESP32-CAM transmitter

### 5. Monitor Serial Output
Open Serial Monitor at 115200 baud to see mesh formation and image transfer.

## Configuration Options

### Image Quality
In `send_one_picture.ino`:
```cpp
config.frame_size = FRAMESIZE_SVGA;  // Use SVGA or smaller for mesh
config.jpeg_quality = 15;             // Higher = smaller file
```

**Recommended Settings for Mesh:**
- Frame size: SVGA (800x600) or VGA (640x480)
- JPEG quality: 15-20 (lower quality = faster transfer)

### Capture Interval
```cpp
#define CAPTURE_INTERVAL 30000  // 30 seconds
```

### Chunk Size
```cpp
#define CHUNK_SIZE 1000  // Bytes per chunk
```
- Smaller = more packets, slower but more reliable
- Larger = fewer packets, faster but may drop

## How It Works

### Mesh Network Formation
1. All devices with matching credentials automatically connect
2. Mesh topology forms with automatic routing
3. No need to specify MAC addresses
4. Self-healing if a node fails

### Image Transfer Protocol
1. **Start Message**: Transmitter announces image transfer
   ```json
   {"type":"image_start", "node":123456, "size":18234, "chunks":19}
   ```

2. **Chunk Messages**: Image data sent in chunks
   ```json
   {"type":"image_chunk", "chunk_id":5, "total":19, "data":"FF D8..."}
   ```

3. **End Message**: Transfer complete notification
   ```json
   {"type":"image_end", "node":123456, "chunks":19}
   ```

## Network Topology

### Supported Configurations

**Single Receiver:**
```
[CAM] → [Receiver]
```

**Multiple Receivers:**
```
[CAM] → [Receiver 1]
    └→ [Receiver 2]
    └→ [Receiver 3]
```

**Multi-hop Relay:**
```
[CAM] → [Relay Node] → [Receiver]
```

**Multiple Transmitters:**
```
[CAM 1] ┐
[CAM 2] ├→ [Mesh Network] → [Receiver]
[CAM 3] ┘
```

## Troubleshooting

| Problem | Solution |
|---------|----------|
| Nodes not connecting | Verify MESH_PREFIX, PASSWORD, PORT match |
| ESP32 crashes | Downgrade to ESP32 board version 2.0.x |
| Out of memory | Reduce CHUNK_SIZE or image quality |
| Missing chunks | Increase delay between chunks |
| Slow transfer | Reduce image size or increase CHUNK_SIZE |
| Mesh unstable | Reduce number of nodes or message frequency |

## Performance

### Typical Transfer Times
- **VGA (640x480)**: 5-8 seconds
- **SVGA (800x600)**: 10-15 seconds
- **Direct connection**: Faster
- **Multi-hop**: Slower but extends range

### Range Extension
- **Direct**: ~30 meters indoor
- **With 1 relay**: ~60 meters
- **With 2 relays**: ~90 meters
- Each hop adds ~30m range

### Scalability
- Supports 10+ nodes in mesh
- Multiple cameras can coexist
- All receivers get all images (broadcast)

## Advanced Features

### Adding Relay Nodes
Upload `receive_one_picture.ino` to regular ESP32 devices to create relay nodes. They automatically forward traffic.

### Targeted Delivery
Modify transmitter to use `mesh.sendSingle(nodeId, msg)` instead of `mesh.sendBroadcast(msg)` for point-to-point.

### Priority Messaging
For critical messages, send multiple times:
```cpp
for(int i=0; i<3; i++) {
  mesh.sendBroadcast(msg);
  delay(10);
}
```

### Mesh Statistics
Add to receiver's `loop()`:
```cpp
Serial.printf("Nodes: %d\n", mesh.getNodeList().size());
Serial.printf("Connections: %d\n", mesh.getNodeList().size() - 1);
```

## Mesh vs ESP-NOW

**Choose Mesh when:**
- You need multi-hop communication
- Dynamic network topology required
- Multiple transmitters and receivers
- Self-healing network needed
- Don't want to manage MAC addresses

**Choose ESP-NOW when:**
- You need fastest transfer speed
- Point-to-point communication sufficient
- Fixed network topology
- Lower latency critical

## Device Configuration

Based on `assests/schema`:

### Network A
```
R1 (Main Receiver)
 └── A_SR (Sub-receiver)
      ├── A_T1 (ESP32-CAM)
      └── A_T2 (ESP32-CAM)
```

All devices use same mesh credentials and automatically form network.

## Memory Management

### ESP32-CAM Considerations
- 4MB PSRAM available
- Image buffer allocated dynamically
- Chunk buffer uses ~2KB
- Leave ~50KB free for mesh operations

### Receiver Considerations
- Allocates buffer equal to image size
- Chunk tracking array uses ~100 bytes
- JSON parsing uses ~2KB temporarily

## Best Practices

1. **Start small**: Test with VGA before SVGA
2. **Monitor serial**: Watch for memory warnings
3. **Power supply**: Use 2A supply for ESP32-CAM
4. **Timing**: Allow mesh to stabilize (5-10 seconds) before first image
5. **Testing**: Test range with 1 device pair first
6. **Scaling**: Add nodes gradually
7. **Reliability**: Implement retry logic for critical applications

## Example Use Cases

### Crack Detection System
- Multiple ESP32-CAM modules at different locations
- One or more receivers for monitoring
- Automatic image capture on interval
- Central processing of images

### Security Network
- Cameras at entry points
- Mesh network for communication
- Central receiver stores images
- Motion-triggered capture

### Industrial Monitoring
- Cameras monitor equipment
- Redundant receivers for reliability
- Multi-hop for large facilities
- Regular interval captures

---
**Compatible with**: ESP32 board version 2.0.x  
**Requires**: painlessMesh, ArduinoJson, TaskScheduler

