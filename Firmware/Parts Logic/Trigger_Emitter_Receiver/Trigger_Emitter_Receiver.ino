#define TRIGGER_SWITCH          2                               /** Trigger SPST Input (triggers interrupt) */
#define EMITTER_OUT             3                               /** PWM Output to Emitter */
#define RECEIVER_IN             6                               /** IR Receiver Input */
#define EMITTER_FREQ            38000                           /** Frequency of PWM Output */
#define PWM_RESOLUTION          8                               /** Resolution for PWM Duty Cycle */
#define DUTY_CYCLE_50           ((1 <<  PWM_RESOLUTION) / 2)    /** 50% Duty Cycle based on Resolution */
#define DUTY_CYCLE_0            0                               /** 0% Duty Cycle regardless of Resolution */
#define BIT_TRANSMIT_TIME_US    562                             /** Time that 1 bit is transmitted for.  */   

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

/** 
* @brief Wrapper function for delayMicroseconds.
* @note Used for Bit Reception. 
*/
inline void receiveDelay() {
  delayMicroseconds(BIT_TRANSMIT_TIME_US);
}

/** 
* @brief Wrapper function for delayMicroseconds. 
* @note Used for Reception Timing Offset to center readings. */
inline void receiveTimeOffset() {
  delayMicroseconds(BIT_TRANSMIT_TIME_US / 2);
}

/** Status Enum. */
typedef enum {
  INVALID_INPUT,
  BAD_WRITE,
  PARITY_ERROR,
  STATUS_OK
} status_t;

// ~~~ GLOBAL VARIABLES ~~~

// Trigger Debouncing 
unsigned long lastTrigger = 0;
uint8_t data = 0x0;


// ~~~ Temp Variables for Testing ~~~
int numTrigger = 0;
uint8_t transmitTest = 0b01000000;


// Forward Declarations
status_t emitter_write(uint8_t data);
status_t emitter_read(uint8_t* buffer);

/**
* @brief Interrupt Handler for trigger button. 
* @note Includes debounce logic. 
*/
void IRAM_ATTR trigger_isr() {
  if ((millis() - lastTrigger) < 200) return;
  lastTrigger = millis();
  emitter_write(transmitTest);
  // numTrigger++;
}

void IRAM_ATTR receiver_isr() {
  digitalWrite(7, 1);
  emitter_read(&data);
}

/** 
* @brief Transmits data over IR emitter. 
* @param data Data to be written.
* @return INVALID_INPUT if Data is Null, BAD_WRITE if write failed, STATUS_OK if write successful.
* @note Package format: 2 Start Bits (First Low, Second High), 8 Data bits (LSB First), 1 Parity Bit [Even], 1 Stop Bit (High).
* @note Bit period of 21 cycles. 21 Cycles @ 38kHz will transmit 1 bit. Baud: 1809 bits / sec.
*/
status_t emitter_write(uint8_t data) {

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

  // Caluclate & Transmit Parity Bit
  if (numHighBits % 2) {
    pwm_write(DUTY_CYCLE_50);
  } else {
    pwm_write(DUTY_CYCLE_0);
  }
  transmitDelay();

  // Transmit stop bit (1 High)
  pwm_write(DUTY_CYCLE_50);
  transmitDelay();

  // Idle Low
  pwm_write(DUTY_CYCLE_0);

  return STATUS_OK;
}

status_t emitter_read(uint8_t* buffer) {
  if (buffer == NULL) {
    return INVALID_INPUT;
  }
  *buffer = 0;
  uint8_t numHighBits = 0;

  // Start Bits
  receiveDelay();
  receiveDelay();

  // Offset to center readings
  receiveTimeOffset();

  // Data Bits
  for (uint8_t i = 0; i < 8; i++) {
    uint8_t val = !digitalRead(RECEIVER_IN);
    *buffer |= (val << i);
    if (val) {
      numHighBits++;
    }
    receiveDelay();
  }


  // Parity Bit
  uint8_t parityActual = !digitalRead(RECEIVER_IN);
  uint8_t parityGoal = numHighBits % 2;

  if (parityActual != parityGoal) {
    return PARITY_ERROR;
  }
  receiveDelay();

  // Stop Bit
  receiveDelay();



// detect & record bits bits 
// , detect parity bit and stop pit
// calculate parity and return accordingly

  return STATUS_OK;
}

void setup() {
  // Initializes Serial Debugging
  Serial.begin(115200);
  Serial.println("Serial Initialized!");

  // Attaches trigger interrupt
  pinMode(TRIGGER_SWITCH, INPUT_PULLDOWN);
  attachInterrupt(TRIGGER_SWITCH, trigger_isr, RISING);

  // Initialize Receiver
  pinMode(RECEIVER_IN, INPUT_PULLDOWN);
  attachInterrupt(RECEIVER_IN, receiver_isr, FALLING);

  // Attach PWM to Emitter Output
  pinMode(EMITTER_OUT, OUTPUT);
  ledcAttach(EMITTER_OUT, EMITTER_FREQ, PWM_RESOLUTION);



  // Test GPIO Pin
  pinMode(7, OUTPUT);
  digitalWrite(7, 0);
}

void loop() {
  Serial.println(data);
}
