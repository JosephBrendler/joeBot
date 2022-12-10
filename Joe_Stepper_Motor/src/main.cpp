#include <Arduino.h>
#include <Stepper.h>

const int numberOfSteps = 20; // number of steps you want your stepper motor to take
                              // determined for my floppy drive stepper, 20 steps = one complete rotation

#define pot A0

// initialize the stepper library on GPIO D1, D2, D4, D5
Stepper myStepper(numberOfSteps, D1, D2, D4, D5);

int previous = 0;
int value = 0;

void setup()
{
  // set the speed at 60 rpm:
  myStepper.setSpeed(60);
}

void loop()
{
  // get the sensor value
  int value = map(analogRead(pot), 0, 1023, 0, 1000);
  myStepper.setSpeed(value);

  // move 1/10 of a revolution
  myStepper.step(numberOfSteps / 10);
}
