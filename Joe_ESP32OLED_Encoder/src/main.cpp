/*----------------------
 Encoder example w OLED
   Joe Brendler 19 Dec 2022
   OLED code from Heltec - https://github.com/HelTecAutomation/Heltec_ESP32
-----------------------*/
#include <Arduino.h>
#include <heltec.h>
#include "images.h"

#define phase1A 17 // consecutive bits can be written easily with three bits shifted to the LSB position (0b111<<phase1A)
#define phase2A 18
#define phase3A 19

// config gpios
gpio_config_t io_conf;

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
const int fifthGearSteps = 2;    // us (Bart: n/a)
const int sixthGearSteps = 2;    // us (Bart: n/a)

// define the threshold for when to shift gears (to steps above)
const int gear_12_threshold = 39950;         // us (old: 39950)
const int gear_23_threshold = 18000;         // us (20000)
const int gear_34_threshold = 3000;          // us (3000)
const int gear_45_threshold = 1100;          // us (1100)
const int gear_56_threshold = minStepLength; // us (n/a)

const int delay_threshold = 16380; // us (above this use delay(ms), below use delayMicroseconds(us))

int steps = firstGearSteps;

bool secondgear = false;
bool thirdgear = false;
bool fourthgear = false;
bool fifthgear = false;
bool sixthgear = false;

uint64_t periodMicros = 21500; // HDD empirically measured

const int gear_y = 10, gear_x = 76, stepLength_y = 20, stepLength_x = 76;
const int line_0 = 0, line_1 = 10, line_2 = 20, line_3 = 30, line_4 = 40;

String msg = "", gear_str = "", stepLength_str = "";
String old_gear_str = "", old_stepLength_str = "";

unsigned long now = 0, old_millis = 0; // loop timing

//--------- function declarations ------------
void myDelay(unsigned long stepMicroseconds);
void switchStep(int stage);
void adjustStepLength();

//---------- setup() -------------------------
void setup()
{
  Serial.begin(115200);
  msg = "Starting setup";
  Serial.println(msg);

  // Initialize the Heltec ESP32 object
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  Heltec.display->setColor(WHITE);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setContrast(255);
  Heltec.display->display();
  delay(250);
  Heltec.display->clear();
  Heltec.display->drawXbm(0, 5, logo_width, logo_height, (const unsigned char *)logo_bits);
  Heltec.display->display();
  delay(250);
  for (int i = 0; i <= 3; i++)
  {
    Heltec.display->invertDisplay();
    delay(250);
    Heltec.display->normalDisplay();
    delay(250);
  }
  msg = "OLED initialized";
  Serial.println(msg);
  Heltec.display->clear();
  Heltec.display->drawString(0, line_0, msg);
  Heltec.display->display();

  // Configure 3 x phase driver outputs
  // - easier with pinMode() and digitalWrite()
  //   because we only write one phase at a time in switchStep()
  msg = "Config 3 phases...";
  Serial.print(msg);
  Heltec.display->clear();
  Heltec.display->drawString(0, line_1, msg);
  Heltec.display->display();
  io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
  io_conf.mode = GPIO_MODE_OUTPUT;
  io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
  io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
  // io_conf.pin_bit_mask = (1 << phase1A) | (1 << phase2A) | (1 << phase3A);   // or, since these are consecutive register bits
  io_conf.pin_bit_mask = (0b111 << phase1A); // or, since phase1A = 25,
  // io_conf.pin_bit_mask = (0b1110000000000000000000000000);
  gpio_config(&io_conf);
  msg = "Config 3 phases: Done";
  Heltec.display->drawString(0, line_1, msg);
  msg = "All phases HIGH...";
  Heltec.display->drawString(0, line_2, msg);
  Heltec.display->display();

  Serial.printf("==> Done\n%s", msg);
  // *** Caution: never write a phase A and B HIGH at the same time
  // (For now, always write something low then maybe the other its complement)

  // B side of each phase is tied low for now
  //  GPIO.w1tc = (1 << phase1A) | (1 << phase2A) | (1 << phase3A); // or
  GPIO.out_w1tc = (0b111 << phase1A);
  // this should push the rotor to an equilibrium position between stator windings
  // (i.e. a known starting position)  digitalWrite(phase1A, HIGH);
  // GPIO.w1ts = (1 << phase1A) | (1 << phase2A) | (1 << phase3A); // or
  GPIO.out_w1ts = (0b111 << phase1A);
  Serial.println("==> Done");
  msg = "All phases HIGH: Done";
  Heltec.display->drawString(0, line_2, msg);
  Heltec.display->display();

  msg = "Done setup";
  Serial.println(msg);
  Heltec.display->drawString(0, line_3, msg);
  msg = "First gear";
  Serial.println(msg);
  Heltec.display->drawString(0, line_4, msg);
  Heltec.display->display();
  delay(2000);

  gear_str = "1";
  stepLength_str = String(stepLength);
  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->drawString(10, gear_y, "Gear: ");
  Heltec.display->drawString(10, stepLength_y, "stepLength: ");
  Heltec.display->drawString(gear_x, gear_y, gear_str);
  Heltec.display->drawString(stepLength_x, stepLength_y, stepLength_str);

  Heltec.display->display();

  GPIO.out_w1tc = (1 << phase1A) | (1 << phase2A); // LLH   /// 8266 doesn't support this
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
  // update display ea 1/2 sec

  now = millis();
  if (now - old_millis > 500)
  {
    // blank the old data, then write calculate and write new data
    old_millis = now;
    old_stepLength_str = stepLength_str;
    stepLength_str = String(stepLength);
    Heltec.display->setColor(BLACK);
    Heltec.display->drawString(gear_x, gear_y, old_gear_str);
    Heltec.display->drawString(stepLength_x, stepLength_y, old_stepLength_str);
    Heltec.display->display();
    Heltec.display->setColor(WHITE);
    Heltec.display->drawString(gear_x, gear_y, gear_str);
    Heltec.display->drawString(stepLength_x, stepLength_y, stepLength_str);
    Heltec.display->display();
  }
}

