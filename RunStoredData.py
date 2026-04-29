import serial
import time
import os

PORT = "COM11"   # change to your port
BAUD = 115200

print("Connecting to ESP32...")
ser = serial.Serial(PORT, BAUD, timeout=3)
time.sleep(2)

print("Sending read command...")
ser.write(b'R')

collecting = False
lines = []

print("Waiting for log data...")
while True:
    try:
        line = ser.readline().decode(errors='ignore').strip()
        if not line:
            continue
        print(line)
        if "START LOG" in line:
            collecting = True
            continue
        if "END LOG" in line:
            break
        if collecting:
            lines.append(line)
    except KeyboardInterrupt:
        break

ser.close()

os.makedirs("data", exist_ok=True)

output_file = "data/test_results.csv"
with open(output_file, 'w') as f:
    f.write("timestamp,status\n")
    for line in lines:
        f.write(line + "\n")

print(f"\nDone. {len(lines)} entries saved to {output_file}")