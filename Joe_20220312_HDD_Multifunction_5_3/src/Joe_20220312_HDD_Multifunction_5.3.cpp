// synchronous led display of multi-function data using spinning slotted disk
// Joe Brendler -  21 Jan 2013;  Rev 4 20190501 (HDD with Arduino mini, total rewrite)
// Rev 5.0 28 Feb 2022 (Try with timer interrupts, register programmed)
// Rev 5.1 7  March 2022 -- New approach
// Rev 5.2 12 March 2022 -- Add multi-functionality
// See explanitory comments in Joe_20220228_HHD_Clock_5.0 and _5.1
#include <Arduino.h>

// all of my outputs (LED drivers) are on PORTB (0-based bits 1, 2, 3)
const int redLED = PB1;   // D9  = PB1  (All leds are on port B)
const int greenLED = PB2; // D10 = PB2  (All leds are on port B)
const int blueLED = PB3;  // D11 = PB3  (All leds are on port B)

// all of my digital inputs are on PORTD (0-based bits 7:2)
const int pb_fn_activate = PD2; // D2  = PD2 (external interrupt 0) - activate
const int photoTrigger = PD3;   // D3  = PD3 (external interrupt 1) - sync
const int fn_bit_0 = PD4;       // D4  = PD4 2^0 = 1
const int fn_bit_1 = PD5;       // D5  = PD5 2^1 = 2
const int fn_bit_2 = PD6;       // D6  = PD6 2^2 = 4
const int fn_bit_3 = PD7;       // D7  = PD7 2^3 = 8

// I have one analog input - used to determine pattern repetition integer
const int checkerNumber_pin = 0; // A0 (ADC0)

int pattern = 0;
int checkerNumber = 0;

const byte BLACK = 0b00000000;   // RGB 000 mapped to Port B LED pins
const byte RED = 0b00000010;     // RGB 001 mapped to Port B LED pins
const byte GREEN = 0b00000100;   // RGB 010 mapped to Port B LED pins
const byte YELLOW = 0b00000110;  // RGB 011 mapped to Port B LED pins
const byte BLUE = 0b00001000;    // RGB 100 mapped to Port B LED pins
const byte MAGENTA = 0b00001010; // RGB 101 mapped to Port B LED pins
const byte CYAN = 0b00001100;    // RGB 110 mapped to Port B LED pins
const byte WHITE = 0b00001110;   // RGB 111 mapped to Port B LED pins

const String RGBstr[8] = {"Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Yellow", "White"};

long milliseconds = 0; //
int seconds = 0;       // integer number of seconds
int minutes = 0;       // integer number of minutes
int hours = 0;         // integer number of hours
int newseconds = 0;    // integer number of seconds
int newminutes = 0;    // integer number of minutes
int newhours = 0;      // integer number of hours

byte ClockFace[60];    // 60 valid positions, each with 3-bit RGB value to be displayed
int clockPosition = 0; // position (0-59) on the clockface; index for ClockFace[]

volatile boolean Triggered = false; // flag on sync interrupt
volatile boolean clockTick = false; // flag on timer interrupt
boolean ACTIVATED = false;          // flag on pushbutton interrupt
boolean INITIALIZED = false;        // flag for clockface
boolean DONE_RADAR = false;         // flag to display radar beacon only once per period

//-----[ user-configurable factors - emperically determined ]------------------------
float fudgeFactor = 3.85;     // Timer1 clock is divided by 4 relative to expected
const int compareTicks = 763; // actually observed
const int offset = 14;        // number of "seconds" to rotate display for orientation
//-----------------------------------------------------------------------------------

const float T1microsPerTick = (0.0625 * fudgeFactor);               // 62.5 nS; see table is setup()
const unsigned int compareTarget = (compareTicks * fudgeFactor);    // OCR1A value for observation
const unsigned int clockSlotWidth = compareTicks * T1microsPerTick; // in uS
const unsigned int intervalOn = (unsigned int)(clockSlotWidth / 3); // draw skinnier lines

