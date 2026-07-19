const int LED_PIN = 13;     // LED connected to digital pin 13
int LDR_VALUE;              // Analog reading from the LDR (0-1023)

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
}

void loop() {
  // Read the analog value from the LDR (0-1023)
  LDR_VALUE = analogRead(A0);

  // Uncomment the next line to print the sensor reading.
  // Serial.println(LDR_VALUE);

  if (LDR_VALUE >= 650) {
    // Environment is dark
    digitalWrite(LED_PIN, HIGH);
  }
  else if (LDR_VALUE <= 230) {
    // Environment is bright
    digitalWrite(LED_PIN, LOW);
  }
}