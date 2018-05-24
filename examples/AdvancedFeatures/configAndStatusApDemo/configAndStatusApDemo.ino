/*    
configAndStatusAPDemo.ino
Copyright (c) 2017 ItKindaWorks All right reserved.
github.com/ItKindaWorks

This file is part of ESPHelper

ESPHelper is free software: you can redistribute it and/or modify
it under the terms of the GNU General Public License as published by
the Free Software Foundation, either version 3 of the License, or
(at your option) any later version.

ESPHelper is distributed in the hope that it will be useful,
but WITHOUT ANY WARRANTY; without even the implied warranty of
MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
GNU General Public License for more details.

You should have received a copy of the GNU General Public License
along with ESPHelper.  If not, see <http://www.gnu.org/licenses/>.
*/


#include "ESPHelper.h"
#include "ESPHelperFS.h"
#include "ESPHelperWebConfig.h"
#include "Metro.h"

#define NET_CONFIG_FILE "/netConfig.json"


netInfo config;
ESPHelper myESP;


// setup a server on port 80 (http). We use an external server here because we want more than just a config page 
// but also a status page or anything else that we want to display
ESP8266WebServer server(80);
ESPHelperWebConfig configPage(&server, "/config");

// default net info for unconfigured devices
netInfo homeNet = {
  .mqttHost = "YOUR MQTT-IP",        // can be blank if not using MQTT
  .mqttUser = "YOUR MQTT USERNAME",  // can be blank
  .mqttPass = "YOUR MQTT PASSWORD",  // can be blank
  .mqttPort = 1883,                  // default port for MQTT is 1883 - change only if needed
  .ssid = "YOUR SSID", 
  .pass = "YOUR NETWORK PASS",
  .otaPassword = "YOUR OTA PASS",
  .hostname = "NEW-ESP8266"
};


// timeout before starting AP mode for configuration
Metro connectTimeout = Metro(20000);
bool timeout = false;

// AP moade setup info
const char* broadcastSSID = "ESP-Hotspot";
const char* broadcastPASS = "";
IPAddress broadcastIP = {192, 168, 1, 1};


void setup() {
  Serial.begin(115200);

  // print some debug
  Serial.println("Starting Up - Please Wait...");
  delay(100);

  // startup the Wi-Fi and web server (more in the lines below)
  startWifi();

  // setup the http server and config page (fillConfig will
  // take the netInfo file and use that for default values)
  configPage.fillConfig(&config);
  configPage.begin(config.hostname);

  // Actually start the server (again this would be done automatically
  // if we were just using the config page and didn't use an external server)
  server.begin();
  server.on("/", HTTP_GET, handleStatus);
}


void loop() {
  // get the current status of ESPHelper
  int espHelperStatus = myESP.loop();
  manageESPHelper(espHelperStatus);

  if(espHelperStatus >= FULL_CONNECTION) {
    // regular loop code goes here
  }

  yield();
}


// ESPHelper & config setup and runtime handler functions
void manageESPHelper(int wifiStatus) {
  // if the unit is broadcasting or connected to Wi-Fi then reset the timeout vars
  if (wifiStatus == BROADCAST || wifiStatus >= WIFI_ONLY){
    connectTimeout.reset();
    timeout = false;
  }
  // otherwise check for a timeout condition and handle setting up broadcast
  else if (wifiStatus < WIFI_ONLY) {
    checkForWifiTimeout();
  }
  // handle saving a new network config
  if (configPage.handle()){
    Serial.println("Saving new network config and restarting...");
    myESP.saveConfigFile(configPage.getConfig(), NET_CONFIG_FILE);
    delay(500);
    ESP.restart();
  }
}


void startWifi() {
  loadConfig();

  // setup other ESPHelper info and enable OTA updates
  myESP.setHopping(false);
  myESP.OTA_setPassword(config.otaPassword);
  myESP.OTA_setHostnameWithVersion(config.hostname);
  myESP.OTA_enable();

  Serial.println("Connecting to network");
  // connect to Wi-Fi before proceeding.
  // If cannot connect then switch to AP mode and create a network to config from
  while (myESP.loop() < WIFI_ONLY) {
    checkForWifiTimeout();
    if (timeout) return;
    delay(10);
  }

  Serial.println("Sucess!");
  Serial.println(String("To connect to this device go to " + String(myESP.getIP())));
}

// attempt to load a network configuration from the filesystem
void loadConfig() {
  // check for a good config file and start ESPHelper with the file stored on the ESP
  if (ESPHelperFS::begin()) {
    Serial.println("Filesystem loaded - Loading Config");
    if (ESPHelperFS::validateConfig(NET_CONFIG_FILE) == GOOD_CONFIG) {
      Serial.println("Config loaded");
      myESP.begin(NET_CONFIG_FILE);
    } else {
      // if no good config can be loaded (no file/corruption/etc.) 
      // then attempt to generate a new config and restart the module
      Serial.println("Could not load config - saving new config from default values and restarting");
      delay(10);
      ESPHelperFS::createConfig(&homeNet, NET_CONFIG_FILE);
      ESPHelperFS::end();
      ESP.restart();
    }
  } else {
    // if the filesystem cannot be started, just fail over
    // to the built in network config hardcoded in here
    Serial.println("Could not load filesystem, proceeding with default config values");
    delay(10);
    myESP.begin(&homeNet);
  }
  // load the netInfo from espHelper for use in the config page
  config = myESP.getNetInfo();
}


// function that checks for no network connection for a period of time 
// and starting up AP mode when that time has elapsed
void checkForWifiTimeout() {
  if (connectTimeout.check() && !timeout) {
      Serial.println("Network Connection timeout - starting broadcast (AP) mode...");
      timeout = true;
      myESP.broadcastMode(broadcastSSID, broadcastPASS, broadcastIP);
      myESP.OTA_setPassword(config.otaPassword);
      myESP.OTA_setHostnameWithVersion(config.hostname);
      myESP.OTA_enable();
    }
}


// main config page that allows user to enter in configuration info
void handleStatus() {      
  server.send(200, "text/html", \
    String("<html>\
    <header>\
    <title>Device Info</title>\
    </header>\
    <body>\
      <p><strong>System Status</strong></br>\
      Device Name: " + String(myESP.getHostname()) + "</br>\
      Connected SSID: " + String(myESP.getSSID()) + "</br>\
      Device IP: " + String(myESP.getIP()) + "</br>\
      Uptime (ms): " + String(millis()) + "</p>\
    </body>\
    </html>")
  );
}
