#ifndef ESP32_PROJECT_X_X_TIME_H
#define ESP32_PROJECT_X_X_TIME_H

#include <DS3231.h>
#include "global.h"

class x_time {

public:
    DateTime now;
    uint32_t nowTS;
    void setup();
    void loop();

private:
    uint32_t fakeNow = 0;
};


#endif //ESP32_PROJECT_X_X_TIME_H
