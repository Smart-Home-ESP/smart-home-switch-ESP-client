#ifndef HEADER_H
#define HEADER_H

#include <Arduino.h>
#include <ESP8266WiFi.h>
#include <WebSocketsClient.h>
#include <ArduinoJson.h>
#include <FS.h>
#include <ArduinoOTA.h>
#include <LittleFS.h>

#include "accesPoint/accesPoint.h"
#include "ota/ota.h"
// #include "spiff/spiff.h"
#include "server/httpServer.h"

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

int ws_port = 9999; //port serwera
int pole = 1;       //switch pole type
//end of setup

int gpio_led = 13;
int gpio_relay = 12;
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

#endif