/*
  ESP32 Main Receiver
  For Hybrid ESP-NOW + Mesh Architecture
  
  Receives images from Sub-Receivers via painlessMesh
  Outputs structured data to Serial for Raspberry Pi
  
  Note: Use ESP32 board version 2.0.x for painlessMesh compatibility
*/

#include "painlessMesh.h"
#include <Arduino_JSON.h>

// ===========================
// Mesh Network Configuration
// ===========================
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555

// ===========================
// Configuration
// ===========================
#define MAX_IMAGES 4  // Maximum simultaneous image receptions

// ===========================
// Image Reception Structure
// ===========================
struct ImageReception {
  uint8_t *data;
  uint32_t size;
  uint16_t chunksReceived;
  uint16_t totalChunks;
  char cameraID[10];
  char subreceiverID[10];
  bool receiving;
  bool *chunkReceived;  // Track which chunks received
  unsigned long startTime;
  unsigned long lastChunkTime;
};

ImageReception imageReceptions[MAX_IMAGES];

Scheduler userScheduler;
painlessMesh mesh;

#define TIMEOUT_MS 10000  // 10 second timeout

// ===========================
// Initialize Image Reception
// ===========================
void initReception(ImageReception &reception) {
  reception.data = NULL;
  reception.size = 0;
  reception.chunksReceived = 0;
  reception.totalChunks = 0;
  reception.cameraID[0] = '\0';
  reception.subreceiverID[0] = '\0';
  reception.receiving = false;
  reception.chunkReceived = NULL;
  reception.startTime = 0;
  reception.lastChunkTime = 0;
}

// ===========================
// Free Image Reception
// ===========================
void freeReception(ImageReception &reception) {
  if (reception.data != NULL) {
    free(reception.data);
    reception.data = NULL;
  }
  if (reception.chunkReceived != NULL) {
    free(reception.chunkReceived);
    reception.chunkReceived = NULL;
  }
  initReception(reception);
}

// ===========================
// Get Reception Slot for Camera
// ===========================
ImageReception* getReceptionSlot(const char* cameraID, const char* subreceiverID) {
  // Check if already receiving from this camera
  for (int i = 0; i < MAX_IMAGES; i++) {
    if (imageReceptions[i].receiving && 
        strcmp(imageReceptions[i].cameraID, cameraID) == 0) {
      return &imageReceptions[i];
    }
  }
  
  // Find free slot
  for (int i = 0; i < MAX_IMAGES; i++) {
    if (!imageReceptions[i].receiving) {
      return &imageReceptions[i];
    }
  }
  
  Serial.println("[R1] WARNING: No free reception slot");
  return NULL;
}

// ===========================
// Process Start Message
// ===========================
void processStartMessage(JSONVar data) {
  const char* cameraID = (const char*)data["camera_id"];
  const char* subreceiverID = (const char*)data["subreceiver_id"];
  int size = (int)data["size"];
  int chunks = (int)data["chunks"];
  
  ImageReception *reception = getReceptionSlot(cameraID, subreceiverID);
  if (reception == NULL) {
    return;
  }
  
  // Free old data if exists
  freeReception(*reception);
  
  // Initialize reception
  strncpy(reception->cameraID, cameraID, sizeof(reception->cameraID) - 1);
  reception->cameraID[sizeof(reception->cameraID) - 1] = '\0';
  strncpy(reception->subreceiverID, subreceiverID, sizeof(reception->subreceiverID) - 1);
  reception->subreceiverID[sizeof(reception->subreceiverID) - 1] = '\0';
  reception->size = size;
  reception->totalChunks = chunks;
  reception->chunksReceived = 0;
  reception->startTime = millis();
  reception->lastChunkTime = millis();
  
  // Allocate buffers
  reception->data = (uint8_t *)malloc(size);
  reception->chunkReceived = (bool *)calloc(chunks, sizeof(bool));
  
  if (reception->data == NULL || reception->chunkReceived == NULL) {
    Serial.printf("[R1] ERROR: Failed to allocate memory for %s from %s\n", 
                  cameraID, subreceiverID);
    freeReception(*reception);
    return;
  }
  
  reception->receiving = true;
  
  Serial.printf("\n[R1] === Starting Reception ===\n");
  Serial.printf("[R1] Camera: %s\n", cameraID);
  Serial.printf("[R1] Sub-receiver: %s\n", subreceiverID);
  Serial.printf("[R1] Size: %d bytes\n", size);
  Serial.printf("[R1] Chunks: %d\n", chunks);
}

// ===========================
// Process Chunk Message
// ===========================
void processChunkMessage(JSONVar data) {
  const char* cameraID = (const char*)data["camera_id"];
  int chunkId = (int)data["chunk_id"];
  String hexData = (const char*)data["data"];
  
  // Find reception
  ImageReception *reception = NULL;
  for (int i = 0; i < MAX_IMAGES; i++) {
    if (imageReceptions[i].receiving && 
        strcmp(imageReceptions[i].cameraID, cameraID) == 0) {
      reception = &imageReceptions[i];
      break;
    }
  }
  
  if (reception == NULL || !reception->receiving) {
    return;
  }
  
  reception->lastChunkTime = millis();
  
  // Validate chunk ID
  if (chunkId >= reception->totalChunks) {
    Serial.printf("[R1] Invalid chunk ID %d from %s\n", chunkId, cameraID);
    return;
  }
  
  // Skip if already received
  if (reception->chunkReceived[chunkId]) {
    return;
  }
  
  // Decode hex data
  int chunkSize = 1000;  // MESH_CHUNK_SIZE
  uint32_t offset = chunkId * chunkSize;
  int hexLen = hexData.length();
  int dataLen = hexLen / 2;
  
  for (int i = 0; i < dataLen && (offset + i) < reception->size; i++) {
    String byteStr = hexData.substring(i * 2, i * 2 + 2);
    reception->data[offset + i] = (uint8_t)strtol(byteStr.c_str(), NULL, 16);
  }
  
  reception->chunkReceived[chunkId] = true;
  reception->chunksReceived++;
  
  // Progress
  if (reception->chunksReceived % 5 == 0 || reception->chunksReceived == reception->totalChunks) {
    Serial.printf("[R1] %s: %d/%d chunks (%.1f%%)\n",
                  reception->cameraID, reception->chunksReceived, 
                  reception->totalChunks,
                  (reception->chunksReceived * 100.0) / reception->totalChunks);
  }
}

