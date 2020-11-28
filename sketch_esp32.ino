//sep32 module


#include <Wire.h>

// i2c : arduino -> esp32
#define I2C_SDA 21
#define I2C_SCL 22

// влажность и температура STH20
#include <DHT.h>
#include <DHT_U.h>

#include <DHT.h>
#include <DHT_U.h>
#define DHTPIN            15         //  Контакт, который подключен к датчику DHT
// введите модель используемого датчика, мы используем DHT11, если вы используете DHT21 или DHT22, измените его
#define DHTTYPE           DHT22     
DHT_Unified dht(DHTPIN, DHTTYPE);
uint32_t delayMS;

// влажность и температура STH20
#include "DFRobot_SHT20.h"
DFRobot_SHT20    sht20;

//температура
#include <OneWire.h>
#include <DallasTemperature.h>
// GPIO where the DS18B20 is connected to
const int oneWireBus = 4; 
// Setup a oneWire instance to communicate with any OneWire devices
OneWire oneWire(oneWireBus);
// Pass our oneWire reference to Dallas Temperature sensor 
DallasTemperature sensors(&oneWire);

// поплавок, датчик воды
#define FLOAT_SENSOR  14     // the number of the pushbutton pin

void setup() {
  Wire.begin(I2C_SDA, I2C_SCL);
  Serial.begin(9600); 
  dht.begin();
  Serial.println("Modified Temperature, Humidity Serial Monitor Example");
  sensor_t sensor;
  dht.temperature().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Temperature");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println(" *C");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println(" *C");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println(" *C");  
  Serial.println("------------------------------------");
  dht.humidity().getSensor(&sensor);
  Serial.println("------------------------------------");
  Serial.println("Humidity");
  Serial.print  ("Sensor:       "); Serial.println(sensor.name);
  Serial.print  ("Driver Ver:   "); Serial.println(sensor.version);
  Serial.print  ("Unique ID:    "); Serial.println(sensor.sensor_id);
  Serial.print  ("Max Value:    "); Serial.print(sensor.max_value); Serial.println("%");
  Serial.print  ("Min Value:    "); Serial.print(sensor.min_value); Serial.println("%");
  Serial.print  ("Resolution:   "); Serial.print(sensor.resolution); Serial.println("%");  
  Serial.println("------------------------------------");
  delayMS = sensor.min_delay / 1000;

  Serial.println("SHT20 Example!");
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20();                                 // Check SHT20 Sensor

  // Start the DS18B20 sensor
  sensors.begin();

  // поплавок, датчик воды
  pinMode(FLOAT_SENSOR, INPUT_PULLUP); // initialize the pushbutton pin as an input
}

void loop() {
  delay(delayMS);
  sensors_event_t event;  
  

//    Serial.print("Humidity: ");
//    Serial.print(event.relative_humidity);
//    Serial.println("%");

    dht.temperature().getEvent(&event);
    Serial.print((String)"d01.txt=\""+event.temperature+"ºC\""+char(255)+char(255)+char(255)); 
    dht.humidity().getEvent(&event);
    Serial.print((String)"d02.txt=\""+event.relative_humidity+"\""+char(255)+char(255)+char(255)); 


  Wire.requestFrom(8, 6);    // request 6 bytes from slave device #8
  Serial.print((String)"d12.txt=\""); 
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }
  Serial.print((String)"\""+char(255)+char(255)+char(255));


  Serial.print((String)"d31.txt=\""+sht20.readTemperature()+"\""+char(255)+char(255)+char(255)); 
  Serial.print((String)"d32.txt=\""+sht20.readHumidity()+"\""+char(255)+char(255)+char(255)); 

  //температура DS18B20 sensor
  sensors.requestTemperatures();
  Serial.print((String)"d11.txt=\""+sensors.getTempCByIndex(0)+"ºC\""+char(255)+char(255)+char(255));

  // поплавок, датчик воды
  if(digitalRead(FLOAT_SENSOR) == LOW) {
    Serial.println("HIGH");
  } else {
    Serial.println("LOW");
  }
}
