#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266WiFiMulti.h>

#include <ESP8266HTTPClient.h>

#include <WiFiClient.h>

ESP8266WiFiMulti WiFiMulti;

String command;
String data;
String ssid;
String password;
String URL;
byte pos;
bool received = false;
int n = 0;
int httpCode = 0;
HTTPClient http;
long lastTryWiFi=0;
long lastCheckOnline=0;
bool waitForConnection=false;
bool online=true;
void
setup ()
{

  Serial.begin (115200);
  //Serial.setDebugOutput(true);

  WiFi.mode (WIFI_STA);
  WiFiMulti.addAP("SSID", "PASSWORD");
}

void
loop ()
{
  if (Serial.available () > 0)
    {
      char in = Serial.read ();
      switch (in)
  {
  case '{':
    pos = 0;
    command = "";
    break;
  case '}':
    received = true;
    break;
  default:
    command[pos++] = in;
    command += in;
    break;
  }
    }

    if (ssid[0]!=0 && password[0]!=0 && WiFi.status() != WL_CONNECTED && (millis()-lastTryWiFi>10000))
    {
      lastTryWiFi=millis();
      Serial.println("Got SSID and password. Trying to connect");
      WiFi.begin(ssid,password);
      waitForConnection=true;
    }

    if(WiFi.status()==WL_CONNECTED && waitForConnection==true)
    {
      waitForConnection=false;
      Serial.println("Connected to WiFi!");
      Serial.println(WiFi.localIP());
    }

    if (WiFi.status() == WL_CONNECTED && millis()-lastCheckOnline>15000)
    {
      lastCheckOnline=millis();
       http.begin("http://ya.ru");
       httpCode = http.GET();
       if (httpCode > 0)
      {
        Serial.printf ("Online Check: [HTTP] GET... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK
      || httpCode == HTTP_CODE_MOVED_PERMANENTLY
      || thhpCode == 301)
    {
      Serial.println("We are online...");
      online=true;
    }
      }
    else
      {
        Serial.printf ("[HTTP] GET... failed, error: %s\n",
           http.errorToString (httpCode).c_str ());
           Serial.println("We are offline...");
           online=false;
      }
    http.end ();
      }
    
  if (received)
    {
      received = false;
      Serial.print ("Received command:");
      Serial.println (command);
      data = command.substring(1);
      Serial.println (data);
      switch (command[0])
  {
  case 'Z':
    ssid = data;
    Serial.print ("SSID set to:");
    Serial.println (ssid);
    break;
  case 'X':
    password = data;
    Serial.print ("Password set to:");
    Serial.println (password);
    break;
  case 'U':
    
    URL = data;
    Serial.print ("Sending:");
    Serial.println (URL);
    http.begin (URL);
    httpCode = http.GET ();
    if (httpCode > 0)
      {
        Serial.printf ("[HTTP] GET... code: %d\n", httpCode);
        if (httpCode == HTTP_CODE_OK
      || httpCode == HTTP_CODE_MOVED_PERMANENTLY)
    {
      String payload = http.getString ();
      Serial.println (payload);
    }
      }
    else
      {
        Serial.printf ("[HTTP] GET... failed, error: %s\n",
           http.errorToString (httpCode).c_str ());
      }
    http.end ();
    break;

  case 'W':
    Serial.println ("scan start");

    // WiFi.scanNetworks will return the number of networks found
    n = WiFi.scanNetworks ();
    Serial.println ("scan done");
    Serial.printf("<w%d>",n);
    if (n == 0)
      {
        Serial.println ("no networks found");
      }
    else
      {
        Serial.print (n);
        Serial.println (" networks found");
        for (int i = 0; i < n; ++i)
    {
      // Print SSID and RSSI for each network found
      Serial.print (i + 1);
      Serial.print (": ");
      Serial.print (WiFi.SSID (i));
      Serial.print (" (");
      Serial.print (WiFi.RSSI (i));
      Serial.print (")");
      Serial.println (
          (WiFi.encryptionType (i) == ENC_TYPE_NONE) ? " " : "*");
      delay (10);
      Serial.print("<e");
      Serial.print(WiFi.SSID(i));
      Serial.println(">");
    }
      }
    break;
    default:
    Serial.println("Unknown command");
  }
    }
}
