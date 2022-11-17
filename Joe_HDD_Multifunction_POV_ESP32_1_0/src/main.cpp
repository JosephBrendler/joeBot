#include <Arduino.h>
#include <WiFi.h>
#include <myNetworkInformation.h>

// define output pins
#define LED2 2       // LED_BUILTIN
#define RED_LED 32   // gpio 32; pin 7
#define GREEN_LED 33 // gpio 33; pin 8
#define BLUE_LED 25  // gpio 25; pin 9

// define input pins
#define photoInterruptPin 27 // photoTrigger; gpio 27; pin 11
#define buttonInterruptPin 14 // function pushbutton; gpio 14; pin 12
#define functionBit0 0        // bit 0 (lsb) of function selected; gpio 0; pin 25
#define functionBit1 4        // bit 1 of function selected; gpio 4; pin 26
#define functionBit2 16       // bit 2 of function selected; gpio 16; pin 27
#define functionBit3 17       // bit 3 (msb) of function selected; gpio 17; pin 28

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
  bool PRESSED;    // boolean logical; is triggered now
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
  pinMode(LED2, OUTPUT);
  pinMode(RED_LED, OUTPUT);
  pinMode(GREEN_LED, OUTPUT);
  pinMode(BLUE_LED, OUTPUT);
  digitalWrite(RED_LED, LED_OFF);
  digitalWrite(GREEN_LED, LED_OFF);
  digitalWrite(BLUE_LED, LED_OFF);

  // configure photo trigger interrupt pin
  pinMode(photoTrigger.PIN, INPUT_PULLUP);
  attachInterrupt(photoTrigger.PIN, trigger, FALLING);

  // configure function pushbutton interrupt and select pin
  pinMode(buttonPress.PIN, INPUT_PULLUP);
  attachInterrupt(buttonPress.PIN, press, FALLING);
  pinMode(functionBit0, INPUT_PULLUP);
  pinMode(functionBit1, INPUT_PULLUP);
  pinMode(functionBit2, INPUT_PULLUP);
  pinMode(functionBit3, INPUT_PULLUP);

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
    digitalWrite(LED2, HIGH);
    digitalWrite(RED_LED, LED_ON);
    digitalWrite(GREEN_LED, LED_ON);
    digitalWrite(BLUE_LED, LED_ON);
    delayMicroseconds(200);
    digitalWrite(LED2, LOW);
    digitalWrite(RED_LED, LED_OFF);
    digitalWrite(GREEN_LED, LED_OFF);
    digitalWrite(BLUE_LED, LED_OFF);
    Serial.printf("photoInterrupt has fired %u times\n", photoTrigger.numHits);
    photoTrigger.TRIGGERED = false;
  }
  if (buttonPress.PRESSED){
    // read input pins, to set function
    function = digitalRead(functionBit0);
    function |= (digitalRead(functionBit1) << 1);
    function |= (digitalRead(functionBit2) << 2);
    function |= (digitalRead(functionBit3) << 3);
    function = (15 - function);
    Serial.printf("Selected: [%d]; fired %u times\n", function, buttonPress.numHits);
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