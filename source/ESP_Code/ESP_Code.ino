/*
 * ESPloitV2
 * WiFi controlled HID Keyboard Emulator
 * By Corey Harding of www.Exploit.Agency / www.LegacySecurityGroup.com
 * Special thanks to minkione for helping port/test original V1 code to the Cactus Micro rev2
 * ESPloit is distributed under the MIT License. The license and copyright notice can not be removed and must be distributed alongside all future copies of the software.
 * MIT License
    
    Copyright (c) [2017] [Corey Harding]
    
    Permission is hereby granted, free of charge, to any person obtaining a copy
    of this software and associated documentation files (the "Software"), to deal
    in the Software without restriction, including without limitation the rights
    to use, copy, modify, merge, publish, distribute, sublicense, and/or sell
    copies of the Software, and to permit persons to whom the Software is
    furnished to do so, subject to the following conditions:
    
    The above copyright notice and this permission notice shall be included in all
    copies or substantial portions of the Software.
    
    THE SOFTWARE IS PROVIDED "AS IS", WITHOUT WARRANTY OF ANY KIND, EXPRESS OR
    IMPLIED, INCLUDING BUT NOT LIMITED TO THE WARRANTIES OF MERCHANTABILITY,
    FITNESS FOR A PARTICULAR PURPOSE AND NONINFRINGEMENT. IN NO EVENT SHALL THE
    AUTHORS OR COPYRIGHT HOLDERS BE LIABLE FOR ANY CLAIM, DAMAGES OR OTHER
    LIABILITY, WHETHER IN AN ACTION OF CONTRACT, TORT OR OTHERWISE, ARISING FROM,
    OUT OF OR IN CONNECTION WITH THE SOFTWARE OR THE USE OR OTHER DEALINGS IN THE
    SOFTWARE.
*/

//We need this stuff
#include "HelpText.h"
#include "License.h"
#include <ESP8266WiFi.h>
#include <WiFiClient.h>
#include <ESP8266WebServer.h>
#include <ESP8266HTTPUpdateServer.h>
#include <ESP8266mDNS.h>
#include <FS.h>
#include <ArduinoJson.h> // ArduinoJson library 5.11.0 by Benoit Blanchon https://github.com/bblanchon/ArduinoJson
//#include <SoftwareSerial.h>
//#include <DoubleResetDetector.h> // Double Reset Detector library VERSION: 1.0.0 by Stephen Denne https://github.com/datacute/DoubleResetDetector

//Setup RX and TX pins to be used for the software serial connection
//const int RXpin=5;
//const int TXpin=4;
//SoftwareSerial SOFTserial(RXpin,TXpin);

//Double Reset Detector
/*
#define DRD_TIMEOUT 5
#define DRD_ADDRESS 0
DoubleResetDetector drd(DRD_TIMEOUT, DRD_ADDRESS);
*/

// Port for web server
ESP8266WebServer server(80);
ESP8266WebServer httpServer(1337);
ESP8266HTTPUpdateServer httpUpdater;

const char* update_path = "/update";
int accesspointmode;
char ssid[32];
char password[64];
int channel;
int hidden;
char local_IPstr[16];
char gatewaystr[16];
char subnetstr[16];
char update_username[32];
char update_password[64];
int DelayLength;
int livepayloaddelay;
int autopwn;
char autopayload[64];

void runpayload() {
    File f = SPIFFS.open(autopayload, "r");
    int defaultdelay = DelayLength;
    int settingsdefaultdelay = DelayLength;
    int custom_delay;
    while(f.available()) {
//      SOFTserial.println(line);
//      Serial.println(line);
      String line = f.readStringUntil('\n');
      Serial.println(line);

      String fullkeys = line;
      int str_len = fullkeys.length()+1; 
      char keyarray[str_len];
      fullkeys.toCharArray(keyarray, str_len);

      char *i;
      String cmd;
      String cmdinput;
      cmd = String(strtok_r(keyarray,":",&i));

//         Serial.println(String()+"cmd:"+cmd);
//         Serial.println(String()+"cmdin:"+cmdinput);
     
      if(cmd == "Rem") {
        cmdinput = String(strtok_r(NULL,":",&i));
        DelayLength = 1500;
      }
      
      else if(cmd == "DefaultDelay") {
        cmdinput = String(strtok_r(NULL,":",&i));
        DelayLength = 1500;
        String newdefaultdelay = cmdinput;
        defaultdelay = newdefaultdelay.toInt();
//          Serial.println(String()+"default delay set to:"+defaultdelay);
      }
      else if(cmd == "CustomDelay") {
        cmdinput = String(strtok_r(NULL,":",&i));
        String customdelay = cmdinput;
        custom_delay = customdelay.toInt();
        DelayLength = custom_delay;
//          Serial.println(String()+"Custom delay set to:"+custom_delay);
      }
//        Serial.println(DelayLength);
      delay(DelayLength); //delay between lines in payload, I found running it slower works best
      DelayLength = defaultdelay;
    }
    f.close();
    DelayLength = settingsdefaultdelay;
}

