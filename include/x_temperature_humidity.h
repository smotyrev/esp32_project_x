#ifndef ESP32_PROJECT_X_X_TEMPERATURE_HUMIDITY_H
#define ESP32_PROJECT_X_X_TEMPERATURE_HUMIDITY_H

// влажность и температура Adafruit DHT22 (внешний датчик, i2c)
// влажность и температура DFRobot STH20 (водонепроницаемый датчик, i2c)
// температура DS18B20 (водонепроницаемый датчик, 1-Wire)

#include <DHT.h>
#include <DHT_U.h>
#include <DFRobot_SHT20.h>
#include <thread>

using namespace std;

#include "global.h"

#define DHTPIN            15        // Контакт, который подключен к датчику DHT
#define DHTTYPE           DHT22     // Модель используемого датчика (DHT11, DHT21 или DHT22)

//температура
#include <OneWire.h>
#include <DallasTemperature.h>

#define DS18B20_PIN     4           // GPIO where the DS18B20 is connected to

class x_temperature_humidity {
public:
    void setup();

    void loop();
};


#endif //ESP32_PROJECT_X_X_TEMPERATURE_HUMIDITY_H
