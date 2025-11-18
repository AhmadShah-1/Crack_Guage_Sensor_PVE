/*
  ESP32-CAM Sub-Receiver Relay (No Camera Usage)
  Receives Joystick Data via ESP-NOW, Forwards to Mesh
  
  Hardware: ESP32-CAM Module
  (Camera is NOT initialized to save power and avoid conflicts)
*/

#include <esp_now.h>
#include <WiFi.h>
#include "esp_wifi.h"
#include "painlessMesh.h"
#include <Arduino_JSON.h>

// ===========================
// DEVICE CONFIGURATION
// ===========================
#define SUBRECEIVER_ID "A_SR"

// ===========================
// Mesh Network Configuration
// ===========================
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555

#define WIFI_PROTOCOL_MASK (WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR)

// Data Structure (Must match Transmitter)
typedef struct struct_message {
  char device_id[10];
  int x;
  int y;
} struct_message;

struct_message myData;

Scheduler userScheduler;
painlessMesh mesh;

// ===========================
// Forward Data to Mesh
// ===========================
void forwardDataToMesh(struct_message &data) {
  JSONVar jsonMsg;
  jsonMsg["type"] = "joystick";
  jsonMsg["device_id"] = data.device_id;
  jsonMsg["subreceiver_id"] = SUBRECEIVER_ID;
  jsonMsg["x"] = data.x;
  jsonMsg["y"] = data.y;
  jsonMsg["ts"] = (int)millis();

  String msgStr = JSON.stringify(jsonMsg);
  
  mesh.sendBroadcast(msgStr);
  Serial.printf("[%s] Fwd to Mesh: %s\n", SUBRECEIVER_ID, msgStr.c_str());
}

// ===========================
// ESP-NOW Receive Callback
// ===========================
void OnDataRecv(const uint8_t *mac_addr, const uint8_t *incomingData, int len) {
  if (len != sizeof(myData)) {
    Serial.println("Error: Packet size mismatch");
    return;
  }

  memcpy(&myData, incomingData, sizeof(myData));
  forwardDataToMesh(myData);
}

// ===========================
// Mesh Callbacks
// ===========================
void receivedCallback(uint32_t from, String &msg) {}
void newConnectionCallback(uint32_t nodeId) { Serial.printf("Mesh New Connection: %u\n", nodeId); }
void changedConnectionCallback() { Serial.printf("Mesh Connections Changed\n"); }
void nodeTimeAdjustedCallback(int32_t offset) {}

// ===========================
// Setup
// ===========================
void setup() {
  Serial.begin(115200);
  delay(1000);

  WiFi.mode(WIFI_STA);
  
  // *** COPY THIS MAC ADDRESS TO YOUR TRANSMITTER CODE ***
  Serial.print("Sub-Receiver MAC: ");
  Serial.println(WiFi.macAddress());

  // Init ESP-NOW
  if (esp_now_init() != ESP_OK) {
    Serial.println("Error initializing ESP-NOW");
    return;
  }
  esp_now_register_recv_cb(OnDataRecv);

  // Init Mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION); 
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);

  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_MASK);
  
  Serial.println("ESP32-CAM Relay Ready (No Camera Init)");
}

void loop() {
  mesh.update();
}
