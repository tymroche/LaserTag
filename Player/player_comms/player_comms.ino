#include "player_comms.h"

void setup() {
  // Initializes Serial Debugging
  Serial.begin(115200);
  Serial.println("Serial Initialized!");

  // Initialize Transmit Variables
  rxFlag = false;
  receiveData = 0;
  transmitData = 152;

  // Initialize Trigger
  trigger_init();

  // Initialize Emitter
  emitter_init();

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