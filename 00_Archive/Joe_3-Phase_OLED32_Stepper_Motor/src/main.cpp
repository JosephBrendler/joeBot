#include <Arduino.h>
#include "images.h"
#include "heltec.h"
#include "Stepper.h"

#define pot A0
#define pushbutton 37

const int stepsPerRevolution = 20; // steps
long speed = 50;                   // analog input reading (in rpm)
String msg = "";                   // for display
String direction_msg, speed_msg, old_direction_msg, old_speed_msg;

const int line_0 = 0, line_1 = 10, line_2 = 20, line_3 = 30, line_4 = 40; // OLED screen positions
const int direction_x = 50, speed_x = 50;

struct buttonInterruptLine
{
  const uint8_t PIN;         // gpio pin number
  volatile uint32_t numHits; // number of times fired
  volatile bool PRESSED;     // boolean logical; is triggered now
};
buttonInterruptLine buttonPress = {pushbutton, 0, false};

// initialize the three-phase stepper library on GPIO
Stepper myStepper(stepsPerRevolution, 17, 18, 19);

//---------------------------[ function declarations ]---------------------------------------------
void update_display(String old_dir, String old_spd, String new_dir, String new_spd);
void OLED_setup();

/*------------------------------------------------------------------------------
   handle_button_interrupt() - set flag and call stepper interrupt method
  ------------------------------------------------------------------------------*/
void IRAM_ATTR handle_button_interrupt()
{
  buttonPress.PRESSED = true;
  myStepper.interrupt(); // method sets a flag
}

void setup()
{
  // initialize the serial port:
  Serial.begin(115200);

  // initialize analog input
  pinMode(pot, INPUT);

  // start up OLED display
  OLED_setup();

  // Configure function pushbutton interrupt pin
  Serial.print("config pushbutton");
  msg = "config pushbutton...";
  Serial.print(msg);
  Heltec.display->drawString(0, line_2, msg);
  Heltec.display->display();
  pinMode(buttonPress.PIN, INPUT_PULLDOWN);
  msg = "config pushbutton: Done";
  Serial.print(msg);
  Heltec.display->drawString(0, line_2, msg);
  Heltec.display->display();
  Serial.println("==> Done");

  msg = "Attach interrupt... ";
  Serial.print(msg);
  Heltec.display->drawString(0, line_3, msg);
  Heltec.display->display();
  Serial.print("Attach interrupt... ");
  attachInterrupt(buttonPress.PIN, handle_button_interrupt, FALLING);
  msg = "Attach interrupt: Done";
  Serial.print(msg);
  Heltec.display->drawString(0, line_3, msg);
  Heltec.display->display();
  Serial.println("==> Done");

  // set stepper speed
  msg = "Setting stepper speed...";
  Serial.print(msg);
  Heltec.display->drawString(0, line_4, msg);
  Heltec.display->display();

  // set the speed
  myStepper.setSpeed(speed);

  msg = "Setting stepper speed ==> Done";
  Serial.println(msg);
  Heltec.display->drawString(0, line_4, msg);
  Heltec.display->display();

  delay(1000); // time to read config

  // blank the OLED screen and prepare to display speed and direction
  Heltec.display->clear();
  Heltec.display->display();
  Heltec.display->drawString(0, line_1, "Direction:");
  Heltec.display->drawString(0, line_2, "Speed:");
  Heltec.display->display();

  old_direction_msg = "wait...";
  old_speed_msg = "wait...";
  Heltec.display->drawString(direction_x, line_1, old_direction_msg);
  Heltec.display->drawString(speed_x, line_2, old_speed_msg);
  Heltec.display->display();
}

void loop()
{
  // read speed from 12-bit ADC input (map 1-1000; stepper doesn't like speed=0)
  speed = (long)(map(analogRead(pot), 0, 4095, 1, 100));
  // speed = 5;
  myStepper.setSpeed(speed);

  // display speed and direction
  direction_msg = "clockwise";
  speed_msg = String(speed);
  update_display(old_direction_msg, old_speed_msg, direction_msg, speed_msg);
  old_direction_msg = direction_msg;
  old_speed_msg = speed_msg;
  Serial.printf("clockwise, speed: %d ", speed);
  if (buttonPress.PRESSED)
    Serial.println(" button PRESSED");
  else
    Serial.println(" button NOT pressed");
  if (buttonPress.PRESSED)
  {
    buttonPress.PRESSED = false;
    myStepper.clear_interrupt();
  }
  // step one revolution in one direction:
  myStepper.step(stepsPerRevolution);
  delay(500);

  // read speed from 12-bit ADC input (map 1-1000; stepper doesn't like speed=0)
  speed = (long)(map(analogRead(pot), 0, 4095, 1, 100));
  myStepper.setSpeed(speed);

  // display speed and direction
  direction_msg = "counterclockwise";
  speed_msg = String(speed);
  update_display(old_direction_msg, old_speed_msg, direction_msg, speed_msg);
  old_direction_msg = direction_msg;
  old_speed_msg = speed_msg;
  Serial.printf("counterclockwise, speed: %d ", speed);
  if (buttonPress.PRESSED)
    Serial.println(" button PRESSED");
  else
    Serial.println(" button NOT pressed");
  if (buttonPress.PRESSED)
  {
    buttonPress.PRESSED = false;
    myStepper.clear_interrupt();
  }
  // step one revolution in the other direction:
  myStepper.step(-stepsPerRevolution);

  delay(500);
}

//---------------------[ functions ]------------------------------------------------------------
void OLED_setup()
{
  // Initialize the Heltec ESP32 object - uses joeBot logo at images.h
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
  Heltec.display->drawString(0, line_0, msg);
  Heltec.display->display();
}

void update_display(String old_dir, String old_spd, String new_dir, String new_spd)
{
  Heltec.display->setColor(BLACK);
  Heltec.display->drawString(direction_x, line_1, old_dir);
  Heltec.display->drawString(speed_x, line_2, old_spd);
  Heltec.display->display();

  Heltec.display->setColor(WHITE);
  Heltec.display->drawString(direction_x, line_1, new_dir);
  Heltec.display->drawString(speed_x, line_2, new_spd);
  Heltec.display->display();
}
