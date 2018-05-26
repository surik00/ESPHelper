/*

HTML page is defined this way and not as variables
or constants to be away from RAM

by Suren Khorenyan

*/

#ifndef WebConfigHTML_H
#define WebConfigHTML_H


// Starting HTML, open head
#define HTML_START "<!DOCTYPE html><html><head>"

// Style (minified)
#define HTML_STYLE "<style type=\"text/css\">h2, label{margin-bottom: .5rem}button, hr, input{overflow: visible}.fc, body{background-color: #fff}.al, .cs3, .cs9{position: relative}*, ::after, ::before{box-sizing: border-box}body{margin: 0; font-family: -apple-system, BlinkMacSystemFont, \"Segoe UI\", Roboto, \"Helvetica Neue\", Arial, sans-serif, \"Apple Color Emoji\", \"Segoe UI Emoji\", \"Segoe UI Symbol\"; font-size: 1rem; font-weight: 400; line-height: 1.5; color: #212529; text-align: left}button, h2, input{font-family: inherit}label{display: inline-block}button{border-radius: 0; text-transform: none}button:focus{outline: dotted 1px; outline: -webkit-focus-ring-color auto 5px}.btn:focus, .fc:focus{outline: 0; box-shadow: 0 0 0 .2rem rgba(0, 123, 255, .25)}button, input{margin: 0; font-size: inherit; line-height: inherit}[type=submit], button{-webkit-appearance: button}[type=submit]::-moz-focus-inner, button::-moz-focus-inner{padding: 0; border-style: none}.c, .cs3, .cs9{padding-right: 15px; padding-left: 15px; width: 100%}h2{font-weight: 500; line-height: 1.2; color: inherit; font-size: 2rem}.btn, .btn-sm, .cfl, .fc{line-height: 1.5}.fc, .fc:focus{color: #495057}hr{margin-top: 1rem; margin-bottom: 1rem; border: 0; border-top: 1px solid rgba(0, 0, 0, .1); box-sizing: content-box; height: 0}.c{margin-right: auto; margin-left: auto}@media (min-width:576px){.c{max-width: 540px}}@media (min-width:768px){.c{max-width: 720px}}@media (min-width:992px){.c{max-width: 960px}}@media (min-width:1200px){.c{max-width: 1140px}}.cs3, .cs9{min-height: 1px}@media (min-width:576px){.cs3{-ms-flex: 0 0 25%; flex: 0 0 25%; max-width: 25%}.cs9{-ms-flex: 0 0 75%; flex: 0 0 75%; max-width: 75%}}.fc{display: block; width: 100%; padding: .375rem .75rem; font-size: 1rem; background-clip: padding-box; border: 1px solid #ced4da; border-radius: .25rem; transition: border-color .15s ease-in-out, box-shadow .15s ease-in-out}.fc::-ms-expand{background-color: transparent; border: 0}.fc:focus{background-color: #fff; border-color: #80bdff}.fc::-webkit-input-placeholder{color: #6c757d; opacity: 1}.fc::-moz-placeholder{color: #6c757d; opacity: 1}.fc:-ms-input-placeholder{color: #6c757d; opacity: 1}.fc::-ms-input-placeholder{color: #6c757d; opacity: 1}.fc::placeholder{color: #6c757d; opacity: 1}.cfl{padding-top: calc(.375rem + 1px); padding-bottom: calc(.375rem + 1px); margin-bottom: 0; font-size: inherit}.al, .fg{margin-bottom: 1rem}.fg{display: -ms-flexbox; display: flex; -ms-flex-wrap: wrap; flex-wrap: wrap; margin-right: -15px; margin-left: -15px}.btn{display: inline-block; font-weight: 400; text-align: center; white-space: nowrap; vertical-align: middle; -webkit-user-select: none; -moz-user-select: none; -ms-user-select: none; user-select: none; border: 1px solid transparent; padding: .375rem .75rem; font-size: 1rem; border-radius: .25rem; transition: color .15s ease-in-out, background-color .15s ease-in-out, border-color .15s ease-in-out, box-shadow .15s ease-in-out}.btn:focus, .btn:hover{text-decoration: none}.btn-pr{color: #fff; background-color: #007bff; border-color: #007bff}.btn-pr:hover{color: #fff; background-color: #0069d9; border-color: #0062cc}.btn-pr:focus{box-shadow: 0 0 0 .2rem rgba(0, 123, 255, .5)}.btn-d{color: #fff; background-color: #dc3545; border-color: #dc3545}.btn-d:hover{color: #fff; background-color: #c82333; border-color: #bd2130}.btn-d:focus, .btn-danger.focus{box-shadow: 0 0 0 .2rem rgba(220, 53, 69, .5)}.al{padding: .75rem 1.25rem; border: 1px solid transparent; border-radius: .25rem}.ai{color: #0c5460; background-color: #d1ecf1; border-color: #bee5eb}.ad{color: #721c24; background-color: #f8d7da; border-color: #f5c6cb}.btn-sm{padding: .25rem .5rem; font-size: .875rem; border-radius: .2rem}a{color: #007bff; text-decoration: none; background-color: transparent; -webkit-text-decoration-skip: objects;}a:hover{color: #0056b3; text-decoration: underline;}code{color: #e83e8c; word-break: break-word; font-family: SFMono-Regular, Menlo, Monaco, Consolas, \"Liberation Mono\", \"Courier New\", monospace; font-size: 1em;}</style>"

// Open title
#define HTML_S_T "<meta name=\"viewport\" content=\"width=device-width, initial-scale=1\"><title>"

