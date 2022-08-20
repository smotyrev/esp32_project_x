// include library to read and write from flash memory
#include <EEPROM.h>

#include "main.h"
#include "x_time.h"
#include "x_temperature_humidity.h"

x_time xTime;                       // Текущее время
x_temperature_humidity xTempHumid;  // 

int xPumpProgram;                   // Текущая программа управления насосом(ами) для орошения
bool xPumpProgramForce = false;     // Принудительно включить программу орашения

DateTime startGrow;
Preferences preferences;

float phVoltage,phValue,phTemperature = 25;
DFRobot_ESP_PH ph;

GravityTDS gravityTds;
float tdsValue = 0;

void setup() {
    Serial.begin(9600);
    Serial.println("\r\n---- ~ SETUP ----");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, (uint32_t) 4000);

    xTime.setup();
    xTempHumid.setup();

    // Сохраненные настройки
    // Datetime:
    Serial.println("setup preferences.begin");
    preferences.begin(PREFS_START_GROW, true);
    uint32_t ts = preferences.getUInt(PREFS_KEY_TIMESTAMP, 0);
    if (DEBUG) {
        Serial.println((String) "[PREF GET] Start grow unixtime: " + ts);
    }
    startGrow = DateTime(ts);
    preferences.end();
    // Program pump:
    preferences.begin(PREFS_PROGRAM_PUPM, true);
    xPumpProgram = preferences.getInt(PREFS_KEY_PP_VAL, 2);
    preferences.end();

    // GreenPonics PH
    EEPROM.begin(32); //needed to permit storage of calibration value in eeprom
    ph.begin();

 /***********Notice and Trouble shooting***************
 1. This code is tested on Arduino Uno with Arduino IDE 1.0.5 r2 and 1.8.2.
 2. Calibration CMD:
     enter -> enter the calibration mode
     cal:tds value -> calibrate with the known tds value(25^c). e.g.cal:707
     exit -> save the parameters and exit the calibration mode
 ****************************************************/
    gravityTds.setPin(TDS_PIN);
    gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
    gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
    gravityTds.begin();  //initialization

    // поплавок, датчик воды
    pinMode(FLOAT_SENSOR_PIN, INPUT_PULLUP); // initialize the pushbutton pin as an input
    pinMode(FLOAT_SENSOR2_PIN, INPUT_PULLUP);

    // RELAY PIN INIT
    pinMode(        PUMP_HIGH_PIN,  OUTPUT);
    digitalWrite(   PUMP_HIGH_PIN,  SRELAY_OFF);
    pinMode(        PUMP_HIGH_PIN_HV,  OUTPUT);
    digitalWrite(   PUMP_HIGH_PIN_HV,  RELAY_OFF);
    pinMode(        PUMP_HIGH2_PIN,  OUTPUT);
    digitalWrite(   PUMP_HIGH2_PIN,  SRELAY_OFF);
    pinMode(        LIGHT_PIN,      OUTPUT);
    digitalWrite(   LIGHT_PIN,      RELAY_OFF);
    pinMode(        BOX_HUMID_PIN,  OUTPUT);
    digitalWrite(   BOX_HUMID_PIN,  RELAY_OFF);
    pinMode(        BOX_VENT_PIN,   OUTPUT);
    digitalWrite(   BOX_VENT_PIN,   RELAY_OFF);
    Serial.println("\r\n---- ~ SETUP ----");

    // устанавливаем время выключения насоса2, это нужно чтобы запустить цикл для Программы 2
    pumpHigh2TS_end = 1;
}

