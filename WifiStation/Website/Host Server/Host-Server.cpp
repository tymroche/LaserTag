/**
 * @file Host-Server.cpp
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Implementation of Host Server and HTML.
 *
 * This file defines all variables, structs, and functions necessary for Laser Tag Game.
 * 
 * @see config.h for the hardware definitions like GPIO mapping
 */

#include <WiFi.h>
#include "Host-Server.h"

const char* ssid = "LaserTagSoftAP"
const char* password = "lasertag"

WiFiServer webServer(80); //Socket on port 80 for HTTP requests.
WiFiServer deviceServer(27015); //Socket on port 27015 for TCP requests between player and host.

const int MAX_PLAYERS = 4;
const int STARTING_HP = 5;
const int STARTING_FFA_POINTS = 0;
const int FFA_HIT_GAIN = 100;
const int FFA_HIT_LOSS = 50;
const int DEFAULT_FFA_SECONDS = 90;

String gameMode = "Free For All";
String statusMessage = "";

int rankedCount = 0;

int ffaDurationSeconds = DEFAULT_FFA_SECONDS;
bool ffaTimerRunning = false;
unsigned long ffaStartMillis = 0;

/**
 * @brief Decodes URL characters from browser request.
 * @param s URL-encoded string to decode.
 * @return Decoded, trimmed string.
 */
String decodeUrl(String s) {
  s.replace("%20", " ");
  s.replace("+", " ");
  s.replace("%21", "!");
  s.replace("%40", "@");
  s.replace("%23", "#");
  s.replace("%24", "$");
  s.replace("%26", "&");
  s.replace("%27", "'");
  s.replace("%28", "(");
  s.replace("%29", ")");
  s.trim();
  return s;
}

/**
 * @brief Extracts query parameter from HTTP GET request.
 * @param req HTTP request string.
 * @param key Query parameter to extract.
 * @return Decoded value of the requested parameter.
 */
String getQueryValue(String req, String key) {
  //GET /submit?name=Alfonso%20Landaverde&mode=ffa HTTP/1.1
  int start = req.indexOf(key + "="); //name=
  if (start < 0) return "";

  start += key.length() + 1; //point to actual key value

  int end = req.indexOf("&", start);
  int httpEnd = req.indexOf(" HTTP/1.1", start);

  if (end < 0 || (httpEnd >= 0 && httpEnd < end)) {
    end = httpEnd;
  }
  if (end < 0) return "";

  return decodeUrl(req.substring(start, end)); //return parsed value
}

/*
  Convert a player slot number to the assigned IR/player ID.
  slot 0 x01
  slot 1 x02
  slot 2 x03
  slot 3 x04
*/
/**
 * @brief Converts player slot index to assigned player ID for IR.
 * @param slot player slot index.
 * @return Player ID (x01-x04) for valid slots, or an empty string for an invalid slot.
 */
String slotToPlayerId(int slot) {
  if (slot < 0 || slot >= MAX_PLAYERS) return "";
  if (slot < 3) return "x0" + String(slot + 1);
  return "x" + String(slot + 1);
}

/**
 * @brief Find a player slot by displayed name.
 * @param name Player name to search for.
 * @return Matching player slot index or -1.
 */
int findPlayerByName(String name) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active && players[i].name == name) {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Find a player slot by vest ID.
 * @param vestDeviceId Vest ID.
 * @return Matching player slot or -1.
 */
int findPlayerByVestDeviceId(String vestDeviceId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active && players[i].vestDeviceId == vestDeviceId) {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Find a player slot by assigned game player ID.
 * @param playerId Assigned player ID.
 * @return Matching player slot index or -1.
 */
int findPlayerByAssignedPlayerId(String playerId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active && players[i].playerId == playerId) {
      return i;
    }
  }
  return -1;
}

/**
 * @brief Finds the first unused player slot.
 * @return Index of the first free player slot, or -1 if the lobby is full.
 */
int findOpenSlot() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (!players[i].active) return i;
  }
  return -1;
}

