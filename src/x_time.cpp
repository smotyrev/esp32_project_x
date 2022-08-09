#include "x_time.h"

// часы

void x_time::setup() {
    now = RTClib::now();
    if (now.hour() > 24 && now.minute() > 60 && now.second() > 60) {
        fakeNow = now.unixtime();
        now = DateTime(fakeNow);
        fakeMillis = millis();
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
    if (fakeNow > 0) {
        if (millis() - fakeMillis > 1000) {
            now = DateTime(fakeNow++);
            fakeMillis = millis();
        }
    } else {
        now = RTClib::now();
    }
    
    if (nowTS == now.unixtime()) {
        return;
    }

    nowTS = now.unixtime();

    if (DEBUG) {
        Serial.println(
                (String) "Date: " + now.day() + "-" + now.month() + "-" + now.year()
                + "\tTime: " + now.hour() + ":" + now.minute() + ":" + now.second()
        );
    } else {
        // Date
        logToScreen("time1.txt", (String) (fakeNow > 0 ? "?" : "") + now.day() + "-" + now.month() + "-" + now.year());
        // Time
        logToScreen("time2.txt", (String) (fakeNow > 0 ? "?" : "") + now.hour() + ":" + now.minute() + ":" + now.second());
    }
}