#include <Arduino.h>
#include "Stepper.h"

#define pot A0

const int stepsPerRevolution = 200;
int previous = 0, value = 0;

// initialize the three-phase stepper library on GPIO
Stepper myStepper(stepsPerRevolution, D5, D6, D7);

void setup()
{
  // set the speed at 60 rpm:
  myStepper.setSpeed(1);
  // initialize the serial port:
  Serial.begin(115200);
}

void loop()
{
  // get the sensor value
  int value = map(analogRead(pot), 0, 1023, 0, 1000);
  myStepper.setSpeed(value);

  // step one revolution  in one direction:
  Serial.printf("clockwise, speed: %d\n", value);
  myStepper.step(stepsPerRevolution);
  delay(500);

  // step one revolution in the other direction:
  Serial.printf("counterclockwise, speed: %d\n", value);
  myStepper.step(-stepsPerRevolution);
  delay(500);
}
/*
void old_variables()
{
  const int numberOfSteps = 20; // number of steps you want your stepper motor to take
                                // determined for my floppy drive stepper, 20 steps = one complete rotation
  int previous = 0;
  int value = 0;

void old_setup()
{
  // set the speed at 60 rpm:
  myStepper.setSpeed(60);
}
void old_loop()
{
  // get the sensor value
  int value = map(analogRead(pot), 0, 1023, 0, 1000);
  myStepper.setSpeed(value);

  // move 1/10 of a revolution
  myStepper.step(numberOfSteps / 10);
}
}
*/