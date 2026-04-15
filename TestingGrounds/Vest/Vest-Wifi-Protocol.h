/**
 * @file Vest-Wifi-Protocol.h
 * @author Alfonso Landaverde, Tyler Roche
 * @date 4/15/2026
 * @version 1.0
 *
 * @brief Definition of Client (Vest, Players) Communication Protocols.
 *
 * This file defines all variables, structs, and functions necessary for Laser Tag Game.
 * 
 * @see config.h for the hardware definitions like GPIO mapping
 */

 #ifndef VEST_WIFI_PROTOCOL
 #define VEST_WIFI_PROTOCOL

 #include <WiFi.h>

extern const char* hostSsid;
extern const char* hostPassword;

extern IPAddress hostIP;
extern const uint16_t hostPort;

extern WiFiClient hostClient;

//unique to hardware
extern String vestDeviceId;
extern String playerName;

//unique to online session
extern String assignedPlayerId;

extern bool canShoot;
extern bool alive;
extern String currentMode;
extern int currentHp;
extern int currentScore;
extern int currentTimer;

extern unsigned long lastReconnectAttempt;

//test blinker
extern const int RED_LED_PIN;
extern bool redBlinkActive;
extern unsigned long redBlinkStart;
extern const unsigned long RED_BLINK_MS;

/**
 * @brief Connects vest to host SoftAP and opens persistent TCP connection to host device server.
 */
void connectVestToHost();

/**
 * @brief Starts a short blink on the red status LED to indicate a received host update.
 */
void triggerRedBlink();

/**
 * @brief Updates the timed red LED blink state and turns the LED off once the blink duration has elapsed.
 */
void updateRedBlink();

/*
  Parse one STATE line sent from the host.
  e.g, STATE PLAYERID=x01 MODE=Free For All ALIVE=1 CANSHOOT=1 HP=5 SCORE=0 TIMER=90
*/
/**
 * @brief Parses a STATE message received from host and updates the vest's game variables.
 * @param line Complete STATE line received from the host TCP connection.
 */
void parseHostStateLine(String line);

/**
 * @brief Read any pushed state updates from the host.
 */
void handleHostMessages();

/**
 * @brief Checks if vest is currently allowed to fire.
 * @return true if so, false otherwise.
 */
bool vestCanFire();

/*
  Send a hit report to the host.
  Eg
  HIT x02
  vest was hit by player x02.
*/

/**
 * @brief Sends a hit report to the host identifying which player ID hit this vest.
 * @param attackerPlayerId Assigned player ID of attacker, decoded from IR.
 */
void reportHitToHost(const String& attackerPlayerId);

/**
 * @brief Attempts to fire and prints player ID when firing is allowed.
 */
void fireShot();

 #endif