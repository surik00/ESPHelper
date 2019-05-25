/*    
ESPHelper.cpp
Copyright (c) 2017 ItKindaWorks Inc All right reserved.
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


#include "ESPHelper.h"
#include <WiFiClientSecure.h>
#include "ESPHelperFS.h"



void ESPHelper::safeApDisconnect() {
  // max timeout of 2 seconds before just dropping out and restarting
  WiFi.softAPdisconnect();
  WiFi.disconnect();
  int timeout = 0;
  while (WiFi.status() != WL_DISCONNECTED && timeout < 200) {
    delay(10);
    timeout++;
  }
}

// empy initializer
ESPHelper::ESPHelper() {
  init("", "", "", "", "", 1883, "defaultWillTopic", "", 0, 1);
}


// Yes yes I know how messy it is to have all these different constructors that all call init AND
// have a bunch of overloaded init methods that do the same thing more or less but this lets me
// instantiate a blank ESPHelper globally and then in setup configure it without having to use new


// initializer with single netInfo network
ESPHelper::ESPHelper(const netInfo *startingNet) {
  init(startingNet->ssid,
       startingNet->pass,
       startingNet->mqttHost,
       startingNet->mqttUser,
       startingNet->mqttPass,
       startingNet->mqttPort,
       startingNet->willTopic,
       startingNet->willMessage,
       startingNet->willQoS,
       startingNet->willRetain);
}

// initializer with single network information and MQTT broker
ESPHelper::ESPHelper(const char *ssid,
                     const char *pass,
                     const char *mqttIP) {
  init(ssid, pass, mqttIP, "", "", 1883, "defaultWillTopic", "", 0, 1);
}

// initializer with single network information, MQTT broker and MQTT Last Will
ESPHelper::ESPHelper(const char *ssid,
                     const char *pass,
                     const char *mqttIP,
                     const char *willTopic,
                     const char *willMessage) {
  init(ssid, pass, mqttIP, "", "", 1883, willTopic, willMessage, 0, 1);
}

// initializer with single network information, MQTT broker, MQTT Last Will and Testament options
ESPHelper::ESPHelper(const char *ssid,
                     const char *pass,
                     const char *mqttIP,
                     const char *willTopic,
                     const char *willMessage,
                     const int willQoS,
                     const int willRetain) {
  init(ssid, pass, mqttIP, "", "", 1883, willTopic, willMessage, 0, 1);
}

// initializer with single network information (MQTT user/pass)
ESPHelper::ESPHelper(const char *ssid,
                     const char *pass,
                     const char *mqttIP,
                     const char *mqttUser,
                     const char *mqttPass,
                     const int mqttPort) {
  init(ssid, pass, mqttIP, mqttUser, mqttPass, mqttPort, "defaultWillTopic", "", 0, 1);
}

// initializer with single network information (MQTT user/pass) + Testament
ESPHelper::ESPHelper(const char *ssid,
                     const char *pass,
                     const char *mqttIP,
                     const char *mqttUser,
                     const char *mqttPass,
                     const int mqttPort,
                     const char *willTopic,
                     const char *willMessage,
                     const int willQoS,
                     const int willRetain) {
  init(ssid, pass,
    mqttIP, mqttUser,
    mqttPass, mqttPort,
    willTopic, willMessage,
    willQoS, willRetain);
}

// initializer with single network information (no MQTT)
ESPHelper::ESPHelper(const char *ssid, const char *pass) {
  init(ssid, pass, "", "", "", 1883, "defaultWillTopic","",0,1);
}

// initializer with netInfo array and index
ESPHelper::ESPHelper(netInfo *netList[], uint8_t netCount, uint8_t startIndex) {
  _netList = netList;
  _netCount = netCount;
  _currentIndex = startIndex;

  // enable hopping since we have an array of networks to use
  _hoppingAllowed = true;

  // disable OTA by default
  _useOTA = false;

  netInfo* tmp = netList[constrain(_currentIndex, 0, _netCount)];
  init(tmp->ssid,
       tmp->pass,
       tmp->mqttHost,
       tmp->mqttUser,
       tmp->mqttPass,
       tmp->mqttPort,
       tmp->willTopic,
       tmp->willMessage,
       tmp->willQoS,
       tmp->willRetain);
}

ESPHelper::ESPHelper(const char* configFile) {
  netInfo ESPConfig = loadConfigFile(configFile);
  init(ESPConfig.ssid,
       ESPConfig.pass,
       ESPConfig.mqttHost,
       ESPConfig.mqttUser,
       ESPConfig.mqttPass,
       ESPConfig.mqttPort,
       ESPConfig.willTopic,
       ESPConfig.willMessage,
       ESPConfig.willQoS,
       ESPConfig.willRetain);
}


//initialize the netinfo data and reset wifi. set hopping and OTA to off
void ESPHelper::init(const char *ssid,
                     const char *pass,
                     const char *mqttIP,
                     const char *mqttUser,
                     const char *mqttPass,
                     const int mqttPort,
                     const char *willTopic,
                     const char *willMessage,
                     const int willQoS,
                     const int willRetain) {
  // diconnect from and previous wifi networks
  WiFi.softAPdisconnect();
  WiFi.disconnect();

  _currentNet.ssid = ssid;
  _currentNet.pass = pass;
  _currentNet.mqttHost= mqttIP;
  _currentNet.mqttUser = mqttUser;
  _currentNet.mqttPass = mqttPass;
  _currentNet.mqttPort = mqttPort;
  _currentNet.willTopic = willTopic;
  _currentNet.willMessage = willMessage;
  _currentNet.willQoS = willQoS;
  _currentNet.willRetain = willRetain;

  // validate various bits of network/MQTT info
  validateConfig();
}

void ESPHelper::validateConfig() {
  // network pass
  if (_currentNet.pass[0] == '\0')
    _passSet = false;
  else
    _passSet = true;

  // ssid
  if (_currentNet.ssid[0] == '\0')
    _ssidSet = false;
  else
    _ssidSet = true;

  // MQTT host
  if (_currentNet.mqttHost[0] == '\0')
    _mqttSet = false;
  else
    _mqttSet = true;

  // MQTT port
  if (_currentNet.mqttPort == 0)
    _currentNet.mqttPort = 1883;

  // MQTT username
  if (_currentNet.mqttUser[0] == '\0')
    _mqttUserSet = false;
  else
    _mqttUserSet = true;

  // MQTT password
  if (_currentNet.mqttPass[0] == '\0')
    _mqttPassSet = false;
  else
    _mqttPassSet = true;

  // Will Topic
  if (_currentNet.willTopic[0] == '\0')
    _willTopicSet = false;
  else
    _willTopicSet = true;

  // Will Message
  if (_currentNet.willMessage[0] == '\0')
    _willMessageSet = false;
  else
    _willMessageSet = true;
}

bool ESPHelper::begin(const char* filename) {
  _currentNet = loadConfigFile(filename);
  return begin(_currentNet.ssid,
               _currentNet.pass,
               _currentNet.mqttHost,
               _currentNet.mqttUser,
               _currentNet.mqttPass,
               _currentNet.mqttPort,
               _currentNet.willTopic,
               _currentNet.willMessage,
               _currentNet.willQoS,
               _currentNet.willRetain);
}


bool ESPHelper::begin(const netInfo *startingNet) {
  return begin(startingNet->ssid,
               startingNet->pass,
               startingNet->mqttHost,
               startingNet->mqttUser,
               startingNet->mqttPass,
               startingNet->mqttPort,
               startingNet->willTopic,
               startingNet->willMessage,
               startingNet->willQoS,
               startingNet->willRetain);
}

// initializer with single network information and MQTT broker
bool ESPHelper::begin(const char *ssid, const char *pass, const char *mqttIP) {
  return begin(ssid, pass, mqttIP, "", "", 1883, "defaultWillTopic","",0,1);
}

// initializer with single network information (no MQTT)
bool ESPHelper::begin(const char *ssid, const char *pass) {
  return begin(ssid, pass, "", "", "", 1883, "defaultWillTopic","",0,1);
}

bool ESPHelper::begin(const char *ssid,
                      const char *pass,
                      const char *mqttIP,
                      const char *mqttUser,
                      const char *mqttPass,
                      const int mqttPort,
                      const char *willTopic,
                      const char *willMessage,
                      const int willQoS,
                      const int willRetain) {
  init(ssid, pass,
       mqttIP,mqttUser,
       mqttPass, mqttPort,
       willTopic, willMessage,
       willQoS, willRetain);
  return begin();
}

bool ESPHelper::begin(const char *ssid,
                      const char *pass,
                      const char *mqttIP,
                      const char *mqttUser,
                      const char *mqttPass,
                      const int mqttPort) {
  return begin(ssid, pass, mqttIP, mqttUser, mqttPass, mqttPort, "defaultWillTopic","",0,1);
}

// start the Wi-Fi & MQTT systems and attempt connection (currently blocking)
// true on: parameter check validated
// false on: parameter check failed
bool ESPHelper::begin(){
  if (_ssidSet) {
    // Generate client name based on MAC address and last 8 bits of microsecond counter
    _clientName = "ESP8266-";
    uint8_t mac[6];
    WiFi.macAddress(mac);
    _clientName += macToStr(mac);

    // set the Wi-Fi mode to station and begin the Wi-Fi (connect using either ssid or ssid/pass)
    WiFi.mode(WIFI_STA);
    if (_passSet)
      WiFi.begin(_currentNet.ssid, _currentNet.pass);
    else
      WiFi.begin(_currentNet.ssid);

    // as long as an MQTT IP has been set create an instance of PubSub for client
    if (_mqttSet) {
      // make MQTT client use either the secure or non-secure wifi client depending on the setting
      if (_useSecureClient)
        client = PubSubClient(_currentNet.mqttHost, _currentNet.mqttPort, wifiClientSecure);
      else
        client = PubSubClient(_currentNet.mqttHost, _currentNet.mqttPort, wifiClient);

      // set the MQTT message callback if needed
      if (_mqttCallbackSet)
        client.setCallback(_mqttCallback);
    } else {
      // define a dummy instance of MQTT so that it is instantiated if no MQTT ip is set
      // make MQTT client use either the secure or non-secure wifi client depending on the setting
      // (this shouldnt be needed if making a dummy connection since the idea would be that there wont be MQTT in this case)
      if (_useSecureClient)
        client = PubSubClient("192.0.2.0", _currentNet.mqttPort, wifiClientSecure);
      else
        client = PubSubClient("192.0.2.0", _currentNet.mqttPort, wifiClient);
    }

    // OTA event handlers
    ArduinoOTA.onStart([]() {/* ota start code */});
    ArduinoOTA.onEnd([this]() {
      // on OTA end we disconnect from Wi-Fi cleanly before restarting.
      safeApDisconnect();
    });
    ArduinoOTA.onProgress([](unsigned int progress, unsigned int total) {/* ota progress code */});
    ArduinoOTA.onError([](ota_error_t error) {/* ota error code */});

    // initially attempt to connect to Wi-Fi when we begin
    // (but only block for 2 seconds before timing out)
    int timeout = 0;  // counter for begin connection attempts
    while (((!client.connected() && _mqttSet) || WiFi.status() != WL_CONNECTED) && timeout < 200 ) {
    // max 2 seconds before timeout
      reconnect();
      delay(10);
      timeout++;
    }

    // attempt to start OTA if needed
    OTA_begin();

    // mark the system as started and return
    _hasBegun = true;
    return true;
  }

  // if no SSID was set even then don't try to begin and return false
  return false;
}

