/* 3-phase motor driver and encoder with OLED display
   Joe Brendler 20 Dec 2022

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
#include <heltec.h>
#include "images.h"

#include <ESP32Encoder.h>

// define pins -- careful to define for your board, ESP32, 8266, etc
#define encoder_a 18
#define encoder_b 19

#define phase1 2
#define phase2 5
#define phase3 26

#define gearLED_0 17
#define gearLED_1 22
#define gearLED_2 27

// initialize encoder and associated variables
ESP32Encoder encoder;

long newPosition = 0, oldPosition = newPosition;
double newSpeed = 0, oldSpeed = 0;
String newDirection = "ccw", oldDirection = newDirection;
unsigned long newMicros = 0, oldMicros = 0; // loop timing
unsigned long delta_t = 0;                  // in usec

// initialize OLED display geometry
const int col_space = 30;
const int col0_x = 0,
          col1_x = col0_x + col_space,
          col2_x = col1_x + col_space,
          col3_x = col2_x + col_space,
          col4_x = col3_x + col_space;
const int line_0 = 0, line_1 = 10, line_2 = 20, line_3 = 30, line_4 = 40, line_5 = 50;

// initialize display text variables
String msg = "", old_msg = "";

//--------- OLED display function declarations ------------
void update_display(String old_var1, String old_var2, String old_var3,
                    String old_var4, String old_var5, String old_var6,
                    String new_var1, String new_var2, String new_var3,
                    String new_var4, String new_var5, String new_var6);
void OLED_setup();

// define baseline starting and minimum driver signal pulse length
uint64_t stepLength = 40000; // us (Bart: 40000) (this is the duration of delay in each stage of the 3-phase signal)
uint64_t oldStepLength = stepLength;
uint64_t minStepLength = 900; // us (Bart: 1400)
double period = 0.0, oldPeriod = period;

// driving 2 x disk platters, I couldn't go that fast until I switched to quatriture SRM (six stages, overlapping)
// then I got as fast as 900 somewhat stable in "5th gear" , but (POV patterns are "wobbly" -- switching back to 1400)

// define how much each cycle will reduce the driver signal pulse duration
const int accelerationFactor = 300; // us (faster than current period)(700 w 5v)
const int firstGearSteps = 50;    // us (Bart: 5)
const int secondGearSteps = 300;  // us (Bart: 300)
const int thirdGearSteps = 50;    // us (Bart: 50)
const int fourthGearSteps = 2;    // us (Bart: 2)
const int fifthGearSteps = 1;     // us (Bart: 2)

// define the threshold for when to shift gears (to steps above)
const int gear_12_threshold = 39950; // us
const int gear_23_threshold = 20000; // us
const int gear_34_threshold = 3000;  // us
const int gear_45_threshold = 1100;  // us

const int delay_threshold = 16380; // us (above this use delay(ms), below use delayMicroseconds(us))

int steps = firstGearSteps;

bool secondgear = false;
bool thirdgear = false;
bool fourthgear = false;
bool fifthgear = false;

int gear = 0, oldGear = gear;

//--------- function declarations ------------
void myDelay(unsigned long stepMicroseconds);
void switchStep(int stage);
void adjustStepLength();

//---------- setup() -------------------------
void setup()
{
  // Initialize the Heltec ESP32 object
  OLED_setup(); // leaves display w "OLED Initialized" on line_0
  delay(500);
  Heltec.display->clear();
  Heltec.display->display();

  Serial.begin(115200);
  msg = "Starting setup";
  Serial.println(msg);
  Heltec.display->drawString(col0_x, line_1, msg);
  Heltec.display->display();

  msg = "Initializing Encoder...";
  Heltec.display->drawString(col0_x, line_2, msg);
  Heltec.display->display();
  // Initialize encoder
  // use pin ___, ___ for the encoder
  // encoder.attachHalfQuad(encoder_a, encoder_b);
  encoder.attachFullQuad(encoder_a, encoder_b);
  // set starting count value after attaching
  // encoder.setCount(37);
  // clear the encoder's raw count and set the tracked count to zero
  encoder.clearCount();
  Serial.println("Encoder Start = " + String((int32_t)encoder.getCount()));
  msg = "Initializing Encoder: Done";
  Heltec.display->drawString(col0_x, line_2, msg);
  Heltec.display->display();
  Serial.println("Encoder Initialized");

  delay(1000); // time to read

  // Configure 3 x phase driver outputs
  msg = "Config outputs...";
  Serial.print(msg);
  Heltec.display->drawString(col0_x, line_3, msg);
  Heltec.display->display();
  pinMode(gearLED_0, OUTPUT);
  pinMode(gearLED_1, OUTPUT);
  pinMode(gearLED_2, OUTPUT);
  pinMode(phase1, OUTPUT);
  pinMode(phase2, OUTPUT);
  pinMode(phase3, OUTPUT);

  digitalWrite(phase1, HIGH);
  digitalWrite(phase2, HIGH);
  digitalWrite(phase3, HIGH);

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

  digitalWrite(gearLED_0, HIGH);
  digitalWrite(gearLED_1, LOW);
  digitalWrite(gearLED_2, LOW);
  Serial.println("Done");
  msg = "Config outputs: Done";
  Heltec.display->drawString(col0_x, line_3, msg);
  Heltec.display->display();

  Serial.println("Done setup");
  Serial.println("Starting in first gear, stage 0");
  msg = "Done Setup";
  Serial.print(msg);
  Heltec.display->drawString(col0_x, line_4, msg);
  Heltec.display->display();

  msg = "Starting motor";
  Serial.print(msg);
  Heltec.display->drawString(col0_x, line_5, msg);
  Heltec.display->display();
  delay(1000);

  gear = 1;
  // GPIO.out_w1tc = (1 << phase1) | (1 << phase2); // LLH
  digitalWrite(phase1, LOW);
  digitalWrite(phase2, LOW);

  // set up persistent display
  Heltec.display->clear();
  Heltec.display->display();
  msg = "Direction: ";
  Heltec.display->drawString(col0_x, line_0, msg);
  msg = "Position: ";
  Heltec.display->drawString(col0_x, line_1, msg);
  msg = "Speed: ";
  Heltec.display->drawString(col0_x, line_2, msg);
  msg = "Gear: ";
  Heltec.display->drawString(col0_x, line_3, msg);
  msg = "stepLength: ";
  Heltec.display->drawString(col0_x, line_4, msg);
  msg = "period: ";
  Heltec.display->drawString(col0_x, line_5, msg);
  // display initial placeholder data
  update_display(oldDirection, String(oldPosition), String(oldSpeed),
                 String(oldGear), String(oldStepLength), String(oldPeriod, 6),
                 newDirection, String(newPosition), String(newSpeed),
                 String(gear), String(stepLength), String(period, 6));
  Heltec.display->display();
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

  // periodically update display with encoder data
  newMicros = micros();
  delta_t = newMicros - oldMicros;
  if (delta_t > 100) // update display periodically
  {
    // update position
    newPosition = encoder.getCount();
    // delay(1000);

    if (newPosition > oldPosition)
      newDirection = "cw";
    else
      newDirection = "ccw";
    // update speed = ticks per second
    newSpeed = (double)((newPosition - oldPosition) * 1000000 / (double)delta_t);

    // update period (60 ticks per revolution) in seconds
    // for now, negate period since rotation is ccw
    period = (-60.0 / newSpeed);

    // Serial.printf("newDirection: %s\n", newDirection);
    // Serial.printf("newPosition: %d\n", newPosition);
    // Serial.printf("newSpeed: %.15f\n", newSpeed);
    update_display(oldDirection, String(oldPosition), String(oldSpeed),
                   String(oldGear), String(oldStepLength), String(oldPeriod, 6),
                   newDirection, String(newPosition), String(newSpeed),
                   String(gear), String(stepLength), String(period, 6));

    oldMicros = newMicros;
    oldPosition = newPosition;
    oldSpeed = newSpeed;
    oldDirection = newDirection;
    oldGear = gear;
    oldStepLength = stepLength;
    oldPeriod = period;
  }
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
    // GPIO.out_w1tc = (1 << phase2); // LLH
    digitalWrite(phase2, LOW);
    myDelay(stepLength);
    break;
  case 1:
    /*    digitalWrite(phase1, HIGH);
        digitalWrite(phase2, LOW);
        digitalWrite(phase3, HIGH);
      */
    // GPIO.out_w1ts = (1 << phase1); // HLH
    digitalWrite(phase1, HIGH);
    myDelay(stepLength);
    break;

  case 2:
    /*    digitalWrite(phase1, HIGH);
        digitalWrite(phase2, LOW);
        digitalWrite(phase3, LOW);
      */
    // GPIO.out_w1tc = (1 << phase3); // HLL
    digitalWrite(phase3, LOW);
    myDelay(stepLength);
    break;

  case 3:
    /*    digitalWrite(phase1, HIGH);
        digitalWrite(phase2, HIGH);
        digitalWrite(phase3, LOW);
      */
    // GPIO.out_w1ts = (1 << phase2); // HHL
    digitalWrite(phase2, HIGH);
    myDelay(stepLength);
    break;

  case 4:
    /*    digitalWrite(phase1, LOW);
        digitalWrite(phase2, HIGH);
        digitalWrite(phase3, LOW);
      */
    // GPIO.out_w1tc = (1 << phase1); // LHL
    digitalWrite(phase1, LOW);
    myDelay(stepLength);
    break;

  default:
    /*    digitalWrite(phase1, LOW);
        digitalWrite(phase2, HIGH);
        digitalWrite(phase3, HIGH);
      */
    // GPIO.out_w1ts = (1 << phase3); // LHH
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
    if (gear < 5)
    {
      stepLength = stepLength - steps;
    }
    else
    {
      // estimate necessary steplength (usec)
      // 20 cycles of 6 steps per revolution; convert to us; adjust to accelerate
      stepLength = (uint64_t)((period * 1000000.0) / (20.0 * 6.0)) - accelerationFactor;
    }
  }
  else
  { // set minimum pulse length
    stepLength = minStepLength;
  }

  if (stepLength < gear_12_threshold)
  {
    if (!secondgear)
    {
      gear = 2;
      secondgear = true;
      // Serial.println("second gear");
      // Serial.printf("stepLength: %d, steps: %d\n", stepLength, steps);
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
      gear = 3;
      thirdgear = true;
      // Serial.println("third gear");
      // Serial.printf("stepLength: %d, steps: %d\n", stepLength, steps);
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
      gear = 4;
      fourthgear = true;
      // Serial.println("fourth gear");
      // Serial.printf("stepLength: %d, steps: %d\n", stepLength, steps);
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
      gear = 5;
      fifthgear = true;
      // Serial.println("fifth gear");
      // Serial.printf("stepLength: %d, steps: %d\n", stepLength, steps);
      digitalWrite(gearLED_0, HIGH);
      digitalWrite(gearLED_1, LOW);
      digitalWrite(gearLED_2, HIGH);
      steps = fifthGearSteps;
    }
  }
}

