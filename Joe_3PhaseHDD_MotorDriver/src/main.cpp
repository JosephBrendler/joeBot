/* 3-phase motor driver
   Joe Brendler 20 Nov 2022
   Initially based on tuturial by Bart Venneker youtube.com/watch?v=CMz2DYpos8w
   Modifications -
   - added ESP32 option - different pins
   - moved steplength adjustment to function (simplifing loop)
   - started slower, lengthened transitions from gear 1-2 and 3-4

   Basic idea - start with long 3-phase pulse signal, and gradually accelerate by reducing the pulse-lenth (stepLength)
   ToDo -
     + shift to direct register manipulation of consecutive gpios
     + shift to hardware timer interrupts instead of delays in loop function
       -- timer0 = stage duration (stepLentgh)
       -- isr() = stage++; STEP=true; (flag to switchstep(stage) and set STEP=false)
     + shift to
     add PID control
   */
#include <Arduino.h>

//#define ESP32 //otherwise 8266

#ifdef ESP32
#define phase1 4  // gpio 4; pin 26
#define phase2 5  // gpio 5; pin 29
#define phase3 16 // gpio 16; pin 27
#else
#define phase1 D5 // gpio 14; pin 4
#define phase2 D6 // gpio 12; pin 5
#define phase3 D7 // gpio 13; pin 6
#endif

// define baseline starting and minimum driver signal pulse length
uint32_t stepLength = 50000;   // us (Bart: 40000) (this is the duration of delay in each stage of the 3-phase signal)
uint16_t minStepLength = 4000; // us (Bart: 1400) (driving 2 x disk platters, I can't go that fast)

// define how much each cycle will reduce the driver signal pulse duration
const int firstGearSteps = 50;   // us (Bart: 5)
const int secondGearSteps = 300; // us (Bart: 300)
const int thirdGearSteps = 50;   // us (Bart: 50)
const int fourthGearSteps = 2;   // us (Bart: 2)

// define the threshold for when to shift gears (to steps above)
const int gear_12_threshold = 39950; // us
const int gear_23_threshold = 20000; // us
const int gear_34_threshold = 3000;  // us

const int delay_threshold = 16380; // us (above this use delay(ms), below use delayMicroseconds(us))

const int LED = LED_BUILTIN;

int steps = firstGearSteps;

bool secondgear = false;
bool thirdgear = false;
bool fourthgear = false;

//--------- function declarations ------------
void myDelay(unsigned long stepMicroseconds);
void switchStep(int stage);
void adjustStepLength();

//---------- setup() -------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup");

  pinMode(LED, OUTPUT);
  pinMode(phase1, OUTPUT);
  pinMode(phase2, OUTPUT);
  pinMode(phase3, OUTPUT);

  digitalWrite(phase1, HIGH);
  digitalWrite(phase2, HIGH);
  digitalWrite(phase3, HIGH);

  digitalWrite(LED, LOW);
  Serial.println("Done setup");
  Serial.println("Starting in first gear");
}

//--------------- loop() --------------------------
void loop()
{
  switchStep(1);
  switchStep(2);
  switchStep(3);
  adjustStepLength();
}

//--------------- switchStep() --------------------------
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

//--------------- myDelay() --------------------------
void myDelay(unsigned long stepMicroseconds)
{
  if (stepMicroseconds > delay_threshold)
  {
    delay(stepMicroseconds / 1000);
  }
  else
  {
    delayMicroseconds(stepMicroseconds);
  }
}

//--------------- adjustStepLength() --------------------------
void adjustStepLength()
{
  // adjust stepLength by stepSize
  if (stepLength > minStepLength)
  {
    stepLength = stepLength - steps;
  }
  else
  { // set minimum pulse length
    stepLength = minStepLength;
  }

  if (stepLength < gear_12_threshold)
  {
    if (!secondgear)
    {
      secondgear = true;
      Serial.println("second gear");
      digitalWrite(LED, HIGH);
      steps = secondGearSteps;
    }
  }

  if (stepLength < gear_23_threshold)
  {
    if (!thirdgear)
    {
      thirdgear = true;
      Serial.println("third gear");
      digitalWrite(LED, LOW);
      steps = thirdGearSteps;
    }
  }

  if (stepLength < gear_34_threshold)
  {
    if (!fourthgear)
    {
      fourthgear = true;
      Serial.println("fourth gear");
      digitalWrite(LED, HIGH);
      steps = fourthGearSteps;
    }
  }
}