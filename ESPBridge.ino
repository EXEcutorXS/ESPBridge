#include <Arduino.h>

#include <ESP8266WiFi.h>
#include <ESP8266HTTPClient.h>
#include <WiFiServer.h>
#include <WiFiClient.h>
#include <WiFiClientSecure.h>

#define DUBUG

String command = "";
String data = "";
String ssid = "";
String password = "";
String host = "";
String postData = "";
bool received = false;
int n = 0;
int httpCode = 0;
bool debug = false;
long lastTryWiFi = 0;
long lastCheckOnline = 0;
bool waitForConnection = false;
bool online = false;
bool forceOnlineCheck = false;
long lastContact=0;

void printLongString (String longString)
{
  int pos = 0;
  int len = longString.length ();
  while (pos < len)
    {
      Serial.print (longString[pos++]);
      if (longString[pos] == '\n')
        delay (1);
    }
}
void send(String s)
{
Serial.print("<");
Serial.print(s);
Serial.print(">");
}
void setup ()
{
  Serial.begin (9600);
  WiFi.disconnect ();
  delay(1000);
  send("z");
  WiFi.mode (WIFI_STA);
  pinMode(2, OUTPUT);
}

void statusIndicator()
{
  int period;
  int dutyCycle;


  if (ssid[0]==0 || password[0]==0)
  {
    period = 3000;
    dutyCycle = 0;
  }
  
  if (ssid[0]!=0 && password[0]!=0)
  {
    period = 3000;
    dutyCycle = 200;
  }
  
  if (WiFi.status () == WL_CONNECTED)
  {
    period = 2000;
    dutyCycle = 1000;
  }
  if (online)
  {
    period = 2000;
    dutyCycle = 2000;
  }
  if (millis()-lastContact<1000)
  {
  period=100;
  dutyCycle = 50;
  }
  if (millis()%period<dutyCycle)
  digitalWrite(2, LOW);
  else
  digitalWrite(2, HIGH);
}
  
