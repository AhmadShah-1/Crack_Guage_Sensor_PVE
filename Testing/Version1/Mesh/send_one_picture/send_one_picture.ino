/*
  ESP32-CAM Image Transfer via painlessMesh
  Transmitter Side
  
  Takes a picture and sends it via Mesh network
  Compatible with ESP32-CAM AI-THINKER model
  
  Note: Install painlessMesh library via Arduino Library Manager
  Note: Use ESP32 board version 2.0.x (not 3.x) for painlessMesh compatibility
*/

#include "painlessMesh.h"
#include "esp_camera.h"
#include <Arduino_JSON.h>

// ===========================
// Camera Model Configuration
// ===========================
#define CAMERA_MODEL_AI_THINKER
#include "camera_pins.h"

// ===========================
// Mesh Network Configuration
// ===========================
#define MESH_PREFIX     "CrackSensorMesh"
#define MESH_PASSWORD   "CrackSensor2024"
#define MESH_PORT       5555

// ===========================
// Configuration
// ===========================
#define CHUNK_SIZE 1000          // Size of each chunk to send
#define FLASH_LED_PIN 4          // Flash LED pin
#define CAPTURE_INTERVAL 30000   // Capture every 30 seconds

Scheduler userScheduler;
painlessMesh mesh;

// Image transmission variables
uint8_t *imageBuffer = NULL;
size_t imageSize = 0;
bool imageCaptured = false;
uint16_t currentChunk = 0;
uint16_t totalChunks = 0;
unsigned long lastCaptureTime = 0;

// ===========================
// Function Prototypes
// ===========================
void captureImage();
void sendImageChunk();
void sendMessage();
Task taskCaptureImage(TASK_SECOND * 30, TASK_FOREVER, &captureImage);
Task taskSendChunk(TASK_MILLISECOND * 100, TASK_FOREVER, &sendImageChunk);

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
  config.frame_size = FRAMESIZE_SVGA;  // SVGA (800x600) for mesh
  config.jpeg_quality = 15;             // 0-63, lower is higher quality
  config.fb_count = 1;
  config.grab_mode = CAMERA_GRAB_LATEST;
  
  esp_err_t err = esp_camera_init(&config);
  if (err != ESP_OK) {
    Serial.printf("Camera init failed: 0x%x\n", err);
    return false;
  }
  
  Serial.println("Camera initialized");
  return true;
}

// ===========================
// Capture Image
// ===========================
void captureImage() {
  Serial.println("\n=== Capturing Image ===");
  
  // Turn on flash
  pinMode(FLASH_LED_PIN, OUTPUT);
  digitalWrite(FLASH_LED_PIN, HIGH);
  delay(200);
  
  // Capture
  camera_fb_t *fb = esp_camera_fb_get();
  digitalWrite(FLASH_LED_PIN, LOW);
  
  if (!fb) {
    Serial.println("Capture failed!");
    return;
  }
  
  // Free old buffer if exists
  if (imageBuffer != NULL) {
    free(imageBuffer);
  }
  
  // Allocate new buffer
  imageSize = fb->len;
  imageBuffer = (uint8_t *)malloc(imageSize);
  
  if (imageBuffer == NULL) {
    Serial.println("Failed to allocate memory!");
    esp_camera_fb_return(fb);
    return;
  }
  
  // Copy image data
  memcpy(imageBuffer, fb->buf, imageSize);
  esp_camera_fb_return(fb);
  
  // Prepare for chunked transmission
  totalChunks = (imageSize + CHUNK_SIZE - 1) / CHUNK_SIZE;
  currentChunk = 0;
  imageCaptured = true;
  
  Serial.printf("Image captured: %d bytes\n", imageSize);
  Serial.printf("Will send in %d chunks\n", totalChunks);
  
  // Send start message
  sendStartMessage();
}

