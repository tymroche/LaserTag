#include <WiFi.h>

/*STILL UNDER CONSTRUCTION

  MAY NEED ESP-NOW OR BLUTOOTH IF WE FOR SURE 100% CANNOT ENCODE THE THINGS*/

/*SET UP*/
const char* ssid = "ESP32S3-Soft-AP";
const char* password = "lasertag";

WiFiServer webServer(80);
WiFiServer deviceServer(3333);

String header;

const int MAX_PLAYERS = 4;
const int STARTING_HP = 5;
const int STARTING_FFA_POINTS = 0;
const int FFA_HIT_GAIN = 100;
const int FFA_HIT_LOSS = 50;
const int DEFAULT_FFA_SECONDS = 90;

String gameMode = "Free For All";
String statusMessage = "";

struct Player {
  String name;
  String vestDeviceId;
  String playerId;
  int hp;
  int score;
  bool active;
  bool alive;
  bool canShoot;
};

Player players[MAX_PLAYERS];
WiFiClient deviceClients[MAX_PLAYERS];

int rankedPlayers[MAX_PLAYERS];
int rankedCount = 0;

int ffaDurationSeconds = DEFAULT_FFA_SECONDS;
bool ffaTimerRunning = false;
unsigned long ffaStartMillis = 0;





/*
  Decode a few common URL-encoded characters from browser requests.
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

/*
  Extract a query parameter from a GET request.
*/
String getQueryValue(String req, String key) {
  int start = req.indexOf(key + "=");
  if (start < 0) return "";

  start += key.length() + 1;

  int end = req.indexOf("&", start);
  int httpEnd = req.indexOf(" HTTP/1.1", start);

  if (end < 0 || (httpEnd >= 0 && httpEnd < end)) {
    end = httpEnd;
  }
  if (end < 0) return "";

  return decodeUrl(req.substring(start, end));
}







/*
  Convert a player slot number to the assigned IR/player ID.
  slot 0 -> x01
  slot 1 -> x02
  slot 2 -> x03
  slot 3 -> x04
*/
String slotToPlayerId(int slot) {
  if (slot < 0 || slot >= MAX_PLAYERS) return "";
  if (slot < 9) return "x0" + String(slot + 1);
  return "x" + String(slot + 1);
}

/*
  Find a player slot by displayed name.
*/
int findPlayerByName(String name) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active && players[i].name == name) {
      return i;
    }
  }
  return -1;
}

/*
  Find a player slot by physical vest device ID.
*/
int findPlayerByVestDeviceId(String vestDeviceId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active && players[i].vestDeviceId == vestDeviceId) {
      return i;
    }
  }
  return -1;
}

/*
  Find a player slot by assigned game/player ID like x01.
*/
int findPlayerByAssignedPlayerId(String playerId) {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active && players[i].playerId == playerId) {
      return i;
    }
  }
  return -1;
}

/*
  Find first free player slot.
*/
int findOpenSlot() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (!players[i].active) return i;
  }
  return -1;
}

/*
  Reset one player's stats for the current mode.
*/
void resetPlayerForCurrentMode(int index) {
  players[index].hp = STARTING_HP;
  players[index].score = STARTING_FFA_POINTS;
  players[index].alive = true;
  players[index].canShoot = true;
}

/*
  Reset all players and sockets.
*/
void initializePlayers() {
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

/*
  Return remaining FFA time in seconds.
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

/*
  Format seconds as M:SS.
*/
String formatTime(int totalSeconds) {
  int minutes = totalSeconds / 60;
  int seconds = totalSeconds % 60;

  String out = String(minutes) + ":";
  if (seconds < 10) out += "0";
  out += String(seconds);
  return out;
}











/*
  Send the current host state to one vest.
  Includes the assigned PLAYERID that the vest should use in IR transmit.
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




/*
  Send state to all connected vests.
*/
void sendStateToAllPlayers() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    sendStateToPlayer(i);
  }
}



