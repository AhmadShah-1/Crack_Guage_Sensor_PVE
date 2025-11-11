/*
  ESP32 Sub-Receiver Relay
  For Hybrid ESP-NOW + Mesh Architecture
 
  Receives images from ESP32-CAM modules via ESP-NOW
  Forwards images to Main Receiver via painlessMesh
 
  CONFIGURATION REQUIRED:
  - Set SUBRECEIVER_ID to match your device (A_SR or B_SR)
 
  Note: Use ESP32 board version 2.0.x for painlessMesh compatibility
*/
 
#include <esp_now.h>
#include <WiFi.h>
#include "painlessMesh.h"
#include <Arduino_JSON.h>
 
// ===========================
// DEVICE CONFIGURATION
// ===========================
#define SUBRECEIVER_ID "A_SR"  // OPTIONS: A_SR, B_SR
 
// ===========================
// Mesh Network Configuration
// ===========================
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555
 
// ===========================
// Configuration
// ===========================
#define DATA_PACKET_SIZE 230
#define MESH_CHUNK_SIZE 1000
#define MAX_CAMERAS 4  // Maximum simultaneous camera receptions
#define MESH_MAX_RETRIES 3  // Retry attempts for mesh transmission
#define MESH_RETRY_DELAY 50  // Delay between retries (ms)
 
// ===========================
// ESP-NOW Packet Structure (must match transmitter)
// ===========================
typedef struct esp_now_packet {
  char device_id[10];
  uint16_t packet_id;
  uint16_t total_packets;
  uint16_t data_length;
  uint8_t data[DATA_PACKET_SIZE];
} esp_now_packet;
 
// ===========================
// Image Buffer Structure
// ===========================
struct ImageBuffer {
  uint8_t *data;
  uint32_t size;
  uint16_t packetsReceived;
  uint16_t totalPackets;
  char cameraID[10];
  bool receiving;
  bool *packetReceived;  // Track which packets received
  unsigned long lastPacketTime;
};
 
// Buffers for simultaneous reception from multiple cameras
ImageBuffer cameraBuffers[MAX_CAMERAS];
 
// Mesh
Scheduler userScheduler;
painlessMesh mesh;
 
// Timeout tracking
#define RECEPTION_TIMEOUT 3000  // 3 seconds
 
// ===========================
// Initialize Image Buffer
// ===========================
void initBuffer(ImageBuffer &buffer) {
  buffer.data = NULL;
  buffer.size = 0;
  buffer.packetsReceived = 0;
  buffer.totalPackets = 0;
  buffer.cameraID[0] = '\0';
  buffer.receiving = false;
  buffer.packetReceived = NULL;
  buffer.lastPacketTime = 0;
}
 
// ===========================
// Free Image Buffer
// ===========================
void freeBuffer(ImageBuffer &buffer) {
  if (buffer.data != NULL) {
    free(buffer.data);
    buffer.data = NULL;
  }
  if (buffer.packetReceived != NULL) {
    free(buffer.packetReceived);
    buffer.packetReceived = NULL;
  }
  initBuffer(buffer);
}
 
// ===========================
// Get Buffer for Camera
// ===========================
ImageBuffer* getBufferForCamera(const char* cameraID) {
  // Check if already receiving from this camera
  for (int i = 0; i < MAX_CAMERAS; i++) {
    if (cameraBuffers[i].receiving && strcmp(cameraBuffers[i].cameraID, cameraID) == 0) {
      return &cameraBuffers[i];
    }
  }
 
  // Assign to free buffer
  for (int i = 0; i < MAX_CAMERAS; i++) {
    if (!cameraBuffers[i].receiving) {
      return &cameraBuffers[i];
    }
  }
 
  // No free buffer
  Serial.printf("[%s] WARNING: No free buffer for %s (all %d buffers in use)\n",
                SUBRECEIVER_ID, cameraID, MAX_CAMERAS);
  return NULL;
}
 
