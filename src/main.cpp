// include library to read and write from flash memory
#include <EEPROM.h>
#include <Preferences.h>

#include "main.h"
#include "x_time.h"
#include "x_temperature_humidity.h"

x_time xTime;
x_temperature_humidity xTempHumid;

Preferences preferences;
DateTime startGrow;

void setup() {
    Wire.begin(I2C_SDA, I2C_SCL);
    Serial.begin(9600);

    xTime.setup();
    xTempHumid.setup();

    // Сохраненные настройки
    preferences.begin("START_GROW", false);
    if (CLEAR_START_GROW) { preferences.clear(); }
    uint32_t ts = preferences.getUInt("TS", 0);
    Serial.println((String) "[PREF GET] Start grow unixtime: " + ts);
    if (ts == 0) {
        ts = xTime.now.unixtime();
        Serial.println((String) "[PREF SET] Start grow unixtime: " + ts);
        preferences.putUShort("TS", ts);
    }
    startGrow = DateTime(ts);

    // поплавок, датчик воды
    pinMode(FLOAT_SENSOR, INPUT_PULLUP); // initialize the pushbutton pin as an input

    // RELAY PIN INIT
    pinMode(PUMP_HIGH_PIN, OUTPUT);
    digitalWrite(PUMP_HIGH_PIN, HIGH);
    pinMode(LIGHT_PIN, OUTPUT);
    digitalWrite(LIGHT_PIN, HIGH);
    Serial.println("\r\n---- ~ SETUP ----");
}

void loop() {
    if (DEBUG) {
        delay(1000);
    }
    //delay(dhtDelayMS);
    if (DEBUG) { Serial.println("\r\n-------------------"); }

    // печатаем время
    xTime.loop();

    // печатаем температуру и влвжность
    xTempHumid.loop();

    // печатаем показания ph-датчика с arduino
    if (DEBUG) {
        logWire("Arduino PH: ", "\r\n");
    } else {
        logWireToScreen("d12.txt", "", "");
    }

    // печатаем - поплавок, датчик воды
    if (DEBUG) {
        Serial.print((String) "Float: " + digitalRead(FLOAT_SENSOR));
    } else {
        if (digitalRead(FLOAT_SENSOR) == LOW) { // поплавок тонет, уровень высокий
            logToScreen("d14.txt", "HIGH");
        } else {
            logToScreen("d14.txt", "LOW");
        }
    }

    // -----------------
    // RELAY processing:
    // -----------------

    // управляяем насосом высокого давления
    if (pumpHighTS_start > 0) {
        uint32_t dTS = xTime.nowTS - pumpHighTS_start;
        if (dTS >= timePumpHigh) {
            pumpHighTS_start = 0;
            pumpHighTS_end = xTime.now.unixtime();
            digitalWrite(PUMP_HIGH_PIN, HIGH);
            if (DEBUG) { logEvent("мотор высокого давления: выкл"); }
        }
    } else {
        // включаем мотор высокого давления
        uint32_t dTS = xTime.nowTS - pumpHighTS_end;
        if (dTS >= timeoutPumpHigh) {
            pumpHighTS_end = 0;
            pumpHighTS_start = xTime.now.unixtime();
            digitalWrite(PUMP_HIGH_PIN, LOW);
            if (DEBUG) { logEvent("мотор высокого давления: вкл."); }
        }
    }

    // управляем светом
    uint32_t deltaSeconds = xTime.now.unixtime() - startGrow.unixtime();
    uint32_t deltaDays = deltaSeconds / (24 * 60 * 60);
    if (DEBUG) {
        logEvent((String) "deltaSeconds=" + deltaSeconds + " deltaDays=" + deltaDays);
    }
//    if (isLightOn) {
//        if (xTime.nowHour >= timeLightOff) {
//            digitalWrite(LIGHT_PIN, HIGH);
//            isLightOn = false;
//            if (DEBUG) { logEvent("свет: выкл."); }
//        }
//    } else {
//        if (xTime.nowHour >= lightTimeOn && xTime.nowHour < timeLightOff) {
//            isLightOn = true;
//            digitalWrite(LIGHT_PIN, LOW);
//            if (DEBUG) { logEvent("свет: вкл."); }
//        }
//    }
}