/**
 * @file Host-Server.h
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Definitions of Host Server and HTML.
 *
 * This file defines all variables, structs, and functions necessary for Laser Tag Game.
 * 
 * @see config.h for the hardware definitions like GPIO mapping
 */

#ifndef HOST_SERVER_H
#define HOST_SERVER_H

#include <WiFi.h>

extern String header;
extern String gameMode;
extern String statusMessage;

extern const int MAX_PLAYERS;
extern const int STARTING_HP;
extern const int STARTING_FFA_POINTS;
extern const int FFA_HIT_GAIN;
extern const int FFA_HIT_LOSS;
extern const int DEFAULT_FFA_SECONDS;

extern const char* ssid;
extern const char* password;

extern WiFiServer webServer;
extern WiFiServer deviceServer;

/**
 * @brief Player Struct used to keep track of identifying information and game-state variables.
 */
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

extern Player players[MAX_PLAYERS];
extern WiFiClient deviceClients[MAX_PLAYERS];

extern int rankedPlayers[MAX_PLAYERS];
extern int rankedCount;

extern int ffaDurationSeconds;
extern bool ffaTimerRunning;
extern unsigned long ffaStartMillis;

/**
 * @brief Decodes URL characters from browser request.
 * @param s URL-encoded string to decode.
 * @return Decoded, trimmed string.
 */
String decodeUrl(String s);

/**
 * @brief Extracts query parameter from HTTP GET request.
 * @param req HTTP request string.
 * @param key Query parameter to extract.
 * @return Decoded value of the requested parameter.
 */
String getQueryValue(String req, String key);

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
String slotToPlayerId(int slot);

/**
 * @brief Find a player slot by displayed name.
 * @param name Player name to search for.
 * @return Matching player slot index or -1.
 */
int findPlayerByName(String name);

/**
 * @brief Find a player slot by vest ID.
 * @param vestDeviceId Vest ID.
 * @return Matching player slot or -1.
 */
int findPlayerByVestDeviceId(String vestDeviceId);

/**
 * @brief Find a player slot by assigned game player ID.
 * @param playerId Assigned player ID.
 * @return Matching player slot index or -1.
 */
int findPlayerByAssignedPlayerId(String playerId);

/**
 * @brief Finds the first unused player slot.
 * @return Index of the first free player slot, or -1 if the lobby is full.
 */
int findOpenSlot();

/**
 * @brief Resets one player's stats for the current mode.
 * @param index Player slot index to reset.
 */
void resetPlayerForCurrentMode(int index);

/**
 * @brief Resets all player records and closes any client connections.
 */
void resetAllPlayers();

/*UNDER CONSTRUCTION, WOULD LIKE TO HAVE TIMER ACTIVELY DISPLAY TIME*/

/**
 * @brief Computes the time remaining for Free For All timer.
 * @return Remaining timer value in seconds, or zero once the timer has expired.
 */
int getRemainingFFATime();

/**
 * @brief Formats seconds as minutes:seconds.
 * @param totalSeconds Total seconds.
 * @return Formatted string in M:SS form.
 */
String formatTime(int totalSeconds);

/**
 * @brief Send the current host state to one vest.
 * Includes the assigned PLAYERID that the vest should use in IR transmit.
 * @param index Player slot whose vest should receive the state update.
 */
void sendStateToPlayer(int index);

/**
 * @brief Sends game state to every player slot.
 */
void sendStateToAllPlayers();


/*
  TEST FUNCTION FOR PLAYER SLOT VALIDITY; REMOVE WHEN DONE HERE AND IN HTML UI
*/
/**
 * @brief Adds a browser-only player to the lobby if a name is valid and a slot is available.
 * @param name Display name to assign to the new browser-only player.
 */
void addBrowserPlayer(String name);

/**
 * @brief Removes one player from the lobby and disconnects any linked vest client.
 * @param index Player slot index to remove.
 */
void removePlayer(int index);

/**
 * @brief Removes all player from lobby.
 */
void removeAllPlayers();

/**
 * @brief Compares two active player slots to determine scoreboard ordering.
 * @param a First player slot index to compare.
 * @param b Second player slot index to compare.
 * @return true if player a should rank above player b; otherwise false.
 */
bool ranksHigher(int a, int b);

/**
 * @brief Builds sorted player list used by scoreboard.
 */
void buildRankings();

/**
 * @brief Disables shooting for all active players and sends updated state.
 */
void disableAllShooting();

/*
  Start, stop, reset FFA timer.
*/

/**
 * @brief Starts Free For All timer and broadcasts updated state.
 */
void startFFATimer();

/**
 * @brief Stops Free For All timer and broadcasts updated state.
 */
void stopFFATimer();

/**
 * @brief Resets the Free For All timer state and broadcasts updated state.
 */
void resetFFATimer();

/**
 * @brief Changes current game mode and resets all active players.
 * @param newMode New game mode string to apply.
 */
void setGameMode(String newMode);

/**
 * @brief Registers a newly connected vest or reconnects an existing vest to a player slot.
 * @param vestDeviceId Unique device identifier reported by the vest.
 * @param name Player name associated with the vest.
 * @param client Connected TCP client for that vest.
 */
void registerVest(String vestDeviceId, String name, WiFiClient client);

/*
  Process one hit report.
  e.g. HIT x02
*/

/**
 * @brief Processes a hit report sent by a vest and applies the appropriate scoring or HP changes.
 * @param targetIndex Player slot index of the vest that reported being hit.
 * @param attackerId Assigned player ID of the attacker IR.
 */
void processHitReport(int targetIndex, String attackerId);

/*
  Accept new vest connections and process vest messages.
    HELLO <vestDeviceId> <playerName>
    HIT <attackerPlayerId>
*/

/**
 * @brief Accepts new vest TCP connections and processes messages from connected vests.
 */
void handleDeviceConnections();

/**
 * @brief Processes a browser request.
 * @param req HTTP request string to inspect for actions.
 */
void handleBrowserAction(String req);

/**
 * @brief Sends an HTTP redirect response back to the browser homepage.
 * clears url as well to prevent duplicate actions.
 * @param client Connected web client to respond to.
 */
void redirectToHome(WiFiClient& client);

/**
 * @brief Generates and sends the current control page and scoreboard HTML to a browser client.
 * @param client Connected web client to respond to.
 */
void serveWebPage(WiFiClient& client);

/**
 * @brief Accepts browser connections, parses requests, and serves html page.
 */
void handleWebClients();

#endif