unsigned long loopStart;
String d14_old = "";
int d16_old;
float d12_old;
void loop() {
    loopStart = millis();
    
    // processConsoleCommand();
    loopPh();

    // if (DEBUG) {
	//     Serial.println("\r\n-------------------");
    //     ph.calibration(phVoltage, phTemperature); // calibration process by Serail CMD
    // }

    // печатаем время
    xTime.loop();

    // печатаем температуру и влвжность
    xTempHumid.loop();

    // печатаем - поплавок, датчик воды
    if (DEBUG) {
        Serial.print((String) "Float: " + digitalRead(FLOAT_SENSOR_PIN));
    } else {
        if (digitalRead(FLOAT_SENSOR_PIN) == LOW) { // поплавок тонет, уровень высокий
            if (d14_old != "HIGH") {
                d14_old = "HIGH";
                logToScreen("d14.txt", "HIGH");
            }
        } else {
            if (d14_old != "LOW") {
                d14_old = "LOW";
                logToScreen("d14.txt", "LOW");
            }
        }
    }

    // -----------------
    // RELAY processing:
    // -----------------

    // Управляяем насосом\насосами высокого давления
    if (xPumpProgramForce) {
        // Принудительная подача раствора, согласно программе
        if (xPumpProgram == 1) {
            if (digitalRead(PUMP_HIGH_PIN) == SRELAY_OFF) {
                digitalWrite(PUMP_HIGH_PIN, SRELAY_ON);
            }
            if (digitalRead(PUMP_HIGH_PIN_HV) == RELAY_OFF) {
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_ON);
            }
        } else if (xPumpProgram == 2) {
            if (digitalRead(PUMP_HIGH_PIN_HV) == RELAY_ON) {
                // если откачка включена, то выключаем откачку
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_OFF);
            }
            if (digitalRead(FLOAT_SENSOR2_PIN) == LOW) {
                if (digitalRead(PUMP_HIGH_PIN) == SRELAY_OFF) {
                    // если поплавок не затоплен и выключен насос накачки воды, 
                    // то включаем насос (корни без воды)
                    digitalWrite(PUMP_HIGH_PIN, SRELAY_ON);
                }
            } else if (digitalRead(PUMP_HIGH_PIN) == SRELAY_ON) {
                // если поплавок затоплен и включен насос накачки воды, 
                // то выключаем насос (корни в воде)
                digitalWrite(PUMP_HIGH_PIN, SRELAY_OFF);
            }
        }
    } else if (xPumpProgram == 1) {
        // Программа 1: Вклюючается периодически на несколько секунд.
        //              Для распыления через форсунки
        if (pumpHighTS_start > 0) {
            uint32_t dTS = xTime.nowTS - pumpHighTS_start;
            if (dTS >= timePumpHigh) {
                pumpHighTS_start = 0;
                pumpHighTS_end = xTime.now.unixtime();
                digitalWrite(PUMP_HIGH_PIN, SRELAY_OFF);
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_OFF);
                if (DEBUG) { logEvent("мотор высокого давления: выкл"); }
            }
        } else {
            // включаем мотор высокого давления
            uint32_t dTS = xTime.nowTS - pumpHighTS_end;
            if (dTS >= timeoutPumpHigh) {
                pumpHighTS_end = 0;
                pumpHighTS_start = xTime.now.unixtime();
                digitalWrite(PUMP_HIGH_PIN, SRELAY_ON);
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_ON);
                if (DEBUG) { logEvent("мотор высокого давления: вкл."); }
            }
        }
    } else if (xPumpProgram == 2) {
        // Программа 2: Включаем периодически, накачиваем раствор, ждем 20 секунд, откачиваем раствор.
        //              Для погружения корней в раствор

        // если насос1 накачивает раствор, то проверяем: затоплен ли поплавок2
        if (digitalRead(PUMP_HIGH_PIN) == SRELAY_ON) {
            // если поплавок2 затонул, уровень воды высокий
            if (digitalRead(FLOAT_SENSOR2_PIN) == LOW) {
                // отключаем насос1
                digitalWrite(PUMP_HIGH_PIN, SRELAY_OFF);
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_OFF);
                if (DEBUG) { logEvent("мотор высокого давления1: выкл"); }
                // запоминаем время выключения насоса1, начинается стадия ожидания пока раствор заполнен
                pumpHighTS_end = xTime.now.unixtime();
            }
        } else {
            // если идет стадия ожидания пока раствор заполнен
            if (pumpHighTS_end > 0) {
                // вычисляем сколько секунд прошло после выключения насоса1 накачивающего раствор
                uint32_t dTS = xTime.nowTS - pumpHighTS_end;
                // если прошло 20 секунд, включаем насос2 для откачки воды
                if (dTS > timeoutPumpHigh2) {
                    // обнуляем время выключения насоса1, начинается стадия откачки
                    pumpHighTS_end = 0;
                    // включаем насос2, для откачки воды
                    digitalWrite(PUMP_HIGH2_PIN, SRELAY_ON);
                    if (DEBUG) { logEvent("мотор высокого давления2: вкл"); }
                    // запоминаем время включения насоса2
                    pumpHigh2TS_start = xTime.now.unixtime();
                }
            }

            // если идет стадия откачки раствора
            if (pumpHigh2TS_start > 0) {
                // вычисляем сколько секунд прошло после включения насоса2
                uint32_t dTS = xTime.nowTS - pumpHigh2TS_start;
                // если прошла минута работы насоса2, отключаем его
                if (dTS > timePumpHigh2) {
                    // отключаем насос2
                    digitalWrite(PUMP_HIGH2_PIN, SRELAY_OFF);
                    if (DEBUG) { logEvent("мотор высокого давления2: выкл"); }
                    // обнуляем время старата насоса2
                    pumpHigh2TS_start = 0;
                    // запоминаем время выключения насоса2, начинается новый цикл
                    pumpHigh2TS_end = xTime.now.unixtime();
                }
            }

            // если начат новый цикл
            if (pumpHigh2TS_end > 0) {
                // вычисляем сколько секунд прошло после выключения насоса2
                uint32_t dTS = xTime.nowTS - pumpHigh2TS_end;
                // если время простоя вышло, и поплавок2 не затоплен (доп проверка)
                if (dTS > timeoutPumpHigh && digitalRead(FLOAT_SENSOR2_PIN) == HIGH) {
                    // включаем насос1, накачиваем раствор
                    digitalWrite(PUMP_HIGH_PIN, SRELAY_ON);
                    digitalWrite(PUMP_HIGH_PIN_HV, RELAY_ON);
                    if (DEBUG) { logEvent("мотор высокого давления1: вкл."); }
                    // запоминаем время включения насоса1
                    pumpHighTS_start = xTime.now.unixtime();
                    // обнуляем время выключения насоса1
                    pumpHigh2TS_end = 0;
                }
            }
        }
    }
    if (d16_old != xPumpProgram) {
        d16_old = xPumpProgram;
        logToScreen("d16.txt", (String) "Program " + xPumpProgram);
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
            isLightOn = false;
            digitalWrite(LIGHT_PIN, RELAY_OFF);
            if (DEBUG) { logEvent("свет: выкл."); }
        }
    } else {
        if ((lightStartMinute < lightEndMinute && (currMinute >= lightStartMinute && currMinute < lightEndMinute))
            || (lightStartMinute > lightEndMinute && (currMinute >= lightStartMinute || currMinute < lightEndMinute))
                ) {
            isLightOn = true;
            digitalWrite(LIGHT_PIN, RELAY_ON);
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
            isBoxVentOn = false;
            digitalWrite(BOX_VENT_PIN, RELAY_OFF);
            if (DEBUG) { logEvent("БОКС вентилятор: выкл."); }
        }
    } else {
        if (xTempHumid.boxHumid >= boxHumidMax) {
            isBoxVentOn = true;
            digitalWrite(BOX_VENT_PIN, RELAY_ON);
            if (DEBUG) { logEvent("БОКС вентилятор: вкл."); }
        }
    }
    if (isBoxHumidOn) {
        if (xTempHumid.boxHumid >= boxHumidOk) {
            isBoxHumidOn = false;
            digitalWrite(BOX_HUMID_PIN, RELAY_OFF);
            if (DEBUG) { logEvent("БОКС увлажнитель: выкл."); }
        }
    } else {
        if (xTempHumid.boxHumid <= boxHumidMin) {
            isBoxHumidOn = true;
            digitalWrite(BOX_HUMID_PIN, RELAY_ON);
            if (DEBUG) { logEvent("БОКС увлажнитель: вкл."); }
        }
    }

    if (DEBUG) {
        delay(LOOP_MS_DEBUG - (millis() - loopStart));
    }
}

