/*
  ESP32 Image Receiver with SD Card Storage
  Receives images via ESP-NOW and saves to SD card
  
  SD Card Wiring (for regular ESP32):
  SD Card Pin | ESP32 Pin
  ------------|-----------
  CS          | GPIO 5
  MOSI        | GPIO 23
  CLK/SCK     | GPIO 18
  MISO        | GPIO 19
  VCC         | 3.3V
  GND         | GND
*/

#include <esp_now.h>
#include <WiFi.h>
#include "FS.h"
#include "SD.h"
#include <SPI.h>

// ===========================
// SD Card Configuration
// ===========================
#define SD_CS_PIN 5
#define SD_MOSI_PIN 23
#define SD_SCK_PIN 18
#define SD_MISO_PIN 19

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
unsigned long imageStartTime = 0;
uint32_t imageCounter = 0;  // Counter for unique filenames
#define TIMEOUT_MS 2000

// ===========================
// Initialize SD Card
// ===========================
bool initSDCard() {
  SPI.begin(SD_SCK_PIN, SD_MISO_PIN, SD_MOSI_PIN, SD_CS_PIN);
  
  if (!SD.begin(SD_CS_PIN)) {
    Serial.println("SD Card Mount Failed!");
    return false;
  }
  
  uint8_t cardType = SD.cardType();
  if (cardType == CARD_NONE) {
    Serial.println("No SD card attached");
    return false;
  }
  
  Serial.print("SD Card Type: ");
  if (cardType == CARD_MMC) {
    Serial.println("MMC");
  } else if (cardType == CARD_SD) {
    Serial.println("SDSC");
  } else if (cardType == CARD_SDHC) {
    Serial.println("SDHC");
  } else {
    Serial.println("UNKNOWN");
  }
  
  uint64_t cardSize = SD.cardSize() / (1024 * 1024);
  Serial.printf("SD Card Size: %lluMB\n", cardSize);
  
  // Create images directory if it doesn't exist
  if (!SD.exists("/images")) {
    if (SD.mkdir("/images")) {
      Serial.println("Created /images directory");
    }
  }
  
  // Find the next available image counter
  File root = SD.open("/images");
  if (root) {
    File file = root.openNextFile();
    while (file) {
      String filename = file.name();
      if (filename.startsWith("img_") && filename.endsWith(".jpg")) {
        // Extract number from filename
        int num = filename.substring(4, filename.length() - 4).toInt();
        if (num >= imageCounter) {
          imageCounter = num + 1;
        }
      }
      file = root.openNextFile();
    }
    root.close();
  }
  
  Serial.printf("Next image will be saved as: img_%04d.jpg\n", imageCounter);
  
  return true;
}

// ===========================
// Save Image to SD Card
// ===========================
bool saveImageToSD() {
  if (imageBuffer == NULL || imageSize == 0) {
    Serial.println("No image data to save");
    return false;
  }
  
  // Generate filename
  char filename[32];
  sprintf(filename, "/images/img_%04d.jpg", imageCounter);
  
  Serial.printf("Saving to: %s\n", filename);
  
  // Open file for writing
  File file = SD.open(filename, FILE_WRITE);
  if (!file) {
    Serial.println("Failed to open file for writing");
    return false;
  }
  
  // Write image data
  size_t written = file.write(imageBuffer, imageSize);
  file.close();
  
  if (written != imageSize) {
    Serial.printf("Write error: wrote %d of %d bytes\n", written, imageSize);
    return false;
  }
  
  Serial.printf("Image saved successfully! (%d bytes)\n", written);
  imageCounter++;
  
  return true;
}

// ===========================
// List saved images
// ===========================
void listImages() {
  File root = SD.open("/images");
  if (!root) {
    Serial.println("Failed to open /images directory");
    return;
  }
  
  Serial.println("\n=== Saved Images ===");
  int count = 0;
  File file = root.openNextFile();
  while (file) {
    if (!file.isDirectory()) {
      Serial.printf("%s - %d bytes\n", file.name(), file.size());
      count++;
    }
    file = root.openNextFile();
  }
  Serial.printf("Total: %d images\n", count);
  Serial.println("====================\n");
  
  root.close();
}

