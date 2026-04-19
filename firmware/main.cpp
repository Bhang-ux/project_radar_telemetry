#include <Arduino.h>

uint8_t packet[10]; 

// ADDED: Keep track of last known values so columns stay full
int currentPresence = 0; 
int currentFall = 0;
int currentDistance = 0;

void setup() {
  Serial.begin(115200);
  Serial2.begin(115200, SERIAL_8N1, 16, 17);
  delay(1000);

  uint8_t unlockCmd[] = {0x53, 0x59, 0x01, 0x01, 0x00, 0x01, 0x0F, 0x64, 0x54, 0x43};
  Serial2.write(unlockCmd, sizeof(unlockCmd));

  Serial.println("Presence,Fall_State,Distance_dm,Raw_Hex");
}

void loop() {
  if (Serial2.available() >= 10) {
    if (Serial2.read() == 0x53 && Serial2.peek() == 0x59) {
      Serial2.read(); 
      
      for (int i = 2; i < 10; i++) {
        packet[i] = Serial2.read();
      }

      uint8_t control = packet[2];

      // Update ONLY the values that the current packet provides
      if (control == 0x80) { 
        currentPresence = packet[6];
        currentFall = packet[7];
      } 
      else if (control == 0x81) { 
        currentDistance = packet[6];
      }

      // PRINT AS CSV - No "if" checks here, so columns never shift
      Serial.print(currentPresence);
      Serial.print(",");
      Serial.print(currentFall);
      Serial.print(",");
      Serial.print(currentDistance);
      Serial.print(",");

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