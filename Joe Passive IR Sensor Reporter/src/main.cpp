/*----------------------------------------------------------------------------------------------------
Use modes of the X.509 validation in the WiFiClientBearSSL object
to connect and exchange information with external web server
Joe Brendler Oct 2022
Based largley on Mar 2018 work by Earle F. Philhower, III - Released to the public domain
----------------------------------------------------------------------------------------------------*/
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include "myNetworkInformation.h"
#include "OTA_Joe2.h"
#include <stdio.h>

#define timeInterval 15 // used to define to keep output (e.g. relay) "on" after trigger

const char *ssid = mySSID;
const char *pass = myPASSWORD;
const char *esp_host = myESP_HOST;
const char *SSL_host = mySSL_HOST;
const int SSL_port = mySSL_PORT;
const char *logPath = myLogFormPATH;
const char *statusPath = myStatusFormPATH;
const char *protocol = myPROTOCOL;

const int OTA_LED = LED_BUILTIN;
const int OTA_LED_ON = HIGH; // active high for ESP32; ESP12 8266 is active low)
const int OTA_LED_OFF = LOW;
// Set GPIOs for trigger_led and PIR Motion Sensor
const int trigger_led = 14;
const int motionSensor = 27;

// time variables used in setting clock and timestamping
time_t timeNow = time(nullptr);
struct tm timeinfo;
char *timeStamp;

int data_len = 0;
const char *i_log_message = "ESP32 Motion Sensor Initialized";
const char *i_status_message = "Initialized";
const char *det_log_message = "Motion Detected!";
const char *det_status_message = "Detected!";
const char *down_log_message = "Motion stopped...";
const char *down_status_message = "Stopped";

const long gmtOffset_sec = (-5 * 3600);
const int daylightOffset_sec = 3600;
const char *ntpServer1 = "time.nist.gov";
const char *ntpServer2 = "pool.ntp.org";

int day = 0;
int month = 0;
int year = 0;
int hours = 0;
int minutes = 0;
int seconds = 0;

// Timer: Auxiliary variables
unsigned long now = millis();
unsigned long lastTrigger = 0;
boolean triggered = false;
boolean startTimer = false;

//-------------------- Define interrupt functions up front -----------------------------------------------
// Checks if motion was detected, sets trigger_led HIGH and starts a timer
void IRAM_ATTR detectsMovement()
{
  triggered = true;
}

// dummy function declarations (so I can move the actual functions below setup() and loop())
void setMyTime();
void printLocalTime();
char *getTimeStamp();
void separator(String msg);
void check_status();
void fetchDataWithKnownKey();
void uploadDataWithKnownKey(const char *uploadURL, const char *fieldName, char *myDATA);
// void uploadDataWithKnownKey(char *myDATA);
void blink(int LED);
char *build_url(const char *proto, const char *host, const int port, const char *logPath);

const char *myLogURL = build_url(protocol, SSL_host, SSL_port, logPath);
const char *myStatusURL = build_url(protocol, SSL_host, SSL_port, statusPath);

/*----------------------------------------------------------------------------------------------------
 * setup()
 *
 *---------------------------------------------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);
  Serial.println("\n");

  // PIR Motion Sensor mode INPUT_PULLUP
  pinMode(motionSensor, INPUT_PULLDOWN);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(motionSensor), detectsMovement, FALLING);

  // Set LEDs to LOW
  pinMode(OTA_LED, OUTPUT);
  digitalWrite(OTA_LED, LOW);
  pinMode(trigger_led, OUTPUT);
  digitalWrite(trigger_led, LOW);

  // connect to wifi and set up over the air re-programming
  blink(OTA_LED);
  setupOTA(esp_host, ssid, pass); // use unique name each time
  check_status();

  // set NTP time (needed for CA Cert expiry validation)
  printLocalTime();
  setMyTime();
  printLocalTime();

  // initialize timestamp
  delay(100);
  timeStamp = getTimeStamp();
  //  Serial.printf("Current time: %s", timeStamp);

  data_len = strlen(timeStamp) + 12 + strlen(i_log_message) + 1;
  char *il_data_chars = new char[data_len];
  sprintf(il_data_chars, "%s --> %s\n", timeStamp, i_log_message);

  Serial.printf("about to call uploadData with data: %s", il_data_chars);
  uploadDataWithKnownKey(myLogURL, "reading", il_data_chars);

  data_len = strlen(i_status_message) + 1;
  char *is_data_chars = new char[data_len];
  sprintf(is_data_chars, "%s\n", i_status_message);

  Serial.printf("about to call uploadData with data: %s", is_data_chars);
  uploadDataWithKnownKey(myStatusURL, "status", is_data_chars);

  blink(OTA_LED);
  check_status();

  Serial.println("Done setup...");
}

/*----------------------------------------------------------------------------------------------------
 * loop()
 *
 *---------------------------------------------------------------------------------------------------*/
