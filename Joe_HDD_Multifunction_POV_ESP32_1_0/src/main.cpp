#include <Arduino.h>
#include <WiFi.h>
#include <myNetworkInformation.h>

// define output pins
#define LED2 2 // LED_BUILTIN
// define RGB LEDs on consecutive sequential bits of GPIO register - to write at once
#define RED_LED 17   // gpio 17; pin 28
#define GREEN_LED 18 // gpio 18; pin 30
#define BLUE_LED 19  // gpio 19; pin 31

// define input pins
#define photoInterruptPin 27  // photoTrigger; gpio 27; pin 11
#define buttonInterruptPin 33 // function pushbutton; gpio 33; pin 8

// define function bits on consecutive sequential bits of GPIO register - to read at once
#define functionBit0 12 // bit 0 (lsb) of function selected; gpio 12; pin 13
#define functionBit1 13 // bit 1 of function selected; gpio 13; pin 15
#define functionBit2 14 // bit 2 of function selected; gpio 14; pin 12
#define functionBit3 15 // bit 3 (msb) of function7 selected; gpio 15; pin 23

// used to set LED2 (active HIGH on ESP32, LOW on 8266)
#define LED_ON HIGH
#define LED_OFF LOW

const char *ssid = mySSID;
const char *pass = myPASSWORD;

// time variables used in setting clock and timestamping
time_t timeNow;
struct tm timeinfo;
const long gmtOffset_sec = (-5 * 3600);
const int daylightOffset_sec = 3600;
const char *ntpServer1 = "time.nist.gov";
const char *ntpServer2 = "pool.ntp.org";

/*
const byte BLACK = 0b00000000;   // RGB 000 mapped to Port B LED pins
const byte RED = 0b00000010;     // RGB 001 mapped to Port B LED pins
const byte GREEN = 0b00000100;   // RGB 010 mapped to Port B LED pins
const byte YELLOW = 0b00000110;  // RGB 011 mapped to Port B LED pins
const byte BLUE = 0b00001000;    // RGB 100 mapped to Port B LED pins
const byte MAGENTA = 0b00001010; // RGB 101 mapped to Port B LED pins
const byte CYAN = 0b00001100;    // RGB 110 mapped to Port B LED pins
const byte WHITE = 0b00001110;   // RGB 111 mapped to Port B LED pins
*/

// define 3-bit RGB color values
const byte BLACK = 0b000;
const byte RED = 0b001;
const byte GREEN = 0b010;
const byte YELLOW = 0b010;
const byte BLUE = 0b100;
const byte MAGENTA = 0b101;
const byte CYAN = 0b110;
const byte WHITE = 0b111;

const String RGBstr[8] = {"Black", "Blue", "Green", "Cyan", "Red", "Magenta", "Yellow", "White"};

uint8_t pattern = 0; // what pattern (function) to draw

byte ClockFace[60];    // 60 valid positions, each with 3-bit RGB value to be displayed
int clockPosition = 0; // position (0-59) on the clockface; index for ClockFace[]

int hours = 0;            // 0-23
int minutes = 0;          // 0-59
int seconds = 0;          // 0-59
long milliseconds = 0;    //
int newseconds = 0;       // integer number of seconds
int newminutes = 0;       // integer number of minutes
int newhours = 0;         // integer number of hours
bool INITIALIZED = false; // clockface initialized?

//-----[ user-configurable factors - emperically determined ]------------------------
// const int compareTicks = 763; // actually observed
const int offset = 14; // number of "seconds" to rotate display for orientation
//-----------------------------------------------------------------------------------

// ToDO - calibration (for now 21.6ms period = 60 x 360us slots)
int slotWidth = 360;

const unsigned int intervalOn = (unsigned int)(slotWidth / 3); // draw skinnier lines

int function = 0; // function read from 4 bits, selected by pushbutton

// initialize timer and associated boolean flag
hw_timer_t *slotTimer = NULL;
volatile bool ENDSLOT = false;
hw_timer_t *secondTimer = NULL;
volatile bool MARKSECOND = false;

// initialize timer mux(s) to take care of the synchronization between
// the main loop and the ISR, when modifying a shared variable (volatile bools)
portMUX_TYPE slotTimerMux = portMUX_INITIALIZER_UNLOCKED;
portMUX_TYPE secondTimerMux = portMUX_INITIALIZER_UNLOCKED;