// Close title, close head, open container div, open heading
#define HTML_T_B "</title></head><body><br><div class=\"c\">"

// Config page heading
#define HTML_CFG_HEADING "<h2>ESP8266 System Configuration</h2>"

// Info page heading
#define HTML_INFO_HEADING "<h2>ESP8266 System Status</h2>"

// Alert; open form, open config POST URI
#define HTML_CFG_AL "<br><div class=\"al ai\">If you leave blank password fields (for OTA, Wi-Fi network and MQTT), previous values will be used.</div><form action=\""

// Close config POST URI, open hostname value
#define HTML_CFG_1 "\" method=\"POST\"><div class=\"fg\"><label for=\"dn\" class=\"cs3 cfl\">Device Name</label><div class=\"cs9\"><input type=\"text\" name=\"hostname\" required=\"\" placeholder=\"My-ESP8266 (Required)\" id=\"dn\" class=\"fc\" maxlength=\"63\" value=\""

// Close hostname value; OTA Password; open SSID
#define HTML_CFG_2 "\"></div></div><div class=\"fg\"><label for=\"op\" class=\"cs3 cfl\">OTA Password</label><div class=\"cs9\"><input type=\"password\" name=\"otaPassword\" placeholder=\"OTA Password\" id=\"op\" class=\"fc\" maxlength=\"63\"></div></div><hr><div class=\"fg\"><label for=\"sn\" class=\"cs3 cfl\">SSID</label><div class=\"cs9\"><input type=\"text\" name=\"ssid\" required=\"\" placeholder=\"My home Wi-Fi Network (Required)\" id=\"sn\" class=\"fc\" maxlength=\"63\" value=\""

// Close SSID; W-Fi password; open MQTT Broker Host
#define HTML_CFG_3 "\"></div></div><div class=\"fg\"><label for=\"wp\" class=\"cs3 cfl\">Wi-Fi Password</label><div class=\"cs9\"><input type=\"password\" name=\"netPass\" placeholder=\"Wi-Fi Password\" id=\"wp\" class=\"fc\" maxlength=\"63\"></div></div><hr><div class=\"fg\"><label for=\"mh\" class=\"cs3 cfl\">MQTT Broker Host</label><div class=\"cs9\"><input type=\"text\" name=\"mqttHost\" placeholder=\"192.168.42.13\" id=\"mh\" class=\"fc\" maxlength=\"63\" value=\""

// Close MQTT Host, open MQTT Port
#define HTML_CFG_4 "\"></div></div><div class=\"fg\"><label for=\"mp\" class=\"cs3 cfl\">MQTT Port</label><div class=\"cs9\"><input type=\"text\" name=\"mqttPort\" placeholder=\"1883 (default)\" id=\"mp\" class=\"fc\" maxlength=\"5\" value=\""

// Close MQTT Port, open MQTT Username
#define HTML_CFG_5 "\"></div></div><div class=\"fg\"><label for=\"mu\" class=\"cs3 cfl\">MQTT Username</label><div class=\"cs9\"><input type=\"text\" name=\"mqttUser\" placeholder=\"MQTT User\" id=\"mu\" class=\"fc\" maxlength=\"63\" value=\""

// Close MQTT Username; MQTT Password; close form
#define HTML_CFG_6 "\"></div></div><div class=\"fg\"><label for=\"qp\" class=\"cs3 cfl\">MQTT Password</label><div class=\"cs9\"><input type=\"password\" name=\"mqttPass\" placeholder=\"MQTT password\" id=\"qp\" class=\"fc\" maxlength=\"63\"></div></div><button type=\"submit\" class=\"btn btn-pr\">Apply changes</button></form>"

// Resetting form
#define HTML_CFG_RESET_START "<hr><br><div class=\"al ad\"><b>WARNING!</b> This will clear the filesystem! All stored files will be removed (including the network configuration file)!</div><form action=\""
#define HTML_CFG_RESET_END "\" method=\"POST\"><button type=\"submit\" class=\"btn btn-d btn-sm\">Format filesystem</button></form>"


// Info lines

// Open Device Name
#define HTML_INFO_1 "<br><div class=\"fg\"><div class=\"cs3\">Device Name:</div><div class=\"cs9\"><code>"

// Close Device Name, open Wi-Fi name
#define HTML_INFO_2 "</code></div></div><div class=\"fg\"><div class=\"cs3\">Wi-Fi to connect</div><div class=\"cs9\"><code>"

// Close Wi-Fi name, open MQTT Broker Host
#define HTML_INFO_3 "</code></div></div><div class=\"fg\"><div class=\"cs3\">MQTT Host</div><div class=\"cs9\"><code>"

// Close MQTT Broker Host, open uptime
#define HTML_INFO_4 "</code></div></div><div class=\"fg\"><div class=\"cs3\">Uptime (ms):</div><div class=\"cs9\"><code>"

// Close uptime, open configuration page URI
#define HTML_INFO_5 "</code></div></div><br><div><a href=\""

// Close configure page URI
#define HTML_INFO_6 "\">Go to the config page</a></div>"

// - info


// Close container div, 
#define HTML_CLOSE "</div><br></body></html>"


//
// Small responses

#define HTML_400_PARAMS_MISSING "<h1>Invalid Request - Did you make sure to specify an SSID and Hostname?</h1>"
#define HTML_400_MQTT_NO_HOST "<h1>Invalid Request - MQTT info specified without host</h1>"
#define HTML_CONFIG_SUCCESS "<h1>Config info successfully loaded, restarting!</h1>"
#define HTML_SPIFFS_FORMAT "<h1>Formatting SPIFFS and restarting with default values</h1>"


#endif
