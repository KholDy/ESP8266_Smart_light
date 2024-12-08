#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266mDNS.h>

#include <ArduinoJson.h>

#include <DNSServer.h>
#include <ElegantOTA.h>

#include <WiFiManager.h>

#define LED 2
#define LED_STATUS 0
#define RELAY 5

#define LIVING_ROOM "Living room"
#define BEDROOM "Bedroom"
#define HALlWAY "Hallway"
#define KITCHEN "Kitchen"

ESP8266WebServer server(80);

const char * welcomePage = "<!DOCTYPE html>"
                        "<html>"
                          "<head>"
                            "<title>Welcome in smart light</title>"
                            "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\">"
                            "<link rel=\"icon\" href=\"data:,\">"
                            "<style>html { font-family: Helvetica; display: inline-block; margin: 0px auto; text-align: center;" 
                            " p,li {font: 1rem 'Fira Sans', sans-serif;} p { font-weight: bold;}} </style>"
                          "</head>"
                          "<body>"
                            "<h2>ESP8266 SMART LIGHT</h2>"
                              "<div class=\"content\">"
                                "<div class=\"card-grid\">"
                                  "<div class=\"card\">"
                                    "<div>"
                                      "<p>Welcome to the kitchen switch!</p>"
                                      "<div style=\"display: flex; justify-content: center; text-align: left; height: 100px;\">"
                                        "<ul>"
                                          "<li>To update the firmware, use the GET method with endpoint ip-device/udate</li>"
                                          "<li>To view information about the esp, use the GET method with endpoint ip-device/info</li>"
                                          "<li>To get the state of light, use the GET method with endpoint ip-device/ledState</li>"
                                          "<li>To the switch of light, use the PUT method with endpoint ip-device/led</li>"
                                        "</ul>"
                                      "</div>"
                                    "</div>"
                                  "</div>"
                                "</div>"
                              "</div>"
                          "</body>"
                        "</html>";

// Check state led
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
}

void getInfo() {
      DynamicJsonDocument doc(512);
 
      doc["ip"] = WiFi.localIP().toString();
      doc["gw"] = WiFi.gatewayIP().toString();
      doc["nm"] = WiFi.subnetMask().toString();
      doc["signalStrengh"] = WiFi.RSSI();
      doc["chipId"] = ESP.getChipId();
      doc["flashChipId"] = ESP.getFlashChipId();
      doc["flashChipSize"] = ESP.getFlashChipSize();
      doc["flashChipRealSize"] = ESP.getFlashChipRealSize();
      doc["freeHeap"] = ESP.getFreeHeap();
      
      String buf;
      serializeJson(doc, buf);
      server.send(200, F("application/json"), buf);
}

//Switch LED post request
void switchLed() {
  String postBody = server.arg("plain");
  Serial.println(postBody);

  DynamicJsonDocument doc(512);
  DeserializationError error = deserializeJson(doc, postBody);

  if(error) {
    Serial.println(F("Error parsing JSON"));
    Serial.println(error.c_str());

    String msg = error.c_str();

    server.send(400, F("text/html"), "Error in parsin json body! <br>" + msg);
  } else {
    JsonObject postObj = doc.as<JsonObject>();

    if (server.method() == HTTP_PUT) {
      if (postObj.containsKey("name") && postObj.containsKey("id")) {
        
          if(postObj["action"] == "off") {
            digitalWrite(LED, HIGH);
            digitalWrite(RELAY, HIGH);
            /*
             * Поставить перемычку 0-5 для контроля срабатывания реле
             * и написать проверку срабатывания по пину 0
             */
            doc["state"] = "off";
          } else if(postObj["action"] == "on") {
            digitalWrite(LED, LOW);
            digitalWrite(RELAY, LOW);
            doc["state"] = "on";
          }
        
        String buf;
        serializeJson(doc, buf);

        server.send(201, F("application/json"), buf);

      }else {
        DynamicJsonDocument doc(512);
        doc["status"] = "KO";
        doc["message"] = F("No data found, or incorrect!");

        String buf;
        serializeJson(doc, buf);

        server.send(400, F("application/json"), buf);
      }
    } 
  }
}
 
// Define routing
void restServerRouting() {
  server.on("/", HTTP_GET, []() {
      server.send(200, F("text/html"),
          welcomePage);
  }); 
  server.on(F("/info"), HTTP_GET, getInfo);
  server.on(F("/ledState"), HTTP_GET, getStateLed);
  server.on(F("/led"), HTTP_PUT, switchLed);
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

void setup() {
    Serial.begin(9600);
    pinMode(LED, OUTPUT);
    pinMode(RELAY, OUTPUT);

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
    res = wm.autoConnect("Kitchen-light","password"); // password protected ap

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
    ElegantOTA.begin(&server);
    server.begin();
}

void loop() {
    server.handleClient();
}