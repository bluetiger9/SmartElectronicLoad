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
#include "shaper.h"
#include "srv.h"

/** Web / HTTP Server */
class WebServer {

public:

  /**Instantiates the Web Server. */
  WebServer(const uint16_t port, Load& load, Shaper &shaper, Service &srv)
    : server(AsyncWebServer(port)), load(load), shaper(shaper), srv(srv) {

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

      // Power set
      this->server.on("/api/power", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetPower(request, data, len, index, total);
      });

      // Resistance set
      this->server.on("/api/resistance", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetResistance(request, data, len, index, total);
      });

      // Fan Speed set
      this->server.on("/api/fan", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetFanSpeed(request, data, len, index, total);
      });

      // Load enable
      this->server.on("/api/enable", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        this->handleApiLoadEnable(request, true);
      });

      // Load disable
      this->server.on("/api/disable", HTTP_PUT, [this](AsyncWebServerRequest *request) {
        this->handleApiLoadEnable(request, false);
      });

      // Operating Mode set
      this->server.on("/api/mode", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetMode(request, data, len, index, total);
      });

      // Pulse (shaper)
      this->server.on("/api/shaper/pulse", HTTP_POST, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiShaperPulse(request, data, len, index, total);
      });

      // State get
      this->server.on("/api/state", HTTP_GET, [this](AsyncWebServerRequest *request) {
        this->handleApiGetState(request);
      });

      // Set over temperature limit
      this->server.on("/api/protections/over-temperature", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetOverTemperatureLimit(request, data, len, index, total);
      });

      // Set over current limit
      this->server.on("/api/protections/over-current", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetOverCurrentLimit(request, data, len, index, total);
      });

      // Set over voltage limit
      this->server.on("/api/protections/over-voltage", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetOverVoltageLimit(request, data, len, index, total);
      });

      // Set over current limit
      this->server.on("/api/protections/over-power", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetOverPowerLimit(request, data, len, index, total);
      });

      // Reset protections
      this->server.on("/api/protections/reset", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiResetProtections(request);
      });

      // Disable protections
      this->server.on("/api/protections/disable", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiDisableProtections(request);
      });

      // Enable protections
      this->server.on("/api/protections/enable", HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiEnableProtections(request);
      });

      // Power auto-detect enable
      this->server.on("/api/power/auto-detect/enable",  HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiSetPowerAutoDetect(request, true);
      });

      // Power auto-detect enable
      this->server.on("/api/power/auto-detect/disable",  HTTP_POST, [this](AsyncWebServerRequest *request) {
        this->handleApiSetPowerAutoDetect(request, false);
      });

      // Set over current limit
      this->server.on("/api/power/auto-detect/delay", HTTP_PUT, [this](AsyncWebServerRequest *request) {}, NULL, [this](AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
        this->handleApiSetPowerAutoDetectDelay(request, data, len, index, total);
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
  Shaper& shaper;
  Service& srv;

  char contentIndexHtml[4096];
  char contentStyleCss[4096];
  char contentLoadJs[8192];

  /** Handle Voltage get request. */
  void handleApiGetVoltage(AsyncWebServerRequest *request) {
    float loadVoltage = this->load.getLoadVoltage();
    float loadVoltage1 = this->load.getLoadVoltage1();
    float loadVoltage2 = this->load.getLoadVoltage2();

    this->sendFormattedJsonResponse(request, "{ \"voltage\": %.3f, \"voltage1\": %.3f, \"voltage2\": %.3f  }", loadVoltage, loadVoltage1, loadVoltage2);
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

    bool success = this->load.setCurrent(current);

    this->sendStatusResponse(request, success);
  }

  /** Handle Power set request. */
  void handleApiSetPower(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float power = atof(valueStr.c_str());

    bool success = this->load.setPower(power);

    this->sendStatusResponse(request, success);
  }

  /** Handle Resistance set request. */
  void handleApiSetResistance(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float resistance = atof(valueStr.c_str());

    bool success = this->load.setResistance(resistance);

    this->sendStatusResponse(request, success);
  }

  /** Handle Fan Speed set request. */
  void handleApiSetFanSpeed(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float value = atof(valueStr.c_str());

    bool success = this->load.setFanSpeed(value);

    this->sendStatusResponse(request, success);
  }

  /** Handle Load Enable / Disable request */
  void handleApiLoadEnable(AsyncWebServerRequest *request, bool enabled) {
    bool success = this->load.setEnabled(enabled);

    this->sendStatusResponse(request, success);
  }

  /** Handle Operating Mode set request. */
  void handleApiSetMode(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    valueStr.toUpperCase();
    Load::Mode mode;
    if (valueStr == "CONSTANT_CURRENT") {
      mode = Load::CONSTANT_CURRENT;

    } else if (valueStr == "CONSTANT_POWER") {
      mode = Load::CONSTANT_POWER;

    } else if (valueStr == "CONSTANT_RESISTANCE") {
      mode = Load::CONSTANT_RESISTANCE;

    } else {
      // invalid mode
      request->send(400, "application/json", "{ \"error\": \"Invalid mode\" }");
      return;
    }

    bool success = this->load.setMode(mode);

    this->sendStatusResponse(request, success);
  }

  /** Pulse */
  void handleApiShaperPulse(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String currentAndDuration = this->readBody(data, len, index, total);
    size_t separatorIdx = currentAndDuration.indexOf(',');
    if (separatorIdx == -1) {
      request->send(400, "application/json", "{ \"error\": \"Invalid parameters\" }");
      return;
    }

    float current = atof(currentAndDuration.substring(0, separatorIdx).c_str());
    uint32_t durationMicros = atoi(currentAndDuration.substring(separatorIdx + 1).c_str());

    bool success = this->shaper.pulse(current, durationMicros);

    this->sendStatusResponse(request, success);
  }

  /** Handle DAC set request (service/test). */
  void handleApiSrvDacSet(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    uint16_t value = atoi(valueStr.c_str());

    this->srv.dacSet(value);

    this->sendStatusResponse(request, true);
  }

  /** Handle State get request */
  void handleApiGetState(AsyncWebServerRequest *request) {
    bool enabled = this->load.isEnabled();
    Load::Mode mode = this->load.getMode();
    float setCurrent = this->load.getSetCurrent();
    float setPower = this->load.getSetPower();
    float setResistance = this->load.getSetResistance();
    float fanSpeed = this->load.getFanSpeed();

    const char* modeStr = "";
    switch (mode) {
      case Load::CONSTANT_CURRENT:
        modeStr = "CONSTANT_CURRENT";
        break;
      case Load::CONSTANT_POWER:
        modeStr = "CONSTANT_POWER";
        break;
      case Load::CONSTANT_RESISTANCE:
        modeStr = "CONSTANT_RESISTANCE";
        break;
    }

    const char* protectionStateStr = "";
    switch (this->load.getProtectState()) {
      case Load::OK:
        protectionStateStr = "OK";
        break;
      case Load::OK_DISABLED:
        protectionStateStr = "OK_DISABLED";
        break;
      case Load::TRIPPED_OVER_TEMPERATURE:
        protectionStateStr = "TRIPPED_OVER_TEMPERATURE";
        break;
      case Load::TRIPPED_OVER_VOLTAGE:
        protectionStateStr = "TRIPPED_OVER_VOLTAGE";
        break;
      case Load::TRIPPED_OVER_CURRENT:
        protectionStateStr = "TRIPPED_OVER_CURRENT";
        break;
      case Load::TRIPPED_OVER_POWER:
        protectionStateStr = "TRIPPED_OVER_POWER";
        break;
    }

    float overTemperatureLimit = this->load.getOverTemperatureLimit();
    float overCurrentLimit = this->load.getOverCurrentLimit();
    float overVoltageLimit = this->load.getOverVoltageLimit();
    float overPowerLimit = this->load.getOverPowerLimit();

    this->sendFormattedJsonResponse(request,
      "{ \"enabled\": %s, \"mode\": \"%s\", \"setCurrent\": %.3f, \"setPower\": %.3f, \"setResistance\": %.3f, \"fanSpeed\": %.2f, \"protections\": { \"state\": \"%s\", \"overTemperatureLimit\": %.2f, \"overCurrentLimit\": %.3f, \"overVoltageLimit\": %.3f, \"overPowerLimit\": %.3f } }",
      enabled ? "true" : "false", modeStr, setCurrent, setPower, setResistance, fanSpeed,
      protectionStateStr, overTemperatureLimit, overCurrentLimit, overVoltageLimit, overPowerLimit);
  }

  /** Handle Over temperature set request */
  void handleApiSetOverTemperatureLimit(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float temp = atof(valueStr.c_str());

    bool success = this->load.setOverTemperatureLimit(temp);

    this->sendStatusResponse(request, success);
  }

  /** Handle Over current set request */
  void handleApiSetOverCurrentLimit(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float current = atof(valueStr.c_str());

    bool success = this->load.setOverCurrentLimit(current);

    this->sendStatusResponse(request, success);
  }

  /** Handle Over voltage set request */
  void handleApiSetOverVoltageLimit(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float voltage = atof(valueStr.c_str());

    bool success = this->load.setOverVoltageLimit(voltage);

    this->sendStatusResponse(request, success);
  }

  /** Handle Over power set request */
  void handleApiSetOverPowerLimit(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    float power = atof(valueStr.c_str());

    bool success = this->load.setOverPowerLimit(power);

    this->sendStatusResponse(request, success);
  }

  /** Handle Reset protections request */
  void handleApiResetProtections(AsyncWebServerRequest *request) {
    bool success = this->load.resetProtections();

    this->sendStatusResponse(request, success);
  }

  /** Handle Disable protections request */
  void handleApiDisableProtections(AsyncWebServerRequest *request) {
    bool success = this->load.enableProtections(false);

    this->sendStatusResponse(request, success);
  }

  /** Handle Enable protections request */
  void handleApiEnableProtections(AsyncWebServerRequest *request) {
    bool success = this->load.enableProtections(false);

    this->sendStatusResponse(request, success);
  }

  /** Handle power auto detection */
  void handleApiSetPowerAutoDetect(AsyncWebServerRequest *request, bool enable) {
    bool success = this->load.setAutoEnableDisableOnPower(enable);

    this->sendStatusResponse(request, success);
  }

  /** Handle power auto detection delay set request */
  void handleApiSetPowerAutoDetectDelay(AsyncWebServerRequest *request, uint8_t *data, size_t len, size_t index, size_t total) {
    String valueStr = this->readBody(data, len, index, total);
    uint32_t delayMs = atoi(valueStr.c_str());

    bool success = this->load.setAutoEnableDelayMs(delayMs);

    this->sendStatusResponse(request, success);
  }

  /** Handle DAC swipe request (service/test). */
  void handleApiSrvDacSwipe(AsyncWebServerRequest *request) {
    this->srv.dacSwipe();

    this->sendStatusResponse(request, true);
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

  /** Send status response. */
  void sendStatusResponse(AsyncWebServerRequest *request, bool success) {
    this->sendFormattedJsonResponse(request, "{ \"status\": \"%s\" }", success ? "OK" : "FAIL");
  }

  /** Send formatted json response. */
  void sendFormattedJsonResponse(AsyncWebServerRequest *request, const char* format, ...) {
    // format response string
    char buffer[512];
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