/*
 DCmotor2.cpp
 Joe Brendler
 30 Nov 2013
 Provides controls for a DC motor driven by an L293D motor driver chip
   Outputs signals 1A, 2A (or 3A, 4A) for L293D motor driver chip
   Caution 1A/2A combination HI,HI is dangerous and may damage your
     equipment or cause an explosion or fire.  (don't do that).
   This object module is a modification of Joe Brendler's "DCmotor" that
     eliminates the 2A control pin as an input, assuming that the hardware
     is driving that with an inverted output from the same signal provided
     for 1A (this way HI-HI is not possible, by the way).  This module therefor
     also assumes that "halt" will be performed by an input of zero-duty-cycle
     PWM on the EN pin. (since LO-LO is also not possible)
   Outputs speed control for the L293 EN1 (or EN2) as PWM using analogWrite
*/

#include "Arduino.h"
#include "DCmotor2.h"

// DCmotor2 Constructor
DCmotor2::DCmotor2(int motor_1A, int pwm)
{
  pinMode(motor_1A, OUTPUT);
  pinMode(pwm, OUTPUT);
  _motor_1A = motor_1A;
  _pwm = pwm;
}

void DCmotor2::halt(){
  analogWrite(_pwm, 0);
}

void DCmotor2::forward(){
  digitalWrite(_motor_1A, HIGH);
}

void DCmotor2::backward(){
  digitalWrite(_motor_1A, LOW);
}

void DCmotor2::go(int motor_speed){
  if ( motor_speed > 0 ) {
    forward();
  }
  else 
    if ( motor_speed < 0 ) {
    backward();
  }
  else {
    halt();
  }

  analogWrite( _pwm, abs(motor_speed) ); 
}
