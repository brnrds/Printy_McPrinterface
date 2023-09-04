#include <time.h>
#include <WiFi.h>
#include <ESPAsyncWebServer.h>
#include <AsyncWebSocket.h>
#include "SPIFFS.h"
#include <ArduinoJson.h>

const char *ssid = "NOS-8146";
const char *password = "W36FPH29";

AsyncWebServer server(80);
AsyncWebSocket ws("/ws");

String formatTime(time_t timestamp) {
  struct tm timeinfo;
  gmtime_r(&timestamp, &timeinfo);
  char buffer[64];
  strftime(buffer, sizeof(buffer), "%Y-%m-%d %H:%M:%S", &timeinfo);
  return String(buffer);
}

String getSPIFFSDirectory() {
  DynamicJsonDocument doc(1024);
  JsonArray filesArray = doc.createNestedArray("files");

  File root = SPIFFS.open("/");
  File file = root.openNextFile();

  while (file) {
    JsonObject fileObject = filesArray.createNestedObject();
    fileObject["filename"] = file.name();
    fileObject["size"] = file.size();
    fileObject["isDirectory"] = file.isDirectory();
    fileObject["modified"] = formatTime(file.getLastWrite());

    file = root.openNextFile();
  }

  String jsonString;
  serializeJson(doc, jsonString);
  return jsonString;
}

void parseAndPrintJson(const uint8_t *data, size_t len) {
  Serial.println("heyo");
  DynamicJsonDocument jsonDocument(1024);
  DeserializationError error = deserializeJson(jsonDocument, data, len);

  if (error) {
    Serial.print("JSON parsing error: ");
    Serial.println(error.c_str());
    return;
  }

  String text = jsonDocument["text"].as<String>();
  bool bold = jsonDocument["bold"].as<bool>();
  bool doubleHeight = jsonDocument["doubleHeight"].as<bool>();
  bool doubleWidth = jsonDocument["doubleWidth"].as<bool>();
  String justify = jsonDocument["justify"].as<String>();
  String font = jsonDocument["font"].as<String>();
  int charSpacing = jsonDocument["charSpacing"].as<int>();

  // Print the parsed JSON values to the Serial monitor
  Serial.println("Parsed JSON:");
  Serial.println("Text: " + text);
  Serial.println("Bold: " + String(bold));
  Serial.println("Double Height: " + String(doubleHeight));
  Serial.println("Double Width: " + String(doubleWidth));
  Serial.println("Justify: " + justify);
  Serial.println("Font: " + font);
  Serial.println("Character Spacing: " + String(charSpacing));
}

void setup() {
  Serial.begin(115200);
  WiFi.begin(ssid, password);
  while (WiFi.status() != WL_CONNECTED) {
    delay(1000);
  }
  Serial.print("IP Address: ");
  Serial.println(WiFi.localIP());

  server.on("/", HTTP_GET, [](AsyncWebServerRequest *request) {
    request->send(SPIFFS, "/index.html", "text/html");
  });

  ws.onEvent([](AsyncWebSocket *server, AsyncWebSocketClient *client, AwsEventType type, void *arg, uint8_t *data, size_t len) {
    if (type == WS_EVT_CONNECT) {
      Serial.println("WebSocket client connected");
    } else if (type == WS_EVT_DISCONNECT) {
      Serial.println("WebSocket client disconnected");
    } else if (type == WS_EVT_DATA) {
      Serial.println("WebSocket message received");

      DynamicJsonDocument jsonDocument(1024);
      DeserializationError error = deserializeJson(jsonDocument, data, len);

      if (error) {
        Serial.print("JSON parsing error: ");
        Serial.println(error.c_str());
        return;
      }

      JsonObject jsonObject = jsonDocument.as<JsonObject>();

      if (jsonObject.containsKey("listFiles") && jsonObject["listFiles"].as<bool>() == true) {
        String fileList = getSPIFFSDirectory();
        client->text(fileList);
      }

      if (jsonObject.containsKey("Text")) {
        parseAndPrintJson(data, len);
      }


            

    }
  });

  server.addHandler(&ws);

  if (!SPIFFS.begin(true)) {
    Serial.println("An error occurred while mounting SPIFFS");
    return;
  }

  server.begin();
}

void loop() {}
