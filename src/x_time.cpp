#include "x_time.h"

// часы
DateTime now;

void x_time::setup(main_data &data) {
    mData = &data;
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

void x_time::loop(bool forceDataSend) {
    if (fakeNow > 0) {
        if (millis() - fakeMillis > 1000) {
            now = DateTime(fakeNow++);
            fakeMillis = millis();
        }
    } else {
        now = RTClib::now();
    }
    
    if (!forceDataSend && mData->nowTS == now.unixtime()) {
        return;
    }

    mData->nowTS = now.unixtime();
    mData->nowMinute = now.minute();
    mData->nowHour = now.hour();
    


    if (DEBUG) {
        Serial.println(
                (String) "Date: " + now.day() + "-" + now.month() + "-" + now.year()
                + "\tTime: " + now.hour() + ":" + now.minute() + ":" + now.second()
        );
    }
    // Date
    logToScreen("time1.txt", std::to_string(now.day()) + "-" + std::to_string(now.month()) + "-" + std::to_string(now.year()));
    // Time
    logToScreen("time2.txt", (fakeNow > 0 ? "?" : "") + std::to_string(now.hour()) + ":" + std::to_string(now.minute()) + ":" + std::to_string(now.second()));
    // Start grow
    logToScreen("time3.txt", std::to_string(mData->startGrow.day()) + "-" +  std::to_string(mData->startGrow.month()) + "-" + std::to_string(mData->startGrow.year()) + " " + std::to_string(mData->startGrow.hour()) + ":00");
}

bool x_time::processConsoleCommand(std::string &cmd) {
    const char * key;
    key = "datetime ";
    if (cmd.rfind(key, 0) == 0) {
        cmd.erase(0, strlen(key));
        if (cmd.length() >= 12) {
            cmd = cmd.substr(0, 12);
            Serial.println((String) "\n[CMD SET DATE AND TIME]: [" + cmd.c_str() + "]");
            DS3231 Clock;
            Clock.setClockMode(false);  // set to 24h - flase; 12h - true
            byte Temp1, Temp2;
            // Read Year first
            Temp1 = (byte) cmd[0] - 48;
            Temp2 = (byte) cmd[1] - 48;
            auto Year = Temp1 * 10 + Temp2;
            Clock.setYear(Year);
            // now month
            Temp1 = (byte) cmd[2] - 48;
            Temp2 = (byte) cmd[3] - 48;
            auto Month = Temp1 * 10 + Temp2;
            Clock.setMonth(Month);
            // now date
            Temp1 = (byte) cmd[4] - 48;
            Temp2 = (byte) cmd[5] - 48;
            auto Date = Temp1 * 10 + Temp2;
            Clock.setDate(Date);
            // now Hour
            Temp1 = (byte) cmd[6] - 48;
            Temp2 = (byte) cmd[7] - 48;
            auto Hour = Temp1 * 10 + Temp2;
            Clock.setHour(Hour);
            // now Minute
            Temp1 = (byte) cmd[8] - 48;
            Temp2 = (byte) cmd[9] - 48;
            auto Minute = Temp1 * 10 + Temp2;
            Clock.setMinute(Minute);
            // now Second
            Temp1 = (byte) cmd[10] - 48;
            Temp2 = (byte) cmd[11] - 48;
            auto Second = Temp1 * 10 + Temp2;
            Clock.setSecond(Second);
            // Test of alarm functions
            // set A1 to one minute past the time we just set the clock
            // on current day of week.
            Clock.setA1Time(0, Hour, Minute + 1, Second, 0x0, true,
                            false, false);
            // set A2 to two minutes past, on current day of month.
            Clock.setA2Time(Date, Hour, Minute + 2, 0x0, false, false,
                            false);
            // Turn on both alarms, with external interrupt
            Clock.turnOnAlarm(1);
            Clock.turnOnAlarm(2);
            Serial.println((String) "[CMD SET DATE AND TIME]: Date " + Year + "-" + Month + "-" + Date +
                           " Time " + Hour + ":" + Minute + ":" + Second);
            Serial.println((String) "[CMD SET DATE AND TIME]: Timestamp " + RTClib::now().unixtime());
            Serial.println("Restart in 2 seconds...");
            logToScreen("d311.txt", "Preferences saved. Don't turn off, restart in 2 seconds...");
            delay(2000);
            ESP.restart();
            return true;
        } else {
            Serial.println((String) "\n[CMD SET DATE AND TIME]: Canceled! "
                                    "Wrong payload (" + cmd.data() + "), (YYMMDDwHHMMSS) expected!");
            logToScreen("d311.txt", "\n[CMD SET DATE AND TIME]: Canceled! ");
        }
    }

    key = "startGrow ";
    if (cmd.rfind(key, 0) == 0) {
        cmd.erase(0, strlen(key));
        if (cmd.length() >= 8) {
            Serial.println((String) "\n[CMD SET START GROW]: " + cmd.data());
            auto year = cmd.substr(0, 2);
            auto month = cmd.substr(2, 2);
            auto day = cmd.substr(4, 2);
            auto hour = cmd.substr(6, 2);
            Serial.println((String) "[CMD SET START GROW]: Date: " +
                           year.data() + "-" + month.data() + "-" + day.data() + " Hour: " + hour.data());
            auto dt = DateTime(atoi(year.data()), atoi(month.data()), atoi(day.data()), atoi(hour.data()));
            Serial.println((String) "[CMD SET START GROW]: DateTime: " + dt.unixtime());
            mData->preferences.begin(PREFS_START_GROW, false);
            mData->preferences.remove(PREFS_KEY_TIMESTAMP);
            mData->preferences.putUInt(PREFS_KEY_TIMESTAMP, dt.unixtime());
            Serial.println("Preferences saved. Don't turn off, restart in 2 seconds...");
            logToScreen("d321.txt", "Preferences saved. Don't turn off, restart in 2 seconds...");
            mData->preferences.end();
            delay(2000);
            ESP.restart();
            return true;
        } else {            
            logToScreen("d321.txt", "\n[CMD SET START GROW]: Canceled! ");
        }
    }

    return false;
}