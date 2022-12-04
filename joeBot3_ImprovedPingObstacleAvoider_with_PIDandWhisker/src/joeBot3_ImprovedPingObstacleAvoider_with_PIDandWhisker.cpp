/* JoeBot3_20190123_ImprovedPingObstacleAvoider_with_PIDandWhiskers
 * Joe Brendler
 * 6 Dec 2013 (5 Jan 2019)(4 Dec 2022 port to platformio)
 * uses Encoder Library from:
 * http://www.pjrc.com/teensy/td_libs_Encoder.html
 * and DC Motor (L293D) library written by Joe Brendler
 * and DCmotor library by Joe Brendler
 * and standard HC_SR04 and PID_V1 Arduino Libraries
 */

// Basic Idea:  use two instances of PID to control speeds
//  of motors; reset when avoiding
//  --> loop = motor.go(pid_output), ping, 
//             if not safe then avoid,
//             else compute-for-pid;

#include <Arduino.h>
#include <DCmotor2.h>
#include <Encoder.h>
#include <HC_SR04.h>
#include <PID_v1.h>

// instantiate L293D driven DC motors (int 1A, int pwm EN1/2)
// (int 3A, int pwm EN3/4 ) if you're using the other L293D channel
DCmotor2 L_Motor( 4, 5 );
DCmotor2 R_Motor( 7, 10 );

// Change these two numbers to the pins connected to your encoder.
//   Best Performance: both pins have interrupt capability
//   Good Performance: only the first pin has interrupt capability
//   Low Performance:  neither pin has interrupt capability
Encoder L_Encoder( 2, 12 );
Encoder R_Encoder( 3, 11 );
//   avoid using pins with LEDs attached

// instantiate the ultrasonic ping sensor (trigPin, echoPin)
HC_SR04 Sensor( 13, 6 );  

int L_Whisker = 0;   // analog pin A0
int R_Whisker = 1;   // analog pin A1

// PID Theory:
// Parameter RiseTime Overshoot SettleTime SteadyErr Stability
//     Kp     Decr      Incr      Small      Decr      Degrade
//     Ki     Decr      Incr      Incr       Elim      Degrade
//     Kd     Minor     Decr      Decr       none      Improve if small

double kp = 0.01705407711;
double ki = 0.16077170418;
double kd = 0.01;

//Define Variables we'll be connecting to
double mySetpoint, L_Input, L_Output, R_Input, R_Output;

//Specify the links and initial tuning parameters
PID L_PID(&L_Input, &L_Output, &mySetpoint, kp, ki, kd, DIRECT);
PID R_PID(&R_Input, &R_Output, &mySetpoint, kp, ki, kd, DIRECT);

// default speed settings (CAUTION: 100% duty cycle draws too much current
// and the 7805 voltage regulator may go into thermal protection.
// (I added a heat sink, but you are warned anyway)
// I.e. may work for a "little while" and then slow way down.
const int slow = 60;
const int medium = 125;
const int fast = 190;
const int sprint = 255;  // use only *briefly* - see caution above
const double scalingFactor = 31.80784313725490196078431372549;  // determined empirically (8111/255 @ 12.1v)
const int drive_speed = fast;   // leave above alone and select here
const int drive_time = 60000;   // program lasts 60 sec
const int v_closed = 256;  // switch closed should ground the pin (0 volts = 0) otherwise tied high 5v=1023

int fwdback = -1;  // (vs +1); this test starts going backward

long dist = 0;
long minSafeDist = 20;  // cm
long ticksPerCm = 100;

unsigned long t_ref = 0;
long dt = 0;

unsigned long l_ticks = 0;  // encoder displacement reading
unsigned long r_ticks = 0;

int j = 0;

//---------- function declarations ------------
void Avoid_Left();
void Avoid_Right();
void Avoid_Straight();
void GoStraight();

//----------- setup() -----------------------
void setup() {
  Serial.begin(115200);
  L_Encoder.write(0);
  R_Encoder.write(0);
  mySetpoint = drive_speed * 19.6419;
  R_Output = drive_speed;
  L_Output = drive_speed;

  //turn the PID on
  L_PID.SetMode(AUTOMATIC);
  R_PID.SetMode(AUTOMATIC);
  // DIRECT will work, since the avoider only goes forward
  L_PID.SetControllerDirection(DIRECT);
  L_PID.SetControllerDirection(DIRECT);

  t_ref = millis();
}

//---------------------- loop() -----------------------
void loop() {
  unsigned long refTime = millis();
  while ( millis() < refTime + 30000 ) {
    dist = Sensor.ping();
    Serial.print("distance: ");
    Serial.print(dist);
    Serial.print(" cm");
    if ( analogRead(L_Whisker) < v_closed ) {
      Serial.println(" (Avoid Left)");
      Avoid_Left();
    } else if ( analogRead(R_Whisker) < v_closed ) {
      Serial.println(" (Avoid Right)");
      Avoid_Right();
    } else if ( dist < minSafeDist ) {
      Serial.println(" (Avoid Straight)");
      Avoid_Straight();
    } else {
      //go straight
      Serial.println(" (Go straight)");
      GoStraight();
    }
  }
}

void Avoid_Left(){
  // note: motors mounted opposite one another take
  // opposite signs for same direction, and vice versa
  // (-1 * speed) is forward on the right  (r_encoder - is positive)
  L_Encoder.write(0);
  R_Encoder.write(0);
  while ( R_Encoder.read() < 400 ) {
    L_Motor.halt();
    R_Motor.go(drive_speed);  // back away 4 cm
  }
}

void Avoid_Right(){
  // note: motors mounted opposite one another take
  // opposite signs for same direction, and vice versa
  // (-1 * speed) is forward on the right  (r_encoder - is positive)
  L_Encoder.write(0);
  R_Encoder.write(0);
  while ( L_Encoder.read() > -400 ) {
    R_Motor.halt();
    L_Motor.go(-1 * drive_speed);  // back away 4 cm
  }
}

void Avoid_Straight(){
  // note: motors mounted opposite one another take
  // opposite signs for same direction, and vice versa
  // (-1 * speed) is forward on the right  (r_encoder - is positive)
  L_Encoder.write(0);
  R_Encoder.write(0);
  while ( L_Encoder.read() < 3000 ) {
    L_Motor.go(drive_speed);
    R_Motor.go(drive_speed);   // rotate ~1ft of arc-distance on each wheel
  }
}

void GoStraight(){
  // note: motors mounted opposite one another take
  // opposite signs for same direction, and vice versa
  // (-1 * speed) is forward on the right
  l_ticks = L_Encoder.read();
  r_ticks = R_Encoder.read();
  //reset encoder for next delta-t
  L_Encoder.write(0);
  R_Encoder.write(0);
  dt = millis() - t_ref;
  L_Input = double ( 1000.0 * l_ticks ) / double( dt ) ;
  R_Input = double ( 1000.0 * r_ticks ) / double( dt ) ;
  L_PID.Compute();
  R_PID.Compute();

  L_Motor.go ( L_Output );
  R_Motor.go ( -1 * R_Output );
}





