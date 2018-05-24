/*    
ESPHelperFS.cpp
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

Refactoring by Suren Khorenyan
*/


#include "ESPHelperFS.h"

// StaticJsonBuffer<JSON_SIZE>* ESPHelperFS::_tmpBufPtr = NULL;


ESPHelperFS::ESPHelperFS() : _filename("/netConfig.json") {
  _networkData = defaultConfig;
}


ESPHelperFS::ESPHelperFS(const char* filename) {
  _filename = filename;
  _networkData = defaultConfig;
}


bool ESPHelperFS::begin() {
  //load the config file
  if (SPIFFS.begin()) {
    // printFSinfo();
    FSdebugPrintln("Loaded Filesystem");
    return true;
  }
  return false;
}


void ESPHelperFS::end() {
  FSdebugPrintln("Unloaded Filesystem");
  SPIFFS.end();
}


void ESPHelperFS::printFile() {
  // this opens the file "f.txt" in read-mode
  File f = SPIFFS.open(_filename, "r");
  if (f) {
    // we successfully opened the file
    while (f.available()) {
      // Let's read line by line from the file
      String line = f.readStringUntil('\n');
      FSdebugPrintln(line);
    }
  }
  f.close();
}


void ESPHelperFS::printFSinfo() {
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  FSdebugPrint("total bytes: ");
  FSdebugPrintln(fs_info.totalBytes);
  FSdebugPrint("used bytes: ");
  FSdebugPrintln(fs_info.usedBytes);
  FSdebugPrint("block size: ");
  FSdebugPrintln(fs_info.blockSize);
  FSdebugPrint("page size: ");
  FSdebugPrintln(fs_info.pageSize);
  FSdebugPrint("max open files: ");
  FSdebugPrintln(fs_info.maxOpenFiles);
  FSdebugPrint("max path length: ");
  FSdebugPrintln(fs_info.maxPathLength);
}


//load the file from FS into var buf
bool ESPHelperFS::loadFile(const char* filename, std::unique_ptr<char[]> &buf) {

  // FSdebugPrint("Opening File: ");  // FS Debug print
  // FSdebugPrintln(filename);  // FS Debug print
  // open file as read only
  File configFile = SPIFFS.open(filename, "r");

  // check to make sure opening was possible
  if (!configFile) {
    // FSdebugPrintln("Failed to open config file");  // FS Debug print
    configFile.close();
    return false;
  }

  // make sure the config isn't too large to store in the JSON container
  size_t size = configFile.size();
  // FSdebugPrint("JSON File Size: ");  // FS Debug print
  // FSdebugPrintln(size);  // FS Debug print
  if (size > JSON_SIZE) { 
    // FSdebugPrintln("JSON File too large - returning");   // FS Debug print
    return false;
  }

  // Allocate a buffer to store contents of the file.
  std::unique_ptr<char[]> newBuf(new char[size]);

  // We don't use String here because ArduinoJson library requires the input
  // buffer to be mutable. If you don't use ArduinoJson, you may as well
  // use configFile.readString instead.
  configFile.readBytes(newBuf.get(), size);

  // move the contents of newBuf into buf
  buf = std::move(newBuf);

  //close out the file
  configFile.close();

  //sucess return true
  return true;
  
}


int8_t ESPHelperFS::validateConfig(const char* filename) {
  
  //create a buffer for the file data
  std::unique_ptr<char[]> buf(new char[JSON_SIZE]);
  loadFile(filename, buf);
  StaticJsonBuffer<JSON_SIZE> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  if (json.size() == 0) {
    // FSdebugPrintln("JSON File is empty");  // FS Debug print
    return NO_CONFIG;
  }

  // return false if the file could not be parsed
  if (!json.success()) {
    // FSdebugPrintln("JSON File corrupt");  // FS Debug print
    return CANNOT_PARSE;
  }

  // check to make sure all netInfo keys exist
  if ( !(json.containsKey("ssid") 
      && json.containsKey("networkPass") 
      && json.containsKey("mqttIP") 
      && json.containsKey("mqttUSER")
      && json.containsKey("mqttPASS")
      && json.containsKey("mqttPORT")
      && json.containsKey("hostname")
      && json.containsKey("OTA_Password")
      && json.containsKey("willTopic")
      && json.containsKey("willMessage")
      && json.containsKey("willQoS")
      && json.containsKey("willRetain")) ) {

    // FSdebugPrintln("Config incomplete");  // FS Debug print
    return INCOMPLETE;
  }
  return GOOD_CONFIG;
}


