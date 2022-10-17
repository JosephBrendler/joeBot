#include <Arduino.h>

int LED_ON = LOW;    //8266 Built-in LED is active low
int LED_OFF = HIGH;
long counter = 0;
String msg = "";

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
  // put your setup code here, to run once:

  pinMode(LED_BUILTIN, OUTPUT);
  blink();
  Serial.begin(115200);
  Serial.println("Setup Complete");
}

void loop()
{
  // put your main code here, to run repeatedly:

  msg = "Running cycle [";
  msg += counter++;
  msg += "]";
  Serial.println(msg);
  delay(1000);
  blink();
}