void loop()
{
  // handle OTA first
  ArduinoOTA.handle();
  // safely handle interrupt flag, if it is set (don't put this kind of code in the interrupt itself)
  if (triggered)
  {
    Serial.println(det_log_message);
    lastTrigger = millis();
    digitalWrite(trigger_led, HIGH);
    startTimer = true;
    triggered = false;

    timeStamp = getTimeStamp();
    data_len = strlen(timeStamp) + 2 + strlen(det_log_message) + 1;
    char *dl_data_chars = new char[data_len];
    sprintf(dl_data_chars, "%s --> %s\n", timeStamp, det_log_message);
    uploadDataWithKnownKey(myLogURL, "reading", dl_data_chars);

    data_len = strlen(det_status_message) + 1;
    char *ds_data_chars = new char[data_len];
    sprintf(ds_data_chars, "%s\n", det_status_message);

    Serial.printf("about to call uploadData with data: %s", ds_data_chars);
    uploadDataWithKnownKey(myStatusURL, "status", ds_data_chars);
  }
  // Current time
  now = millis();
  // notify and stop the timer when appropriate (nothing happens until then)
  if (startTimer && (now - lastTrigger > (timeInterval * 1000)))
  {
    startTimer = false;
    timeStamp = getTimeStamp();
    digitalWrite(trigger_led, LOW);
    Serial.println(down_log_message);
    data_len = strlen(timeStamp) + 2 + strlen(down_log_message) + 1;
    char *sl_data_chars = new char[data_len];
    sprintf(sl_data_chars, "%s --> %s\n", timeStamp, down_log_message);
    uploadDataWithKnownKey(myLogURL, "reading", sl_data_chars);

    data_len = strlen(down_status_message) + 1;
    char *ss_data_chars = new char[data_len];
    sprintf(ss_data_chars, "%s\n", down_status_message);

    Serial.printf("about to call uploadData with data: %s", ss_data_chars);
    uploadDataWithKnownKey(myStatusURL, "status", ss_data_chars);
  }

  // Remainder of routine loop code here

  check_status();
}

/*----------------------------------------------------------------------------------------------------*/
// check status of wifi
void check_status()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(OTA_LED, OTA_LED_ON);
  }
  else
  {
    digitalWrite(OTA_LED, OTA_LED_OFF);
  }
  delay(100);
}

/*----------------------------------------------------------------------------------------------------*/
// print a separator in Serial output, for readability
void separator(String msg)
{
  Serial.println("----------[ " + msg + " ]----------");
}

/*----------------------------------------------------------------------------------------------------*/
// Blink the built-in LED
void blink(int LED)
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED, OTA_LED_ON);
    delay(100);
    digitalWrite(LED, OTA_LED_OFF);
    delay(100);
  }
}

