#include "httpServer.h"


AsyncWebServer server(80);

const char *PARAM_INPUT_1 = "input1";
const char *PARAM_INPUT_2 = "input2";
const char *PARAM_INPUT_3 = "input3";
const char *PARAM_INPUT_4 = "input4";
const char *PARAM_INPUT_5 = "input5";
const char *PARAM_INPUT_RESTART = "restart";

const char *ssid_file_path = "/ssid_file.txt";
const char *password_file_path = "/password_file.txt";
const char *serial_file_path = "/serial_file.txt";
const char *ws_host_file_path = "/ws_host_file.txt";
const char *stompUrl_file_path = "/stompUrl.txt";

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

void notFound(AsyncWebServerRequest *request)
{
  request->send(404, "text/plain", "Not found");
}

String processor(const String &var)
{
  //Serial.println(var);
  if (var == "input1")
  {
    return readFile(LittleFS, "/inputString.txt");
  }
  else if (var == "inputInt")
  {
    return readFile(LittleFS, "/inputInt.txt");
  }
  else if (var == "inputFloat")
  {
    return readFile(LittleFS, "/inputFloat.txt");
  }
  return String();
}

void serverSetup()
{
  // Send web page with input fields to client
  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request)
            { request->send_P(200, "text/html", index_html, processor); });

  // Send a GET request to <ESP_IP>/get?input1=<inputMessage>
  server.on("/get", HTTP_GET, [](AsyncWebServerRequest *request)
            {
              String inputMessage;
              String inputParam;
              // GET input1 value on <ESP_IP>/get?input1=<inputMessage>
              if (request->hasParam(PARAM_INPUT_1))
              {
                inputMessage = request->getParam(PARAM_INPUT_1)->value();
                writeFile(LittleFS, ssid_file_path, inputMessage.c_str());
              }
              // GET input2 value on <ESP_IP>/get?input2=<inputMessage>
              else if (request->hasParam(PARAM_INPUT_2))
              {
                inputMessage = request->getParam(PARAM_INPUT_2)->value();
                writeFile(LittleFS, password_file_path, inputMessage.c_str());
              }
              // GET input3 value on <ESP_IP>/get?input3=<inputMessage>
              else if (request->hasParam(PARAM_INPUT_3))
              {
                inputMessage = request->getParam(PARAM_INPUT_3)->value();
                writeFile(LittleFS, serial_file_path, inputMessage.c_str());
              }
              else if (request->hasParam(PARAM_INPUT_4))
              {
                inputMessage = request->getParam(PARAM_INPUT_4)->value();
                writeFile(LittleFS, ws_host_file_path, inputMessage.c_str());
              }
              else if (request->hasParam(PARAM_INPUT_5))
              {
                inputMessage = request->getParam(PARAM_INPUT_5)->value();
                writeFile(LittleFS, stompUrl_file_path, inputMessage.c_str());
              }
              else if (request->hasParam(PARAM_INPUT_RESTART))
              {
                ESP.restart();
              }
              else
              {
                inputMessage = "No message sent";
                inputParam = "none";
              }

              Serial.println(inputMessage);
              request->send(200, "text/html", "HTTP GET request sent to your ESP on input field (" + inputParam + ") with value: " + inputMessage + "<br><a href=\"/\">Return to Home Page</a>");
            });
  server.onNotFound(notFound);
  server.begin();
}