void settingsPage()
{
  if(!server.authenticate(update_username, update_password))
    return server.requestAuthentication();
  String accesspointmodeyes;
  String accesspointmodeno;
  if (accesspointmode==1){
    accesspointmodeyes=" checked=\"checked\"";
    accesspointmodeno="";
  }
  else {
    accesspointmodeyes="";
    accesspointmodeno=" checked=\"checked\"";
  }
  String autopwnyes;
  String autopwnno;
  if (autopwn==1){
    autopwnyes=" checked=\"checked\"";
    autopwnno="";
  }
  else {
    autopwnyes="";
    autopwnno=" checked=\"checked\"";
  }
  String hiddenyes;
  String hiddenno;
  if (hidden==1){
    hiddenyes=" checked=\"checked\"";
    hiddenno="";
  }
  else {
    hiddenyes="";
    hiddenno=" checked=\"checked\"";
  }
  server.send(200, "text/html", 
  String()+
  "<!DOCTYPE HTML>"
  "<html>"
  "<head>"
  "<meta name = \"viewport\" content = \"width = device-width, initial-scale = 1.0, maximum-scale = 1.0, user-scalable=0\">"
  "<title>ESPloit Settings</title>"
  "<style>"
  "\"body { background-color: #808080; font-family: Arial, Helvetica, Sans-Serif; Color: #000000; }\""
  "</style>"
  "</head>"
  "<body>"
  "<a href=\"/\"><- BACK TO INDEX</a><br><br>"
  "<h1>ESPloit Settings</h1>"
  "<hr>"
  "<FORM action=\"/settings\"  id=\"configuration\" method=\"post\">"
  "<P>"
  "<b>WiFi Configuration:</b><br><br>"
  "<b>Network Type</b><br>"
  "Access Point Mode: <INPUT type=\"radio\" name=\"accesspointmode\" value=\"1\""+accesspointmodeyes+"><br>"
  "Join Existing Network: <INPUT type=\"radio\" name=\"accesspointmode\" value=\"0\""+accesspointmodeno+"><br><br>"
  "<b>Hidden<br></b>"
  "Yes <INPUT type=\"radio\" name=\"hidden\" value=\"1\""+hiddenyes+"><br>"
  "No <INPUT type=\"radio\" name=\"hidden\" value=\"0\""+hiddenno+"><br><br>"
  "SSID: <input type=\"text\" name=\"ssid\" value=\""+ssid+"\" maxlength=\"31\" size=\"31\"><br>"
  "Password: <input type=\"password\" name=\"password\" value=\""+password+"\" maxlength=\"64\" size=\"31\"><br>"
  "Channel: <select name=\"channel\" form=\"configuration\"><option value=\""+channel+"\" selected>"+channel+"</option><option value=\"1\">1</option><option value=\"2\">2</option><option value=\"3\">3</option><option value=\"4\">4</option><option value=\"5\">5</option><option value=\"6\">6</option><option value=\"7\">7</option><option value=\"8\">8</option><option value=\"9\">9</option><option value=\"10\">10</option><option value=\"11\">11</option><option value=\"12\">12</option><option value=\"13\">13</option><option value=\"14\">14</option></select><br><br>"
  "IP: <input type=\"text\" name=\"local_IPstr\" value=\""+local_IPstr+"\" maxlength=\"16\" size=\"31\"><br>"
  "Gateway: <input type=\"text\" name=\"gatewaystr\" value=\""+gatewaystr+"\" maxlength=\"16\" size=\"31\"><br>"
  "Subnet: <input type=\"text\" name=\"subnetstr\" value=\""+subnetstr+"\" maxlength=\"16\" size=\"31\"><br><br>"
  "<hr>"
  "<b>ESPloit Administration Settings:</b><br><br>"
  "Username: <input type=\"text\" name=\"update_username\" value=\""+update_username+"\" maxlength=\"31\" size=\"31\"><br>"
  "Password: <input type=\"password\" name=\"update_password\" value=\""+update_password+"\" maxlength=\"64\" size=\"31\"><br><br>"
  "<hr>"
  "<b>Payload Settings:</b><br><br>"
  "Delay Between Sending Lines of Code in Payload:<br><input type=\"number\" name=\"DelayLength\" value=\""+DelayLength+"\" maxlength=\"31\" size=\"10\"> milliseconds (Default: 2000)<br><br>"
  "Delay Before Starting a Live Payload:<br><input type=\"number\" name=\"LivePayloadDelay\" value=\""+livepayloaddelay+"\" maxlength=\"31\" size=\"10\"> milliseconds (Default: 3000)<br><br>"
  "<b>Automatically Deploy Payload Upon Insetion</b><br>"
  "Yes <INPUT type=\"radio\" name=\"autopwn\" value=\"1\""+autopwnyes+"><br>"
  "No <INPUT type=\"radio\" name=\"autopwn\" value=\"0\""+autopwnno+"><br><br>"
  "Automatic Payload: <input type=\"text\" name=\"autopayload\" value=\""+autopayload+"\" maxlength=\"64\" size=\"31\"><br><br>"
  "<INPUT type=\"radio\" name=\"SETTINGS\" value=\"1\" hidden=\"1\" checked=\"checked\">"
  "<hr>"
  "<INPUT type=\"submit\" value=\"Apply Settings\">"
  "</P>"
  "</FORM>"
  "</body>"
  "</html>"
  );
}

