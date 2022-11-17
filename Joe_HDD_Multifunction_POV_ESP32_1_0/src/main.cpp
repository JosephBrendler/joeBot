// This is an attempt to "port" my HDD POV "Clock" project from atmega328P to ESP32
// synchronous led display of multi-function data using spinning slotted disk
// Joe Brendler - 7 Nov 2022 (based on Arduino mini project of 2013-2022)
// Rev 1.0 ESP32 -- starting from scratch
#include <Arduino.h>
#include <WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include <HTTPClient.h>
#include "myNetworkInformation.h"
#include "OTA_Joe2.h"
#include <stdio.h>

#define timeInterval 5 // used to define to keep output (e.g. relay) "on" after trigger

const char *ssid = mySSID;
const char *pass = myPASSWORD;
const char *esp_host = myESP_HOST;
const char *SSL_host = mySSL_HOST;
const int SSL_port = mySSL_PORT;
const char *logPath = myLogFormPATH;
const char *statusPath = myStatusFormPATH;
const char *protocol = myPROTOCOL;

// identify digital inputs
const gpio_num_t photoTrigger = GPIO_NUM_27; // gpio 27; pin 11

// identify digital outputs
const gpio_num_t RED_LED = GPIO_NUM_34;   // gpio 34; pin 5
const gpio_num_t GREEN_LED = GPIO_NUM_35; // gpio 35; pin 6
const gpio_num_t BLUE_LED = GPIO_NUM_32;  // gpio 32; pin 7
const gpio_num_t OTA_LED = GPIO_NUM_2;    // LED_BUILTIN

const int OTA_LED_ON = HIGH; // active high for ESP32; ESP12 8266 is active low)
const int OTA_LED_OFF = LOW;
const long clockSlotWidth = 100; // microseconds

// time variables used in setting clock and timestamping
time_t timeNow = time(nullptr);
struct tm timeinfo;
char *timeStamp;

int data_len = 0;
const char *i_log_message = "ESP32 HDD POV Clock Initialized";
const char *i_status_message = "Initialized";

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

int interruptCount = 0;

// dummy function declarations (so I can move the actual functions below setup() and loop())
void setMyTime();
void printLocalTime();
char *getTimeStamp();
void separator(String msg);
void check_status();
void fetchDataWithKnownKey();
void uploadDataWithKnownKey(const char *uploadURL, const char *fieldName, char *myDATA);
void blink(gpio_num_t LED, int onMicros);
char *build_url(const char *proto, const char *host, const int port, const char *logPath);

const char *myLogURL = build_url(protocol, SSL_host, SSL_port, logPath);
const char *myStatusURL = build_url(protocol, SSL_host, SSL_port, statusPath);

//--- ran into trouble using some gpio pins - configure here
#define OUTPUT_BIT_MASK ((1ULL << RED_LED) | (1ULL << GREEN_LED) | (1ULL << BLUE_LED) | (1ULL < OTA_LED))
#define INPUT_BIT_MASK (1ULL << photoTrigger)

//-------------------- Define interrupt functions up front -----------------------------------------------
// Checks if motion was detected, sets trigger_led HIGH and starts a timer
void IRAM_ATTR trigger()
{
  triggered = true;
}

/*----------------------------------------------------------------------------------------------------
 * setup()
 *
 *---------------------------------------------------------------------------------------------------*/
void setup()
{
  //--- ran into trouble using some gpio pins - configure here
  gpio_config_t input_config;
  // config inputs
  input_config.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_NEGEDGE; // enable interrupt, falling edge
  input_config.mode = GPIO_MODE_INPUT;                             // input for photo trigger
  input_config.pin_bit_mask = INPUT_BIT_MASK;                      // (above) enables rgb led pins
  input_config.pull_down_en = GPIO_PULLDOWN_DISABLE;               // disable pulldown
  input_config.pull_up_en = GPIO_PULLUP_ENABLE;                    // enable pullup
  gpio_config(&input_config);
  gpio_config_t output_config;
  // config outputs
  output_config.intr_type = (gpio_int_type_t)GPIO_PIN_INTR_DISABLE; // disable interrupts
  output_config.mode = GPIO_MODE_OUTPUT;
  output_config.pin_bit_mask = OUTPUT_BIT_MASK; // (above) enables rgb led pins
  output_config.pull_down_en = GPIO_PULLDOWN_DISABLE;
  output_config.pull_up_en = GPIO_PULLUP_DISABLE;
  gpio_config(&output_config);

  Serial.begin(115200);
  Serial.println("\n");

  // to do - disable interrupts

  // PIR Motion Sensor mode INPUT_PULLUP
  //    pinMode(photoTrigger, INPUT_PULLUP);
  // Set motionSensor pin as interrupt, assign interrupt function and set RISING mode
  attachInterrupt(digitalPinToInterrupt(photoTrigger), trigger, FALLING);

  // Set LEDs to LOW
  //    pinMode(OTA_LED, OUTPUT);
  gpio_set_level(OTA_LED, LOW);
  //    pinMode(RED_LED, OUTPUT);
  gpio_set_level(RED_LED, HIGH);
  //    pinMode(GREEN_LED, OUTPUT);
  gpio_set_level(GREEN_LED, LOW);
  //    pinMode(BLUE_LED, OUTPUT);
  gpio_set_level(BLUE_LED, LOW);

  // connect to wifi and set up over the air re-programming
  blink(OTA_LED, 100000);
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

  blink(OTA_LED, 10000);
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
    lastTrigger = millis();
    startTimer = true;
    triggered = false;
    interruptCount++;
    // Serial.printf("Interrupt# [%d] just received\n", interruptCount);
    blink(RED_LED, clockSlotWidth);
  }

  // notify and stop the timer when appropriate (nothing happens until then)
  if (startTimer && ((millis() - lastTrigger) > (timeInterval * 1000)))
  {
    startTimer = false;
    //  Serial.printf("millis(): %d\nlastTrigger: %d\ntimeInterval*1000: %d\n", millis(), lastTrigger, (timeInterval * 1000));
    //  Serial.printf("Current accumulated interrupt count: %d\n", interruptCount);
    // Serial.printf("#: %d\n", interruptCount);
  }

  // Remainder of routine loop code here
  // Serial.println("In loop()");
  // delay(1000);
  check_status();
}

/*----------------------------------------------------------------------------------------------------*/
// check status of wifi
void check_status()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    gpio_set_level(OTA_LED, OTA_LED_ON);
  }
  else
  {
    gpio_set_level(OTA_LED, OTA_LED_OFF);
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
void blink(gpio_num_t LED, int onMicros)
{
  for (int i = 0; i < 3; i++)
  {
    gpio_set_level(LED, OTA_LED_ON);
    delayMicroseconds(onMicros);
    gpio_set_level(LED, OTA_LED_OFF);
  }
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
