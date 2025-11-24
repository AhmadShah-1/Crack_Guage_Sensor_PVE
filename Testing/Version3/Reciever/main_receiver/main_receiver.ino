/*
  ESP32 Main Receiver (Joystick Version)
  Receives Joystick Data via Mesh, Outputs to Serial
*/

#include "painlessMesh.h"
#include <Arduino_JSON.h>
#include "esp_wifi.h"

// ===========================
// Mesh Network Configuration
// ===========================
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555

Scheduler userScheduler;
painlessMesh mesh;

// ===========================
// Mesh Callback
// ===========================
void receivedCallback(uint32_t from, String &msg) {
  JSONVar data = JSON.parse(msg.c_str());
  
  if (JSON.typeof(data) == "undefined") {
    Serial.println("Error parsing JSON");
    return;
  }
  
  // Check message type
  String type = (const char*)data["type"];
  
  if (type == "joystick") {
    const char* device_id = (const char*)data["device_id"];
    const char* sub_id = (const char*)data["subreceiver_id"];
    int x = (int)data["x"];
    int y = (int)data["y"];

    int rssi = (int)data["rssi"]; 

    
    // Output formatted string for Serial Parser (Raspberry Pi / PC)
    // Format: JOYSTICK,DEVICE_ID,SUB_ID,X,Y
    Serial.printf("JOYSTICK,%s,%s,%d,%d\n", device_id, sub_id, x, y, rssi);
  }
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("Mesh: New Connection, nodeId = %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.printf("Mesh: Changed connections\n");
}

void nodeTimeAdjustedCallback(int32_t offset) {
}

void setup() {
  Serial.begin(115200);
  
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION); 
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  // Ensure LR protocol is active if needed
  esp_wifi_set_protocol(WIFI_IF_STA, WIFI_PROTOCOL_11B | WIFI_PROTOCOL_11G | WIFI_PROTOCOL_11N | WIFI_PROTOCOL_LR);
  
  Serial.println("\n\nMain Receiver Ready. Waiting for Joystick Data...");
}

void loop() {
  mesh.update();
}
