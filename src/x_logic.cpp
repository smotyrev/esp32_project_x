// include library to read and write from flash memory
#include <EEPROM.h>

#include "x_logic.h"

// насос высокого давления
uint32_t pumpHighTS_start = 0;
uint32_t pumpHighTS_end = 0;

// насос высокого давления 2
uint32_t pumpHigh2TS_start = 0;
uint32_t pumpHigh2TS_end = 0;

// Свет
bool isLightOn = false;

// БОКС вентилятор
bool isBoxVentOn = false;

// БОКС увлажнитель
bool isBoxHumidOn = false;

float phVoltage,phValue,phTemperature = 25;
DFRobot_ESP_PH ph;

GravityTDS gravityTds;
float tdsValue = 0;

int pumpProgram = 1;                // Текущая программа управления насосом(ами) для орошения
bool pumpProgramForce = false;      // Принудительное включение программы орашения
bool pumpProgramOFF = false;        // Принудительно выключение орашения

int lightProgram = 1;               // Текущая программа управления светом
int lightForce = LightForce::NoForce;
int lightForceOld = lightForce;

void x_logic::setup(main_data &data) {
    mData = &data;
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

    // поплавок, датчик воды
    pinMode(FLOAT_SENSOR_PIN, INPUT_PULLUP); // initialize the pushbutton pin as an input
    pinMode(FLOAT_SENSOR2_PIN, INPUT_PULLUP);

    mData->preferences.begin(PREFS_START_GROW, true);
    uint32_t ts = mData->preferences.getUInt(PREFS_KEY_TIMESTAMP, 0);
    if (DEBUG) {
        Serial.println((String) "[PREF GET] Start grow unixtime: " + ts);
    }
    mData->startGrow = DateTime(ts);
    mData->preferences.end();
    // Program pump:
    mData->preferences.begin(PREFS_PROGRAM_PUPM, true);
    pumpProgram = mData->preferences.getInt(PREFS_KEY_PP_VAL, 2);
    Serial.println((String) "Pump Program: " + pumpProgram);
    mData->preferences.end();

    mData->preferences.begin(PREFS_LIGHT, true);
    lightProgram = mData->preferences.getInt(PREFS_KEY_LI_VAL, 1);
    Serial.println((String) "Light Program: " + lightProgram);
    mData->preferences.end();

    // GreenPonics PH
    EEPROM.begin(32); //needed to permit storage of calibration value in eeprom
    ph.begin();

    /***********Notice and Trouble shooting***************
     * 1. This code is tested on Arduino Uno with Arduino IDE 1.0.5 r2 and 1.8.2.
     * 2. Calibration CMD:
     *      enter -> enter the calibration mode
     *      cal:tds value -> calibrate with the known tds value(25^c). e.g.cal:707
     *      exit -> save the parameters and exit the calibration mode
     */
    gravityTds.setPin(TDS_PIN);
    gravityTds.setAref(5.0);  //reference voltage on ADC, default 5.0V on Arduino UNO
    gravityTds.setAdcRange(1024);  //1024 for 10bit ADC;4096 for 12bit ADC
    gravityTds.begin();  //initialization

    // устанавливаем время выключения насоса2, это нужно чтобы запустить цикл для Программы 2
    pumpHigh2TS_end = 1;

    logToScreen("r0.val", "1");
    logToScreen("r1.val", "0");
    logToScreen("r2.val", "0");
}