// -------- define and initialize interrupt line structs -----------
struct photoInterruptLine
{
    const uint8_t PIN; // gpio pin number
    uint32_t numHits;  // number of times fired
    bool TRIGGERED;    // boolean logical; is triggered now
};
photoInterruptLine photoTrigger = {photoInterruptPin, 0, false};

struct buttonInterruptLine
{
    const uint8_t PIN; // gpio pin number
    uint32_t numHits;  // number of times fired
    bool PRESSED;      // boolean logical; is triggered now
};
buttonInterruptLine buttonPress = {buttonInterruptPin, 0, false};

int i = 0, j = 0; // integer loop counters

// --------------- function declarations ---------------
void setMyTime();
void printLocalTime();
void blink();
void loop_fn_clock();
void loop_fn_radar();
void loop_fn_RedBlack();
void loop_fn_colors();
void loop_fn_checkers();
void loop_fn_checker_colors();
void loop_fn_fan();

// --------------- interrupt function declarations ---------------
void IRAM_ATTR trigger()
{
    photoTrigger.numHits++;
    photoTrigger.TRIGGERED = true;
}

/*------------------------------------------------------------------------------
   set_loop_fn() -- called by setup and by interrupt
  ------------------------------------------------------------------------------*/
void IRAM_ATTR set_loop_fn()
{
    buttonPress.numHits++;
    buttonPress.PRESSED = true;
}

void IRAM_ATTR onSecondTimer()
{
    portENTER_CRITICAL(&secondTimerMux);
    MARKSECOND = true;
    portEXIT_CRITICAL(&secondTimerMux);
}

void IRAM_ATTR onSlotTimer()
{
    portENTER_CRITICAL(&slotTimerMux);
    ENDSLOT = true;
    portEXIT_CRITICAL(&slotTimerMux);
}

//--------------- setup() --------------------------------
void setup()
{

    // configure outputs
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, LED_OFF);

    // Give me 5 seconds to spin up the disk (note: can't use delay with interrupts off)
    delay(5000); // ToDo - loop will spin up motor now, so this has to go

    Serial.begin(115200);
    Serial.println("Starting setup()... ");

    // Configure 3 x RGB LED outputs
    Serial.print("configure 3 x RGB LED outputs... ");
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
    gpio_config(&io_conf);
    // Turn LEDs off (clear output pins -- my LEDs are active LOW but driven by inverting NPN transistors)
    Serial.print("==> Done\nTurning off LEDs... ");
    GPIO.out_w1tc = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
    Serial.println("==> Done");

    // Configure photo trigger interrupt pin
    Serial.print("Configure photo trigger interrupt pin... ");
    pinMode(photoTrigger.PIN, INPUT_PULLUP);
    Serial.println("==> Done");
    Serial.print("Attach photo trigger interrupt... ");
    attachInterrupt(photoTrigger.PIN, trigger, FALLING);
    Serial.println("==> Done");

    // Configure function pushbutton interrupt pin
    Serial.print("Configure function pushbutton interrupt pin... ");
    pinMode(buttonPress.PIN, INPUT_PULLUP);
    Serial.println("==> Done");
    Serial.print("Attach function pushbutton interrupt... ");
    attachInterrupt(buttonPress.PIN, set_loop_fn, FALLING);
    Serial.println("==> Done");

    // no interrupts
    // Serial.print("Disabling interrupts... ");
    // cli();
    // Serial.println("==> Done");

    // Configure 4 x digital input pins used to select function
    Serial.print("Configure 4 x digital input pins used to select function... ");
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << functionBit0) | (1ULL << functionBit1) | (1ULL << functionBit2) | (1ULL << functionBit3);
    gpio_config(&io_conf);
    Serial.println("==> Done");

    // Configure timers for interrupt (use prescaler 80 to clock at 1mhz, 1us/tick)
    Serial.print("Configure timers for interrupt (use prescaler 80 to clock at 1mhz, 1us/tick)... ");
    secondTimer = timerBegin(0, 80, true);
    timerAttachInterrupt(secondTimer, &onSecondTimer, true);
    timerAlarmWrite(secondTimer, slotWidth, false); // period will be reset by local time (seconds)
    timerAlarmDisable(secondTimer);                 // enabled by photoTrigger; disabled by self
    slotTimer = timerBegin(3, 80, true);
    timerAttachInterrupt(slotTimer, &onSlotTimer, true);
    timerAlarmWrite(slotTimer, slotWidth, false);
    timerAlarmDisable(slotTimer); // enabled by clockHand timer; disabled by self
    Serial.println("==> Done");

    // Configure and start the WiFi station
    Serial.printf("Starting WiFi; connecting to %s ", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n==> Done. CONNECTED to WiFi [%s] on IP: ", ssid);
    Serial.println(WiFi.localIP());
    Serial.println("Setting local time from NTP server... ");

    // print unititialized time, just to be able to compare
    printLocalTime();
    setMyTime();
    printLocalTime();
    Serial.println("==> Done setting time");

    // disconnect WiFi as it's no longer needed
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("==> WiFi disconnected");

    // read function from pattern bits (call isr for artificial button press)
    set_loop_fn();

    // enable global interrupts
    Serial.print("Done setup; now enabling interrupts globally... ");
    sei();
    Serial.println("==> Done");
    digitalWrite(LED2, LED_ON); // demonstrate setup (blink uses delay, so it needs interrupts turned on
    blink();

    Serial.println("==> Setup complete");
    Serial.println("---------------[ Runtime Output Follows ]-------------------");
}

