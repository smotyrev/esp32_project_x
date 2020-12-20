//sep32 module
#define DEBUG false


#include <Wire.h>

// i2c : arduino -> esp32
#define I2C_SDA 21
#define I2C_SCL 22

// RELAY PINS:
#define PUMP_HIGH  23       // насос высокого давления
bool isPumpHighOn = false;
#define timeoutPumpHigh 15        //15 секунд таймаут
#define timePumpHigh 7            //7 екунд
uint32_t pumpHighTS_start = 0;
uint32_t pumpHighTS_end = 0;

// часы
#include <DS3231.h>
RTClib myRTC;
DS3231 Clock;
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;

// влажность и температура STH20
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

  // часы
  DateTime now = myRTC.now();
  Serial.println("Date:");
  Serial.print(now.year(), DEC);
  Serial.print('/');
  Serial.print(now.month(), DEC);
  Serial.print('/');
  Serial.print(now.day(), DEC);
  Serial.print(' ');
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.println();
  Serial.print(" timestamp = ");
  Serial.println(now.unixtime());
  
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

  Serial.println("SHT20 Init!");
  sht20.initSHT20();                                  // Init SHT20 Sensor
  delay(100);
  sht20.checkSHT20();                                 // Check SHT20 Sensor

  // Start the DS18B20 sensor
  sensors.begin();

  // поплавок, датчик воды
  pinMode(FLOAT_SENSOR, INPUT_PULLUP); // initialize the pushbutton pin as an input

  // RELAY PIN INIT
  pinMode(PUMP_HIGH, OUTPUT);
  digitalWrite(PUMP_HIGH, HIGH);
}

void loop() {
  //delay(delayMS);
  if (DEBUG) { Serial.println(); }
  uint32_t nowTS;
  DateTime now = myRTC.now();
  nowTS = now.unixtime();

  // RELAY processing:
  if (pumpHighTS_start > 0) {
    uint32_t dTS = nowTS - pumpHighTS_start;
    if (dTS >= timePumpHigh) {
      pumpHighTS_start = 0;
      pumpHighTS_end = now.unixtime();
      digitalWrite(PUMP_HIGH, HIGH);
      if (DEBUG) { Serial.println("мотор высокого давления: выкл."); }
    }
  } else {
    // включаем мотор высокого давления
    uint32_t dTS = nowTS - pumpHighTS_end;
    if (dTS >= timeoutPumpHigh) {
      pumpHighTS_end = 0;
      pumpHighTS_start = now.unixtime();
      digitalWrite(PUMP_HIGH, LOW);
      if (DEBUG) { Serial.println("мотор высокого давления: вкл."); }
    }
  }

  // если пришла команда из serial line, выполняем действия по настройке
  // напр. устанавливаем часы
  if (Serial.available()) {
    GetDateStuff(Year, Month, Date, DoW, Hour, Minute, Second);
    Clock.setClockMode(false);  // set to 24h - flase; 12h - true
    Clock.setYear(Year);
    Clock.setMonth(Month);
    Clock.setDate(Date);
    Clock.setDoW(DoW);
    Clock.setHour(Hour);
    Clock.setMinute(Minute);
    Clock.setSecond(Second);
    // Test of alarm functions
    // set A1 to one minute past the time we just set the clock
    // on current day of week.
    Clock.setA1Time(DoW, Hour, Minute+1, Second, 0x0, true, 
      false, false);
    // set A2 to two minutes past, on current day of month.
    Clock.setA2Time(Date, Hour, Minute+2, 0x0, false, false, 
      false);
    // Turn on both alarms, with external interrupt
    Clock.turnOnAlarm(1);
    Clock.turnOnAlarm(2);
  }

  // печатаем время
  Serial.print((String)"time1.txt=\""); 
  Serial.print(now.day(), DEC);
  Serial.print('-');
  Serial.print(now.month(), DEC);
  Serial.print('-');
  Serial.print(now.year(), DEC);
  Serial.print((String)"\""+char(255)+char(255)+char(255));
  Serial.print((String)"time2.txt=\"");
  Serial.print(now.hour(), DEC);
  Serial.print(':');
  Serial.print(now.minute(), DEC);
  Serial.print(':');
  Serial.print(now.second(), DEC);
  Serial.print((String)"\""+char(255)+char(255)+char(255));

  // печатаем температуру и влвжность (DHT)
  sensors_event_t event;
  dht.temperature().getEvent(&event);
  Serial.print((String)"d01.txt=\""+event.temperature+" C\""+char(255)+char(255)+char(255)); 
  dht.humidity().getEvent(&event);
  Serial.print((String)"d02.txt=\""+event.relative_humidity+" %\""+char(255)+char(255)+char(255)); 

  // печатаем показания ph-датчика с arduino
  Wire.requestFrom(8, 6);    // request 6 bytes from slave device #8
  Serial.print((String)"d12.txt=\""); 
  while (Wire.available()) { // slave may send less than requested
    char c = Wire.read(); // receive a byte as character
    Serial.print(c);         // print the character
  }
  Serial.print((String)"\""+char(255)+char(255)+char(255));

  // печатаем температуру и влвжность (SHT20)
  Serial.print((String)"d31.txt=\""+sht20.readTemperature()+" C\""+char(255)+char(255)+char(255)); 
  Serial.print((String)"d32.txt=\""+sht20.readHumidity()+" %\""+char(255)+char(255)+char(255)); 

  // печатаем температура DS18B20 sensor
  sensors.requestTemperatures();
  Serial.print((String)"d11.txt=\""+sensors.getTempCByIndex(0)+" C\""+char(255)+char(255)+char(255));

  // печатаем - поплавок, датчик воды
  if(digitalRead(FLOAT_SENSOR) == LOW) {
    Serial.print((String)"d14.txt=\"HIGH\""+char(255)+char(255)+char(255));
  } else {
    Serial.print((String)"d14.txt=\"LOW\""+char(255)+char(255)+char(255));
  }
}