inline void processConsoleCommand() {
    if (!Serial.available()) {
        return;
    }

    std::string str;
    char _char;
    do {
        _char = (char) Serial.read();
        if (std::isdigit(_char) || std::isalpha(_char) || std::ispunct(_char)) {
            if (VERBOSE) {
                Serial.println((String) "OK CHAR = " + _char);
            }
            str.append(1, _char);
            continue;
        }
    } while (Serial.available());

    if (VERBOSE) {
        Serial.println((String) "ConsoleCommand: " + str.c_str());
    }

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

    cmd = "testCommand";
    if (str.rfind(cmd, 0) == 0) {
        str.erase(0, strlen(cmd));
        Serial.println("\n\tOK!\n");
    }

    cmd = "PN+";                // Кнопка выбора программы, смена на след. прогр-му
    if (str.rfind(cmd, 0) == 0) {
        str.erase(0, strlen(cmd));
        if (xPumpProgram >= MAX_PROGRAMS) {
            xPumpProgram = 1;
        } else {
            xPumpProgram++;
        }
        Serial.print("\nSave preference, xPumpProgram=" + xPumpProgram);
        preferences.begin(PREFS_PROGRAM_PUPM, false);
        preferences.remove(PREFS_KEY_PP_VAL);
        preferences.putInt(PREFS_KEY_PP_VAL, xPumpProgram);
        preferences.end();
        Serial.println("\tOK!");
    }

    // Кнопка включения насосов принудительно, согласно выбраной программе
    cmd = "ForceNON";
    if (str.rfind(cmd, 0) == 0) {
        str.erase(0, strlen(cmd));
        Serial.println("\nPreferences saved, xPumpProgramForce=true");
        xPumpProgramForce = true;
    }
    cmd = "ForceNOFF";
    if (str.rfind(cmd, 0) == 0) {
        str.erase(0, strlen(cmd));
        Serial.println("\nPreferences saved, xPumpProgramForce=false");
        xPumpProgramForce = false;
    }

    if (VERBOSE && str.length() > 0) {
        Serial.print("\nUnknown command: ");
        Serial.println(str.c_str());
    }
}

inline void loopPh() {
    static unsigned long timepoint = millis();
	if (millis() - timepoint > 1000U) //time interval: 1s
	{
		timepoint = millis();
		//voltage = rawPinValue / esp32ADC * esp32Vin
		phVoltage = analogRead(PH_PIN) / ESPADC * ESPVOLTAGE; // read the voltage
		phValue = ph.readPH(phVoltage, phTemperature); // convert voltage to pH with temperature compensation
		//temperature = readTemperature();  // read your temperature sensor to execute temperature compensation
        if (DEBUG) {
            Serial.print("PH voltage:");
            Serial.print(phVoltage, 4);
            Serial.print(" temperature:");
            Serial.print(phTemperature, 1);
            Serial.print(" pH:");
            Serial.println(phValue, 4);
        } else {
            if (d12_old != phValue) {
                d12_old = phValue;
                logToScreen("d12.txt", (String) phValue);
            }
        }
	}

    gravityTds.setTemperature(phTemperature);  // set the temperature and execute temperature compensation
    gravityTds.update();  //sample and calculate
    tdsValue = gravityTds.getTdsValue();  // then get the value
    if (DEBUG) {
        Serial.println("---");
        Serial.print(tdsValue,0);
        Serial.println("ppm");
    }
}