/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#ifndef WEB_H
#define WEB_H

#include <Arduino.h>
#include <WiFi.h>
#include <AsyncTCP.h>
#include <ESPAsyncWebServer.h>
#include <LittleFS.h>
#include "load.h"
#include "srv.h"

/** Web / HTTP Server */
class WebServer {

public:

  /**Instantiates the Web Server. */
  WebServer(const uint16_t port, Load& load, Service &srv)
    : server(AsyncWebServer(port)), load(load), srv(srv) {

      DefaultHeaders::Instance().addHeader("Access-Control-Allow-Origin", "*");
      DefaultHeaders::Instance().addHeader("Access-Control-Allow-Methods", "GET, POST, PUT");
      DefaultHeaders::Instance().addHeader("Access-Control-Allow-Headers", "Content-Type");

      const WebServer *that = this;

      /** API Handlers **/

      // Voltage get
      this->server.on("/api/voltage", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleApiGetVoltage(request);
      });

      // Current get
      this->server.on("/api/current", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleApiGetCurrent(request);
      });

      // Temperature get
      this->server.on("/api/temperature", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleApiGetTemperature(request);
      });

      // Current set
      this->server.on("/api/current", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetCurrent(request, data, len, index, total);
      });

      // Fan Speed set
      this->server.on("/api/fan", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetFanSpeed(request, data, len, index, total);
      });

      /** Service / Test API Handler **/

      // DAC set
      this->server.on("/api/srv/dac/set", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSrvDacSet(request, data, len, index, total);
      });

      // DAC swipe
      this->server.on("/api/srv/dac/swipe", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiSrvDacSwipe(request);
      });

      // OTA restart
      this->server.on("/api/srv/ota/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiSrvOtaRestart(request);
      });

      // Restart (non-OTA)
      this->server.on("/api/srv/restart", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiSrvRestart(request);
      });

      /** CORS Handler */
      this->server.on("*", HTTP_OPTIONS, [this](AsyncWebServerRequest *request) {
        this->handleCors(request);
      });

      /** Static Content */
      this->server.on("/", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleGetPreloadedStaticFile(request, "text/html", this->contentIndexHtml);
      });

      this->server.on("/style.css", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleGetPreloadedStaticFile(request, "text/css", this->contentStyleCss);
      });

      this->server.on("/load.js", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleGetPreloadedStaticFile(request, "text/javascript", this->contentLoadJs);
      });

      // note: normal static file service does not works because of the continuous ADC
      // this->server.serveStatic("/", LittleFS, "/").setDefaultFile("index.html");

      /** Misc Handlers  */

      // 404 Not Found
      this->server.onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
      });
  }

  /** Start the Web Server */
  void begin() {
    // pre-load static files (as normal static file service does not works because of the continuous ADC)
    this->readStaticFile("/index.html", this->contentIndexHtml, 4096);
    this->readStaticFile("/style.css", this->contentStyleCss, 4096);
    this->readStaticFile("/load.js", this->contentLoadJs, 8192);

    // start the web server
    this->server.begin();
  }

private:
  AsyncWebServer server;
  Load& load;
  Service& srv;

  char contentIndexHtml[4096];
  char contentStyleCss[4096];
  char contentLoadJs[8192];

  /** Handle Voltage get request. */
  void handleApiGetVoltage(AsyncWebServerRequest *request) {
    float loadVoltage = this->load.getLoadVoltage();

    this->sendFormattedJsonResponse(request, "{ \"voltage\": %.3f }", loadVoltage);
  }

  /** Handle Current get request. */
  void handleApiGetCurrent(AsyncWebServerRequest *request) {
    float loadCurrent1 = this->load.getLoadCurrent1();
    float loadCurrent2 = this->load.getLoadCurrent2();
    float totalCurrent = loadCurrent1 + loadCurrent2;

    this->sendFormattedJsonResponse(request, "{ \"current\": %.3f, \"current1\" : %.3f, \"current2\": %.3f }", totalCurrent, loadCurrent1, loadCurrent2);
  }

  /** Handle Temperature get request. */
  void handleApiGetTemperature(AsyncWebServerRequest *request) {
    float temp = load.getTemperature();

    this->sendFormattedJsonResponse(request, "{ \"temperature\": %.2f }", temp);
  }

  /** Handle Current set request. */
  void handleApiSetCurrent(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float current = atof(valueStr.c_str());

    this->load.setCurrent(current);

    this->sendFormattedJsonResponse(request, "OK");
  }

  /** Handle Fan Speed set request. */
  void handleApiSetFanSpeed(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float value = atof(valueStr.c_str());

    this->load.setFanSpeed(value);

    this->sendFormattedJsonResponse(request, "OK");
  }

  /** Handle DAC set request (service/test). */
  void handleApiSrvDacSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    uint16_t value = atoi(valueStr.c_str());

    this->srv.dacSet(value);
  }

  /** Handle DAC swipe request (service/test). */
  void handleApiSrvDacSwipe(AsyncWebServerRequest *request) {
    this->srv.dacSwipe();
  }

  /** Handle OTA restart (service/test). */
  void handleApiSrvOtaRestart(AsyncWebServerRequest *request) {
    // send response
    request->send(200, "text/plain", "OK");

    // set prog mode
    this->srv.progModeRestart();
  }

  /** Handle OTA restart (service/test). */
  void handleApiSrvRestart(AsyncWebServerRequest *request) {
    // send response
    request->send(200, "text/plain", "OK");

    // restart
    this->srv.restart();
  }

  /** Handle pre-loaded static file request. */
  void handleGetPreloadedStaticFile(AsyncWebServerRequest *request, const char* contentType, char* content) {
    request->send(200, contentType, content);
  }

  /** Handle CORS */
  void handleCors(AsyncWebServerRequest *request) {
    AsyncWebServerResponse *response = request->beginResponse(200, "", "");
    // note: cors headers already set
    request->send(response);
  }
  /** Send 404 Not Found response. */
  void handleNotFound(AsyncWebServerRequest *request) {
    request->send(404, "text/plain", "Not Found");
  }

  /** Send formatted json response. */
  void sendFormattedJsonResponse(AsyncWebServerRequest *request, const char* format, ...) {
    // format response string
    char buffer[128];
    va_list args; va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // send the formatted string
    request->send(200, "application/json", String(buffer));
  }

  /** Read body from request. */
  String readBody(uint8_t *data, size_t len, size_t index, size_t total) {
    if ((index != 0) || (len >= 255) || (len != total)) {
      // limit body length
      return "";
    }

    String res;
    for (int idx = 0; idx < len; ++idx) {
      res += (char) data[idx];
    }
    return res;
  }

  /** Read (pre-load) a static file from LittleFS.  */
  void readStaticFile(const char *path, char *buffer, size_t maxLength) {
    File file = LittleFS.open(path);
    size_t bytes = file.readBytes(buffer, maxLength - 1);
    Serial.printf("Read file %s size=%d\n", path, bytes);
    buffer[bytes] = (char) 0;
  }

};

#endif