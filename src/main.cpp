#include <Arduino.h>
#include <Hash.h>

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>
#include <ESPAsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <EEPROM.h>
#include <Hash.h>
#include <FS.h>


AsyncWebServer server(80);

const char* PARAM_INPUT_1 = "input1";
const char* PARAM_INPUT_2 = "input2";
const char* PARAM_INPUT_3 = "input3";
const char* PARAM_INPUT_4 = "input4";
const char* PARAM_INPUT_5 = "input5";
const char* PARAM_INPUT_RESTART = "restart";

void notFound(AsyncWebServerRequest *request) {
  request->send(404, "text/plain", "Not found");
}

String readFile(fs::FS &fs, const char * path){
  Serial.printf("Reading file: %s\r\n", path);
  File file = fs.open(path, "r");
  if(!file || file.isDirectory()){
    Serial.println("- empty file or failed to open file");
    return String();
  }
  Serial.println("- read from file:");
  String fileContent;
  while(file.available()){
    fileContent+=String((char)file.read());
  }
  Serial.println(fileContent);
  return fileContent;
}

void writeFile(fs::FS &fs, const char * path, const char * message){
  Serial.printf("Writing file: %s\r\n", path);
  File file = fs.open(path, "w");
  if(!file){
    Serial.println("- failed to open file for writing");
    return;
  }
  if(file.print(message)){
    Serial.println("- file written");
  } else {
    Serial.println("- write failed");
  }
}

String processor(const String& var){
  //Serial.println(var);
  if(var == "input1"){
    return readFile(SPIFFS, "/inputString.txt");
  }
  else if(var == "inputInt"){
    return readFile(SPIFFS, "/inputInt.txt");
  }
  else if(var == "inputFloat"){
    return readFile(SPIFFS, "/inputFloat.txt");
  }
  return String();
}

// Replace with your network credentials
String ssid;
String password;
String serial; 
String ws_host;
String stompUrl; //stompowy endpoint
//file names
const char *ssid_file = "/ssid_file.txt";
const char *password_file = "/password_file.txt";
const char *serial_file = "/serial_file.txt"; //(biurko)
const char *ws_host_file = "/ws_host_file.txt";
const char *stompUrl_file = "/stompUrl.txt";
int ws_port = 9999;                        //port serwera
int pole = 1; //switch pole type
//end of setup

int gpio_13_led = 13;
int gpio_12_relay = 12;
int gpio_12_button = 0; 
int gpio_s2_button = 0; //4

// VARIABLES
float time_ap = 1;
int buttonState = 0;
int buttonState2 = 0;
int lastState = 1;
boolean currentStateFlag = true;
const char *status;
const char *task;
boolean is_on = false;
boolean is_on_previous = false;
const char *deviceType = "switch";
boolean pressed = false;
//end of variables

StaticJsonDocument<256> doc;
WebSocketsClient webSocket;

// FUNCTIONS
void prepareLedUpdate(StaticJsonDocument<256> doc);
void initialSetup(StaticJsonDocument<256> doc);
void ApSetup();
void spiffsSetup();
void serverSetup();
// END OF FUNCTIONS

// HTML web page to handle 3 input fields (input1, input2, input3)
const char index_html[] PROGMEM = R"rawliteral(
<!DOCTYPE HTML><html><head>
  <title>ESP Input Form</title>
  <meta name="viewport" content="width=device-width, initial-scale=1">
  </head><body>
<form action="/get" target="hidden-form">
  ssid (current value %ssid%): <input type="text" name="input1">
  <input type="submit" value="Submit" onclick="submitMessage()">
</form><br>
<form action="/get" target="hidden-form">
  password (current value %password%): <input type="text" name="input2">
  <input type="submit" value="Submit" onclick="submitMessage()">
</form><br>
<form action="/get" target="hidden-form">
  serial (current value %serial%): <input type="text" name="input3">
  <input type="submit" value="Submit" onclick="submitMessage()">
</form><br>
<form action="/get" target="hidden-form">
  ws_host (current value %ws_host%): <input type="text" name="input4">
  <input type="submit" value="Submit" onclick="submitMessage()">
</form><br>
<form action="/get" target="hidden-form">
  stomp url (current value %stompUrl%): <input type="text" name="input5">
  <input type="submit" value="Submit" onclick="submitMessage()">
</form><br>
<form action="/get" target="hidden-form">
  <input type="text" name="restart">
  <input type="submit" value="Restart" onclick="submitMessage()">
