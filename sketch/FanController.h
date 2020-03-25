#ifndef FAN_CONTROLLER_H
#define FAN_CONTROLLER_H

#include "TemperatureSensor.h"
#include <PID_v1.h>

class FanController {
  // PID arguments
  static const double KP = 5;
  static const double KI = 0.1;
  static const double KD = 1;
  static const double START_OFFSET = -1;
  static const double STOP_OFFSET = -2;

  bool enabled = false;

  uint8_t fan_pin;
  
  double current_temperature;
  double target_temperature;

  uint16_t logging_interval = 0;
  uint16_t last_log = 0;
  
  double speed;
  uint8_t min_speed = 0;
  uint8_t max_speed = 255;
  
  TemperatureSensor *sensor;
  PID *pid;

public:
  FanController(uint8_t fan_pin, TemperatureSensor *sensor, double target_temperature, uint8_t min_speed, uint8_t max_speed) {
    this->fan_pin = fan_pin;
    this->sensor = sensor;
    this->target_temperature = target_temperature;

    pid = new PID(&current_temperature, &speed, &this->target_temperature, KP, KI, KD, REVERSE);
    pid->SetOutputLimits((double) min_speed, (double) max_speed);

    this->speed = 0;
  }

  bool process() {
    TemperatureSensorResult result = sensor->read(&current_temperature);
    pid->Compute();

    if (result == TemperatureSensorResult::NO_SENSOR) {
      // no sensor, disable controller
      enabled = false;
      speed = 0;
    } else if (enabled && target_temperature + STOP_OFFSET > current_temperature) {
      // controller on but temperature is low enough to stop it
      enabled = false;
      speed = 0;
    } else if (!enabled && target_temperature + START_OFFSET < current_temperature) {
      // controller not on but temperature is high enough to start it
      enabled = true;
      speed = min_speed;
    }

    pid->SetMode(enabled ? AUTOMATIC : MANUAL);
    analogWrite(this->fan_pin, speed);

    return result != TemperatureSensorResult::NO_SENSOR;
  }

  double get_temperature() const {
    return current_temperature;
  }

  uint8_t get_speed() const {
    return (uint8_t) speed; 
  }
};

#endif