int d14_old = -1;
int d16_old = -1;
int d05_old = -1;
float d12_old = 0;
void x_logic::loop(bool forceDataSend) {
    loopPhAndPpm(forceDataSend);
    
    // -----------------
    // поплавок, датчик воды:
    // -----------------

    if (digitalRead(FLOAT_SENSOR_PIN) == LOW) { // поплавок тонет, уровень высокий
        if (forceDataSend || d14_old != HIGH) {
            d14_old = HIGH;
            logToScreen("d14.txt", "HIGH");
        }
    } else {
        if (forceDataSend || d14_old != LOW) {
            d14_old = LOW;
            logToScreen("d14.txt", "LOW");
        }
    }

    // -----------------
    // RELAY processing:
    // -----------------

    // Управляяем насосом питающего раствора
    if (pumpProgramOFF) {
        // Принудительное отключение насосов 
        digitalWrite(PUMP_HIGH_PIN, SRELAY_OFF);
        digitalWrite(PUMP_HIGH_PIN_HV, RELAY_OFF);        
    } else if (pumpProgramForce) {
        // Принудительная подача раствора, согласно программе
        if (pumpProgram == 1) {
            if (digitalRead(PUMP_HIGH_PIN) == SRELAY_OFF) {
                digitalWrite(PUMP_HIGH_PIN, SRELAY_ON);
            }
            if (digitalRead(PUMP_HIGH_PIN_HV) == RELAY_OFF) {
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_ON);
            }
        } else if (pumpProgram == 2) {
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
    } else if (pumpProgram == 1) {
        // Программа 1: Вклюючается периодически на несколько секунд.
        //              Для распыления через форсунки
        if (pumpHighTS_start > 0) {
            uint32_t dTS = mData->nowTS - pumpHighTS_start;
            if (dTS >= timePumpHigh) {
                pumpHighTS_start = 0;
                pumpHighTS_end = mData->nowTS;
                digitalWrite(PUMP_HIGH_PIN, SRELAY_OFF);
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_OFF);
                if (DEBUG) { logEvent("мотор высокого давления: выкл"); }
            }
        } else {
            // включаем мотор высокого давления
            uint32_t dTS = mData->nowTS - pumpHighTS_end;
            if (dTS >= timeoutPumpHigh) {
                pumpHighTS_end = 0;
                pumpHighTS_start = mData->nowTS;
                digitalWrite(PUMP_HIGH_PIN, SRELAY_ON);
                digitalWrite(PUMP_HIGH_PIN_HV, RELAY_ON);
                if (DEBUG) { logEvent("мотор высокого давления: вкл."); }
            }
        }
    } else if (pumpProgram == 2) {
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
                pumpHighTS_end = mData->nowTS;
            }
        } else {
            // если идет стадия ожидания пока раствор заполнен
            if (pumpHighTS_end > 0) {
                // вычисляем сколько секунд прошло после выключения насоса1 накачивающего раствор
                uint32_t dTS = mData->nowTS - pumpHighTS_end;
                // если прошло 20 секунд, включаем насос2 для откачки воды
                if (dTS > timeoutPumpHigh2) {
                    // обнуляем время выключения насоса1, начинается стадия откачки
                    pumpHighTS_end = 0;
                    // включаем насос2, для откачки воды
                    digitalWrite(PUMP_HIGH2_PIN, SRELAY_ON);
                    if (DEBUG) { logEvent("мотор высокого давления2: вкл"); }
                    // запоминаем время включения насоса2
                    pumpHigh2TS_start = mData->nowTS;
                }
            }

            // если идет стадия откачки раствора
            if (pumpHigh2TS_start > 0) {
                // вычисляем сколько секунд прошло после включения насоса2
                uint32_t dTS = mData->nowTS - pumpHigh2TS_start;
                // если прошла минута работы насоса2, отключаем его
                if (dTS > timePumpHigh2) {
                    // отключаем насос2
                    digitalWrite(PUMP_HIGH2_PIN, SRELAY_OFF);
                    if (DEBUG) { logEvent("мотор высокого давления2: выкл"); }
                    // обнуляем время старата насоса2
                    pumpHigh2TS_start = 0;
                    // запоминаем время выключения насоса2, начинается новый цикл
                    pumpHigh2TS_end = mData->nowTS;
                }
            }

            // если начат новый цикл
            if (pumpHigh2TS_end > 0) {
                // вычисляем сколько секунд прошло после выключения насоса2
                uint32_t dTS = mData->nowTS - pumpHigh2TS_end;
                // если время простоя вышло, и поплавок2 не затоплен (доп проверка)
                if (dTS > timeoutPumpHigh && digitalRead(FLOAT_SENSOR2_PIN) == HIGH) {
                    // включаем насос1, накачиваем раствор
                    digitalWrite(PUMP_HIGH_PIN, SRELAY_ON);
                    digitalWrite(PUMP_HIGH_PIN_HV, RELAY_ON);
                    if (DEBUG) { logEvent("мотор высокого давления1: вкл."); }
                    // запоминаем время включения насоса1
                    pumpHighTS_start = mData->nowTS;
                    // обнуляем время выключения насоса1
                    pumpHigh2TS_end = 0;
                }
            }
        }
    }
    if (forceDataSend || d16_old != pumpProgram) {
        d16_old = pumpProgram;
        logToScreen("d16.txt", "Program " + std::to_string(pumpProgram));
    }

    // Управляем светом
    uint32_t deltaSeconds = mData->nowTS - mData->startGrow.unixtime();
    if (lightForce == LightForce::Off) {
        // Принудительное выключение света
        if (isLightOn == true) {
            isLightOn = false;
            logToScreen("d07.txt", "svet OFF");
            digitalWrite(LIGHT_PIN, RELAY_OFF);
        }
    } else if (lightForce == LightForce::On) {
        // Принудительное включение света
        if (isLightOn == false) {
            isLightOn = true;
            digitalWrite(LIGHT_PIN, RELAY_ON);
            logToScreen("d07.txt", "svet ON");
            if (DEBUG) { logEvent("Svet OF"); }
        }
    } else if (lightProgram == 1 || lightProgram == 2 || lightProgram == 3) {
        // Программа 1: 12x12
        // Программа 2: 16x8
        // Программа 3: 20x4
        int lightHourEnd = 0;
        if (lightProgram == 1) {
            lightHourEnd = 12;
        } else if (lightProgram == 2) {
            lightHourEnd = 16;
        } else if (lightProgram == 3) {
            lightHourEnd = 20;
        }
        auto deltaHours = deltaSeconds / 60 / 60;
        auto numberOfDays = deltaHours / 24;
        auto lastDayHours = deltaHours - numberOfDays * 24;
        if (VERBOSE && DEBUG) {
            logEvent((String) "now: " + mData->nowTS + "; deltaSeconds: " +  deltaSeconds 
            + "; lastDayHours: " + lastDayHours + "; lightHourEnd: "+ lightHourEnd);
        }
        if (lastDayHours < lightHourEnd) {
            if (isLightOn == false) {
                isLightOn = true;
                digitalWrite(LIGHT_PIN, RELAY_ON);
                if (DEBUG) { logEvent("свет: вкл."); }
            }
        } else {
            if (isLightOn == true) {
                isLightOn = false;
                digitalWrite(LIGHT_PIN, RELAY_OFF);
                if (DEBUG) { logEvent("свет: выкл."); }
            }
        }
    } else if (lightProgram == 4) {
        // Программа 4: Постепенное нарастане света
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
        const auto currMinute = mData->nowHour * 60 + mData->nowMinute;
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
    }
    if (forceDataSend || d05_old != lightProgram) {
        d05_old = lightProgram;
        logToScreen("d05.txt", "Program " + std::to_string(lightProgram));
    }
    if (forceDataSend || lightForceOld != lightForce) {
        lightForceOld = lightForce;
        switch (lightForce) {
            case LightForce::Off:
                logToScreen("r0.val", 0);logToScreen("r1.val", 0);logToScreen("r2.val", 1);
                break;
            case LightForce::On:
                logToScreen("r0.val", 0);logToScreen("r1.val", 1);logToScreen("r2.val", 0);
                break;
            default:
                logToScreen("r0.val", 1);logToScreen("r1.val", 0);logToScreen("r2.val", 0);
                break;
        }
    }

    // Управление в БОКСЕ
    if (DEBUG && VERBOSE) {
        logEvent((String) "xTempHumid.boxHumid=" + mData->boxHumid + " isBoxVentOn=" + isBoxVentOn +
                 " isBoxHumidOn=" + isBoxHumidOn
                 + " >Min?=" + (mData->boxHumid > mData->boxHumidMin) + " <Max?=" + (mData->boxHumid < mData->boxHumidMax));
    }
    if (isBoxVentOn) {
        if (mData->boxHumid <= (mData->boxHumidMax - 5.0)) {
            isBoxVentOn = false;
            digitalWrite(BOX_VENT_PIN, RELAY_OFF);
            if (DEBUG) { logEvent("БОКС вентилятор: выкл."); }
        }
    } else {
        if (mData->boxHumid >= mData->boxHumidMax) {
            isBoxVentOn = true;
            digitalWrite(BOX_VENT_PIN, RELAY_ON);
            if (DEBUG) { logEvent("БОКС вентилятор: вкл."); }
        }
    }
    if (isBoxHumidOn) {
        if (mData->boxHumid >= (mData->boxHumidMin + 5.0)) {
            isBoxHumidOn = false;
            digitalWrite(BOX_HUMID_PIN, RELAY_OFF);
            if (DEBUG) { logEvent("БОКС увлажнитель: выкл."); }
        }
    } else {
        if (mData->boxHumid <= mData->boxHumidMin) {
            isBoxHumidOn = true;
            digitalWrite(BOX_HUMID_PIN, RELAY_ON);
            if (DEBUG) { logEvent("БОКС увлажнитель: вкл."); }
        }
    }
}

