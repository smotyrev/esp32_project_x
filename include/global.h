#ifndef ESP32_PROJECT_X_GLOBAL_H
#define ESP32_PROJECT_X_GLOBAL_H

#include <DS3231.h>
#include <Preferences.h>

// true - Если нужно в TTY-мониторе удобно смотреть все данные по датчикам и событиям.
//      В этом режиме можно менять настройки, путем отправки команд в консоли.
//      Команды:
//          1) Установка даты и времени, для часов с батарейкой:
//              `datetime YYMMDDwHHMMSS` (прим. `datetime 2104165223000` => 16 апреля 2021 года 22:30:00, пятница)
//                  YY - Год
//                  MM - Месяц
//                  DD - День
//                  w - Day Of Week (день недели 1-7)
//                  HH - Час (0-23)
//                  MM - Минута
//          2) Установить дату начала цикла роста:
//              `startGrow YYMMDDwHH` (прим. `startGrow 21041622` => 16 апреля 2021 года в 22 часа)
// false - Если нужен боевой режим, для отправки данных на экран
#define DEBUG true

// Время каждого цикла main::loop() в миллисекундах
#define LOOP_MS_DEBUG 2000
#define LOOP_MS_NORMAL 1000

// Температура и влажность
#define DHT_PIN         15      // GPIO-пин, который подключен к датчику DHT
#define DHT_TYPE        DHT22   // Модель используемого датчика (DHT11, DHT21 или DHT22)
#define DS18B20_PIN     4       // GPIO where the DS18B20 is connected to

// Пины для коннекта Arduino:
// i2c : arduino -> esp32
#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22
#define I2C_ADDR        8       // номер девайса, должен быть выставлен одинаково на обоих концах

// поплавок, датчик воды
#define FLOAT_SENSOR_PIN  14    // the number of the pushbutton pin

//////////////
// Реле пины:
//////////////

// Насос высокого давления. Вклюючается периодически на несколько секунда
#define PUMP_HIGH_PIN   23        // насос высокого давления
#define timeoutPumpHigh 15        // 15 секунд таймаут
#define timePumpHigh    7         // 7 екунд - время работы насоса

// Свет и охлаждение света. По несколько реле на один пин, для разного напряжения.
// Из расчета мощьности ламп и мощьности релешек.
// Логика работы:
//  Когда запустился цикл выращивания, система запускает свет на 12 часов. Далее каждый день прибавляется по 20 минут,
//  Пока длинна "светового дня" не достигнет 19 часов.
#define LIGHT_PIN           13  // провести провода с этого пина на 2 реле включения света + на 2 реле охлаждения света
#define lightTimeOn         10  // Во сколько часов свет включается
#define lightMinHours       12  // Стартуем с 12-ти часовым "световым днем"
#define lightMaxHours       19  // Максимальное время "светового дня", в часах
#define lightDeltaMinutes   20  // На сколько минут увеличивается световой день, каждый день



// ~~~~~~~~~~~~~~~~~~
// Внутренние, вспомогательные функции

#define PREFS_START_GROW "start-grow"
#define PREFS_KEY_TIMESTAMP "TS"
#define TERMINATE_SCREEN_POSTFIX char(255)+char(255)+char(255)

inline void logEvent(const String &event) {
    Serial.println("\n\t[" + event + "]");
}

inline void logToScreen(const String &key, const String &value) {
    Serial.print((String) key + "=\"" + value + "\"" + TERMINATE_SCREEN_POSTFIX);
}

inline void logWire(const String &prefix, const String &postfix) {
    Serial.print(prefix);
    Wire.requestFrom(I2C_ADDR, 6); // request 6 bytes from slave device #8
    while (Wire.available()) {          // slave may send less than requested
        char c = (char) Wire.read();    // receive a byte as character
        Serial.print(c);                // print the character
    }
    Serial.print(postfix);
}

inline void logWireToScreen(const String &key, const String &prefix, const String &postfix) {
    logWire(key + "=\"" + prefix, postfix + "\"");
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

#endif //ESP32_PROJECT_X_GLOBAL_H
