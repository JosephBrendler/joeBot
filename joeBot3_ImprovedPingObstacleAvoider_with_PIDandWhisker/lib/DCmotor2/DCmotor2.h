/* DCmotor2.h
 Joe Brendler
 30 Nov 2013
 Provides controls for a DC motor driven by an L293D motor driver chip
   Outputs signals 1A, 2A (or 3A, 4A) for L293D motor driver chip
   Caution 1A/2A combination HI,HI is dangerous and may damage your
     equipment or cause an explosion or fire.
   This object module is a modification of Joe Brendler's "DCmotor" that
     eliminates the 2A control pin as an input, assuming that the hardware
     is driving that with an inverted output from the same signal provided
     for 1A (this way HI-HI is not possible, by the way).  This module therefor
     also assumes that "halt" will be performed by an input of zero-duty-cycle
     PWM on the EN pin. (since LO-LO is also not possible)
   Outputs speed control for the L293 EN1 (or EN2) as PWM using analogWrite
*/

#ifndef DCmotor2_h
#define DCmotor2_h

#include "Arduino.h"
class DCmotor2
{
  public:
    DCmotor2(int motor_1A, int pwm);
    void halt();
    void go(int motor_speed);
  private:
    void forward();
    void backward();
    int _motor_1A;
    int _pwm;
};

#endif