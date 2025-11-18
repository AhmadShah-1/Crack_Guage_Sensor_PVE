/*
 * This ESP32 code is created by esp32io.com
 *
 * This ESP32 code is released in the public domain
 *
 * For more detail (instruction and wiring diagram), visit https://esp32io.com/tutorials/esp32-joystick
 */

 /*
 For esp32 Wroom
 #define VRX_PIN  39 // ESP32 pin GPIO39 (ADC3) connected to VRX pin
 #define VRY_PIN  36 // ESP32 pin GPIO36 (ADC0) connected to VRY pin
 */

 // Pin definitions for ESP32-CAM (avoiding camera pins)
 #define VRX_PIN  33 // ESP32-CAM GPIO33 (ADC1_CH5) connected to VRX pin
 #define VRY_PIN  32 // ESP32-CAM GPIO32 (ADC1_CH4) connected to VRY pin
 // Note: GPIO32 is PWDN on ESP32-CAM. If camera needs power control, use GPIO2 or GPIO12 instead
 
 int valueX = 0; // to store the X-axis value
 int valueY = 0; // to store the Y-axis value
 
 void setup() {
   Serial.begin(9600);
 
   // Set the ADC attenuation to 11 dB (up to ~3.3V input)
   analogSetAttenuation(ADC_11db);
 }
 
 void loop() {
   // read X and Y analog values
   valueX = analogRead(VRX_PIN);
   valueY = analogRead(VRY_PIN);
 
   // print data to Serial Monitor on Arduino IDE
   Serial.print("x = ");
   Serial.print(valueX);
   Serial.print(", y = ");
   Serial.println(valueY);
   delay(200);
 }
 