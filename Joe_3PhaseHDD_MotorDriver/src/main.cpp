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
uint32_t stepLength = 40000;  // us (Bart: 40000) (this is the duration of delay in each stage of the 3-phase signal)
uint16_t minStepLength = 900; // us (Bart: 1400) (driving 2 x disk platters, I can't go that fast)

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

const int delay_threshold = 16380; // us (above this use delay(ms), below use delayMicroseconds(us))

const int LED = LED_BUILTIN;

int steps = firstGearSteps;

bool secondgear = false;
bool thirdgear = false;
bool fourthgear = false;
bool fifthgear = false;

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
  Serial.print("Configure 3 x phase driver outputs... ");
  pinMode(LED, OUTPUT);
  pinMode(phase1, OUTPUT);
  pinMode(phase2, OUTPUT);
  pinMode(phase3, OUTPUT);
  Serial.print("==> Done\nSetting all phase drivers HIGH... ");
  digitalWrite(phase1, HIGH);
  digitalWrite(phase2, HIGH);
  digitalWrite(phase3, HIGH);
  Serial.println("==> Done");

  /*  Serial.print("Configure 3 x phase driver outputs... ");
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = (1 << phase1) | (1 << phase2) | (1 << phase3);
    gpio_config(&io_conf);
    // set all phases HIGH (set output pins)
    Serial.print("==> Done\nSetting all phase drivers HIGH... ");
    GPIO.out_w1ts = (1 << phase1) | (1 << phase2) | (1 << phase3);
    Serial.println("==> Done");
  */

  digitalWrite(LED, LOW);
  Serial.println("Done setup");
  Serial.println("Starting in first gear, stage 0");
  //GPIO.out_w1tc = (1 << phase1) | (1 << phase2); // LLH
  digitalWrite(phase1, LOW);
  digitalWrite(phase2, LOW);
}

//--------------- loop() --------------------------
void loop()
{
  switchStep(0);
  switchStep(1);
  switchStep(2);
  switchStep(3);
  switchStep(4);
  switchStep(5);
  adjustStepLength();
}

//--------------- switchStep() --------------------------
void switchStep(int stage)
{
  switch (stage)
  {
  case 0:
    /*
      digitalWrite(phase1, LOW);
      digitalWrite(phase2, LOW);
      digitalWrite(phase3, HIGH);
      */
    //GPIO.out_w1tc = (1 << phase2); // LLH
    digitalWrite(phase2, LOW);
    myDelay(stepLength);
    break;
  case 1:
    /*    digitalWrite(phase1, HIGH);
        digitalWrite(phase2, LOW);
        digitalWrite(phase3, HIGH);
      */
    //GPIO.out_w1ts = (1 << phase1); // HLH
    digitalWrite(phase1, HIGH);
    myDelay(stepLength);
    break;

  case 2:
    /*    digitalWrite(phase1, HIGH);
        digitalWrite(phase2, LOW);
        digitalWrite(phase3, LOW);
      */
    //GPIO.out_w1tc = (1 << phase3); // HLL
    digitalWrite(phase3, LOW);
    myDelay(stepLength);
    break;

  case 3:
    /*    digitalWrite(phase1, HIGH);
        digitalWrite(phase2, HIGH);
        digitalWrite(phase3, LOW);
      */
    //GPIO.out_w1ts = (1 << phase2); // HHL
    digitalWrite(phase2, HIGH);
    myDelay(stepLength);
    break;

  case 4:
    /*    digitalWrite(phase1, LOW);
        digitalWrite(phase2, HIGH);
        digitalWrite(phase3, LOW);
      */
    //GPIO.out_w1tc = (1 << phase1); // LHL
    digitalWrite(phase1, LOW);
    myDelay(stepLength);
    break;

  default:
    /*    digitalWrite(phase1, LOW);
        digitalWrite(phase2, HIGH);
        digitalWrite(phase3, HIGH);
      */
    //GPIO.out_w1ts = (1 << phase3); // LHH
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
  if (stepLength < gear_45_threshold)
  {
    if (!fifthgear)
    {
      fifthgear = true;
      Serial.println("fifth gear");
      digitalWrite(LED, HIGH);
      steps = fifthGearSteps;
    }
  }
}