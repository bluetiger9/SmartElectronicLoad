/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#ifndef WIFI_H
#define WIFI_H

#include <ArduinoOTA.h>
#include <WiFi.h>

static WiFiClass &globalWifi = WiFi;

/** Handles WiFi connection */
class Wireless {
public:
  const char *ssid = WIFI_SSID;
  const char *password = WIFI_PASSWORD;

  void begin() {
    Serial.println();
    Serial.print("Connecting to ");
    Serial.println(ssid);

    globalWifi.begin(this->ssid, this->password);

    while (globalWifi.status() != WL_CONNECTED) {
      delay(500);
      Serial.print(".");
    }

    Serial.println("");
    Serial.println("WiFi connected.");
    Serial.println("IP address: ");
    Serial.println(globalWifi.localIP());
  }

};

#endif