//---------------------[ functions ]------------------------------------------------------------
void OLED_setup()
{
  // Initialize the Heltec ESP32 object - uses joeBot logo at images.h
  Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
  delay(500); // time to read the organic "initialized" message
  Heltec.display->setColor(WHITE);
  Heltec.display->setFont(ArialMT_Plain_10);
  Heltec.display->setContrast(255);
  Heltec.display->clear();
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
  Heltec.display->drawString(col0_x, line_0, msg);
  Heltec.display->display();
}

void update_display(String old_var1, String old_var2, String old_var3,
                    String old_var4, String old_var5, String old_var6,
                    String new_var1, String new_var2, String new_var3,
                    String new_var4, String new_var5, String new_var6)
{
  // blank out the old data
  Heltec.display->setColor(BLACK);
  Heltec.display->drawString(col2_x, line_0, old_var1);
  Heltec.display->drawString(col2_x, line_1, old_var2);
  Heltec.display->drawString(col2_x, line_2, old_var3);
  Heltec.display->drawString(col2_x, line_3, old_var4);
  Heltec.display->drawString(col2_x, line_4, old_var5);
  Heltec.display->drawString(col2_x, line_5, old_var6);

  Heltec.display->display();

  // write the new data
  Heltec.display->setColor(WHITE);
  Heltec.display->drawString(col2_x, line_0, new_var1);
  Heltec.display->drawString(col2_x, line_1, new_var2);
  Heltec.display->drawString(col2_x, line_2, new_var3);
  Heltec.display->drawString(col2_x, line_3, new_var4);
  Heltec.display->drawString(col2_x, line_4, new_var5);
  Heltec.display->drawString(col2_x, line_5, new_var6);
  Heltec.display->display();
}