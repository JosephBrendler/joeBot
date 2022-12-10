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

#define phase1 D5    // gpio 14; pin 4
#define phase2 D6    // gpio 12; pin 5
#define phase3 D7    // gpio 13; pin 6
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
bool calibrated = false;

int calibrationCount = 0;

uint64_t periodMicros = 21500; // HDD empirically measured
uint64_t lastTrigger = 0;
uint64_t thisTrigger = 0;
uint64_t delta = 0;

uint64_t periodSum = 0;
uint64_t periodAvg = 0;

// -------- define and initialize interrupt line structs -----------
struct photoInterruptLine
{
  const uint8_t PIN;         // gpio pin number
  volatile uint32_t numHits; // number of times fired
  volatile bool TRIGGERED;   // boolean logical; is triggered now
};
photoInterruptLine photoTrigger = {photoInterruptPin, 0, false};

//--------- function declarations ------------
void myDelay(unsigned long stepMicroseconds);
void switchStep(int stage);
void adjustStepLength();
void updateTiming();
void controlStepLength();
void calibrate();

// --------------- interrupt function declarations ---------------
/*------------------------------------------------------------------------------
   trigger() -- photo-trigger interrupt
  ------------------------------------------------------------------------------*/
void IRAM_ATTR trigger()
{
  photoTrigger.TRIGGERED = true;
}

//---------- setup() -------------------------
void setup()
{
  Serial.begin(115200);
  Serial.println("Starting setup");

  // Configure 3 x phase driver outputs
  // - easier with pinMode() and digitalWrite()
  //   because we only write one phase at a time in switchStep()
  Serial.print("Configure 3 x phase driver outputs... ");
  pinMode(phase1, OUTPUT);
  pinMode(phase2, OUTPUT);
  pinMode(phase3, OUTPUT);
  Serial.print("==> Done\nSetting all phase drivers HIGH... ");
  // this should push the rotor to an equilibrium position between stator windings
  // (i.e. a known starting position)
  digitalWrite(phase1, HIGH);
  digitalWrite(phase2, HIGH);
  digitalWrite(phase3, HIGH);
  Serial.println("==> Done");

  // Configure photo trigger interrupt pin
  Serial.print("Configure photo trigger interrupt pin... ");
  pinMode(photoTrigger.PIN, INPUT_PULLUP);
  Serial.println("==> Done");
  Serial.print("Attach photo trigger interrupt... ");
  attachInterrupt(photoTrigger.PIN, trigger, FALLING);
  Serial.println("==> Done");

  // start timing
  updateTiming();

  // Configure 3 x gearLED pins -
  Serial.print("Configure 3 x gearLED outputs... ");
  pinMode(gearLED_0, OUTPUT);
  pinMode(gearLED_1, OUTPUT);
  pinMode(gearLED_2, OUTPUT);
  // ToDo - convert to gpio.config() and write all 3 at once?
  /*    gpio.config
          io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  io_conf.pin_bit_mask = (1 << gearLED_0) | (1 << gearLED_1) | (1 << gearLED_2);
  gpio_config(&io_conf);
  // set all phases HIGH (set output pins)
  Serial.print("==> Done\nSetting all phase drivers HIGH... ");
  GPIO.out_w1ts = (1 << gearLED_0) | (1 << gearLED_1) | (1 << gearLED_2);
  Serial.println("==> Done");
  */
  digitalWrite(gearLED_0, HIGH);
  digitalWrite(gearLED_1, LOW);
  digitalWrite(gearLED_2, LOW);
  Serial.println("Done setup");
  Serial.println("Starting in first gear, stage 0");
  // GPIO.out_w1tc = (1 << phase1) | (1 << phase2); // LLH
  digitalWrite(phase1, LOW);
  digitalWrite(phase2, LOW);
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
  if (sixthgear)
  {
    // if (calibrated)
    if (false)
    {
      controlStepLength();
    }
    else
    {
      adjustStepLength();
    }
  }
  else
  {
    adjustStepLength();
  }
}

void updateTiming()
{
  thisTrigger = micros();
  delta = thisTrigger - lastTrigger;
  lastTrigger = thisTrigger;
  if (((periodMicros > delta) && (periodMicros - delta) < 1000) ||
      ((periodMicros < delta) && (delta - periodMicros) < 1000))
  {
    periodMicros = delta;
  }
  // calibrate to the average period in sixth gear, then control to that
  if (sixthgear && !calibrated)
  {
    if (calibrationCount++ < 1000)
    {
      periodSum += periodMicros;
    }
    else
    {
      periodSum += periodMicros;
      periodAvg = (uint64_t)(periodSum / 1000.0);
      calibrated = true;
    }
  }
}
//--------------- switchStep() --------------------------
void switchStep(int stage)
{
  // put interrupt flag handler here, note each case: below has a delay of about 1/8 of the period
  if (photoTrigger.TRIGGERED)
  {
    updateTiming();
    photoTrigger.TRIGGERED = false;
  }
  switch (stage)
  {
  case 0:
    // LLH
    digitalWrite(phase2, LOW);
    myDelay(stepLength);
    break;
  case 1:
    // HLH
    digitalWrite(phase1, HIGH);
    myDelay(stepLength);
    break;
  case 2:
    // HLL
    digitalWrite(phase3, LOW);
    myDelay(stepLength);
    break;
  case 3:
    // HHL
    digitalWrite(phase2, HIGH);
    myDelay(stepLength);
    break;
  case 4:
    // LHL
    digitalWrite(phase1, LOW);
    myDelay(stepLength);
    break;
  default:
    // LHH
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
  // put interrupt flag handler here
  // the rammp-up controller code below is fast but executed once every loop until at stable controlled speed
  if (photoTrigger.TRIGGERED)
  {
    updateTiming();
    photoTrigger.TRIGGERED = false;
  }
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

void controlStepLength()
{
  if (photoTrigger.TRIGGERED)
  {
    updateTiming();
    photoTrigger.TRIGGERED = false;
  }

  if (periodMicros > periodAvg)
  {
    stepLength = (uint32_t)(periodMicros / float(24.05));
  }
  else
  {
    stepLength = (uint32_t)(periodAvg / float(24));
  }
}