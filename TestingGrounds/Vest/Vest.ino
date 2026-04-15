/**
 * @file Vest.ino
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

/**
 * @brief setup vest
 */
void setup() {
  Serial.begin(115200);
  pinMode(RED_LED_PIN, OUTPUT);
  digitalWrite(RED_LED_PIN, LOW);
  connectVestToHost();
}

/**
 * @brief main super loop
 */
void loop() {
  handleHostMessages();
  updateRedBlink();
}