/**
 * @brief Resets one player's stats for the current mode.
 * @param index Player slot index to reset.
 */
void resetPlayerForCurrentMode(int index) {
  players[index].hp = STARTING_HP;
  players[index].score = STARTING_FFA_POINTS;
  players[index].alive = true;
  players[index].canShoot = true;
}

/**
 * @brief Initializes/resets all player records and closes any client connections.
 */
void resetAllPlayers() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    players[i].name = "";
    players[i].vestDeviceId = "";
    players[i].playerId = "";
    players[i].hp = STARTING_HP;
    players[i].score = STARTING_FFA_POINTS;
    players[i].active = false;
    players[i].alive = true;
    players[i].canShoot = true;
    deviceClients[i].stop();
  }
}

/*UNDER CONSTRUCTION, WOULD LIKE TO HAVE TIMER ACTIVELY DISPLAY TIME*/
/**
 * @brief Computes the time remaining for Free For All timer.
 * @return Remaining timer value in seconds, or zero once the timer has expired.
 */
int getRemainingFFATime() {
  if (!ffaTimerRunning) return ffaDurationSeconds;

  int elapsed = (millis() - ffaStartMillis) / 1000;
  int remaining = ffaDurationSeconds - elapsed;

  if (remaining <= 0) {
    ffaTimerRunning = false;
    return 0;
  }

  return remaining;
}

/**
 * @brief Formats seconds as minutes:seconds.
 * @param totalSeconds Total seconds.
 * @return Formatted string in M:SS form.
 */
String formatTime(int totalSeconds) {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  String out = String(minutes) + ":";
  if (seconds < 10) out += "0";
  out += String(seconds);
  return out;
}

/**
 * @brief Send the current host state to one vest.
 * Includes the assigned PLAYERID that the vest should use in IR transmit.
 * @param index Player slot whose vest should receive the state update.
 */
void sendStateToPlayer(int index) {
  if (index < 0 || index >= MAX_PLAYERS) return;
  if (!players[index].active) return;
  if (!deviceClients[index] || !deviceClients[index].connected()) return;

  int timerValue = 0;
  if (gameMode == "Free For All") {
    timerValue = getRemainingFFATime();
  }

  String msg = "STATE ";
  msg += "PLAYERID=" + players[index].playerId;
  msg += " MODE=" + gameMode;
  msg += " ALIVE=" + String(players[index].alive ? 1 : 0);
  msg += " CANSHOOT=" + String(players[index].canShoot ? 1 : 0);
  msg += " HP=" + String(players[index].hp);
  msg += " SCORE=" + String(players[index].score);
  msg += " TIMER=" + String(timerValue);

  deviceClients[index].println(msg);
}

/**
 * @brief Sends game state to every player slot.
 */
void sendStateToAllPlayers() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    sendStateToPlayer(i);
  }
}

/*
  TEST FUNCTION FOR PLAYER SLOT VALIDITY; REMOVE WHEN DONE
*/
/**
 * @brief Adds a browser-only player to the lobby if a name is valid and a slot is available.
 * @param name Display name to assign to the new browser-only player.
 */
void addBrowserPlayer(String name) {
  if (name.length() == 0) {
    statusMessage = "Please enter a name.";
    return;
  }
  if (findPlayerByName(name) >= 0) {
    statusMessage = name + " is already connected.";
    return;
  }
  int slot = findOpenSlot();
  if (slot < 0) {
    statusMessage = "Lobby is full.";
    return;
  }

  players[slot].name = name;
  players[slot].vestDeviceId = "";
  players[slot].playerId = slotToPlayerId(slot);
  players[slot].active = true;
  resetPlayerForCurrentMode(slot);

  statusMessage = name + " joined the game as " + players[slot].playerId + ".";
}


/**
 * @brief Removes one player from the lobby and disconnects any linked vest client.
 * @param index Player slot index to remove.
 */