// ===========================
// Callback when data is received
// ===========================
void OnDataRecv(const uint8_t *mac, const uint8_t *incomingData, int len) {
  memcpy(&receivedData, incomingData, sizeof(receivedData));
  lastPacketTime = millis();
  
  // First packet - initialize buffer
  if (receivedData.packet_id == 0) {
    imageStartTime = millis();
    
    // Free old buffer if exists
    if (imageBuffer != NULL) {
      free(imageBuffer);
      imageBuffer = NULL;
    }
    
    packetsReceived = 0;
    expectedPackets = receivedData.total_packets;
    imageSize = expectedPackets * MAX_PACKET_SIZE;
    
    // Allocate buffer
    imageBuffer = (uint8_t *)malloc(imageSize);
    if (imageBuffer == NULL) {
      Serial.println("Failed to allocate memory!");
      return;
    }
    
    receivingImage = true;
    Serial.printf("\n--- Starting image reception ---\n");
    Serial.printf("Expected packets: %d\n", expectedPackets);
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
    // Calculate actual image size
    imageSize = (expectedPackets - 1) * MAX_PACKET_SIZE + receivedData.data_length;
    
    unsigned long transferTime = millis() - imageStartTime;
    
    Serial.println("\n=== Image Reception Complete ===");
    Serial.printf("Total packets: %d\n", packetsReceived);
    Serial.printf("Image size: %d bytes\n", imageSize);
    Serial.printf("Transfer time: %lu ms\n", transferTime);
    
    // Verify and save
    if (verifyJPEG()) {
      if (saveImageToSD()) {
        Serial.println("Image saved to SD card!");
      }
    }
    
    Serial.println("================================\n");
    
    // Reset for next image
    receivingImage = false;
  }
}

// ===========================
// Verify JPEG Format
// ===========================
bool verifyJPEG() {
  if (imageBuffer == NULL || imageSize < 2) {
    Serial.println("ERROR: Invalid image data");
    return false;
  }
  
  // Check JPEG header (FF D8)
  if (imageBuffer[0] != 0xFF || imageBuffer[1] != 0xD8) {
    Serial.println("ERROR: Invalid JPEG header!");
    Serial.printf("Header: %02X %02X (expected FF D8)\n", imageBuffer[0], imageBuffer[1]);
    return false;
  }
  
  // Check JPEG end marker (FF D9)
  if (imageSize >= 2) {
    if (imageBuffer[imageSize-2] == 0xFF && imageBuffer[imageSize-1] == 0xD9) {
      Serial.println("Valid JPEG image verified!");
      return true;
    } else {
      Serial.println("WARNING: JPEG end marker not found!");
      Serial.printf("End bytes: %02X %02X (expected FF D9)\n", 
                    imageBuffer[imageSize-2], imageBuffer[imageSize-1]);
      return true;  // Still try to save
    }
  }
  
  return true;
}

// ===========================
// Check for timeout
// ===========================
void checkTimeout() {
  if (receivingImage && (millis() - lastPacketTime > TIMEOUT_MS)) {
    Serial.printf("\nTimeout! Only received %d/%d packets\n", 
                  packetsReceived, expectedPackets);
    receivingImage = false;
    
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
  
  Serial.println("\n\n====================================");
  Serial.println("ESP32 Image Receiver with SD Storage");
  Serial.println("====================================");
  
  // Initialize SD Card
  if (!initSDCard()) {
    Serial.println("SD Card initialization failed!");
    Serial.println("Check wiring and SD card");
    Serial.println("System will continue without SD card");
    Serial.println("====================================\n");
  } else {
    Serial.println("SD Card initialized successfully");
    Serial.println("====================================\n");
    
    // List existing images
    listImages();
  }
  
  // Set WiFi to Station mode
  WiFi.mode(WIFI_STA);
  
  // Print MAC Address
  Serial.print("Receiver MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    while (1) delay(1000);
  }
  
  Serial.println("ESP-NOW initialized");
  
  // Register receive callback
  esp_now_register_recv_cb(OnDataRecv);
  
  Serial.println("\nWaiting for images...");
  Serial.println("Images will be saved to /images/ directory");
  Serial.println("====================================\n");
}

// ===========================
// Loop
// ===========================
void loop() {
  checkTimeout();
  delay(100);
  
  // Optional: Press a button or serial command to list images
  // Uncomment to enable periodic listing (every 60 seconds)
  /*
  static unsigned long lastListTime = 0;
  if (millis() - lastListTime > 60000) {
    listImages();
    lastListTime = millis();
  }
  */
}

