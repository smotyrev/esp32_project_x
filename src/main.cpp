// include library to read and write from flash memory
#include <EEPROM.h>

#include "main.h"
#include "x_time.h"
#include "x_temperature_humidity.h"
#include "x_logic.h"

// Setup UART buffered IO with event queue
const int uart_buffer_size = (1024);
QueueHandle_t uart_queue;

x_time xTime;
x_temperature_humidity xTempHumid;
x_logic xLogic;
main_looper* mainLoopers[] = {&xTime, &xTempHumid, &xLogic};

void setup() {
    Serial.begin(9600);
    Serial.println("\r\n---- ~ SETUP ----");
    Wire.begin(I2C_SDA_PIN, I2C_SCL_PIN, (uint32_t) 4000);

    uart_config_t uart_config = {
        .baud_rate = 9600, //115200, //9600,
        .data_bits = UART_DATA_8_BITS,
        .parity = UART_PARITY_DISABLE,
        .stop_bits = UART_STOP_BITS_1,
        .flow_ctrl = UART_HW_FLOWCTRL_DISABLE, //UART_HW_FLOWCTRL_CTS_RTS,
        // .rx_flow_ctrl_thresh = 122,
    };
    int intr_alloc_flags = 0;
    // Configure UART parameters
    ESP_ERROR_CHECK(uart_param_config(UART_NUM_2, &uart_config));
    ESP_ERROR_CHECK(uart_set_pin(UART_NUM_2, 17, 16, UART_PIN_NO_CHANGE, UART_PIN_NO_CHANGE));
    ESP_ERROR_CHECK(uart_driver_install(UART_NUM_2, uart_buffer_size * 2, uart_buffer_size * 2, 
                    10, &uart_queue, intr_alloc_flags));

    for (auto i = std::begin(mainLoopers); i != std::end(mainLoopers); ++i) {
        auto item = *i;
        item->setup();
    }

    Serial.println("\r\n---- ~ SETUP ----");
}

uint32_t freeHeap;
bool resendAllData = false;
int lastTxFromScreen = 0;
long loopStartMillis = 0;
void loop() {
    loopStartMillis = millis();
    
    if (!processConsoleCommand()) {
        // Если не было никаких TX данных, скорее всего экран отключился\подключился, 
        // обновляем дфнные принудительно 
        if (!resendAllData && loopStartMillis - lastTxFromScreen > 5000) {
            lastTxFromScreen = loopStartMillis;
            resendAllData = true;
        }
    } else {
        //noop
    }

    if (VERBOSE && resendAllData) {
        Serial.println("Resending all data ...");
    }

    // if (DEBUG) {
	//     Serial.println("\r\n-------------------");
    //     ph.calibration(phVoltage, phTemperature); // calibration process by Serail CMD
    // }
    
    for (auto i = std::begin(mainLoopers); i != std::end(mainLoopers); ++i) {
        auto item = *i;
        item->loop(resendAllData);
    }

    if (DEBUG) {
        delay(LOOP_MS_DEBUG - (millis() - loopStartMillis));
    }

    resendAllData = false;

    if (freeHeap != ESP.getFreeHeap()) {
        freeHeap = ESP.getFreeHeap();
        if (DEBUG || VERBOSE) {
            logEvent((String) "\nFree heap: " + freeHeap);
        }
    }
}

// Configure a temporary buffer for the incoming data
char *rx_data = (char *) malloc(uart_buffer_size);
inline bool processConsoleCommand() {
    // Read data from the UART
    int len = uart_read_bytes(UART_NUM_2, rx_data, (uart_buffer_size - 1), 20 / portTICK_PERIOD_MS);
    if (len == 0) {
        return false;
    }

    std::string str(rx_data);
    str.erase(str.begin(), std::find_if(str.begin(), str.end(), [](unsigned char ch) {
        return std::isdigit(ch) || std::isalpha(ch) || std::ispunct(ch);
    }));
    if (str.length() == 0) {
        return false;
    }
    memset(rx_data, 0, uart_buffer_size);
    // char _char;
    // do {
    //     _char = (char) Serial.read();
    //     if (std::isdigit(_char) || std::isalpha(_char) || std::ispunct(_char)) {
    //         if (VERBOSE) {
    //             Serial.println((String) "OK CHAR = " + _char);
    //         }
    //         str.append(1, _char);
    //         continue;
    //     }
    // } while (Serial.available());

    if (DEBUG || VERBOSE) {
        logEvent(("Console command [" + str + "]").c_str());
    }

    rtrim(str);
    ltrim(str);
    lastTxFromScreen = loopStartMillis;

    if (str.length() == 0) {
        if (DEBUG || VERBOSE) {
            logEvent("Skip, empty command");
        }
        return !resendAllData;
    }

    for (auto i = std::begin(mainLoopers); i != std::end(mainLoopers); ++i) {
        auto item = *i;
        if (item->processConsoleCommand(str)) {
            if (DEBUG || VERBOSE) {
                logEvent("Command handled");
            }
            return true;
        }
    }

    const char * cmd;
    
    // Nexion commands
    cmd = "e";
    if (str.rfind(cmd, 0) == 0) {
        str.erase(0, strlen(cmd));
        resendAllData = true;
        if (DEBUG || VERBOSE) {
            logEvent("Screen changed, resend all data");
        }
        return true;
    }

    if (DEBUG || VERBOSE && str.length() > 0) {
        logEvent("Unknown command");
    }

    return false;
}