/*----------------------------------------------------------------------------------------------------
Connect using a known key and a custome cipher list -- and retrieve data from server
** Known key:  The server certificate can be completely ignored, with its public key
hardcoded in the application. This should be secure as the public key
needs to be paired with the private key of the site, which is obviously
private and not shared.  A MITM without the private key would not be
able to establish communications.
** The ciphers used to set up the SSL connection can be configured to
only support faster but less secure ciphers  -- If you care more about security
you won't want to do this, but if you need to maximize battery life, these
may make sense. */
void fetchDataWithKnownKey()
{
  separator("fetchDataWithKnownKeyCustomCipherList()");
  HTTPClient https;
  WiFiClientSecure client;
  /* Options for securing vs MITM --
    (1) fingerprint of server cert
    (2) ca cert (needs NTP time)
    (3) known key (Server cert itself)
  //  client->setFingerprint(myFINGERPRINT);
  //  BearSSL::X509List cert(myCERT);
  //  client->setTrustAnchors(&cert); // Note: this method needs time to be set
    (using known key)  */
  client.setCertificate(myPUBKEY);
  // fetchURL(&client, SSL_host, SSL_port, logPath);
  Serial.print("[HTTPS] begin connection to url: " + String(myLogURL) + "\n");
  if (https.begin(myLogURL))
  { // HTTPS
    Serial.print("[HTTPS] Sending GET request...\n");
    // start connection and send HTTP header
    int httpCode = https.GET();
    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] GET returns code: %d (%s)\n\n", httpCode, https.errorToString(httpCode).c_str());

      // file found at server
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        Serial.println("Requesting Command and Control instruction with getString() method...");
        String payload = https.getString();
        Serial.println("Retrieved command: " + payload);
      }
    }
    else
    {
      Serial.printf("[HTTPS] GET... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    Serial.println("Done with payload...");
    delay(100);
    client.stop();
    https.end();
    delay(100);
    Serial.println("ended https...");
  }
  else
  {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  Serial.println("done with function...");
}

/*----------------------------------------------------------------------------------------------------
Connect using a known key and a custome cipher list -- and upload data to server
** Known key:  The server certificate can be completely ignored, with its public key
hardcoded in the application. This should be secure as the public key
needs to be paired with the private key of the site, which is obviously
private and not shared.  A MITM without the private key would not be
able to establish communications.
** The ciphers used to set up the SSL connection can be configured to
only support faster but less secure ciphers  -- If you care more about security
you won't want to do this, but if you need to maximize battery life, these
may make sense. */
void uploadDataWithKnownKey(const char *uploadURL, const char *fieldName, char *myDATA)
{
  separator("uploadDataWithKnownKey()");
  HTTPClient https;
  WiFiClientSecure client;
  /* Options for securing vs MITM --
    (1) fingerprint of server cert
    (2) ca cert (needs NTP time)
    (3) known key (Server cert itself)
  //  client->setFingerprint(myFINGERPRINT);
  //  BearSSL::X509List cert(myCERT);
  //  client->setTrustAnchors(&cert); // Note: this method needs time to be set
    (using known key)  */
  client.setCertificate(myPUBKEY);
  // fetchURL(&client, SSL_host, SSL_port, logPath);
  Serial.print("[HTTPS] begin connection to url: " + String(uploadURL) + "\n");

  struct tm timeinfo;
  gmtime_r(&timeNow, &timeinfo);

  // note: "fieldName" identifies the name of the form field we are POSTing to
  String postData = String(fieldName) + "=" + String(myDATA);

  if (https.begin(uploadURL))
  { // HTTPS
    Serial.print("[HTTPS] Sending POST request with data [");
    Serial.print(postData);
    Serial.println("]");

    https.addHeader("Content-Type", "application/x-www-form-urlencoded");
    // start connection and send HTTP header
    auto httpCode = https.POST(postData);
    // httpCode will be negative on error
    if (httpCode > 0)
    {
      // HTTP header has been send and Server response header has been handled
      Serial.printf("[HTTPS] POST returns code: %d (%s)\n\n", httpCode, https.errorToString(httpCode).c_str());

      // data upload succeeded
      if (httpCode == HTTP_CODE_OK || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
      {
        Serial.println("Data upload succeeded...");
        //        String payload = https.getString();
        //        Serial.println("Response to getString(): " + payload);
      }
    }
    else
    {
      Serial.printf("[HTTPS] POST ... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    Serial.println("Done with data transfer...");
    delay(100);
    client.stop();
    https.end();
    delay(100);
    Serial.println("ended https...");
  }
  else
  {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  Serial.println("done with function...");
}

char *build_url(const char *proto, const char *host, const int port, const char *logPath)
{
  // allocate memory for url characters -- note the number of
  // digits in an integer is {log(base)(integer) + 1} and the
  // char[] must be terminated with a '\0' char, so again + 1
  int url_size = strlen(protocol) + 3 + strlen(host) + (log10(port) + 1) + strlen(logPath) + 1;

  char *result = new char[url_size];

  // initialize result with all chars in host[] colon and logPath[]
  sprintf(result, "%s://%s:%d%s", protocol, host, port, logPath);
  return result;
}

/*----------------------------------------------------------------------------------------------------*/
// set clock from NTP (as required for validation of x.509 CA Certificate vs expiration
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

/*----------------------------------------------------------------------------------------------------*/
// Print UTC and Local Time
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

/*----------------------------------------------------------------------------------------------------*/
// return a timestamp
char *getTimeStamp()
{
  int stamp_length = 0;

  time(&timeNow);
  gmtime_r(&timeNow, &timeinfo);

  year = timeinfo.tm_year + 1900;
  month = timeinfo.tm_mon;
  day = timeinfo.tm_mday;
  hours = timeinfo.tm_hour;
  minutes = timeinfo.tm_min;
  seconds = timeinfo.tm_sec;

  // the string-length of an integer is log(base) + 1
  stamp_length = log10(hours) + 1;
  stamp_length += log10(minutes) + 1;
  stamp_length += log10(seconds) + 1;
  stamp_length += log10(year) + 1;
  stamp_length += log10(month) + 1;
  stamp_length += log10(day) + 1;
  stamp_length += 3; // 2 x ':' + 1 x [space]

  char *result = new char[stamp_length];
  sprintf(result, "%4d%02d%02d %02d:%02d:%02d", year, month, day, hours, minutes, seconds);
  // Serial.printf("year: %4d\n", year);
  // Serial.printf("result: %s\n", result);
  return result;
  // return asctime(&timeinfo);
}
