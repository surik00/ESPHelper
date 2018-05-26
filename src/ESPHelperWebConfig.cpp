/*    
ESPHelperWebConfig.cpp
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

Refactoring and improvements by Suren Khorenyan
*/


#include "ESPHelperWebConfig.h"
#include "WebConfigHTML.h"
#include <FS.h>


ESPHelperWebConfig::ESPHelperWebConfig(int port, const char* URI) : _localServer(port) {
  _server = &_localServer;
  _runningLocal = true;
  _pageURI = URI;
}


ESPHelperWebConfig::ESPHelperWebConfig(ESP8266WebServer *server, const char* URI) {
  _server = server;
  _runningLocal = false;
  _pageURI = URI;
}


bool ESPHelperWebConfig::begin(const char* _hostname) {
  MDNS.begin(_hostname);
  return begin();
}


bool ESPHelperWebConfig::begin() {
  // setup server handlers
  // these handler function definitions use lambdas to pass the funtion... more information can be found here:
  // https://stackoverflow.com/questions/39803135/c-unresolved-overloaded-function-type
  _server->on(_pageURI, HTTP_GET, [this](){handleGetConfig();});    // Call the 'handleGetConfig' function when a client requests the specified URI with GET
  _server->on(_pageURI, HTTP_POST, [this](){handlePostConfig();});  // Call the 'handlePostConfig' function when a POST request is made to the specified URI
  _server->onNotFound([this](){handleNotFound();});           // When a client requests an unknown URI (i.e. something other than "/"), call function "handleNotFound"

  if (_runningLocal)
    _server->begin(); // Actually start the server

	return true;
}


void ESPHelperWebConfig::fillConfig(netInfo* fillInfo) {
  _fillData = fillInfo;
  _preFill = true;
}


bool ESPHelperWebConfig::handle() {
	_server->handleClient();
	return _configLoaded;
}


netInfo ESPHelperWebConfig::getConfig() {
	_configLoaded = false;
	return _config;
}

// main config page that allows user to enter configuration info
void ESPHelperWebConfig::handleGetConfig() {
  // sometimes works incorrect, removed
  // _server->send(200, "text/html", configPageHTML());

  // prepare prefillable data
  String hostname = String();
  String ssid = String();
  String mqttHost = String();
  String mqttPort = String();
  String mqttUser = String();

  if (_preFill) {
    hostname = String(_fillData->hostname);
    ssid = String(_fillData->ssid);
    mqttHost = String(_fillData->mqttHost);
    mqttPort = String(_fillData->mqttPort);
    mqttUser = String(_fillData->mqttUser);
  }

  /* Like in example https://github.com/esp8266/Arduino/blob/61cd8d83859524db0066a647de3de3f6a0039bb2/libraries/DNSServer/examples/CaptivePortalAdvanced/handleHttp.ino#L40-L92*/
  _server->sendHeader("Cache-Control", "no-cache, no-store, must-revalidate");
  _server->sendHeader("Pragma", "no-cache");
  _server->sendHeader("Expires", "-1");
  _server->setContentLength(CONTENT_LENGTH_UNKNOWN);
  // Empty content inhibits Content-length header so we have to close the socket ourselves.
  _server->send(200, "text/html", "");
  // sending all content as separate values to escape Strings concatenation
  _server->sendContent(HTML_START);
  _server->sendContent(HTML_STYLE);
  _server->sendContent(HTML_S_T);
  _server->sendContent("Configure ESP8266");
  _server->sendContent(HTML_T_B);
  _server->sendContent(HTML_CFG_HEADING);
  _server->sendContent(HTML_CFG_AL);
  _server->sendContent(_pageURI);
  _server->sendContent(HTML_CFG_1);
  _server->sendContent(hostname);
  _server->sendContent(HTML_CFG_2);
  _server->sendContent(ssid);
  _server->sendContent(HTML_CFG_3);
  _server->sendContent(mqttHost);
  _server->sendContent(HTML_CFG_4);
  _server->sendContent(mqttPort);
  _server->sendContent(HTML_CFG_5);
  _server->sendContent(mqttUser);
  _server->sendContent(HTML_CFG_6);
  if (_resetSet) {
    _server->sendContent(HTML_CFG_RESET_START);
    _server->sendContent(_resetURI);
    _server->sendContent(HTML_CFG_RESET_END);
  }
  _server->sendContent(HTML_CLOSE);
  _server->client().stop();
  /**/
}

