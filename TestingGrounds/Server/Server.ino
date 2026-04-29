/**
 * @file Server.ino
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Main File for ESP32C6
 * Contains all server protocols (tcp connection, html webpage).
 */

#include <Arduino.h>
#include <WiFi.h>
#include "Host-Server.h"

/**
 * @brief Initializes serial output, player state, SoftAP mode, and both the web and device servers.
 */
void setup() {
  Serial.begin(115200);
  delay(1000);

  resetAllPlayers();

  Serial.println();
  Serial.println("Start AP test");

  WiFi.mode(WIFI_AP);
  delay(200);

  bool ok = WiFi.softAP(ssid, password);
  Serial.print("softAP result: ");
  Serial.println(ok ? "SUCCESS" : "FAIL");

  Serial.print("SSID: ");
  Serial.println(WiFi.softAPSSID());

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  webServer.begin();
  deviceServer.begin();

  Serial.println("Web server started on port 80");
  Serial.println("Device server started on port 27015");
}

/**
 * @brief Runs the main server loop, handling device and browser traffic, and Free For All timer.
 */
void loop() {
  handleDeviceConnections();
  handleWebClients();

  if (gameMode == "Free For All" && ffaTimerRunning && getRemainingFFATime() == 0) {
    disableAllShooting();
    statusMessage = "Free For All ended.";
    ffaTimerRunning = false;
  }

  static unsigned long lastPrint = 0;
  if (millis() - lastPrint >= 3000) {
    lastPrint = millis();
    Serial.print("Stations: ");
    Serial.println(WiFi.softAPgetStationNum());
  }
}