void loop ()
{

statusIndicator();

  if (Serial.available () > 0)
    {
      char in = Serial.read ();
      switch (in)
      {
        case '<':
          command = "";
          break;
        case '>':
          received = true;
          break;
        default:
          command += in;
          break;
      }
    }

  if (ssid[0] != 0 && password[0] != 0 && WiFi.status () != WL_CONNECTED && (millis () - lastTryWiFi > 10000))
    {
      lastTryWiFi = millis ();
      if (debug)
        Serial.println ("Got SSID and password. Trying to connect(" + ssid + ", " + password + ")");
        else
        send ("f1");
      WiFi.disconnect ();
      delay (100);
      WiFi.begin (ssid, password);
      waitForConnection = true;
    }

  if (WiFi.status () == WL_CONNECTED && waitForConnection == true)
    {
      waitForConnection = false;
      forceOnlineCheck = true;
      if (debug)
      {
      Serial.print ("Connected to WiFi.Rssi: ");
      Serial.print (WiFi.RSSI ());
      Serial.print (" IP:");
      Serial.print (WiFi.localIP ());
      Serial.println (")");
      }
      else
      {
        send ("u1");
        Serial.print("<I");
        Serial.print( WiFi.localIP());
        Serial.print(">");
      }
    }

  if ((WiFi.status () == WL_CONNECTED && online==false && millis () - lastCheckOnline > 60000) || forceOnlineCheck)
    {

      HTTPClient http;
      lastCheckOnline = millis ();
      String URL = "http://www.google.ru";
      forceOnlineCheck = false;
      http.begin (URL);
      httpCode = http.GET ();
      if (httpCode == 200 || httpCode == 301 || httpCode == 302)
        {
          online = true;
          if (debug)
            Serial.println ("Online check ok...(" + URL + " " + ssid + "," + password + ") Code:" + httpCode);
          else
            send ("o1");
        }
      else
        {
          online = false;
          if (debug)
            Serial.println ("Online check fail...(" + URL + " " + ssid + "," + password + ") Code:" + httpCode);
          else
            send ("o0");

        }
      http.end ();
    }

  if (received)
    {
      received = false;
      data = command.substring (1);
      if (debug && command[0]!='d')
      {
      Serial.print ("/Received command:");
      Serial.println (command);
      Serial.println (data);
      }
      
      switch (command[0])
      {
        case 'Z':
          ssid = data;
          WiFi.disconnect ();
          if (debug)
            {
              Serial.print ("SSID set to:");
              Serial.println (ssid);
            }
          break;
        case 'X':
          password = data;
          WiFi.disconnect ();
          if (debug)
            {
              Serial.print ("Password set to:");
              Serial.println (password);
            }
          break;
        case 'G':
          {
              {
                HTTPClient http;
                String URL = host + data;
                long lastCheckOnline = millis ();
                if (debug)
                  {
                    Serial.print ("Sending:");
                    Serial.println (URL);
                  }

                http.begin (URL);
                httpCode = http.GET();
                
                if (httpCode == 200)
                  {
                    lastContact=millis();
                    online = true;
                    String payload = http.getString();
                    if (debug)
                    {
                    Serial.println("/HTTP response:");
                    printLongString (payload);
                    }
                    else
                    {
                    send("K"+payload);
                    }
                    Serial.println();
                  }
                else
                  {
                    forceOnlineCheck=true;
                    if(debug)
                    {
                    Serial.print ("Error code:");
                    Serial.println (httpCode);
                    }
                    else
                    {
                     send("N");
                    }
                  }
                http.end ();
              }
            break;
          }
            case 'F':
          {
              {
                HTTPClient http;
                String URL = host + data;
                long lastCheckOnline = millis ();
                if (debug)
                  {
                    Serial.print ("/Sending:");
                    Serial.println (URL);
                  }

                http.begin (URL);
                httpCode = http.GET ();
                
                if (httpCode == 200)
                  {
                    lastContact=millis();
                    String payload = http.getString();
                    if (debug)
                    Serial.print("/First string of response:");
                    else
                    Serial.print("<D");
                    online = true;
                    int pos=0;
                    while(payload[pos]!=13&&payload[pos]!=0&&payload[pos]!=10)
                    Serial.print(payload[pos++]);
                    Serial.print(">");
                  }
                else
                  {
                    forceOnlineCheck = true;
                    if(debug)
                    {
                    Serial.print ("/Http request failed. Error code: ");
                    Serial.println (httpCode);
                    }
                    else
                    {
                     send ("N");
                    }
                  }
                http.end ();
              }
            break;
          }
        case 'P':
          {
              {
                HTTPClient http;
                String URL = data;
                long lastCheckOnline = millis ();
                if (debug)
                  {
                    Serial.print ("Sending:");
                    Serial.println (URL);
                  }
                http.begin (host);
                http.addHeader ("Content-Type", "application/x-www-form-urlencoded");
                httpCode = http.POST (postData);
                if (debug)
                  Serial.printf ("[HTTP] GET... code: %d\n", httpCode);
                if (httpCode == 200 || httpCode == 301 || httpCode == 302)
                  {
                    online = true;
                    String payload = http.getString ();
                    if (debug)
                      printLongString (payload);
                    else
                      {
                        printLongString ("K" + payload);
                        Serial.println ();
                      }
                  }
                else
                  {
                    Serial.print ("Error code:");
                    Serial.println (httpCode);
                  }
                http.end ();
              }
          }
          break;
        case 'M':
          {
            ESP.restart ();
          }
          break;
        case 'W':
          {
            if (debug)
              Serial.println ("/Scan start");
            n = WiFi.scanNetworks ();
            if (debug)
              Serial.println ("/Scan done");
            if (n == 0)
              {
                if (debug)
                  Serial.println ("/No networks found");
                else
                  Serial.println ("<y0>");
              }
            else
              {
                if (debug)
                  Serial.print ("/Networks found: ");
                else
                  Serial.print ("<y");
                Serial.print (n);
                Serial.print (">");
              }

            for (int i = 0; i < n; ++i)
              {
                if (debug)
                  {
                    Serial.print (i + 1);
                    Serial.print (": ");
                    Serial.print (WiFi.SSID (i));
                    Serial.print (" (");
                    Serial.print (WiFi.RSSI (i));
                    Serial.print (")");
                    Serial.println ((WiFi.encryptionType (i) == ENC_TYPE_NONE) ? " " : "*");
                    delay (10);
                  }
                else
                  {
                    Serial.print ("<e");
                    Serial.print (WiFi.SSID (i));
                    Serial.print (">");
                  }
              }
          }
          break;
        case 'J':
          host = data;
          break;
        case 'd':
        if (data[0]=='0')
          debug = false;
          else if (data[0]=='1')
          {
          debug = true;
          Serial.println ("Switched to debug mode");
          }
          break;
        case 'b':
          if (WiFi.status () == WL_CONNECTED)
            {
              if (debug)
                Serial.print ("RSSI: ");
              else
                Serial.print ("b");
              Serial.println (WiFi.RSSI ());
            }
          else
            {
              if (debug)
                Serial.println ("WiFi:Disconnected");
              else
                Serial.print ("<u0>");
            }
            break;
        case 't':
          if (debug)
            {
              if (online)
                Serial.println ("Internet status: Online");
              else
                Serial.println ("Internet status: Offline");
              if (WiFi.status () == WL_CONNECTED)
                {
                  Serial.print ("Wifi connection: Connected, IP: ");
                  Serial.print (WiFi.localIP ());
                  Serial.println (")");
                }
              else
                Serial.println ("Wifi connection: Disconnected");
              if (ssid[0] != 0 && password[0] != 0)
                Serial.println ("Settings: Initialized (SSID: " + ssid + ", Password: " + password + ")");
              else
                Serial.println ("Settings: Not Initialized (SSID: " + ssid + ", Password: " + password + ")");
            }
          else
            {
              send("z");
              if (online)
                send ("o1");
              else
                send ("o0");
                delay(100);
              if (WiFi.status () == WL_CONNECTED)
              {
                send ("u1");
                Serial.print("<I");
                Serial.print( WiFi.localIP());
                Serial.print(">");
              }
                
              else
                send ("u0");
                delay(100);
              if (ssid[0] != 0 && password[0] != 0)
                send ("f1");
              else
                send ("f0");
            }
          break;
      }
    }
}