// ===========================
// ESP-NOW Receive Callback
// ===========================
// Note: For ESP32 core 3.x+ (IDF 5.x)
void OnDataRecv(const esp_now_recv_info *recv_info, const uint8_t *incomingData, int len) {
  esp_now_packet receivedPacket;
  memcpy(&receivedPacket, incomingData, sizeof(receivedPacket));
 
  // Get or assign buffer for this camera
  ImageBuffer *buffer = getBufferForCamera(receivedPacket.device_id);
  if (buffer == NULL) {
    return;  // No buffer available
  }
 
  buffer->lastPacketTime = millis();
 
  // First packet - initialize buffer
  if (receivedPacket.packet_id == 0) {
    // Free old data if exists
    if (buffer->data != NULL) {
      free(buffer->data);
    }
    if (buffer->packetReceived != NULL) {
      free(buffer->packetReceived);
    }
   
    // Initialize buffer
    strncpy(buffer->cameraID, receivedPacket.device_id, sizeof(buffer->cameraID) - 1);
    buffer->cameraID[sizeof(buffer->cameraID) - 1] = '\0';
    buffer->packetsReceived = 0;
    buffer->totalPackets = receivedPacket.total_packets;
    buffer->size = buffer->totalPackets * DATA_PACKET_SIZE;
   
    // Allocate memory for image data
    buffer->data = (uint8_t *)malloc(buffer->size);
    if (buffer->data == NULL) {
      Serial.printf("[%s] ERROR: Failed to allocate memory for %s\n",
                    SUBRECEIVER_ID, buffer->cameraID);
      initBuffer(*buffer);
      return;
    }
   
    // Allocate memory for packet tracking
    buffer->packetReceived = (bool *)malloc(buffer->totalPackets * sizeof(bool));
    if (buffer->packetReceived == NULL) {
      Serial.printf("[%s] ERROR: Failed to allocate packet tracking for %s\n",
                    SUBRECEIVER_ID, buffer->cameraID);
      free(buffer->data);
      initBuffer(*buffer);
      return;
    }
   
    // Initialize packet tracking array
    for (uint16_t i = 0; i < buffer->totalPackets; i++) {
      buffer->packetReceived[i] = false;
    }
   
    buffer->receiving = true;
    Serial.printf("[%s] Started receiving from %s (%d packets)\n",
                  SUBRECEIVER_ID, buffer->cameraID, buffer->totalPackets);
  }
 
  // Validate packet
  if (!buffer->receiving) {
    return;
  }
 
  if (receivedPacket.packet_id >= buffer->totalPackets) {
    Serial.printf("[%s] Invalid packet ID %d from %s\n",
                  SUBRECEIVER_ID, receivedPacket.packet_id, buffer->cameraID);
    return;
  }
 
  // Skip if already received (duplicate)
  if (buffer->packetReceived[receivedPacket.packet_id]) {
    return;  // Already have this packet
  }
 
  // Copy packet data to buffer
  uint32_t offset = receivedPacket.packet_id * DATA_PACKET_SIZE;
  memcpy(buffer->data + offset, receivedPacket.data, receivedPacket.data_length);
 
  // Mark packet as received
  buffer->packetReceived[receivedPacket.packet_id] = true;
  buffer->packetsReceived++;
 
  // Progress indicator
  if (buffer->packetsReceived % 20 == 0 || buffer->packetsReceived == buffer->totalPackets) {
    Serial.printf("[%s] %s: %d/%d packets (%.1f%%)\n",
                  SUBRECEIVER_ID, buffer->cameraID,
                  buffer->packetsReceived, buffer->totalPackets,
                  (buffer->packetsReceived * 100.0) / buffer->totalPackets);
  }
 
  // Check if complete
  if (buffer->packetsReceived >= buffer->totalPackets) {
    // Verify all packets received
    uint16_t missingCount = 0;
    for (uint16_t i = 0; i < buffer->totalPackets; i++) {
      if (!buffer->packetReceived[i]) {
        if (missingCount == 0) {
          Serial.printf("[%s] Missing packets from %s: ", SUBRECEIVER_ID, buffer->cameraID);
        }
        Serial.printf("%d ", i);
        missingCount++;
      }
    }
   
    if (missingCount > 0) {
      Serial.printf("\n[%s] ERROR: %d packets missing from %s - discarding image\n",
                    SUBRECEIVER_ID, missingCount, buffer->cameraID);
      freeBuffer(*buffer);
      return;
    }
   
    // Calculate actual size from last packet
    uint32_t actualSize = (buffer->totalPackets - 1) * DATA_PACKET_SIZE + receivedPacket.data_length;
    buffer->size = actualSize;
   
    Serial.printf("[%s] Image complete from %s: %d bytes\n",
                  SUBRECEIVER_ID, buffer->cameraID, buffer->size);
   
    // Forward to mesh
    forwardImageToMesh(*buffer);
   
    // Free buffer
    freeBuffer(*buffer);
  }
}
 
