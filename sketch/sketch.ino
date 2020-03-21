#include <OneWire.h>
#include <DallasTemperature.h>

// pin definitions
static const uint8_t fan_count = 2;
static const uint8_t fan_pins[2] = {9, 11};
static const uint8_t sensor_pins[2] = {10, 12};

// sensors
static OneWire wires[fan_count];
static DallasTemperature sensors[fan_count];
static bool running[fan_count] = {false, false};

// dallas results
static const int8_t NOT_INITIALIZED = 85;
static const int8_t NO_SENSOR = -127;

// fan profile
static const uint8_t profile_values_size = 4;
static const uint8_t profile_values[profile_values_size] = {50, 55, 70, 128};
static const float profile_start_temperature = 28.0; // profile min range (fans at minimum profile setting)
static const float profile_stop_temperature = 40.0; // profile max range (fans at highest profile setting)
static const float profile_delta_temperature = profile_stop_temperature - profile_start_temperature; // pre-calculate delta

// fan hysteresis
static const float fan_start_temperature = 32.0; // temperature to start the fans at
static const float fan_stop_temperature = 28.0; // temperature to stop the fans at

// others
static const short interval = 500;

uint8_t calculateSpeed(float temperature) {
  temperature = constrain(temperature, profile_start_temperature, profile_stop_temperature);
  
  float quality_factor = (temperature - profile_start_temperature) / profile_delta_temperature;
  
  // calculate the value between two points
  float x = (profile_values_size - 1) * quality_factor;
  uint8_t x_0 = x;
  uint8_t x_1 = x + 1;
  uint8_t y_0 = profile_values[x_0];
  uint8_t y_1 = profile_values[x_1];

  if (x_1 >= profile_values_size) {
    x_1 = profile_values_size - 1;
  }

  if (x_0 == x_1) {
    return y_0;
  }

  float slope = (y_1 - y_0) / (x_1 - x_0);
  return (slope * (x - x_0)) + y_0;
}


bool readTemperature(DallasTemperature* sensor, float* temperature) {
  sensor->requestTemperatures();
  *temperature = sensor->getTempCByIndex(0);

  if (*temperature == NOT_INITIALIZED) {
    sensor->begin();
    return false;
  } else if (*temperature == NO_SENSOR) {
    return false;
  }

  return true;
}


void setup() {
  // put your setup code here, to run once:
  Serial.begin(9600);

  for (uint8_t i = 0; i < fan_count; i++) {
    // set fan pin as output
    pinMode(fan_pins[i], OUTPUT);

    // initialize sensors
    wires[i] = OneWire(sensor_pins[i]);
    sensors[i] = DallasTemperature(&wires[i]);
  }
}


void loop() {
  static float temperature;
  static bool result;
  static uint8_t speed;
  
  for (uint8_t i = 0; i < fan_count; i++) {
    // read temperature
    result = readTemperature(&sensors[i], &temperature);
    speed = 0;

    if (!result) {
      analogWrite(fan_pins[i], 0);
      continue;
    }

    // hysteresis control
    if (running[i] && temperature <= fan_stop_temperature) {
      running[i] = false;
    } else if (!running[i] && temperature >= fan_start_temperature) {
      running[i] = true;
    }

    // calculate and set speed for fan if it needs to be running
    if (running[i]) {
      speed = calculateSpeed(temperature);
    }

    analogWrite(fan_pins[i], speed);

    // print info to serial
    Serial.print(i);
    Serial.print(": ");
    Serial.print(temperature);
    Serial.print("C, ");
    Serial.print(uint8_t(float(speed) / 255.0 * 100.0));
    Serial.println("%");    
  }

  delay(interval);
}