void handleSettings()
{
  if (server.hasArg("SETTINGS")) {
    handleSubmitSettings();
  }
  else {
    settingsPage();
  }
}

void returnFail(String msg)
{
  server.sendHeader("Connection", "close");
  server.sendHeader("Access-Control-Allow-Origin", "*");
  server.send(500, "text/plain", msg + "\r\n");
}

void handleSubmitSettings()
{
  String SETTINGSvalue;

  if (!server.hasArg("SETTINGS")) return returnFail("BAD ARGS");
  
  SETTINGSvalue = server.arg("SETTINGS");
  accesspointmode = server.arg("accesspointmode").toInt();
  server.arg("ssid").toCharArray(ssid, 32);
  server.arg("password").toCharArray(password, 64);
  channel = server.arg("channel").toInt();
  hidden = server.arg("hidden").toInt();
  server.arg("local_IPstr").toCharArray(local_IPstr, 16);
  server.arg("gatewaystr").toCharArray(gatewaystr, 16);
  server.arg("subnetstr").toCharArray(subnetstr, 16);
  server.arg("update_username").toCharArray(update_username, 32);
  server.arg("update_password").toCharArray(update_password, 64);
  DelayLength = server.arg("DelayLength").toInt();
  livepayloaddelay = server.arg("LivePayloadDelay").toInt();
  autopwn = server.arg("autopwn").toInt();
  server.arg("autopayload").toCharArray(autopayload, 64);
  
  if (SETTINGSvalue == "1") {
    saveConfig();
    server.send(200, "text/html", "<a href=\"/\"><- BACK TO INDEX</a><br><br>Settings have been saved.<br>If network configuration has changed then connect to the new network to access ESPloit.");
    loadConfig();
  }
  else if (SETTINGSvalue == "0") {
    settingsPage();
  }
  else {
    returnFail("Bad SETTINGS value");
  }
}

bool loadDefaults() {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();
  json["accesspointmode"] = "1";
  json["ssid"] = "Exploit";
  json["password"] = "DotAgency";
  json["channel"] = "6";
  json["hidden"] = "0";
  json["local_IP"] = "192.168.1.1";
  json["gateway"] = "192.168.1.1";
  json["subnet"] = "255.255.255.0";
  json["update_username"] = "admin";
  json["update_password"] = "hacktheplanet";
  json["DelayLength"] = "2000";
  json["LivePayloadDelay"] = "3000";
  json["autopwn"] = "0";
  json["autopayload"] = "/payloads/payload.txt";
  File configFile = SPIFFS.open("/config.json", "w");
  json.printTo(configFile);
  loadConfig();
}

