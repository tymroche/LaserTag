#define RECEIVER_IN             6                               /** IR Receiver Input */
#define BIT_TRANSMIT_TIME_US    562                             /** Time that 1 bit is transmitted for.  */   



/** MACRO FUNCTIONS  */
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



/** ENUM DECLARATIONS  */
/** Status Enum. See printStatus() for Serial Output. */
typedef enum {
  INVALID_INPUT,
  BAD_WRITE,
  START_ERROR,
  PARITY_ERROR,
  STOP_ERROR,
  STATUS_OK
} status_t;



/** GLOBAL VARIABLES  */
// ISR Variables
volatile bool rxFlag = false;
// Temp Variables (Used for Testing)
uint8_t receiveData = 0;



/** FORWARD DECLARATIONS  */
status_t receiverRead(volatile uint8_t* buffer);
void printStatus(status_t status);



/** INTERRUPT SERVICE ROUTINES (ISRs) */
/**
* @brief Interrupt Handler for receiver signal. 
*/
void IRAM_ATTR receiver_isr() {
  rxFlag = true;
}



/** GENERAL PURPOSE FUNCTIONS */
/** 
* @brief Receives transmitted receiveData. 
* @param buffer Buffer to store read receiveData in.
* @return INVALID_INPUT if receiveData is Null, PARITY_ERROR if parity does not match, STATUS_OK if read successful.
* @note Receiver inverts all signals when demodulated.
* @note Interrupt triggered via polled flag.
* @note Reads in the middle of bit periods.
*/
status_t receiverRead(volatile uint8_t* buffer) {
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
    uint8_t currVal = !digitalRead(RECEIVER_IN);
    *buffer |=  (currVal << i);
    if (currVal) {
      numHighBits ++;
    }
  }

  // Compares Parity Bit
    receiveDelay();    
    if (!digitalRead(RECEIVER_IN) != (numHighBits % 2)) { // compare parity
      return PARITY_ERROR;
    }

  // Checks Stop Bit
  receiveDelay();
  if (digitalRead(RECEIVER_IN)) {
    return STOP_ERROR;
  }

  return STATUS_OK;
}



/** SETUP FUNCTIONS */
/**
* @brief Helper function that initializes Receiver Pin & Attaches ISR
* @note Used in setup
*/
void receiver_init() {
  pinMode(RECEIVER_IN, INPUT);
  attachInterrupt(RECEIVER_IN, receiver_isr, FALLING);
}



/** ARDUINO FUNCTIONS */
void setup() {
  // Initializes Serial Debugging
  Serial.begin(115200);
  Serial.println("Serial Initialized!");

  // Initialize Receiver
  receiver_init();
}

void loop() {
  if (rxFlag) {
    printStatus(receiverRead(&receiveData));
    Serial.printf("Received receiveData: %d\n", receiveData);
    Serial.flush();
    rxFlag = 0;
  }
}



/** PRINT FUNCTIONS */
/** 
* @brief Helper function to print based on status.
* @param status Status to print.
*/
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
