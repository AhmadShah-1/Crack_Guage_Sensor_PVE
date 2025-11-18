/*
  ESP32 WROOM Transmitter (Joystick)
  Reads Joystick X/Y and sends to ESP32-CAM Sub-Receiver via ESP-NOW
*/

#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"

// ===========================
// CONFIGURATION
// ===========================
#define DEVICE_ID "A_T1"

// TARGET MAC ADDRESS (The ESP32-CAM Sub-Receiver)
// ⚠️ UPDATE THIS to your ESP32-CAM's MAC address! 4C:C3:82:C0:3B:6C
uint8_t subReceiverMAC[] = {0x4C, 0xC3, 0x82, 0xC0, 0x3B, 0x6C}; 

// ===========================
// PINS (ESP32 WROOM)
// ===========================
// Using Sensor VN/VP pins which are Input-Only and perfect for Analog
#define VRX_PIN  39  // GPIO 39 (VN)
#define VRY_PIN  36  // GPIO 36 (VP)

// ===========================
// DATA STRUCTURE
// ===========================
typedef struct struct_message {
  char device_id[10];
  int x;
  int y;
} struct_message;

struct_message myData;
esp_now_peer_info_t peerInfo;

#define WIFI_PROTOCOL_MASK (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR)

// Callback when data is sent
void OnDataSent(const uint8_t *mac_addr, esp_now_send_status_t status) {
  // Serial.print("\r\nLast Packet Send Status:\t");
  // Serial.println(status == ESP_NOW_SEND_SUCCESS ? "Delivery Success" : "Delivery Fail");
}

void setup() {
  Serial.begin(115200);
  
  // Set ADC attenuation for 3.3V range
  analogSetAttenuation(ADC_11db);

  WiFi.mode(WIFI_STA);
  
  // Initialize ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }

  // Set LR Protocol
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_MASK);

  esp_now_register_send_cb(OnDataSent);
  
  // Register Peer (The ESP32-CAM)
  memcpy(peerInfo.peer_addr, subReceiverMAC, 6);
  peerInfo.channel = 0;  
  peerInfo.encrypt = false;
  
  if (esp_now_add_peer(&peerInfo) != ESP_OK){
    Serial.println("Failed to add peer");
    return;
  }
  
  strncpy(myData.device_id, DEVICE_ID, sizeof(myData.device_id) - 1);
  myData.device_id[sizeof(myData.device_id) - 1] = '\0';
  
  Serial.printf("WROOM Transmitter Ready. Pins: VRX=%d, VRY=%d\n", VRX_PIN, VRY_PIN);
}

void loop() {
  myData.x = analogRead(VRX_PIN);
  myData.y = analogRead(VRY_PIN);

  esp_err_t result = esp_now_send(subReceiverMAC, (uint8_t *) &myData, sizeof(myData));
   
  if (result == ESP_OK) {
    Serial.printf("Sent -> X: %d, Y: %d\n", myData.x, myData.y);
  } else {
    Serial.println("Error sending data");
  }

  delay(50); // 20 updates per second
}
