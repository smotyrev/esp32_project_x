#ifndef ESP32_PROJECT_X_X_TIME_H
#define ESP32_PROJECT_X_X_TIME_H

#include <DS3231.h>
#include "global.h"

//      Команды:
//          1) Установка даты и времени, для часов с батарейкой:
//              `datetimeYYMMDDwHHMMSS` (прим. `datetime 2104165223000` => 16 апреля 2021 года 22:30:00, пятница)
//                  YY - Год
//                  MM - Месяц
//                  DD - День
//                  w - Day Of Week (день недели 1-7)
//                  HH - Час (0-23)
//                  MM - Минута
//          2) Установить дату начала цикла роста:
//              `startGrowYYMMDDHH` (прим. `startGrow 21041622` => 16 апреля 2021 года в 22 часа)

class x_time : public main_looper {

public:
    void setup(main_data &data);
    void loop(bool forceDataSend);
    bool processConsoleCommand(std::string &cmd);

private:
    main_data *mData;
    uint32_t fakeNow = 0;
    unsigned long fakeMillis = 0;
};


#endif //ESP32_PROJECT_X_X_TIME_H
