// Example of the different modes of the X.509 validation options
// in the WiFiClientBearSSL object
//
// Mar 2018 by Earle F. Philhower, III
// Released to the public domain
// Adapted by Joe Brendler Oct 2022

#include <ESP8266WiFi.h>
#include <WiFiClientSecure.h>
#include <StackThunk.h>
#include <time.h>
#include "myNetworkInformation.h"

const char *ssid = mySSID;
const char *pass = myPASSWORD;
const char *path = myPATH;
const char *SSL_host = mySSL_HOST;
const int SSL_port = mySSL_PORT;

// Function dummy declarations (so I can move them below loop())
void setClock();
void separator(String msg);
void fetchURL(BearSSL::WiFiClientSecure *client, const char *host, const uint16_t port, const char *path);

/* Connect using a CA certificate and a custome cipher list --
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

/* Connect using a known key and a custome cipher list --
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
  fetchKnownKeyCustomCipherList();
  delay(500);
  fetchCaCertCustomCipherList();
}

void loop()
{
  // Nothing to do here
}

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

// print a separator in Serial output, for readability
void separator(String msg)
{
  Serial.println("----------[ " + msg + "]----------");
}

// Try and connect using a WiFiClientBearSSL to specified host:port and dump HTTP response
void fetchURL(BearSSL::WiFiClientSecure *client, const char *host, const uint16_t port, const char *path)
{
  if (!path)
  {
    path = "/";
  }

  ESP.resetFreeContStack();
  uint32_t freeStackStart = ESP.getFreeContStack();
  Serial.printf("Trying: %s:%d...", host, port);
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
