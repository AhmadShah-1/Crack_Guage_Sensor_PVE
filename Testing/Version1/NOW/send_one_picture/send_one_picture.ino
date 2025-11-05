/*
  ESP32-CAM Image Transfer via ESP-NOW
  Transmitter Side
  
  Takes a picture and sends it in chunks via ESP-NOW
  Compatible with ESP32-CAM AI-THINKER model
*/

#include <esp_now.h>
#include <WiFi.h>
#include "esp_camera.h"

// ===========================
// Camera Model Configuration
// ===========================
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// ===========================
// Receiver MAC Address
// Replace with your receiver's MAC Address
// ===========================
uint8_t receiverAddress[] = {0xA4, 0xCF, 0x12, 0x9B, 0x7E, 0x23}; // R1 from MACaddress.csv

// ===========================
// Data Structure for Image Transfer
// ===========================
#define MAX_PACKET_SIZE 240  // ESP-NOW max is 250, leave some buffer
#define FLASH_LED_PIN 4      // Flash LED pin for AI-THINKER

typedef struct struct_message {
  uint16_t packet_id;      // Packet number
  uint16_t total_packets;  // Total number of packets
  uint16_t data_length;    // Length of data in this packet
  uint8_t data[MAX_PACKET_SIZE]; // Image data chunk
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

// Variables
bool sendSuccess = false;
int successCount = 0;
int totalPacketsSent = 0;

// ===========================
// Callback when data is sent
// ===========================
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  if (status == ESP_NOW_SEND_SUCCESS) {
    successCount++;
  } else {
    Serial.printf("Packet failed: %d\n", myData.packet_id);
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
  config.frame_size = FRAMESIZE_VGA;  // VGA for reasonable size
  config.jpeg_quality = 12;            // 0-63, lower is higher quality
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  // Initialize camera
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed with error 0x%x\n", err);
    return false;
  }
  
  Serial.println("Camera initialized successfully");
  return true;
}

// ===========================
// Capture and Send Image
// ===========================
bool captureAndSendImage() {
  // Turn on flash LED
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200);  // Give time for flash
  
  // Capture image
  camera_fb_t *fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);
  
  if (!fb) {
    Serial.println("Camera capture failed");
    return false;
  }
  
  Serial.printf("Image captured: %d bytes\n", fb->len);
  
  // Calculate number of packets needed
  uint16_t totalPackets = (fb->len + MAX_PACKET_SIZE - 1) / MAX_PACKET_SIZE;
  Serial.printf("Sending %d packets...\n", totalPackets);
  
  successCount = 0;
  totalPacketsSent = 0;
  
  // Send image in chunks
  for (uint16_t i = 0; i < totalPackets; i++) {
    myData.packet_id = i;
    myData.total_packets = totalPackets;
    
    // Calculate data length for this packet
    uint16_t remainingBytes = fb->len - (i * MAX_PACKET_SIZE);
    myData.data_length = (remainingBytes < MAX_PACKET_SIZE) ? remainingBytes : MAX_PACKET_SIZE;
    
    // Copy data chunk
    memcpy(myData.data, fb->buf + (i * MAX_PACKET_SIZE), myData.data_length);
    
    // Send packet
    esp_err_t result = esp_now_send(receiverAddress, (uint8_t *)&myData, sizeof(myData));
    totalPacketsSent++;
    
    if (result != ESP_OK) {
      Serial.printf("Error sending packet %d\n", i);
    }
    
    // Small delay between packets
    delay(5);
    
    // Progress indicator every 20 packets
    if ((i + 1) % 20 == 0) {
      Serial.printf("Progress: %d/%d packets sent\n", i + 1, totalPackets);
    }
  }
  
  // Wait for all callbacks to complete
  delay(100);
  
  // Release frame buffer
  esp_camera_fb_return(fb);
  
  Serial.printf("Transfer complete: %d/%d packets successful\n", successCount, totalPackets);
  return (successCount == totalPackets);
}

// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);
  
  Serial.println("\n\nESP32-CAM Image Transmitter");
  Serial.println("============================");
  
  // Initialize Camera
  if (!initCamera()) {
    Serial.println("Camera initialization failed!");
    while (1) delay(1000);
  }
  
  // Set WiFi to Station mode
  WiFi.mode(WIFI_STA);
  
  // Print MAC Address
  Serial.print("Transmitter MAC Address: ");
  Serial.println(WiFi.macAddress());
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    while (1) delay(1000);
  }
  
  Serial.println("ESP-NOW initialized");
  
  // Register send callback
  esp_now_register_send_cb(OnDataSent);
  
  // Register peer
  memcpy(peerInfo.peer_addr, receiverAddress, 6);
  peerInfo.channel = 0;
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK) {
    Serial.println("Failed to add peer");
    while (1) delay(1000);
  }
  
  Serial.println("Peer registered successfully");
  Serial.println("\nPress RESET to capture and send image");
  Serial.println("============================\n");
  
  delay(2000);
  
  // Capture and send image on startup
  captureAndSendImage();
}

// ===========================
// Loop
// ===========================
void loop() {
  // Wait 10 seconds between captures
  delay(10000);
  
  Serial.println("\n--- Taking new picture ---");
  captureAndSendImage();
}

