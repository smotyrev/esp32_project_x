#ifndef ESP32_PROJECT_X_GLOBAL_H
#define ESP32_PROJECT_X_GLOBAL_H

#include <DS3231.h>
#include <Preferences.h>

//#define RELAY_ON LOW        // для нормально-замкнутых реле
//#define RELAY_OFF HIGH      // для нормально-замкнутых реле
#define RELAY_ON HIGH       // для нормально-разомкнутых реле
#define RELAY_OFF LOW       // для нормально-разомкнутых реле
#define SRELAY_ON LOW       // для соленойдного реле
#define SRELAY_OFF HIGH     // для соленойдного реле

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
//              `startGrow YYMMDDHH` (прим. `startGrow 21041622` => 16 апреля 2021 года в 22 часа)
// false - Если нужен боевой режим, для отправки данных на экран
#define DEBUG false
// true - Печатать больше DEBUG данных, помимо датчиков и событий реле
#define VERBOSE false

// Время каждого цикла main::loop() в миллисекундах
#define LOOP_MS_DEBUG 2000
#define LOOP_MS_NORMAL 1000

// Температура и влажность
#define DHT_PIN         15      // GPIO-пин, который подключен к датчику DHT
#define DHT_TYPE        DHT22   // Модель используемого датчика (DHT11, DHT21 или DHT22)
#define DS18B20_PIN     4       // GPIO where the DS18B20 is connected to

#define PH_PIN          32 //the esp gpio data pin number
#define ESPADC          4096.0   //the esp Analog Digital Convertion value
#define ESPVOLTAGE      3300 //the esp voltage supply value

// Пины для коннекта Arduino:
// i2c : arduino -> esp32
#define I2C_SDA_PIN     21
#define I2C_SCL_PIN     22
#define I2C_ADDR        8       // номер девайса, должен быть выставлен одинаково на обоих концах

// поплавок, датчик воды
#define FLOAT_SENSOR_PIN  27        // Поплавок на бак с водой
#define FLOAT_SENSOR2_PIN  26       // Поплавок 2 на резервуар с раствором (Программа 2)

//////////////
// Реле пины:
//////////////

// Программа 1: Вклюючается периодически на несколько секунд.
// Программа 2: Включаем периодически, накачиваем раствор, ждем 20 секунд, откачиваем раствор.
#define PUMP_PROGRAM 2

// Программа 1: Насос высокого давления. Накачиваем раствор.
#define PUMP_HIGH_PIN   23          // насос высокого давления
#define PUMP_HIGH_PIN_HV   19       // насос высокого давления высоковольтное реле
#define timeoutPumpHigh 160         // секунд - таймаут, между включениями
#define timePumpHigh    15          // секунд - время работы насоса

// Программа 2: Насос высокого давления. Откачиваем раствор.
#define PUMP_HIGH2_PIN   25         // насос высокого давления 2
#define timeoutPumpHigh2    20      // секунд - время, после срабатывания поплавка 2, после которого нужно включать 1ый насос
#define timePumpHigh2    60         // секунд - время работы 2ого насоса

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

// ВЛАЖНОСТЬ В БОКСЕ
// Если влажность падает ниже 55% включается увлажнитель, пока значение влажности не достигнет 65%
// Если влажность более 70%, включается вентилятор вытяжки и вентиляторы притока, пока влажность не достигнет 65%
#define BOX_HUMID_PIN       12  // Увлажнитель
#define BOX_VENT_PIN        14  // Вентилятор
#define boxHumidMin         55.0f
#define boxHumidOk          65.0f
#define boxHumidMax         70.0f

// ТЕМПЕРАТУРА В БОКСЕ.
// Если температура падает ниже 17 градусов, включается обогрев пока температура не достигнет показания 22 градуса.
// Если температура достигает выше 28 градусов, то включается вентилятор вытяжки и вентиляторы
// притока(те же, что в логике "ВЛАЖНОСТЬ В БОКСЕ"), пока температура не упадёт до 26 градусов.

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
    logWire(key + "=\"" + prefix, postfix + "\"" + TERMINATE_SCREEN_POSTFIX);
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
