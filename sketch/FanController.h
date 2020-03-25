#include <OneWire.h>
#include <DallasTemperature.h>
#include <PID_v1.h>

class FanController {
  // dallas results
  static const int8_t NOT_INITIALIZED = 85;
  static const int8_t NO_SENSOR = -127;

  static const uint16_t MEASUREMENT_INTERVAL = 750;

  OneWire wire;
  DallasTemperature sensor;
  
  bool connected = false;
  unsigned long last_measurement = 0;

  uint8_t fan_pin;
  uint8_t sensor_pin;
  float current_temperature;

  bool connect_sensor() {
    sensor.begin();

    if (sensor.getDS18Count() > 0) {
      connected = true;
      sensor.requestTemperatures();
    }
  }

  bool read_temperature() {
    unsigned long delta_t = millis() - last_measurement;
    float temperature;

    if (delta_t < MEASUREMENT_INTERVAL) {
      // not ready yet
      return false;
    }

    // read temperature from sensor
    temperature = sensor.getTempCByIndex(0);

    if (temperature == NO_SENSOR || temperature == NOT_INITIALIZED) {
      // sensor needs to be reconnected
      connected = false;
      return false;
    }

    // re-request temperature
    sensor.requestTemperatures();
    last_measurement = millis();

    current_temperature = temperature;
    return true;
  }
  

public:
  FanController(uint8_t fan_pin, uint8_t sensor_pin) {
    this->fan_pin = fan_pin;
    this->sensor_pin = sensor_pin;

    wire = OneWire(sensor_pin);
    sensor = DallasTemperature(&wire);

    sensor.setWaitForConversion(false);
  }

  bool process() {
    if (!connected && !connect_sensor()) {
      // no sensor connected and connecting failed
      return false;
    }

    if (!read_temperature()) {
      // temperature measurement not ready yet or sensor disconnected
      return false;
    }

    return true;
  }

  float get_temperature() const {
    return current_temperature;
  }
};