// ===========================
// Process End Message
// ===========================
void processEndMessage(JSONVar data) {
  const char* cameraID = (const char*)data["camera_id"];
  
  // Find reception
  ImageReception *reception = NULL;
  for (int i = 0; i < MAX_IMAGES; i++) {
    if (imageReceptions[i].receiving && 
        strcmp(imageReceptions[i].cameraID, cameraID) == 0) {
      reception = &imageReceptions[i];
      break;
    }
  }
  
  if (reception == NULL || !reception->receiving) {
    return;
  }
  
  unsigned long transferTime = millis() - reception->startTime;
  
  Serial.printf("\n[R1] === Reception Complete ===\n");
  Serial.printf("[R1] Camera: %s\n", reception->cameraID);
  Serial.printf("[R1] Sub-receiver: %s\n", reception->subreceiverID);
  Serial.printf("[R1] Received: %d/%d chunks\n", 
                reception->chunksReceived, reception->totalChunks);
  Serial.printf("[R1] Transfer time: %lu ms\n", transferTime);
  
  // Check for missing chunks
  int missingCount = 0;
  for (int i = 0; i < reception->totalChunks; i++) {
    if (!reception->chunkReceived[i]) {
      missingCount++;
    }
  }
  
  if (missingCount > 0) {
    Serial.printf("[R1] WARNING: %d chunks missing\n", missingCount);
  } else {
    Serial.println("[R1] All chunks received!");
    outputImageToSerial(*reception);
  }
  
  Serial.println("[R1] ================================\n");
  
  // Free reception
  freeReception(*reception);
}

// ===========================
// Output Image to Serial (for Raspberry Pi)
// ===========================
void outputImageToSerial(ImageReception &reception) {
  // Verify JPEG
  if (reception.size < 2 || reception.data[0] != 0xFF || reception.data[1] != 0xD8) {
    Serial.println("[R1] ERROR: Invalid JPEG header");
    return;
  }
  
  Serial.println("[R1] Valid JPEG confirmed");
  
  // Structured output for Raspberry Pi
  Serial.println("\n===IMAGE_START===");
  Serial.printf("CAMERA: %s\n", reception.cameraID);
  Serial.printf("SUBRECEIVER: %s\n", reception.subreceiverID);
  Serial.printf("SIZE: %d\n", reception.size);
  Serial.printf("TIMESTAMP: %lu\n", millis());
  Serial.println("===DATA===");
  
  // Output as hex (Raspberry Pi can decode)
  for (uint32_t i = 0; i < reception.size; i++) {
    Serial.printf("%02X", reception.data[i]);
    if ((i + 1) % 64 == 0) {
      Serial.println();  // New line every 64 bytes for readability
    }
  }
  
  Serial.println("\n===IMAGE_END===\n");
  
  Serial.printf("[R1] Image data sent to serial (%d bytes)\n", reception.size);
}

// ===========================
// Check for Timeouts
// ===========================
void checkTimeouts() {
  unsigned long now = millis();
  
  for (int i = 0; i < MAX_IMAGES; i++) {
    if (imageReceptions[i].receiving && 
        (now - imageReceptions[i].lastChunkTime > TIMEOUT_MS)) {
      Serial.printf("[R1] Timeout: %s from %s (%d/%d chunks)\n",
                    imageReceptions[i].cameraID,
                    imageReceptions[i].subreceiverID,
                    imageReceptions[i].chunksReceived,
                    imageReceptions[i].totalChunks);
      freeReception(imageReceptions[i]);
    }
  }
}

// ===========================
// Mesh Receive Callback
// ===========================
void receivedCallback(uint32_t from, String &msg) {
  JSONVar data = JSON.parse(msg.c_str());
  
  if (JSON.typeof(data) == "undefined") {
    return;
  }
  
  String type = (const char*)data["type"];
  
  if (type == "image_start") {
    processStartMessage(data);
  } else if (type == "image_chunk") {
    processChunkMessage(data);
  } else if (type == "image_end") {
    processEndMessage(data);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("[R1] Mesh: New connection %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("[R1] Mesh: Connections changed\n");
  Serial.printf("[R1] Mesh: %d nodes connected\n", mesh.getNodeList().size());
}

void nodeTimeAdjustedCallback(int32_t offset) {
  // Time sync
}

// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\n[R1] Main Receiver");
  Serial.println("========================================");
  
  // Initialize reception slots
  for (int i = 0; i < MAX_IMAGES; i++) {
    initReception(imageReceptions[i]);
  }
  
  // Initialize Mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  Serial.printf("[R1] Mesh initialized\n");
  Serial.printf("[R1] Node ID: %u\n", mesh.getNodeId());
  Serial.println("========================================");
  Serial.println("[R1] Waiting for images from sub-receivers...");
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

