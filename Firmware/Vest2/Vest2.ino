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
#include "speaker.h"

/**
 * @brief setup vest
 */
void setup() {
  Serial.begin(115200);

  speaker_init();
  delay(1000);
  playStartUpBeeps();
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
  rxFlag2 = false;
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
  static uint8_t previousHp = currentHp;
  static bool previousAlive = alive;

  if (connectVestToHost()) {
    playConnectedBeeps();
  }
  handleHostMessages();

  // Only play damage/death sounds when state changes
  if (currentHp < previousHp) {
    playGotHitBeeps();

    if (currentHp == 0 && previousAlive) {
      playDeadBeeps();
    } else if (currentHp > 0 && currentHp < 3) {
      playLowHealthBeeps();
    }
  }

  previousHp = currentHp;
  previousAlive = alive;

  if (currentTimer == 0) {
    rxFlag = false;
    rxFlag2 = false;
    receiveData = 0;
  }

  transmitData = playerIdToByte(assignedPlayerId);

  if (triggerFlag) {
    triggerFlag = false;

    if (vestCanFire()) {
      emitter_write(transmitData);
      playFireBeeps();
      Serial.println("Shot fired");
    } else {
      playCantFireBeeps();
    }
  }

  if (rxFlag) {
    rxFlag = false;

    status_t status = receiverRead(&receiveData, RECEIVER_IN);
    printStatus(status);
    Serial.printf("Received receiveData on RECEIVER_IN: %d\n", receiveData);

    if (status == STATUS_OK) {
      String attackerId = byteToPlayerId(receiveData);

      if (attackerId.length() > 0 &&
          attackerId != assignedPlayerId &&
          alive &&
          canShoot &&
          currentTimer > 0) {
        reportHitToHost(attackerId);
      }
    }
  }

  if (rxFlag2) {
    rxFlag2 = false;

    status_t status = receiverRead(&receiveData, RECEIVER_IN_2);
    printStatus(status);
    Serial.printf("Received receiveData on RECEIVER_IN_2: %d\n", receiveData);

    if (status == STATUS_OK) {
      String attackerId = byteToPlayerId(receiveData);

      if (attackerId.length() > 0 &&
          attackerId != assignedPlayerId &&
          alive &&
          canShoot &&
          currentTimer > 0) {
        reportHitToHost(attackerId);
      }
    }
  }
}