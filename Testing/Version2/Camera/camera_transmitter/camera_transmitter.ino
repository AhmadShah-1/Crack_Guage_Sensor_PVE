/*
  ESP32-CAM Image Transmitter via ESP-NOW
  For Hybrid ESP-NOW + Mesh Architecture
  
  Captures images and sends to Sub-Receiver via ESP-NOW
  
  CONFIGURATION REQUIRED:
  - Set DEVICE_ID to match your camera (A_T1, A_T2, B_T1, B_T2)
  - Set subReceiverMAC to match your target sub-receiver
*/

#include <esp_now.h>
#include <WiFi.h>
#include "esp_camera.h"

// ===========================
// DEVICE CONFIGURATION - CHANGE FOR EACH CAMERA
// ===========================
#define DEVICE_ID "A_T1"  // OPTIONS: A_T1, A_T2, B_T1, B_T2

// Sub-receiver MAC addresses
// A_SR: {0xA4, 0xCF, 0x12, 0x58, 0xD1, 0x9A}
// B_SR: {0xA4, 0xCF, 0x11, 0x9B, 0x7E, 0x24}
uint8_t subReceiverMAC[] = {0xA4, 0xCF, 0x12, 0x58, 0xD1, 0x9A}; // A_SR

// ===========================
// Camera Model Configuration
// ===========================
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// ===========================
// Configuration
// ===========================
#define DATA_PACKET_SIZE 230     // Reduced to fit device_id
#define FLASH_LED_PIN 4
#define CAPTURE_INTERVAL 10000   // 10 seconds between captures

// ===========================
// ESP-NOW Packet Structure
// ===========================
typedef struct esp_now_packet {
  char device_id[10];           // Device identifier (A_T1, etc.)
  uint16_t packet_id;           // Current packet number
  uint16_t total_packets;       // Total number of packets
  uint16_t data_length;         // Bytes in this packet
  uint8_t data[DATA_PACKET_SIZE]; // Image data
} esp_now_packet;

esp_now_packet myData;
esp_now_peer_info_t peerInfo;

// Variables
int successCount = 0;
int totalPacketsSent = 0;
unsigned long lastCaptureTime = 0;

// ===========================
// Callback when data is sent
// ===========================
// Note: Signature varies by ESP32 core version
// For ESP32 core 2.0.x (required for painlessMesh)
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    successCount++;
  } else {
    Serial.printf("[%s] Packet %d failed\n", DEVICE_ID, myData.packet_id);
  }
}

// ===========================
// Initialize Camera
// ===========================
bool initCamera() {
  camera_config_t config;
  config.ledc_channel = LEDC_CHANNEL_0;
  config.ledc_timer = LEDC_TIMER_0;
  config.pin_d0 = Y2_GPIO_NUM;
  config.pin_d1 = Y3_GPIO_NUM;
  config.pin_d2 = Y4_GPIO_NUM;
  config.pin_d3 = Y5_GPIO_NUM;
  config.pin_d4 = Y6_GPIO_NUM;
  config.pin_d5 = Y7_GPIO_NUM;
  config.pin_d6 = Y8_GPIO_NUM;
  config.pin_d7 = Y9_GPIO_NUM;
  config.pin_xclk = XCLK_GPIO_NUM;
  config.pin_pclk = PCLK_GPIO_NUM;
  config.pin_vsync = VSYNC_GPIO_NUM;
  config.pin_href = HREF_GPIO_NUM;
  config.pin_sccb_sda = SIOD_GPIO_NUM;
  config.pin_sccb_scl = SIOC_GPIO_NUM;
  config.pin_pwdn = PWDN_GPIO_NUM;
  config.pin_reset = RESET_GPIO_NUM;
  config.xclk_freq_hz = 20000000;
  config.pixel_format = PIXFORMAT_JPEG;
  config.frame_size = FRAMESIZE_VGA;  // VGA for good quality
  config.jpeg_quality = 12;            // 0-63, lower is better
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("[%s] Camera init failed: 0x%x\n", DEVICE_ID, err);
    return false;
  }
  
  Serial.printf("[%s] Camera initialized\n", DEVICE_ID);
  return true;
}

