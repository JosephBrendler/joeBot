#include <Arduino.h>

/*
#define phase1 4  // gpio 4; pin 26
#define phase2 5  // gpio 5; pin 29
#define phase3 16 // gpio 16; pin 27
*/
const int phase1 = D5;
const int phase2 = D6;
const int phase3 = D7;

uint32_t stepLength = 40000;   // ms
uint16_t minStepLength = 1400; // ms
int steps = 5;

const int LED = LED_BUILTIN;

//--------- function declarations ------------
void myDelay(unsigned long p);
void switchStep(int stage);

void setup()
{
  pinMode(LED, OUTPUT);
  pinMode(phase1, OUTPUT);
  pinMode(phase2, OUTPUT);
  pinMode(phase3, OUTPUT);

  digitalWrite(phase1, HIGH);
  digitalWrite(phase2, HIGH);
  digitalWrite(phase3, HIGH);

  digitalWrite(LED, LOW);
}

void loop()
{
  switchStep(1);
  switchStep(2);
  switchStep(3);

  if (stepLength > minStepLength)
  {
    stepLength = stepLength - steps;
  }
  else
  { // set minimum pulse length
    stepLength = minStepLength;
  }

  if (stepLength < 39950)
  {
    // second gear
    digitalWrite(LED, HIGH);
    steps = 300;
  }

  if (stepLength < 20000)
  {
    // third gear
    digitalWrite(LED, LOW);
    steps = 50;
  }

  if (stepLength < 3000)
  {
    // fourth gear
    digitalWrite(LED, HIGH);
    steps = 2;
  }
}

void switchStep(int stage)
{
  switch (stage)
  {
  case 1:
    digitalWrite(phase1, HIGH);
    digitalWrite(phase2, LOW);
    digitalWrite(phase3, LOW);
    myDelay(stepLength);
    break;

  case 2:
    digitalWrite(phase1, LOW);
    digitalWrite(phase2, HIGH);
    digitalWrite(phase3, LOW);
    myDelay(stepLength);
    break;

  default:
    digitalWrite(phase1, LOW);
    digitalWrite(phase2, LOW);
    digitalWrite(phase3, HIGH);
    myDelay(stepLength);
    break;
  }
}

void myDelay(unsigned long p)
{
  if (p > 16380)
  {
    delay(p / 1000);
  }
  else
  {
    delayMicroseconds(p);
  }
}