int i = 0, j = 0; // integer loop counters

//------[ function declarations ]-----------------------------------------------
ISR(TIMER1_COMPA_vect);
void set_loop_fn();
void trigger();
void calculateClockFace();
void blink();
void loop_fn_clock();
void loop_fn_radar();
void loop_fn_RedBlack();
void loop_fn_colors();
void loop_fn_checkers();
void loop_fn_checker_colors();
void loop_fn_fan();

/*------------------------------------------------------------------------------
   setup()
  ------------------------------------------------------------------------------*/
void setup()
{
  // Give me 5 seconds to spin up the disk (note: can't use delay with interrupts off)
  delay(5000);

  // no interrupts
  cli();

  Serial.begin(2000000);
  Serial.println("Begin Setup");

  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;

  // enable interupts on output compare match by setting TOIE and OCIE bits
  // Set the Timer1 interrupt Mask Register (TIMSK1) so that we disable  TOIE1
  // (Timer/Counter 1 Overflow Interrupt Enable) and enable compare interrupt A (OCIE1A)
  TIMSK1 = (1 << OCIE1A);

  // Set TCCR1B to CTC mode and prescaler (CS2:1)to 001 (same as system clock)
  TCCR1B = 0b00001001; // CTC mode, clock tick period 62.5nS (reset in functions)

  // set led pinMode to OUTPUT (all 3 LEDs are on PORTB)
  DDRB |= ((1 << redLED) | (1 << greenLED) | (1 << blueLED));

  // set pinmode of pb_fn_activate, photoTrigger, and fn_bit_x pins as INPUT
  DDRD &= ~((1 << pb_fn_activate) |
            (1 << photoTrigger) |
            (1 << fn_bit_3) | (1 << fn_bit_2) | (1 << fn_bit_1) | (1 << fn_bit_0));
  // enable pullups on all INPUT pins
  PORTD |= ((1 << pb_fn_activate) |
            (1 << photoTrigger) |
            (1 << fn_bit_3) | (1 << fn_bit_2) | (1 << fn_bit_1) | (1 << fn_bit_0));
  // PORTD &= ~(1 << photoTrigger); // disable pullup
  //  attach photoTrigger as an interrupt
  attachInterrupt(digitalPinToInterrupt(photoTrigger), trigger, FALLING);
  attachInterrupt(digitalPinToInterrupt(pb_fn_activate), set_loop_fn, FALLING);

  // set pinmode of analog potentiometer pin to INPUT
  DDRC &= ~(1 << checkerNumber_pin);
  PORTC &= ~(1 << checkerNumber_pin); // disable pullup

  // ToDo: calibrate -- this will set output compare register OCR1A
  //  calibrate();
  OCR1A = compareTarget; // default to clock

  // calculate initial time-reference (ToDo: set refTime w thumbwheel and PushButton_interrupt)
  milliseconds = millis();
  seconds = (int)(milliseconds / 1000.0) % 60;
  minutes = (int)(milliseconds / 60000.0) % 60;
  hours = (int)(milliseconds / 3600000.0) % 12;

  // read function from pattern bits
  set_loop_fn();

  // enable global interrupts
  sei();

  // demonstrate setup (blink uses delay, so it needs interrupts turned on
  blink();
  Serial.println("Done setup; now enabling interrupts");
}

/*------------------------------------------------------------------------------
   set_loop_fn() -- called by setup and by interrupt
  ------------------------------------------------------------------------------*/