// If a POST request is made to the _pageURI
void ESPHelperWebConfig::handlePostConfig() {   
  // make sure that all the arguments exist and that at least SSID and hostname have been entered                      
  if ( !(_server->hasArg("ssid")
      && _server->arg("ssid") != NULL
      && _server->hasArg("hostname")
      && _server->arg("hostname") != NULL)
      && _server->hasArg("netPass")
      && _server->hasArg("mqttHost")
      && _server->hasArg("mqttUser")
      && _server->hasArg("mqttPass")
      && _server->hasArg("mqttPort")
      && _server->hasArg("otaPassword") ) {

    // If the POST request doesn't have username and password data
    // then the request is invalid, so send HTTP status 400
    _server->send(400, "text/html", HTML_400_PARAMS_MISSING);
    return;
  }

  // if there is an mqtt user/pass/port entered then there better also be a host!
  if ((_server->arg("mqttUser") != NULL || _server->arg("mqttPass") != NULL) && _server->arg("mqttHost") == NULL) {
     // The request is invalid, so send HTTP status 400
    _server->send(400, "text/html", HTML_400_MQTT_NO_HOST); 
    return;
  }

  // convert the Strings returned by _server->arg to char arrays that can be entered into netInfo

  // Wi-Fi network pass
  if (_preFill && _server->arg("netPass").length() == 0) {
    strncpy(_newNetPass, _fillData->pass, 64);
    _newNetPass[sizeof(_newNetPass) - 1] = '\0';
  } else {
    _server->arg("netPass").toCharArray(_newNetPass, sizeof(_newNetPass));
  }
  
  // MQTT pass
  if (_preFill && _server->arg("mqttPass").length() == 0) {
    strncpy(_newMqttPass, _fillData->mqttPass, 64);
    _newMqttPass[sizeof(_newMqttPass) - 1] = '\0';
  } else {
    _server->arg("mqttPass").toCharArray(_newMqttPass, sizeof(_newMqttPass));
  }

  // OTA pass
  if (_preFill && _server->arg("otaPassword").length() == 0) {
    strncpy(_newOTAPass, _fillData->otaPassword, 64);
    _newOTAPass[sizeof(_newOTAPass) - 1] = '\0';
  } else {
    _server->arg("otaPassword").toCharArray(_newOTAPass, sizeof(_newOTAPass));
  }

  // other non-protected vars
  _server->arg("ssid").toCharArray(_newSsid, sizeof(_newSsid));
  _server->arg("hostname").toCharArray(_newHostname, sizeof(_newHostname));
  _server->arg("mqttHost").toCharArray(_newMqttHost, sizeof(_newMqttHost));
  _server->arg("mqttUser").toCharArray(_newMqttUser, sizeof(_newMqttUser));

  // the port is special because it doesn't get stored as a string so we take care of that
  if (_server->arg("mqttPort") != NULL) {
    _newMqttPort = _server->arg("mqttPort").toInt();
  } else {
    _newMqttPort = 1883;
  }

  // tell user that the config is successfully loaded and the module is restarting
  _server->send(200, "text/html", HTML_CONFIG_SUCCESS);

  // enter in the new data 
  _config = {
    mqttHost : _newMqttHost,     
    mqttUser : _newMqttUser,   
    mqttPass : _newMqttPass,   
    mqttPort : _newMqttPort,
    ssid : _newSsid, 
    pass : _newNetPass,
    otaPassword : _newOTAPass,
    hostname : _newHostname
  };

  _configLoaded = true;
}


void ESPHelperWebConfig::setSpiffsReset(const char* URI){
  _resetURI = URI;
  _server->on(_resetURI, HTTP_POST, [this](){handleReset();});
  _resetSet = true;
}


void ESPHelperWebConfig::handleReset(){
  _server->send(200, "text/html", HTML_SPIFFS_FORMAT);

  SPIFFS.format();
  ESP.restart();
}


void ESPHelperWebConfig::handleNotFound(){
   // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  _server->send(404, "text/html",
    String("<h1>Could not find page " + String(_server->uri()) + "</h1><a href=\"/\">Go to the index page</a>")
    );
}