// end the instance of ESPHelper (shutdown Wi-Fi, OTA, MQTT)
void ESPHelper::end(){
  ESPHelperFS::end();
  OTA_disable();
  client.disconnect();
  safeApDisconnect();
  _connectionStatus = NO_CONNECTION;
}

// attempts to load a config file from the filesystem - returns blank netInfo on failure
// This will also create a new config with default values if none currently exists or is corrupted.
netInfo ESPHelper::loadConfigFile(const char* filename) {
  bool configLoaded = false;
  netInfo returnConfig;

  // create a static instance of the FS handler so that one isn't created
  // by default but is left in memory if we need to load a config
  static ESPHelperFS configLoader(filename);

  // attempt to load the config 3 times (really if it fails once it's probably going to fail again but why not try...)
  for (int tryCount = 0; tryCount < 3 && !configLoaded; tryCount++) {
    // attempt to start the filesystem
    if (configLoader.begin()) {
      // attempt to load a configuration
      configLoaded = configLoader.loadNetworkConfig();
      // if no config loaded (either from corruption or no file),
      // create a new config and close the FS
      if (!configLoaded) {
        // debugPrintln("Could not load config - generating new config and restarting...");  // Debug Print
        configLoader.createConfig(filename);
        configLoader.end();
        // debugPrintln("Config File loading failed. Retrying...");  // Debug Print
      } else {
        // if there is a good config, load it and close the FS
        // debugPrintln("Config loaded, getting info");  // Debug Print
        returnConfig = configLoader.getNetInfo();
        configLoader.end();
        // debugPrintln("done.");  // Debug Print
      }
    }
  }
  // return either a loaded config or a blank one
  return returnConfig;
}

