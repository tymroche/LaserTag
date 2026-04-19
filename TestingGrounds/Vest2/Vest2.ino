/**
 * @file Vest2.ino
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Main File for ESP32S3
 * Contains all wifi protocols, all emitter and receiver methods, audio DAC controls, and power
 */

#include <Arduino.h>
#include <WiFi.h>
#include "Vest-Wifi-Protocol.h"
#include "player_comms.h"

/**
 * @brief setup vest
 */
void setup() {
  Serial.begin(115200);


  WiFi.mode(WIFI_STA);
  WiFi.begin(hostSsid, hostPassword);
  Serial.println(hostSsid);
  Serial.println(hostPassword);
  delay(2000);

  //testing
  Serial.print("Trying to join SSID: ");
  Serial.println(hostSsid);

    // Initialize Transmit Variables
  rxFlag = false;
  receiveData = 0;
  transmitData = 152;
  triggerFlag = false;
  // Initialize Trigger
  trigger_init();

  // Initialize Emitter
  emitter_init();

  // Initialize Receiver
  receiver_init();
}

/**
 * @brief main super loop
 */
void loop() {
  connectVestToHost();
  handleHostMessages();
  transmitData = playerIdToByte(assignedPlayerId);
  //transmitData = 85;
  if (triggerFlag) {
    triggerFlag = false;

    if (vestCanFire()) {
      emitter_write(transmitData);
      Serial.println("Shot fired");
      }
  }
  if (rxFlag) {
    
    status_t status = receiverRead(&receiveData);
    printStatus(status);
    Serial.printf("Received receiveData: %d\n", receiveData);

    String attackerId = byteToPlayerId(receiveData);
    if (attackerId.length() > 0) {
      reportHitToHost(attackerId);
    }
    rxFlag = 0;
  }
}