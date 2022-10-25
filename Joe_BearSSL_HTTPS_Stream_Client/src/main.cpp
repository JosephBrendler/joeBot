/*----------------------------------------------------------------------------------------------------
  Use modes of the X.509 validation in the WiFiClientBearSSL object
  to connect and exchange information with external web server
  Joe Brendler Oct 2022
  Based largley on Mar 2018 work by Earle F. Philhower, III - Released to the public domain
----------------------------------------------------------------------------------------------------*/

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <StackThunk.h>
#include <time.h>
#include <Arduino_JSON.h>
#include <ESP8266HTTPClient.h>
#include "myNetworkInformation.h"

const char *ssid = mySSID;
const char *pass = myPASSWORD;
const char *path = myPATH;
const char *SSL_host = mySSL_HOST;
const int SSL_port = mySSL_PORT;

// Used for millisecond count of timeouts (big integers)
unsigned long lastTime = 0;
// Timer set to 10 minutes (600000)
// unsigned long timerDelay = 600000;
// Set timer to 5 seconds (5000)
unsigned long timerDelay = 5000;

// Function dummy declarations (so I can move them below loop())
void setClock();
void separator(String msg);
void fetchURL(BearSSL::WiFiClientSecure *client, const char *host, const uint16_t port, const char *path);

/*----------------------------------------------------------------------------------------------------
Connect insecure (do not use except for testing pysical connectivity)
This is absolutely *insecure*, but you can tell BearSSL not to check the
certificate of the server.  In this mode it will accept ANY certificate,
which is subject to man-in-the-middle (MITM) attacks. */
void fetchInsecure()
{
  separator("fetchInsecure()");
  BearSSL::WiFiClientSecure client;
  client.setInsecure();
  fetchURL(&client, SSL_host, SSL_port, path);
}

/*----------------------------------------------------------------------------------------------------
Connect using the SHA-1 fingerprint to validate an X.509 certificate
instead of using the whole certificate.  This is not nearly as secure as real
X.509 validation, but is better than nothing.  Also be aware that these
fingerprints will change if anything changes in the certificate chain
(i.e. re-generating the certificate for a new end date, any updates to
the root authorities, etc.).*/
void fetchFingerprint()
{
  separator("fetchFingerprint()");
  BearSSL::WiFiClientSecure client;
  client.setFingerprint(myFINGERPRINT);
  fetchURL(&client, SSL_host, SSL_port, path);
}

void fetchDataWithFingerprint()
{
  separator("fetchDataWithFingerprint()");
  HTTPClient https;
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  //  BearSSL::WiFiClientSecure client;
  client->setFingerprint(myFINGERPRINT);
  //  fetchURL_for_C2(&https, &client, SSL_host, SSL_port, path);
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
      Serial.printf("[HTTPS] GET returns code: %d\n", httpCode);
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
    delay(2000);
    client.release();
    https.end();
    delay(2000);
    Serial.println("ended https...");
  }
  else
  {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  Serial.println("done with function...");
}

/*----------------------------------------------------------------------------------------------------
Connect using a CA certificate and a custome cipher list --
** CA Certificate:  A specific certification authority can be passed in and used to validate
a chain of certificates from a given server.  These will be validated
using BearSSL's rules, which do NOT include certificate revocation lists.
A specific server's certificate, or your own self-signed root certificate
can also be used.  ESP8266 time needs to be valid for checks to pass as
BearSSL does verify the notValidBefore/After fields.
** The ciphers used to set up the SSL connection can be configured to
only support faster but less secure ciphers  -- If you care more about security
you won't want to do this, but if you need to maximize battery life, these
may make sense. */
void fetchCaCertCustomCipherList()
{
  separator("fetchCaCertCustomCipherList()");
  BearSSL::WiFiClientSecure client;
  BearSSL::X509List cert(myCERT);
  client.setTrustAnchors(&cert); // Note: this method needs time to be set
  client.setCiphers(myCustomCipherList);
  fetchURL(&client, SSL_host, SSL_port, path);
}

void fetchDataWithCaCertCustomCipherList()
{
  separator("fetchDataWithCaCertCustomCipherList()");
  HTTPClient https;
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
//  BearSSL::WiFiClientSecure client;
  BearSSL::X509List cert(myCERT);
  client->setTrustAnchors(&cert); // Note: this method needs time to be set
  client->setCiphers(myCustomCipherList);
  //  fetchURL_for_C2(&https, &client, SSL_host, SSL_port, path);
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
      Serial.printf("[HTTPS] GET returns code: %d\n", httpCode);
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
    delay(2000);
    client.release();
    https.end();
    delay(2000);
    Serial.println("ended https...");
  }
  else
  {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  Serial.println("done with function...");
}