void GetDateStuff(byte& Year, byte& Month, byte& Day, byte& DoW, 
    byte& Hour, byte& Minute, byte& Second) {
  // Call this if you notice something coming in on 
  // the serial port. The stuff coming in should be in 
  // the order YYMMDDwHHMMSS, with an 'x' at the end.
  boolean GotString = false;
  char InChar;
  byte Temp1, Temp2;
  char InString[20];
  byte j=0;
  while (!GotString) {
    if (Serial.available()) {
      InChar = Serial.read();
      InString[j] = InChar;
      j += 1;
      if (InChar == 'x') {
        GotString = true;
      }
    }
  }
  Serial.println();
  Serial.println("SET TIME:");
  Serial.println(InString);
  // Read Year first
  Temp1 = (byte)InString[0] -48;
  Temp2 = (byte)InString[1] -48;
  Year = Temp1*10 + Temp2;
  // now month
  Temp1 = (byte)InString[2] -48;
  Temp2 = (byte)InString[3] -48;
  Month = Temp1*10 + Temp2;
  // now date
  Temp1 = (byte)InString[4] -48;
  Temp2 = (byte)InString[5] -48;
  Day = Temp1*10 + Temp2;
  // now Day of Week
  DoW = (byte)InString[6] - 48;   
  // now Hour
  Temp1 = (byte)InString[7] -48;
  Temp2 = (byte)InString[8] -48;
  Hour = Temp1*10 + Temp2;
  // now Minute
  Temp1 = (byte)InString[9] -48;
  Temp2 = (byte)InString[10] -48;
  Minute = Temp1*10 + Temp2;
  // now Second
  Temp1 = (byte)InString[11] -48;
  Temp2 = (byte)InString[12] -48;
  Second = Temp1*10 + Temp2;
}