// attempts to saves a new config file with a given netInfo and filename
// Returns true on success / false on failure
bool ESPHelper::saveConfigFile(const netInfo config, const char* filename) {
  // init ESPHelper FS and begin
  ESPHelperFS configLoader(filename);
  if (configLoader.begin()) {
    configLoader.createConfig(&config);
    configLoader.end();
    return true;
  }
  // if the FS could not be started then return false (failed)
  return false;
}

// enables the use of a secure (SSL) connection to an MQTT broker.
// (Make sure your mqtt port is set to one expecting a secure connection)
void ESPHelper::useSecureClient(const char* fingerprint) {
  _fingerprint = fingerprint;

  // fall back to Wi-Fi only connection if it was previously at full connection
  // (because we just changed how the device is going to connect to the mqtt broker)
  if (setConnectionStatus() == FULL_CONNECTION)
    _connectionStatus = WIFI_ONLY;

  // if use of secure connection is set retroactivly (after begin)
  // then re-instantiate client
  if (_hasBegun)
    client = PubSubClient(_currentNet.mqttHost, _currentNet.mqttPort, wifiClientSecure);

  // flag use of secure client
  _useSecureClient = true;
}

// enables and sets up broadcast mode rather than station mode. This allows users to create a network from the ESP
// and upload using OTA even if there is no network already present. This disables all MQTT connections
void ESPHelper::broadcastMode(const char* ssid, const char* password, const IPAddress ip) {
  // disconnect from any previous wifi networks (max timeout of 2 seconds)
  safeApDisconnect();
  // set the mode for access point
  WiFi.mode(WIFI_AP);
  // config the AP
  WiFi.softAPConfig(ip, ip, IPAddress(255, 255, 255, 0));
  // set the ssid and password
  WiFi.softAP(ssid, password);

  // update the connection status
  _connectionStatus = BROADCAST;
  _broadcastIP = ip;
  strcpy(_broadcastSSID, ssid);
  strcpy(_broadcastPASS, password);
}

