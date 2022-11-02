/*----------------------------------------------------------------------------------------------------
  Use modes of the X.509 validation in the WiFiClientBearSSL object
  to connect and exchange information with external web server
  Joe Brendler Oct 2022
  Based largley on Mar 2018 work by Earle F. Philhower, III - Released to the public domain
----------------------------------------------------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include "myNetworkInformation.h"
#include "OTA_Joe2.h"
#include <stdio.h>

const char *ssid = mySSID;
const char *pass = myPASSWORD;
const char *esp_host = myESP_HOST;
const char *SSL_host = mySSL_HOST;
const int SSL_port = mySSL_PORT;
const char *path = myPATH;
const char *protocol = myPROTOCOL;

const int LED_ON = LOW; // ctive high for ESP32; ESP12 8266 is active low)
const int LED_OFF = HIGH;

// time variables used in setting clock and timestamping
time_t now = time(nullptr);
struct tm timeinfo;
char *timeStamp;

// dummy function declarations (so I can move the actual functions below setup() and loop())
char * getTime();
void separator(String msg);
void fetchURL(BearSSL::WiFiClientSecure *client, const char *host, const uint16_t port, const char *path);
void check_status();
void fetchDataWithKnownKeyCustomCipherList();
void uploadDataWithKnownKeyCustomCipherList(char *myDATA);
void blink();
char *build_url(const char *proto, const char *host, const int port, const char *path);

const char *myURL = build_url(protocol, SSL_host, SSL_port, path);

/*----------------------------------------------------------------------------------------------------
 * setup()
 *
 *---------------------------------------------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);
  Serial.println("\n");

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LOW);

  // connect to wifi and set up over the air re-programming
  blink();
  setupOTA(esp_host, ssid, pass); // use unique name each time
  check_status();

  // set NTP time (needed for CA Cert expiry validation)
  delay(100);
  timeStamp = getTime();
  delay(100);
  Serial.println("Current time: " + String(timeStamp));

  const char *placeholdervariable = "this is a yet another test";
  int placeholdersize = strlen(placeholdervariable) + 1;
  char *adcvalue = new char[placeholdersize];
  Serial.println("about to asign placeholdervaroable...");
  sprintf(adcvalue, "%s", placeholdervariable);
  Serial.printf("timeStamp: %s reading = ", timeStamp);
  Serial.printf("%s \nAbout to asign data_constructor...\n", adcvalue);
  int data_len = strlen(timeStamp) + 12 + strlen(adcvalue) + 1;
  char *data_chars = new char[data_len];
  sprintf(data_chars, "%s: reading = %s\n", timeStamp, adcvalue);

  Serial.println("about to call uploadData...");
  uploadDataWithKnownKeyCustomCipherList(data_chars);

  blink();
  check_status();
}

/*----------------------------------------------------------------------------------------------------
 * loop()
 *
 *---------------------------------------------------------------------------------------------------*/
void loop()
{
#if defined(ESP32_RTOS) && defined(ESP32)
#else // If you do not use FreeRTOS, you have to regulary call the handle method.
  ArduinoOTA.handle();
#endif

  // Your code here
  check_status();
}

/*----------------------------------------------------------------------------------------------------*/
// check status of wifi
void check_status()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    //    blink();
    digitalWrite(LED_BUILTIN, LED_ON);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
  delay(100);
}

/*----------------------------------------------------------------------------------------------------*/
// Set time via NTP, as required for x.509 validation
char * getTime()
{
  configTime(int(3 * 60 * 60), 0, "time.nist.gov", "pool.ntp.org");
  Serial.print("Waiting for NTP time sync: ");
  while (now < 8 * 3600 * 2)
  {
    delay(4010); // systems that query for time more frequently than every 4 sec will be refused service
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  gmtime_r(&now, &timeinfo);
  //  Serial.print("Current time: ");
  //  Serial.print(asctime(&timeinfo));
  return asctime(&timeinfo);
}

/*----------------------------------------------------------------------------------------------------*/
// print a separator in Serial output, for readability
void separator(String msg)
{
  Serial.println("----------[ " + msg + " ]----------");
}

/*----------------------------------------------------------------------------------------------------*/
// Blink the built-in LED
void blink()
{
  for (int i = 0; i < 3; i++)
  {
    digitalWrite(LED_BUILTIN, LED_ON);
    delay(100);
    digitalWrite(LED_BUILTIN, LED_OFF);
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
void fetchDataWithKnownKeyCustomCipherList()
{
  separator("fetchDataWithKnownKeyCustomCipherList()");
  HTTPClient https;
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  /* Options for securing vs MITM --
    (1) fingerprint of server cert
    (2) ca cert (needs NTP time)
    (3) known key (Server cert itself)
  //  client->setFingerprint(myFINGERPRINT);
  //  BearSSL::X509List cert(myCERT);
  //  client->setTrustAnchors(&cert); // Note: this method needs time to be set
    (using known key)  */
  BearSSL::PublicKey key(myPUBKEY);
  client->setKnownKey(&key);
  client->setCiphers(myCustomCipherList);
  // fetchURL(&client, SSL_host, SSL_port, path);
  Serial.print("[HTTPS] begin connection to url: " + String(myURL) + "\n");
  if (https.begin(*client, myURL))
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
    client.release();
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
void uploadDataWithKnownKeyCustomCipherList(char *myDATA)
{
  separator("uploadDataWithKnownKeyCustomCipherList()");
  HTTPClient https;
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  /* Options for securing vs MITM --
    (1) fingerprint of server cert
    (2) ca cert (needs NTP time)
    (3) known key (Server cert itself)
  //  client->setFingerprint(myFINGERPRINT);
  //  BearSSL::X509List cert(myCERT);
  //  client->setTrustAnchors(&cert); // Note: this method needs time to be set
    (using known key)  */
  BearSSL::PublicKey key(myPUBKEY);
  client->setKnownKey(&key);
  client->setCiphers(myCustomCipherList);
  // fetchURL(&client, SSL_host, SSL_port, path);
  Serial.print("[HTTPS] begin connection to url: " + String(myURL) + "\n");

  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  
  String postData = "reading=" + String(asctime(&timeinfo)) + String(myDATA);

  if (https.begin(*client, myURL))
  { // HTTPS
    Serial.print("[HTTPS] Sending POST request...\n");

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
        Serial.println("Data upload succeeded, reading getString() method...");
        String payload = https.getString();
        Serial.println("Response to getString(): " + payload);
      }
    }
    else
    {
      Serial.printf("[HTTPS] POST ... failed, error: %s\n", https.errorToString(httpCode).c_str());
    }

    Serial.println("Done with data transfer...");
    delay(100);
    client.release();
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

char *build_url(const char *proto, const char *host, const int port, const char *path)
{
  // allocate memory for url characters -- note the number of
  // digits in an integer is {log(base)(integer) + 1} and the
  // char[] must be terminated with a '\0' char, so again + 1
  int url_size = strlen(protocol) + 3 + strlen(host) + (log10(port) + 1) + strlen(path) + 1;

  char *result = new char[url_size];

  // initialize result with all chars in host[] colon and path[]
  sprintf(result, "%s://%s:%d%s", protocol, host, port, path);
  return result;
}