/* 
 HC_SR04.h
 Joe Brendler
 16 Dec 2013
 HC-SR04 Ping distance sensor (returns distance in centimeters)
 Some code inspired by http://www.instructables.com/id/Simple-Arduino-and-HC-SR04-Example
*/

#ifndef HC_SR04_h
#define HC_SR04_h

#include "Arduino.h"


class HC_SR04
{
  public:
    HC_SR04(int trigPin, int echoPin);
    int ping();
  private:
    long _duration;
    int _distance;
    int _trigPin;
    int _echoPin;

};

#endif