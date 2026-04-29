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

const char* ssid = "LaserTagSoftAP";
const char* password = "lasertag";

WiFiServer webServer(80); //Socket on port 80 for HTTP requests.
WiFiServer deviceServer(27015); //Socket on port 27015 for TCP requests between player and host.

String header = "";
Player players[MAX_PLAYERS];
WiFiClient deviceClients[MAX_PLAYERS];
int rankedPlayers[MAX_PLAYERS];

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
bool duelsGameRunning = false;
bool roundOver = false;
String roundOverMessage = "";
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


/**
 * @brief Converts player slot index to assigned player ID for IR.
 * @param slot player slot index.
 * @return Player ID for valid slots, or an empty string for an invalid slot.
 */
String slotToPlayerId(int slot) {
  if (slot < 0 || slot >= MAX_PLAYERS) return "";
  return String(slot + 1);
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
  players[index].canShoot = false;
}

/**
 * @brief Resets all player records and closes any client connections.
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
    players[i].canShoot = false;
    deviceClients[i].stop();
  }
  ffaTimerRunning = false;
  duelsGameRunning = false;
  roundOver = false;
  roundOverMessage = "";
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
 * @brief Clears the current round-over state and winner message.
 */
void clearRoundOverState() {
  roundOver = false;
  roundOverMessage = "";
}

/**
 * @brief Checks whether the current round has ended and updates round-over state.
 * @return true if the round is over, otherwise false.
 */
bool isRoundOver() {
  if (gameMode == "Free For All") {
    if (!ffaTimerRunning && getRemainingFFATime() == 0) {
      int bestIndex = -1;
      bool tie = false;

      for (int i = 0; i < MAX_PLAYERS; i++) {
        if (!players[i].active) continue;

        if (bestIndex < 0 || players[i].score > players[bestIndex].score) {
          bestIndex = i;
          tie = false;
        } else if (bestIndex >= 0 && players[i].score == players[bestIndex].score) {
          tie = true;
        }
      }

      roundOver = true;
      if (bestIndex < 0) {
        roundOverMessage = "Round Over - No winner.";
      } else if (tie) {
        roundOverMessage = "Round Over - Tie game!";
      } else {
        roundOverMessage = "Round Over - " + players[bestIndex].name + " wins Free For All!";
      }

      return true;
    }
    return false;
  }

  if (gameMode == "Duels") {
    if (!duelsGameRunning) return roundOver;

    int aliveCount = 0;
    int winnerIndex = -1;

    for (int i = 0; i < MAX_PLAYERS; i++) {
      if (!players[i].active) continue;
      if (!players[i].alive) continue;
      aliveCount++;
      winnerIndex = i;
    }

    if (aliveCount <= 1) {
      duelsGameRunning = false;
      roundOver = true;

      if (winnerIndex >= 0) {
        roundOverMessage = "Round Over - " + players[winnerIndex].name + " wins Duels!";
      } else {
        roundOverMessage = "Round Over - No winner.";
      }

      sendStateToAllPlayers();
      return true;
    }
  }

  return roundOver;
}

/**
 * @brief Returns true when the current mode is actively running.
 * @return true if the current mode is in an active round; otherwise false.
 */
bool isGameRunning() {
  if (gameMode == "Free For All") {
    return ffaTimerRunning && getRemainingFFATime() > 0;
  }
  if (gameMode == "Duels") {
    return duelsGameRunning;
  }
  return false;
}

/**
 * @brief Returns the timer value that should be sent to vest clients.
 * @return Remaining FFA time during Free For All, 1 during active Duels, otherwise 0.
 */
int getTimerValueForState() {
  if (gameMode == "Free For All") {
    return getRemainingFFATime();
  }
  if (gameMode == "Duels") {
    return duelsGameRunning ? 1 : 0;
  }
  return 0;
}

/**
 * @brief Applies current mode state to one player's alive/canShoot fields.
 * @param index Player slot index to update.
 */
