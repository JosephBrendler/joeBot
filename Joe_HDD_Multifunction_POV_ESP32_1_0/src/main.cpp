#include <Arduino.h>
#include <WiFi.h>
#include <myNetworkInformation.h>

// define output pins
#define LED2 2       // LED_BUILTIN
#define RED_LED 5    // gpio 5; pin 29
#define GREEN_LED 18 // gpio 18; pin 30
#define BLUE_LED 19  // gpio 19; pin 31

// define input pins
#define photoInterruptPin 27  // photoTrigger; gpio 27; pin 11
#define buttonInterruptPin 33 // function pushbutton; gpio 33; pin 8

#define functionBit0 12 // bit 0 (lsb) of function selected; gpio 12; pin 13
#define functionBit1 13 // bit 1 of function selected; gpio 13; pin 15
#define functionBit2 14 // bit 2 of function selected; gpio 14; pin 12
#define functionBit3 15 // bit 3 (msb) of function7 selected; gpio 15; pin 23

#define LED_ON LOW   // my LEDs are common collector (active LOW)
#define LED_OFF HIGH // my LEDs are common collector (active LOW)

const char *ssid = mySSID;
const char *pass = myPASSWORD;

// time variables used in setting clock and timestamping
time_t timeNow;
struct tm timeinfo;
const long gmtOffset_sec = (-5 * 3600);
const int daylightOffset_sec = 3600;
const char *ntpServer1 = "time.nist.gov";
const char *ntpServer2 = "pool.ntp.org";

int hours = 0;   // 0-23
int minutes = 0; // 0-59
int seconds = 0; // 0-59

int function = 0; // function read from 4 bits, selected by pushbutton

// ------------------ define and initialize interrupt line -----------
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

// --------------- function declarations ---------------
void setMyTime();
void printLocalTime();

// --------------- interrupt function declarations ---------------
void IRAM_ATTR trigger()
{
    photoTrigger.numHits++;
    photoTrigger.TRIGGERED = true;
}
void IRAM_ATTR press()
{
    buttonPress.numHits++;
    buttonPress.PRESSED = true;
}

//--------------- setup() --------------------------------
void setup()
{
    Serial.begin(115200);
    Serial.println("Starting setup()...");

    // configure outputs
    pinMode(LED2, OUTPUT);
    digitalWrite(LED2, LED_OFF);
    // config 3 x RGB LED outputs
    gpio_config_t io_conf;
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_OUTPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_DISABLE;
    io_conf.pin_bit_mask = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
    gpio_config(&io_conf);
    // turn LEDs off (set output pins -- my LEDs are active LOW)
    GPIO.out_w1ts = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);

    // configure photo trigger interrupt pin
    pinMode(photoTrigger.PIN, INPUT_PULLUP);
    attachInterrupt(photoTrigger.PIN, trigger, FALLING);

    // configure function pushbutton interrupt
    pinMode(buttonPress.PIN, INPUT_PULLUP);
    attachInterrupt(buttonPress.PIN, press, FALLING);

    // configure 4 x digital input pins used to select function
    io_conf.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE;
    io_conf.mode = GPIO_MODE_INPUT;
    io_conf.pull_down_en = GPIO_PULLDOWN_DISABLE;
    io_conf.pull_up_en = GPIO_PULLUP_ENABLE;
    io_conf.pin_bit_mask = (1ULL << functionBit0) | (1ULL << functionBit1) | (1ULL << functionBit2) | (1ULL << functionBit3);
    gpio_config(&io_conf);

    // Configure and start the WiFi station
    Serial.printf("Connecting to %s ", ssid);
    WiFi.mode(WIFI_STA);
    WiFi.begin(ssid, pass);
    while (WiFi.status() != WL_CONNECTED)
    {
        delay(500);
        Serial.print(".");
    }
    Serial.printf("\n   CONNECTED to WiFi [%s] on IP: ", ssid);
    Serial.println(WiFi.localIP());

    // print unititialized time, just to be able to compare
    printLocalTime();
    setMyTime();
    printLocalTime();

    // disconnect WiFi as it's no longer needed
    WiFi.disconnect(true);
    WiFi.mode(WIFI_OFF);
    Serial.println("WiFi disconnected");

    Serial.println("setup complete");
}

//--------------- loop() --------------------------------
void loop()
{
    // first handle flagged interrupts, then do other stuff
    if (photoTrigger.TRIGGERED)
    {
        GPIO.out_w1tc = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);
        delayMicroseconds(200);
        GPIO.out_w1ts = (1 << RED_LED) | (1 << GREEN_LED) | (1 << BLUE_LED);

        Serial.printf("photoInterrupt has fired %u times\n", photoTrigger.numHits);
        photoTrigger.TRIGGERED = false;
    }
    if (buttonPress.PRESSED)
    {
        // read input pins, to set function - 4 x bits are active LOW so substract from 0xff
        uint8_t state = 0xff - (GPIO_REG_READ(GPIO_IN_REG) >> functionBit0) & 0b1111;
        Serial.printf("Selected: [%d], Pressed %d times\n", state, buttonPress.numHits);
        buttonPress.PRESSED = false;
    }
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
