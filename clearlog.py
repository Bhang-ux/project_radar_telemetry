import serial
import time

PORT = "COM11"
BAUD = 115200

ser = serial.Serial(PORT, BAUD, timeout=3)
time.sleep(2)

print("Sending clear command...")
ser.write(b'C')

time.sleep(1)
while ser.available() if hasattr(ser, 'available') else ser.in_waiting:
    print(ser.readline().decode(errors='ignore').strip())

ser.close()
print("Done")