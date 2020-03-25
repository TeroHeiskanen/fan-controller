#include "FanController.h"
#include "TemperatureSensor.h"

static const uint8_t MIN_SPEED = 45;
static const uint8_t MAX_SPEED = 80;
static const double TARGET_TEMPERATURE = 28;
static const uint16_t LOG_INTERVAL = 2500;

static unsigned long last_log = 0;
static unsigned long delta_t;
static float temperature;

static const TemperatureSensor sensors[2] = { 
  TemperatureSensor(10), 
  TemperatureSensor(12)
};

static const FanController fan_controllers[2] = {
  FanController(9, &sensors[0], TARGET_TEMPERATURE, MIN_SPEED, MAX_SPEED),
  FanController(11, &sensors[1], TARGET_TEMPERATURE, MIN_SPEED, MAX_SPEED)
};

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

bool should_log() {
  unsigned long now = millis();

  if (now - last_log < LOG_INTERVAL) {
    // not ready yet
    return false;
  }

  last_log = now;
  return true;
}

void loop() {
  float temperature;
  bool log = should_log();

  for (uint8_t i = 0; i < 2; i++) {
    if (fan_controllers[i].process() && log) {
      Serial.print(i);
      Serial.print(": ");
      Serial.print(fan_controllers[i].get_temperature());
      Serial.print("C, ");
      Serial.print((uint8_t) (((double) fan_controllers[i].get_speed() / (double) MAX_SPEED) * 100.0));
      Serial.println("%");  
    }
  }
}
