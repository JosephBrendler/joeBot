#include <Arduino.h>
#include <Stepper.h>
#include <heltec.h>
#include "images.h"

// define pins -- careful to define for your board, ESP32, 8266, etc
#define encoder_a 18
#define encoder_b 19

#define phase1 2
#define phase2 5
#define phase3 26

#define stepsPerCycle 6
// initialize stepper motor and associated variables
Stepper myStepper(stepsPerCycle, phase1, phase2, phase3);

long loopcount = 0;
int newSpeed = 100, oldSpeed = newSpeed; // initial value
int speedIncrement = 1;                  // us
int speedLimit = 7200;                   // rpm

// initialize OLED display geometry
const int col_space = 40;
const int col0_x = 0,
          col1_x = col0_x + col_space,
          col2_x = col1_x + col_space,
          col3_x = col2_x + col_space,
          col4_x = col3_x + col_space;
const int line_0 = 0, line_1 = 10, line_2 = 20, line_3 = 30, line_4 = 40, line_5 = 50;

// initialize display text variables
String msg = "", old_msg = "";

// initialize timing variables
uint64_t newMicros = 0, oldMicros = 0;

// initialize interrupt and associated isr
struct InterruptLine
{
  const uint8_t PIN;
  uint32_t numHits;
  bool TRIGGERED;
};

InterruptLine photoTrigger = {encoder_a, 0, false};

void IRAM_ATTR trigger_isr()
{
  photoTrigger.numHits++;
  photoTrigger.TRIGGERED = true;
}

//--------- OLED display function declarations ------------
void update_display(String old_var1, String old_var2, String old_var3,
                    String old_var4, String old_var5, String old_var6,
                    String new_var1, String new_var2, String new_var3,
                    String new_var4, String new_var5, String new_var6);
void OLED_setup();

//-------------------[ setup() ]-------------------------------
void setup()
{
  OLED_setup();
  delay(500);
  Heltec.display->clear();
  Heltec.display->display();

  Serial.begin(115200);
  msg = "Starting setup";
  Serial.println(msg);
  Heltec.display->drawString(col0_x, line_1, msg);
  Heltec.display->display();

  msg = "Config Interrupt...";
  Heltec.display->drawString(col0_x, line_2, msg);
  Heltec.display->display();
  pinMode(photoTrigger.PIN, INPUT_PULLUP);
  attachInterrupt(photoTrigger.PIN, trigger_isr, FALLING);
  msg = "Config Interrupt: Done";
  Heltec.display->drawString(col0_x, line_2, msg);
  Heltec.display->display();
  Serial.println("Interrupt Initialized");

  delay(1000); // time to read

  myStepper.setSpeed(newSpeed);
  myStepper.step(stepsPerCycle);

  // set up persistent display
  Heltec.display->clear();
  Heltec.display->display();
  msg = "Direction: ";
  Heltec.display->drawString(col0_x, line_0, msg);
  msg = "Position: ";
  Heltec.display->drawString(col0_x, line_1, msg);
  msg = "Speed (rpm): ";
  Heltec.display->drawString(col0_x, line_2, msg);
  msg = "Speed (t/s): ";
  Heltec.display->drawString(col0_x, line_3, msg);
  msg = "stepLength: ";
  Heltec.display->drawString(col0_x, line_4, msg);
  msg = "loopcount: ";
  Heltec.display->drawString(col0_x, line_5, msg);
  // display initial placeholder data
  update_display("", "", String(oldSpeed),
                 "", "", "",
                 "", "", String(newSpeed),
                 "", "", "");
  Heltec.display->display();

  // move to a known start point
  digitalWrite(phase1, HIGH);
  digitalWrite(phase2, HIGH);
}

void loop()
{
  newMicros = micros();
  loopcount++;
  if (photoTrigger.TRIGGERED)
  {
    photoTrigger.TRIGGERED = false;
    if (photoTrigger.numHits % 4 == 0)
    {
      Serial.printf("Interrupt has fired %u times\n", photoTrigger.numHits);
      photoTrigger.numHits = 0;
      myStepper.step(stepsPerCycle);
    }
  }
  if (newMicros - oldMicros > 1000000) // 1 sec, 1000 ms
  {
    //  newPosition = encoder.getCount();
    //  newMicros = micros();
    //  delta_t = newMicros - oldMicros;
    //  oldMicros = newMicros;
    //  newMeasuredSpeed = (double)((newPosition - oldPosition) * 1000000 / (double)delta_t);
    speedIncrement = map(newSpeed, 0, speedLimit, 25, 1);
    if (newSpeed < speedLimit)
    {
      oldSpeed = newSpeed;
      newSpeed += speedIncrement;
      myStepper.setSpeed(newSpeed);
    }
    else
    {
      oldSpeed = newSpeed;
      newSpeed = speedLimit;
    }
    update_display("", "", String(oldSpeed),
                   "", "", "",
                   "", "", String(newSpeed),
                   "", "", "");
    oldMicros = newMicros;
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