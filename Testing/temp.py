import serial
import serial.tools.list_ports
import sys

def find_esp32_port():
    """Auto-detect ESP32 COM port"""
    ports = serial.tools.list_ports.comports()
    
    print("Available COM ports:")
    for i, port in enumerate(ports):
        print(f"  [{i}] {port.device} - {port.description}")
    
    if not ports:
        print("ERROR: No COM ports found!")
        return None
    
    # Try to auto-detect ESP32 (usually has "CP210" or "CH340" in description)
    for port in ports:
        if any(chip in port.description.upper() for chip in ['CP210', 'CH340', 'USB-SERIAL', 'UART']):
            print(f"\n✅ Auto-detected ESP32 on: {port.device}")
            return port.device
    
    # If not found, ask user
    if len(ports) == 1:
        print(f"\n✅ Using only available port: {ports[0].device}")
        return ports[0].device
    
    print("\nCould not auto-detect ESP32. Please select port number [0-{}]: ".format(len(ports)-1), end='')
    try:
        choice = int(input())
        if 0 <= choice < len(ports):
            return ports[choice].device
    except (ValueError, IndexError):
        pass
    
    return None

# Find COM port
port = find_esp32_port()
if not port:
    print("ERROR: No valid port selected!")
    sys.exit(1)

print(f"\n🔌 Connecting to {port} at 115200 baud...")

try:
    ser = serial.Serial(port, 115200, timeout=1)
    print("✅ Connected! Reading serial output...\n")
    print("="*60)
except serial.SerialException as e:
    print(f"ERROR: Could not open {port}: {e}")
    sys.exit(1)

buffer = []
image_active = False
image_metadata = {}

try:
    while True:
        line = ser.readline().decode(errors='ignore').strip()
        if not line:
            continue

        # Print normal logs
        if not image_active and not line.startswith("===IMAGE_START==="):
            print(line)
            continue

        # Detect start of image
        if line.startswith("===IMAGE_START==="):
            image_active = True
            buffer = []
            image_metadata = {}
            print("\n" + "="*60)
            print("📸 Image reception started...")
            continue

        # Collect metadata
        if image_active and ":" in line and not line.startswith("==="):
            key, value = line.split(":", 1)
            image_metadata[key.strip()] = value.strip()
            print(f"  {key.strip()}: {value.strip()}")
            continue

        # Detect data section
        if line.startswith("===DATA==="):
            print("  Receiving image data...")
            continue

        # Detect end of image
        if line.startswith("===IMAGE_END==="):
            print("✅ Image received. Saving...")
            
            # Generate filename
            camera = image_metadata.get('CAMERA', 'unknown')
            timestamp = image_metadata.get('TIMESTAMP', '0')
            filename = f"image_{camera}_{timestamp}.hex"
            
            # Save hex data
            with open(filename, "w") as f:
                f.write("\n".join(buffer))
            
            image_active = False
            print(f"🗂 Saved as {filename}")
            print(f"  Size: {len(''.join(buffer))//2} bytes")
            print("="*60 + "\n")
            continue

        if image_active:
            buffer.append(line)

except KeyboardInterrupt:
    print("\n\n⚠️  Interrupted by user. Closing connection...")
    ser.close()
    print("✅ Connection closed.")
    sys.exit(0)
except Exception as e:
    print(f"\n❌ Error: {e}")
    ser.close()
    sys.exit(1)