// ===========================
// Capture and Send Image
// ===========================
bool captureAndSendImage() {
  Serial.printf("\n[%s] === Capturing Image ===\n", DEVICE_ID);
  
  // Turn on flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200);
  
  // Capture image
  camera_fb_t *fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);
  
  if (!fb) {
    Serial.printf("[%s] Camera capture failed\n", DEVICE_ID);
    return false;
  }
  
  Serial.printf("[%s] Image captured: %d bytes\n", DEVICE_ID, fb->len);
  
  // Calculate number of packets
  uint16_t totalPackets = (fb->len + DATA_PACKET_SIZE - 1) / DATA_PACKET_SIZE;
  Serial.printf("[%s] Sending %d packets to sub-receiver...\n", DEVICE_ID, totalPackets);
  
  successCount = 0;
  totalPacketsSent = 0;
  
  // Set device ID in packet structure
  strncpy(myData.device_id, DEVICE_ID, sizeof(myData.device_id) - 1);
  myData.device_id[sizeof(myData.device_id) - 1] = '\0';
  
  // Send image in packets
  for (uint16_t i = 0; i < totalPackets; i++) {
    myData.packet_id = i;
    myData.total_packets = totalPackets;
    
    // Calculate data length for this packet
    uint16_t remainingBytes = fb->len - (i * DATA_PACKET_SIZE);
    myData.data_length = (remainingBytes < DATA_PACKET_SIZE) ? remainingBytes : DATA_PACKET_SIZE;
    
    // Copy data chunk
    memcpy(myData.data, fb->buf + (i * DATA_PACKET_SIZE), myData.data_length);
    
    // Send packet
    esp_err_t result = esp_now_send(subReceiverMAC, (uint8_t *)&myData, sizeof(myData));
    totalPacketsSent++;
    
    if (result != ESP_OK) {
      Serial.printf("[%s] Error sending packet %d\n", DEVICE_ID, i);
    }
    
    // Small delay between packets
    delay(5);
    
    // Progress indicator every 20 packets
    if ((i + 1) % 20 == 0) {
      Serial.printf("[%s] Progress: %d/%d packets\n", DEVICE_ID, i + 1, totalPackets);
    }
  }
  
  // Wait for callbacks
  delay(100);
  
  // Release frame buffer
  esp_camera_fb_return(fb);
  
  Serial.printf("[%s] Transfer complete: %d/%d packets successful\n", 
                DEVICE_ID, successCount, totalPackets);
  
  return (successCount == totalPackets);
}

// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.printf("\n\n[%s] ESP32-CAM Transmitter\n", DEVICE_ID);
  Serial.println("========================================");
  
  // Initialize Camera
  if (!initCamera()) {
    Serial.printf("[%s] FATAL: Camera init failed!\n", DEVICE_ID);
    while (1) delay(1000);
  }
  
  // Set WiFi to Station mode
  WiFi.mode(WIFI_STA);
  
  // Print MAC Address
  Serial.printf("[%s] MAC Address: %s\n", DEVICE_ID, WiFi.macAddress().c_str());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.printf("[%s] FATAL: ESP-NOW init failed!\n", DEVICE_ID);
    while (1) delay(1000);
  }
  
  Serial.printf("[%s] ESP-NOW initialized\n", DEVICE_ID);
  
  // Register send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer (sub-receiver)
  memcpy(peerInfo.peer_addr, subReceiverMAC, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.printf("[%s] FATAL: Failed to add peer\n", DEVICE_ID);
    while (1) delay(1000);
  }
  
  Serial.printf("[%s] Sub-receiver registered\n", DEVICE_ID);
  Serial.printf("[%s] Target: %02X:%02X:%02X:%02X:%02X:%02X\n", 
                DEVICE_ID,
                subReceiverMAC[0], subReceiverMAC[1], subReceiverMAC[2],
                subReceiverMAC[3], subReceiverMAC[4], subReceiverMAC[5]);
  Serial.println("========================================\n");
  
  // First capture after 2 seconds
  delay(2000);
  captureAndSendImage();
  lastCaptureTime = millis();
}

// ===========================
// Loop
// ===========================
void loop() {
  // Capture and send at interval
  if (millis() - lastCaptureTime >= CAPTURE_INTERVAL) {
    captureAndSendImage();
    lastCaptureTime = millis();
  }
  
  delay(100);
}

