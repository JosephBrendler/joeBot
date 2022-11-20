#include <Arduino.h>

#define LED_ON LOW
#define LED_OFF HIGH

void setup()
{
  // put your setup code here, to run once:
  pinMode(LED_BUILTIN, OUTPUT);
  Serial.begin(921600);
  Serial.println("Hello from setup");
}

void loop()
{
  // put your main code here, to run repeatedly:
  delay(100);
  Serial.println("Hello from the loop");
  digitalWrite(LED_BUILTIN, LED_ON);
  delay(3000);
  digitalWrite(LED_BUILTIN, LED_OFF);
}