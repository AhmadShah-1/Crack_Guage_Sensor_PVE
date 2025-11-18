import time
import json
from azure.iot.device import IoTHubDeviceClient, Message

class IoTClient:
    def __init__(self, connection_string):
        self.connection_string = connection_string
        self.client = None

    def connect(self):
        try:
            # Create the client
            self.client = IoTHubDeviceClient.create_from_connection_string(self.connection_string)
            print("Connecting to Azure IoT Hub...")
            self.client.connect()
            print("Connected to Azure IoT Hub!")
            return True
        except Exception as e:
            print(f"Failed to connect to Azure IoT Hub: {e}")
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