// ===========================
// Send Mesh Message with Retry
// ===========================
bool sendMeshMessageWithRetry(String &message, const char* messageType) {
  for (int retry = 0; retry < MESH_MAX_RETRIES; retry++) {
    bool success = mesh.sendBroadcast(message);
    
    if (success) {
      return true;
    }
    
    // Failed, retry
    if (retry < MESH_MAX_RETRIES - 1) {
      Serial.printf("[%s] Mesh send failed for %s (attempt %d/%d), retrying...\n",
                    SUBRECEIVER_ID, messageType, retry + 1, MESH_MAX_RETRIES);
      delay(MESH_RETRY_DELAY);
    }
  }
  
  // All retries failed
  Serial.printf("[%s] ERROR: Failed to send %s after %d attempts\n",
                SUBRECEIVER_ID, messageType, MESH_MAX_RETRIES);
  return false;
}

// ===========================
// Forward Image to Mesh Network
// ===========================
void forwardImageToMesh(ImageBuffer &buffer) {
  Serial.printf("[%s] Forwarding %s image to mesh...\n", SUBRECEIVER_ID, buffer.cameraID);
 
  // Calculate number of chunks
  uint16_t totalChunks = (buffer.size + MESH_CHUNK_SIZE - 1) / MESH_CHUNK_SIZE;
 
  // Send start message
  JSONVar startMsg;
  startMsg["type"] = "image_start";
  startMsg["camera_id"] = buffer.cameraID;
  startMsg["subreceiver_id"] = SUBRECEIVER_ID;
  startMsg["size"] = (int)buffer.size;
  startMsg["chunks"] = totalChunks;
 
  String startStr = JSON.stringify(startMsg);
  if (!sendMeshMessageWithRetry(startStr, "START")) {
    Serial.printf("[%s] FATAL: Could not send start message for %s\n",
                  SUBRECEIVER_ID, buffer.cameraID);
    return;
  }
  delay(50);  // Small delay for message processing
 
  // Send data chunks with retry
  int failedChunks = 0;
  for (uint16_t i = 0; i < totalChunks; i++) {
    uint32_t offset = i * MESH_CHUNK_SIZE;
    uint16_t chunkLen = min((uint16_t)MESH_CHUNK_SIZE, (uint16_t)(buffer.size - offset));
   
    // Convert chunk to hex string
    String hexData = "";
    hexData.reserve(chunkLen * 2);
    for (uint16_t j = 0; j < chunkLen; j++) {
      char buf[3];
      sprintf(buf, "%02X", buffer.data[offset + j]);
      hexData += buf;
    }
   
    // Create chunk message
    JSONVar chunkMsg;
    chunkMsg["type"] = "image_chunk";
    chunkMsg["camera_id"] = buffer.cameraID;
    chunkMsg["subreceiver_id"] = SUBRECEIVER_ID;
    chunkMsg["chunk_id"] = i;
    chunkMsg["total"] = totalChunks;
    chunkMsg["data"] = hexData;
   
    String chunkStr = JSON.stringify(chunkMsg);
    
    // Send with retry
    char chunkLabel[20];
    sprintf(chunkLabel, "CHUNK_%d", i);
    if (!sendMeshMessageWithRetry(chunkStr, chunkLabel)) {
      failedChunks++;
      Serial.printf("[%s] WARNING: Chunk %d failed after all retries\n",
                    SUBRECEIVER_ID, i);
    }
   
    // Progress
    if ((i + 1) % 5 == 0 || (i + 1) == totalChunks) {
      Serial.printf("[%s] Mesh forward: %d/%d chunks (%.1f%%) - %d failed\n",
                    SUBRECEIVER_ID, i + 1, totalChunks,
                    ((i + 1) * 100.0) / totalChunks, failedChunks);
    }
   
    delay(20);  // Delay between chunks for mesh stability
  }
 
  // Send end message
  JSONVar endMsg;
  endMsg["type"] = "image_end";
  endMsg["camera_id"] = buffer.cameraID;
  endMsg["subreceiver_id"] = SUBRECEIVER_ID;
  endMsg["chunks"] = totalChunks;
 
  String endStr = JSON.stringify(endMsg);
  if (!sendMeshMessageWithRetry(endStr, "END")) {
    Serial.printf("[%s] ERROR: Could not send end message for %s\n",
                  SUBRECEIVER_ID, buffer.cameraID);
  }
 
  if (failedChunks > 0) {
    Serial.printf("[%s] Forward complete for %s with %d failed chunks (may have missing data)\n",
                  SUBRECEIVER_ID, buffer.cameraID, failedChunks);
  } else {
    Serial.printf("[%s] Forward complete for %s - all chunks sent successfully\n",
                  SUBRECEIVER_ID, buffer.cameraID);
  }
}
 
