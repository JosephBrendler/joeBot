#include <Arduino.h>
//#define ESP32_RTOS // Uncomment this line if you want to use the code with freertos only on the ESP32
                   // Has to be done before including "OTA.h"

#include "OTA.h"
#include <credentials.h>

const int LED_ON = LOW; // ctive high for ESP32; ESP12 8266 is active low)
const int LED_OFF = HIGH;

void check_status(); // function definition (see function further below)

void blink()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(100);
    digitalWrite(LED_BUILTIN, LED_OFF);
    delay(100);
  }
}

void setup()
{
  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);
  Serial.begin(115200);
  Serial.println("Booting");

  setupOTA("JoeESP12_01", mySSID, myPASSWORD);   // use unique name each time

  // Your setup code
}

void loop()
{
//#ifdef defined(ESP32_RTOS) && defined(ESP32)
#if defined(ESP32_RTOS) && defined(ESP32)
#else // If you do not use FreeRTOS, you have to regulary call the handle method.
  ArduinoOTA.handle();
#endif

  // Your code here
  check_status();
}

void check_status()
{
  if (WiFi.status() == WL_CONNECTED)
  {
//    blink();
    digitalWrite(LED_BUILTIN, LED_ON);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
  delay(100);
}