/*
  Luminance_LED_DriverController_v2_20200418
    As chosen by switch S2, either fades the colors on an LED strip, repeatedly, or sets color as chosen by trim-pots
    (See Schematic: Luminance_LED_Driver_Congroller_v2.sch)
 */

// initialize led pins
//int red_led = 10;      //pwm pin D10; ATMEGA328P pin 16
//int green_led = 9;     //pwm pin D9; ATMEGA328P pin 15
//int blue_led = 11;     //pwm pin D11; ATMEGA328P pin 17
// These assignments below for Thuvia computer case LED strip ONLY. (wired wrong)
int red_led = 11;      //pwm pin D10; ATMEGA328P pin 16
int green_led = 10;     //pwm pin D9; ATMEGA328P pin 15
int blue_led = 9;     //pwm pin D11; ATMEGA328P pin 17

int S2_pin = 12;      //pin D12; ATMEGA328P pin 18 -- for Switch S2 (Analog vs Programmed pattern)

int red_in = 0;       //analog A0; ATMEGA328P pin 23 - Input for RGB (Red)
int green_in = 1;     //analog A1; ATMEGA328P pin 24 - Input for RGB (Green)
int blue_in = 2;      //analog A2; ATMEGA328P pin 25 - Input for RGB (Blue)

// timing - increase to slow down
int factor = 1;
//int factor = 10;
//int factor = 100;
int pause = 25 * factor;   // pause with color set (ms)
int wait = 2 * factor;     // control speed of color transition

// color variables
int redness = 0;
int greeness = 0;
int blueness = 0;
int brightness = 0;

const int ON = 255;     
const int OFF = 0;  

// the setup routine runs once when you press reset:
void setup() {
  //for debugging only -- comment out after use
//  Serial.begin(115200);
//  Serial.println("Luminance_LED_DriveController_v2 (setup)");
    
  // initialize digital pins as an output.
  pinMode(red_led, OUTPUT);
  pinMode(green_led, OUTPUT);
  pinMode(blue_led, OUTPUT);
    LED_write(red_led, ( OFF ));
    LED_write(blue_led, ( OFF ));
    LED_write(green_led, ( OFF ));
}

// the loop routine runs over and over again forever:
void loop() {
  delay(pause);
  // read S2, and use Analog Setting or Programmed Pattern
  //Serial.println( digitalRead(S2_pin) );
  if (digitalRead(S2_pin)) {
    //Serial.println( "Going to fade_sequence" );
    fade_sequence();
  } else {
    //Serial.println( "Going to analog_setting" );
    analog_setting();
  }
}

void analog_setting() {
/*  For debugging.  Comment out when not in use  * /
    Serial.print( "red_in: " );
    Serial.println( map(analogRead(red_in),0,1023,0,255) );
    Serial.print( "green_in: " );
    Serial.println( map(analogRead(blue_in),0,1023,0,255) );
    Serial.print( "blue_in: " );
    Serial.println( map(analogRead(green_in),0,1023,0,255) );
*/
    LED_write(red_led, ( map(analogRead(red_in),0,1023,0,255) ));
    LED_write(blue_led, ( map(analogRead(blue_in),0,1023,0,255) ));
    LED_write(green_led, ( map(analogRead(green_in),0,1023,0,255) ));
    delay(pause);
}
void fade_sequence() {
    LED_write(red_led, ( OFF ));
    LED_write(blue_led, ( OFF ));
    LED_write(green_led, ( OFF ));
  // fade dark to red     ( R)  (ON=255 OFF=0)
  for (int redness=OFF; redness<=ON; redness++) {
    LED_write(red_led, ( redness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause);
   // fade red to violet   ( R + B )  (ON=255 OFF=0)
  for (int blueness=OFF; blueness<=ON; blueness++) {
    LED_write(blue_led, ( blueness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause);
  // fade violet to blue  ( B )  (ON=255 OFF=0)
  for (int redness=ON; redness>=OFF; redness--) {
    LED_write(red_led, ( redness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause);
  // fade blue to bluegreen  ( B + G )  (ON=255 OFF=0)
  for (int greeness=OFF; greeness<=ON; greeness++) {
    LED_write(green_led, ( greeness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause);
  // fade bluegreen to green ( G )  (ON=255 OFF=0)
  for (int blueness=ON; blueness>=OFF; blueness--) {
    LED_write(blue_led, ( blueness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause);
  // fade green to yellow ( G + R )  (ON=255 OFF=0)
  for (int redness=OFF; redness<=ON; redness++) {
    LED_write(red_led, ( redness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause);
  // fade yellow to white ( R + B + G )  (ON=255 OFF=0)
  for (int blueness=OFF; blueness<=ON; blueness++) {
    LED_write(blue_led, ( blueness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause);
  // fade to dark         (  )  (ON=255 OFF=0)
  for (int brightness=ON; brightness>=OFF; brightness--) {
    LED_write(red_led, ( brightness ));
    LED_write(green_led, ( brightness ));
    LED_write(blue_led, ( brightness ));
    if ( ! digitalRead(S2_pin) ) break;
    delay(wait);
  }
  if ( digitalRead(S2_pin) ) delay(pause); 
}

// reverse logic for inverting power transistors
void LED_write(int pin, int LED_brightness) {
      analogWrite(pin, ( 255 - LED_brightness ));
}
