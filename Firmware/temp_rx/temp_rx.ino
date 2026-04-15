#define RECEIVER_IN             6                               /** IR Receiver Input */
#define PWM_RESOLUTION          8                               /** Resolution for PWM Duty Cycle */
#define DUTY_CYCLE_50           ((1 <<  PWM_RESOLUTION) / 2)    /** 50% Duty Cycle based on Resolution */
#define DUTY_CYCLE_0            0                               /** 0% Duty Cycle regardless of Resolution */
#define BIT_TRANSMIT_TIME_US    562                             /** Time that 1 bit is transmitted for.  */   


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
  START_ERROR,
  PARITY_ERROR,
  STATUS_OK
} status_t;

// ~~~ GLOBAL VARIABLES ~~~

// Trigger Debouncing 
unsigned long lastTrigger = 0;

// ISR Variables
volatile bool rxFlag = false;


// ~~~ Temp Variables for Testing ~~~
volatile uint8_t data = 0x0;
volatile uint8_t* dataPtr = &data;


// Forward Declarations
status_t emitter_read(volatile uint8_t* buffer);
void printStatus(status_t status);



void IRAM_ATTR receiver_isr() {
  rxFlag = true;
}


// TO-DO: Rename This
status_t emitter_read(volatile uint8_t* buffer) {
  if (buffer == NULL) {
    return INVALID_INPUT;
  }

  uint8_t numHighBits = 0;

    // Empty Buffer
    *buffer = 0;

    // Offset to read bits in middle
    receiveTimeOffset();

    // Read Bits & Calculate Parity
    for (int i = 0; i < 8; i++) {
      receiveDelay();
      uint8_t currVal = !digitalRead(RECEIVER_IN);
      *buffer |=  (currVal << i);
      if (currVal) {
        numHighBits ++;
      }
    }

  // Parity Bit 
    receiveDelay();    
    if (!digitalRead(RECEIVER_IN) != (numHighBits % 2)) { // compare parity
      return PARITY_ERROR;
    }

    // Stop bit (also check stop bit)
    receiveDelay();

  return STATUS_OK;
}

void setup() {
  // Initializes Serial Debugging
  Serial.begin(115200);
  Serial.println("Serial Initialized!");

  // Initialize Receiver
  pinMode(RECEIVER_IN, INPUT);
  attachInterrupt(RECEIVER_IN, receiver_isr, FALLING);


  // Test GPIO Pin
  pinMode(7, OUTPUT);
  digitalWrite(7, 0);
}

void loop() {
  if (rxFlag) {
    printStatus(emitter_read(&data));
    Serial.printf("Received Data: %d\n", *dataPtr);
    Serial.flush();
    rxFlag = 0;
  }
}

void printStatus(status_t status) {
  switch (status) {
    case INVALID_INPUT: Serial.println("Status: INVALID_INPUT"); break;
    case BAD_WRITE:     Serial.println("Status: BAD_WRITE");     break;
    case START_ERROR:   Serial.println("Status: START_ERROR");   break;
    case PARITY_ERROR:  Serial.println("Status: PARITY_ERROR");  break;
    case STATUS_OK:     Serial.println("Status: OK");            break;
    default:            Serial.println("Status: UNKNOWN");       break;
  }
}
