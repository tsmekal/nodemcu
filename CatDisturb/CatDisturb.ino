#define ECHO_PIN 2 // attach pin D2 Arduino to pin Echo of HC-SR04
#define TRIG_PIN 3 //attach pin D3 Arduino to pin Trig of HC-SR04
#define BLINK_LED_PIN 13

#define DELAY_BETWEEN_DISTURB 10  // In seconds
#define DISTURB_TIME 5  // In seconds

long blinkTimePrev = 0;
long sensorTimePrev = 0;

// defines variables
long duration; // variable for the duration of sound wave travel
int distance; // variable for the distance measurement

boolean movementDetected = false;

void setup() {
  Serial.begin(9600); // // Serial Communication is starting with 9600 of baudrate speed
  pinMode(TRIG_PIN, OUTPUT); // Sets the TRIG_PIN as an OUTPUT
  pinMode(ECHO_PIN, INPUT); // Sets the ECHO_PIN as an INPUT
  pinMode(BLINK_LED_PIN, OUTPUT);  
}
void loop() {
  _detectedMovement();
  if(movementDetected){
    _runSensor();
  }
}

void _runSensor(){

  long timeNow = millis();
  long diff = timeNow - sensorTimePrev;
  
  if (diff >= _getMilis(DELAY_BETWEEN_DISTURB) && diff <= _getMilis(DELAY_BETWEEN_DISTURB + DISTURB_TIME)){
    if(diff >= _getMilis(DELAY_BETWEEN_DISTURB + DISTURB_TIME) - 500){  // 500 is offset
      sensorTimePrev = timeNow;
    }
    digitalWrite(TRIG_PIN, LOW);
    delayMicroseconds(2);
    // Sets the TRIG_PIN HIGH (ACTIVE) for 10 microseconds
    digitalWrite(TRIG_PIN, HIGH);
    delayMicroseconds(10);
    digitalWrite(TRIG_PIN, LOW);
    // Reads the ECHO_PIN, returns the sound wave travel time in microseconds
    duration = pulseIn(ECHO_PIN, HIGH); 
    // Calculating the distance
    distance = duration * 0.034 / 2; // Speed of sound wave divided by 2 (go and back)
    // Displays the distance on the Serial Monitor
    Serial.print("Distance: ");
    Serial.print(distance);
    Serial.println(" cm");
    _blink();
  }
  else{
    digitalWrite(LED_BUILTIN, LOW);
  }
}

void _blink(void){
  
  long timeNow = millis();
  if (timeNow - blinkTimePrev >= 400){
    blinkTimePrev = timeNow;
    if(digitalRead(LED_BUILTIN) == HIGH){
      digitalWrite(LED_BUILTIN, LOW);
    }
    else{
      digitalWrite(LED_BUILTIN, HIGH);
    }
  }
    
}

void _detectedMovement(void){
  movementDetected = true;
}

long _getMilis(int seconds){
  return seconds * 1000;
}
