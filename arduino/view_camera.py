import serial
import numpy as np
import cv2
import sys
import time

# ================= CONFIGURATION =================
# On Ubuntu/Linux, the Nano ESP32 is usually /dev/ttyACM0
# Check via 'ls /dev/ttyACM*'
SERIAL_PORT = '/dev/ttyACM0' 
BAUD_RATE = 2000000
# =================================================

def main():
    print(f"Connecting to {SERIAL_PORT} at {BAUD_RATE}...")
    try:
        ser = serial.Serial(SERIAL_PORT, BAUD_RATE, timeout=2)
        # Clear buffer
        ser.reset_input_buffer()
    except Exception as e:
        print(f"Error opening serial port: {e}")
        sys.exit(1)

    print("Connected! Waiting for video stream...")
    print("Press 'q' to quit.")

    buffer = b""

    while True:
        try:
            # Read a line to check for the "IMG:" header
            # readline() reads until \n
            line = ser.readline().strip()
            
            # Check for error messages from Arduino
            if b"ERROR" in line:
                print(f"Arduino Error: {line.decode('utf-8', errors='ignore')}")
                continue

            # Check for Image Header
            if line.startswith(b"IMG:"):
                try:
                    # Parse the length
                    length_str = line.split(b":")[1]
                    img_len = int(length_str)
                    
                    # Read exactly that many bytes
                    # Note: read() might return fewer bytes than requested, 
                    # so we loop until we have the full frame.
                    img_data = b""
                    while len(img_data) < img_len:
                        chunk = ser.read(img_len - len(img_data))
                        if not chunk:
                            break
                        img_data += chunk
                    
                    if len(img_data) == img_len:
                        # Convert raw bytes to numpy array
                        nparr = np.frombuffer(img_data, np.uint8)
                        # Decode JPEG
                        frame = cv2.imdecode(nparr, cv2.IMREAD_GRAYSCALE)
                        
                        if frame is not None:
                            cv2.imshow('OV7670 Stream', frame)
                        else:
                            print("Failed to decode JPEG")
                    else:
                        print("Incomplete frame received")

                except ValueError:
                    print(f"Bad header: {line}")
            
            # Exit on 'q' key
            if cv2.waitKey(1) & 0xFF == ord('q'):
                break

        except KeyboardInterrupt:
            break
        except Exception as e:
            print(f"Error: {e}")
            break

    ser.close()
    cv2.destroyAllWindows()

if __name__ == "__main__":
    main()