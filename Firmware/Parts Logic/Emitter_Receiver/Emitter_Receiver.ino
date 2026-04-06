#define BUTTON 21
#define EMITTER_TX 1
#define RECEIVER_RX 9                 // GPIO Pin 9 w/ ADC

void setup() {
  Serial.begin(115200);
  delay(2000);                        // Allow Serial to attach
  Serial.println("Serial Initialized!");

  // Initialize Pins
  pinMode(BUTTON, INPUT_PULLDOWN);
  pinMode(RECEIVER_RX, INPUT);        // Set mode explicitly
  pinMode(EMITTER_TX, OUTPUT);

}

void loop() {
  digitalWrite(EMITTER_TX, digitalRead(BUTTON) ? HIGH : LOW);
  if (analogRead(RECEIVER_RX) != 4095)    Serial.println("Input Detected!");
  delay(100);
}