const float SPEED_OF_SOUND = 0.0343; // cm/µs

const int LED_PIN = 13; // LED connected to digital pin 13
const int BUZZER_PIN = 12; // BUZZER connected to digital pin 12
const int ULTRASONIC_TRIG_PIN = 10; // UltraSonic Trig pin connected to digital pin 10
const int ULTRASONIC_ECHO_PIN = 2; // UltraSonic Echo pin connected to digital pin 2

const float STOP_DISTANCE = 8.0;
const float FAST_DISTANCE = 15.0;
const float MEDIUM_DISTANCE = 25.0;
const float MAX_DISTANCE = 40.0;

const unsigned long TRIGGER_INTERVAL = 60; // ms
const unsigned long MEASUREMENT_TIMEOUT = 200; // ms
const unsigned long BEEP_DURATION = 80; // ms (Fixed duration for each beep)

volatile unsigned long echo_start = 0;
volatile unsigned long pulse_duration = 0;  // received pulse duration in µs
volatile bool new_measurement = false;

float distance_cm = 100.0; // cm
bool distance_valid = false;

unsigned long last_measurement_time = 0;
unsigned long last_toggle = 0;
bool output_state = false;

void setup() {
  Serial.begin(9600);
  pinMode(LED_PIN, OUTPUT);
  pinMode(BUZZER_PIN, OUTPUT);
  pinMode(ULTRASONIC_TRIG_PIN, OUTPUT);
  pinMode(ULTRASONIC_ECHO_PIN, INPUT);
  
  attachInterrupt(digitalPinToInterrupt(ULTRASONIC_ECHO_PIN), echo_isr, CHANGE);
}

// *******************************************************

void loop() {

  // Trigger every 60 ms
  static unsigned long last_trigger = 0;

  if (millis() - last_trigger >= TRIGGER_INTERVAL) {
    last_trigger = millis();
    trigger_ultrasonic();
  }

  // Read new measurement from ISR
  if (new_measurement) {
    noInterrupts();
    unsigned long duration = pulse_duration;
    new_measurement = false;
    interrupts();

    // Hardware noise filter: ignore GND disconnect glitches and out-of-bounds errors
    if (duration > 100 && duration < 30000) {
      distance_cm = distance_calculator(duration);
      distance_valid = true;
      last_measurement_time = millis();
      /*
      Serial.print("Distance: ");
      Serial.print(distance_cm);
      Serial.println(" cm");
      */
    }
  }
  
  // Handle measurement timeout (e.g. wire disconnected)
  if (distance_valid && (millis() - last_measurement_time) >= MEASUREMENT_TIMEOUT) {
    distance_valid = false;
  }

  update_warning(distance_cm, distance_valid);
}
// *******************************************************

float distance_calculator(unsigned long p_duration){
  return (SPEED_OF_SOUND * p_duration) / 2.0;
}

void trigger_ultrasonic(void){
  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
  delayMicroseconds(2);

  digitalWrite(ULTRASONIC_TRIG_PIN, HIGH);
  delayMicroseconds(10);

  digitalWrite(ULTRASONIC_TRIG_PIN, LOW);
}

void echo_isr(void){
  // Direct Port Manipulation (PIND & _BV(PD2)) -> faster for interrupt
  if (PIND & _BV(PD2)) {  
    echo_start = micros();
  } else {
    pulse_duration = micros() - echo_start;
    new_measurement = true;
  }
}

void set_output(bool state){
  digitalWrite(LED_PIN, state);
  if (state){
    tone(BUZZER_PIN, 1000); // 1000Hz frequency for a sharp beep
  } else {
    noTone(BUZZER_PIN);
  }
}

void update_warning(float distance, bool valid){
  // Out of range or sensor timeout
  if (!valid || distance > MAX_DISTANCE){
    output_state = false;
    set_output(false);
    return;
  }
  
  // Danger zone: Continuous output
  if (distance <= STOP_DISTANCE){
    output_state = true;
    set_output(true); 
    return;
  }

  // Silence interval between beeps based on distance
  unsigned long silence_interval = (distance > MEDIUM_DISTANCE) ? 600 :
                                   (distance > FAST_DISTANCE) ? 300 : 120;

  unsigned long current_time = millis();

  if (output_state) {
    // If ON, check if BEEP_DURATION has passed to turn OFF
    if (current_time - last_toggle >= BEEP_DURATION) {
      output_state = false;
      set_output(false);
      last_toggle = current_time;
    }
  } else {
    // If OFF, check if silence_interval has passed to turn ON
    if (current_time - last_toggle >= silence_interval) {
      output_state = true;
      set_output(true);
      last_toggle = current_time;
    }
  }
}
