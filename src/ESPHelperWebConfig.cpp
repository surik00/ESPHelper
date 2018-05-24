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
  _server->on(_pageURI, HTTP_GET, [this](){handleGet();});    // Call the 'handleGet' function when a client requests the specified URI with GET
  _server->on(_pageURI, HTTP_POST, [this](){handlePost();});  // Call the 'handlePost' function when a POST request is made to the specified URI
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


// main config page that allows user to enter in configuration info
void ESPHelperWebConfig::handleGet() {
  String showReset = "";

  if (_resetSet) {
    showReset = "</br></br></br></br></br><form action=\"" + String(_resetURI) + "\" method=\"POST\">\
    <input type=\"submit\" value=\"Click Here to Reset ESP Filesystem\"> (WARNING: Deletes all files on device!)</form>";
  }

  if (_preFill) {
    _server->send(200, "text/html", 
      String("<form action=\"" + String(_pageURI) + "\" method=\"POST\">\
      <input type=\"text\" name=\"hostname\" size=\"64\" maxlength=\"63\" placeholder=\"Device Hostname (Required)\" required=\"\" value=\"" + String(_fillData->hostname) + "\"></br>\
      <input type=\"text\" name=\"ssid\" size=\"64\" maxlength=\"63\" placeholder=\"SSID (Required)\" required=\"\" value=\"" + String(_fillData->ssid) + "\"></br>\
      <input type=\"password\" name=\"netPass\" size=\"64\" maxlength=\"63\" placeholder=\"Network Password (Previous value used if blank)\"></br>\
      <input type=\"password\" name=\"otaPassword\" size=\"64\" maxlength=\"63\" placeholder=\"OTA Password (Previous value used if blank)\"></br>\
      <input type=\"text\" name=\"mqttHost\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Host\" value=\"" + String(_fillData->mqttHost) + "\"></br>\
      <input type=\"text\" name=\"mqttPort\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Port (default is 1883)\" value=\"" + String(_fillData->mqttPort) + "\"></br>\
      <input type=\"text\" name=\"mqttUser\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Username\" value=\"" + String(_fillData->mqttUser) + "\"></br>\
      <input type=\"password\" name=\"mqttPass\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Password (Previous value used if blank)\"></br>\
      <input type=\"submit\" value=\"Submit\"></form>\
      <p>Press Submit to update ESP8266 config file</p>" + showReset)
    );
  } else {
    _server->send(200, "text/html", 
      String("<form action=\"" + String(_pageURI) + "\" method=\"POST\">\
      <input type=\"text\" name=\"hostname\" size=\"64\" maxlength=\"63\" placeholder=\"Device Hostname (Required)\" required=\"\"></br>\
      <input type=\"text\" name=\"ssid\" size=\"64\" maxlength=\"63\" placeholder=\"SSID (Required)\" required=\"\"></br>\
      <input type=\"password\" name=\"netPass\" size=\"64\" maxlength=\"63\" placeholder=\"Network Password\"></br>\
      <input type=\"password\" name=\"otaPassword\" size=\"64\" maxlength=\"63\" placeholder=\"OTA Password\"></br>\
      <input type=\"text\" name=\"mqttHost\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Host\"></br>\
      <input type=\"text\" name=\"mqttPort\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Port (default is 1883)\"></br>\
      <input type=\"text\" name=\"mqttUser\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Username\"></br>\
      <input type=\"password\" name=\"mqttPass\" size=\"64\" maxlength=\"63\" placeholder=\"MQTT Password\"></br>\
      <input type=\"submit\" value=\"Submit\"></form>\
      <p>Press Submit to update ESP8266 config file</p>" + showReset)
    );
  }
}


// If a POST request is made to the _pageURI
void ESPHelperWebConfig::handlePost() {   
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
    _server->send(400, "text/plain", "400: Invalid Request - Did you make sure to specify an SSID and Hostname?");
    return;
  }

  // if there is an mqtt user/pass/port entered then there better also be a host!
  if ((_server->arg("mqttUser") != NULL || _server->arg("mqttPass") != NULL) && _server->arg("mqttHost") == NULL) {
     // The request is invalid, so send HTTP status 400
    _server->send(400, "text/plain", "400: Invalid Request - MQTT info specified without host"); 
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
  _server->send(200, "text/html", "<h1>Config info successfully loaded, restarting!</h1>");

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
  // tell the user that the config is loaded in and the module is restarting
  _server->send(200, "text/html", "<h1>Resetting SPIFFS and restarting with default values</h1>");

  SPIFFS.format();
  ESP.restart();
}


void ESPHelperWebConfig::handleNotFound(){
   // Send HTTP status 404 (Not Found) when there's no handler for the URI in the request
  _server->send(404, "text/html",
    String("<h1>Could not find page " + String(_server->uri()) + "</h1><a href=\"/\">Go to the index page</a>")
    );
}
