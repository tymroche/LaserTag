#define LED 21
#define EMITTER_TX 1

#define ON 1
#define OFF 0
bool status;

void setup() {
  Serial.begin(115200);
  pinMode(LED, OUTPUT);
  pinMode(EMITTER_TX, OUTPUT);
  status = OFF;
}

void loop() {
  if (status) { digitalWrite(LED, HIGH); digitalWrite(EMITTER_TX, HIGH); status = OFF; 
  } else { digitalWrite(LED, LOW); digitalWrite(EMITTER_TX, LOW); status = ON; }
  delay(100);
}