void syncPlayerRoundState(int index) {
  if (index < 0 || index >= MAX_PLAYERS) return;
  if (!players[index].active) return;

  if (!players[index].alive) {
    players[index].canShoot = false;
    return;
  }

  players[index].canShoot = isGameRunning();
}

/**
 * @brief Applies current mode state to all active players.
 */
void syncAllPlayersRoundState() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    syncPlayerRoundState(i);
  }
}

/**
 * @brief Builds the STATE line sent to one vest client.
 * @param index Player slot index whose state is being serialized.
 * @return Complete STATE message line.
 */
String buildStateMessage(int index) {
  String msg = "STATE ";
  msg += "PLAYERID=" + players[index].playerId;
  msg += " MODE=" + gameMode;
  msg += " ALIVE=" + String(players[index].alive ? 1 : 0);
  msg += " CANSHOOT=" + String(players[index].canShoot ? 1 : 0);
  msg += " HP=" + String(players[index].hp);
  msg += " SCORE=" + String(players[index].score);
  msg += " TIMER=" + String(getTimerValueForState());
  return msg;
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

  syncPlayerRoundState(index);
  deviceClients[index].println(buildStateMessage(index));
}

/**
 * @brief Sends game state to every player slot.
 */
void sendStateToAllPlayers() {
  syncAllPlayersRoundState();
  for (int i = 0; i < MAX_PLAYERS; i++) {
    sendStateToPlayer(i);
  }
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
  players[index].canShoot = false;

  statusMessage = removedName + " was removed.";
}

/**
 * @brief Removes all player from lobby.
 */
void removeAllPlayers() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active) {
      removePlayer(i);
    }
  }
  clearRoundOverState();
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
    }
  }
  sendStateToAllPlayers();
}

/*
  Start, stop, reset FFA timer.
*/

/**
 * @brief Starts Free For All timer and broadcasts updated state.
 */
void startFFATimer() {
  ffaTimerRunning = true;
  duelsGameRunning = false;
  ffaStartMillis = millis();
  clearRoundOverState();
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
  clearRoundOverState();
  statusMessage = "Free For All timer reset.";
  sendStateToAllPlayers();
}

/**
 * @brief Starts Duels game and broadcasts updated state.
 */
void startDuelsGame() {
  duelsGameRunning = true;
  ffaTimerRunning = false;
  clearRoundOverState();
  statusMessage = "Duels game started.";
  sendStateToAllPlayers();
}

/**
 * @brief Stops Duels game and broadcasts updated state.
 */
void stopDuelsGame() {
  duelsGameRunning = false;
  statusMessage = "Duels game stopped.";
  sendStateToAllPlayers();
}

/**
 * @brief Changes current game mode and resets all active players.
 * @param newMode New game mode string to apply.
 */
void setGameMode(String newMode) {
  gameMode = newMode;
  ffaTimerRunning = false;
  duelsGameRunning = false;
  clearRoundOverState();

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active) {
      resetPlayerForCurrentMode(i);
    }
  }

  statusMessage = "Gamemode set to " + gameMode + ". Press Start Game to begin.";
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
    syncPlayerRoundState(existingVest);
    statusMessage = name + " reconnected as " + players[existingVest].playerId + ".";
    sendStateToPlayer(existingVest);
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
  syncPlayerRoundState(slot);

  deviceClients[slot] = client;

  statusMessage = name + " joined as " + players[slot].playerId + ".";
  sendStateToPlayer(slot);
}


