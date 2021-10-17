
#include "websocket.h"
#include <WebSocketsClient.h>

WebSocketsClient websocketClient;

void sendMessage(String &msg)
{
  websocketClient.sendTXT(msg.c_str(), msg.length() + 1);
};