</form><br>
</body>
<script>
  function submitMessage() {
    alert("Saved value to ESP SPIFFS");
    setTimeout(function(){ document.location.reload(false); }, 500);
  }
</script>
</html>)rawliteral";

void sendMessage(String &msg)
{
  webSocket.sendTXT(msg.c_str(), msg.length() + 1);
}

void webSocketEvent(WStype_t type, uint8_t *payload, size_t length)
{

  switch (type)
  {
  case WStype_DISCONNECTED:
    Serial.printf("[WSc] Disconnected!\n");
    break;
  case WStype_CONNECTED:
  {
    Serial.printf("[WSc] Connected to url: %s\n", payload);

    String msg = "CONNECT\r\naccept-version:1.1,1.0\r\nheart-beat:1000,1000\r\n\r\n";
    sendMessage(msg);
  }
  break;
  case WStype_TEXT:
  {
    // #####################
    // handle STOMP protocol
    // #####################

    String text = (char *)payload;
    Serial.printf("[WSc] get text: %s\n", payload);

    if (text.startsWith("CONNECTED"))
    {

      // subscribe to some channels

      String msg = "SUBSCRIBE\nid:sub-0\ndestination:/device/device/" + serial + "\n\n";
      sendMessage(msg);
    }
    else
    {
      int startPosition = text.indexOf("{");
      deserializeJson(doc, text.substring(startPosition - 1));

      if (doc.containsKey("serial"))
      {
        initialSetup(doc);
      }
      else if (doc.containsKey("response"))
      {
        String msg = "SEND\ndestination:/device/doesntExists\n\n{\"serial\":" + serial + ",\"deviceType\":\"" + deviceType + "\"}";
        sendMessage(msg);
      }
      else
      {
        prepareLedUpdate(doc);
      }
    }

    break;
  }
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    hexdump(payload, length);
    break;
  }
}

void setup()
{
  Serial.begin(115200);
  delay(5000);
  spiffsSetup();
  serverSetup();
  //  Init
  pinMode(gpio_13_led, OUTPUT);
  digitalWrite(gpio_13_led, HIGH);
  pressed = false;

  pinMode(gpio_12_relay, OUTPUT);
  digitalWrite(gpio_12_relay, LOW);


  WiFi.begin(ssid, password);
  Serial.println("Connecting to wifi..");



  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(gpio_13_led, LOW);
    delay(500);
    Serial.print(".");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.status());
    digitalWrite(gpio_13_led, HIGH);
    delay(500);
    time_ap = time_ap +1;
    if(time_ap == 60){
      Serial.println("AP START");
        ApSetup();
        
    }
  }

  Serial.println("");
  Serial.print("Connected to ");
  Serial.println(ssid);
  Serial.print("IP address: ");
  Serial.println(WiFi.localIP());
  Serial.println(ws_host);

  // connect to websocket
  webSocket.begin(ws_host, ws_port, stompUrl);
  webSocket.onEvent(webSocketEvent);


}

void spiffsSetup() {
  SPIFFS.begin();
  if(readFile(SPIFFS, ssid_file) == NULL) {
      writeFile(SPIFFS, ssid_file, "wifi");
  } else {
      ssid = readFile(SPIFFS, ssid_file);
  }
    if(readFile(SPIFFS, password_file) == NULL) {
      writeFile(SPIFFS, password_file, "0987654321");
  } else {
      password = readFile(SPIFFS, password_file);
  }
    if(readFile(SPIFFS, serial_file) == NULL) {
      writeFile(SPIFFS, serial_file, "55551");
  } else {
      serial = readFile(SPIFFS, serial_file);
  }
    if(readFile(SPIFFS, ws_host_file) == NULL) {
      writeFile(SPIFFS, ws_host_file, "192.168.1.214");
  } else {
      ws_host = readFile(SPIFFS, ws_host_file);
  }
    if(readFile(SPIFFS, stompUrl_file) == NULL) {
      writeFile(SPIFFS, stompUrl_file, "/mywebsocket/websocket");
  } else {
      stompUrl = readFile(SPIFFS, stompUrl_file);
  }


  Serial.println(ssid);
}

void ApSetup() {
      Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP("mini2_AP", "password");
  if(result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }
}

