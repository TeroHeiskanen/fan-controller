#ifndef TEMPERATURE_SENSOR_H
#define TEMPERATURE_SENSOR_H

#include <OneWire.h>
#include <DallasTemperature.h>

enum class TemperatureSensorResult { OK, WAIT, NO_SENSOR };

class TemperatureSensor {
  // dallas results
  static const int8_t NOT_INITIALIZED = 85;
  static const int8_t NO_SENSOR = -127;

  static const uint16_t MEASUREMENT_INTERVAL = 750;

  OneWire wire;
  DallasTemperature sensor;
  bool connected = false;
  unsigned long last_measurement = 0;

  bool connect() {
    sensor.begin();

    if (sensor.getDS18Count() > 0) {
      connected = true;
      sensor.requestTemperatures();
      last_measurement = millis();
    }
  }  

public: 
  TemperatureSensor(uint8_t sensor_pin) {
    wire = OneWire(sensor_pin);
    sensor = DallasTemperature(&wire);
    sensor.setWaitForConversion(false);
  }

  TemperatureSensorResult read(double *temperature) {
    if (!connected && !connect()) {
      // no sensor connected and connecting failed
      return TemperatureSensorResult::NO_SENSOR;
    }

    unsigned long delta_t = millis() - last_measurement;

    if (delta_t < MEASUREMENT_INTERVAL) {
      // not ready yet
      return TemperatureSensorResult::WAIT;
    }

    // read temperature from sensor
    *temperature = sensor.getTempCByIndex(0);

    if (*temperature == NO_SENSOR || *temperature == NOT_INITIALIZED) {
      // sensor needs to be reconnected
      connected = false;
      return TemperatureSensorResult::NO_SENSOR;
    }

    // re-request temperature
    sensor.requestTemperatures();
    last_measurement = millis();

    return TemperatureSensorResult::OK;
  }
};

#endif