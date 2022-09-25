#ifndef ESP32_PROJECT_X_X_LOGIC_H
#define ESP32_PROJECT_X_X_LOGIC_H

#include "global.h"
#include "DFRobot_ESP_PH.h"
#include "GravityTDS.h"

class x_logic : public main_looper {

public:
    void setup(main_data &data);
    void loop(bool forceDataSend);
    bool processConsoleCommand(std::string &cmd);

protected:
    void loopPhAndPpm(bool forceDataSend);

private:
    main_data *mData;
};


#endif