// disable broadcast mode and reset to station mode
// (causes a call to begin - may want to change this in the future...)
void ESPHelper::disableBroadcast() {
  // disconnect from any previous wifi networks (max timeout of 2 seconds)
  safeApDisconnect();
  _connectionStatus = NO_CONNECTION;
  begin();
}

// main loop - should be called as often as possible - handles Wi-Fi/MQTT connection and MQTT handler
// true on: network / server connected
// false on: network or server disconnected
int ESPHelper::loop(){
  if (_ssidSet) {
    // check for good connections and attempt a reconnect if needed
    if ( ((_mqttSet && !client.connected()) || setConnectionStatus() < WIFI_ONLY)
          && _connectionStatus != BROADCAST) {
      reconnect();
    }

    // run the Wi-Fi loop as long as the connection status is at a minimum of BROADCAST
    if (_connectionStatus >= BROADCAST) {
      // run the MQTT loop if we have a full connection
      if (_connectionStatus == FULL_CONNECTION)
        client.loop();

      // run the heartbeat - why?
      // heartbeat();

      // check for whether we want to use OTA and whether the system is running
      if (_useOTA && _OTArunning)
        ArduinoOTA.handle();
      else if (_useOTA && !_OTArunning) {
        // if we want to use OTA but it's not running yet, start it up.
        OTA_begin();
        ArduinoOTA.handle();
      }
      return _connectionStatus;
    }
  }
  // return -1 for no connection because of bad network info
  return -1;
}

// subscribe to a speicifc topic (does not add to topic list)
// true on: subscription success
// false on: subscription failed (either from PubSub lib or network is disconnected)
bool ESPHelper::subscribe(const char* topic, int qos) {
  if (_connectionStatus == FULL_CONNECTION) {
    // set the return value to the output of subscribe
    bool returnVal = client.subscribe(topic, qos);
    // loop MQTT client
    client.loop();
    return returnVal;
  }
  // if not fully connected return false
  return false;
}

// add a topic to the list of subscriptions and attempt to subscribe to the topic on the spot
// true on: subscription added to list
// (does not guarantee that the topic was subscribed to, only that it was added to the list)
// false on: subscription not added to list
bool ESPHelper::addSubscription(const char* topic) {
  // default return value is false
  bool subscribed = false;

  // loop through finding the next available slot for a subscription and add it
  for (int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
    if (_subscriptions[i].isUsed == false) {
      _subscriptions[i].topic = topic;
      _subscriptions[i].isUsed = true;
      subscribed = true;
      break;
    }
  }

  // if added to the list, subscibe to the topic
  if (subscribed)
    subscribe(topic, _qos);

  return subscribed;
}

// loops through the list of subscriptions and attempts to subscribe to all topics
void ESPHelper::resubscribe() {
  for(int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
    if (_subscriptions[i].isUsed) {
      subscribe(_subscriptions[i].topic, _qos);
      yield();
    }
  }
}

