/*
 * Для прошивки через ESP LINL 1.0 замкнуть IO0 на GND
*/
#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>
#include <ArduinoJson.h>

#define LED 2
#define LED_STATUS 0
#define LIVING_ROOM "Living room"
#define BEDROOM "Bedroom"
#define HALlWAY "Hallway"
#define KITCHEN "Kitchen"

const char* ssid = "KholDy_C80";
const char* password = "38506062";

ESP8266WebServer server(80);

//---------------------Check state led------------------------------------------------------------------------------
void getStateLed() {
  DynamicJsonDocument doc(512);

  if(digitalRead(LED_STATUS)) {
    doc["state"] = "off";
  } else if(!digitalRead(LED_STATUS)) {
    doc["state"] = "on";
  }

  String buf;
  serializeJson(doc, buf);
  server.send(200, F("application/json"), buf);
  Serial.println(buf);
}

//-----------------Switch LED post request-------------------------------------------------------------------------
void switchLed() {
  String postBody = server.arg("plain");
  Serial.println("Post body: ");
  Serial.println(postBody);

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
            doc["state"] = "off";
          } else if(postObj["action"] == "on") {
            digitalWrite(LED, LOW);
            doc["state"] = "on";
          }
        
        String buf;
        serializeJson(doc, buf);

        server.send(201, F("application/json"), buf);

      } else {
        DynamicJsonDocument doc(512);
        doc["status"] = "error";
        doc["message"] = F("No data found, or incorrect!");

        String buf;
        serializeJson(doc, buf);

        server.send(400, F("application/json"), buf);
      }
    } 
  }
}

//-----------------------------------------Define routing----------------------------------------------------------
void restServerRouting() {
  server.on("/", HTTP_GET, []() {
    server.send(200, F("text/html"),
      F("Welcome to the kitchen switch!"));
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
  pinMode(LED, OUTPUT);

  WiFi.mode(WIFI_STA);
  WiFi.begin(ssid, password);
 
  // Wait for connection
  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
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