void removePlayer(int index) {
  if (index < 0 || index >= MAX_PLAYERS) return;
  if (!players[index].active) return;

  String removedName = players[index].name;

  if (deviceClients[index]) {
    deviceClients[index].println("STATE PLAYERID= MODE=" + gameMode + " ALIVE=0 CANSHOOT=0 HP=0 SCORE=0 TIMER=0");
    deviceClients[index].stop();
  }

  players[index].name = "";
  players[index].vestDeviceId = "";
  players[index].playerId = "";
  players[index].hp = STARTING_HP;
  players[index].score = STARTING_FFA_POINTS;
  players[index].active = false;
  players[index].alive = true;
  players[index].canShoot = true;

  statusMessage = removedName + " was removed.";
}

/**
 * @brief Removes all player from lobby.
 */
void removeAllPlayers() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    removePlayer(i);
  }
  statusMessage = "All players removed.";
}

/**
 * @brief Compares two active player slots to determine scoreboard ordering.
 * @param a First player slot index to compare.
 * @param b Second player slot index to compare.
 * @return true if player a should rank above player b; otherwise false.
 */
bool ranksHigher(int a, int b) {
  if (gameMode == "Duels") {
    if (players[a].hp != players[b].hp) return players[a].hp > players[b].hp;
  } else {
    if (players[a].score != players[b].score) return players[a].score > players[b].score;
  }

  return players[a].name < players[b].name;
}

/**
 * @brief Builds sorted player list used by scoreboard.
 */
void buildRankings() {
  rankedCount = 0;

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active) {
      rankedPlayers[rankedCount++] = i;
    }
  } // build array

  for (int i = 0; i < rankedCount - 1; i++) {
    for (int j = 0; j < rankedCount - 1 - i; j++) {
      int left = rankedPlayers[j];
      int right = rankedPlayers[j + 1];

      if (!ranksHigher(left, right)) {
        int temp = rankedPlayers[j];
        rankedPlayers[j] = rankedPlayers[j + 1];
        rankedPlayers[j + 1] = temp;
      } 
    }
  } //compares all players
}

/**
 * @brief Disables shooting for all active players and sends updated state.
 */
void disableAllShooting() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active) {
      players[i].canShoot = false;
      sendStateToPlayer(i);
    }
  }
}

/*
  Start, stop, reset FFA timer.
*/

/**
 * @brief Starts Free For All timer and broadcasts updated state.
 */
void startFFATimer() {
  ffaTimerRunning = true;
  ffaStartMillis = millis();
  statusMessage = "Free For All timer started.";
  sendStateToAllPlayers();
}

/**
 * @brief Stops Free For All timer and broadcasts updated state.
 */
void stopFFATimer() {
  ffaTimerRunning = false;
  statusMessage = "Free For All timer stopped.";
  sendStateToAllPlayers();
}

/**
 * @brief Resets the Free For All timer state and broadcasts updated state.
 */
void resetFFATimer() {
  ffaTimerRunning = false;
  statusMessage = "Free For All timer reset.";
  sendStateToAllPlayers();
}

/**
 * @brief Changes current game mode and resets all active players.
 * @param newMode New game mode string to apply.
 */
void setGameMode(String newMode) {
  gameMode = newMode;
  ffaTimerRunning = false;

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active) {
      resetPlayerForCurrentMode(i);
    }
  }

  statusMessage = "Gamemode set to " + gameMode + ".";
  sendStateToAllPlayers();
}

/**
 * @brief Registers a newly connected vest or reconnects an existing vest to a player slot.
 * @param vestDeviceId Unique device identifier reported by the vest.
 * @param name Player name associated with the vest.
 * @param client Connected TCP client for that vest.
 */