// attempts to remove a topic from the topic list
// true on: subscription removed from list
// (does not guarantee that the topic was unsubscribed from, only that it was removed from the list)
// false on: topic was not found in list and therefore cannot be removed
bool ESPHelper::removeSubscription(const char* topic) {
  bool returnVal = false;
  String topicStr = topic;

  // loop through all subscriptions
  for (int i = 0; i < MAX_SUBSCRIPTIONS; i++) {
    // if an element is used, check for it being the one we want to remove
    if (_subscriptions[i].isUsed) {
      String subStr = _subscriptions[i].topic;
      if (subStr.equals(topicStr)) {
        // reset the used flag to false
        _subscriptions[i].isUsed = false;

        // unsubscribe
        client.unsubscribe(_subscriptions[i].topic);
        returnVal = true;
        break;
      }
    }
  }

  return returnVal;
}

// manually unsubscribes from a topic
// (This is basically just a wrapper for the pubsubclient function)
bool ESPHelper::unsubscribe(const char* topic) {
  return client.unsubscribe(topic);
}

// publish to a specified topic
void ESPHelper::publish(const char* topic, const char* payload) {
  publish(topic, payload, false);
}

// publish to a specified topic with a given retain level
void ESPHelper::publish(const char* topic, const char* payload, bool retain) {
  client.publish(topic, payload, retain);
}

// set the callback function for MQTT
void ESPHelper::setMQTTCallback(MQTT_CALLBACK_SIGNATURE) {
  _mqttCallback = callback;

  // only set the callback if using MQTT AND the system has already been started.
  // Otherwise just save it for later
  if(_hasBegun && _mqttSet)
    client.setCallback(_mqttCallback);

  _mqttCallbackSet = true;
}

// legacy funtion - here for compatibility.
// Sets the callback function for MQTT (see function above)
bool ESPHelper::setCallback(MQTT_CALLBACK_SIGNATURE) {
  setMQTTCallback(callback);
  return true;
}

// sets a custom function to run when connection to Wi-Fi is established
void ESPHelper::setWifiCallback(void (*callback)()) {
  _wifiCallback = callback;
  _wifiCallbackSet = true;
}