bool ESPHelperFS::loadNetworkConfig() {

  // check if the config file is good. And if not - create a new one.
  if (validateConfig(_filename) != GOOD_CONFIG) {
    createConfig(&defaultConfig);
    return false;
  }


  else {
    // create a buffer for the file data
    std::unique_ptr<char[]> buf(new char[JSON_SIZE]);
    loadFile(_filename, buf);
    StaticJsonBuffer<JSON_SIZE> jsonBuffer;
    JsonObject& json = jsonBuffer.parseObject(buf.get());


    // copy the keys into char arrays
    strcpy(ssid, json["ssid"]);
    strcpy(netPass, json["networkPass"]);
    strcpy(hostName, json["hostname"]);
    strcpy(mqtt_ip, json["mqttIP"]);
    strcpy(otaPass, json["OTA_Password"]);
    strcpy(mqttPort, json["mqttPORT"]);
    strcpy(mqttUser, json["mqttUSER"]);
    strcpy(mqttPass, json["mqttPASS"]);
    strcpy(willTopic, json["willTopic"]);
    strcpy(willMessage, json["willMessage"]);
    strcpy(willQoS, json["willQoS"]);
    strcpy(willRetain, json["willRetain"]);

    int port = atoi(mqttPort);
    int numQoS = atoi(willQoS);
    int numRetain = atoi(willRetain);

    // then set that data into a netInfo object
    _networkData = {
      mqttHost : mqtt_ip,
      mqttUser : mqttUser,
      mqttPass : mqttPass,
      mqttPort : port,
      ssid : ssid,
      pass : netPass,
      otaPassword : otaPass,
      hostname : hostName,
      willTopic : willTopic,
      willMessage : willMessage,
      willQoS : numQoS,
      willRetain : numRetain
    };

    // FSdebugPrintln("Reading config file with values: ");  // FS Debug print
    // FSdebugPrint("MQTT Server: ");  // FS Debug print
    // FSdebugPrintln(_networkData.mqttHost);  // FS Debug print
    // FSdebugPrint("MQTT User: ");  // FS Debug print
    // FSdebugPrintln(_networkData.mqttUser);  // FS Debug print
    // FSdebugPrint("MQTT Password: ");  // FS Debug print
    // FSdebugPrintln(_networkData.mqttPass);  // FS Debug print
    // FSdebugPrint("MQTT Port: ");  // FS Debug print
    // FSdebugPrintln(_networkData.mqttPort);  // FS Debug print
    // FSdebugPrint("SSID: ");  // FS Debug print
    // FSdebugPrintln(_networkData.ssid);  // FS Debug print
    // FSdebugPrint("Network Pass: ");  // FS Debug print
    // FSdebugPrintln(_networkData.pass);  // FS Debug print
    // FSdebugPrint("Device Name: ");  // FS Debug print
    // FSdebugPrintln(_networkData.hostname);  // FS Debug print
    // FSdebugPrint("OTA Password: ");  // FS Debug print
    // FSdebugPrintln(_networkData.otaPassword);  // FS Debug print
    // FSdebugPrint("Last Will Topic: ");  // FS Debug print
    // FSdebugPrintln(_networkData.willTopic);  // FS Debug print
    // FSdebugPrint("Last Will Message: ");  // FS Debug print
    // FSdebugPrintln(_networkData.willMessage);  // FS Debug print
    // FSdebugPrint("Last Will QoS: ");  // FS Debug print
    // FSdebugPrintln(_networkData.willQoS);  // FS Debug print
    // FSdebugPrint("Last Will Retain: ");  // FS Debug print
    // FSdebugPrintln(_networkData.willRetain);  // FS Debug print

    // configFile.close();  // donno why it is here
  }

  return true;
}


