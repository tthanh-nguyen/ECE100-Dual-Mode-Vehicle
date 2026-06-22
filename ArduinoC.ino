#include <Wire.h>
#include <VL53L0X.h>


// ===================================================================
//                      VARIABLES
// ===================================================================
char mode = 0;   
char command = 0;

// ===================================================================
//                      MOTOR DRIVER PINS
// ===================================================================
#define AIN1 7
#define AIN2 8
#define PWMA 5
#define BIN1 10
#define BIN2 11
#define PWMB 6
#define STBY 9

// ===================================================================
//                      ENCODER PINS
// ===================================================================
#define ENC_LEFT_A 2
#define ENC_LEFT_B 4
#define ENC_RIGHT_A 3
#define ENC_RIGHT_B 12

volatile long leftCount = 0;
volatile long rightCount = 0;

// ===================================================================
//                       WHEEL PARAMETERS
// ===================================================================
const float WHEEL_DIAMETER = 34.0;
const int PULSES_PER_REV = 200;
const float WHEEL_CIRCUMFERENCE = PI * WHEEL_DIAMETER;
const float MM_PER_PULSE = WHEEL_CIRCUMFERENCE / PULSES_PER_REV;

const int BASE_SPEED = 80;
const int CORRECTION = 5;


// ===================================================================
//                     MANUAL CONTROL STATE MACHINE
// ===================================================================

enum RobotState {STOP, FORWARD, BACKWARD, TURN_LEFT, TURN_RIGHT, WAITING};
RobotState state = STOP;

// ===================================================================
//                      SENSOR PINS
// ===================================================================
#define XSHUT_LEFT  A0
#define XSHUT_FRONT A1
#define XSHUT_RIGHT A2

VL53L0X sensorLeft, sensorFront, sensorRight;

#define WALL_THRESHOLD 200
#define CLEAR_THRESHOLD 300

// ===================================================================
//                      ISR — ENCODER
// ===================================================================
void leftISR() 
{
  if (digitalRead(ENC_LEFT_B)) leftCount++;
  else leftCount--;
}

void rightISR() 
{
  if (digitalRead(ENC_RIGHT_B)) rightCount++;
  else rightCount--;
}

void stopSensors() 
{
  sensorLeft.stopContinuous();
  sensorFront.stopContinuous();
  sensorRight.stopContinuous();
  delay(50);
}

void startSensors() 
{
  sensorLeft.startContinuous(20);
  sensorFront.startContinuous(20);
  sensorRight.startContinuous(20);
  delay(50);
}

// ===================================================================
//                      MOTOR FUNCTIONS
// ===================================================================
void reverseForSeconds(int ms) 
{
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, HIGH); analogWrite(PWMA, BASE_SPEED);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, HIGH); analogWrite(PWMB, BASE_SPEED);
  delay(ms);
  stopMotors();
}

void moveForward(int ls, int rs) 
{
  ls = constrain(ls, 0, 255);
  rs = constrain(rs, 0, 255);
  digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); analogWrite(PWMA, ls);
  digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW); analogWrite(PWMB, rs);
}

void stopMotors() 
{
  Serial.println("da vao Stopmotors");
  analogWrite(PWMA, 0); analogWrite(PWMB, 0);
  digitalWrite(AIN1, LOW); digitalWrite(AIN2, LOW);
  digitalWrite(BIN1, LOW); digitalWrite(BIN2, LOW);
}

// ===================================================================
//   SAFE FORWARD MOVEMENT WITH WALL DETECTION
// ===================================================================

void moveForwardDistanceSafe(float distance_mm) 
{
  long target = (long)(distance_mm / MM_PER_PULSE);
  noInterrupts(); leftCount = 0; rightCount = 0; interrupts();

  while (true) 
  {
    // Check encoder progress
    noInterrupts();
    long l = abs(leftCount);
    long r = abs(rightCount);
    interrupts();

    // Check front wall
    int frontDist = distFront();
    if (frontDist <= 40) 
    {
      Serial.println("⚠️ Wall detected ahead — stopping early");
      break;
    }

    if ((l + r) / 2 >= target) break;

    long diff = l - r;
    int ls = BASE_SPEED;
    int rs = BASE_SPEED;
    if (diff > 3) ls -= CORRECTION;
    else if (diff < -3) rs -= CORRECTION;

    moveForward(ls, rs);
  }
  stopMotors();
}

