#include "x_temperature_humidity.h"

DHT_Unified dht(DHT_PIN, DHT_TYPE);

DFRobot_SHT20 sht20;
uint32_t dhtDelayMS;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWireDS18B20(DS18B20_PIN);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensorDS18B20(&oneWireDS18B20);
float temperatureDS18B20 = 0.0;

void x_temperature_humidity::setup(main_data &data) {
    mData = &data;
    dht.begin();
    Serial.println("Modified Temperature, Humidity Serial Monitor Example");
    sensor_t dhtSensor;
    dht.temperature().getSensor(&dhtSensor);
    Serial.println("------------------------------------");
    Serial.println("Temperature");
    Serial.println((String) "Sensor:       " + dhtSensor.name);
    Serial.println((String) "Driver Ver:   " + dhtSensor.version);
    Serial.println((String) "Unique ID:    " + dhtSensor.sensor_id);
    Serial.println((String) "Max Value:    " + dhtSensor.max_value + " *C");
    Serial.println((String) "Min Value:    " + dhtSensor.min_value + " *C");
    Serial.println((String) "Resolution:   " + dhtSensor.resolution + " *C");
    Serial.println((String) "Delay:        " + (dhtSensor.min_delay / 1000));
    Serial.println("------------------------------------");
    dht.humidity().getSensor(&dhtSensor);
    Serial.println("------------------------------------");
    Serial.println("Humidity");
    Serial.println((String) "Sensor:       " + dhtSensor.name);
    Serial.println((String) "Driver Ver:   " + dhtSensor.version);
    Serial.println((String) "Unique ID:    " + dhtSensor.sensor_id);
    Serial.println((String) "Max Value:    " + dhtSensor.max_value + "%");
    Serial.println((String) "Min Value:    " + dhtSensor.min_value + "%");
    Serial.println((String) "Resolution:   " + dhtSensor.resolution + "%");
    Serial.println((String) "Delay:        " + (dhtSensor.min_delay / 1000));
    Serial.println("------------------------------------");
    dhtDelayMS = dhtSensor.min_delay / 1000;

    Serial.println("SHT20 Init!");
    sht20.initSHT20();                                  // Init SHT20 Sensor
    delay(100);
    sht20.checkSHT20();                                 // Check SHT20 Sensor

    // Start the DS18B20 sensor
    sensorDS18B20.begin();
    sensorDS18B20.setWaitForConversion(false);
    sensorDS18B20.requestTemperatures();

    mData->preferences.begin(PREFS_BOX, true);
    mData->boxHumidMax = mData->preferences.getFloat(PREFS_KEY_BOX_HUMID_MAX, mData->boxHumidMax);
    if (DEBUG) {
        Serial.println((String) "[PREF GET] Box Humid Ok: " + mData->boxHumidMax);
    }
    mData->preferences.end();
}

float d01_old = 0;
float d02_old = 0;
float d31_old = 0;
float d32_old = 0;
float d11_old = 0;
void x_temperature_humidity::loop(bool forceDataSend) {
    if (DEBUG) {
        Serial.print("Temperature & Humidity:");
    }

    // печатаем температуру и влажность (DHT)
    sensors_event_t dhtVal;

    dht.temperature().getEvent(&dhtVal);
    if (DEBUG) {
        Serial.print((String) " | DHT22: " + dhtVal.temperature + "C, ");
    }
    if (forceDataSend || !isnan(dhtVal.temperature) && d01_old != dhtVal.temperature) {
        d01_old = dhtVal.temperature;
        logToScreen("d01.txt", std::to_string(dhtVal.temperature) + " C");
        // logToScreen("t01.txt", std::to_string(mData->boxHumidMin()) + "-" +  std::to_string(mData->boxHumidMax()));
    }

    dht.humidity().getEvent(&dhtVal);
    mData->boxHumid = dhtVal.relative_humidity;
    if (DEBUG) {
        Serial.print((String) dhtVal.relative_humidity + "%");
    }
    if (forceDataSend || !isnan(dhtVal.relative_humidity) && d02_old != dhtVal.relative_humidity) {
        d02_old = dhtVal.relative_humidity;
        logToScreen("d02.txt", std::to_string(dhtVal.relative_humidity)  + " %");
    }

    auto Temperature = sht20.readTemperature();
    auto Humidity = sht20.readHumidity();
    // печатаем температуру и влвжность (SHT20)
    if (DEBUG) {
        Serial.print((String) " | STH20: " + Temperature + "C, " + Humidity + "%");
    }
    if (forceDataSend || d31_old != Temperature) {
        d31_old = Temperature;
        logToScreen("d31.txt", std::to_string(Temperature) + " C");
    }
    if (forceDataSend || d32_old != Humidity) {
        d32_old = Humidity;
        logToScreen("d32.txt", std::to_string(Humidity) + " %");
    }

    // печатаем температура DS18B20 sensor
    sensorDS18B20.requestTemperatures();
    temperatureDS18B20 = sensorDS18B20.getTempCByIndex(0);
    if (DEBUG) {
        Serial.print((String) " | DS18B20: " + temperatureDS18B20 + "C");
    }
    if (forceDataSend || d11_old != temperatureDS18B20) {
        d11_old = temperatureDS18B20;
        logToScreen("d11.txt", std::to_string(temperatureDS18B20) + " C");       
    }

    if (DEBUG) {
        Serial.println();
    }
}

bool x_temperature_humidity::processConsoleCommand(std::string &cmd) {
    const char * key;
    key = "boxHumidMax ";
    if (cmd.rfind(key, 0) == 0) {
        cmd.erase(0, strlen(key));
        if (cmd.length() >= 1) {
            cmd = cmd.substr(0, 2);
            auto newHumid = atoi(cmd.data());
            mData->preferences.begin(PREFS_BOX, false);
            mData->preferences.remove(PREFS_KEY_BOX_HUMID_MAX);
            mData->preferences.putUInt(PREFS_KEY_BOX_HUMID_MAX, newHumid);
            Serial.println("Preferences saved. Don't turn off, restart in 2 seconds...");
            mData->preferences.end();
            delay(1000);
            ESP.restart();
            return true;
        }
    }

    key = "boxHumidMin ";
    if (cmd.rfind(key, 0) == 0) {
        cmd.erase(0, strlen(key));
        if (cmd.length() >= 1) {
            cmd = cmd.substr(0, 2);
            auto newHumid = atoi(cmd.data());
            mData->preferences.begin(PREFS_BOX, false);
            mData->preferences.remove(PREFS_KEY_BOX_HUMID_MIN);
            mData->preferences.putUInt(PREFS_KEY_BOX_HUMID_MIN, newHumid);
            Serial.println("Preferences saved. Don't turn off, restart in 2 seconds...");
            mData->preferences.end();
            delay(1000);
            ESP.restart();
            return true;
        }
    }

    return false;
}