// include library to read and write from flash memory
#include <EEPROM.h>

#include "main.h"
#include "x_time.h"
#include "x_temperature_humidity.h"

x_time xTime;
x_temperature_humidity xTempHumid;

DateTime startGrow;
Preferences preferences;

void setup() {
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN);
    Serial.begin(9600);

    xTime.setup();
    xTempHumid.setup();

    // Сохраненные настройки
    Serial.println("setup preferences.begin");
    preferences.begin(PREFS_START_GROW, true);
    uint32_t ts = preferences.getUInt(PREFS_KEY_TIMESTAMP, 0);
    if (DEBUG) {
        Serial.println((String) "[PREF GET] Start grow unixtime: " + ts);
    }
    startGrow = DateTime(ts);
    preferences.end();

    // поплавок, датчик воды
    pinMode(FLOAT_SENSOR_PIN, INPUT_PULLUP); // initialize the pushbutton pin as an input

    // RELAY PIN INIT
    pinMode(        PUMP_HIGH_PIN,  OUTPUT);
    digitalWrite(   PUMP_HIGH_PIN,  RELAY_OLD_OFF);
    pinMode(        LIGHT_PIN,      OUTPUT);
    digitalWrite(   LIGHT_PIN,      RELAY_NEW_OFF);
    pinMode(        BOX_HUMID_PIN,  OUTPUT);
    digitalWrite(   BOX_HUMID_PIN,  RELAY_NEW_OFF);
    pinMode(        BOX_VENT_PIN,   OUTPUT);
    digitalWrite(   BOX_VENT_PIN,   RELAY_NEW_OFF);
    Serial.println("\r\n---- ~ SETUP ----");
}

unsigned long loopStart;

