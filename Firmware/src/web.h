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
#include "load.h"

/** Digital to Analog Converter (DAC) implemented on hardware in R-2R configuration. */
class WebServer {

public:

  /**
   * Instantiates the Web Server.
   */
  WebServer(const uint16_t port, Load& load)
    : server(AsyncWebServer(port)), load(load) {

      const WebServer *that = this;

      // add API handlers
      this->server.on("/api/voltage", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleApiGetVoltage(request);
      });

      this->server.on("/api/current", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleApiGetCurrent(request);
      });

      this->server.on("/api/temperature", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleApiGetTemperature(request);
      });

      this->server.on("^\\/api\\/current\\/([0-9]+)$", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        this->handleApiSetCurrent(request);
      });

      this->server.onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
      });
  }

  void begin() {
    this->server.begin();
  }


private:
  AsyncWebServer server;
  Load& load;

  void handleApiGetVoltage(AsyncWebServerRequest *request) {
    uint16_t loadVoltage = this->load.getLoadVoltage();

    this->sendFormattedJsonResponse(request, "{ \"voltage\": %d }", loadVoltage);
  }

  void handleApiGetCurrent(AsyncWebServerRequest *request) {
    uint16_t loadCurrent1 = this->load.getLoadCurrent1();
    uint16_t loadCurrent2 = this->load.getLoadCurrent2();
    uint16_t totalCurrent = loadCurrent1 + loadCurrent2;

    this->sendFormattedJsonResponse(request, "{ \"current\": %d, \"current1\" : %d, \"current2\": %d }", totalCurrent, loadCurrent1, loadCurrent2);
  }

  void handleApiGetTemperature(AsyncWebServerRequest *request) {
    float temp = load.getTemperature();

    this->sendFormattedJsonResponse(request, "{ \"temperature\": %.2f }", temp);
  }

  void handleApiSetCurrent(AsyncWebServerRequest *request) {
    String valueStr = request->pathArg(0);
    uint16_t value = atoi(valueStr.c_str());

    this->load.setCurrent(value);

    this->sendFormattedJsonResponse(request, "OK");
  }

  void handleNotFound(AsyncWebServerRequest *request) {
    request->send(200, "text/plain", "Not Found");
  }

  void sendFormattedJsonResponse(AsyncWebServerRequest *request, const char* format, ...) {
    // format response string
    char buffer[128];
    va_list args; va_start(args, format);
    vsnprintf(buffer, sizeof(buffer), format, args);
    va_end(args);

    // send the formatted string
    request->send(200, "application/json", String(buffer));
  }

};

#endif