void set_loop_fn()
{
  pattern = ((PIND & 0b11110000) >> 4); // function pins are PD7:4 ; mask and shift right 4
  pattern = (0b1111 & ~pattern);        // convert active LOW; is 32 bit int, so mask bitwise NOT
  ACTIVATED = true;
  Serial.print("pattern (BIN): ");
  Serial.println(pattern, BIN);
  Serial.print("pattern (DEC): ");
  Serial.println(pattern, DEC);
  // Serial.print("~pattern (BIN): ");
  // Serial.println(~pattern, BIN);
  // Serial.print("~pattern (DEC): ");
  // Serial.println(~pattern, DEC);
  // Serial.print("0b1111 & ~pattern (BIN): ");
  // Serial.println(0b1111 & ~pattern, BIN);
  // Serial.print("0b1111 & ~pattern (DEC): ");
  // Serial.println(0b1111 & ~pattern, DEC);
}

/*------------------------------------------------------------------------------
   trigger() --  do this stuff when the photo trigger interrupt fires
  ------------------------------------------------------------------------------*/
void trigger()
{
  // raise flag; reset timer
  Triggered = true;
  TCNT1 = 0;
}

/*------------------------------------------------------------------------------
   ISR(clockSlot) --  do this stuff when the timer interrupt fires
  ------------------------------------------------------------------------------*/
ISR(TIMER1_COMPA_vect)
{
  // raise flag
  clockTick = true;
}

/*------------------------------------------------------------------------------
   calculateClockFace() -- set the 60 values of the data to be displayed on clock
  ------------------------------------------------------------------------------*/
void calculateClockFace()
{
  // ClockFace[i] is an array modelling the 60 positions of the clock face, with
  // each element set to the RGB value of color that should be displayed in that position
  // this function updates this model with a minimum of instructions

  if (!INITIALIZED)
  {
    // blank the clockface
    for (i = 0; i < 60; i++)
    {
      ClockFace[i] = 0;
    }

    // mark the "graduations"
    for (i = 0; i < 60; i += 5)
    {
      if (i % 15 == 0)
      {
        // quater - mark yellow
        ClockFace[i] = YELLOW;
      }
      else
      {
        // 1/12th - mark white
        ClockFace[i] = WHITE;
      }
    }

    // raise flag
    INITIALIZED = true;
  }

  // Get new time; change only values that need to be changed
  // Assign (if needed) in this order so each supercedes previous,
  //   if more than one hand is in the same position on the clock
  // Calculate/assign seconds every time b/c this function is called every second
  newseconds = (int)(milliseconds / 1000.0) % 60;
  ClockFace[seconds] = BLACK; // blank the old one
  // if the old second hand was "covering" something else, rewrite that w supercession
  if (seconds == minutes)
  {
    ClockFace[minutes] = BLUE;
  }
  else if (seconds == hours)
  {
    ClockFace[hours] = GREEN;
  }
  else if (seconds % 5 == 0)
  {
    if (seconds % 15 == 0)
    {
      ClockFace[seconds] = YELLOW;
    }
    else
    {
      ClockFace[seconds] = WHITE;
    }
  }

  // calculate/assign minutes only when seconds gets back to 0
  if (newseconds == 0)
  {
    newminutes = (int)(milliseconds / 60000.0) % 60;
    ClockFace[minutes] = BLACK; // blank the old one
    // if the minute hand was "covering" something else, rewrite that w supercession
    if (minutes == hours)
    {
      ClockFace[hours] = GREEN;
    }
    else if (minutes % 5 == 0)
    {
      if (minutes % 15 == 0)
      {
        ClockFace[minutes] = YELLOW;
      }
      else
      {
        ClockFace[minutes] = WHITE;
      }
    }
  }

  // calculate/assign hours only when minutes gets back to 0
  if (newminutes == 0)
  {
    newhours = (int)(milliseconds / 3600000.0) % 12;
    ClockFace[hours] = BLACK; // blank the old one
    // if the hour hand was "covering" something else, rewrite that w supercession
    if (hours % 5 == 0)
    {
      if (hours % 15 == 0)
      {
        ClockFace[hours] = YELLOW;
      }
      else
      {
        ClockFace[hours] = WHITE;
      }
    }
  }

  if (newminutes == 0)
    ClockFace[newhours] = GREEN; // write the new one
  if (newseconds == 0)
    ClockFace[newminutes] = BLUE; // write the new one
  ClockFace[newseconds] = RED;    // write the new one

  seconds = newseconds;
  minutes = newminutes;
  hours = newhours;
}

