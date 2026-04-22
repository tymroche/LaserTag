#ifndef PLAYER_COMMS_H
#define PLAYER_COMMS_H

#include <Arduino.h>


/** PIN + CONFIG MACROS */
#define TRIGGER_SWITCH          11                               /** Trigger SPST Input (triggers interrupt) */
#define EMITTER_OUT             10                               /** PWM Output to Emitter */
#define RECEIVER_IN             8                               /** IR Receiver Input */
#define RECEIVER_IN_2           18                               /** IR Receiver Input 2*/
#define SPEAKER_OUT             2                              /** PWM Output to Speaker */
#define EMITTER_FREQ            38000                           /** Frequency of PWM Output */
#define PWM_RESOLUTION          8                               /** Resolution for PWM Duty Cycle */
#define DUTY_CYCLE_50           ((1 <<  PWM_RESOLUTION) / 2)    /** 50% Duty Cycle based on Resolution */
#define DUTY_CYCLE_0            0                               /** 0% Duty Cycle regardless of Resolution */
#define BIT_TRANSMIT_TIME_US    562                             /** Time that 1 bit is transmitted for.  */   
#define BIT_RECEIVE_TIME_US     750                             /** Time that 1 bit is detected as received. */


/** ENUM DECLARATIONS  */
/** Status Enum. */
typedef enum {
  INVALID_INPUT,
  BAD_WRITE,
  START_ERROR,
  STOP_ERROR,
  PARITY_ERROR,
  STATUS_OK
} status_t;


/** SETUP FUNCTIONS */
/**
 * @brief Helper function that initializes Trigger Pin & Attaches ISR
 * @note Used in setup
 */
void trigger_init();

/**
 * @brief Helper function that initializes Emitter Pin & Attaches PWM
 * @note Used in setup
 */
void emitter_init();

/**
* @brief Helper function that initializes Receiver Pin & Attaches ISR
* @note Used in setup
*/
void receiver_init();

/**
 * @brief Helper function that turns string id into bits
 * @return bit version of id
 */
uint8_t playerIdToByte(const String& id);

/**
 * @brief Helper function that turns bits back into string id
 * @return string version of id
 */
String byteToPlayerId(uint8_t idvalue);

/** GENERAL PURPOSE FUNCTIONS */
/** 
* @brief Transmits data over IR emitter. 
* @param data Data to be written.
* @return INVALID_INPUT if Data is Null, BAD_WRITE if write failed, STATUS_OK if write successful.
* @note Package format: 2 Start Bits (First Low, Second High), 8 Data bits (LSB First), 1 Parity Bit [Even], 1 Stop Bit (High).
* @note Bit period of 21 cycles. 21 Cycles @ 38kHz will transmit 1 bit. Baud: 1809 bits / sec.
*/
status_t emitter_write(uint8_t data);

/** 
* @brief Receives transmitted receiveData. 
* @param buffer Buffer to store read receiveData in.
* @return INVALID_INPUT if receiveData is Null, PARITY_ERROR if parity does not match, STATUS_OK if read successful.
* @note Receiver inverts all signals when demodulated.
* @note Interrupt triggered via polled flag.
* @note Reads in the middle of bit periods.
*/
status_t receiverRead(volatile uint8_t* buffer, uint8_t pin);

/**
 * @brief initializes speaker
 */
void speaker_init();

/**
 * @brief this function beeps for a frequency and time
 * @param frequency of beep
 * @param duration of beep
 */
void beep(uint16_t frequency, uint16_t duration);

/**
 * @brief this function plays beeps when connected
 */
void playConnectedBeeps();

/**
 * @brief this function plays beeps when disconnected
 */
void playDisconnectedBeeps();

/**
 * @brief this function plays beeps when firing
 */
void playFireBeeps();

/**
 * @brief this function plays beeps when getting hit
 */
void playGotHitBeeps();

/**
 * @brief this function plays beeps when at low health
 */
void playLowHealthBeeps();

/**
 * @brief this function plays beeps when dead
 */
void playDeadBeeps();

/**
 * @brief this function plays beeps when you win
 */
void playWinningBeeps();

/**
 * @brief this function plays beeps when you lose
 */
void playLosingBeeps();


/** PRINT FUNCTIONS */
/** 
* @brief Helper function to print based on status.
* @param status Status to print.
*/
void printStatus(status_t status);

/** GLOBAL FLAGS */
extern volatile bool rxFlag;
extern volatile bool rxFlag2;
extern uint8_t transmitData;
extern uint8_t receiveData;
extern volatile bool triggerFlag;
#endif
