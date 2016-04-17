/*
  To upload through terminal you can use: curl -F "image=@firmware.bin" esp8266-webupdate.local/update
*/

#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <ESP8266WiFiMulti.h>
#include <Ticker.h>
#include <FS.h>

extern "C" {
#include "aes.h"
}

#include "MeshNet.h"

Ticker HB; //heartbeat LED
Ticker scan; //network scanning
ESP8266WiFiMulti WiFiMulti;

/*
   Useful stuff:
   ESP.getChipId() - for unique IDs
   ESP.getCycleCount()
     Serial.printf("ID %u\n",ESP.getChipId());
  Serial.printf("RANDOM %u\n",RANDOM_REG32);  // VERY GOOD random source
  Serial.printf("Cycles %u\n",ESP.getCycleCount());

*/

const char* host = "esp8266-webupdate";
const char* ssid = "MrRobot_2G";
const char* password = "nochurches!N2H0B3";

ESP8266WebServer server(80);
ESP8266HTTPUpdateServer httpUpdater;
//const char* serverIndex = "<form method='POST' action='/update' enctype='multipart/form-data'><input type='file' name='update'><input type='submit' value='Update'></form>";

byte LED = 2;
unsigned long upTime = 0;
bool searchFlag = 0;

void search()
{
  searchFlag = 1;
}

void beat()
{
  digitalWrite(LED, 1 - digitalRead(LED));
  upTime++;

  // Testing functions
  Serial.println(WiFi.localIP());
  Serial.printf("ID %u\n\r", ESP.getChipId());
  Serial.printf("RANDOM %u\n\r", RANDOM_REG32);
  Serial.printf("Cycles %u\n\r", ESP.getCycleCount());
  SPIFFS.begin();
  {
    Dir dir = SPIFFS.openDir("/");
    while (dir.next()) {
      String fileName = dir.fileName();
      size_t fileSize = dir.fileSize();
      Serial.printf("FS File: %s, size: %s\n\r", fileName.c_str(), formatBytes(fileSize).c_str());
    }
    Serial.println("Up Time " + String(upTime));
  }
}

void setup(void) {
  pinMode(LED, OUTPUT);
  Serial.begin(115200); //Bootloader Speed, ease of use
  Serial.println();
  Serial.println("Booting Sketch...");
  WiFi.mode(WIFI_AP_STA);
  WiFi.softAP("MeshCom");
  //WiFi.begin(ssid, password);
  //WiFiMulti.addAP("MrRobot_2G", "nochurches!N2H0B3");

  while (WiFi.status() != WL_CONNECTED) {
    delay(500);
    Serial.print(".");
  }

  Serial.print("Connected! IP address: ");
  Serial.println(WiFi.localIP());

  MDNS.begin(host);

  Serial.print("Open http://");
  Serial.print(host);
  Serial.println(".local/edit to see the file browser");

  //SERVER INIT
  //list directory
  server.on("/list", HTTP_GET, handleFileList);
  //load editor
  server.on("/edit", HTTP_GET, []() {
    if (!handleFileRead("/edit.htm")) server.send(404, "text/plain", "FileNotFound");
  });
  //create file
  server.on("/edit", HTTP_PUT, handleFileCreate);
  //delete file
  server.on("/edit", HTTP_DELETE, handleFileDelete);
  //first callback is called after the request has ended with all parsed arguments
  //second callback handles file uploads at that location
  server.on("/edit", HTTP_POST, []() {
    server.send(200, "text/plain", "");
  }, handleFileUpload);

  //called when the url is not defined here
  //use it to load content from SPIFFS
  server.onNotFound([]() {
    if (!handleFileRead(server.uri()))
      server.send(404, "text/plain", "FileNotFound");
  });

  //get heap status, analog input value and all GPIO statuses in one json call
  server.on("/all", HTTP_GET, []() {
    String json = "{";
    json += "\"heap\":" + String(ESP.getFreeHeap());
    json += ", \"analog\":" + String(analogRead(A0));
    json += ", \"time\":" + String(upTime);
    json += ", \"gpio\":" + String((uint32_t)(((GPI | GPO) & 0xFFFF) | ((GP16I & 0x01) << 16)));
    json += "}";
    server.send(200, "text/json", json);
    json = String();
  });
  httpUpdater.setup(&server);
  server.begin();
  MDNS.addService("http", "tcp", 80);
  Serial.println("HTTP server started");

  HB.attach(1, beat);
  scan.attach(10, search);
}

void loop(void) {
  server.handleClient();

  // Search for other devices
  if (searchFlag) {
    searchFlag = 0;
    int n = WiFi.scanNetworks();
    Serial.print("Starting Scan: " + String(n));
    for (int i = 0; i < n; ++i) {
      String current_ssid = WiFi.SSID(i) + " ";
      long strength = WiFi.RSSI(i);
      Serial.print (current_ssid);
      Serial.println(strength);
    }
  }
  delay(1);
}