void loop() {
    loopStart = millis();
    if (DEBUG) {
        Serial.println("\r\n-------------------");
        processConsoleCommand();
    }

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
        Serial.print((String) "Float: " + digitalRead(FLOAT_SENSOR_PIN));
    } else {
        if (digitalRead(FLOAT_SENSOR_PIN) == LOW) { // поплавок тонет, уровень высокий
            logToScreen("d14.txt", "HIGH");
        } else {
            logToScreen("d14.txt", "LOW");
        }
    }

    // -----------------
    // RELAY processing:
    // -----------------

    // Управляяем насосом высокого давления
    if (pumpHighTS_start > 0) {
        uint32_t dTS = xTime.nowTS - pumpHighTS_start;
        if (dTS >= timePumpHigh) {
            pumpHighTS_start = 0;
            pumpHighTS_end = xTime.now.unixtime();
            digitalWrite(PUMP_HIGH_PIN, RELAY_OLD_OFF);
            if (DEBUG) { logEvent("мотор высокого давления: выкл"); }
        }
    } else {
        // включаем мотор высокого давления
        uint32_t dTS = xTime.nowTS - pumpHighTS_end;
        if (dTS >= timeoutPumpHigh) {
            pumpHighTS_end = 0;
            pumpHighTS_start = xTime.now.unixtime();
            digitalWrite(PUMP_HIGH_PIN, RELAY_OLD_ON);
            if (DEBUG) { logEvent("мотор высокого давления: вкл."); }
        }
    }

    // Управляем светом
    uint32_t deltaSeconds = xTime.nowTS - startGrow.unixtime();
    uint32_t deltaDays = deltaSeconds / (24 * 60 * 60);
    if (DEBUG && VERBOSE) {
        logEvent((String) "V: deltaSeconds=" + deltaSeconds + " deltaDays=" + deltaDays);
    }
    auto addMinutes = deltaDays * lightDeltaMinutes;
    auto lightMinutes = lightMinHours * 60 + addMinutes;
    if (lightMinutes > lightMaxHours * 60) {
        lightMinutes = lightMaxHours * 60;
    }
    // последняя минута включенного света с начала дня
    auto lightEndMinute = lightTimeOn * 60 + lightMinutes;
    if (lightEndMinute >= 24 * 60) {
        lightEndMinute -= 24 * 60;
    }
    const auto lightStartMinute = lightTimeOn * 60;
    // текущая минута с начала дня
    const auto currMinute = xTime.now.hour() * 60 + xTime.now.minute();
    if (DEBUG && VERBOSE) {
        logEvent((String) "V: lightMinutes=" + lightMinutes + " lightStartMinute=" + lightStartMinute
                 + " lightEndMinute=" + lightEndMinute + " currMinute=" + currMinute + " isLightOn=" + isLightOn);
    }
    if (isLightOn) {
        if ((lightStartMinute < lightEndMinute && (currMinute < lightStartMinute || currMinute >= lightEndMinute))
            || (lightStartMinute > lightEndMinute && (currMinute < lightStartMinute && currMinute >= lightEndMinute))
                ) {
            digitalWrite(LIGHT_PIN, RELAY_NEW_OFF);
            isLightOn = false;
            if (DEBUG) { logEvent("свет: выкл."); }
        }
    } else {
        if ((lightStartMinute < lightEndMinute && (currMinute >= lightStartMinute && currMinute < lightEndMinute))
            || (lightStartMinute > lightEndMinute && (currMinute >= lightStartMinute || currMinute < lightEndMinute))
                ) {
            isLightOn = true;
            digitalWrite(LIGHT_PIN, RELAY_NEW_ON);
            if (DEBUG) { logEvent("свет: вкл."); }
        }
    }

    // Управление в БОКСЕ
    if (DEBUG && VERBOSE) {
        logEvent((String) "xTempHumid.boxHumid=" + xTempHumid.boxHumid + " isBoxVentOn=" + isBoxVentOn +
                 " isBoxHumidOn=" + isBoxHumidOn + " >Ok?=" + (xTempHumid.boxHumid > boxHumidOk)
                 + " >Min?=" + (xTempHumid.boxHumid > boxHumidMin) + " <Max?=" + (xTempHumid.boxHumid < boxHumidMax));
    }
    if (isBoxVentOn) {
        if (xTempHumid.boxHumid <= boxHumidOk) {
            isLightOn = false;
            digitalWrite(BOX_VENT_PIN, RELAY_NEW_OFF);
            if (DEBUG) { logEvent("БОКС вентилятор: выкл."); }
        }
    } else {
        if (xTempHumid.boxHumid >= boxHumidMax) {
            isLightOn = true;
            digitalWrite(BOX_VENT_PIN, RELAY_NEW_ON);
            if (DEBUG) { logEvent("БОКС вентилятор: вкл."); }
        }
    }
    if (isBoxHumidOn) {
        if (xTempHumid.boxHumid >= boxHumidOk) {
            isBoxHumidOn = false;
            digitalWrite(BOX_HUMID_PIN, RELAY_NEW_OFF);
            if (DEBUG) { logEvent("БОКС увлажнитель: выкл."); }
        }
    } else {
        if (xTempHumid.boxHumid <= boxHumidMin) {
            isBoxHumidOn = true;
            digitalWrite(BOX_HUMID_PIN, RELAY_NEW_ON);
            if (DEBUG) { logEvent("БОКС увлажнитель: вкл."); }
        }
    }

    if (DEBUG) {
        delay(LOOP_MS_DEBUG - (millis() - loopStart));
    } else {
        delay(min(max((int) (LOOP_MS_NORMAL - (millis() - loopStart)), 0), LOOP_MS_NORMAL));
    }
}