void registerVest(String vestDeviceId, String name, WiFiClient client) {
  if (vestDeviceId.length() == 0 || name.length() == 0) {
    client.stop();
    return;
  }

  int existingVest = findPlayerByVestDeviceId(vestDeviceId);
  if (existingVest >= 0) {
    if (deviceClients[existingVest]) {
      deviceClients[existingVest].stop();
    }
    deviceClients[existingVest] = client;
    players[existingVest].name = name;
    players[existingVest].active = true;
    statusMessage = name + " reconnected as " + players[existingVest].playerId + ".";
    sendStateToPlayer(existingVest);
    return;
  }

  int existingName = findPlayerByName(name);
  if (existingName >= 0) {
    if (deviceClients[existingName]) {
      deviceClients[existingName].stop();
    }
    deviceClients[existingName] = client;
    players[existingName].vestDeviceId = vestDeviceId;
    if (players[existingName].playerId == "") {
      players[existingName].playerId = slotToPlayerId(existingName);
    }
    statusMessage = name + " device linked as " + players[existingName].playerId + ".";
    sendStateToPlayer(existingName);
    return;
  }

  int slot = findOpenSlot();
  if (slot < 0) {
    client.println("STATE PLAYERID= MODE=" + gameMode + " ALIVE=0 CANSHOOT=0 HP=0 SCORE=0 TIMER=0");
    client.stop();
    statusMessage = "Lobby is full.";
    return;
  }

  players[slot].name = name;
  players[slot].vestDeviceId = vestDeviceId;
  players[slot].playerId = slotToPlayerId(slot);
  players[slot].active = true;
  resetPlayerForCurrentMode(slot);

  deviceClients[slot] = client;

  statusMessage = name + " joined as " + players[slot].playerId + ".";
  sendStateToPlayer(slot);
}


/*
  Process one hit report.
  e.g. HIT x02
*/

/**
 * @brief Processes a hit report sent by a vest and applies the appropriate scoring or HP changes.
 * @param targetIndex Player slot index of the vest that reported being hit.
 * @param attackerId Assigned player ID of the attacker IR.
 */
void processHitReport(int targetIndex, String attackerId) {
  int attackerIndex = findPlayerByAssignedPlayerId(attackerId);

  if (targetIndex < 0 || targetIndex >= MAX_PLAYERS) return;
  if (attackerIndex < 0 || attackerIndex >= MAX_PLAYERS) return;
  if (!players[targetIndex].active || !players[attackerIndex].active) return;

  if (gameMode == "Free For All") {
    if (getRemainingFFATime() <= 0) return;
    if (!players[attackerIndex].canShoot) return;

    players[attackerIndex].score += FFA_HIT_GAIN;
    players[targetIndex].score -= FFA_HIT_LOSS;

    statusMessage = players[attackerIndex].name + " hit " + players[targetIndex].name + ".";

    sendStateToPlayer(attackerIndex);
    sendStateToPlayer(targetIndex);
    return;
  }

  if (gameMode == "Duels") {
    if (!players[attackerIndex].canShoot) return;
    if (!players[targetIndex].alive) return;

    players[targetIndex].hp -= 1;

    if (players[targetIndex].hp <= 0) {
      players[targetIndex].hp = 0;
      players[targetIndex].alive = false;
      players[targetIndex].canShoot = false;
      statusMessage = players[targetIndex].name + " is out.";
    } else {
      statusMessage = players[targetIndex].name + " was hit.";
    }

    sendStateToPlayer(attackerIndex);
    sendStateToPlayer(targetIndex);
  }
}


/*
  Accept new vest connections and process vest messages.
    HELLO <vestDeviceId> <playerName>
    HIT <attackerPlayerId>
*/

/**
 * @brief Accepts new vest TCP connections and processes messages from connected vests.
 */
