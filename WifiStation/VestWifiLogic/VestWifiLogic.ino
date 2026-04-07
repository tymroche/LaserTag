#include <WiFi.h>


const char* hostSsid = "ESP32S3-Soft-AP";
const char* hostPassword = "lasertag";

IPAddress hostIP(192, 168, 4, 1);
const uint16_t hostPort = 3333;

WiFiClient hostClient;

String vestDeviceId = "vestA";
String playerName = "Player 1";
String assignedPlayerId = "";

bool canShoot = true;
bool alive = true;
String currentMode = "Free For All";
int currentHp = 5;
int currentScore = 0;
int currentTimer = 90;

unsigned long lastReconnectAttempt = 0;

/*
  Connect this vest to the host SoftAP and then open the persistent TCP
  connection.
*/
void connectVestToHost() {
  if (millis() - lastReconnectAttempt < 2000) return;
  lastReconnectAttempt = millis();

  if (WiFi.status() != WL_CONNECTED) {
    WiFi.mode(WIFI_STA);
    WiFi.begin(hostSsid, hostPassword);

    unsigned long start = millis();
    while (WiFi.status() != WL_CONNECTED && millis() - start < 8000) {
      delay(100);
    }

    if (WiFi.status() == WL_CONNECTED) {
      Serial.println("Connected to host SoftAP");
      Serial.print("Vest IP: ");
      Serial.println(WiFi.localIP());
    }
  }

  if (WiFi.status() == WL_CONNECTED && !hostClient.connected()) {
    hostClient.stop();
    hostClient.connect(hostIP, hostPort);

    if (hostClient.connected()) {
      hostClient.println("HELLO " + vestDeviceId + " " + playerName);
      Serial.println("Connected to host device server");
    }
  }
}

/*
  Parse one STATE line sent from the host.
  e.g, STATE PLAYERID=x01 MODE=Free For All ALIVE=1 CANSHOOT=1 HP=5 SCORE=0 TIMER=90
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
    end = line.indexOf(' ', idx);
    if (end < 0) end = line.length();
    currentMode = line.substring(idx + 5, end);
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

/*
  Read any pushed state updates from the host.
*/
void handleHostMessages() {
  if (!hostClient.connected()) {
    connectVestToHost();
    return;
  }

  while (hostClient.available()) {
    String line = hostClient.readStringUntil('\n');
    line.trim();
    parseHostStateLine(line);
  }
}

/*
  Return whether this vest is currently allowed to shoot.
*/
bool vestCanFire() {
  return hostClient.connected() && alive && canShoot && assignedPlayerId.length() > 0;
}

/*
  Send a hit report to the host.
  Eg
  HIT x02
  vest was hit by player x02.
*/
void reportHitToHost(const String& attackerPlayerId) {
  if (!hostClient.connected()) return;
  if (attackerPlayerId.length() == 0) return;

  hostClient.println("HIT " + attackerPlayerId);

  Serial.print("Reported hit by ");
  Serial.println(attackerPlayerId);
}

/*
  Fire this vest's IR shot if allowed.
*/
void fireShot() {
  if (!vestCanFire()) {
    Serial.println("Dead no shoot");
    return;
  }

  Serial.print("Shot allowed, ");
  Serial.println(assignedPlayerId);
}

/*
  Check IR receiver for hits.
  When hit, decode the attacker's player ID
  from the IR signal and report it to the host.
*/

void setup() {
  Serial.begin(115200);
  connectVestToHost();
}

void loop() {
  handleHostMessages();
  handleTrigger();
  handleIncomingHits();
  updateStatusLed();
}