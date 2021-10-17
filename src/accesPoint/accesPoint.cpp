#include "accesPoint.h"
#include <ESP8266WiFi.h>

const char *AP_NAME = "CHANGEME";
const char *AP_PASSWORD = "CHANGEME";

void ApSetup()
{
  Serial.print("Setting soft-AP ... ");
  boolean result = WiFi.softAP(AP_NAME, AP_PASSWORD);
  if (result == true)
  {
    Serial.println("Ready");
  }
  else
  {
    Serial.println("Failed!");
  }
}