/*
  Process one hit report.
  e.g. HIT 2
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
  if (attackerIndex == targetIndex) return;
  if (!players[targetIndex].alive || !players[attackerIndex].alive) return;
  if (!isGameRunning()) return;
  if (!players[attackerIndex].canShoot) return;

  if (gameMode == "Free For All") {
    if (getRemainingFFATime() <= 0) {
      stopFFATimer();
      isRoundOver();
      return;
    }

    players[attackerIndex].score += FFA_HIT_GAIN;
    players[targetIndex].score -= FFA_HIT_LOSS;

    statusMessage = players[attackerIndex].name + " hit " + players[targetIndex].name + ".";

    isRoundOver();
    sendStateToPlayer(attackerIndex);
    sendStateToPlayer(targetIndex);
    return;
  }

  if (gameMode == "Duels") {
    players[targetIndex].hp -= 1;

    if (players[targetIndex].hp <= 0) {
      players[targetIndex].hp = 0;
      players[targetIndex].alive = false;
      players[targetIndex].canShoot = false;
      statusMessage = players[targetIndex].name + " is out.";
    } else {
      statusMessage = players[targetIndex].name + " was hit.";
    }

    isRoundOver();
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
 * @brief Reads the HELLO line from a newly connected vest.
 * @param client Newly connected TCP client.
 * @return Parsed HELLO line, or an empty string on timeout/failure.
 */
String readHelloLine(WiFiClient& client) {
  unsigned long start = millis();
  String hello = "";

  while (client.connected() && millis() - start < 1000) {
    while (client.available()) {
      char c = client.read();
      if (c == '\n') {
        hello.trim();
        return hello;
      }
      if (c != '\r') {
        hello += c;
      }
    }
  }

  return "";
}

/**
 * @brief Removes vest clients that have disconnected from the host.
 */
void cleanupDisconnectedClients() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active && deviceClients[i] && !deviceClients[i].connected()) {
      String removedName = players[i].name;
      removePlayer(i);
      statusMessage = removedName + " disconnected and was removed from the session.";
    }
  }
}

/**
 * @brief Handles one new vest connection if available.
 */
void handleNewDeviceConnection() {
  WiFiClient newClient = deviceServer.available();
  if (!newClient) return;

  String hello = readHelloLine(newClient);
  if (!hello.startsWith("HELLO ")) {
    newClient.stop();
    return;
  }

  int firstSpace = hello.indexOf(' ', 6);
  if (firstSpace <= 0) {
    newClient.stop();
    return;
  }

  String vestDeviceId = hello.substring(6, firstSpace);
  String name = hello.substring(firstSpace + 1);
  name.trim();
  registerVest(vestDeviceId, name, newClient);
}

/**
 * @brief Handles incoming messages from already connected vest clients.
 */
void handleExistingDeviceMessages() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
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
 * @brief Accepts new vest TCP connections and processes messages from connected vests.
 */
void handleDeviceConnections() {
  cleanupDisconnectedClients();

  if (gameMode == "Free For All" && ffaTimerRunning && getRemainingFFATime() <= 0) {
    stopFFATimer();
    isRoundOver();
  }

  handleNewDeviceConnection();
  handleExistingDeviceMessages();
}

/**
 * @brief Processes a browser request.
 * @param req HTTP request string to inspect for actions.
 */
