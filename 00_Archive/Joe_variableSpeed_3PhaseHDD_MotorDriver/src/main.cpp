#include <Arduino.h>

const int phase1pin = D5;
const int phase2pin = D6;
const int phase3pin = D7;

unsigned long p1start,
    p1end,
    p2start,
    p2end,
    p3start,
    p3end,
    currentTime,
    refractory,
    td;

float holdTime;
float targetHoldTime;

// Change these until you find your minPulse and maxPulse speeds.
// This looks backwards because minPulse/maxPulse are pulse length (us) inversly proportional to speed
float minPulse = 5000.0;
float maxPulse = 50000.0;

void setup()
{
    Serial.begin(115200);
    pinMode(phase1pin, OUTPUT);
    pinMode(phase2pin, OUTPUT);
    pinMode(phase3pin, OUTPUT);
    Serial.println("");
    Serial.println("setup() complete. \nEnter command. (e.g. 50 forward)");
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
    currentTime = micros();
    td = currentTime - p1start;
    refractory = 2.25 * holdTime;
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
    currentTime = micros();
    td = currentTime - p1start;
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
    currentTime = micros();
    td = currentTime - p1start;
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

void setupMotor(float inputPercent, String new_direction)
{
    // Converts the percent value into something usable
    // This looks backwards because minPulse/maxPulse are pulse length (us) inversly proportional to speed
    targetHoldTime = maxPulse - ((maxPulse - minPulse) * (inputPercent * 0.01));

    // See if the direction has changed
    if (new_direction != direction)
    {
        holdTime = targetHoldTime / 2.0;  // (20000) set to mid range and then adjust
        direction = new_direction;
        // Wait for it to stop, weird things can happen if it's still moving.
        delay(1500);  // (500)

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
        float inputPercent = Serial.readStringUntil(' ').toInt();

        // Serial.read(); //next character is space, so skip it using this
        String new_direction = Serial.readStringUntil('\n');
        Serial.printf("Setting: %.2f ", inputPercent);
        Serial.println(new_direction);
        setupMotor(inputPercent, new_direction);
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

        if (holdTime > targetHoldTime)
        {
            holdTime -= 0.025 * holdTime; // (=0.5)
        }
        else if (holdTime < targetHoldTime)
        {
            holdTime += 0.025; // (=0.25)
        }

        // Monitors the current speed
        status = -(holdTime - maxPulse) / ((maxPulse - minPulse) * (0.01));
        if (millis() % 1000 == 0)
        {
            /*
                  Serial.printf("status: %.3f\n", status);
                  Serial.printf("  holdTime: %.3f\n", holdTime);
                  Serial.printf("  targetHoldTime: %.3f\n", targetHoldTime);
                  Serial.printf("  td: %d\n", td);
            */
        }

        if (status == 100)
        {
            // setupMotor(100, "reverse");
        }
    }
}