bool loadConfig() {
  File configFile = SPIFFS.open("/config.json", "r");
  if (!configFile) {
    loadDefaults();
  }

  size_t size = configFile.size();

  std::unique_ptr<char[]> buf(new char[size]);
  configFile.readBytes(buf.get(), size);
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.parseObject(buf.get());

  strcpy(ssid, (const char*)json["ssid"]);
  strcpy(password, (const char*)json["password"]);
  channel = json["channel"];
  hidden = json["hidden"];
  accesspointmode = json["accesspointmode"];
  strcpy(local_IPstr, (const char*)json["local_IP"]);
  strcpy(gatewaystr, (const char*)json["gateway"]);
  strcpy(subnetstr, (const char*)json["subnet"]);

  strcpy(update_username, (const char*)json["update_username"]);
  strcpy(update_password, (const char*)json["update_password"]);

  DelayLength = json["DelayLength"];
  livepayloaddelay = json["LivePayloadDelay"];

  autopwn = json["autopwn"];
  strcpy(autopayload, (const char*)json["autopayload"]);

  IPAddress local_IP;
  local_IP.fromString(local_IPstr);
  IPAddress gateway;
  gateway.fromString(gatewaystr);
  IPAddress subnet;
  subnet.fromString(subnetstr);

/*
  Serial.println(accesspointmode);
  Serial.println(ssid);
  Serial.println(password);
  Serial.println(channel);
  Serial.println(hidden);
  Serial.println(local_IP);
  Serial.println(gateway);
  Serial.println(subnet);
*/

// Determine if set to Access point mode
  if (accesspointmode == 1) {
    ESP.eraseConfig();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_AP);
//    Serial.print("Setting up Network Configuration ... ");
//    Serial.println(WiFi.softAPConfig(local_IP, gateway, subnet) ? "Success" : "Failed!");
    WiFi.softAPConfig(local_IP, gateway, subnet);

//    Serial.print("Starting Access Point ... ");
//    Serial.println(WiFi.softAP(ssid, password, channel, hidden) ? "Success" : "Failed!");
    WiFi.softAP(ssid, password, channel, hidden);
//    WiFi.reconnect();

//    Serial.print("IP address = ");
//    Serial.println(WiFi.softAPIP());
  }
// or Join existing network
  else if (accesspointmode != 1) {
    ESP.eraseConfig();
    WiFi.disconnect(true);
    WiFi.mode(WIFI_STA);
//    Serial.print("Setting up Network Configuration ... ");
    WiFi.config(local_IP, gateway, subnet);
//    WiFi.config(local_IP, gateway, subnet);

//    Serial.print("Connecting to network ... ");
//    WiFi.begin(ssid, password);
    WiFi.begin(ssid, password);
    WiFi.reconnect();

//    Serial.print("IP address = ");
//    Serial.println(WiFi.localIP());
  }

  return true;
}

bool saveConfig() {
  StaticJsonBuffer<500> jsonBuffer;
  JsonObject& json = jsonBuffer.createObject();

  json["accesspointmode"] = accesspointmode;
  json["ssid"] = ssid;
  json["password"] = password;
  json["channel"] = channel;
  json["hidden"] = hidden;
  json["local_IP"] = local_IPstr;
  json["gateway"] = gatewaystr;
  json["subnet"] = subnetstr;
  json["update_username"] = update_username;
  json["update_password"] = update_password;
  json["DelayLength"] = DelayLength;
  json["LivePayloadDelay"] = livepayloaddelay;
  json["autopwn"] = autopwn;
  json["autopayload"] = autopayload;

  File configFile = SPIFFS.open("/config.json", "w");
  json.printTo(configFile);
  return true;
}

File fsUploadFile;
String webString;

void handleFileUpload()
{
  if(server.uri() != "/upload") return;
  HTTPUpload& upload = server.upload();
  if(upload.status == UPLOAD_FILE_START){
    String filename = upload.filename;
    filename = "/payloads/"+filename;
//    Serial.print("Uploading file "); 
//    Serial.print(filename+" ... ");
    fsUploadFile = SPIFFS.open(filename, "w");
    filename = String();
  }
  else if(upload.status == UPLOAD_FILE_WRITE){
    if(fsUploadFile)
    fsUploadFile.write(upload.buf, upload.currentSize);
  }
  else if(upload.status == UPLOAD_FILE_END){
    if(fsUploadFile)
    fsUploadFile.close();
//    Serial.println("Success");
  }
}

