#ifndef SERVER_H
#define SERVER_H
#include "../spiff/spiff.h"
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>

String processor(const String &var);
void serverSetup();

#endif