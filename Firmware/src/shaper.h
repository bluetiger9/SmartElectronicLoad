/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>

#ifndef SHAPER_H
#define SHAPER_H

#include "load.h"

/** Generate custom current (/power /resistance) shapes. */
class Shaper {

public:

  struct Entry {
    float value;
    uint64_t durationMicros;
  };

  Shaper(Load &load, const uint16_t maxEntries)
    : load(load), maxEntries(maxEntries), nrEntries(0) {

      this->entries = new Entry[maxEntries];
  }

  /** Is the shaper currently active */
  bool isActive() {
    return this->active;
  }

  /** Handle shape generation */
  void handle() {
    if (!this->active) {
      return;
    }

    uint64_t now = micros();
    if (this->lastChangeMicros == 0) {
      // first entry
      float current = this->entries[this->currentIdx].value;
      this->load.setCurrent(current);
      this->lastChangeMicros = micros();
      return;
    }

    if (now - this->lastChangeMicros >= this->entries[this->currentIdx].durationMicros) {
      // move to next entry
      this->currentIdx++;
      if (this->currentIdx >= this->nrEntries) {
        // end of shape
        this->load.setCurrent(0.0);
        this->active = false;
        return;
      }

      // set load value
      float current = this->entries[this->currentIdx].value;
      this->load.setCurrent(current);

      this->lastChangeMicros = now;
    }
  }

  /** Generate a current pulse */
  bool pulse(float current, uint32_t durationMicros) {
    this->entries[0].value = 0.0;
    this->entries[0].durationMicros = 0;
    this->currentIdx = 0;
    this->nrEntries = 1;
    this->lastChangeMicros = 0;
    this->active = true;
    return true;
  }

private:
  Load &load;
  Entry *entries;
  uint16_t nrEntries;
  const uint16_t maxEntries;

  bool active = false;
  uint16_t currentIdx = 0;
  uint64_t lastChangeMicros = 0;
};

#endif