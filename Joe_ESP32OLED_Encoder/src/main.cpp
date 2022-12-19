/*----------------------
 Encoder example w OLED
   Joe Brendler 19 Dec 2022
   OLED code from Heltec - https://github.com/HelTecAutomation/Heltec_ESP32
-----------------------*/
#include <Arduino.h>
#include <heltec.h>
#include "images.h"
#include <Encoder.h>

#define phase1A 17 // consecutive bits can be written easily with three bits shifted to the LSB position (0b111<<phase1A)
#define phase2A 18
#define phase3A 19

Encoder myEncoder(37, 38); // inputs GPIO 37, 38 for encoder interrupts
long oldPosition = -999, newPosition = 0;
long newSpeed = 0, oldSpeed = 0;
unsigned long delta_t = 0; // in usec

// config gpios
gpio_config_t io_conf;

uint64_t periodMicros = 21500; // HDD empirically measured

const int col_space = 30;
const int col0_x = 0,
          col1_x = col0_x + col_space,
          col2_x = col1_x + col_space,
          col3_x = col2_x + col_space,
          col4_x = col3_x + col_space;
const int line_0 = 0, line_1 = 10, line_2 = 20, line_3 = 30, line_4 = 40;

String msg = "", old_msg = "";

unsigned long now = 0, old_millis = 0; // loop timing

//--------- function declarations ------------
void update_display(String old_var1, String old_var2, String old_var3, String new_var1, String new_var2, String new_var3);
void OLED_setup();

//---------- setup() -------------------------
void setup()
{
  // Initialize the Heltec ESP32 object
  OLED_setup(); // leaves display w "OLED Initialized" on line_0

  Serial.begin(115200);
  msg = "Starting setup";
  Serial.println(msg);
  Heltec.display->drawString(col0_x, line_1, msg);
  Heltec.display->display();

  msg = "Initializing Encoder...";
  Heltec.display->drawString(col0_x, line_2, msg);
  Heltec.display->display();
  // Initialize encoder
  myEncoder.write(0);
  msg = "Initializing Encoder: Done";
  Heltec.display->drawString(col0_x, line_2, msg);
  Heltec.display->display();
  Serial.println("Encoder Initialized");

  msg = "Done Configuration";
  Serial.println(msg);
  Heltec.display->drawString(col0_x, line_3, msg);
  Heltec.display->display();
  delay(2000); // time to read

  // set up persistent display
  Heltec.display->clear();
  Heltec.display->display();
  msg = "Direction: ";
  Heltec.display->drawString(col0_x, line_1, msg);
  msg = "Position: ";
  Heltec.display->drawString(col0_x, line_2, msg);
  msg = "Speed: ";
  Heltec.display->drawString(col0_x, line_3, msg);
  update_display("0", "0", "0", "0", "0", "0"); // initial placeholder data
  Heltec.display->display();

  Serial.println("setup() done");
}

//--------------- loop() --------------------------
void loop()
{
  // every half-second, update display with encoder data
  now = millis();
  delta_t = now - old_millis;
  if (delta_t > 500) // update display periodically
  {
    String newDir = "ccw (temp)", oldDir = newDir;
    old_millis = now;
    newPosition = myEncoder.read();
    if (newPosition != oldPosition)
    {
      newSpeed = (long)((newPosition - oldPosition) / (float)delta_t);

      Serial.printf("newDir: %s\n", newDir);
      Serial.printf("newPosition: %d\n", newPosition);
      Serial.printf("newSpeed: %d\n", newSpeed);
      update_display(oldDir, String(oldPosition), String(oldSpeed), newDir, String(newPosition), String(newSpeed));

      oldPosition = newPosition;
      oldSpeed = newSpeed;
      oldDir = newDir;
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

void update_display(String old_var1, String old_var2, String old_var3, String new_var1, String new_var2, String new_var3)
{
  // blank out the old data
  Heltec.display->setColor(BLACK);
  Heltec.display->drawString(col2_x, line_1, old_var1);
  Heltec.display->drawString(col2_x, line_2, old_var2);
  Heltec.display->drawString(col2_x, line_3, old_var3);
  Heltec.display->display();

  // write the new data
  Heltec.display->setColor(WHITE);
  Heltec.display->drawString(col2_x, line_1, new_var1);
  Heltec.display->drawString(col2_x, line_2, new_var2);
  Heltec.display->drawString(col2_x, line_3, new_var3);
  Heltec.display->display();
}