bool x_logic::processConsoleCommand(std::string &cmd) {
    // Serial.println((String) "\n\t CMD [" + ccmd + "]");

    // Кнопка выбора программы, смена на след. прогр-му для насоса
    if (cmd.rfind("PN+", 0) == 0) {
        if (pumpProgram >= MAX_PUMP_PROGRAMS) {
            pumpProgram = 1;
        } else {
            pumpProgram++;
        }
        logEvent("\nSave preference, pumpProgram=" + pumpProgram);
        mData->preferences.begin(PREFS_PROGRAM_PUPM, false);
        mData->preferences.remove(PREFS_KEY_PP_VAL);
        mData->preferences.putInt(PREFS_KEY_PP_VAL, pumpProgram);
        mData->preferences.end();
        Serial.println("\tOK!");
        logToScreen("d17.txt", "switching programm ok");
        return true;
    }

    // Кнопка выбора программы, смена на след. прогр-му для света
    if (cmd.rfind("PS+", 0) == 0) {
        if (lightProgram >= MAX_LIGHT_PROGRAMS) {
            lightProgram = 1;
        } else {
            lightProgram++;
        }
        logEvent("\nSave preference, lightProgram=" + lightProgram);
        mData->preferences.begin(PREFS_LIGHT, false);
        mData->preferences.remove(PREFS_KEY_LI_VAL);
        mData->preferences.putInt(PREFS_KEY_LI_VAL, lightProgram);
        mData->preferences.end();
        Serial.println("\tOK!");
        logToScreen("d06.txt", "switching programm ok");
        return true;
    }

    // Кнопка включения насоса принудительно, согласно выбраной программе
    if (cmd.rfind("ForceNON", 0) == 0) {
        Serial.println("\nPreferences saved, xPumpProgramForce=true");
        logToScreen("d17.txt", "The pump is turned on forcibly, according to a given program.");
        pumpProgramForce = true;
        return true;
    }
    // Кнопка выключения насосов принудительно 
    if (cmd.rfind("ForceNoff", 0) == 0) {
        Serial.println("\nPreferences saved, xPumpProgramOFF=true");
        logToScreen("d17.txt", "Рump off");
        pumpProgramOFF = true;
        return true;
    }
    // Кнопка работы насосов по программе (FORCE)
    if (cmd.rfind("ForceNOFF", 0) == 0) {
        Serial.println("\nPreferences saved, xPumpProgramForce=false");
        logToScreen("d17.txt", "The pump turns on according to the set program.");
        pumpProgramForce = false;
        return true;
    }
    // Кнопка работы насосов по программе (OFF)
    if (cmd.rfind("ForceNoff", 0) == 0) {
        Serial.println("\nPreferences saved, xPumpProgramOFF=false");
        logToScreen("d17.txt", "The pump turns on according to the set program.");
        pumpProgramOFF = false;
        return true;
    }


    // Кнопка света
    if (cmd.rfind("NoForceS", 0) == 0) {
        Serial.println("\nPreferences saved, xlightProgramForceOn=true");
        logToScreen("d06.txt", "The light is turned by a given program.");
        lightForce = LightForce::NoForce;
        return true;
    }
    if (cmd.rfind("ForceSON", 0) == 0) {
        Serial.println("\nPreferences saved, xlightProgramForceOn=true");
        logToScreen("d06.txt", "The light on (force)");
        lightForce = LightForce::On;
        return true;
    }
    // Кнопка выключения света
    if (cmd.rfind("ForceSOff", 0) == 0) {
        Serial.println("\nPreferences saved, xlightProgramOFF=true");
        logToScreen("d06.txt", "The light off (force)");
        lightForce = LightForce::Off;
        return true;
    }
    return false;
}

void x_logic::loopPhAndPpm(bool forceDataSend) {
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
        }
        if (forceDataSend || d12_old != phValue) {
            d12_old = phValue;
            logToScreen("d12.txt", std::to_string(phValue));
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