#include "x_time.h"

// часы
byte Year;
byte Month;
byte Date;
byte DoW;
byte Hour;
byte Minute;
byte Second;

void x_time::setup() {
    now = RTClib::now();
    if (DEBUG && (now.year() > 2021 || now.month() > 12 || now.day() > 31)) {
        fakeNow = now.unixtime();
        now = DateTime(fakeNow);
    }
    Serial.println((String) "\n\nDate: fakeNow=" + fakeNow);
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
}

void x_time::loop() {
    now = RTClib::now();
    if (DEBUG && fakeNow != 0) {
        Serial.print("Fake! ");
        now = DateTime(fakeNow++);
    }
    nowTS = now.unixtime();
    nowHour = now.hour();

    if (SET_DATE_AND_TIME) {
        SetDateAndTime();
    }

    if (DEBUG) {
        Serial.println(
                (String) "Date: " + now.day() + "-" + now.month() + "-" + now.year()
                + "\tTime: " + now.hour() + ":" + now.minute() + ":" + now.second()
        );
    } else {
        // Date
        logToScreen("time1.txt", (String) now.day() + "-" + now.month() + "-" + now.year());
        // Time
        logToScreen("time2.txt", (String) now.hour() + ":" + now.minute() + ":" + now.second());
    }
}

void x_time::SetDateAndTime() {
    // если пришла команда из serial line, выполняем действия по настройке
    // напр. устанавливаем часы
    if (Serial.available()) {
        GetDateStuff();
        DS3231 Clock;
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
        Clock.setA1Time(DoW, Hour, Minute + 1, Second, 0x0, true,
                        false, false);
        // set A2 to two minutes past, on current day of month.
        Clock.setA2Time(Date, Hour, Minute + 2, 0x0, false, false,
                        false);
        // Turn on both alarms, with external interrupt
        Clock.turnOnAlarm(1);
        Clock.turnOnAlarm(2);
    }
}

void x_time::GetDateStuff() {
    // Call this if you notice something coming in on
    // the serial port. The stuff coming in should be in
    // the order YYMMDDwHHMMSS, with an 'x' at the end.
    boolean GotString = false;
    char InChar;
    byte Temp1, Temp2;
    char InString[20];
    byte j = 0;
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
    Temp1 = (byte) InString[0] - 48;
    Temp2 = (byte) InString[1] - 48;
    Year = Temp1 * 10 + Temp2;
    // now month
    Temp1 = (byte) InString[2] - 48;
    Temp2 = (byte) InString[3] - 48;
    Month = Temp1 * 10 + Temp2;
    // now date
    Temp1 = (byte) InString[4] - 48;
    Temp2 = (byte) InString[5] - 48;
    Date = Temp1 * 10 + Temp2;
    // now Day of Week
    DoW = (byte) InString[6] - 48;
    // now Hour
    Temp1 = (byte) InString[7] - 48;
    Temp2 = (byte) InString[8] - 48;
    Hour = Temp1 * 10 + Temp2;
    // now Minute
    Temp1 = (byte) InString[9] - 48;
    Temp2 = (byte) InString[10] - 48;
    Minute = Temp1 * 10 + Temp2;
    // now Second
    Temp1 = (byte) InString[11] - 48;
    Temp2 = (byte) InString[12] - 48;
    Second = Temp1 * 10 + Temp2;
}