// attempts to connect to Wi-Fi & MQTT server if not connected
void ESPHelper::reconnect() {
  static int tryCount = 0;

  if (reconnectMetro.check()
      && _connectionStatus != BROADCAST
      && setConnectionStatus() != FULL_CONNECTION) {
    // debugPrintln("Attempting Wi-Fi Connection...");  // Debug Print
    // attempt to connect to the Wi-Fi if connection is lost
    if (WiFi.status() != WL_CONNECTED) {
      _connectionStatus = NO_CONNECTION;

      // increment try count each time it cannot connect
      // (this is used to determine when to hop to a new network)
      tryCount++;
      if(tryCount == 20){
        // change networks (if possible) when we have
        // tried to connnect 20 times unsucessfully
        changeNetwork();
        tryCount = 0;
        return;
      }
    }

    // make sure we are connected to WIFI before attemping to reconnect to MQTT
    // ----note---- maybe want to reset tryCount whenever we succeed at getting Wi-Fi connection?
    if (WiFi.status() == WL_CONNECTED) {
      // if the Wi-Fi previously wasnt connected but now is, run the callback
      if (_connectionStatus < WIFI_ONLY && _wifiCallbackSet)
        _wifiCallback();

      // debugPrintln("\n---WIFI Connected!---");  // Debug Print
      _connectionStatus = WIFI_ONLY;

      // attempt to connect to MQTT when we finally get connected to Wi-Fi
      if (_mqttSet) {
        static int timeout = 0;  // allow a max of 5 MQTT connection attempts before timing out
        if (!client.connected() && timeout < 5) {
          // debugPrint("Attemping MQTT connection");  // Debug Print
          int connected = 0;

          // connect to MQTT with user/pass
          if (_mqttUserSet && _willMessageSet) {
            // debugPrintln(" - Using user & last will");  // Debug Print
            // debugPrintln(String("\t Client Name: " + String(_clientName.c_str())));  // Debug Print
            // debugPrintln(String("\t User Name: " + String(_currentNet.mqttUser)));  // Debug Print
            // debugPrintln(String("\t Password: " + String(_currentNet.mqttPass)));  // Debug Print
            // debugPrintln(String("\t Will Topic: " + String(_currentNet.willTopic)));  // Debug Print
            // debugPrintln(String("\t Will QOS: " + String(_currentNet.willQoS)));  // Debug Print
            // debugPrintln(String("\t Will Retain?: " + String(_currentNet.willRetain)));  // Debug Print
            // debugPrintln(String("\t Will Message: " + String(_currentNet.willMessage)));  // Debug Print
            connected = client.connect((char*) _clientName.c_str(),
                                       _currentNet.mqttUser,
                                       _currentNet.mqttPass,
                                       _currentNet.willTopic,
                                       (int) _currentNet.willQoS,
                                       _currentNet.willRetain,
                                       (char*) _currentNet.willMessage);
          }

          // connect to MQTT without credentials
          else if (!_mqttUserSet && _willMessageSet) {
            // debugPrintln(" - Using last will");  // Debug Print
            // debugPrintln(String("\t Client Name: " + String(_clientName.c_str())));  // Debug Print
            // debugPrintln(String("\t Will Topic: " + String(_currentNet.willTopic)));  // Debug Print
            // debugPrintln(String("\t Will QOS: " + String(_currentNet.willQoS)));  // Debug Print
            // debugPrintln(String("\t Will Retain?: " + String(_currentNet.willRetain)));  // Debug Print
            // debugPrintln(String("\t Will Message: " + String(_currentNet.willMessage)));  // Debug Print
            connected = client.connect((char*) _clientName.c_str(),
                                       _currentNet.willTopic,
                                       (int) _currentNet.willQoS,
                                       _currentNet.willRetain,
                                       (char*) _currentNet.willMessage);
          } else if (_mqttUserSet && !_willMessageSet) {
            // debugPrintln(" - Using user");  // Debug Print
            // debugPrintln(String("\t Client Name: " + String(_clientName.c_str())));  // Debug Print
            // debugPrintln(String("\t User Name: " + String(_currentNet.mqttUser)));  // Debug Print
            // debugPrintln(String("\t Password: " + String(_currentNet.mqttPass)));  // Debug Print
            connected = client.connect((char*) _clientName.c_str(),
                                       _currentNet.mqttUser,
                                       _currentNet.mqttPass);
          } else {
            // debugPrintln(" - Using default");  // Debug Print
            // debugPrintln(String("\t Client Name: " + String(_clientName.c_str())));  // Debug Print
            connected = client.connect((char*) _clientName.c_str());
          }

          // if connected, subscribe to the topic(s) we want to be notified about
          if (connected) {
            // debugPrintln(" -- Connected");  // Debug Print

            // if using https, verify the fingerprint of the server
            // before setting full connection (return on fail)
            if (_useSecureClient) {
              if (wifiClientSecure.verify(_fingerprint, _currentNet.mqttHost)) {
                // debugPrintln("Certificate Matches - SUCESS");  // Debug Print
              } else {
                // debugPrintln("Certificate Doesn't Match - FAIL");  // Debug Print
                return;
              }
            }

            _connectionStatus = FULL_CONNECTION;
            resubscribe();
            timeout = 0;
          } else {
            // debugPrintln(" -- Failed");  // Debug Print
          }
          timeout++;
        }

        // if we still can't connect to MQTT after 10 attempts increment the try count
        if (timeout >= 5 && !client.connected()) {
          timeout = 0;
          tryCount++;
          if (tryCount == 20) {
            changeNetwork();
            tryCount = 0;
            return;
          }
        }
      }
    }

    //reset the reconnect metro
    reconnectMetro.reset();
  }
}

int ESPHelper::setConnectionStatus() {
  // assume no connection
  int returnVal = NO_CONNECTION;

  // make sure were not in broadcast mode
  if (_connectionStatus != BROADCAST) {
    // if connected to Wi-Fi set the mode to Wi-Fi only and run the callback if needed
    if (WiFi.status() == WL_CONNECTED) {
        //if the Wi-Fi previously wasnt connected but now is, run the callback
      if (_connectionStatus < WIFI_ONLY && _wifiCallbackSet)
        _wifiCallback();

      returnVal = WIFI_ONLY;

      // if MQTT is connected as well then set the status to full connection
      if (client.connected())
        returnVal = FULL_CONNECTION;
    }
  } else {
    returnVal = BROADCAST;
  }

  // set the connection status and return
  _connectionStatus = returnVal;
  return returnVal;
}

// changes the current network settings to the next listed network
// if network hopping is allowed
void ESPHelper::changeNetwork() {
  // only attempt to change networks if hopping is allowed
  if (_hoppingAllowed) {
    // change the index/reset to 0 if we've hit the last network setting
    _currentIndex++;
    if (_currentIndex >= _netCount)
      _currentIndex = 0;

    // set the current netlist to the new network
    _currentNet = *_netList[_currentIndex];

    // verify various bits of network info

    // network password
    if (_currentNet.pass[0] == '\0')
      _passSet = false;
    else
      _passSet = true;

    // SSID
    if (_currentNet.ssid[0] == '\0')
      _ssidSet = false;
    else
      _ssidSet = true;

    // MQTT host
    if (_currentNet.mqttHost[0] == '\0')
      _mqttSet = false;
    else
      _mqttSet = true;

    // MQTT username
    if (_currentNet.mqttUser[0] == '\0')
      _mqttUserSet = false;
    else
      _mqttUserSet = true;

    // MQTT password
    if (_currentNet.mqttPass[0] == '\0')
      _mqttPassSet = false;
    else
      _mqttPassSet = true;

    // Will Topic
    if (_currentNet.willTopic[0] == '\0')
      _willTopicSet = false;
    else
      _willTopicSet = true;

    // Will Message
    if (_currentNet.willTopic[0] == '\0')
      _willMessageSet = false;
    else
      _willMessageSet = true;

    // debugPrint("Trying next network: ");  // Debug Print
    // debugPrintln(_currentNet.ssid);  // Debug Print

    // update the network connection
    updateNetwork();
  }
}

