#include <WiFiClientSecure.h>
#include <credentials.h>

const char *ssid = "your_ssid";
const char *password = "your_password";

// Given below is the CA Certificate of google.com. Replace it with the CA Certificate of your server
const char *ca_cert =
    "-----BEGIN CERTIFICATE-----\n"
    "MIIF2TCCA8ECFCgqMgycxhyJetBluiVzSdR60YYEMA0GCSqGSIb3DQEBCwUAMIGo\n"
    "MQswCQYDVQQGEwJVUzERMA8GA1UECAwIVmlyZ2luaWExEjAQBgNVBAcMCUFubmFu\n"
    "ZGFsZTEgMB4GA1UECgwXQnJlbmRsZXIgQ29uc3VsdGluZyBMTEMxFTATBgNVBAsM\n"
    "DFdlYiBTZXJ2aWNlczEPMA0GA1UEAwwGdGh1dmlhMSgwJgYJKoZIhvcNAQkBFhlq\n"
    "b3NlcGguYnJlbmRsZXJAZ21haWwuY29tMB4XDTIxMDEyMzAxMzgwMVoXDTQwMTAx\n"
    "MDAxMzgwMVowgagxCzAJBgNVBAYTAlVTMREwDwYDVQQIDAhWaXJnaW5pYTESMBAG\n"
    "A1UEBwwJQW5uYW5kYWxlMSAwHgYDVQQKDBdCcmVuZGxlciBDb25zdWx0aW5nIExM\n"
    "QzEVMBMGA1UECwwMV2ViIFNlcnZpY2VzMQ8wDQYDVQQDDAZ0aHV2aWExKDAmBgkq\n"
    "hkiG9w0BCQEWGWpvc2VwaC5icmVuZGxlckBnbWFpbC5jb20wggIiMA0GCSqGSIb3\n"
    "DQEBAQUAA4ICDwAwggIKAoICAQDkGmrnoTPX1bBEQ2b3ROregXIiLx9sUMvk/Qov\n"
    "e1FaGO7LQ/i8TqzyQJX10bd3ud19/xwx+ZkJAmNFX15IOHQ0tIiNpt6v2oaCdndE\n"
    "9CKVvdKns1F10WxeQxBNP8dtmm0QbKpEW4yCG5+blQy6I/plxLauphpTfNp5/VjR\n"
    "imDaQ9UP3LBJrHP0TrvVXXKDF1ozhVcp1emzcn+fJMWatvk42N23fTD2WlcBjQfW\n"
    "yHO8rhDwiwLV0dERfx9kZKyQmPhmqOISN2j9nHV7MjwoxKeDUBLY7IX7wXAaC8ms\n"
    "chMh1Zr/WNU9W8pgrJdkvH5Us0WbOACEfRqFjpPVYq1CHODm4JSmMBUHId+F53W6\n"
    "ZUdgCHhf5nKChyqy3DOV1aYNqoWTq0Y7E1PRhBv7e/TxAPnFz9tx9Bx5IJVFHihz\n"
    "bDokB6lndGsESXAXGXeZ6O1xu+Vau2sPAJi/D2JZqKSxLg1yaktEEHqwbkppuqBb\n"
    "pazWqNZNM+8fW+7YiUPWGDAx3KnnCcGmRxnEQNnOW2MXOo2F5X4WfJDwhwcjEfKP\n"
    "pD6Y5SRxYYCadurxAO0kmIn7N/ATtEQewlq0Rcn1vYy0NvdQWlflbx6f70hNfIzS\n"
    "f/v64YPUUaf6i8m0jJmwZz6GG/SJrX+/HvE9ddkM9ymBIpsf0zxhkEukJWyQ4jDo\n"
    "IGHrWQIDAQABMA0GCSqGSIb3DQEBCwUAA4ICAQCvz+eeIOPgm8PS7ZSLP3NOI/6d\n"
    "hF4ByIaR4rbsB9au2so5+PpOMCDKpqO4oB0eZmvlpjiNp8i2qwLe96fy8wV59jGf\n"
    "O2V6H1DDtUt22f4qneHjWC3l70zPBSSFb2coSit7VRCfq4LrMhgY0irz0SIRsW9m\n"
    "FUsBkTDCMcGQiB+POIgMoet/ZlGeocDZDbD1DU45iFg64sma70zLB193OaO4eqd+\n"
    "Cv4IKWD904TwLm30lMG1waU1cK1zR4zAehzrQCk08YY2QXa/TUaQ4OpXQdePtwYZ\n"
    "AaKpzyj5Waak0PhPlUSf9egbkMJufPs3Q0gQ2LkBAsvE38oFxFWnR8BI0kbDP9yc\n"
    "N0FLM7IM47cjF86DPqaQAXbUB9DKFiczoF9JkO7/bYbjRSk+WPDB6TgONs3AHxBG\n"
    "szzBsMymCuGnmR6tpGvcHCkbetQVRJiMOcuxkpR1YwpjrCzq3JHKcD4zdCah5LjI\n"
    "i8xIZuI5ju6IQFvx1rQ6YNY4RdONe3iUv1OMHLeMJqBpV0NaOIONCef7eBlsPKYu\n"
    "6h1IrBDAVOsEZmuJ5iHnGWB/7SB+DgAvzx0hxxqwbzzwS9sss0uyxbcdH/W/HC4E\n"
    "4SRzCUEWUoLDdexRaCJhjWIFLbusgqSBqJEav2RHpwK8RPQEpybJQZtcZSMrAFc/\n"
    "OGkMjEeyO3YQiD8MrA==\n"
    "-----END CERTIFICATE-----\n";

WiFiClientSecure client;

void setup()
{
  Serial.begin(115200);
  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED)
  {
    delay(500);
    Serial.print(".");
  }
  Serial.println("");
  Serial.print("WiFi connected to: ");
  Serial.println(ssid);
  Serial.println("IP address: ");
  Serial.println(WiFi.localIP());
  client.setCACert(ca_cert);
  delay(2000);
}

void loop()
{
  SendData("thuvia.brendler", 443);
  delay(5000);
}