inline void processConsoleCommand() {
    char _char;
    char _string[] = "                      ";
    for (char &i : _string) {
        if (Serial.available()) {
            _char = (char) Serial.read();
            if (std::isdigit(_char) || std::isalpha(_char)) {
                if (VERBOSE) {
                    Serial.println((String) "OK CHAR = " + _char);
                }
                i = _char;
                continue;
            }
        }
    }
    if (VERBOSE) {
        Serial.println((String) "OK STRING = " + _string);
    }
    std::string str = _string;
    rtrim(str);
    ltrim(str);

    auto cmd = "datetime ";
    if (str.rfind(cmd, 0) == 0) {
        str.erase(0, strlen(cmd));
        if (str.length() == 13) {
            str = str.substr(0, 13);
            Serial.println((String) "\n[CMD SET DATE AND TIME]: " + str.data());
            DS3231 Clock;
            Clock.setClockMode(false);  // set to 24h - flase; 12h - true
            byte Temp1, Temp2;
            // Read Year first
            Temp1 = (byte) str[0] - 48;
            Temp2 = (byte) str[1] - 48;
            auto Year = Temp1 * 10 + Temp2;
            Clock.setYear(Year);
            // now month
            Temp1 = (byte) str[2] - 48;
            Temp2 = (byte) str[3] - 48;
            auto Month = Temp1 * 10 + Temp2;
            Clock.setMonth(Month);
            // now date
            Temp1 = (byte) str[4] - 48;
            Temp2 = (byte) str[5] - 48;
            auto Date = Temp1 * 10 + Temp2;
            Clock.setDate(Date);
            // now Day of Week
            auto DoW = (byte) str[6] - 48;
            Clock.setDoW(DoW);
            // now Hour
            Temp1 = (byte) str[7] - 48;
            Temp2 = (byte) str[8] - 48;
            auto Hour = Temp1 * 10 + Temp2;
            Clock.setHour(Hour);
            // now Minute
            Temp1 = (byte) str[9] - 48;
            Temp2 = (byte) str[10] - 48;
            auto Minute = Temp1 * 10 + Temp2;
            Clock.setMinute(Minute);
            // now Second
            Temp1 = (byte) str[11] - 48;
            Temp2 = (byte) str[12] - 48;
            auto Second = Temp1 * 10 + Temp2;
            Clock.setSecond(Second);
            // Test of alarm functions
            // set A1 to one minute past the time we just set the clock
            // on current day of week.
            Clock.setA1Time(DoW, Hour, Minute + 1, Second, 0x0, true,
                            false, false);
            // set A2 to two minutes past, on current day of month.
            Clock.setA2Time(Date, Hour, Minute + 2, 0x0, false, false,
                            false);
            // Turn on both alarms, with external interrupt
            Clock.turnOnAlarm(1);
            Clock.turnOnAlarm(2);
            Serial.println((String) "[CMD SET DATE AND TIME]: Date " + Year + "-" + Month + "-" + Date +
                           " DoW=" + DoW + " Time " + Hour + ":" + Minute + ":" + Second);
            Serial.println((String) "[CMD SET DATE AND TIME]: Timestamp " + RTClib::now().unixtime());
            Serial.println("Restart in 5 seconds...");
            delay(5000);
            ESP.restart();
        } else {
            Serial.println((String) "\n[CMD SET DATE AND TIME]: Canceled! "
                                    "Wrong payload (" + str.data() + "), (YYMMDDwHHMMSS) expected!");
        }
    }

    cmd = "startGrow ";
    if (str.rfind(cmd, 0) == 0) {
        str.erase(0, strlen(cmd));
        if (str.length() >= 8) {
            Serial.println((String) "\n[CMD SET START GROW]: " + str.data());
            auto year = str.substr(0, 2);
            auto month = str.substr(2, 2);
            auto day = str.substr(4, 2);
            auto hour = str.substr(6, 2);
            Serial.println((String) "[CMD SET START GROW]: Date: " +
                           year.data() + "-" + month.data() + "-" + day.data() + " Hour: " + hour.data());
            auto dt = DateTime(atoi(year.data()), atoi(month.data()), atoi(day.data()), atoi(hour.data()));
            Serial.println((String) "[CMD SET START GROW]: DateTime: " + dt.unixtime());
            preferences.begin(PREFS_START_GROW, false);
            preferences.remove(PREFS_KEY_TIMESTAMP);
            preferences.putUInt(PREFS_KEY_TIMESTAMP, dt.unixtime());
            Serial.println("Preferences saved. Don't turn off, restart in 5 seconds...");
            preferences.end();
            delay(5000);
            ESP.restart();
        }
    }
}