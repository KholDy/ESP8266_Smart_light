#include <WiFiManager.h>
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#define LED D1
#define LED_STATUS D2

//#define HOTSPOT_NAME "The living room switch!"
#define HOTSPOT_NAME "The bedroom switch"
#define WELCOME "Welcome to the living room switch!\n"\
                "To get state switch light send GET HTTP request: ip/ledState\n"\
                "To turn on/off ligh send POST HTTP request: ip/led"
//#define WELCOME "Welcome to the bedroom switch!\n"\
                "To get state switch light send GET HTTP request: ip/ledState\n"\
                "To turn on/off ligh send POST HTTP request: ip/led"
#define ID 2
#define DESCRIPTION "Main light in the living room"
// #define DESCRIPTION "Main light in the bedroom"

ESP8266WebServer server(80);

DynamicJsonDocument light(512);

//---------------------Check state led------------------------------------------------------------------------------
void getStateLed() {
  light["ip"] = WiFi.localIP();

  if(digitalRead(LED_STATUS)) {
    light["state"] = "off";
  } else if(!digitalRead(LED_STATUS)) {
    light["state"] = "on";
  }

  String buf;
  serializeJson(light, buf);
  server.send(200, F("application/json"), buf);
}

//-----------------Switch LED post request-------------------------------------------------------------------------
void switchLed() {
  light["ip"] = WiFi.localIP();
  String postBody = server.arg("plain");

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);

  if(error) {
    Serial.println(F("Error parsing JSON"));
    Serial.println(error.c_str());

    String msg = error.c_str();

    server.send(400, F("text/html"), "Error in parsing json body!" + msg);
  } else {
    JsonObject postObj = doc.as<JsonObject>();

    if (server.method() == HTTP_POST) {
      if (postObj.containsKey("description") && postObj.containsKey("id")) {
        
          if(postObj["action"] == "off") {
            digitalWrite(LED, HIGH);
            light["state"] = "off";
          } else if(postObj["action"] == "on") {
            digitalWrite(LED, LOW);
            light["state"] = "on";
          }
        
        String buf;
        serializeJson(light, buf);
        server.send(201, F("application/json"), buf);

      } else {
        light["status"] = "error";
        light["message"] = F("No data found, or incorrect!");

        String buf;
        serializeJson(light, buf);
        server.send(400, F("application/json"), buf);
      }
    } 
  }
}

//-----------------------------------------Define routing----------------------------------------------------------
void restServerRouting() {
  server.on("/", HTTP_GET, []() {
    server.send(200, F("text/html"),
      F(WELCOME));
  });
  // handle post request
  server.on(F("/ledState"), HTTP_GET, getStateLed);
  server.on(F("/led"), HTTP_POST, switchLed);
}

// Manage not found URL
void handleNotFound() {
  String message = "File Not Found\n\n";
  message += "URI: ";
  message += server.uri();
  message += "\nMethod: ";
  message += (server.method() == HTTP_GET) ? "GET" : "POST";
  message += "\nArguments: ";
  message += server.args();
  message += "\n";
  for (uint8_t i = 0; i < server.args(); i++) {
    message += " " + server.argName(i) + ": " + server.arg(i) + "\n";
  }
  server.send(404, "text/plain", message);
}

//----------------------------------------------------------------------------------------------------------------
void setup() {
  light["id"] = ID;
  light["description"] = DESCRIPTION;
  light["ip"] = "null";
  light["state"] = "null";
  light["action"] = "null"; 

  pinMode(LED, OUTPUT);
  pinMode(LED_STATUS, INPUT);

  WiFi.mode(WIFI_STA);

    //WiFiManager, Local intialization. Once its business is done, there is no need to keep it around
    WiFiManager wm;

    // reset settings - wipe stored credentials for testing
    // these are stored by the esp library
    //wm.resetSettings();

    // Automatically connect using saved credentials,
    // if connection fails, it starts an access point with the specified name ( "AutoConnectAP"),
    // if empty will auto generate SSID, if password is blank it will be anonymous AP (wm.autoConnect())
    // then goes into a blocking loop awaiting configuration and will return success result

    bool res;
    //res = wm.autoConnect(); // auto generated AP name from chipid
    //res = wm.autoConnect("Kitchen-light"); // anonymous ap
    res = wm.autoConnect(HOTSPOT_NAME,"password"); // password protected ap

    if(!res) {
        Serial.println("Failed to connect");
        // ESP.restart();
    } 
    else {
        //if you get here you have connected to the WiFi    
        Serial.println("connected...yeey :)");
    }

    // Set server routing
    restServerRouting();
    // Set not found response
    server.onNotFound(handleNotFound);
    // Start server
    server.begin();
}

//----------------------------------------------------------------------------------------------------------------
void loop() {
  server.handleClient();
}