// ===================================================================
//                      TURN FUNCTIONS
// ===================================================================
// ===== Turn right =====
void turnRightAuto() 
{
  stopMotors();
  stopSensors();


  Serial.println("→ Turn right");
  noInterrupts(); leftCount = 0; rightCount = 0; interrupts();
  long target = 135;
  while (abs(leftCount) < target || abs(rightCount) < target) {
    digitalWrite(AIN1, HIGH); digitalWrite(AIN2, LOW); analogWrite(PWMA, BASE_SPEED);
    digitalWrite(BIN1, LOW);  digitalWrite(BIN2, HIGH); analogWrite(PWMB, BASE_SPEED);
  }
  stopMotors();
  startSensors();
}

// ===== Turn left =====
void turnLeftAuto() 
{
  stopMotors();
  stopSensors();
  //delay(300);

  Serial.println("← Turn left");
  noInterrupts(); leftCount = 0; rightCount = 0; interrupts();
  long target = 130;
  while (abs(leftCount) < target || abs(rightCount) < target) {
    digitalWrite(AIN1, LOW);  digitalWrite(AIN2, HIGH); analogWrite(PWMA, BASE_SPEED);
    digitalWrite(BIN1, HIGH); digitalWrite(BIN2, LOW);  analogWrite(PWMB, BASE_SPEED);
  }
  stopMotors();
  startSensors();
}
// ===================================================================
//                      AUTO MODE
// ===================================================================
void autoRun() 
{
  
  int L = distLeft();
  int F = distFront();
  int R = distRight();

  Serial.print("L:"); Serial.print(L);
  Serial.print(" F:"); Serial.print(F);
  Serial.print(" R:"); Serial.println(R);

  // Dead-end logic
  if (wallFront() && (wallLeft() || wallRight())) 
  {
    Serial.println("Calibration");

    if (!wallLeft()) 
    {
      turnLeftAuto();
    } 
    else if (!wallRight()) 
    {
      turnRightAuto();
    } 
    else 
    {
      turnLeftAuto(); // fallback turn if all sides blocked
    }

    reverseForSeconds(3000);
  }

  // Right wall-following
  if (!wallRight()) 
  {
    turnRightAuto();
    moveForwardDistanceSafe(250);
  }

  else if (!wallFront()) 
  {
    // Front is clear → go forward
    moveForwardDistanceSafe(250);
  }
  else if (!wallLeft()) 
  {
    // Left is last resort
    turnLeftAuto();
    moveForwardDistanceSafe(250);
  }
  else 
  {
    // Fallback
    turnLeftAuto();
    reverseForSeconds(3000);
  }

  delay(100);
}

// ===================================================================
//                      SENSOR FUNCTIONS
// ===================================================================
int distLeft()  { return sensorLeft.readRangeContinuousMillimeters(); }
int distFront() { return sensorFront.readRangeContinuousMillimeters(); }
int distRight() { return sensorRight.readRangeContinuousMillimeters(); }

bool wallLeft()  { return distLeft()  < WALL_THRESHOLD; }
bool wallFront() { return distFront() < 40; }
bool wallRight() { return distRight() < WALL_THRESHOLD; }

// ===================================================================
//                      MANUAL MODE
// ===================================================================
void motorRaw(int ls, int rs, bool leftForward=true, bool rightForward=true) {
  ls = constrain(ls, 0, 255);
  rs = constrain(rs, 0, 255);

  digitalWrite(AIN1, leftForward ? HIGH : LOW);
  digitalWrite(AIN2, leftForward ? LOW : HIGH);
  digitalWrite(BIN1, rightForward ? HIGH : LOW);
  digitalWrite(BIN2, rightForward ? LOW : HIGH);

  analogWrite(PWMA, ls);
  analogWrite(PWMB, rs);
}

void balanceMotor(bool leftForward = true, bool rightForward = true) 
{
    noInterrupts();
    long l = leftCount;
    long r = rightCount;
    interrupts();

    long diff = l - r;
    int ls = BASE_SPEED;
    int rs = BASE_SPEED;

    if (diff > 0) ls -= CORRECTION;
    else if (diff < -0) rs -= CORRECTION;

    motorRaw(ls, rs, leftForward, rightForward);
}

void movement() 
{
  Serial.println("Tôi đã vào movement rồi");
  switch(state) 
  {
    case FORWARD:
      Serial.println("chạy forward nhé");
      balanceMotor(true, true);        
      break;
    case BACKWARD:
      balanceMotor(false, false);
      break;
    case TURN_LEFT:
      motorRaw(BASE_SPEED, BASE_SPEED, false, true);
      break;
    case TURN_RIGHT:
      motorRaw(BASE_SPEED, BASE_SPEED, true, false);
      break;
    case STOP:
      stopMotors();
      break;
    case WAITING:
      // waiting for a new command 
      break;
  }
}


