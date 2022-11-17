#include <Arduino.h>
#include <WiFi.h>
#include <myNetworkInformation.h>

#define LED2 2
#define RED_LED 32
#define GREEN_LED 33
#define BLUE_LED 25
#define LED_ON LOW
#define LED_OFF HIGH

struct InterruptLine
{
  const uint8_t PIN;
  uint32_t numHits;
  bool TRIGGERED;
};

InterruptLine photoTrigger = {27, 0, false};

void IRAM_ATTR isr()
{
  photoTrigger.numHits++;
  photoTrigger.TRIGGERED = true;
}

void setup()
{
  Serial.begin(115200);
  pinMode(LED2, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(RED_LED, LED_OFF);
  digitalWrite(GREEN_LED, LED_OFF);
  digitalWrite(BLUE_LED, LED_OFF);

  pinMode(photoTrigger.PIN, INPUT_PULLUP);
  attachInterrupt(photoTrigger.PIN, isr, FALLING);
  Serial.println("setup complete");
}

void loop()
{
  if (photoTrigger.TRIGGERED)
  {
    Serial.printf("Interrupt has fired %u times\n", photoTrigger.numHits);
    photoTrigger.TRIGGERED = false;
    digitalWrite(LED2, HIGH);
    digitalWrite(RED_LED, LED_ON);
    digitalWrite(GREEN_LED, LED_ON);
    digitalWrite(BLUE_LED, LED_ON);
    delayMicroseconds(200);
    digitalWrite(LED2, LOW);
    digitalWrite(RED_LED, LED_OFF);
    digitalWrite(GREEN_LED, LED_OFF);
    digitalWrite(BLUE_LED, LED_OFF);
  }
}