/*------------------------------------------------------------------------------
   blink()
  ------------------------------------------------------------------------------*/
void blink()
{
  for (i = 0; i <= 2; i++)
  {
    PORTB |= (1 << redLED);
    delay(100);
    PORTB &= ~(1 << redLED);
    delay(100);
  }
  for (i = 0; i <= 3; i++)
  {
    PORTB |= (1 << greenLED);
    delay(100);
    PORTB &= ~(1 << greenLED);
    delay(100);
  }
  for (i = 0; i <= 3; i++)
  {
    PORTB |= (1 << blueLED);
    delay(100);
    PORTB &= ~(1 << blueLED);
    delay(100);
  }
}

/*------------------------------------------------------------------------------
   function 0:  loop_fn_clock()
  ------------------------------------------------------------------------------*/
void loop_fn_clock()
{
  cli();
  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = (1 << OCIE1A);
  TCCR1B = 0b00001001;   // CTC mode, clock tick period 62.5nS
  OCR1A = compareTarget; // default
  // clear leds
  PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
  sei();
  // perform this function until reset
  while (!ACTIVATED)
  {
    // (process trigger interrupt)
    if (Triggered)
    {
      clockPosition = 0; // ToDo -- use relative number to "align the clock face"
      // display the 0-marker; provides offset and reverses b/c disk rotates ccw
      PORTB = ClockFace[59 - ((offset + clockPosition) % 60)];
      delayMicroseconds(intervalOn);
      PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
      Triggered = false; // lower the Triggered flag
    }

    // display clock face
    if (clockTick)
    {
      clockPosition++;
      // set my active LED bits - up to the 60th position
      if (clockPosition < 60)
      { // provides offset and reverses b/c disk rotates ccw
        PORTB = ClockFace[59 - ((offset + clockPosition) % 60)];
        delayMicroseconds(intervalOn);
        PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
      }
      clockTick = false; // lower the clockTick flag
    }

    // every 1 seconds, recalculate the time
    // assign milliseconds here to avoid having to do so again in calculateClockFace()
    milliseconds = millis();
    if (milliseconds % 1000 == 0)
    {
      calculateClockFace();
    }
  }
}

/*------------------------------------------------------------------------------
   function 1: loop_fn_radar() -- display the radar pattern
  ------------------------------------------------------------------------------*/
void loop_fn_radar()
{
  cli();
  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = (1 << OCIE1A);
  TCCR1B = 0b00001010;              // CTC mode, clock tick period 0.5uS
  OCR1A = compareTarget;            // default
  unsigned int periodTicks = 21995; // empirically determined for CS2:0 = 010
  unsigned int radarSlot = 0;       // also in ticks
  // clear leds
  PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
  sei();
  // perform this function until reset
  while (!ACTIVATED)
  {
    if (Triggered)
    {
      Triggered = false;
      DONE_RADAR = false;
      // mark sync spot
      PORTB |= ((1 << redLED) | (1 << greenLED) | (1 << blueLED));
      delayMicroseconds(intervalOn);
      PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
      // calculate how long to wait to display radar beacon
      radarSlot = (unsigned int)(periodTicks * (millis() % 5000) / 5000.0);
      OCR1A = radarSlot;
      // now return to loop to wait for clockTick
    }

    if (clockTick)
    {
      if (!DONE_RADAR)
      { // only once per period
        PORTB |= (1 << redLED);
        delayMicroseconds(intervalOn);
        PORTB &= ~(1 << redLED);
        DONE_RADAR = true;
        clockTick = false;
      }
      else
      {
        clockTick = false;
      }
    }
  }
}

/*------------------------------------------------------------------------------
   function 2: loop_fn_RedBlack() -- display the color pattern
  ------------------------------------------------------------------------------*/