// ===========================
// Check for Reception Timeouts
// ===========================
void checkTimeouts() {
  unsigned long now = millis();
 
  for (int i = 0; i < MAX_CAMERAS; i++) {
    if (cameraBuffers[i].receiving && (now - cameraBuffers[i].lastPacketTime > RECEPTION_TIMEOUT)) {
      Serial.printf("[%s] Timeout receiving from %s (%d/%d packets)\n",
                    SUBRECEIVER_ID, cameraBuffers[i].cameraID,
                    cameraBuffers[i].packetsReceived, cameraBuffers[i].totalPackets);
      freeBuffer(cameraBuffers[i]);
    }
  }
}
 
// ===========================
// Mesh Callbacks
// ===========================
void receivedCallback(uint32_t from, String &msg) {
  // Sub-receiver doesn't need to process mesh messages
  // Only forwards to mesh
}
 
void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[%s] Mesh: New connection %u\n", SUBRECEIVER_ID, nodeId);
}
 
void changedConnectionCallback() {
  Serial.printf("[%s] Mesh: Connection changed\n", SUBRECEIVER_ID);
}
 
void nodeTimeAdjustedCallback(int32_t offset) {
  // Time sync callback
}
 
// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);
 
  Serial.printf("\n\n[%s] Sub-Receiver Relay\n", SUBRECEIVER_ID);
  Serial.println("========================================");
 
  // Initialize buffers
  for (int i = 0; i < MAX_CAMERAS; i++) {
    initBuffer(cameraBuffers[i]);
  }
 
  // Set WiFi to Station mode
  WiFi.mode(WIFI_STA);
 
  // Print MAC Address
  Serial.printf("[%s] MAC Address: %s\n", SUBRECEIVER_ID, WiFi.macAddress().c_str());
 
  // Initialize ESP-NOW first
  if (esp_now_init() != ESP_OK) {
    Serial.printf("[%s] FATAL: ESP-NOW init failed!\n", SUBRECEIVER_ID);
    while (1) delay(1000);
  }
 
  Serial.printf("[%s] ESP-NOW initialized\n", SUBRECEIVER_ID);
 
  // Register ESP-NOW receive callback
  esp_now_register_recv_cb(OnDataRecv);
 
  // Initialize Mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
 
  Serial.printf("[%s] Mesh initialized\n", SUBRECEIVER_ID);
  Serial.printf("[%s] Mesh Node ID: %u\n", SUBRECEIVER_ID, mesh.getNodeId());
  Serial.println("========================================");
  Serial.printf("[%s] Ready to receive from cameras and forward to mesh\n", SUBRECEIVER_ID);
  Serial.println("========================================\n");
}
 
// ===========================
// Loop
// ===========================
void loop() {
  mesh.update();
  checkTimeouts();
  delay(10);
}
