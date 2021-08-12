#include <Arduino.h>
#include <Hash.h>

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <ESP8266WiFi.h>

// Replace with your network credentials
const char *ssid = "van24";
const char *password = "ruterPawla24";
const String serial = "72340"; //(biurko)
String ws_host = "192.168.1.214";
const int ws_port = 9999;                        //port serwera
const char *stompUrl = "/mywebsocket/websocket"; //stompowy endpoint
//end of setup

int gpio_13_led = 13;
int gpio_12_relay = 12;
int gpio_12_button = 0;
int gpio_s2_button = 4;

// VARIABLES
int buttonState = 0;
int buttonState2 = 0;
const char *status;
const char *task;
boolean is_on = false;
boolean is_on_previous = false;
const char *deviceType = "led_rgb";
boolean pressed = false;
//end of variables

StaticJsonDocument<256> doc;
WebSocketsClient webSocket;

// FUNCTIONS
void prepareLedUpdate(StaticJsonDocument<256> doc);
void initialSetup(StaticJsonDocument<256> doc);
// END OF FUNCTIONS

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
  //  Init
  pinMode(gpio_13_led, OUTPUT);
  digitalWrite(gpio_13_led, HIGH);

  pinMode(gpio_12_relay, OUTPUT);
  digitalWrite(gpio_12_relay, HIGH);

  Serial.begin(115200);
  delay(5000);

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

void loop()
{
  buttonState = digitalRead(gpio_12_button);
  buttonState2 = digitalRead(gpio_s2_button);
  webSocket.loop();
  pressed = false;
  if (buttonState == 0 || buttonState2 == 0)
  {

    if (is_on == true && pressed == false)
    {
      Serial.printf(" On");
      digitalWrite(gpio_13_led, HIGH);
      digitalWrite(gpio_12_relay, LOW);
      is_on = false;
      pressed = true;
      status = "On";
      String msg = "SEND\ndestination:/device/changeDeviceStatus/" + serial + "\n\n{\"status\":" + status + "\"}";
      sendMessage(msg);
      delay(500);
    }

    if (is_on == false && pressed == false)
    {
      Serial.printf(" Off");
      digitalWrite(gpio_13_led, LOW);
      digitalWrite(gpio_12_relay, HIGH);
      is_on = true;
      pressed = true;
      status = "Off";
      String msg = "SEND\ndestination:/device/changeDeviceStatus/" + serial + "\n\n{\"status\":" + status + "\"}";
      sendMessage(msg);
      delay(500);
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
