import pylink
import time
import sys

# --- CONFIGURATION ---
DEVICE_NAME = 'STM32L432KC'
OUTPUT_PREFIX = "threshold_255/capture_"
IMAGE_WIDTH = 320
IMAGE_HEIGHT = 240
# ---------------------

def capture_rtt_images():
    jlink = pylink.JLink()
    
    try:
        print(f"Connecting to J-Link...")
        jlink.open()
        print(f"Connecting to target {DEVICE_NAME}...")
        jlink.connect(DEVICE_NAME, verbose=True)
        
        print("Searching for RTT Control Block...")
        jlink.rtt_start()
        
        # Wait for RTT to be found
        while True:
            try:
                num_up = jlink.rtt_get_num_up_buffers()
                if num_up > 0:
                    print(f"RTT Started. Found {num_up} Up-Buffers.")
                    # Force the config on the Python side too, just in case
                    break
            except pylink.errors.JLinkRTTException:
                pass
            time.sleep(0.1)

        print(f"Capturing {IMAGE_WIDTH}x{IMAGE_HEIGHT} images... (Press Ctrl+C to stop)")
        
        current_file = None
        image_count = 0
        buffer_data = ""
        
        # Performance: Read larger chunks (16KB) to drain the buffer faster
        READ_CHUNK_SIZE = 16384 

        while True:
            # Read data from RTT Channel 0
            data_bytes = jlink.rtt_read(0, READ_CHUNK_SIZE)
            
            if not data_bytes:
                # No data? Sleep briefly to avoid 100% CPU usage
                time.sleep(0.001)
                continue

            # Convert bytes to ASCII string
            # 'replace' handles any glitchy bytes without crashing
            chunk = "".join(map(chr, data_bytes))
            buffer_data += chunk

            # Process buffer line-by-line
            while '\n' in buffer_data:
                line, buffer_data = buffer_data.split('\n', 1)
                line = line.strip()

                if line == 'P1':
                    # Close previous file
                    if current_file:
                        current_file.close()
                        print(f"Saved {filename}")

                    # Open new file
                    image_count += 1
                    filename = f"{OUTPUT_PREFIX}{image_count:03d}.pbm"
                    current_file = open(filename, 'w')
                    current_file.write("P1\n")
                    # Note: We rely on the C-code sending the dimensions next
                
                elif current_file:
                    current_file.write(line + "\n")

    except KeyboardInterrupt:
        print("\nStopping...")
    except Exception as e:
        print(f"Error: {e}")
    finally:
        if current_file:
            current_file.close()
            print(f"Closed {filename}")
        if jlink.opened():
            jlink.close()
        print("Disconnected.")

if __name__ == "__main__":
    capture_rtt_images()