//--------------- old_loop() --------------------------------
void old_loop()
{
    // first handle flagged interrupts, then do other stuff
    if (photoTrigger.TRIGGERED)
    {
        // new spin-cycle; start second hand timer
        portENTER_CRITICAL(&secondTimerMux);
        timerAlarmWrite(secondTimer, (seconds * slotWidth), false);
        timerAlarmEnable(secondTimer);
        portEXIT_CRITICAL(&secondTimerMux);

        // Serial.printf("photoInterrupt has fired %u times\n", photoTrigger.numHits);
        photoTrigger.TRIGGERED = false;
    }
    if (buttonPress.PRESSED)
    {
        // read input pins, to set function - 4 x bits are active LOW so substract from 0xff
        pattern = 0xff - (GPIO_REG_READ(GPIO_IN_REG) >> functionBit0) & 0b1111;
        Serial.printf("Selected: [%d], Pressed %d times\n", pattern, buttonPress.numHits);
        buttonPress.PRESSED = false;
    }
    if (ENDSLOT)
    {
        // turn off leds; stop slot timer
        // turn LEDs off (clear output pins -- my LEDs are active LOW but driven by inverting NPN transistors)
        GPIO.out_w1tc = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
        portENTER_CRITICAL(&slotTimerMux);
        timerAlarmDisable(slotTimer);
        ENDSLOT = false;
        // Serial.println("ENDSLOT");
        portEXIT_CRITICAL(&slotTimerMux);
    }
    if (MARKSECOND)
    {
        // turn on LEDs (flag MARKSECOND to start slot timer, stop second timer)
        // turn LEDs on (set output pins -- my LEDs are active LOW but driven by inverting NPN transistors)
        GPIO.out_w1ts = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
        portENTER_CRITICAL(&slotTimerMux);
        timerAlarmWrite(slotTimer, (slotWidth), false);
        timerAlarmEnable(slotTimer);
        portENTER_CRITICAL(&secondTimerMux);
        timerAlarmDisable(secondTimer);
        MARKSECOND = false;
        // Serial.println("MARKSECOND");
        portEXIT_CRITICAL(&secondTimerMux);
        portEXIT_CRITICAL(&slotTimerMux);
    }
    // now do other stuff
}

/*------------------------------------------------------------------------------
   loop()
  ------------------------------------------------------------------------------*/
