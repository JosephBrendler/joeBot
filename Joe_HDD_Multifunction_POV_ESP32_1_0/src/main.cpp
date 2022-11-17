#include <Arduino.h>
#include <WiFi.h>
#include <myNetworkInformation.h>

#define LED2 2
#define RED_LED 32
#define GREEN_LED 33
#define BLUE_LED 25
#define LED_ON LOW
#define LED_OFF HIGH

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

// ------------------ define and initialize interrupt line -----------
struct InterruptLine
{
  const uint8_t PIN; // gpio pin number
  uint32_t numHits;  // number of times fired
  bool TRIGGERED;    // boolean logical; is triggered now
};

InterruptLine photoTrigger = {27, 0, false};

// --------------- function declarations ---------------
void setMyTime();
void printLocalTime();

void IRAM_ATTR trigger()
{
  photoTrigger.numHits++;
  photoTrigger.TRIGGERED = true;
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

  pinMode(photoTrigger.PIN, INPUT_PULLUP);
  attachInterrupt(photoTrigger.PIN, trigger, FALLING);

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
  if (photoTrigger.TRIGGERED)
  {
    Serial.printf("Interrupt has fired %u times\n", photoTrigger.numHits);
    photoTrigger.TRIGGERED = false;
    digitalWrite(LED2, HIGH);
    digitalWrite(RED_LED, LED_ON);
    digitalWrite(GREEN_LED, LED_ON);
    digitalWrite(BLUE_LED, LED_ON);
    delayMicroseconds(200);
    digitalWrite(LED2, LOW);
    digitalWrite(RED_LED, LED_OFF);
    digitalWrite(GREEN_LED, LED_OFF);
    digitalWrite(BLUE_LED, LED_OFF);
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