void serverSetup(){
      // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request){
    request->send_P(200, "text/html", index_html, processor);
  });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [] (AsyncWebServerRequest *request) {
    String inputMessage;
    String inputParam;
    // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
    if (request->hasParam(PARAM_INPUT_1)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      writeFile(SPIFFS, ssid_file, inputMessage.c_str());
    }
    // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_2)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      writeFile(SPIFFS, password_file, inputMessage.c_str());
    }
    // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
    else if (request->hasParam(PARAM_INPUT_3)) {
      inputMessage = request->getParam(PARAM_INPUT_1)->value();
      writeFile(SPIFFS, serial_file, inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_INPUT_4)) {
      inputMessage = request->getParam(PARAM_INPUT_4)->value();
      writeFile(SPIFFS, ws_host_file, inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_INPUT_5)) {
      inputMessage = request->getParam(PARAM_INPUT_5)->value();
      writeFile(SPIFFS, stompUrl_file, inputMessage.c_str());
    }
    else if (request->hasParam(PARAM_INPUT_RESTART)) {
        ESP.restart();
    }
    else {
      inputMessage = "No message sent";
      inputParam = "none";
    }

    Serial.println(inputMessage);
    request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" 
                                     + inputParam + ") with value: " + inputMessage +
                                     "<br><a href=\"/\">Return to Home Page</a>");
  });
  server.onNotFound(notFound);
  server.begin();
}

void loop()
{
  buttonState2 = digitalRead(gpio_s2_button);
  webSocket.loop();
  if (pole == 1) {                   
    if (buttonState2 == 0){
      if (is_on == true && pressed == false)
      {
        Serial.printf(" On");
        digitalWrite(gpio_13_led, HIGH);
        digitalWrite(gpio_12_relay, LOW);
        is_on = false;
        pressed = true;
        status = "On";
        String msg = "SEND\ndestination:/device/changeDeviceStatus/" + serial + "\n\n{\"deviceStatus\":\"" + status + "\"}";
        sendMessage(msg);
        delay(200);
      }

      if (is_on == false && pressed == false)
      {
        Serial.printf(" Off");
        digitalWrite(gpio_13_led, LOW);
        digitalWrite(gpio_12_relay, HIGH);
        is_on = true;
        pressed = true;
        status = "Off";
        String msg = "SEND\ndestination:/device/changeDeviceStatus/" + serial + "\n\n{\"deviceStatus\":\"" + status + "\"}";
        sendMessage(msg);
        delay(200);
      }
  }

    if (buttonState2 == 1 && pressed== true) {
        pressed = false;
              delay(100);
    }
  } 
  //pole 2
  if (pole == 2) {
    delay(200);

    if (lastState == buttonState2 && buttonState2 == 1){
          Serial.println("first");
      lastState = 0;
      pressed = false;
    }
    if (lastState == buttonState2 && buttonState2 == 0){
                    Serial.println("second");
      lastState = 1;
      pressed = false;
    }

        if (is_on == true && pressed == false)
        {
          Serial.printf(" On");
          digitalWrite(gpio_13_led, HIGH);
          digitalWrite(gpio_12_relay, LOW);
          is_on = false;
          pressed = true;
          status = "On";
          String msg = "SEND\ndestination:/device/changeDeviceStatus/" + serial + "\n\n{\"deviceStatus\":\"" + status + "\"}";
          sendMessage(msg);
          delay(200);
        }

        if (is_on == false && pressed == false)
        {
          Serial.printf(" Off");
          digitalWrite(gpio_13_led, LOW);
          digitalWrite(gpio_12_relay, HIGH);
          is_on = true;
          pressed = true;
          status = "Off";
          String msg = "SEND\ndestination:/device/changeDeviceStatus/" + serial + "\n\n{\"deviceStatus\":\"" + status + "\"}";
          sendMessage(msg);
          delay(200);
        }
  }
}

void initialSetup(StaticJsonDocument<256> doc)
{
  status = doc["deviceStatus"];
  if (0 == strcmp(status, "On"))
  {
    Serial.printf(" On");
    is_on = true;
    digitalWrite(gpio_13_led, LOW);
    digitalWrite(gpio_12_relay, HIGH);
  }
  else if (0 == strcmp(status, "Off"))
  {
    Serial.printf(" Off");
    is_on = false;
    digitalWrite(gpio_13_led, HIGH);
    digitalWrite(gpio_12_relay, LOW);
  }
}

void prepareLedUpdate(StaticJsonDocument<256> doc)
{
  status = doc["status"];
  Serial.write(status);

  if (0 == strcmp(status, "On"))
  {
    is_on = true;
    digitalWrite(gpio_13_led, LOW);
    digitalWrite(gpio_12_relay, HIGH);
  }

  if (0 == strcmp(status, "Off"))
  {
    is_on = false;
    digitalWrite(gpio_13_led, HIGH);
    digitalWrite(gpio_12_relay, LOW);
  }
}
