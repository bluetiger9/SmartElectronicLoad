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
#include "srv.h"

/** Web / HTTP Server */
class WebServer {

public:

  /**Instantiates the Web Server. */
  WebServer(const uint16_t port, Load& load, Service &srv)
    : server(AsyncWebServer(port)), load(load), srv(srv) {

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
      this->server.on("^\\/api\\/current\\/([0-9]+[.][0-9]+)$", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        this->handleApiSetCurrent(request);
      });

      // Fan Speed set
      this->server.on("^\\/api\\/fan\\/([0-9]+[.][0-9]+)$", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        this->handleApiSetFanSpeed(request);
      });

      /** Service / Test API Handler **/

      // DAC set
      this->server.on("^\\/api\\/srv\\/dac\\/set/([0-9]+)$", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        this->handleApiSrvDacSet(request);
      });

      // DAC swipe
      this->server.on("^\\/api\\/srv\\/dac\\/swipe$", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiSrvDacSwipe(request);
      });


      /** Misc Handlers  */

      // 404 Not Found
      this->server.onNotFound([this](AsyncWebServerRequest *request) {
        this->handleNotFound(request);
      });
  }

  /** Start the Web Server */
  void begin() {
    this->server.begin();
  }


private:
  AsyncWebServer server;
  Load& load;
  Service& srv;

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
  void handleApiSetCurrent(AsyncWebServerRequest *request) {
    String valueStr = request->pathArg(0);
    float current = atof(valueStr.c_str());

    this->load.setCurrent(current);

    this->sendFormattedJsonResponse(request, "OK");
  }

  /** Handle Fan Speed set request. */
  void handleApiSetFanSpeed(AsyncWebServerRequest *request) {
    String valueStr = request->pathArg(0);
    float value = atof(valueStr.c_str());

    this->load.setFanSpeed(value);

    this->sendFormattedJsonResponse(request, "OK");
  }

  /** Handle DAC set request (service/test). */
  void handleApiSrvDacSet(AsyncWebServerRequest *request) {
    String valueStr = request->pathArg(0);
    uint16_t value = atoi(valueStr.c_str());

    this->srv.dacSet(value);
  }

  /** Handle DAC swipe request (service/test). */
  void handleApiSrvDacSwipe(AsyncWebServerRequest *request) {
    this->srv.dacSwipe();
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

};

#endif