/*
 HC_SR04.cpp
 Joe Brendler
 16 Dec 2013
 HC-SR04 Ping distance sensor (returns distance in centimeters)
 Some code inspired by http://www.instructables.com/id/Simple-Arduino-and-HC-SR04-Example
*/

#include "Arduino.h"
#include "HC_SR04.h"

// HC_SR04 Constructor
HC_SR04::HC_SR04(int trigPin, int echoPin)
{
  pinMode(trigPin, OUTPUT);
  pinMode(echoPin, INPUT);
  _trigPin = trigPin;
  _echoPin = echoPin;
}

int HC_SR04::ping(){
  long _duration;
  int _distance;

  // trigger ultrasonic pulse. LOW to clear 2 microseconds, then HIGH for 10
  // microseconds, then LOW
  digitalWrite(_trigPin, LOW);
//  delayMicroseconds(3);
//  digitalWrite(_trigPin, HIGH);
//  delayMicroseconds(11);
  delayMicroseconds(300);
  digitalWrite(_trigPin, HIGH);
  delayMicroseconds(1000);
  digitalWrite(_trigPin, LOW);

  // Time the duration untill arrival of the echo'd return pulse
  // and calculate the distance (half the round trip travelling at the speed of sound)
  // Vs=343.643m/s, or 29.1 microseconds per centimeter.
  // Set pulseIn() timeout to duration of time to return of pulse from obstacle at 
  // 3m distance (17460 microseconds round trip); pulseIn() will ignore and return 0
  // at distances larger than 3m taking more than that long to return
  _duration = pulseIn(_echoPin, HIGH);
  _distance = (_duration/2) / 29.1;

  return _distance;

}
