#ifndef ESP32_PROJECT_X_X_TIME_H
#define ESP32_PROJECT_X_X_TIME_H

#include <DS3231.h>
#include "global.h"

class x_time {

public:
    DateTime now;
    uint32_t nowTS;
    uint32_t nowHour;
    void setup();
    void loop();

private:
    uint32_t fakeNow = 0;
    void SetDateAndTime();
    void GetDateStuff();
};


#endif //ESP32_PROJECT_X_X_TIME_H
