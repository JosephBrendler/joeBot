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


    Note - currently 8266 only, using gpio.config() rather than esp32's gpio_config_t
   */
#include <Arduino.h>

#define phase1A D5
#define phase2A D6
#define phase3A D7
#define phase1B 0  // just physically tie these low for now
#define phase2B 0
#define phase3B 0

#define gearLED_0 D1 // gpio 5; pin 14
#define gearLED_1 D8 // gpio 0; pin 7
#define gearLED_2 D4 // gpio 2; pin 11

#define photoInterruptPin D2 // photoTrigger; gpio 4; pin 13

// define baseline starting and minimum driver signal pulse length
uint32_t stepLength = 40000;  // us (Bart: 40000) (this is the duration of delay in each stage of the 3-phase signal)
uint16_t minStepLength = 900; // us (Bart: 1400)
// driving 2 x disk platters, I couldn't go that fast until I switched to quatriture SRM (six stages, overlapping)
// then I got as fast as 900 somewhat stable in "5th gear" , but (POV patterns are "wobbly" -- switching back to 1400)

// define how much each cycle will reduce the driver signal pulse duration
const int firstGearSteps = 50;   // us (Bart: 5)
const int secondGearSteps = 300; // us (Bart: 300)
const int thirdGearSteps = 50;   // us (Bart: 50)
const int fourthGearSteps = 2;   // us (Bart: 2)
const int fifthGearSteps = 1;    // us (Bart: 2)

// define the threshold for when to shift gears (to steps above)
const int gear_12_threshold = 39950; // us
const int gear_23_threshold = 20000; // us
const int gear_34_threshold = 3000;  // us
const int gear_45_threshold = 1100;  // us
const int gear_56_threshold = 1000;  // us

const int delay_threshold = 16380; // us (above this use delay(ms), below use delayMicroseconds(us))

const int LED = LED_BUILTIN;

int steps = firstGearSteps;

bool secondgear = false;
bool thirdgear = false;
bool fourthgear = false;
bool fifthgear = false;
bool sixthgear = false;

uint64_t periodMicros = 21500; // HDD empirically measured

//--------- function declarations ------------
void myDelay(unsigned long stepMicroseconds);
void switchStep(int stage);
void adjustStepLength();

//---------- setup() -------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup");

  // Configure 3 x phase driver outputs
  // - easier with pinMode() and digitalWrite()
  //   because we only write one phase at a time in switchStep()
  Serial.print("Configure 3 x phase driver outputs... ");
  pinMode(phase1A, OUTPUT);
  pinMode(phase2A, OUTPUT);
  pinMode(phase3A, OUTPUT);
  Serial.print("==> Done\nSetting all phase drivers HIGH... ");
  // *** Caution: never write a phase A and B HIGH at the same time
  // (For now, always write something low then maybe the other its complement)
  digitalWrite(phase1A, LOW);
  digitalWrite(phase2A, LOW);
  digitalWrite(phase3A, LOW);  // B side of each phase is tied low for now
  // this should push the rotor to an equilibrium position between stator windings
  // (i.e. a known starting position)  digitalWrite(phase1A, HIGH);
  digitalWrite(phase1A, HIGH);
  digitalWrite(phase2A, HIGH);
  digitalWrite(phase3A, HIGH);
  Serial.println("==> Done");

  // Configure 3 x gearLED pins -
  Serial.print("Configure 3 x gearLED outputs... ");
  pinMode(gearLED_0, OUTPUT);
  pinMode(gearLED_1, OUTPUT);
  pinMode(gearLED_2, OUTPUT);
  // ToDo - convert to gpio.config() and write all 3 at once? /// 8266 doesn't support this

  digitalWrite(gearLED_0, HIGH);
  digitalWrite(gearLED_1, LOW);
  digitalWrite(gearLED_2, LOW);
  Serial.println("Done setup");
  Serial.println("Starting in first gear, stage 0");
  // GPIO.out_w1tc = (1 << phase1A) | (1 << phase2A); // LLH   /// 8266 doesn't support this
  digitalWrite(phase1A, LOW);
  digitalWrite(phase2A, LOW);
}

//--------------- loop() --------------------------
void loop()
{
  // these procedures take too long each to put an interrupt flag handler here
  // so put one in each switchStep(), adjustStepLength(), and controlStepLength()
  for (int i = 0; i <= 5; i++)
  {
    switchStep(i);
  }

  adjustStepLength();
}

//--------------- switchStep() --------------------------
void switchStep(int stage)
{
  switch (stage)
  {
  case 0:
    // LLH
    digitalWrite(phase2A, LOW);
    myDelay(stepLength);
    break;
  case 1:
    // HLH
    digitalWrite(phase1A, HIGH);
    myDelay(stepLength);
    break;
  case 2:
    // HLL
    digitalWrite(phase3A, LOW);
    myDelay(stepLength);
    break;
  case 3:
    // HHL
    digitalWrite(phase2A, HIGH);
    myDelay(stepLength);
    break;
  case 4:
    // LHL
    digitalWrite(phase1A, LOW);
    myDelay(stepLength);
    break;
  default:
    // LHH
    digitalWrite(phase3A, HIGH);
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
    stepLength = stepLength - steps; // steps is set by what gear you're in
  }
  else
  { // set minimum pulse length
    stepLength = minStepLength;
  }

  // if we've reached a gear threshold, shift gears and set new "steps"
  if (stepLength < gear_12_threshold)
  {
    if (!secondgear)
    {
      secondgear = true;
      digitalWrite(gearLED_0, LOW);
      digitalWrite(gearLED_1, HIGH);
      digitalWrite(gearLED_2, LOW);
      steps = secondGearSteps;
    }
  }

  if (stepLength < gear_23_threshold)
  {
    if (!thirdgear)
    {
      thirdgear = true;
      digitalWrite(gearLED_0, HIGH);
      digitalWrite(gearLED_1, HIGH);
      digitalWrite(gearLED_2, LOW);
      steps = thirdGearSteps;
    }
  }

  if (stepLength < gear_34_threshold)
  {
    if (!fourthgear)
    {
      fourthgear = true;
      digitalWrite(gearLED_0, LOW);
      digitalWrite(gearLED_1, LOW);
      digitalWrite(gearLED_2, HIGH);
      steps = fourthGearSteps;
    }
  }
  if (stepLength < gear_45_threshold)
  {
    if (!fifthgear)
    {
      fifthgear = true;
      digitalWrite(gearLED_0, HIGH);
      digitalWrite(gearLED_1, LOW);
      digitalWrite(gearLED_2, HIGH);
      steps = fifthGearSteps;
    }
  }
  if (stepLength < gear_56_threshold)
  {
    if (!sixthgear)
    {
      sixthgear = true;
      digitalWrite(gearLED_0, LOW);
      digitalWrite(gearLED_1, HIGH);
      digitalWrite(gearLED_2, HIGH);
      steps = fifthGearSteps;
    }
  }
}