/*----------------------------------------------------------------------------------------------------
Connect using a known key and a custome cipher list --
** Known key:  The server certificate can be completely ignored, with its public key
hardcoded in the application. This should be secure as the public key
needs to be paired with the private key of the site, which is obviously
private and not shared.  A MITM without the private key would not be
able to establish communications.
** The ciphers used to set up the SSL connection can be configured to
only support faster but less secure ciphers  -- If you care more about security
you won't want to do this, but if you need to maximize battery life, these
may make sense. */
void fetchKnownKeyCustomCipherList()
{
  separator("fetchKnownKeyCustomCipherList()");
  BearSSL::WiFiClientSecure client;
  BearSSL::PublicKey key(myPUBKEY);
  client.setKnownKey(&key);
  client.setCiphers(myCustomCipherList);
  fetchURL(&client, SSL_host, SSL_port, path);
}

void fetchDataWithKnownKeyCustomCipherList()
{
  separator("fetchDataWithKnownKeyCustomCipherList()");
  HTTPClient https;
  std::unique_ptr<BearSSL::WiFiClientSecure> client(new BearSSL::WiFiClientSecure);
  BearSSL::PublicKey key(myPUBKEY);
  client->setKnownKey(&key);
  client->setCiphers(myCustomCipherList);
  //fetchURL(&client, SSL_host, SSL_port, path);
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
      Serial.printf("[HTTPS] GET returns code: %d\n", httpCode);
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
    delay(2000);
    client.release();
    https.end();
    delay(2000);
    Serial.println("ended https...");
  }
  else
  {
    Serial.printf("[HTTPS] Unable to connect\n");
  }
  Serial.println("done with function...");
}

/*----------------------------------------------------------------------------------------------------
 * setup()
 *
 *---------------------------------------------------------------------------------------------------*/
void setup()
{
  Serial.begin(115200);
  Serial.println("\n");

  // We start by connecting to a WiFi network
  Serial.print("Connecting to " + String(ssid) + ".");
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, pass);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.print("\nWiFi connected on IP address: ");
  Serial.println(WiFi.localIP());

  delay(500);
  setClock();
  delay(500);

  fetchDataWithKnownKeyCustomCipherList();
}

/*----------------------------------------------------------------------------------------------------
 * loop()
 *
 *---------------------------------------------------------------------------------------------------*/
void loop()
{
  // Nothing to do here
}

/*----------------------------------------------------------------------------------------------------*/
// Set time via NTP, as required for x.509 validation
void setClock()
{
  configTime(int(3 * 60 * 60), 0, "time.nist.gov", "pool.ntp.org");
  // configTime(int(3 * 60 * 60), 0, "oromis.brendler");

  Serial.print("Waiting for NTP time sync: ");
  time_t now = time(nullptr);
  while (now < 8 * 3600 * 2)
  {
    delay(4010); // systems that query for time more frequently than every 4 sec will be refused service
    Serial.print(".");
    now = time(nullptr);
  }
  Serial.println("");
  struct tm timeinfo;
  gmtime_r(&now, &timeinfo);
  Serial.print("Current time: ");
  Serial.print(asctime(&timeinfo));
}

/*----------------------------------------------------------------------------------------------------*/
// print a separator in Serial output, for readability
void separator(String msg)
{
  Serial.println("----------[ " + msg + " ]----------");
}

/*----------------------------------------------------------------------------------------------------*/
// Try and connect using a WiFiClientBearSSL to specified host:port and dump HTTP response
void fetchURL(BearSSL::WiFiClientSecure *client, const char *host, const uint16_t port, const char *path)
{
  if (!path)
  {
    path = "/";
  }

  ESP.resetFreeContStack();
  uint32_t freeStackStart = ESP.getFreeContStack();
  Serial.printf("Trying: %s:%d%s...", host, port, path);
  client->connect(host, port);
  if (!client->connected())
  {
    Serial.printf("*** Can't connect. ***\n-------\n");
    return;
  }
  Serial.printf("Connected!\n-------\n");
  client->write("GET ");
  client->write(path);
  client->write(" HTTP/1.0\r\nHost: ");
  client->write(host);
  client->write("\r\nUser-Agent: ESP8266\r\n");
  client->write("\r\n");
  uint32_t to = millis() + 5000;
  if (client->connected())
  {
    do
    {
      char tmp[32];
      memset(tmp, 0, 32);
      int rlen = client->read((uint8_t *)tmp, sizeof(tmp) - 1);
      yield();
      if (rlen < 0)
      {
        break;
      }
      // Only print out first line up to \r, then abort connection
      char *nl = strchr(tmp, '\r');
      if (nl)
      {
        *nl = 0;
        Serial.print(tmp);
        break;
      }
      Serial.print(tmp);
    } while (millis() < to);
  }
  client->stop();
  uint32_t freeStackEnd = ESP.getFreeContStack();
  Serial.printf("\nCONT stack used: %d\n", freeStackStart - freeStackEnd);
  Serial.printf("BSSL stack used: %d\n-------\n\n", stack_thunk_get_max_usage());
}