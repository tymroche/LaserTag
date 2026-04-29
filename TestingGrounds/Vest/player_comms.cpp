#include "player_comms.h"
#include "Vest-Wifi-Protocol.h"

/** MACRO FUNCTIONS  */
/** 
* @brief Wrapper function for ledcWrite used.
* @param duty Duty cycle to set PWM to. 
*/
static inline void pwm_write(uint8_t duty) {
  ledcWrite(EMITTER_OUT, duty);
}

/** 
* @brief Wrapper functin for delayMicroseconds. 
* @note Used for Bit Transmission. 
*/
static inline void transmitDelay() {
  delayMicroseconds(BIT_TRANSMIT_TIME_US);
}

/** 
* @brief Wrapper function for delayMicroseconds.
* @note Used for Bit Reception. 
*/
static inline void receiveDelay() {
  delayMicroseconds(BIT_RECEIVE_TIME_US);
}

/** 
* @brief Wrapper function for delayMicroseconds. 
* @note Used for Reception Timing Offset to center readings. */
static inline void receiveTimeOffset() {
  delayMicroseconds(BIT_RECEIVE_TIME_US / 2);
}


/** GLOBAL VARIABLES  */
// Trigger Debouncing 
unsigned long lastTrigger = 0;
// Data Transmission
volatile bool rxFlag = false;
volatile bool rxFlag2 = false;
uint8_t receiveData = 0;
uint8_t transmitData = 0;
volatile bool triggerFlag = false;


/** INTERRUPT SERVICE ROUTINES (ISRs) */
/**
* @brief Interrupt Handler for trigger button. 
* @note Includes debounce logic. 
*/
void IRAM_ATTR trigger_isr() {
  unsigned long now = millis();
  if ((now - lastTrigger) < 200) return;
  lastTrigger = now;
  triggerFlag = true;
}

/**
* @brief Interrupt Handler for receiver signal. 
*/
void IRAM_ATTR receiver_isr() {
  rxFlag = true;
}

/**
* @brief Interrupt Handler for receiver signal. 
*/
void IRAM_ATTR receiver_isr_2() {
  rxFlag2 = true;
}

/** GENERAL PURPOSE FUNCTIONS */
status_t emitter_write(uint8_t data) {

  // Initialize Local Variables
  uint8_t numHighBits = 0;

  // Transmit start bits (1 Low, 1 High)
  pwm_write(DUTY_CYCLE_0);
  transmitDelay();

  pwm_write(DUTY_CYCLE_50);
  transmitDelay();

  // Transmit data bits (8 Total) LSB First
  for (uint8_t i = 0; i < 8; i++) {
    if (data & 0x1) {
      pwm_write(DUTY_CYCLE_50);
      numHighBits++;
    } else {
      pwm_write(DUTY_CYCLE_0);
    }
    data >>= 0x1;
    transmitDelay();
  }

  // Calculate & Transmit Parity Bit
  pwm_write((numHighBits % 2) ? DUTY_CYCLE_50 : DUTY_CYCLE_0);
  transmitDelay();

  // Transmit stop bit (1 High)
  pwm_write(DUTY_CYCLE_50);
  transmitDelay();

  // Idle Low
  pwm_write(DUTY_CYCLE_0);

  return STATUS_OK;
}

status_t receiverRead(volatile uint8_t* buffer, uint8_t pin) {
  if (buffer == NULL) {
    return INVALID_INPUT;
  }

  // Initialize Local Variables
  uint8_t numHighBits = 0;
  *buffer = 0;

  // Centers read to middle of bit period
  receiveTimeOffset();

  // Reads bits & Stores in buffer
  for (int i = 0; i < 8; i++) {
    receiveDelay();
    uint8_t currVal = !digitalRead(pin);
    *buffer |=  (currVal << i);
    if (currVal) {
      numHighBits ++;
    }
  }

  // Compares Parity Bit
    receiveDelay();    
    if (!digitalRead(pin) != (numHighBits % 2)) { // compare parity
      return PARITY_ERROR;
    }

  // Checks Stop Bit
  receiveDelay();
  if (digitalRead(pin)) {
    return STOP_ERROR;
  }

  return STATUS_OK;
}


