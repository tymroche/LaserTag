#define TRIGGER_SWITCH          2                               /** Trigger SPST Input (triggers interrupt) */
#define EMITTER_OUT             3                               /** PWM Output to Emitter */
#define EMITTER_FREQ            38000                           /** Frequency of PWM Output */
#define PWM_RESOLUTION          8                               /** Resolution for PWM Duty Cycle */
#define DUTY_CYCLE_50           ((1 <<  PWM_RESOLUTION) / 2)    /** 50% Duty Cycle based on Resolution */
#define DUTY_CYCLE_0            0                               /** 0% Duty Cycle regardless of Resolution */
#define BIT_TRANSMIT_TIME_US    562                             /** Time that 1 bit is transmitted for.  */   

/** MACRO FUNCTIONS  */
/** 
* @brief Wrapper function for ledcWrite used.
* @param duty Duty cycle to set PWM to. 
*/
inline void pwm_write(uint8_t duty) {
  ledcWrite(EMITTER_OUT, duty);
}

/** 
* @brief Wrapper functin for delayMicroseconds. 
* @note Used for Bit Transmission. 
*/
inline void transmitDelay() {
  delayMicroseconds(BIT_TRANSMIT_TIME_US);
}


/** ENUM DECLARATIONS  */
/** Status Enum. */
typedef enum {
  INVALID_INPUT,
  BAD_WRITE,
  START_ERROR,
  PARITY_ERROR,
  STATUS_OK
} status_t;


/** GLOBAL VARIABLES  */
// Trigger Debouncing 
unsigned long lastTrigger = 0;
// Temp Variables (Used for Testing)
volatile int numTrigger = 0;
uint8_t transmitData = 80;  // Max 255


/** FORWARD DECLARATIONS  */
status_t emitter_write(uint8_t data);


/** INTERRUPT SERVICE ROUTINES (ISRs) */
/**
* @brief Interrupt Handler for trigger button. 
* @note Includes debounce logic. 
*/
void IRAM_ATTR trigger_isr() {
  if ((millis() - lastTrigger) < 200) return;
  lastTrigger = millis();
  emitter_write(transmitData);
  numTrigger++;
}


/** GENERAL PURPOSE FUNCTIONS */
/** 
* @brief Transmits data over IR emitter. 
* @param data Data to be written.
* @return INVALID_INPUT if Data is Null, BAD_WRITE if write failed, STATUS_OK if write successful.
* @note Package format: 2 Start Bits (First Low, Second High), 8 Data bits (LSB First), 1 Parity Bit [Even], 1 Stop Bit (High).
* @note Bit period of 21 cycles. 21 Cycles @ 38kHz will transmit 1 bit. Baud: 1809 bits / sec.
*/
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


/** SETUP FUNCTIONS */
/**
 * @brief Helper function that initializes Trigger Pin & Attaches ISR
 * @note Used in setup
 */
void trigger_init() {
  pinMode(TRIGGER_SWITCH, INPUT_PULLDOWN);
  attachInterrupt(TRIGGER_SWITCH, trigger_isr, RISING);
}

/**
 * @brief Helper function that initializes Emitter Pin & Attaches PWM
 * @note Used in setup
 */
void emitter_init() {
  pinMode(EMITTER_OUT, OUTPUT);
  ledcAttach(EMITTER_OUT, EMITTER_FREQ, PWM_RESOLUTION);
}

/** ARDUINO FUNCTIONS */
void setup() {
  // Initializes Serial Debugging
  Serial.begin(115200);
  Serial.println("Serial Initialized!");

  // Initialize Trigger
  trigger_init();

  // Initialize Emitter
  emitter_init();
}

void loop() { }
