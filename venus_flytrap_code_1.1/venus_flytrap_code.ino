#include <Servo.h>

#define OUTPUT_DATA true //comment this out if not printing to serial monitor

//pin definitions
#define PIR_PIN 2
#define ECHO_PIN 3
#define TRIGGER_PIN 4
#define SERVO_PIN 5
#define ENABLE_ULTRASONIC 6

////variables
//servo
Servo Engine;
#define CLOSED_POS 180 ///ADJUST AS REQUIRED!!!!!
#define OPEN_POS 90 ///ADJUST AS REQUIRED!!!!!
#define SERVO_NEW_PERCENT 0.05
#define SERVO_OLD_PERCENT (1 - SERVO_NEW_PERCENT)
float smoothed_servo_pos = OPEN_POS;  //set this to the stationary position for the flytrap
float previous_servo_pos = smoothed_servo_pos;

//ultrasonic sensor
#define SPEED_OF_SOUND_MM_PER_S 343000 //millimetres per seconds
uint16_t trigger_distance_mm = 1000; //millimetres

//PIR sensor
bool is_human_present = false;

void setup() {
  //tell the Arduino what is attached to it
  pinMode(PIR_PIN, INPUT);
  pinMode(ECHO_PIN, INPUT);
  pinMode(ENABLE_ULTRASONIC, INPUT_PULLUP); //this means it defaults to enabled (HIGH)
  
  pinMode(TRIGGER_PIN, OUTPUT);
  digitalWrite(TRIGGER_PIN, LOW);

  Engine.attach(SERVO_PIN);

  #ifdef OUTPUT_DATA
  Serial.begin(115200);
  #endif

  Engine.write(CLOSED_POS);
  delay(2000);
  Engine.write(OPEN_POS);
  delay(2000);
}

uint16_t getDistance() {
  digitalWrite(TRIGGER_PIN, HIGH);
  delayMicroseconds(10); //send a 10uS trigger pulse
  digitalWrite(TRIGGER_PIN, LOW);
  
  /* Normally: 
   *     speed = distance/time 
   *     distance = speed * time
   * However the soundwave must bounce BACK, taking twice as long, therefore:
   *     2*distance = speed * time
   *       distance = 0.5 * speed * time
   */
  uint16_t duration_uS = pulseIn(ECHO_PIN, HIGH, 0); //Time (in microseconds, uS) how long the ECHO pin pulse is HIGH
  uint16_t distance_mm = 0.5 * SPEED_OF_SOUND_MM_PER_S * duration_uS * 1e-6; //conversions done inline to prevent inaccuracies through rounding

  #ifdef OUTPUT_DATA
  //display results; helps debugging/calibration
  Serial.print("\nuS: "); 
  Serial.print(duration_uS); 
  Serial.print(" distance_mm: ");
  Serial.print(distance_mm);
  #endif

  return distance_mm;
}

void determineMotion() {
  if (digitalRead(PIR_PIN)) { //if motion is detected...
    if (is_human_present == false) { //if motion hasn't been detected previously
      #ifdef OUTPUT_DATA
      Serial.print(" | New Motion detected!");
      #endif
      is_human_present = true;
    }
    
  } else { //if motion not detected...
    if (is_human_present) {
      #ifdef OUTPUT_DATA
      Serial.print(" | Motion ended");
      #endif
      is_human_present = false;
    }
  }
}

bool isPositionReached(int starting_pos, int target_pos) {
  int current_pos = Engine.read(); //does not actually read the physical position of the servo
  //Serial.print(" current_pos under isPositionReached(...) ");
  //Serial.print(current_pos);
  if (starting_pos < target_pos) {
    if (current_pos >= target_pos*0.99) {
      #ifdef OUTPUT_DATA
      Serial.println("position reached");
      #endif
      return true;
    } else {
      return false;
    }
  } else if (starting_pos > target_pos) {
    if (current_pos <= target_pos*0.99) {
      #ifdef OUTPUT_DATA
      Serial.println("position reached");
      #endif
      return true;
    } else {
      return false;
    }
  } else {
    return false; //means starting_pos = target_pos; don't know how to throw error
  }
}

void closeJaws(int close_pos) {
  #ifdef OUTPUT_DATA
  Serial.print(" Closing Jaws!");
  #endif
  
  int initial_pos = Engine.read();
  while (!isPositionReached(initial_pos, close_pos)) { //this checks to stop once actually closed
    
    float smoothed_servo_pos = (close_pos * SERVO_NEW_PERCENT) + (previous_servo_pos * SERVO_OLD_PERCENT); //calculate smoothed servo angle. Automatically casts to int, rounding down, when passed to Servo
    
    previous_servo_pos = smoothed_servo_pos; //update prev_smoothed_angle
    //Serial.print(" smoothed_servo_pos: ");
    //Serial.print(smoothed_servo_pos);
    //Serial.print("\n");
    Engine.write(smoothed_servo_pos);
    delay(10);
  }
}

void openJaws(int open_pos) {
  #ifdef OUTPUT_DATA
  Serial.print(" Opening Jaws!");
  #endif
  
  int initial_pos = Engine.read();
  while (isPositionReached(open_pos, initial_pos)) { //this checks to stop once actually closed
    
    float smoothed_servo_pos = (open_pos * SERVO_NEW_PERCENT) + (previous_servo_pos * SERVO_OLD_PERCENT); //calculate smoothed servo angle. Automatically casts to int, rounding down, when passed to Servo
    
    previous_servo_pos = smoothed_servo_pos; //update prev_smoothed_angle
    Engine.write(smoothed_servo_pos);
    delay(10);
  }
}

void loop() {
    
  uint16_t current_distance_mm;// = getDistance();
  if (digitalRead(ENABLE_ULTRASONIC)) {
    current_distance_mm = getDistance();
  } else {
    current_distance_mm = trigger_distance_mm; //means that the 'main trigger if' is only dependant on the PIR state
  }
  

  determineMotion(); //update is_human_present depending on the PIR sensor

  //main flytrap trigger if
  if ((current_distance_mm <= trigger_distance_mm) && is_human_present) {
    #ifdef OUTPUT_DATA
    Serial.print("\nTriggered!!! ENABLE_ULTRASONIC: ");
    Serial.print(digitalRead(ENABLE_ULTRASONIC));
    #endif
    Engine.write(CLOSED_POS); //closeJaws(CLOSED_POS);
    delay(2000);
    Engine.write(OPEN_POS); //openJaws(OPEN_POS);
  }

  delay(60); //to prevent Ultrasonic sensor's sound waves from interfering with itself
}