/** SETUP FUNCTIONS */
void trigger_init() {
  pinMode(TRIGGER_SWITCH, INPUT_PULLDOWN);
  attachInterrupt(TRIGGER_SWITCH, trigger_isr, RISING);
}

void emitter_init() {
  pinMode(EMITTER_OUT, OUTPUT);
  ledcAttach(EMITTER_OUT, EMITTER_FREQ, PWM_RESOLUTION);
}

void receiver_init() {
  pinMode(RECEIVER_IN, INPUT);
  attachInterrupt(RECEIVER_IN, receiver_isr, FALLING);
  
  pinMode(RECEIVER_IN_2, INPUT);
  attachInterrupt(RECEIVER_IN_2, receiver_isr_2, FALLING);
}

uint8_t playerIdToByte(const String& id) {
  int idvalue = id.toInt();
  if (idvalue < 1 || idvalue > 255) return 0;
  return (uint8_t) idvalue;
}

String byteToPlayerId(uint8_t idvalue) {
  if (idvalue == 0) return "";
  return String (idvalue);
}

/**
 * @brief initializes speaker
 */
void speaker_init() {
  Serial.println("speaker_init(): attaching speaker PWM");
  ledcAttach(SPEAKER_OUT, 2000, PWM_RESOLUTION);
  ledcWrite(SPEAKER_OUT, 0);
  Serial.println("speaker_init(): ready");
}

/**
 * @brief this function beeps for a frequency and time
 * @param frequency of beep
 * @param duration of beep
 */
void beep(uint16_t frequency, uint16_t duration) {
  ledcWrite(SPEAKER_OUT, DUTY_CYCLE_50);
  ledcWriteTone(SPEAKER_OUT, frequency);
  delay(duration);
  ledcWrite(SPEAKER_OUT, 0);
}

/**
 * @brief this function plays beeps when connected
 */
void playConnectedBeeps() {
  beep(800, 100); 
  delay(50);
  beep(1100, 200);
}
/**
 * @brief this function plays beeps when disconnected
 */
void playDisconnectedBeeps() {
  beep(800, 100);
  delay(50);
  beep(400, 200);
}
/**
 * @brief this function plays beeps when firing
 */
void playFireBeeps() {
  beep(1500,50);
  beep(1400,50);
}
/**
 * @brief this function plays beeps when getting hit
 */
void playGotHitBeeps() {
  beep(1000, 100);
}
/**
 * @brief this function plays beeps when at low health
 */
void playLowHealthBeeps() {
  beep(700, 50);
  delay(50);
  beep(700, 50);
  delay(50);
  beep(700, 50);
}
/**
 * @brief this function plays beeps when dead
 */
void playDeadBeeps() {
  beep(200, 50);
  delay(50);
  beep(200, 50);
  delay(50);
  beep(200, 50);
}
/**
 * @brief this function plays beeps when you win
 */
void playWinningBeeps() {
  beep(1200, 70);
  delay(30);
  beep(800, 40);
  delay(20);
  beep(1400, 100);
}

/**
 * @brief this function plays beeps when you lose
 */
void playLosingBeeps() {
  beep(400, 50);
  delay(50);
  beep(600, 50);
}

/** PRINT FUNCTIONS */
void printStatus(status_t status) {
  switch (status) {
    case INVALID_INPUT: Serial.println("Status: INVALID_INPUT");  break;
    case BAD_WRITE:     Serial.println("Status: BAD_WRITE");      break;
    case START_ERROR:   Serial.println("Status: START_ERROR");    break;
    case PARITY_ERROR:  Serial.println("Status: PARITY_ERROR");   break;
    case STOP_ERROR:    Serial.println("Status: STOP_ERROR");     break;
    case STATUS_OK:     Serial.println("Status: OK");             break;
    default:            Serial.println("Status: UNKNOWN");        break;
  }
}