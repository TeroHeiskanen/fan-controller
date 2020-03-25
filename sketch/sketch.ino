#include "FanController.h"

static const FanController fan_controllers[2] = {
  FanController(9, 10),
  FanController(11, 12)
};
static const uint16_t interval = 5000;

void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);
}

unsigned long max_time = 0;

void loop() {
  static float temperature;

  for (uint8_t i = 0; i < 2; i++) {
    

    if (fan_controllers[i].process()) {
      Serial.print(i);
      Serial.print(": ");
      Serial.println(fan_controllers[i].get_temperature());
    }
  }

  delay(interval - (millis() % interval));
}
