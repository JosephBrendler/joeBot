#include <Arduino.h>
//#include <WiFiMulti.h>   // for blocking method
#include <WiFi.h> // for non-blocking method (core wifi library)
#include <WiFi.h>

#define WIFI_SSID "enterSSIDhere"
#define WIFI_PASSWORD "enterPASSWORDhere"

// WiFiMulti wifiMulti;  // for blocking method

bool isConnected = false;
bool toggle = false;
const int LED_ON = HIGH; // digitalWrite active HIGH to turn on LED (8266 is active LOW)
const int LED_OFF = LOW;

void check_status(); // function declaration (see further below)

void setup()
{
  Serial.begin(115200);
  pinMode(LED_BUILTIN, OUTPUT); // on-board wifi status indicator

  WiFi.begin(WIFI_SSID, WIFI_PASSWORD);
  WiFi.setAutoConnect(true);
  WiFi.setAutoReconnect(true);

  Serial.println("\n\nStarting...");
}

void loop()
{
  // blink while connecting; go solid when connected

  if (WiFi.status() == WL_CONNECTED && !isConnected)
  {
    Serial.println("Connected");
    Serial.print("localIP: ");
    Serial.println(WiFi.localIP());
    digitalWrite(LED_BUILTIN, LED_ON);
    isConnected = true;
    delay(1000);
  }

  if (WiFi.status() != WL_CONNECTED)
  {
    Serial.println(".");
    toggle = !toggle;
    digitalWrite(LED_BUILTIN, toggle); // toggle the LED
    isConnected = false;
    delay(250);
  }

  // check_status();
}

void check_status()
{
  Serial.print("Status: ");
  if (WiFi.status() == WL_CONNECTED)
  {
    Serial.print("Connected [");
    Serial.print(WiFi.localIP());
    Serial.println("]");
    digitalWrite(LED_BUILTIN, LED_ON);
    Serial.print("LED_BUILTIN Status [");
    Serial.print(digitalRead(LED_BUILTIN));
    Serial.println("]");
    delay(100);
  }
  else
  {
    Serial.print("Not Connected [Status ");
    Serial.print(WiFi.status());
    Serial.println("]");
    digitalWrite(LED_BUILTIN, LED_OFF);
    Serial.print("LED_BUILTIN Status [");
    Serial.print(digitalRead(LED_BUILTIN));
    Serial.println("]");
    delay(100);
  }
}

/*---------------------------------------------------
void BlockingMethod_setup()
{
  Serial.begin(921600);
  pinMode(LED_BUILTIN, OUTPUT); // on-board wifi status indicator
  wifiMulti.addAP(WIFI_SSID, WIFI_PASSWORD);

  // code for blocking method - won't leave setup until connected
  while (wifiMulti.run() != WL_CONNECTED) {
    delay(100);
  }

  Serial.println("\nConnected");
}

void BlockingMethod_loop()
{
  // write the result of wifi status connection comparison to LED
  // WiFi.status is available because WiFiMulti is built on top of it
  digitalWrite(LED_BUILTIN, WiFi.status() == WL_CONNECTED);

}
------------------------------------------------------*/