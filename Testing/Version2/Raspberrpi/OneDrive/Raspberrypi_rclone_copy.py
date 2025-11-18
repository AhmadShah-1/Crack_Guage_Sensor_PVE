#!/usr/bin/env python3
"""
ESP32 Image Receiver with rclone copy to OneDrive
Uses rclone copy instead of mount for simpler operation
"""

import serial
import sys
import time
import subprocess
from datetime import datetime
from pathlib import Path

# ===========================
# CONFIGURATION
# ===========================
SITE_LOCATION = "BryantPark"  # ← CHANGE THIS for different sites

PORT = '/dev/serial/by-id/usb-1a86_USB_Serial-if00-port0'
BAUD = 115200

# Local directory for images
LOCAL_DIR = Path("/home/ahmad/CrackSensorImages")
LOCAL_DIR.mkdir(parents=True, exist_ok=True)

# OneDrive remote name (from rclone config)
ONEDRIVE_REMOTE = "onedrive:"

# Remote path in OneDrive (adjust to match your OneDrive structure)
ONEDRIVE_PATH = "PVEDI/CrackSensor"  # Path in your OneDrive

# ===========================
# Upload to OneDrive using rclone copy
# ===========================
def upload_to_onedrive(local_dir, relative_path):
    """Upload directory to OneDrive using rclone copy"""
    try:
        # Build remote path
        remote_path = f"{ONEDRIVE_REMOTE}{ONEDRIVE_PATH}/{relative_path}"
        
        # Use rclone copy to sync the entire camera directory
        cmd = ["rclone", "copy", str(local_dir), remote_path, "--update", "--verbose"]
        
        result = subprocess.run(cmd, capture_output=True, text=True, timeout=30)
        
        if result.returncode == 0:
            print(f"   ☁️  Uploaded to OneDrive: {relative_path}")
            return True
        else:
            # Only show error if it's not just "nothing to transfer"
            if "Transferred:" not in result.stderr:
                print(f"   ⚠️  Upload issue: {result.stderr.strip()}")
            return False
            
    except subprocess.TimeoutExpired:
        print(f"   ⚠️  Upload timeout")
        return False
    except Exception as e:
        print(f"   ⚠️  Upload error: {e}")
        return False

# ===========================
# SERIAL SETUP
# ===========================
ser = serial.Serial()
ser.port = PORT
ser.baudrate = BAUD
ser.timeout = 0.1
ser.dtr = False
ser.rts = False

try:
    ser.open()
    ser.reset_input_buffer()
    print(f"✅ Opened {PORT} @ {BAUD} baud")
except serial.SerialException as e:
    print(f"❌ ERROR: Could not open {PORT}: {e}")
    sys.exit(1)

# Test rclone
print("🔍 Testing rclone connection...")
try:
    result = subprocess.run(["rclone", "lsd", ONEDRIVE_REMOTE], 
                          capture_output=True, text=True, timeout=10)
    if result.returncode == 0:
        print("✅ OneDrive connection OK")
    else:
        print(f"⚠️  OneDrive connection issue: {result.stderr}")
        print("   Images will be saved locally only")
except Exception as e:
    print(f"⚠️  rclone test failed: {e}")
    print("   Images will be saved locally only")

# ===========================
# IMAGE CAPTURE STATE
# ===========================
image_active = False
image_metadata = {}
image_data_hex = []

