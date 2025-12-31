/*
 * Copyright (c) 2025 by Attila Tőkés.
 *
 * Licence: MIT
 */
#include <Arduino.h>

#ifndef CALIB_H
#define CALIB_H

/** Software calibration for sensor values */
class Calibration {

public:

  struct Entry {
    float adcValue;
    float calibratedValue;
  };

  const Entry *entries;
  const uint8_t nrEntries;

  Calibration(const Entry *entries, const uint8_t nrEntries)
    : entries(entries), nrEntries(nrEntries) {
  }

  /** Get calibrated value */
  float getCalibratedValue(float value) {
    // binary search
    uint8_t idx = findIdx(value, 0, nrEntries - 1);

    if ((idx <= 1) || (idx >= nrEntries)) {
      // outside of all calibration ranges, return raw value
      return value;

    } else {
      // linear interpolation
      float x0 = entries[idx - 1].adcValue;
      float y0 = entries[idx - 1].calibratedValue;
      float x1 = entries[idx].adcValue;
      float y1 = entries[idx].calibratedValue;

      return y0 + (value - x0) * (y1 - y0) / (x1 - x0);
    }
  }

private:
  uint8_t findIdx(float value, uint8_t start, uint8_t end) {
    if (start >= end) {
      return start;
    }

    uint8_t mid = start + (end - start) / 2;
    if (value < entries[mid].adcValue) {
      return findIdx(value, start, mid);
    } else {
      return findIdx(value, mid + 1, end);
    }
  }
};

#endif