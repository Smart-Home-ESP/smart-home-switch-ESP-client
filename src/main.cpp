#include "headers.h"

// FUNCTIONS
void prepareLedUpdate(StaticJsonDocument<256> doc);
void initialSetup(StaticJsonDocument<256> doc);
void spiffsSetup();
void webSocketEvent(WStype_t type, uint8_t *payload, size_t length);
void sendMessage(String &msg);
// END OF FUNCTIONS

void setup()
{
  Serial.begin(115200);
  delay(5000);
  spiffsSetup();
  serverSetup();
  otaSetup();
  //  Init
  pinMode(gpio_led, OUTPUT);
  digitalWrite(gpio_led, HIGH);
  pressed = false;

  pinMode(gpio_relay, OUTPUT);
  digitalWrite(gpio_relay, LOW);

  WiFi.begin(ssid, password);
  Serial.println("Connecting to wifi..");

  // Wait for connection
  while (WiFi.status() != WL_CONNECTED)
  {
    digitalWrite(gpio_led, LOW);
    delay(400);
    Serial.print(".");
    Serial.println(WiFi.localIP());
    Serial.println(WiFi.status());
    digitalWrite(gpio_led, HIGH);
    delay(500);
    time_ap = time_ap + 1;
    if (time_ap == 60)
    {
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

void loop()
{
  ArduinoOTA.handle();
  buttonState2 = digitalRead(gpio_s2_button);
  webSocket.loop();
  if (pole == 1)
  {
    if (buttonState2 == 0)
    {
      if (is_on == true && pressed == false)
      {
        Serial.printf(" On");
        digitalWrite(gpio_led, HIGH);
        digitalWrite(gpio_relay, LOW);
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
        digitalWrite(gpio_led, LOW);
        digitalWrite(gpio_relay, HIGH);
        is_on = true;
        pressed = true;
        status = "Off";
        String msg = "SEND\ndestination:/device/changeDeviceStatus/" + serial + "\n\n{\"deviceStatus\":\"" + status + "\"}";
        sendMessage(msg);
        delay(200);
      }
    }

    if (buttonState2 == 1 && pressed == true)
    {
      pressed = false;
      delay(100);
    }
  }
  //pole 2
  if (pole == 2)
  {
    delay(200);

    if (lastState == buttonState2 && buttonState2 == 1)
    {
      Serial.println("first");
      lastState = 0;
      pressed = false;
    }
    if (lastState == buttonState2 && buttonState2 == 0)
    {
      Serial.println("second");
      lastState = 1;
      pressed = false;
    }

    if (is_on == true && pressed == false)
    {
      Serial.printf(" On");
      digitalWrite(gpio_led, HIGH);
      digitalWrite(gpio_relay, LOW);
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
      digitalWrite(gpio_led, LOW);
      digitalWrite(gpio_relay, HIGH);
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
    digitalWrite(gpio_led, LOW);
    digitalWrite(gpio_relay, HIGH);
  }
  else if (0 == strcmp(status, "Off"))
  {
    Serial.printf(" Off");
    is_on = false;
    digitalWrite(gpio_led, HIGH);
    digitalWrite(gpio_relay, LOW);
  }
}

void prepareLedUpdate(StaticJsonDocument<256> doc)
{
  status = doc["status"];
  Serial.write(status);

  if (0 == strcmp(status, "On"))
  {
    is_on = true;
    digitalWrite(gpio_led, LOW);
    digitalWrite(gpio_relay, HIGH);
  }

  if (0 == strcmp(status, "Off"))
  {
    is_on = false;
    digitalWrite(gpio_led, HIGH);
    digitalWrite(gpio_relay, LOW);
  }
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

void sendMessage(String &msg)
{
  webSocket.sendTXT(msg.c_str(), msg.length() + 1);
};

void spiffsSetup()
{

  LittleFS.begin();
  Serial.println("Start LitteFS setup");
  if (readFile(LittleFS, ssid_file) == NULL)
  {
    writeFile(LittleFS, ssid_file, "wifi");
    ssid = readFile(LittleFS, ssid_file);
  }
  else
  {
    ssid = readFile(LittleFS, ssid_file);
  }
  if (readFile(LittleFS, password_file) == NULL)
  {
    writeFile(LittleFS, password_file, "0987654321");
    password = readFile(LittleFS, password_file);
  }
  else
  {
    password = readFile(LittleFS, password_file);
  }
  if (readFile(LittleFS, serial_file) == NULL)
  {
    writeFile(LittleFS, serial_file, "55551");
    serial = readFile(LittleFS, serial_file);
  }
  else
  {
    serial = readFile(LittleFS, serial_file);
  }
  if (readFile(LittleFS, ws_host_file) == NULL)
  {
    writeFile(LittleFS, ws_host_file, "192.168.1.214");
    ws_host = readFile(LittleFS, ws_host_file);
  }
  else
  {
    ws_host = readFile(LittleFS, ws_host_file);
  }
  if (readFile(LittleFS, stompUrl_file) == NULL)
  {
    writeFile(LittleFS, stompUrl_file, "/mywebsocket/websocket");
    stompUrl = readFile(LittleFS, stompUrl_file);
  }
  else
  {
    stompUrl = readFile(LittleFS, stompUrl_file);
  }

  Serial.println("End LitteFS setup");
}