void manualControl() 
{
  // ============================
  // MANUAL COMMAND EXECUTION
  // ============================
  noInterrupts();              // Disable encoder interrupts
  leftCount = 0;               // Reset left encoder
  rightCount = 0;              // Reset right encoder
  interrupts();                // Re-enable interrupts
  
  switch(command)              // Process manual command
  {
    case 'W': 
      state = FORWARD;     // Move forward
      movement();        
      delay(1000);       
      stopMotors();      
      state = STOP;  
      break;
    case 'X': 
      state = BACKWARD;        // Move backward
      movement();        
      delay(1000);       
      stopMotors();      
      state = STOP;  
      break;
    case 'A': 
      state = TURN_LEFT;       // Turn left
      movement();       
      delay(1300);      
      stopMotors();    
      state = STOP;  
      break;
    case 'D': 
      state = TURN_RIGHT;      // Turn right
      movement();        
      delay(1300);       
      stopMotors();     
      state = STOP;  
      break;
    case 'S': 
      stopMotors();            // Stop motors
      break;
    case 'Q':
             // Idle state
      break;
  }

  command = 'Q';               // Reset command
  Serial.println("set command = Q");
}


// ===================================================================
//                      SETUP
// ===================================================================
void setup() 
{
  Serial.begin(9600);
  Wire.begin();

  pinMode(AIN1, OUTPUT); pinMode(AIN2, OUTPUT); pinMode(PWMA, OUTPUT);
  pinMode(BIN1, OUTPUT); pinMode(BIN2, OUTPUT); pinMode(PWMB, OUTPUT);
  pinMode(STBY, OUTPUT); digitalWrite(STBY, HIGH);

  pinMode(ENC_LEFT_A, INPUT_PULLUP);
  pinMode(ENC_LEFT_B, INPUT_PULLUP);
  pinMode(ENC_RIGHT_A, INPUT_PULLUP);
  pinMode(ENC_RIGHT_B, INPUT_PULLUP);

  attachInterrupt(digitalPinToInterrupt(ENC_LEFT_A), leftISR, RISING);
  attachInterrupt(digitalPinToInterrupt(ENC_RIGHT_A), rightISR, RISING);

  // init VL53L0X
  pinMode(XSHUT_LEFT,  OUTPUT);
  pinMode(XSHUT_FRONT, OUTPUT);
  pinMode(XSHUT_RIGHT, OUTPUT);

  digitalWrite(XSHUT_LEFT, LOW);
  digitalWrite(XSHUT_FRONT, LOW);
  digitalWrite(XSHUT_RIGHT, LOW);
  delay(10);

  digitalWrite(XSHUT_LEFT, HIGH);  delay(10);
  sensorLeft.init(true);  sensorLeft.setAddress(0x30);  sensorLeft.startContinuous(20);

  digitalWrite(XSHUT_FRONT, HIGH); delay(10);
  sensorFront.init(true); sensorFront.setAddress(0x31); sensorFront.startContinuous(20);

  digitalWrite(XSHUT_RIGHT, HIGH); delay(10);
  sensorRight.init(true); sensorRight.setAddress(0x32); sensorRight.startContinuous(20);

  Serial.println("Ready!");
  delay(1000);
}

// ===================================================================
//                      MAIN LOOP
// ===================================================================
void loop() 
{
  // ====================================================
  // RECEIVE DATA FROM SERIAL (FROM CONTROLLER)
  // ====================================================
  if (Serial.available())                   // Check if serial data is available
  {    
    char incoming = Serial.read();          // Read incoming character
    Serial.println("Receive data from Serial.");

    if (incoming == '\n' || incoming == '\r') return;

    if (incoming == '1' || incoming == '2') // If input is mode selection
    {
      mode = incoming;                      // Update robot mode
      Serial.print("Mode: ");
      Serial.println(mode);
    } 
    else                                    // Otherwise, it is a manual command
    {
      command = incoming;                   // Update movement command
      Serial.print("Command: ");
      Serial.println(command);
    }
  }
  // ======================================
  //       MODE PROCESSING & EXECUTION
  // ======================================
  if (mode == '1')                          // Manual control mode
  {
    manualControl();                       // Execute manual movement (W A S D X)
  }
  else if (mode == '2')                    // Automatic mode
  {
    autoRun();                             // Run maze-solving algorithm
  }
}

