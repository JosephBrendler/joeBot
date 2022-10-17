/*********
  ESP32 Web server
  Joe Brendler - 10/15/2022
  Built on a foundation of various tutorials including Tomasz Tarnowski's getting started
  and WiFi Connect, a number of OTA methods, etc.
  "Implementa un servidor web al que se accede mediante una conección WiFi"
  Based on work by Osvaldo Cantone  correo@cantone.com.ar
  from Ramos Mejía, ARGENTINA, Agosto 2020
  Cantone also credits sketches developed by Rui Santos https://randomnerdtutorials.com
  */
#include <Arduino.h>
//#define ESP32_RTOS // Uncomment this line if you want to use the code with freertos only on the ESP32
// Has to be done before including "OTA.h"

#include "OTA.h"
#include <credentials.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>

const int LED_ON = HIGH; // ctive high for ESP32; ESP12 8_16 is active low)
const int LED_OFF = LOW;

// function definition (see function further below)
void check_wifi_status();
void handle_web_client();

// AsyncWebServer webServer(80);  //used in OTA via web
//  Instatiate web server via port 80
WiFiServer wifiServer(80);

// This variable will hold our HTTP request
String header;

// These variables hold the logical state of two digital relays
String output_1_State = "off";
String output_2_State = "off";

// The two relays are mapped to the following GPIO pins
// Note LED_BUILTIN uses GPIO2, so don't use 2 here
const int output_1_ = 15; // GPIO15 Pin J2-16
const int output_2_ = 13; // GPIO13 Pin J1-15

// Current time
unsigned long currentTime = millis();
// Previous time
unsigned long previousTime = 0;
// Define timeout time in milliseconds (example: 2000ms = 2s)
const long timeoutTime = 2000;
int show = -1;

void setup()
{
  int error;

  pinMode(LED_BUILTIN, OUTPUT);
  digitalWrite(LED_BUILTIN, LED_OFF);

  Serial.begin(115200);
  Serial.println("Booting");

  // Initialize the relays to an "ON" state
  pinMode(output_1_, OUTPUT);
  pinMode(output_2_, OUTPUT);
  digitalWrite(output_1_, HIGH);
  digitalWrite(output_2_, HIGH);

  // Note that OTA turns on wifi, using credentials.h
  // Specify hostname here (use unique name each time)
  setupOTA("JoeESP32_WebServer_01", mySSID, myPASSWORD);

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(WiFi.SSID());
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());

  /*
    webServer.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
              { request->send(200, "text/plain", "Hi! I am ESP32."); });

   // AsyncElegantOTA.begin(&server); // Start ElegantOTA
    webServer.begin();
    Serial.println("HTTP webServer started");
  */
  // start wifi web server
  wifiServer.begin();
}

void loop()
{
//#ifdef defined(ESP32_RTOS) && defined(ESP32)
#if defined(ESP32_RTOS) && defined(ESP32)
#else // If you do not use FreeRTOS, you have to regulary call the handle method.
  ArduinoOTA.handle();
#endif

  // Your code here
  check_wifi_status();
  handle_web_client();
}

void check_wifi_status()
{
  if (WiFi.status() == WL_CONNECTED)
  {
    digitalWrite(LED_BUILTIN, LED_ON);
  }
  else
  {
    digitalWrite(LED_BUILTIN, LED_OFF);
  }
  delay(100);
}

void handle_web_client()
{
  WiFiClient client = wifiServer.available(); // Listen for incoming clients

  if (client)
  { // If a new client connects,
    currentTime = millis();
    previousTime = currentTime;
    Serial.println("New Client."); // print a message out in the serial port
    String currentLine = "";       // make a String to hold incoming data from the client
    while (client.connected() && currentTime - previousTime <= timeoutTime)
    { // loop while the client's connected
      currentTime = millis();
      if (client.available())
      {                         // if there's bytes to read from the client,
        char c = client.read(); // read a byte, then
        Serial.write(c);        // print it out the serial monitor
        header += c;
        if (c == '\n')
        { // if the byte is a newline character
          // if the current line is blank, you got two newline characters in a row.
          // that's the end of the client HTTP request, so send a response:
          if (currentLine.length() == 0)
          {
            // HTTP headers always start with a response code (e.g. HTTP/1.1 200 OK)
            // and a content-type so the client knows what's coming, then a blank line:
            client.println("HTTP/1.1 200 OK");
            client.println("Content-type:text/html");
            client.println("Connection: close");
            client.println();

            // turns the GPIOs on and off
            if (header.indexOf("GET /_1_/on") >= 0)
            {
              Serial.println("GPIO _1_ on");
              output_1_State = "on";
              digitalWrite(output_1_, HIGH);
            }
            else if (header.indexOf("GET /_1_/off") >= 0)
            {
              Serial.println("GPIO _1_ off");
              output_1_State = "off";
              digitalWrite(output_1_, LOW);
            }
            else if (header.indexOf("GET /_2_/on") >= 0)
            {
              Serial.println("GPIO _2_ on");
              output_2_State = "on";
              digitalWrite(output_2_, HIGH);
            }
            else if (header.indexOf("GET /_2_/off") >= 0)
            {
              Serial.println("GPIO _2_ off");
              output_2_State = "off";
              digitalWrite(output_2_, LOW);
            }

            // Display the HTML web page
            client.println("<!DOCTYPE html><html>");
            client.println("<head><meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">");
            client.println("<link rel=\"icon\" href=\"data:,\">");
            // CSS to style the on/off buttons
            // Feel free to change the background-color and font-size attributes to fit your preferences
            client.println("<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;}");
            client.println(".button { background-color: #4CAF50; border: none; color: white; padding: 16px 40px;");
            client.println("text-decoration: none; font-size: 30px; margin: 2px; cursor: pointer;}");
            client.println(".button2 {background-color: #555555;}</style></head>");

            // Web Page Heading
            client.println("<body><h1>ESP32 Web Server</h1>");
            client.println("<p>Adapted by Osvaldo Cantone <a href=https://github.com/ocantone/ESP32-Relay-LCD-Display-I2C> GItHub/ocantone </a> </p>");

            // Display current state, and ON/OFF buttons for GPIO 15
            client.println("<p>GPIO _1_ - State " + output_1_State + "</p>");
            // If the output_1_State is off, it displays the ON button
            if (output_1_State == "off")
            {
              client.println("<p><a href=\"/_1_/on\"><button class=\"button\">ON</button></a></p>");
            }
            else
            {
              client.println("<p><a href=\"/_1_/off\"><button class=\"button button2\">OFF</button></a></p>");
            }

            // Display current state, and ON/OFF buttons for GPIO 2
            client.println("<p>GPIO _2_ - State " + output_2_State + "</p>");
            // If the output_2_State is off, it displays the ON button
            if (output_2_State == "off")
            {
              client.println("<p><a href=\"/_2_/on\"><button class=\"button\">ON</button></a></p>");
            }
            else
            {
              client.println("<p><a href=\"/_2_/off\"><button class=\"button button2\">OFF</button></a></p>");
            }
            client.println("</body></html>");

            // The HTTP response ends with another blank line
            client.println();
            // Break out of the while loop
            break;
          }
          else
          { // if you got a newline, then clear currentLine
            currentLine = "";
          }
        }
        else if (c != '\r')
        {                   // if you got anything else but a carriage return character,
          currentLine += c; // add it to the end of the currentLine
        }
      }
    }
    // Clear the header variable
    header = "";
    // Close the connection
    client.stop();
    Serial.println("Client disconnected.");
    Serial.println("");
  }
}
