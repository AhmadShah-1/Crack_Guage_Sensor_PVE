import time
import json
from azure.iot.device import IoTHubDeviceClient, Message
import sys

class IoTClient:
    def __init__(self, connection_string):
        self.connection_string = connection_string
        self.client = None

    def connect(self):
        try:
            print(f"Attempting to connect with string starting: {self.connection_string[:30]}...")
            # Create the client
            self.client = IoTHubDeviceClient.create_from_connection_string(self.connection_string)
            print("Client created. Connecting to Azure IoT Hub...")
            self.client.connect()
            print("Connected to Azure IoT Hub!")
            return True
        except Exception as e:
            print("\n" + "="*40)
            print(f"CONNECTION ERROR: {e}")
            print("="*40 + "\n")
            # Check for common errors
            if "Unauthorized" in str(e):
                print("HINT: Your Connection String might be wrong. Check DeviceId and SharedAccessKey.")
            elif "getaddrinfo failed" in str(e):
                print("HINT: DNS/Network Error. Check your internet connection and HostName.")
            elif "Time" in str(e) or "ssl" in str(e).lower():
                print("HINT: Check your System Time. SSL requires correct clock time.")
            return False

    def send_telemetry(self, data_dict):
        if not self.client:
            print("Client not connected, cannot send telemetry.")
            return

        try:
            # Convert dictionary to JSON string
            msg_body = json.dumps(data_dict)
            message = Message(msg_body)
            
            # Optional: Add custom properties
            # message.custom_properties["sensorType"] = "joystick"

            print(f"Sending message: {msg_body}")
            self.client.send_message(message)
            print("Message sent successfully")
        except Exception as e:
            print(f"Error sending message: {e}")

    def disconnect(self):
        if self.client:
            self.client.shutdown()
