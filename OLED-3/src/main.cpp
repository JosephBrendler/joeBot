#include <heltec.h>
#include "images.h"
void setup()
{
    Serial.begin(115200);
    // Initialize the Heltec ESP32 object
    Heltec.begin(true /*DisplayEnable Enable*/, false /*LoRa Disable*/, true /*Serial Enable*/);
    Heltec.display->setContrast(255);
    Heltec.display->display();
    Serial.println("initialized");
    delay(3000);
    Heltec.display->clear();
    Heltec.display->drawXbm(0, 5, logo_width, logo_height, (const unsigned char *)logo_bits);
    Heltec.display->display();
    delay(3000);
}

void loop()
{

    Heltec.display->invertDisplay();
    delay(3000);
    Heltec.display->normalDisplay();
    delay(3000);
}