void handleDeviceConnections() {
  WiFiClient newClient = deviceServer.available();

  if (newClient) { // new connection message
    unsigned long start = millis();
    String hello = "";

    while (newClient.connected() && millis() - start < 1000) {
      while (newClient.available()) {
        char c = newClient.read();
        if (c == '\n') {
          hello.trim();
          start = 2000;
          break;
        }
        if (c != '\r') hello += c;
      }
      if (start == 2000) break;
    }

    if (hello.startsWith("HELLO ")) { // make new connection in server
      int firstSpace = hello.indexOf(' ', 6);
      if (firstSpace > 0) {
        String vestDeviceId = hello.substring(6, firstSpace);
        String name = hello.substring(firstSpace + 1);
        name.trim();
        registerVest(vestDeviceId, name, newClient);
      } else {
        newClient.stop();
      }
    } else {
      newClient.stop();
    }
  }

  for (int i = 0; i < MAX_PLAYERS; i++) { //message processing loop
    if (deviceClients[i] && deviceClients[i].connected() && deviceClients[i].available()) {
      String line = deviceClients[i].readStringUntil('\n');
      line.trim();

      if (line.startsWith("HIT ")) {
        String attackerId = line.substring(4);
        attackerId.trim();
        processHitReport(i, attackerId);
      }
    }
  }
}

/**
 * @brief Processes a browser request.
 * @param req HTTP request string to inspect for actions.
 */
void handleBrowserAction(String req) {
  if (req.indexOf("GET /join?name=") >= 0) {
    addBrowserPlayer(getQueryValue(req, "name"));
  } else if (req.indexOf("GET /mode/ffa") >= 0) {
    setGameMode("Free For All");
  } else if (req.indexOf("GET /mode/duels") >= 0) {
    setGameMode("Duels");
  } else if (req.indexOf("GET /remove?slot=") >= 0) {
    removePlayer(getQueryValue(req, "slot").toInt());
  } else if (req.indexOf("GET /clear") >= 0) {
    removeAllPlayers();
  } else if (req.indexOf("GET /time/add15") >= 0) {
  ffaDurationSeconds += 15;
  statusMessage = "Free For All time set to " + formatTime(ffaDurationSeconds) + ".";
  sendStateToAllPlayers();
} else if (req.indexOf("GET /time/sub15") >= 0) {
  if (ffaDurationSeconds > 15) {
    ffaDurationSeconds -= 15;
  } else {
    ffaDurationSeconds = 15;
  }
  statusMessage = "Free For All time set to " + formatTime(ffaDurationSeconds) + ".";
  sendStateToAllPlayers();
  } else if (req.indexOf("GET /timer/start") >= 0) {
    if (gameMode == "Free For All") startFFATimer();
  } else if (req.indexOf("GET /timer/stop") >= 0) {
    stopFFATimer();
  } else if (req.indexOf("GET /timer/reset") >= 0) {
    resetFFATimer();
  }
}

/**
 * @brief Sends an HTTP redirect response back to the browser homepage.
 * clears url as well to prevent duplicate actions.
 * @param client Connected web client to respond to.
 */
