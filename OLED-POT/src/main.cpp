#include <Arduino.h>
#include "images.h"
#include "heltec.h"
#include "Stepper.h"

#define pot A0

int speed = 0;   // analog input reading
String msg = ""; // for display
String direction_msg, speed_msg, old_direction_msg, old_speed_msg;

const int line_0 = 0, line_1 = 10, line_2 = 20, line_3 = 30, line_4 = 40; // OLED screen positions
const int direction_x = 64, speed_x = 64;

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

void setup()
{
  // initialize the serial port:
  Serial.begin(115200);

  // initialize analog input
  pinMode(pot, INPUT);

  // start up OLED display
  OLED_setup();

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
  // read speed from 12-bit ADC input
  speed = map(analogRead(pot), 0, 4095, 0, 1000);
  // speed = ((speed +1) % 10);

  // display speed and direction
  Heltec.display->setColor(BLACK);
  Heltec.display->drawString(direction_x, line_1, old_direction_msg);
  Heltec.display->drawString(speed_x, line_2, old_speed_msg);
  direction_msg = "clockwise";
  speed_msg = String(speed);
  Heltec.display->setColor(WHITE);
  Heltec.display->drawString(direction_x, line_1, direction_msg);
  Heltec.display->drawString(speed_x, line_2, speed_msg);
  Heltec.display->display();
  old_direction_msg = direction_msg;
  old_speed_msg = speed_msg;
  Serial.printf("clockwise, speed: %d\n", speed);

  delay(500);
}