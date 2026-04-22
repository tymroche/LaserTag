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

  speaker_init();
  delay(1000);
  beep(500, 200);
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
  if (currentTimer <= 0) {
    rxFlag = 0;
    receiveData = 0;
  }
  transmitData = playerIdToByte(assignedPlayerId);
  //transmitData = 85;
  if (triggerFlag) {
    triggerFlag = false;
    playFireBeeps();
    if (vestCanFire()) {
      emitter_write(transmitData);
      Serial.println("Shot fired");
      } else {
        beep(600, 50);
        beep(200, 30);
      }
  }
  if (rxFlag) {
    
    status_t status = receiverRead(&receiveData);
    printStatus(status);
    Serial.printf("Received receiveData: %d\n", receiveData);

    String attackerId = byteToPlayerId(receiveData);
    if (attackerId.length() > 0 && alive && canShoot && currentTimer > 0) {
      reportHitToHost(attackerId);
      playGotHitBeeps();

      if (currentHp < 3 && currentHp != 1) {
        playLowHealthBeeps();
      } else if (currentHp == 1) {
        playDeadBeeps();
      }
    }
    rxFlag = 0;
  }
}