void loop_fn_RedBlack()
{
  cli();
  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = (1 << OCIE1A);
  TCCR1B = 0b00001010;              // CTC mode, clock tick period 0.5uS
  unsigned int periodTicks = 21995; // empirically determined for CS2:0 = 010
  //  unsigned int radarSlot = ( periodTicks / 8 ) + adjustmentFactor;    // also in ticks
  unsigned int radarSlot = (periodTicks / 8); // also in ticks
  OCR1A = radarSlot;                          // fire 8 times per disk rotation
  // clear leds
  PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
  sei();
  // perform this function until reset
  while (!ACTIVATED)
  {
    if (Triggered)
    {
      i = 0;
      Triggered = false;
    }

    if (clockTick)
    {
      // slot for eight bands, alternating red/black
      PORTB ^= (1 << redLED);
      clockTick = false;
    }
  }
}

/*------------------------------------------------------------------------------
   function 3: loop_fn_colors() -- display the color pattern
  ------------------------------------------------------------------------------*/
void loop_fn_colors()
{
  cli();
  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = (1 << OCIE1A);
  TCCR1B = 0b00001010;              // CTC mode, clock tick period 0.5uS
  unsigned int periodTicks = 21995; // empirically determined for CS2:0 = 010
  //  unsigned int radarSlot = ( periodTicks / 8 ) + adjustmentFactor;    // also in ticks
  unsigned int radarSlot = (periodTicks / 8); // also in ticks
  int slotColor = 0;
  OCR1A = radarSlot; // fire 8 times per disk rotation
  // clear leds
  PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
  sei();
  // perform this function until reset
  while (!ACTIVATED)
  {
    if (Triggered)
    {
      i = 0;
      Triggered = false;
    }

    if (clockTick)
    {
      // slot for eight colors, including black (3-bit values RGB)
      slotColor = ((++i % 8) << 1);
      PORTB = ((PINB & 0b11110001) | slotColor);
      clockTick = false;
    }
  }
}

/*------------------------------------------------------------------------------
   function 4: loop_fn_checkers() -- display the checker pattern
  ------------------------------------------------------------------------------*/
void loop_fn_checkers()
{
  cli();
  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = (1 << OCIE1A);
  TCCR1B = 0b00001010;              // CTC mode, clock tick period 0.5uS
  unsigned int periodTicks = 21995; // empirically determined for CS2:0 = 010
  //  unsigned int radarSlot = ( periodTicks / 8 ) + adjustmentFactor;    // also in ticks
  unsigned int radarSlot = (periodTicks / 8); // default; also in ticks
  OCR1A = radarSlot;                          // fire 8 times per disk rotation
  // clear leds
  PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
  sei();
  // perform this function until reset
  while (!ACTIVATED)
  {
    if (Triggered)
    {
      Triggered = false;
      // get an even number between 0 and 60, for checker pattern
      checkerNumber = 2 * map(analogRead(checkerNumber_pin), 0, 1023, 0, 30);
      radarSlot = (periodTicks / checkerNumber);
      OCR1A = radarSlot; // fire checkerNumber times per disk rotation
    }

    if (clockTick)
    {
      PORTB ^= (1 << redLED); // toggle red led
      clockTick = false;
    }
  }
}

/*------------------------------------------------------------------------------
   function5: loop_fn_checker_colors() -- display the checker pattern
  ------------------------------------------------------------------------------*/