// ===========================
// Send Start Message
// ===========================
void sendStartMessage() {
  JSONVar jsonData;
  jsonData["type"] = "image_start";
  jsonData["node"] = mesh.getNodeId();
  jsonData["size"] = (int)imageSize;
  jsonData["chunks"] = totalChunks;
  jsonData["chunk_size"] = CHUNK_SIZE;
  
  String msg = JSON.stringify(jsonData);
  mesh.sendBroadcast(msg);
  
  Serial.println("Sent start message");
}

// ===========================
// Send Image Chunk
// ===========================
void sendImageChunk() {
  if (!imageCaptured || currentChunk >= totalChunks) {
    return;
  }
  
  // Calculate chunk parameters
  uint16_t offset = currentChunk * CHUNK_SIZE;
  uint16_t chunkLen = min((uint16_t)CHUNK_SIZE, (uint16_t)(imageSize - offset));
  
  // Create JSON message with base64-like encoding
  JSONVar jsonData;
  jsonData["type"] = "image_chunk";
  jsonData["node"] = mesh.getNodeId();
  jsonData["chunk_id"] = currentChunk;
  jsonData["total"] = totalChunks;
  
  // Convert chunk to hex string (more efficient than base64 for mesh)
  String hexData = "";
  for (uint16_t i = 0; i < chunkLen; i++) {
    char buf[3];
    sprintf(buf, "%02X", imageBuffer[offset + i]);
    hexData += buf;
  }
  
  jsonData["data"] = hexData;
  
  String msg = JSON.stringify(jsonData);
  mesh.sendBroadcast(msg);
  
  currentChunk++;
  
  // Progress
  if (currentChunk % 5 == 0 || currentChunk == totalChunks) {
    Serial.printf("Sent chunk %d/%d (%.1f%%)\n", 
                  currentChunk, totalChunks,
                  (currentChunk * 100.0) / totalChunks);
  }
  
  // All chunks sent
  if (currentChunk >= totalChunks) {
    Serial.println("All chunks sent!");
    sendEndMessage();
    imageCaptured = false;
    free(imageBuffer);
    imageBuffer = NULL;
  }
}

// ===========================
// Send End Message
// ===========================
void sendEndMessage() {
  JSONVar jsonData;
  jsonData["type"] = "image_end";
  jsonData["node"] = mesh.getNodeId();
  jsonData["chunks"] = totalChunks;
  
  String msg = JSON.stringify(jsonData);
  mesh.sendBroadcast(msg);
  
  Serial.println("Sent end message\n");
}

// ===========================
// Mesh Callbacks
// ===========================
void receivedCallback(uint32_t from, String &msg) {
  Serial.printf("Received from %u: %s\n", from, msg.c_str());
}

void newConnectionCallback(uint32_t nodeId) {
  Serial.printf("New connection: %u\n", nodeId);
}

void changedConnectionCallback() {
  Serial.println("Connection changed");
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
  
  Serial.println("\n\nESP32-CAM Mesh Transmitter");
  Serial.println("===========================");
  
  // Initialize camera
  if (!initCamera()) {
    Serial.println("Camera init failed!");
    while (1) delay(1000);
  }
  
  // Initialize mesh
  mesh.setDebugMsgTypes(ERROR | STARTUP | CONNECTION);
  mesh.init(MESH_PREFIX, MESH_PASSWORD, &userScheduler, MESH_PORT);
  mesh.onReceive(&receivedCallback);
  mesh.onNewConnection(&newConnectionCallback);
  mesh.onChangedConnections(&changedConnectionCallback);
  mesh.onNodeTimeAdjusted(&nodeTimeAdjustedCallback);
  
  Serial.print("Node ID: ");
  Serial.println(mesh.getNodeId());
  
  // Add tasks
  userScheduler.addTask(taskCaptureImage);
  userScheduler.addTask(taskSendChunk);
  
  taskCaptureImage.enable();
  taskSendChunk.enable();
  
  Serial.println("\nMesh network started");
  Serial.println("Capturing first image in 5 seconds...");
  Serial.println("===========================\n");
  
  // Capture first image after 5 seconds
  delay(5000);
  captureImage();
}

// ===========================
// Loop
// ===========================
void loop() {
  mesh.update();
}

