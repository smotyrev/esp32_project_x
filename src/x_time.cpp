#include "x_time.h"

// часы

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