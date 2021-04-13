#include "x_temperature_humidity.h"

DHT_Unified dht(DHT_PIN, DHT_TYPE);

DFRobot_SHT20 sht20;
uint32_t dhtDelayMS;

// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWireDS18B20(DS18B20_PIN);
// Pass our oneWire reference to Dallas Temperature sensor
DallasTemperature sensorDS18B20(&oneWireDS18B20);
float temperatureDS18B20 = 0.0;

void x_temperature_humidity::setup() {
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
    sensorDS18B20.setWaitForConversion(true);
    sensorDS18B20.begin();
}

void x_temperature_humidity::loop() {
    if (DEBUG) {
        Serial.print("Temperature & Humidity:");
    }

    // печатаем температуру и влвжность (DHT)
    sensors_event_t dhtVal;
    dht.temperature().getEvent(&dhtVal);
    if (DEBUG) {
        Serial.print((String) " | DHT22: " + dhtVal.temperature + "C, ");
    } else {
        logToScreen("d01.txt", (String) dhtVal.temperature + " C");
    }
    dht.humidity().getEvent(&dhtVal);
    boxHumid = dhtVal.relative_humidity;
    if (DEBUG) {
        Serial.print((String) dhtVal.relative_humidity + "%");
    } else {
        logToScreen("d02.txt", (String) dhtVal.relative_humidity + " %");
    }

    // печатаем температуру и влвжность (SHT20)
    if (DEBUG) {
        Serial.print((String) " | STH20: " + sht20.readTemperature() + "C, " + sht20.readHumidity() + "%");
    } else {
        logToScreen("d31.txt", (String) sht20.readTemperature() + " C");
        logToScreen("d32.txt", (String) sht20.readHumidity() + " %");
    }

    // печатаем температура DS18B20 sensor
    sensorDS18B20.requestTemperatures();
    if (DEBUG) {
        Serial.print((String) " | DS18B20: " + temperatureDS18B20 + "C");
    } else {
        logToScreen("d11.txt", (String) temperatureDS18B20 + " C");
    }
    if (sensorDS18B20.isConversionComplete()) {
        temperatureDS18B20 = sensorDS18B20.getTempCByIndex(0);
    } else if (DEBUG) {
        Serial.print(" [Processing] ");
    }

    if (DEBUG) {
        Serial.println();
    }
}