void loop_fn_checker_colors()
{
  cli();
  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = (1 << OCIE1A);
  TCCR1B = 0b00001010;              // CTC mode, clock tick period 0.5uS
  unsigned int periodTicks = 21995; // empirically determined for CS2:0 = 010
  //  unsigned int radarSlot = ( periodTicks / 8 ) + adjustmentFactor;    // also in ticks
  unsigned int radarSlot = (periodTicks / 8); // default; also in ticks
  int slotColor = 0;
  OCR1A = radarSlot; // fire 8 times per disk rotation
  // clear leds
  PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
  sei();
  // perform this function until reset
  while (!ACTIVATED)
  {
    if (Triggered)
    {
      i = 0;
      Triggered = false;
      // get an even number between 0 and 60, for checker pattern
      checkerNumber = 2 * map(analogRead(checkerNumber_pin), 0, 1023, 0, 30);
      radarSlot = (periodTicks / checkerNumber);
      OCR1A = radarSlot; // fire checkerNumber times per disk rotation
    }

    if (clockTick)
    {
      // checkerNumber slots of eight colors, including black (3-bit values RGB)
      slotColor = ((++i % 8) << 1);
      PORTB = ((PINB & 0b11110001) | slotColor);
      clockTick = false;
    }
  }
}

/*------------------------------------------------------------------------------
   function 6: loop_fn_fan() -- display the radar pattern
  ------------------------------------------------------------------------------*/
void loop_fn_fan()
{
  cli();
  // Reset timer
  TCCR1A = 0; // Timer 1 is 16 bits; counts up to 65535
  TCCR1B = 0;
  TCNT1 = 0;
  TIMSK1 = (1 << OCIE1A);
  TCCR1B = 0b00001010;              // CTC mode, clock tick period 0.5uS
  OCR1A = compareTarget;            // default
  unsigned int periodTicks = 21995; // empirically determined for CS2:0 = 010
  unsigned int radarSlot = 0;       // also in ticks
  // clear leds
  PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
  sei();
  // perform this function until reset
  while (!ACTIVATED)
  {
    if (Triggered)
    {
      Triggered = false;
      DONE_RADAR = false;
      // mark sync spot
      PORTB |= ((1 << redLED) | (1 << greenLED) | (1 << blueLED));
      delayMicroseconds(intervalOn);
      PORTB &= ~((1 << redLED) | (1 << greenLED) | (1 << blueLED));
      // calculate how long to wait to display radar beacon
      radarSlot = (unsigned int)(periodTicks * (millis() % 5000) / 5000.0);
      OCR1A = radarSlot;
      // now return to loop to wait for clockTick
    }

    if (clockTick)
    {
      if (!DONE_RADAR)
      { // only once per period
        // mark beginning of fan
        PORTB |= ((1 << redLED) | (1 << blueLED) | (1 << blueLED));
        delayMicroseconds(intervalOn);
        // clear green and blue
        PORTB &= ~((1 << blueLED) | (1 << blueLED));
        // leave led on twice the duration as delay to get there
        delayMicroseconds((2 * radarSlot * T1microsPerTick) - intervalOn);
        PORTB &= ~(1 << redLED);
        DONE_RADAR = true;
        clockTick = false;
      }
      else
      {
        clockTick = false;
      }
    }
  }
}

/*------------------------------------------------------------------------------
   loop()
  ------------------------------------------------------------------------------*/
void loop()
{
  ACTIVATED = false;
  // Note: pattern bits are active low, so take the bitwise NOT of pattern
  switch (pattern)
  {
  case 0:
    // Serial.println("Executing clock function");
    loop_fn_clock();
    break;
  case 1:
    // Serial.println("Executing radar function");
    loop_fn_radar();
    break;
  case 2:
    // Serial.println("Executing RedBlack function");
    loop_fn_RedBlack();
    break;
  case 3:
    // Serial.println("Executing colors function");
    loop_fn_colors();
    break;
  case 4:
    // Serial.println("Executing checkers function");
    loop_fn_checkers();
    break;
  case 5:
    // Serial.println("Executing checker_colors function");
    loop_fn_checker_colors();
    break;
  case 6:
    loop_fn_fan();
    break;
  // with current hardware (10 pos. rotary fn switch), cases 7, 8, 9 fall to default
  default:
    loop_fn_clock();
    break;
  }
  delayMicroseconds(1);
}
