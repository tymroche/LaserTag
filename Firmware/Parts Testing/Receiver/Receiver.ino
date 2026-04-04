#define RECEIVER_RX 1       // GPIO Pin 9 w/ ADC
int rxVal;

void setup() {
  Serial.begin(115200);
  delay(2000);              // Allow Serial to attach
  Serial.println("Serial Initialized!");

  pinMode(RECEIVER_RX, INPUT);        // Set mode explicitly
}

void loop() {
  rxVal = analogRead(RECEIVER_RX);
  if (rxVal == 4095) {    Serial.println("No input Detected!");
  } else {                Serial.printf("Input Recieved. Strength: %f\n", analogRead(RECEIVER_RX)); }

  delay(1000);
}
