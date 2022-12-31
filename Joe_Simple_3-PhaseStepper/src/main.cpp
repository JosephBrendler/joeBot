#include <Arduino.h>
#include <Stepper.h>

//#define phase1 2
//#define phase2 5
//#define phase3 26 
#define phase1 D5
#define phase2 D6
#define phase3 D7

#define stepsPerCycle 6
// initialize stepper motor and associated variables
Stepper myStepper(stepsPerCycle, phase1, phase2, phase3);

int speed = 100; // initial value
int speedIncrement = 1;
int speedLimit = 7000;
String msg = "";

uint64_t now = 0, before = 0;

void setup()
{
    Serial.begin(115200);
    msg = "Starting setup";
    Serial.println(msg);

    myStepper.setSpeed(speed);
    // bump rotor to known position
    digitalWrite(phase1, HIGH);
    digitalWrite(phase2, HIGH);
    delay(100);
    msg = "done setup()";
    Serial.println(msg);
}

void loop()
{
    now = micros();
    if (now - before > 1000)
    {
        // adjust speed
        if (speed < speedLimit)
        {
            speedIncrement = map(speed, 0, speedLimit, 50, 1);
            speed += speedIncrement;
        }
        else
        {
            speed = speedLimit;
        }
        myStepper.setSpeed(speed);
        before = now;
    }
    myStepper.step(stepsPerCycle);
}