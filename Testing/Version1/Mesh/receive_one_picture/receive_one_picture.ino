/*
  ESP32 Image Receiver via painlessMesh
  Receiver Side
  
  Receives image chunks via Mesh network and reconstructs the image
  
  Note: Install painlessMesh library via Arduino Library Manager
  Note: Use ESP32 board version 2.0.x (not 3.x) for painlessMesh compatibility
*/

#include "painlessMesh.h"
#include <Arduino_JSON.h>

// ===========================
// Mesh Network Configuration
// ===========================
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// Image reception variables
uint8_t *imageBuffer = NULL;
uint32_t imageSize = 0;
uint16_t totalChunks = 0;
uint16_t chunksReceived = 0;
uint16_t chunkSize = 0;
uint32_t senderNodeId = 0;
bool receivingImage = false;
bool *chunkReceived = NULL;  // Track which chunks we have

// ===========================
// Process Start Message
// ===========================
void processStartMessage(JSONVar data) {
  // Free old data if exists
  if (imageBuffer != NULL) {
    free(imageBuffer);
    imageBuffer = NULL;
  }
  if (chunkReceived != NULL) {
    free(chunkReceived);
    chunkReceived = NULL;
  }
  
  senderNodeId = (int)data["node"];
  imageSize = (int)data["size"];
  totalChunks = (int)data["chunks"];
  chunkSize = (int)data["chunk_size"];
  chunksReceived = 0;
  
  // Allocate buffer
  imageBuffer = (uint8_t *)malloc(imageSize);
  chunkReceived = (bool *)calloc(totalChunks, sizeof(bool));
  
  if (imageBuffer == NULL || chunkReceived == NULL) {
    Serial.println("Failed to allocate memory!");
    return;
  }
  
  receivingImage = true;
  
  Serial.println("\n=== Starting Image Reception ===");
  Serial.printf("From Node: %u\n", senderNodeId);
  Serial.printf("Image size: %d bytes\n", imageSize);
  Serial.printf("Total chunks: %d\n", totalChunks);
  Serial.printf("Chunk size: %d bytes\n", chunkSize);
}

// ===========================
// Process Chunk Message
// ===========================
void processChunkMessage(JSONVar data) {
  if (!receivingImage || imageBuffer == NULL) {
    return;
  }
  
  uint32_t nodeId = (int)data["node"];
  if (nodeId != senderNodeId) {
    return;  // Ignore chunks from other nodes
  }
  
  uint16_t chunkId = (int)data["chunk_id"];
  uint16_t total = (int)data["total"];
  String hexData = (const char*)data["data"];
  
  // Validate
  if (chunkId >= totalChunks) {
    Serial.printf("Invalid chunk ID: %d\n", chunkId);
    return;
  }
  
  // Skip if already received
  if (chunkReceived[chunkId]) {
    return;
  }
  
  // Convert hex string to bytes
  uint16_t offset = chunkId * chunkSize;
  uint16_t dataLen = hexData.length() / 2;
  
  for (uint16_t i = 0; i < dataLen && (offset + i) < imageSize; i++) {
    String byteStr = hexData.substring(i * 2, i * 2 + 2);
    imageBuffer[offset + i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
  }
  
  chunkReceived[chunkId] = true;
  chunksReceived++;
  
  // Progress
  if (chunksReceived % 5 == 0 || chunksReceived == totalChunks) {
    Serial.printf("Received: %d/%d chunks (%.1f%%)\n", 
                  chunksReceived, totalChunks,
                  (chunksReceived * 100.0) / totalChunks);
  }
}

// ===========================
// Process End Message
// ===========================
void processEndMessage(JSONVar data) {
  if (!receivingImage) {
    return;
  }
  
  uint32_t nodeId = (int)data["node"];
  if (nodeId != senderNodeId) {
    return;
  }
  
  Serial.println("\n=== Image Reception Complete ===");
  Serial.printf("Received %d/%d chunks\n", chunksReceived, totalChunks);
  
  // Check for missing chunks
  int missingCount = 0;
  for (int i = 0; i < totalChunks; i++) {
    if (!chunkReceived[i]) {
      if (missingCount == 0) {
        Serial.println("Missing chunks:");
      }
      Serial.printf("  %d", i);
      missingCount++;
      if (missingCount % 10 == 0) Serial.println();
    }
  }
  
  if (missingCount > 0) {
    Serial.printf("\nTotal missing: %d chunks\n", missingCount);
  } else {
    Serial.println("All chunks received successfully!");
    processImage();
  }
  
  receivingImage = false;
  
  Serial.println("================================\n");
}

// ===========================
// Process Complete Image
// ===========================
void processImage() {
  if (imageBuffer == NULL || imageSize == 0) {
    return;
  }
  
  // Verify JPEG header
  if (imageSize < 2 || imageBuffer[0] != 0xFF || imageBuffer[1] != 0xD8) {
    Serial.println("ERROR: Invalid JPEG header!");
    return;
  }
  
  Serial.println("Valid JPEG image received!");
  
  // Print first bytes
  Serial.print("First 16 bytes (hex): ");
  for (int i = 0; i < 16 && i < imageSize; i++) {
    Serial.printf("%02X ", imageBuffer[i]);
  }
  Serial.println();
  
  // Check JPEG end marker
  if (imageSize >= 2) {
    if (imageBuffer[imageSize-2] == 0xFF && imageBuffer[imageSize-1] == 0xD9) {
      Serial.println("JPEG end marker confirmed!");
    } else {
      Serial.println("WARNING: JPEG end marker not found!");
    }
  }
  
  // Here you can:
  // 1. Save to SD card
  // 2. Send to serial
  // 3. Process the image
  // 4. Forward to another network
  
  Serial.println("\nImage ready for processing!");
}

// ===========================
// Mesh Callbacks
// ===========================
void receivedCallback(uint32_t from, String &msg) {
  // Parse JSON
  JSONVar data = JSON.parse(msg.c_str());
  
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Failed to parse JSON");
    return;
  }
  
  String type = (const char*)data["type"];
  
  if (type == "image_start") {
    processStartMessage(data);
  } 
  else if (type == "image_chunk") {
    processChunkMessage(data);
  } 
  else if (type == "image_end") {
    processEndMessage(data);
  }
  else {
    // Other message types
    Serial.printf("Received from %u: %s\n", from, msg.c_str());
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("Connection changed");
  Serial.printf("Nodes in mesh: %d\n", mesh.getNodeList().size());
}

void nodeTimeAdjustedCallback(int32_t offset) {
  Serial.printf("Time adjusted: %d\n", offset);
}

// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32 Mesh Image Receiver");
  Serial.println("==========================");
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  Serial.print("Node ID: ");
  Serial.println(mesh.getNodeId());
  Serial.println("\nWaiting for images...");
  Serial.println("==========================\n");
}

// ===========================
// Loop
// ===========================
void loop() {
  mesh.update();
}

