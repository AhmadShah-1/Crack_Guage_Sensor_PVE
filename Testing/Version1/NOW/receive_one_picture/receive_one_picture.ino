/*
  ESP32 Image Receiver via ESP-NOW
  Receiver Side
  
  Receives image chunks via ESP-NOW and reconstructs the image
  Saves to SD card or displays via Serial
*/

#include <esp_now.h>
#include <WiFi.h>

// ===========================
// Data Structure (must match transmitter)
// ===========================
#define MAX_PACKET_SIZE 240

typedef struct struct_message {
  uint16_t packet_id;
  uint16_t total_packets;
  uint16_t data_length;
  uint8_t data[MAX_PACKET_SIZE];
} struct_message;

struct_message receivedData;

// ===========================
// Image Reception Variables
// ===========================
uint8_t *imageBuffer = NULL;
uint32_t imageSize = 0;
uint16_t packetsReceived = 0;
uint16_t expectedPackets = 0;
bool receivingImage = false;
unsigned long lastPacketTime = 0;
#define TIMEOUT_MS 2000  // Timeout if no packet received for 2 seconds

// ===========================
// Callback when data is received
// ===========================
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  lastPacketTime = millis();
  
  // First packet - initialize buffer
  if (receivedData.packet_id == 0) {
    // Free old buffer if exists
    if (imageBuffer != NULL) {
      free(imageBuffer);
      imageBuffer = NULL;
    }
    
    packetsReceived = 0;
    expectedPackets = receivedData.total_packets;
    imageSize = expectedPackets * MAX_PACKET_SIZE;  // Allocate max size
    
    // Allocate buffer
    imageBuffer = (uint8_t *)malloc(imageSize);
    if (imageBuffer == NULL) {
      Serial.println("Failed to allocate memory for image!");
      return;
    }
    
    receivingImage = true;
    Serial.printf("\n--- Starting image reception ---\n");
    Serial.printf("Expected packets: %d\n", expectedPackets);
    Serial.printf("Estimated size: ~%d bytes\n", imageSize);
  }
  
  // Validate packet
  if (!receivingImage || imageBuffer == NULL) {
    return;
  }
  
  if (receivedData.packet_id >= expectedPackets) {
    Serial.printf("Invalid packet ID: %d\n", receivedData.packet_id);
    return;
  }
  
  // Copy data to buffer
  memcpy(imageBuffer + (receivedData.packet_id * MAX_PACKET_SIZE), 
         receivedData.data, 
         receivedData.data_length);
  
  packetsReceived++;
  
  // Progress indicator
  if (packetsReceived % 20 == 0 || packetsReceived == expectedPackets) {
    Serial.printf("Received: %d/%d packets (%.1f%%)\n", 
                  packetsReceived, 
                  expectedPackets,
                  (packetsReceived * 100.0) / expectedPackets);
  }
  
  // Check if all packets received
  if (packetsReceived >= expectedPackets) {
    // Calculate actual image size from last packet
    imageSize = (expectedPackets - 1) * MAX_PACKET_SIZE + receivedData.data_length;
    
    Serial.println("\n=== Image Reception Complete ===");
    Serial.printf("Total packets received: %d\n", packetsReceived);
    Serial.printf("Image size: %d bytes\n", imageSize);
    
    processImage();
    
    // Reset for next image
    receivingImage = false;
  }
}

// ===========================
// Process Received Image
// ===========================
void processImage() {
  if (imageBuffer == NULL || imageSize == 0) {
    Serial.println("No image data to process");
    return;
  }
  
  // Verify JPEG header
  if (imageSize < 2 || imageBuffer[0] != 0xFF || imageBuffer[1] != 0xD8) {
    Serial.println("ERROR: Invalid JPEG header!");
    return;
  }
  
  Serial.println("Valid JPEG image received!");
  
  // Here you can:
  // 1. Save to SD card
  // 2. Send to serial as base64
  // 3. Process the image
  // 4. Send to a web server
  
  // Example: Print first few bytes as hex
  Serial.print("First 16 bytes (hex): ");
  for (int i = 0; i < 16 && i < imageSize; i++) {
    Serial.printf("%02X ", imageBuffer[i]);
  }
  Serial.println();
  
  // Example: Check for JPEG end marker
  if (imageSize >= 2) {
    if (imageBuffer[imageSize-2] == 0xFF && imageBuffer[imageSize-1] == 0xD9) {
      Serial.println("JPEG end marker confirmed!");
    } else {
      Serial.println("WARNING: JPEG end marker not found!");
    }
  }
  
  Serial.println("\nTo save image to SD card or send via serial,");
  Serial.println("implement your preferred method in processImage()");
  Serial.println("================================\n");
}

// ===========================
// Check for timeout
// ===========================
void checkTimeout() {
  if (receivingImage && (millis() - lastPacketTime > TIMEOUT_MS)) {
    Serial.printf("\nTimeout! Only received %d/%d packets\n", 
                  packetsReceived, expectedPackets);
    receivingImage = false;
    
    // Free buffer
    if (imageBuffer != NULL) {
      free(imageBuffer);
      imageBuffer = NULL;
    }
  }
}

// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32 Image Receiver");
  Serial.println("====================");
  
  // Set WiFi to Station mode
  WiFi.mode(WIFI_STA);
  
  // Print MAC Address
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
  Serial.println("Use this address in the transmitter code!");
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    while (1) delay(1000);
  }
  
  Serial.println("ESP-NOW initialized");
  
  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("\nWaiting for image data...");
  Serial.println("====================\n");
}

// ===========================
// Loop
// ===========================
void loop() {
  checkTimeout();
  delay(100);
}

