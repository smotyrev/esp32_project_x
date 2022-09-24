#ifndef ESP32_PROJECT_X_X_TEMPERATURE_HUMIDITY_H
#define ESP32_PROJECT_X_X_TEMPERATURE_HUMIDITY_H

// влажность и температура Adafruit DHT22 (внешний датчик)
// влажность и температура DFRobot STH20 (водонепроницаемый датчик, i2c)
// температура DS18B20 (водонепроницаемый датчик, 1-Wire)

#include <DHT.h>
#include <DHT_U.h>
#include <DFRobot_SHT20.h>
#include <OneWire.h>
#include <DallasTemperature.h>

#include "global.h"

class x_temperature_humidity : public main_looper {
public:
    void setup();
    void loop(bool forceDataSend);
    bool processConsoleCommand(std::string &cmd);
};


#endif //ESP32_PROJECT_X_X_TEMPERATURE_HUMIDITY_H
