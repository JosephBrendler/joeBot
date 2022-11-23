#include <Arduino.h>

const int phase1pin = D5;
const int phase2pin = D6;
const int phase3pin = D7;

float holdTime;
float targetSpeed;

unsigned long p1start,
    p1end,
    p2start,
    p2end,
    p3start,
    p3end;

// Change these until you find your min and max speeds.
// This looks backwards because min/max are pulse length (us) inversly proportional to speed
float min = 3200;
float max = 50000;

void setup()
{
    Serial.begin(115200);
    pinMode(phase1pin, OUTPUT);
    pinMode(phase2pin, OUTPUT);
    pinMode(phase3pin, OUTPUT);
}

String direction;
bool running = false;
float status;

// Total pulse period is "refractory", 2.25 * current holdTime
// Each in sequence is on for 1 * hold and off for 1.25 * hold
// Each is offset from the previous by 0.75 * hold
// chkP1 goes on at td=0, off at td = 1
// chkP2 goes on at td=0.75, off at 1.75
// chkP3 goes on at td=1.5, off at 2.5, but
// a new cycle starts at 2.25 (or whatever "refractory changes to")
void chkP1(int pin)
{
    unsigned long currentTime = micros();
    unsigned long td = currentTime - p1start;
    unsigned long refractory = 2.25 * holdTime;
    if (digitalRead(pin))
    {
        if (td > holdTime)
        {
            digitalWrite(pin, LOW);
            p1end = currentTime;
        }
    }
    else if (td > refractory)
    {
        digitalWrite(pin, HIGH);
        p1start = currentTime;
    }
}

void chkP2(int pin)
{
    unsigned long currentTime = micros();
    unsigned long td = currentTime - p1start;
    if (digitalRead(pin))
    {
        if (td > 1.75 * holdTime || td < 0.75 * holdTime)
        {
            digitalWrite(pin, LOW);
            p2end = currentTime;
        }
    }
    else if (td > 0.75 * holdTime && td < 1.75 * holdTime)
    {
        digitalWrite(pin, HIGH);
        p2start = currentTime;
    }
}

void chkP3(int pin)
{
    unsigned long currentTime = micros();
    unsigned long td = currentTime - p1start;
    if (digitalRead(pin))
    {
        if (td > 0.25 * holdTime && p3start < p1start)
        {
            digitalWrite(pin, LOW);
            p3end = currentTime;
        }
    }
    else if (td > 1.5 * holdTime)
    {
        digitalWrite(pin, HIGH);
        p3start = currentTime;
    }
}

void setupMotor(float input_targetSpeed, String new_direction)
{
    // Converts the percent value into something usable
    // This looks backwards because min/max are pulse length (us) inversly proportional to speed
    targetSpeed = max - ((max - min) * (input_targetSpeed * 0.01));

    // See if the direction has changed
    if (new_direction != direction)
    {
        holdTime = 20000;
        direction = new_direction;
        // Wait for it to stop, weird things can happen if it's still moving.
        delay(500);

        // Initialize first phase - get it moving
        if (direction == "forward")
        {
            digitalWrite(phase1pin, HIGH);
        }
        else if (direction == "reverse")
        {
            digitalWrite(phase3pin, HIGH);
        }
    }

    running = true;
}

void loop()
{

    if (Serial.available() > 0)
    {
        // Read serial input ex. "100 reverse" or "50 forward"
        float input_targetSpeed = Serial.readStringUntil(' ').toInt();

        // Serial.read(); //next character is space, so skip it using this
        String new_direction = Serial.readStringUntil('\n');
        setupMotor(input_targetSpeed, new_direction);
    }

    if (running)
    {

        if (direction == "forward")
        {
            chkP1(phase3pin);
            chkP2(phase2pin);
            chkP3(phase1pin);
        }
        else if (direction == "reverse")
        {
            chkP1(phase1pin);
            chkP2(phase2pin);
            chkP3(phase3pin);
        }

        delayMicroseconds(100);

        if (holdTime > targetSpeed)
        {
            holdTime -= 0.5;
        }
        else if (holdTime < targetSpeed)
        {
            holdTime += 0.25;
        }

        // Monitors the current speed
        status = -(holdTime - max) / ((max - min) * (0.01));

        if (status == 100)
        {
            // setupMotor(100, "reverse");
        }
    }
}