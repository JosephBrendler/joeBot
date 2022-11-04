#include <Arduino.h>
#include <WiFi.h>

#define ssid "Cayenne-guest"
#define password "hotsauc3isgoodforyou!"

time_t timeNow;
struct tm timeinfo;
const long gmtOffset_sec = (-5 * 3600);
const int daylightOffset_sec = 3600;
const char *ntpServer1 = "time.nist.gov";
const char *ntpServer2 = "pool.ntp.org";

int hours = 0;
int minutes = 0;
int seconds = 0;

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

void printLocalTime()
{
  time(&timeNow);
  Serial.printf("timeNow.: Coordinated Universal Time is %s", asctime(gmtime(&timeNow)));
  gmtime_r(&timeNow, &timeinfo);
  Serial.print("timeinfo: Coordinated Universal Time is ");
  //  Serial.println(&timeinfo, "%A, %B %d %Y %H:%M:%S");
  Serial.println(&timeinfo, "%a %b %d %H:%M:%S %Y");
  Serial.print("timeinfo: ............ my Local Time is ");
  localtime_r(&timeNow, &timeinfo);
  Serial.println(&timeinfo, "%a %b %d %H:%M:%S %Y");
  // hours = timeinfo.tm_hour;
  hours = (timeinfo.tm_hour % 12);
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;
  Serial.printf("Hours: %d, Minutes: %d, Seconds: %d\n", hours, minutes, seconds);
}

void setup()
{
  Serial.begin(115200);

  // connect to WiFi
  Serial.printf("Connecting to %s ", ssid);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.printf("\n   CONNECTED to WiFi [%s] on IP: %s\n", ssid, String(WiFi.localIP()));

  // print unititialized time, just to be able to compare
  printLocalTime();

  // initialize and print the time
  setMyTime();
  printLocalTime();

  // disconnect WiFi as it's no longer needed
  WiFi.disconnect(true);
  WiFi.mode(WIFI_OFF);
  Serial.println("WiFi disconnected");
}

void loop()
{
  // code
  printLocalTime();
  delay(1000);
}