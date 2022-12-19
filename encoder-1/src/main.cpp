#include <Arduino.h>
#include <ESP32Encoder.h>

ESP32Encoder encoder;

// timer and flag for example, not needed for encoders
unsigned long lastToggled;

boolean encoderPaused = false;

void setup()
{

  Serial.begin(115200);
  // Enable the weak pull down resistors

  // ESP32Encoder::useInternalWeakPullResistors=DOWN;
  //  Enable the weak pull up resistors
  ESP32Encoder::useInternalWeakPullResistors = UP;

  // use pin 19 and 18 for the encoder
  encoder.attachHalfQuad(19, 18);


  // set starting count value after attaching
  encoder.setCount(37);

  // clear the encoder's raw count and set the tracked count to zero
  encoder.clearCount();
  Serial.println("Encoder Start = " + String((int32_t)encoder.getCount()));
  // set the lastToggle
 lastToggled = millis();
}

void loop()
{
  // Loop and read the count
  Serial.println("Encoder count = " + String((int32_t)encoder.getCount()));
  delay(1000);

  
}