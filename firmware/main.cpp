#include <Arduino.h>

uint8_t packet[10]; 

// --- THE SCOREBOARD (Remembers everything for the CSV) ---
int presence = 0;
int fallState = 0;
int distance = 0;
int intensity = 0;
int breathRate = 0;
int heartRate = 0;

void setup() {
  Serial.begin(115200);
  // Using Serial2 (P16=RX, P17=TX)
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  
  delay(1000);

  // Command to ensure sensor is active and reporting
  uint8_t unlockCmd[] = {0x53, 0x59, 0x01, 0x01, 0x00, 0x01, 0x0F, 0x64, 0x54, 0x43};
  Serial2.write(unlockCmd, sizeof(unlockCmd));

  // CSV Header - All variables now included
  Serial.println("Presence,Fall_State,Distance_dm,Intensity,Breath_Rate,Heart_Rate,Raw_Hex");
}

void loop() {
  if (Serial2.available() >= 10) {
    // Look for Header 0x53 0x59
    if (Serial2.read() == 0x53 && Serial2.peek() == 0x59) {
      Serial2.read(); // Consume 0x59
      
      for (int i = 2; i < 10; i++) {
        packet[i] = Serial2.read();
      }

      uint8_t control = packet[2];

      // --- LOGIC: UPDATE ONLY WHAT THE PACKET CONTAINS ---
      if (control == 0x80) {        // STATUS PACKET
        presence = packet[6];
        fallState = packet[7];
      } 
      else if (control == 0x81) {   // MOTION PACKET
        distance = packet[6];
        intensity = packet[7];
      }
      else if (control == 0x84) {   // MEDICAL PACKET
        breathRate = packet[6];
        heartRate = packet[7];
      }

      // --- PRINT THE COMPLETE CSV ROW ---
      // We print EVERY variable every time a packet arrives to keep columns stable
      Serial.print(presence);   Serial.print(",");
      Serial.print(fallState);  Serial.print(",");
      Serial.print(distance);   Serial.print(",");
      Serial.print(intensity);  Serial.print(",");
      Serial.print(breathRate); Serial.print(",");
      Serial.print(heartRate);  Serial.print(",");

      // Raw Hex for verification
      Serial.print("53 59 ");
      for(int j=2; j<10; j++) {
        if(packet[j] < 16) Serial.print("0");
        Serial.print(packet[j], HEX);
        Serial.print(" ");
      }
      Serial.println();
    }
  }
}