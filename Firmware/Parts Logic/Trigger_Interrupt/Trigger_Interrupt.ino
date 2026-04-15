#define TRIGGER_SWITCH  2 

int numTrigger = 0;
unsigned long lastTrigger = 0;

void IRAM_ATTR trigger_isr() {
  if ((millis() - lastTrigger) < 200) return;
  numTrigger++;
  lastTrigger = millis();
}

void setup() {
  Serial.begin(115200);
  Serial.println("Serial Initialized!");

  pinMode(TRIGGER_SWITCH, INPUT_PULLDOWN);
  attachInterrupt(TRIGGER_SWITCH, trigger_isr, RISING);// handler, mode);

}

void loop() {
  Serial.printf("Button Pressed %d times.\n", numTrigger);
  delay(500);
}