/*
  Add a browser-only player.
  Browser-only players still get assigned x01-x04 based on slot.
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




/*
  Remove one player.
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





/*
  Remove all players.
*/
void removeAllPlayers() {
  for (int i = 0; i < MAX_PLAYERS; i++) {
    removePlayer(i);
  }
  statusMessage = "All players removed.";
}




/*
  Ranking comparison.
*/
bool ranksHigher(int a, int b) {
  if (gameMode == "Duels") {
    if (players[a].hp != players[b].hp) return players[a].hp > players[b].hp;
  } else {
    if (players[a].score != players[b].score) return players[a].score > players[b].score;
  }

  return players[a].name < players[b].name;
}



/*
  Build sorted player list for scoreboard.
*/
void buildRankings() {
  rankedCount = 0;

  for (int i = 0; i < MAX_PLAYERS; i++) {
    if (players[i].active) {
      rankedPlayers[rankedCount++] = i;
    }
  }

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
  }
}




/*
  Disable shooting for all active players.
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
void startFFATimer() {
  ffaTimerRunning = true;
  ffaStartMillis = millis();
  statusMessage = "Free For All timer started.";
  sendStateToAllPlayers();
}

void stopFFATimer() {
  ffaTimerRunning = false;
  statusMessage = "Free For All timer stopped.";
  sendStateToAllPlayers();
}

void resetFFATimer() {
  ffaTimerRunning = false;
  statusMessage = "Free For All timer reset.";
  sendStateToAllPlayers();
}






/*
  Change mode and reset all active players for that mode.
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



/*
  Register or reconnect a vest.
  Host assigns playerId based on slot and sends it back in STATE.
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

  targetIndex comes from the socket that sent the message.
  attackerId comes from the IR-decoded player ID in the message.
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
  Accept new vest connections and process incoming vest messages.
    HELLO <vestDeviceId> <playerName>
    HIT <attackerPlayerId>
*/
void handleDeviceConnections() {
  WiFiClient newClient = deviceServer.available();

  if (newClient) {
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

    if (hello.startsWith("HELLO ")) {
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





/*
  Handle browser actions.
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
  } else if (req.indexOf("GET /time?seconds=") >= 0) {
    int secs = getQueryValue(req, "seconds").toInt();
    if (secs > 0) {
      ffaDurationSeconds = secs;
      ffaTimerRunning = false;
      statusMessage = "Free For All time set to " + formatTime(ffaDurationSeconds) + ".";
      sendStateToAllPlayers();
    }
  } else if (req.indexOf("GET /timer/start") >= 0) {
    if (gameMode == "Free For All") startFFATimer();
  } else if (req.indexOf("GET /timer/stop") >= 0) {
    stopFFATimer();
  } else if (req.indexOf("GET /timer/reset") >= 0) {
    resetFFATimer();
  }
}






/*
  HTML, potential senior dev use to get it looking pretty
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
  client.println("<meta http-equiv=\"refresh\" content=\"2\">");
  client.println("<link rel=\"icon\" href=\"data:,\">");

  client.println("<style>");
  client.println("body { margin: 0; padding: 16px; font-family: Helvetica, Arial, sans-serif; background: #111; color: white; text-align: center; }");
  client.println(".container { max-width: 420px; margin: 0 auto; }");
  client.println(".card { background: #1e1e1e; border-radius: 14px; padding: 18px; margin-bottom: 16px; box-shadow: 0 2px 8px rgba(0,0,0,0.35); }");
  client.println("h1 { margin: 0; font-size: 2rem; }");
  client.println("h2 { margin-top: 0; margin-bottom: 12px; font-size: 1.3rem; }");
  client.println(".mode-card { touch-action: pan-y; }");
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

  client.println("<script>");
  client.println("let touchStartX = 0;");
  client.println("let touchEndX = 0;");
  client.println("function handleSwipe() {");
  client.println("  if (touchEndX < touchStartX - 40 || touchEndX > touchStartX + 40) {");
  client.println("    const currentMode = '" + gameMode + "';");
  client.println("    if (currentMode === 'Free For All') window.location.href='/mode/duels';");
  client.println("    else window.location.href='/mode/ffa';");
  client.println("  }");
  client.println("}");
  client.println("window.addEventListener('load', function() {");
  client.println("  const modeCard = document.getElementById('modeCard');");
  client.println("  if (modeCard) {");
  client.println("    modeCard.addEventListener('touchstart', function(e){ touchStartX = e.changedTouches[0].screenX; }, false);");
  client.println("    modeCard.addEventListener('touchend', function(e){ touchEndX = e.changedTouches[0].screenX; handleSwipe(); }, false);");
  client.println("  }");
  client.println("});");
  client.println("</script>");

  client.println("</head><body><div class=\"container\">");

  client.println("<div class=\"card\"><h1>Laser Tag</h1></div>");

  client.println("<div class=\"card mode-card\" id=\"modeCard\">");
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
  client.println("<div class=\"hint\">Tap a mode button or swipe left/right on this card.</div>");
  client.println("</div>");

  if (gameMode == "Free For All") {
    client.println("<div class=\"card\">");
    client.println("<h2>Free For All Timer</h2>");
    client.println("<div class=\"timer-value\">" + formatTime(getRemainingFFATime()) + "</div>");

    if (ffaDurationSeconds == 60) client.println("<a class=\"time-btn active\" href=\"/time?seconds=60\">1:00</a>");
    else client.println("<a class=\"time-btn\" href=\"/time?seconds=60\">1:00</a>");

    if (ffaDurationSeconds == 90) client.println("<a class=\"time-btn active\" href=\"/time?seconds=90\">1:30</a>");
    else client.println("<a class=\"time-btn\" href=\"/time?seconds=90\">1:30</a>");

    if (ffaDurationSeconds == 120) client.println("<a class=\"time-btn active\" href=\"/time?seconds=120\">2:00</a>");
    else client.println("<a class=\"time-btn\" href=\"/time?seconds=120\">2:00</a>");

    if (ffaDurationSeconds == 180) client.println("<a class=\"time-btn active\" href=\"/time?seconds=180\">3:00</a>");
    else client.println("<a class=\"time-btn\" href=\"/time?seconds=180\">3:00</a>");

    client.println("<div>");
    client.println("<a class=\"timer-btn\" href=\"/timer/start\">Start</a>");
    client.println("<a class=\"timer-btn\" href=\"/timer/stop\">Stop</a>");
    client.println("<a class=\"timer-btn\" href=\"/timer/reset\">Reset</a>");
    client.println("</div>");
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

/*
  Handle browser connections.
*/
void handleWebClients() {
  WiFiClient client = webServer.available();
  if (!client) return;

  String currentLine = "";

  while (client.connected()) {
    if (client.available()) {
      char c = client.read();
      header += c;

      if (c == '\n') {
        if (currentLine.length() == 0) {
          Serial.print("Stations: ");
          Serial.print(WiFi.softAPgetStationNum());
          Serial.println(" | New Client");

          handleBrowserAction(header);
          serveWebPage(client);
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










void setup() {
  Serial.begin(115200);
  delay(1000);

  initializePlayers();

  Serial.println();
  Serial.println("Start AP test");

  WiFi.mode(WIFI_AP);
  delay(200);

  bool ok = WiFi.softAP(ssid, password, 6, 0, 8);
  Serial.print("softAP result: ");
  Serial.println(ok ? "SUCCESS" : "FAIL");

  WiFi.setTxPower(WIFI_POWER_15dBm);

  Serial.print("SSID: ");
  Serial.println(WiFi.softAPSSID());

  Serial.print("IP: ");
  Serial.println(WiFi.softAPIP());

  webServer.begin();
  deviceServer.begin();

  Serial.println("Web server started on port 80");
  Serial.println("Device server started on port 3333");
}

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