void ESPHelper::updateNetwork() {
  // debugPrintln("\tDisconnecting from WiFi");  // Debug Print
  WiFi.disconnect();
  // debugPrintln("\tAttempting to begin on new network");  // Debug Print

  // set the Wi-Fi mode
  WiFi.mode(WIFI_STA);

  // connect to the network
  if (_passSet && _ssidSet)
    WiFi.begin(_currentNet.ssid, _currentNet.pass);
  else if (_ssidSet)
    WiFi.begin(_currentNet.ssid);
  else
    WiFi.begin("ESP8266_NO_SSID_SET");

  // debugPrintln("\tSetting new MQTT server");  // Debug Print
  // setup the MQTT broker info
  if (_mqttSet)
    client.setServer(_currentNet.mqttHost, _currentNet.mqttPort);
  else
    client.setServer("192.0.2.0", 1883);

  // debugPrintln("\tDone - Ready for next reconnect attempt");  // Debug Print
}

// generate unique MQTT name from MAC addr
String ESPHelper::macToStr(const uint8_t* mac) {
  String result;
  for (int i = 0; i < 6; ++i) {
    result += String(mac[i], 16);
    if (i < 5)
      result += ':';
  }
  return result;
}

// change the current network info to a new netInfo
// does not automatically disconnect from current network if already connected
void ESPHelper::setNetInfo(netInfo newNetwork) {
  _currentNet = newNetwork;
  _ssidSet = true;
  _passSet = true;
  _mqttSet = true;
  _mqttUserSet = true;
}

// change the current network info to a new *netInfo
// does not automatically disconnect from current network if already connected
void ESPHelper::setNetInfo(netInfo *newNetwork) {
  _currentNet = *newNetwork;
  _ssidSet = true;
  _passSet = true;
  _mqttSet = true;
  _mqttUserSet = true;
}

// return the current netInfo state
netInfo ESPHelper::getNetInfo() {
  return _currentNet;
}

// return the current SSID
const char* ESPHelper::getSSID() {     
  if (_ssidSet && _connectionStatus != BROADCAST)
    return _currentNet.ssid;
  else if (_connectionStatus == BROADCAST)
    return _broadcastSSID;
  return "SSID IS NOT SET";
}

// set a new SSID - does not automatically disconnect
// from current network if already connected
void ESPHelper::setSSID(const char* ssid) {
  _currentNet.ssid = ssid;
  _ssidSet = true;
}

// return the current network password
const char* ESPHelper::getPASS() {
  if( _passSet && _connectionStatus != BROADCAST)
    return _currentNet.pass;
  else if (_connectionStatus == BROADCAST)
    return _broadcastPASS;
  return "PASSWORD IS NOT SET";
}

// set a new network password - does not automatically disconnect
// from current network if already connected
void ESPHelper::setPASS(const char* pass) {
  _currentNet.pass = pass;
  _passSet = true;
}

// return the current MQTT server IP
const char* ESPHelper::getMQTTIP() {
  if (_mqttSet)
    return _currentNet.mqttHost;
  return "MQTT IP NOT SET";
}

// set a new MQTT server IP - does not automatically
// disconnect from current network/server if already connected
void ESPHelper::setMQTTIP(const char* mqttIP) {
  _currentNet.mqttHost= mqttIP;
  _mqttSet = true;
}

// set a new MQTT server IP - does not automatically disconnect from current network/server if already connected
void ESPHelper::setMQTTIP(const char* mqttIP, const char* mqttUser, const char* mqttPass) {
  _currentNet.mqttHost = mqttIP;
  _currentNet.mqttUser = mqttUser;
  _currentNet.mqttPass = mqttPass;
  _mqttSet = true;
  _mqttUserSet = true;
}

// set a new MQTT Will - does not automatically disconnect
// from current network/server if already connected
void ESPHelper::setWill(const char *willTopic, const char *willMessage) {
  _currentNet.willTopic = willTopic;
  _currentNet.willMessage = willMessage;
  _willTopicSet = true;
  _willMessageSet = true;
}

