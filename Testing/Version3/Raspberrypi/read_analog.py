import serial
import time
import sys
from datetime import datetime
import config
from send_to_iot_hub import IoTClient

def main():
    # 1. Initialize IoT Client
    iot_client = IoTClient(config.CONNECTION_STRING)
    if not iot_client.connect():
        print("Exiting due to Azure connection failure.")
        sys.exit(1)

    # 2. Initialize Serial Connection
    try:
        ser = serial.Serial(config.SERIAL_PORT, config.BAUD_RATE, timeout=1)
        ser.flush()
        print(f"Listening on {config.SERIAL_PORT} at {config.BAUD_RATE}...")
    except Exception as e:
        print(f"Error opening serial port {config.SERIAL_PORT}: {e}")
        sys.exit(1)

    try:
        while True:
            if ser.in_waiting > 0:
                # Read line from serial
                try:
                    line = ser.readline().decode('utf-8').strip()
                except UnicodeDecodeError:
                    continue # Skip bad frames
                
                # Expected Format: JOYSTICK,DEVICE_ID,SUB_ID,X,Y,[RSSI]
                if line.startswith("JOYSTICK"):
                    parts = line.split(',')
                    
                    if len(parts) >= 5:
                        # Create structured data
                        telemetry = {
                            "timestamp": datetime.utcnow().isoformat(),
                            "type": "joystick",
                            "device_id": parts[1],      # e.g., A_T1
                            "subreceiver_id": parts[2], # e.g., A_SR
                            "x": int(parts[3]),
                            "y": int(parts[4])
                        }
                        
                        # Handle Optional Signal Strength (RSSI)
                        signal_str = "N/A"
                        if len(parts) >= 6:
                            try:
                                rssi = int(parts[5])
                                telemetry["rssi"] = rssi
                                signal_str = f"{rssi} dBm"
                            except ValueError:
                                pass

                        # Print status to console
                        print(f"[{telemetry['timestamp']}] Dev: {telemetry['device_id']} | X: {telemetry['x']:<4} | Y: {telemetry['y']:<4} | Signal: {signal_str}")

                        # Send to Azure
                        iot_client.send_telemetry(telemetry)
                    else:
                        print(f"Invalid format: {line}")
                else:
                    # Print other debug messages from ESP32
                    # print(f"Debug: {line}")
                    pass
            
            time.sleep(0.01) # Prevent CPU hogging

    except KeyboardInterrupt:
        print("\nStopping...")
    finally:
        ser.close()
        iot_client.disconnect()

if __name__ == "__main__":
    main()