//--------------- switchStep() --------------------------
void switchStep(int stage)
{

  switch (stage)
  {
  case 0:
    // LLH
    GPIO.out_w1tc = (1 << phase2A);
    // digitalWrite(phase2A, LOW);
    myDelay(stepLength);
    break;
  case 1:
    // HLH
    GPIO.out_w1ts = (1 << phase1A);
    // digitalWrite(phase1A, HIGH);
    myDelay(stepLength);
    break;
  case 2:
    // HLL
    GPIO.out_w1tc = (1 << phase3A);
    // digitalWrite(phase3A, LOW);
    myDelay(stepLength);
    break;
  case 3:
    // HHL
    GPIO.out_w1ts = (1 << phase2A);
    // digitalWrite(phase2A, HIGH);
    myDelay(stepLength);
    break;
  case 4:
    // LHL
    GPIO.out_w1tc = (1 << phase1A);
    // digitalWrite(phase1A, LOW);
    myDelay(stepLength);
    break;
  default:
    // LHH
    GPIO.out_w1ts = (1 << phase3A);
    // digitalWrite(phase3A, HIGH);
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
  old_stepLength_str = stepLength_str;
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
      old_gear_str = "1";
      gear_str = "2";
      steps = secondGearSteps;
    }
  }

  if (stepLength < gear_23_threshold)
  {
    if (!thirdgear)
    {
      thirdgear = true;
      old_gear_str = "2";
      gear_str = "3";
      steps = thirdGearSteps;
    }
  }

  if (stepLength < gear_34_threshold)
  {
    if (!fourthgear)
    {
      fourthgear = true;
      old_gear_str = "3";
      gear_str = "4";
      steps = fourthGearSteps;
    }
  }
  if (stepLength < gear_45_threshold)
  {
    if (!fifthgear)
    {
      fifthgear = true;
      old_gear_str = "4";
      gear_str = "5";
      steps = fifthGearSteps;
    }
  }
  if (stepLength < gear_56_threshold)
  {
    if (!sixthgear)
    {
      sixthgear = true;
      old_gear_str = "5";
      gear_str = "6";
      steps = sixthGearSteps;
    }
  }
}