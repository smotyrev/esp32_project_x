#ifndef ESP32_PROJECT_X_GLOBAL_H
#define ESP32_PROJECT_X_GLOBAL_H

#include <DS3231.h>
#include <Preferences.h>

#include <stdio.h>
#include <iostream>   
#include <string>  
#include "freertos/FreeRTOS.h"
#include "freertos/task.h"
#include "driver/uart.h"
#include "driver/gpio.h"
#include "sdkconfig.h"
#include "esp_log.h"

//#define RELAY_ON LOW        // для нормально-замкнутых реле
//#define RELAY_OFF HIGH      // для нормально-замкнутых реле
#define RELAY_ON HIGH       // для нормально-разомкнутых реле
#define RELAY_OFF LOW       // для нормально-разомкнутых реле
#define SRELAY_ON LOW       // для соленойдного реле
#define SRELAY_OFF HIGH     // для соленойдного реле

// true - Если нужно в TTY-мониторе удобно смотреть все данные по датчикам и событиям.
// false - Если нужен боевой режим, для отправки данных на экран
#define DEBUG false
// true - Печатать больше DEBUG данных, помимо датчиков и событий реле
#define VERBOSE true

// Время каждого цикла main::loop() в миллисекундах
#define LOOP_MS_DEBUG 2000

// Температура и влажность
#define DHT_PIN         15      // GPIO-пин, который подключен к датчику DHT
#define DHT_TYPE        DHT22   // Модель используемого датчика (DHT11, DHT21 или DHT22)
#define DS18B20_PIN     4       // GPIO where the DS18B20 is connected to

// PH meter
#define PH_PIN          32 //the esp gpio data pin number
#define ESPADC          4096.0   //the esp Analog Digital Convertion value
#define ESPVOLTAGE      3300 //the esp voltage supply value

// TDS meter
#define TDS_PIN         25

// Пины для коннекта Arduino:
// i2c : arduino -> esp32
#define I2C_SDA_PIN     SCL
#define I2C_SCL_PIN     SDA

// поплавок, датчик воды
#define FLOAT_SENSOR_PIN  27        // Поплавок на бак с водой
#define FLOAT_SENSOR2_PIN  26       // Поплавок 2 на резервуар с раствором (Программа 2)

//////////////
// Реле пины:
//////////////

#define MAX_PUMP_PROGRAMS 2
#define MAX_LIGHT_PROGRAMS 4

// Программа 1: Насос высокого давления. Накачиваем раствор.
#define PUMP_HIGH_PIN 23
#define PUMP_HIGH_PIN_HV 19
 
#define timeoutPumpHigh 160         // секунд - таймаут, между включениями
#define timePumpHigh    15          // секунд - время работы насоса

// Программа 2: Насос высокого давления. Откачиваем раствор.
#define PUMP_HIGH2_PIN   5          // насос высокого давления 2
#define timeoutPumpHigh2    20      // секунд - время, после срабатывания поплавка 2, после которого нужно включать 1ый насос
#define timePumpHigh2    60         // секунд - время работы 2ого насоса

enum pumpForce { NoForcep = 0, Onp = 1, Offp = 2 }; // Принудительное включение/выключение насоса

// Свет и охлаждение света. По несколько реле на один пин, для разного напряжения.
// Из расчета мощьности ламп и мощьности релешек.
// Логика работы:
//  Когда запустился цикл выращивания, система запускает свет на 12 часов. Далее каждый день прибавляется по 20 минут,
//  Пока длинна "светового дня" не достигнет 19 часов.
#define LIGHT_PIN           13  // провести провода с этого пина на 2 реле включения света + на 2 реле охлаждения света
#define lightTimeOn         18  // Во сколько часов свет включается
#define lightMinHours       12  // Стартуем с 12-ти часовым "световым днем"
#define lightMaxHours       19  // Максимальное время "светового дня", в часах
#define lightDeltaMinutes   20  // На сколько минут увеличивается световой день, каждый день
enum LightForce { NoForce = 0, On = 1, Off = 2 }; // Принудительное включение/выключение света

