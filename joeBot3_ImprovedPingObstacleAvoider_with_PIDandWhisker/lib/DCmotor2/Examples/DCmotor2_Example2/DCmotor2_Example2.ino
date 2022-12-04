#include <DCmotor2.h>

// instantiate L293D driven DC motors (int 1A, int EN1/2)
// (int 3A, int EN3/4 ) if you're using the other L293D channel
DCmotor2 R_motor( 4, 5 );
DCmotor2 L_motor( 7, 10 );

// default speed settings (CAUTION: 100% duty cycle draws too much current
// and the 7805 voltage regulator will go into thermal protection.
// I.e. will work for a "little while" and then slow way down.
const int slow = 60;
const int medium = 125;
const int fast = 190;
const int sprint = 255;  // use only *briefly* - see caution above

const int fwd = 1;      // positive direction
const int bkwd = -1;    // negative direction
int dir = 0;            // variable direction

boolean DONE = false;

void setup()
{
  Serial.begin(115200);
  DONE = false;
}

void test()
{
  int i;  // for-loop counter
  
  Serial.println("Halt");
  R_motor.halt();
  L_motor.halt();
  delay(1000);
  
  Serial.println("Forward");
  dir = fwd;
  // change speeds
  Serial.println("Go Slow");
  R_motor.go(dir * slow);
  L_motor.go(dir * slow);
  delay(1000);
  Serial.println("Go Medium");
  R_motor.go(dir * medium);
  L_motor.go(dir * medium);
  delay(1000);
  Serial.println("Go Fast");
  R_motor.go(dir * fast);
  L_motor.go(dir * fast);
  delay(1000);
  Serial.println("Go Sprint");
  R_motor.go(dir * sprint); 
  L_motor.go(dir * sprint); 
  delay(1000);
  R_motor.halt();
  L_motor.halt();
  delay(300);
  // change speeds gradually
  for ( int i = 60; i <= 255; i += 5 ) {
    Serial.print("Go ");
    Serial.println(i);
    R_motor.go(i);
    L_motor.go(i);
    delay(200);
  }
  Serial.println("Halt");
  R_motor.halt();                       
  L_motor.halt();                       
  delay(1000);

  Serial.println("Backward");  
  dir = bkwd;
  // change speeds
  // change speeds
  Serial.println("Go Slow");
  R_motor.go(dir * slow);
  L_motor.go(dir * slow);
  delay(1000);
  Serial.println("Go Medium");
  R_motor.go(dir * medium);
  L_motor.go(dir * medium);
  delay(1000);
  Serial.println("Go Fast");
  R_motor.go(dir * fast);
  L_motor.go(dir * fast);
  delay(1000);
  Serial.println("Go Sprint");
  R_motor.go(dir * sprint); 
  L_motor.go(dir * sprint); 
  delay(1000);
  R_motor.halt();
  L_motor.halt();
  delay(300);
  // change speeds gradually
  for ( i = -60; i >= -255; i -= 5 ) {
    Serial.print("Go ");
    Serial.println(i);
    R_motor.go(i);
    L_motor.go(i);
    delay(200);
  }
  Serial.println("Halt");
  R_motor.halt();                       
  L_motor.halt();                       
  delay(1000);
  DONE = true;
}

void loop() {
  if (! DONE) test();
}
