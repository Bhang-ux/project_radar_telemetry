#include <Arduino.h>

uint8_t packet[10]; 

void setup() {
  Serial.begin(115200);
  // P16 = RX (Sensor TX), P17 = TX (Sensor RX)
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  
  delay(1000);

  // FORCE UNLOCK: Tells the sensor to start sending 81 (Distance) packets
  uint8_t unlockCmd[] = {0x53, 0x59, 0x01, 0x01, 0x00, 0x01, 0x0F, 0x64, 0x54, 0x43};
  Serial2.write(unlockCmd, sizeof(unlockCmd));

  // CSV Header - This will be the first line of your file
  Serial.println("Presence,Fall_State,Distance_dm,Raw_Hex");
}

void loop() {
  // Wait until we have at least one full packet (10 bytes)
  if (Serial2.available() >= 10) {
    
    // Search for Header 0x53 0x59
    if (Serial2.read() == 0x53 && Serial2.peek() == 0x59) {
      Serial2.read(); // Consume the 0x59
      
      // Load the rest of the 10-byte packet into our array
      for (int i = 2; i < 10; i++) {
        packet[i] = Serial2.read();
      }

      uint8_t control = packet[2];
      int presence = -1;  // -1 means no data in this packet
      int fallState = -1;
      int distance = -1;

      // Decode logic based on Control Word
      if (control == 0x80) {
        presence = packet[6];
        fallState = packet[7];
      } 
      else if (control == 0x81) {
        distance = packet[6];
      }

      // PRINT IN CSV FORMAT
      // If a value is -1, it stays empty between commas
      if(presence != -1) Serial.print(presence);
      Serial.print(",");
      
      if(fallState != -1) Serial.print(fallState);
      Serial.print(",");
      
      if(distance != -1) Serial.print(distance);
      Serial.print(",");

      // Add the Raw Hex at the end so you can double check
      Serial.print("53 59 ");
      for(int j=2; j<10; j++) {
        if(packet[j] < 16) Serial.print("0");
        Serial.print(packet[j], HEX);
        Serial.print(" ");
      }
      Serial.println(); // New line for next entry
    }
  }
}