// ВЛАЖНОСТЬ В БОКСЕ
// Если влажность падает ниже 55% включается увлажнитель, пока значение влажности не достигнет 65%
// Если влажность более 70%, включается вентилятор вытяжки и вентиляторы притока, пока влажность не достигнет 65%
#define BOX_HUMID_PIN       12  // Увлажнитель
#define BOX_VENT_PIN        14  // Вентилятор

// ТЕМПЕРАТУРА В БОКСЕ.
// Если температура падает ниже 17 градусов, включается обогрев пока температура не достигнет показания 22 градуса.
// Если температура достигает выше 28 градусов, то включается вентилятор вытяжки и вентиляторы
// притока(те же, что в логике "ВЛАЖНОСТЬ В БОКСЕ"), пока температура не упадёт до 26 градусов.

// ~~~~~~~~~~~~~~~~~~
// Внутренние, вспомогательные функции

#define PREFS_START_GROW "start-grow"
#define PREFS_KEY_TIMESTAMP "TS"
#define TERMINATE_SCREEN_POSTFIX "\xFF\xFF\xFF" //char(255)+char(255)+char(255)

#define PREFS_BOX "box"
#define PREFS_KEY_BOX_HUMID_MAX "humid-max"
#define PREFS_KEY_BOX_HUMID_MIN "humid-min"

#define PREFS_PROGRAM_PUPM "pp"
#define PREFS_KEY_PP_VAL "ppval"

#define PREFS_LIGHT "li"
#define PREFS_KEY_LI_VAL "lival"

inline void logEvent(const String &event) {
    Serial.println("\n\t[" + event + "]");
}

const byte ndt[3] = {255,255,255};
inline void logToScreen(std::string key, std::string value) {
    // Serial.print((String) TERMINATE_SCREEN_POSTFIX + key + "=\"" + value + "\"" + TERMINATE_SCREEN_POSTFIX);
    std::string data = key + "=\"" + value + "\"";
    auto res = uart_write_bytes(UART_NUM_2, (const char*) data.c_str(), data.length());
    uart_write_bytes(UART_NUM_2, ndt, 3);
    if (DEBUG || VERBOSE) {
        Serial.println(("logToScreen(" + data + "); res=" + std::to_string(res)).c_str());
    }
}
inline void logToScreen(std::string key, int value) {
    // Serial.print((String) TERMINATE_SCREEN_POSTFIX + key + "=\"" + value + "\"" + TERMINATE_SCREEN_POSTFIX);
    std::string data = key + "=" + std::to_string(value) ;
    auto res = uart_write_bytes(UART_NUM_2, (const char*) data.c_str(), data.length());
    uart_write_bytes(UART_NUM_2, ndt, 3);
    if (DEBUG || VERBOSE) {
        Serial.println(("logToScreen(" + data + "); res=" + std::to_string(res)).c_str());
    }
}

// trim from start (in place)
static inline void ltrim(std::string &s) {
    s.erase(s.begin(), std::find_if(s.begin(), s.end(), [](unsigned char ch) {
        return !std::isspace(ch);
    }));
}

// trim from end (in place)
static inline void rtrim(std::string &s) {
    s.erase(std::find_if(s.rbegin(), s.rend(), [](unsigned char ch) {
        return !std::isspace(ch);
    }).base(), s.end());
}

class main_data {
public:
    uint32_t nowTS;
    uint8_t nowHour;
    uint8_t nowMinute;
    float boxHumid = 0;
    float boxHumidMax = 70.0f;
    float boxHumidMin = 55.0f;
    DateTime startGrow;                 // Время начала роста
    Preferences preferences;    // Основные настройки
};

class main_looper {
public:
    virtual void setup(main_data &data) = 0;
    virtual void loop(bool forceDataSend) = 0;
    virtual bool processConsoleCommand(std::string &cmd) = 0;
};

#endif //ESP32_PROJECT_X_GLOBAL_H