void loop()
{
    if (buttonPress.PRESSED)
    {
        // read input pins, to set function - 4 x bits are active LOW so substract from 0xff
        pattern = 0xff - (GPIO_REG_READ(GPIO_IN_REG) >> functionBit0) & 0b1111;
        Serial.printf("Selected: [%d], Pressed %d times\n", pattern, buttonPress.numHits);
        buttonPress.PRESSED = false;
    }
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

//--------------- setMyTime() --------------------------------
void setMyTime()
{
    configTime(gmtOffset_sec, daylightOffset_sec, ntpServer1, ntpServer2);
    // while (time(nullptr) < 8 * 3600)
    while (!getLocalTime(&timeinfo))
    {
        Serial.println("Failed to obtain time");
        delay(5000);
    }
}

//--------------- printLocalTime() --------------------------------
void printLocalTime()
{
    time(&timeNow);
    // Serial.printf("timeNow.: Coordinated Universal Time is %s", asctime(gmtime(&timeNow)));
    gmtime_r(&timeNow, &timeinfo);
    // Serial.print("timeinfo: Coordinated Universal Time is ");
    //   Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
    // Serial.println(&timeinfo, "%a %b %d %H:%M:%S %Y");
    Serial.print("timeinfo: ............ my Local Time is ");
    localtime_r(&timeNow, &timeinfo);
    Serial.println(&timeinfo, "%a %b %d %H:%M:%S %Y");
    // hours = timeinfo.tm_hour;
    hours = (timeinfo.tm_hour % 12);
    minutes = timeinfo.tm_min;
    seconds = timeinfo.tm_sec;
    Serial.printf("Hours: %d, Minutes: %d, Seconds: %d\n", hours, minutes, seconds);
}

//--------------- printLocalTime() --------------------------------
void setLocalTime()
{
    localtime_r(&timeNow, &timeinfo);
    newhours = (timeinfo.tm_hour % 12);
    newminutes = timeinfo.tm_min;
    newseconds = timeinfo.tm_sec;
    // Serial.printf("Hours: %d, Minutes: %d, Seconds: %d\n", newhours, newminutes, newseconds);
}

/*------------------------------------------------------------------------------
   blink()
  ------------------------------------------------------------------------------*/
void blink()
{
    for (i = 0; i <= 2; i++)
    {
        GPIO.out_w1ts = (1 << RED_LED);
        delay(100);
        GPIO.out_w1tc = (1 << RED_LED);
        delay(100);
    }
    for (i = 0; i <= 3; i++)
    {
        GPIO.out_w1ts = (1 << GREEN_LED);
        delay(100);
        GPIO.out_w1tc = (1 << GREEN_LED);
        delay(100);
    }
    for (i = 0; i <= 3; i++)
    {
        GPIO.out_w1ts = (1 << BLUE_LED);
        delay(100);
        GPIO.out_w1tc = (1 << BLUE_LED);
        delay(100);
    }
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

    setLocalTime;  // should be current, as long as NTP set it in setup()
    
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

    // assign minutes only when seconds gets back to 0
    if (newseconds == 0)
    {
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

    // assign hours only when minutes gets back to 0
    if (newminutes == 0)
    {
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
    ClockFace[newseconds] = RED;      // write the new one

    seconds = newseconds;
    minutes = newminutes;
    hours = newhours;
}

/*------------------------------------------------------------------------------
   function 0:  loop_fn_clock()
  ------------------------------------------------------------------------------*/
void loop_fn_clock()
{
    // turn LEDs off (clear output pins -- my LEDs are active LOW but driven by inverting NPN transistors)
    GPIO.out_w1tc = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
    // protect from interruption; enable slot timer, disable other timer(s) while in this function
    portENTER_CRITICAL(&slotTimerMux);
    timerAlarmWrite(slotTimer, (slotWidth), true); // make this auto-reloading for this function
    timerAlarmEnable(slotTimer);
    portEXIT_CRITICAL(&slotTimerMux);
    portENTER_CRITICAL(&secondTimerMux);
    timerAlarmDisable(secondTimer);
    portEXIT_CRITICAL(&secondTimerMux);

    // perform this function until reset
    while (!buttonPress.PRESSED)
    {
        // (process trigger interrupt)
        if (photoTrigger.TRIGGERED)
        {
            clockPosition = 0; // ToDo -- use relative number to "align the clock face"
            // display the 0-marker; provides offset and reverses b/c disk rotates ccw
            // (then write 3-bit RGB to the consecutive LED register bits)
            GPIO.out_w1ts = ((ClockFace[59 - ((offset + clockPosition) % 60)]) << RED_LED);
            // now delay slotwidth and then turn them off
            delayMicroseconds(intervalOn);
            GPIO.out_w1tc = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
            photoTrigger.TRIGGERED = false; // lower the Triggered flag
        }

        // display clock face
        if (ENDSLOT)  // spinning slot has arrived at new clock slot position
        {
            clockPosition++;
            // set my active LED bits - up to the 60th position
            if (clockPosition < 60)
            { // provides offset and reverses b/c disk rotates ccw
                GPIO.out_w1ts = ((ClockFace[59 - ((offset + clockPosition) % 60)]) << RED_LED);
                // now delay slotwidth and then turn them off
                delayMicroseconds(intervalOn);
                GPIO.out_w1tc = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
                ;
            }
            ENDSLOT = false; // lower the clockTick flag
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

// --- other function placeholders for now
void loop_fn_radar(){;}
void loop_fn_RedBlack(){;}
void loop_fn_colors(){;}
void loop_fn_checkers(){;}
void loop_fn_checker_colors(){;}
void loop_fn_fan(){;}