void ListPayloads(){
  FSInfo fs_info;
  SPIFFS.info(fs_info);
  String total;
  total=fs_info.totalBytes;
  String used;
  used=fs_info.usedBytes;
  String freespace;
  freespace=fs_info.totalBytes-fs_info.usedBytes;
  String FileList = "<a href=\"/\"><- BACK TO INDEX</a><br><br>";
  Dir dir = SPIFFS.openDir("/payloads");
  FileList += "File System Info Calculated in Bytes<br><b>Total:</b> "+total+" <b>Free:</b> "+freespace+" "+" <b>Used:</b> "+used+"<br><br><a href=\"/uploadpayload\">Upload Payload</a><br><br><a href=\"/livepayload\">Live Payload Mode</a><br><br><table border='1'><tr><td><b>Display Payload Contents</b></td><td><b>Size in Bytes</b></td><td><b>Run Payload</b></td><td><b>Delete Payload</b></td></tr>";
  while (dir.next()) {
    String FileName = dir.fileName();
    File f = dir.openFile("r");
    FileList += " ";
    FileList += "<tr><td><a href=\"/showpayload?payload="+FileName+"\">"+FileName+"</a></td>"+"<td>"+f.size()+"</td><td><a href=\"/dopayload?payload="+FileName+"\"><button>Run Payload</button></a></td><td><a href=\"/deletepayload?payload="+FileName+"\"><button>Delete Payload</button></td></tr>";
  }
  FileList += "</table>";
  server.send(200, "text/html", FileList);
}

