// LIBRARIES

#include <Arduino.h>
#include <Hash.h>

#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>

// Replace with your network credentials
const char *ssid = "VAN24";
const char *password = "vectraPawla24";

// SETTINGS
//setup
const char *wlan_ssid = "VAN24";
const char *wlan_password = "vectraPawla24";

//end of setup
// const String serial = "12345";  //(testowy)                 // musi byc zmieniony przy kazdym urzadzeniu
const String serial = "12349"; //(biurko)
// const char *ws_host = "192.168.0.21";           //adres serwera
const char *ws_host = "192.168.0.21";
const int ws_port = 9999;                        //port serwera
const char *stompUrl = "/mywebsocket/websocket"; //stompowy endpoint
//end of setup

// URL for STOMP endpoint.
// For the default config of Spring's STOMP support, the default URL is "/socketentry/websocket".
// don't forget the leading "/" !!!

int gpio_13_led = 13;
int gpio_12_relay = 12;
int gpio_12_button = 0;

// VARIABLES
int buttonState = 0;
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

// FUNCTIONS

/**
 * STOMP messages need to be NULL-terminated (i.e., \0 or \u0000).
 * However, when we send a String or a char[] array without specifying 
 * a length, the size of the message payload is derived by strlen() internally,
 * thus dropping any NULL values appended to the "msg"-String.
 * 
 * To solve this, we first convert the String to a NULL terminated char[] array
 * via "c_str" and set the length of the payload to include the NULL value.
 */
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
      // delay(1000);

      // and send a message
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
        // Serial.write("check doesnt");
      }

      // String msg = "SEND\ndestination:/device/initdevice/"+serial+"\n\n{\"user\":\"esp\",\"message\":\"Hello!\"}";
      // sendMessage(msg);
      // delay(10000);
      // do something with messages
    }

    break;
  }
  case WStype_BIN:
    Serial.printf("[WSc] get binary length: %u\n", length);
    hexdump(payload, length);

    // send data to server
    // webSocket.sendBIN(payload, length);
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

  char str[12];
  // sprintf(str, "%d", WiFi.localIP());
  Serial.println(str);
  // connect to websocket
  webSocket.begin(ws_host, ws_port, stompUrl);
  // webSocket.setExtraHeaders(); // remove "Origin: file://" header because it breaks the connection with Spring's default websocket config
  // webSocket.setExtraHeaders("foo: I am so funny\r\nbar: not"); // some headers, in case you feel funny
  webSocket.onEvent(webSocketEvent);
}

void loop()
{
  buttonState = digitalRead(gpio_12_button);
  webSocket.loop();
  pressed = false;
  if (buttonState == 0)
  {

    if (is_on == true && pressed == false)
    {
      Serial.printf(" On");
      digitalWrite(gpio_13_led, HIGH);
      digitalWrite(gpio_12_relay, LOW);
      is_on = false;
      pressed = true;
      status = "On";
      String msg = "SEND\ndestination:/device/doesntExists\n\n{\"serial\":" + serial + ",\"deviceType\":\"" + deviceType + ",\"status\":\"" + status + "\"}";
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
            String msg = "SEND\ndestination:/device/doesntExists\n\n{\"serial\":" + serial + ",\"deviceType\":\"" + deviceType + ",\"status\":\"" + status + "\"}";
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
