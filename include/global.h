#ifndef ESP32_PROJECT_X_GLOBAL_H
#define ESP32_PROJECT_X_GLOBAL_H

// true - Если нужно в TTY-мониторе удобно смотреть все данные по датчикам и событиям
// false - Если нужен боевой режим, для отправки данных на экран
#define DEBUG true

// true - Если нужно задать время для часов с батарейкой
// В TTY-монитор отправить дату и время в формате:
// YYMMDDwHHMMSSx
// "x" в конце строки - просто спец-символ
#define SET_DATE_AND_TIME false

// true - Если надо очистить дату начала грова
#define CLEAR_START_GROW false

// Пины для коннекта:
// i2c : arduino -> esp32
#define I2C_SDA 21
#define I2C_SCL 22
#define I2C_ADDR 8 // номер девайса, должен быть выставлен одинаково на обоих концах

// поплавок, датчик воды
#define FLOAT_SENSOR  14     // the number of the pushbutton pin

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

#endif //ESP32_PROJECT_X_GLOBAL_H