void setup(void)
{
//  SOFTserial.begin(38400);
//  Serial.begin(115200);
//  Serial.println("");
//  Serial.println("ESPloit - Wifi Controlled HID Keyboard Emulator");
//  Serial.println("");
  pinMode(LED_BUILTIN, OUTPUT); 
  Serial.begin(38400);
  SPIFFS.begin();

 // loadDefaults(); //uncomment to restore default settings if double reset fails for some reason
 /*
  if (drd.detectDoubleReset()) {
    Serial.println("Double Reset Detected");
    loadDefaults();
  }
  */
  
  loadConfig();

//Set up Web Pages
  server.on("/", [](){
    FSInfo fs_info;
    SPIFFS.info(fs_info);
    String total;
    total=fs_info.totalBytes;
    String used;
    used=fs_info.usedBytes;
    String freespace;
    freespace=fs_info.totalBytes-fs_info.usedBytes;
    
    server.send(200, "text/html", "<html><body>-----<br><b>ESPloit v2.0</b> - WiFi controlled HID Keyboard Emulator<br><img width='86' height='86' src='data:image/png;base64,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'><br><i>by Corey Harding<br>www.LegacySecurityGroup.com / www.Exploit.Agency</i><br>-----<br>File System Info Calculated in Bytes<br><b>Total:</b> "+total+" <b>Free:</b> "+freespace+" "+" <b>Used:</b> "+used+"<br>-----<br><a href=\"/uploadpayload\">Upload Payload</a><br>-<br><a href=\"/listpayloads\">Choose Payload</a><br>-<br><a href=\"/livepayload\">Live Payload Mode</a><br>-<br><a href=\"/settings\">Configure ESPloit</a><br>-<br><a href=\"/format\">Format File System</a><br>-<br><a href=\"/firmware\">Upgrade ESPloit Firmware</a><br>-<br><a href=\"/help\">Help</a></body></html>");
  });

  server.on("/settings", handleSettings);

  server.on("/firmware", [](){
    server.send(200, "text/html", String()+"<html><body style=\"height: 100%;\"><a href=\"/\"><- BACK TO INDEX</a><br><br>Open Arduino IDE.<br>Pull down Sketch Menu then select Export Compiled Binary.<br>Open Sketch Folder and upload the exported BIN file.<br>You may need to manually reboot the device to reconnect.<br><iframe style =\"border: 0; height: 100%;\" src=\"http://"+local_IPstr+":1337/update\"><a href=\"http://"+local_IPstr+":1337/update\">Click here to Upload Firmware</a></iframe></body></html>");
  });

  server.on("/livepayload", [](){
    server.send(200, "text/html", String()+"<html><body style=\"height: 100%;\"><a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/listpayloads\">List Payloads</a><br><br><a href=\"/uploadpayload\">Upload Payload</a><br><br><FORM action=\"/runlivepayload\" method=\"post\" id=\"live\" target=\"iframe\">Payload: <br><textarea style =\"width: 100%;\" form=\"live\" rows=\"4\" cols=\"50\" name=\"livepayload\"></textarea><br><br><INPUT type=\"radio\" name=\"livepayloadpresent\" value=\"1\" hidden=\"1\" checked=\"checked\"><INPUT type=\"submit\" value=\"Run Payload\"></form><br><hr><br><iframe style =\"border: 0; height: 100%; width: 100%;\" src=\"http://"+local_IPstr+"/runlivepayload\" name=\"iframe\"></iframe></body></html>");
  });

  server.on("/runlivepayload", [](){
    String livepayload;
    livepayload += server.arg("livepayload");
    if (server.hasArg("livepayloadpresent")) {
      server.send(200, "text/html", "<pre>Running live payload: <br>"+livepayload+"</pre>");
      char* splitlines;
      int payloadlen = livepayload.length()+1;
      char request[payloadlen];
      livepayload.toCharArray(request,payloadlen);
      splitlines = strtok(request,"\r\n");
      int defaultdelay = DelayLength;
      int settingsdefaultdelay = DelayLength;
      int custom_delay;
      delay(livepayloaddelay);
      while(splitlines != NULL)
      {
         Serial.println(splitlines);

         char *i;
         String cmd;
         String cmdinput;
         cmd = String(strtok_r(splitlines,":",&i));

//         Serial.println(String()+"cmd:"+cmd);
//         Serial.println(String()+"cmdin:"+cmdinput);
         
         splitlines = strtok(NULL,"\r\n");
         
         if(cmd == "Rem") {
           cmdinput = String(strtok_r(NULL,":",&i));
           DelayLength = 1500;
         }
         
         else if(cmd == "DefaultDelay") {
           cmdinput = String(strtok_r(NULL,":",&i));
           DelayLength = 1500;
           String newdefaultdelay = cmdinput;
           defaultdelay = newdefaultdelay.toInt();
 //          Serial.println(String()+"default delay set to:"+defaultdelay);
         }
         else if(cmd == "CustomDelay") {
           cmdinput = String(strtok_r(NULL,":",&i));
           String customdelay = cmdinput;
           custom_delay = customdelay.toInt();
           DelayLength = custom_delay;
 //          Serial.println(String()+"Custom delay set to:"+custom_delay);
         }
 //        Serial.println(DelayLength);
         delay(DelayLength); //delay between lines in payload, I found running it slower works best
         DelayLength = defaultdelay;  
      }
      DelayLength = settingsdefaultdelay;
      return 0;
    }
    else {
      server.send(200, "text/html", "Type or Paste a payload and click \"Run Payload\".");
    }
  });

  server.on("/deletepayload", [](){
    String deletepayload;
    deletepayload += server.arg(0);
    server.send(200, "text/html", "<html><body>This will delete the payload: "+deletepayload+".<br><br>Are you sure?<br><br><a href=\"/deletepayload/yes?payload="+deletepayload+"\">YES</a> - <a href=\"/\">NO</a></body></html>");
  });

  server.on("/deletepayload/yes", [](){
    String deletepayload;
    deletepayload += server.arg(0);
    server.send(200, "text/html", "<a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/listpayloads\">List Payloads</a><br><br>Deleting payload: "+deletepayload);
    SPIFFS.remove(deletepayload);
  });

  server.on("/format", [](){
    server.send(200, "text/html", "<html><body><a href=\"/\"><- BACK TO INDEX</a><br><br>This will reformat the SPIFFS File System.<br><br>Are you sure?<br><br><a href=\"/format/yes\">YES</a> - <a href=\"/\">NO</a></body></html>");
  });
  
  server.on("/format/yes", [](){
    server.send(200, "text/html", "<a href=\"/\"><- BACK TO INDEX</a><br><br>Formatting file system: This may take up to 90 seconds");
//    Serial.print("Formatting file system...");
    SPIFFS.format();
//    Serial.println(" Success");
    saveConfig();
  });
    
  server.on("/uploadpayload", []() {
    server.send(200, "text/html", "<a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/listpayloads\">List Payloads</a><br><br><a href=\"/livepayload\">Live Payload Mode</a><br><br><b>Upload Payload:</b><br><br><form method='POST' action='/upload' enctype='multipart/form-data'><input type='file' name='upload'><input type='submit' value='Upload'></form>");
  });
    
  server.on("/listpayloads", ListPayloads);
    
  server.onFileUpload(handleFileUpload);
    
  server.on("/upload", HTTP_POST, []() {
    server.send(200, "text/html", "<a href=\"/\"><- BACK TO INDEX</a><br><br>Upload Successful!<br><br><a href=\"/listpayloads\">List Payloads</a><br><br><a href=\"/uploadpayload\">Upload Another Payload</a>");
  });

  server.on("/help", []() {
    server.send_P(200, "text/html", HelpText);
  });
  
  server.on("/license", []() {
    server.send_P(200, "text/html", License);
  });
    
  server.on("/showpayload", [](){
    webString="";
    String payload;
    payload += server.arg(0);
    File f = SPIFFS.open(payload, "r");
    String webString = f.readString();
    f.close();
    server.send(200, "text/html", "<a href=\"/\"><- BACK TO INDEX</a><br><br><a href=\"/listpayloads\">List Payloads</a><br><br><a href=\"/dopayload?payload="+payload+"\"><button>Run Payload</button></a> - <a href=\"/deletepayload?payload="+payload+"\"><button>Delete Payload</button></a><pre>"+payload+"\n-----\n"+webString+"</pre>");
    webString="";
  });

  server.on("/dopayload", [](){
    String dopayload;
    dopayload += server.arg(0);
    server.send(200, "text/html", "<a href=\"/\"><- BACK TO INDEX</a><br><br><pre>Running payload: "+dopayload+"</pre><br>This may take a while to complete...");
//    Serial.println("Running payaload: "+dopayload);
    File f = SPIFFS.open(dopayload, "r");
    int defaultdelay = DelayLength;
    int settingsdefaultdelay = DelayLength;
    int custom_delay;
    while(f.available()) {
//      SOFTserial.println(line);
//      Serial.println(line);
      String line = f.readStringUntil('\n');
      Serial.println(line);

      String fullkeys = line;
      int str_len = fullkeys.length()+1; 
      char keyarray[str_len];
      fullkeys.toCharArray(keyarray, str_len);

      char *i;
      String cmd;
      String cmdinput;
      cmd = String(strtok_r(keyarray,":",&i));

//         Serial.println(String()+"cmd:"+cmd);
//         Serial.println(String()+"cmdin:"+cmdinput);
     
      if(cmd == "Rem") {
        cmdinput = String(strtok_r(NULL,":",&i));
        DelayLength = 1500;
      }
      
      else if(cmd == "DefaultDelay") {
        cmdinput = String(strtok_r(NULL,":",&i));
        DelayLength = 1500;
        String newdefaultdelay = cmdinput;
        defaultdelay = newdefaultdelay.toInt();
//          Serial.println(String()+"default delay set to:"+defaultdelay);
      }
      else if(cmd == "CustomDelay") {
        cmdinput = String(strtok_r(NULL,":",&i));
        String customdelay = cmdinput;
        custom_delay = customdelay.toInt();
        DelayLength = custom_delay;
//          Serial.println(String()+"Custom delay set to:"+custom_delay);
      }
//        Serial.println(DelayLength);
      delay(DelayLength); //delay between lines in payload, I found running it slower works best
      DelayLength = defaultdelay;
    }
    f.close();
    DelayLength = settingsdefaultdelay;
  });
  
  server.begin();
  WiFiClient client;
  client.setNoDelay(1);

//  Serial.println("Web Server Started");

  MDNS.begin("ESPloit");

  httpUpdater.setup(&httpServer, update_path, update_username, update_password);
  httpServer.begin();

  MDNS.addService("http", "tcp", 1337);
  
  if (autopwn==1){
    runpayload();
  }
}

void loop() {
  server.handleClient();
  httpServer.handleClient();
//  drd.loop();
  while (Serial.available()) {
    String cmd = Serial.readStringUntil(':');
        if(cmd == "ResetDefaultConfig"){
          loadDefaults();
        }
  }
  //Serial.print("Free heap-");
  //Serial.println(ESP.getFreeHeap(),DEC);
}