void redirectToHome(WiFiClient& client) {
  client.println("HTTP/1.1 303 See Other");
  client.println("Location: /");
  client.println("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
  client.println("Pragma: no-cache");
  client.println("Expires: 0");
  client.println("Connection: close");
  client.println();
}


/**
 * @brief Generates and sends the current control page and scoreboard HTML to a browser client.
 * @param client Connected web client to respond to.
 */
void serveWebPage(WiFiClient& client) {
  buildRankings();

  client.println("HTTP/1.1 200 OK");
  client.println("Content-type:text/html");
  client.println("Connection: close");
  client.println("Cache-Control: no-store, no-cache, must-revalidate, max-age=0");
  client.println("Pragma: no-cache");
  client.println("Expires: 0");
  client.println();

  client.println("<!DOCTYPE html><html>");
  client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");

  client.println("<style>");
  client.println("body { margin: 0; padding: 16px; font-family: Helvetica, Arial, sans-serif; background: #111; color: white; text-align: center; }");
  client.println(".container { max-width: 420px; margin: 0 auto; }");
  client.println(".card { background: #1e1e1e; border-radius: 14px; padding: 18px; margin-bottom: 16px; box-shadow: 0 2px 8px rgba(0,0,0,0.35); }");
  client.println("h1 { margin: 0; font-size: 2rem; }");
  client.println("h2 { margin-top: 0; margin-bottom: 12px; font-size: 1.3rem; }");
  client.println(".mode { font-size: 1.5rem; font-weight: bold; margin-bottom: 12px; }");
  client.println(".mode-buttons { display: flex; gap: 10px; justify-content: center; flex-wrap: wrap; }");
  client.println(".mode-btn { display: inline-block; width: 44%; max-width: 160px; padding: 12px; border-radius: 10px; text-decoration: none; color: white; background: #2f80ed; font-size: 1rem; }");
  client.println(".mode-btn.active { background: #27ae60; }");
  client.println(".time-btn { display: inline-block; margin: 6px; padding: 10px 14px; border-radius: 8px; text-decoration: none; color: white; background: #16a085; font-size: 0.95rem; }");
  client.println(".time-btn.active { background: #27ae60; }");
  client.println(".timer-btn { display: inline-block; margin: 6px; padding: 10px 14px; border-radius: 8px; text-decoration: none; color: white; background: #2f80ed; font-size: 0.95rem; }");
  client.println("input[type=text] { width: 90%; padding: 12px; font-size: 1rem; border-radius: 10px; border: none; margin-top: 10px; margin-bottom: 12px; }");
  client.println("button { width: 95%; padding: 12px; font-size: 1rem; border: none; border-radius: 10px; background: #2f80ed; color: white; }");
  client.println(".status { font-size: 1rem; font-weight: bold; margin-top: 12px; }");
  client.println(".player { padding: 12px; border-radius: 10px; background: #2a2a2a; margin-bottom: 10px; text-align: left; }");
  client.println(".player-name { font-size: 1.1rem; font-weight: bold; margin-bottom: 6px; }");
  client.println(".player-stat { font-size: 1rem; margin: 3px 0; }");
  client.println(".remove-btn { display: inline-block; margin-top: 10px; padding: 10px 14px; border-radius: 8px; text-decoration: none; background: #c0392b; color: white; font-size: 0.95rem; }");
  client.println(".clear-btn { display: inline-block; margin-top: 8px; padding: 12px 16px; border-radius: 10px; text-decoration: none; background: #8e44ad; color: white; font-size: 1rem; width: 87%; }");
  client.println(".hint { font-size: 0.9rem; opacity: 0.8; margin-top: 10px; }");
  client.println(".timer-value { font-size: 1.6rem; font-weight: bold; margin-bottom: 12px; }");
  client.println(".rank { opacity: 0.8; margin-right: 6px; }");
  client.println("</style>");

  client.println("</head><body><div class=\"container\">");

  client.println("<div class=\"card\"><h1>Laser Tag</h1></div>");

  client.println("<div class=\"card\">");
  client.println("<h2>Gamemode</h2>");
  client.println("<div class=\"mode\">" + gameMode + "</div>");
  client.println("<div class=\"mode-buttons\">");

  if (gameMode == "Free For All") {
    client.println("<a class=\"mode-btn active\" href=\"/mode/ffa\">Free For All</a>");
    client.println("<a class=\"mode-btn\" href=\"/mode/duels\">Duels</a>");
  } else {
    client.println("<a class=\"mode-btn\" href=\"/mode/ffa\">Free For All</a>");
    client.println("<a class=\"mode-btn active\" href=\"/mode/duels\">Duels</a>");
  }

  client.println("</div>");
  client.println("<div class=\"hint\">Pick a mode.</div>");
  client.println("</div>");

  if (gameMode == "Free For All") {
    client.println("<div class=\"card\">");
    client.println("<h2>Free For All Timer</h2>");
    client.println("<div style=\"display:flex; align-items:center; justify-content:center; gap:12px; margin-bottom:12px;\">");
    client.println("<a class=\"time-btn\" href=\"/time/sub15\">- 15 sec</a>");
    client.println("<div class=\"timer-value\" style=\"margin-bottom:0; min-width:90px;\">" + formatTime(getRemainingFFATime()) + "</div>");
    client.println("<a class=\"time-btn\" href=\"/time/add15\">+ 15 sec</a>");
    client.println("</div>");

    client.println("<div>");
    client.println("<a class=\"timer-btn\" href=\"/timer/start\">Start Game</a>");
    client.println("<a class=\"timer-btn\" href=\"/timer/stop\">Stop Game</a>");
    client.println("</div>");
  }

  client.println("<div class=\"card\">");
  client.println("<h2>Join Game</h2>");
  client.println("<form action=\"/join\" method=\"get\">");
  client.println("<input type=\"text\" name=\"name\" placeholder=\"Enter a browser-only name\">");
  client.println("<br><button type=\"submit\">Join</button></form>");

  if (statusMessage.length() > 0) {
    client.println("<div class=\"status\">" + statusMessage + "</div>");
  }

  client.println("</div>");

  client.println("<div class=\"card\">");
  client.println("<h2>Scoreboard</h2>");

  if (rankedCount == 0) {
    client.println("<div class=\"player\">");
    client.println("<div class=\"player-name\">No players yet</div>");
    client.println("<div class=\"player-stat\">Join to appear on the scoreboard.</div>");
    client.println("</div>");
  } else {
    for (int k = 0; k < rankedCount; k++) {
      int i = rankedPlayers[k];

      client.println("<div class=\"player\">");
      client.println("<div class=\"player-name\"><span class=\"rank\">#" + String(k + 1) + "</span>" + players[i].name + "</div>");

      if (gameMode == "Duels") {
        client.println("<div class=\"player-stat\">HP: " + String(players[i].hp) + "</div>");
      } else {
        client.println("<div class=\"player-stat\">Points: " + String(players[i].score) + "</div>");
      }

      if (players[i].playerId.length() > 0) {
        client.println("<div class=\"player-stat\">Player ID: " + players[i].playerId + "</div>");
      }

      if (players[i].vestDeviceId.length() > 0) {
        client.println("<div class=\"player-stat\">Vest: " + players[i].vestDeviceId + "</div>");
      }

      client.println("<a class=\"remove-btn\" href=\"/remove?slot=" + String(i) + "\">Remove Player</a>");
      client.println("</div>");
    }
  }

  client.println("<a class=\"clear-btn\" href=\"/clear\">Remove All Players</a>");
  client.println("</div>");

  client.println("</div></body></html>");
  client.println();
}

/**
 * @brief Accepts browser connections, parses requests, and serves html page.
 */
void handleWebClients() {
  WiFiClient client = webServer.available();
  if (!client) return;

  String currentLine = "";
  String requestLine = "";

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      header += c;

      if (c == '\n') {
        if (requestLine.length() == 0 && currentLine.length() > 0) {
          requestLine = currentLine;
        }

        if (currentLine.length() == 0) {
          Serial.print("Stations: ");
          Serial.print(WiFi.softAPgetStationNum());
          Serial.println(" | New Client");

          bool isActionRequest =
            requestLine.indexOf("GET /join?name=") >= 0 ||
            requestLine.indexOf("GET /mode/ffa") >= 0 ||
            requestLine.indexOf("GET /mode/duels") >= 0 ||
            requestLine.indexOf("GET /remove?slot=") >= 0 ||
            requestLine.indexOf("GET /clear") >= 0 ||
            requestLine.indexOf("GET /time/add15") >= 0 ||
            requestLine.indexOf("GET /time/sub15") >= 0 ||
            requestLine.indexOf("GET /timer/start") >= 0 ||
            requestLine.indexOf("GET /timer/stop") >= 0 ||
            requestLine.indexOf("GET /timer/reset") >= 0;

          handleBrowserAction(header);

          if (isActionRequest) {
            redirectToHome(client);
          } else {
            serveWebPage(client);
          }
          break;
        } else {
          currentLine = "";
        }
      } else if (c != '\r') {
        currentLine += c;
      }
    }
  }

  header = "";
  client.stop();
}
