#include <HC_SR04.h>

HC_SR04 Pinger ( 13, 12 );  // trigPin, echoPin

int dist = 0;
int redLED = 11;
int greenLED = 10;

void setup() {
  Serial.begin(9600);
  pinMode(redLED, OUTPUT);
  pinMode(greenLED, OUTPUT);
}

void loop() {
  dist = Pinger.ping();
  Serial.print("Distance: ");
  Serial.println(dist);
  if ( dist > 10 ) {
    digitalWrite(greenLED, HIGH);
    digitalWrite(redLED, LOW);
  } else {
    digitalWrite(greenLED, LOW);
    digitalWrite(redLED, HIGH);
   
  }
  delay(500);
}