// add a key to a json file
bool ESPHelperFS::addKey(const char* keyName, const char* value) {
  if(_filename == "")
    return false;
  return addKey(keyName, value, _filename);
}

// add a key to a json file
bool ESPHelperFS::addKey(const char* keyName, const char* value, const char* filename) {
  // WTF is that? Why?
  // if(!SPIFFS.exists(filename)) {
  //   File configFile = SPIFFS.open(filename, "w");
  //   configFile.close();
  // }

  // create a buffer for the file data
  std::unique_ptr<char[]> buf(new char[JSON_SIZE]);
  loadFile(filename, buf);
  StaticJsonBuffer<JSON_SIZE> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  if (json.success()) {
    // add the key to the json object
    json[keyName] = value;
    // FSdebugPrint("Added Key ");  // FS Debug print
    // FSdebugPrint(keyName);  // FS Debug print
    // FSdebugPrint(" with Value ");  // FS Debug print
    // FSdebugPrintln(value);  // FS Debug print

    // save the new config with added key
    return saveConfig(json, filename);
  }


  StaticJsonBuffer<JSON_SIZE> blankBuffer;
  JsonObject& blankJson = blankBuffer.createObject();
  if(blankJson.success()){
    //add the key to the json object
    blankJson[keyName] = value;
    // FSdebugPrint("Added Key ");  // FS Debug print
    // FSdebugPrint(keyName);  // FS Debug print
    // FSdebugPrint(" With Value ");  // FS Debug print
    // FSdebugPrintln(value);  // FS Debug print

    // save the new config with added key
    // and return the result of saving
    return saveConfig(blankJson, filename);
  }
    
  return false;
}


// read a key from a json file
String ESPHelperFS::loadKey(const char* keyName) {
  if(_filename == "")
    return String();
  return loadKey(keyName, _filename);
}

// read a key from a json file
String ESPHelperFS::loadKey(const char* keyName, const char* filename) {
  // create a buffer for the file data
  std::unique_ptr<char[]> buf(new char[JSON_SIZE]);
  loadFile(filename, buf);
  StaticJsonBuffer<JSON_SIZE> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());
  if (json.success()){
    // if the key does not exist then return an empty string
    if(!json.containsKey(keyName)){
      // FSdebugPrintln("Key not found");  // FS Debug print
      return "";
    }
    // return value of the key
    return (const char*)json[keyName];
  }
  // return blank if file could not be opened
  return "";
}


netInfo ESPHelperFS::getNetInfo() {
  return _networkData;
}


bool ESPHelperFS::createConfig(const char* filename) {
  return createConfig(_filename,
                      defaultConfig.ssid, 
                      defaultConfig.pass, 
                      defaultConfig.hostname, 
                      defaultConfig.mqttHost,  
                      defaultConfig.mqttUser, 
                      defaultConfig.mqttPass, 
                      defaultConfig.mqttPort,
                      defaultConfig.otaPassword,
                      defaultConfig.willTopic,
                      defaultConfig.willMessage,
                      defaultConfig.willQoS,
                      defaultConfig.willRetain);
}


bool ESPHelperFS::createConfig(const netInfo* config) {
  return createConfig(_filename,
                      config->ssid, 
                      config->pass, 
                      config->hostname, 
                      config->mqttHost,  
                      config->mqttUser, 
                      config->mqttPass, 
                      config->mqttPort,
                      config->otaPassword,
                      config->willTopic,
                      config->willMessage,
                      config->willQoS,
                      config->willRetain);
}

bool ESPHelperFS::createConfig(const netInfo* config, const char* filename) {
  return createConfig(filename,
                      config->ssid, 
                      config->pass, 
                      config->hostname, 
                      config->mqttHost,  
                      config->mqttUser, 
                      config->mqttPass, 
                      config->mqttPort,
                      config->otaPassword,
                      config->willTopic,
                      config->willMessage,
                      config->willQoS,
                      config->willRetain);
}