// set a new MQTT Will - does not automatically disconnect
// from current network/server if already connected
void ESPHelper::setWill(const char *willTopic,
                        const char *willMessage,
                        const int willQoS,
                        const int willRetain) {
  _currentNet.willTopic = willTopic;
  _currentNet.willMessage = willMessage;
  _currentNet.willQoS = willQoS;
  _currentNet.willRetain = willRetain;
  _willTopicSet = true;
  _willMessageSet = true;
}

// return the QOS level for MQTT
int ESPHelper::getMQTTQOS() {
  return _qos;
}

// set the QOS level for MQTT
void ESPHelper::setMQTTQOS(int qos) {
  _qos = qos;
}

// return the local IP address of the ESP as a string
String ESPHelper::getIP() {
  if (_connectionStatus != BROADCAST)
    return WiFi.localIP().toString(); 
  return _broadcastIP.toString();
}

// return the local IP address of the ESP
IPAddress ESPHelper::getIPAddress() {
  if(_connectionStatus != BROADCAST)
    return WiFi.localIP();  
  return _broadcastIP;
}

// get the current connection status of ESPHelper
int ESPHelper::getStatus() {
  return _connectionStatus;
}

// enable or disable hopping - generally set automatically by initializer
void ESPHelper::setHopping(bool canHop) {
  _hoppingAllowed = canHop;
}

// DEBUG ONLY - print the subscribed topics list to the serial line
void ESPHelper::listSubscriptions() {
  for(int i = 0; i < MAX_SUBSCRIPTIONS; i++){
    if(_subscriptions[i].isUsed){
      // debugPrintln(_subscriptions[i].topic);  // Debug Print
    }
  }
}

// enable the connection heartbeat on a given pin
void ESPHelper::enableHeartbeat(int16_t pin) {
  #ifdef DEBUG
    if (pin == 1) {
      _heartbeatEnabled = false;
    } else {
      _heartbeatEnabled = true;
      _ledPin = pin;
      pinMode(_ledPin, OUTPUT);
      digitalWrite(_ledPin, HIGH);
    }
  #else
    _heartbeatEnabled = true;
    _ledPin = pin;
    pinMode(_ledPin, OUTPUT);
    digitalWrite(_ledPin, HIGH);
  #endif
}

// disable the connection heartbeat
void ESPHelper::disableHeartbeat() {
  _heartbeatEnabled = false;
}

// heartbeat to indicate network connection
void ESPHelper::heartbeat() {
  static Metro heartbeatMetro = Metro(10);
  static int counter = 0;

  static bool ledState = true;

  if (heartbeatMetro.check() && _heartbeatEnabled) {
    if (counter == 1) {
      digitalWrite(_ledPin, ledState);
      heartbeatMetro.interval(10);
      ledState = !ledState;
    } else if (counter == 2) {
      digitalWrite(_ledPin, ledState);
      heartbeatMetro.interval(300);
      ledState = !ledState;
    } else if (counter == 3) {
      digitalWrite(_ledPin, ledState);
      heartbeatMetro.interval(10);
      ledState = !ledState;
    } else {
      digitalWrite(_ledPin, ledState);
      heartbeatMetro.interval(1000);
      ledState = !ledState;
      counter = 0;
    }
    counter++;
  }
}

// enable use of OTA updates
void ESPHelper::OTA_enable() {
  _useOTA = true;
  OTA_begin();
}

// begin the OTA subsystem but with a check
// for connectivity and enabled use of OTA
void ESPHelper::OTA_begin() {
  if (_connectionStatus >= BROADCAST && _useOTA) {
    ArduinoOTA.begin();
    _OTArunning = true;
  }
}

// disable use of OTA updates
void ESPHelper::OTA_disable() {
  _useOTA = false;
  _OTArunning = false;
}

// set a password for OTA updates
void ESPHelper::OTA_setPassword(const char* pass) {
  ArduinoOTA.setPassword(pass);
}

// set the hostname of the ESP for OTA uploads
void ESPHelper::OTA_setHostname(const char* hostname) {
  strcpy(_hostname, hostname);
  ArduinoOTA.setHostname(_hostname);
}

// set the hostname of the ESP for OTA uploads and append the ESPHelper version number
void ESPHelper::OTA_setHostnameWithVersion(const char* hostname) {
  strcpy(_hostname, hostname);
  strcat(_hostname, "---v");
  strcat(_hostname, VERSION);

  ArduinoOTA.setHostname(_hostname);
}

char* ESPHelper::getHostname() {
  return _hostname;
}

uint8_t ESPHelper::softAPgetStationNum() {
  return WiFi.softAPgetStationNum();
}