def save_image(metadata, hex_data):
    """Convert hex data to JPEG and save with folder structure"""
    try:
        # Extract metadata
        camera = metadata.get('CAMERA', 'UNKNOWN')
        subreceiver = metadata.get('SUBRECEIVER', 'UNKNOWN')
        timestamp = metadata.get('TIMESTAMP', str(int(time.time())))
        size = metadata.get('SIZE', 'unknown')
        
        # Create local folder structure
        save_dir = LOCAL_DIR / SITE_LOCATION / subreceiver / camera
        save_dir.mkdir(parents=True, exist_ok=True)
        
        # Filename: timestamp.jpg
        filename = f"{timestamp}.jpg"
        filepath = save_dir / filename
        
        # Convert hex string to binary
        hex_string = ''.join(hex_data)
        image_bytes = bytes.fromhex(hex_string)
        
        # Verify JPEG header
        if len(image_bytes) < 2 or image_bytes[0:2] != b'\xFF\xD8':
            print(f"⚠️  WARNING: Invalid JPEG header!")
        
        # Save image locally
        with open(filepath, 'wb') as f:
            f.write(image_bytes)
        
        # Create metadata file
        meta_filepath = save_dir / f"{timestamp}.txt"
        with open(meta_filepath, 'w') as f:
            f.write(f"Site: {SITE_LOCATION}\n")
            f.write(f"Camera: {camera}\n")
            f.write(f"Subreceiver: {subreceiver}\n")
            f.write(f"Timestamp: {timestamp}\n")
            f.write(f"Size: {size} bytes\n")
            f.write(f"Received: {datetime.now().strftime('%Y-%m-%d %H:%M:%S')}\n")
        
        print(f"✅ Saved: {filepath.relative_to(LOCAL_DIR)}")
        print(f"   Size: {len(image_bytes)} bytes")
        
        # Upload to OneDrive (upload the entire camera directory)
        upload_to_onedrive(save_dir, f"{SITE_LOCATION}/{subreceiver}/{camera}")
        
        return True
        
    except Exception as e:
        print(f"❌ ERROR saving image: {e}")
        return False

# ===========================
# MAIN LOOP
# ===========================
print(f"📁 Local save: {LOCAL_DIR / SITE_LOCATION}")
print(f"☁️  OneDrive sync: {ONEDRIVE_REMOTE}{ONEDRIVE_PATH}")
print(f"🔍 Monitoring serial for images... Press Ctrl+C to stop.\n")
print("="*70)

try:
    while True:
        if ser.in_waiting:
            try:
                line = ser.readline().decode('utf-8', errors='ignore').strip()
            except UnicodeDecodeError:
                continue
            
            if not line:
                continue
            
            # Detect image start
            if line.startswith("===IMAGE_START==="):
                image_active = True
                image_metadata = {}
                image_data_hex = []
                print("\n📸 Image reception started...")
                continue
            
            # Collect metadata
            if image_active and ":" in line and not line.startswith("==="):
                parts = line.split(":", 1)
                if len(parts) == 2:
                    key = parts[0].strip()
                    value = parts[1].strip()
                    image_metadata[key] = value
                    print(f"   {key}: {value}")
                continue
            
            # Detect data section
            if line.startswith("===DATA==="):
                print("   Receiving image data...")
                continue
            
            # Detect image end
            if line.startswith("===IMAGE_END==="):
                if image_data_hex:
                    print("   Processing image...")
                    if save_image(image_metadata, image_data_hex):
                        camera = image_metadata.get('CAMERA', 'UNKNOWN')
                        print(f"   📂 Location: {SITE_LOCATION}/{image_metadata.get('SUBRECEIVER', 'UNKNOWN')}/{camera}/")
                    print("="*70)
                image_active = False
                image_metadata = {}
                image_data_hex = []
                continue
            
            # Collect hex data
            if image_active and line and not line.startswith("==="):
                clean_line = ''.join(c for c in line if c in '0123456789ABCDEFabcdef')
                if clean_line:
                    image_data_hex.append(clean_line)
                continue
            
            # Print normal logs (when not capturing image)
            if not image_active:
                if any(keyword in line for keyword in ['[R1]', 'ERROR', 'WARNING', 'Image', 'complete']):
                    print(line)
        
        else:
            time.sleep(0.01)

except KeyboardInterrupt:
    print("\n\n⚠️  Interrupted by user. Closing connection...")
except Exception as e:
    print(f"\n❌ Unexpected error: {e}")
finally:
    ser.close()
    print("✅ Serial connection closed.")
    print(f"📁 Images saved to: {LOCAL_DIR / SITE_LOCATION}")