void handleBrowserAction(String req) {
  if (req.indexOf("GET /mode/ffa") >= 0) {
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
  } else if (req.indexOf("GET /game/start") >= 0) {
    if (gameMode == "Free For All") {
      startFFATimer();
    } else if (gameMode == "Duels") {
      startDuelsGame();
    }
  } else if (req.indexOf("GET /game/stop") >= 0) {
    if (gameMode == "Free For All") {
      stopFFATimer();
    } else if (gameMode == "Duels") {
      stopDuelsGame();
    }
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
  if (!roundOver) {
    client.println("<meta http-equiv=\"refresh\" content=\"5\">");
  }

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
  client.println(".timer-btn { display: inline-block; margin: 6px; padding: 10px 14px; border-radius: 8px; text-decoration: none; color: white; background: #2f80ed; font-size: 0.95rem; }");
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

  if (roundOver) {
    client.println("<div class=\"card\" style=\"background:#27ae60; color:white;\">");
    client.println("<h2>Round Over</h2>");
    client.println("<div class=\"timer-value\">" + roundOverMessage + "</div>");
    client.println("<div class=\"hint\">Start a new round when ready.</div>");
    client.println("</div>");
  }

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

  client.println("<div class=\"card\">");

  if (gameMode == "Free For All") {
    client.println("<h2>Free For All Timer</h2>");
    client.println("<div style=\"display:flex; align-items:center; justify-content:center; gap:12px; margin-bottom:12px;\">");
    client.println("<a class=\"time-btn\" href=\"/time/sub15\">- 15 sec</a>");
    client.println("<div class=\"timer-value\" style=\"margin-bottom:0; min-width:90px;\">" + formatTime(getRemainingFFATime()) + "</div>");
    client.println("<a class=\"time-btn\" href=\"/time/add15\">+ 15 sec</a>");
    client.println("</div>");
  } else {
    client.println("<h2>Duels Controls</h2>");
    client.println("<div class=\"timer-value\" style=\"margin-bottom:0; min-width:90px;\">");
    client.println(duelsGameRunning ? "Live" : "Waiting");
    client.println("</div>");
  }

  client.println("<div>");
  client.println("<a class=\"timer-btn\" href=\"/game/start\">Start Game</a>");
  client.println("<a class=\"timer-btn\" href=\"/game/stop\">Stop Game</a>");
  if (gameMode == "Free For All") {
    client.println("<a class=\"timer-btn\" href=\"/timer/reset\">Reset Timer</a>");
  }
  client.println("</div>");

  if (statusMessage.length() > 0) {
    client.println("<div class=\"status\">" + statusMessage + "</div>");
  }

  client.println("</div>");

  client.println("<div class=\"card\">");
  client.println("<h2>Scoreboard</h2>");

  if (rankedCount == 0) {
    client.println("<div class=\"player\">");
    client.println("<div class=\"player-name\">No players yet</div>");
    client.println("<div class=\"player-stat\">Connect a vest to appear on the scoreboard.</div>");
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
 * @brief Returns whether a browser request should be redirected after action processing.
 * @param requestLine First line of the HTTP request.
 * @return true if the request mutates state and should redirect; otherwise false.
 */
bool isActionRequest(String requestLine) {
  return
    requestLine.indexOf("GET /mode/ffa") >= 0 ||
    requestLine.indexOf("GET /mode/duels") >= 0 ||
    requestLine.indexOf("GET /remove?slot=") >= 0 ||
    requestLine.indexOf("GET /clear") >= 0 ||
    requestLine.indexOf("GET /time/add15") >= 0 ||
    requestLine.indexOf("GET /time/sub15") >= 0 ||
    requestLine.indexOf("GET /game/start") >= 0 ||
    requestLine.indexOf("GET /game/stop") >= 0 ||
    requestLine.indexOf("GET /timer/reset") >= 0;
}

/**
 * @brief Accepts browser connections, parses requests, and serves html page.
 */
void handleWebClients() {
  WiFiClient client = webServer.available();
  if (!client) return;

  String currentLine = "";
  String requestLine = "";
  header = "";

  while (client.connected()) {
    if (client.available()) {
      char clientChar = client.read();
      header += clientChar;

      if (clientChar == '\n') {
        if (requestLine.length() == 0 && currentLine.length() > 0) {
          requestLine = currentLine;
        }

        if (currentLine.length() == 0) {
          Serial.print("Stations: ");
          Serial.print(WiFi.softAPgetStationNum());
          Serial.println(" | New Client");

          bool action = isActionRequest(requestLine);
          handleBrowserAction(header);

          if (action) {
            redirectToHome(client);
          } else {
            serveWebPage(client);
          }
          break;
        } else {
          currentLine = "";
        }
      } else if (clientChar != '\r') {
        currentLine += clientChar;
      }
    }
  }

  header = "";
  client.stop();
}