bool ESPHelperFS::createConfig(const char* filename,
                               const char* _ssid, 
                               const char* _networkPass, 
                               const char* _deviceName, 
                               const char* _mqttIP,
                               const char* _mqttUser,
                               const char* _mqttPass,
                               const int _mqttPort,
                               const char* _otaPass,
                               const char* _willTopic,
                               const char* _willMessage,
                               const int _willQoS,
                               const int _willRetain) {

  // FSdebugPrintln("Generating new config file with values: ");  // FS Debug print
  // FSdebugPrint("SSID: ");  // FS Debug print
  // FSdebugPrintln(_ssid);  // FS Debug print
  // FSdebugPrint("Network Pass: ");  // FS Debug print
  // FSdebugPrintln(_networkPass);  // FS Debug print
  // FSdebugPrint("hostname: ");  // FS Debug print
  // FSdebugPrintln(_deviceName);  // FS Debug print
  // FSdebugPrint("MQTT Server: ");  // FS Debug print
  // FSdebugPrintln(_mqttIP);  // FS Debug print
  // FSdebugPrint("MQTT Username: ");  // FS Debug print
  // FSdebugPrintln(_mqttUser);  // FS Debug print
  // FSdebugPrint("MQTT Password: ");  // FS Debug print
  // FSdebugPrintln(_mqttPass);  // FS Debug print
  // FSdebugPrint("MQTT PORT: ");  // FS Debug print
  // FSdebugPrintln(_mqttPort);  // FS Debug print
  // FSdebugPrint("OTA Password: ");  // FS Debug print
  // FSdebugPrintln(_otaPass);  // FS Debug print
  // FSdebugPrint("Last Will Topic: ");  // FS Debug print
  // FSdebugPrintln(_willTopic);  // FS Debug print
  // FSdebugPrint("Last Will Message: ");  // FS Debug print
  // FSdebugPrintln(_willMessage);  // FS Debug print
  // FSdebugPrint("Last Will QoS: ");  // FS Debug print
  // FSdebugPrintln(_willQoS);  // FS Debug print
  // FSdebugPrint("Last Will Retain: ");  // FS Debug print
  // FSdebugPrintln(_willRetain);  // FS Debug print

  char portString[10];
  sprintf(portString, "%d", _mqttPort);

  char qoSString[10];
  sprintf(qoSString, "%d", _willQoS);

  char retainString[10];
  sprintf(retainString, "%d", _willRetain);

  // FSdebugPrintln("creating json");  // FS Debug print

  // create a buffer for the file data
  std::unique_ptr<char[]> buf(new char[JSON_SIZE]);
  loadFile(filename, buf);
  StaticJsonBuffer<JSON_SIZE> jsonBuffer;

  // if a json file already exists then use that as the base
  JsonObject& json = validateConfig(filename) == GOOD_CONFIG ? jsonBuffer.parseObject(buf.get()) : jsonBuffer.createObject();

  json["ssid"] = _ssid;
  json["networkPass"] = _networkPass;
  json["hostname"] = _deviceName;
  json["mqttIP"] = _mqttIP;
  json["mqttPORT"] = portString;
  json["mqttUSER"] = _mqttUser;
  json["mqttPASS"] = _mqttPass;
  json["OTA_Password"] = _otaPass;
  json["willTopic"] = _willTopic;
  json["willMessage"] = _willMessage;
  json["willQoS"] = qoSString;
  json["willRetain"] = retainString;

  // FSdebugPrintln("done");  // FS Debug print
  return saveConfig(json, filename);
}


bool ESPHelperFS::saveConfig(JsonObject& json, const char* filename) {
  // FSdebugPrintln("Saving File...");  // FS Debug print
  // if (SPIFFS.exists(filename)) {
  //   SPIFFS.remove(filename);
  // }

  // FSdebugPrintln("Opening File as write only");  // FS Debug print
  // The "w" argument makes the existing file to be ignored (if there is one)
  // so the new config is written over the old (not matter if it ever existed)
  File configFile = SPIFFS.open(filename, "w");
  if (!configFile) {
    // FSdebugPrintln("Failed to open config file for writing");  // FS Debug print
    return false;
  }

  // FSdebugPrintln("File open now writing");  // FS Debug print
  json.printTo(configFile);

  // FSdebugPrintln("Writing done now closing");  // FS Debug print
  configFile.close();

  // FSdebugPrintln("done.");  // FS Debug print
  return true;
}
