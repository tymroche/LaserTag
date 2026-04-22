/**
 * @file Vest-Wifi-Protocol.cpp
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Implementation of Client (Vest, Players) Communication Protocols.
 *
 * This file defines all variables, structs, and functions necessary for Laser Tag Game.
 * 
 * @see config.h for the hardware definitions like GPIO mapping
 */

#include <WiFi.h>
#include "Vest-Wifi-Protocol.h"

const char* hostSsid = "LaserTagSoftAP";
const char* hostPassword = "lasertag";

IPAddress hostIP(192, 168, 4, 1);
const uint16_t hostPort = 27015;

WiFiClient hostClient;

//unique to hardware, ESP32 
String vestDeviceId = "vestD";
String playerName = "Diego";

//unique to online session
String assignedPlayerId = "";

bool canShoot = true;
bool alive = true;
bool sessionRegistered = false;
String currentMode = "Free For All";
int currentHp = 5;
int currentScore = 0;
int currentTimer = 90;

unsigned long lastReconnectAttempt = 0;

/**
 * @brief Connects vest to host SoftAP and opens persistent TCP connection to host device server.
 */
void connectVestToHost() {
  static unsigned long lastStatusPrint = 0;
  wl_status_t s = WiFi.status();

  if (s != WL_CONNECTED) {
    sessionRegistered = false;

    if (millis() - lastStatusPrint >= 2000) {
      lastStatusPrint = millis();
      Serial.print("WiFi status: ");
      Serial.println((int)s);
    }

    if (millis() - lastReconnectAttempt >= 10000) {
      lastReconnectAttempt = millis();
      Serial.println("Retrying Wi-Fi join...");
      WiFi.disconnect(true, true);
      delay(500);
      WiFi.begin(hostSsid, hostPassword);
    }
    return;
  }

  if (!hostClient.connected()) {
    sessionRegistered = false;

    if (millis() - lastReconnectAttempt >= 2000) {
      lastReconnectAttempt = millis();

      hostClient.stop();
      Serial.println("Connecting to host TCP server...");

      if (hostClient.connect(hostIP, hostPort)) {
        Serial.println("TCP connected, sending HELLO once");
        hostClient.println("HELLO " + vestDeviceId + " " + playerName);
        playConnectedBeeps();
      } else {
        Serial.println("TCP connection failed");
      }
    }
    return;
  }

  // already connected, do not keep re-registering
}
/*
  Parse one STATE line sent from the host.
  e.g, STATE PLAYERID=1 MODE=Free For All ALIVE=1 CANSHOOT=1 HP=5 SCORE=0 TIMER=90
*/
/**
 * @brief Parses a STATE message received from host and updates the vest's game variables.
 * @param line Complete STATE line received from the host TCP connection.
 */
void parseHostStateLine(String line) {
  if (!line.startsWith("STATE ")) return;

  int idx;
  int end;

  idx = line.indexOf("PLAYERID=");
  if (idx >= 0) {
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    assignedPlayerId = line.substring(idx + 9, end);
  }

  idx = line.indexOf("MODE=");
  if (idx >= 0) {
    int modeStart = idx + 5;
    int modeEnd = line.indexOf(" ALIVE=", modeStart);
    if (modeEnd < 0) modeEnd = line.length();
    currentMode = line.substring(modeStart, modeEnd);
  }

  idx = line.indexOf("ALIVE=");
  if (idx >= 0) {
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    alive = line.substring(idx + 6, end).toInt() == 1;
  }

  idx = line.indexOf("CANSHOOT=");
  if (idx >= 0) {
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    canShoot = line.substring(idx + 9, end).toInt() == 1;
  }

  idx = line.indexOf("HP=");
  if (idx >= 0) {
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    currentHp = line.substring(idx + 3, end).toInt();
  }

  idx = line.indexOf("SCORE=");
  if (idx >= 0) {
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    currentScore = line.substring(idx + 6, end).toInt();
  }

  idx = line.indexOf("TIMER=");
  if (idx >= 0) {
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    currentTimer = line.substring(idx + 6, end).toInt();
  }

  idx = line.indexOf("PLAYERID=");
  if (idx >= 0) {
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    assignedPlayerId = line.substring(idx + 9, end);

    if (assignedPlayerId.length() > 0) {
      sessionRegistered = true;
    }
  }

  Serial.print("Assigned ID: ");
  Serial.print(assignedPlayerId);
  Serial.print(" | Mode: ");
  Serial.print(currentMode);
  Serial.print(" | Alive: ");
  Serial.print(alive);
  Serial.print(" | CanShoot: ");
  Serial.print(canShoot);
  Serial.print(" | HP: ");
  Serial.print(currentHp);
  Serial.print(" | Score: ");
  Serial.print(currentScore);
  Serial.print(" | Timer: ");
  Serial.println(currentTimer);
}

/**
 * @brief Read any pushed state updates from the host.
 */
void handleHostMessages() {
  while (hostClient.available()) {
    String line = hostClient.readStringUntil('\n');
    line.trim();
    parseHostStateLine(line);
  }
}

/**
 * @brief Checks if vest is currently allowed to fire.
 * @return true if so, false otherwise.
 */
bool vestCanFire() {
  return hostClient.connected() && alive && canShoot && currentTimer > 0 && assignedPlayerId.length() > 0;
}
}

/*
  Send a hit report to the host.
  Eg
  HIT 2
  vest was hit by player 2.
*/

/**
 * @brief Sends a hit report to the host identifying which player ID hit this vest.
 * @param attackerPlayerId Assigned player ID of attacker, decoded from IR.
 */
void reportHitToHost(const String& attackerPlayerId) {
  if (!hostClient.connected()) return;
  if (attackerPlayerId.length() == 0) return;

  hostClient.println("HIT " + attackerPlayerId);

  Serial.print("Reported hit by ");
  Serial.println(attackerPlayerId);
}

/**
 * @brief Attempts to fire and prints player ID when firing is allowed.
 */
void fireShot() {
  if (!vestCanFire()) {
    Serial.println("Dead no shoot");
    return;
  }

  Serial.print("Shot allowed, ");
  Serial.println(assignedPlayerId);
}