#include <Arduino.h>
#include <Wire.h>
#include "global.h"

// RELAY PINS:
#define PUMP_HIGH_PIN   23        // насос высокого давления
#define timeoutPumpHigh 15        //15 секунд таймаут
#define timePumpHigh 7            //7 екунд
uint32_t pumpHighTS_start = 0;
uint32_t pumpHighTS_end = 0;
// Свет
#define LIGHT_PIN           13  // провести провода с этого пина на 2 реле включения света + на 2 реле охлаждения света
#define lightTimeOn         10  // Во сколько часов свет включается
#define lightMinHours       12  // Стартуем с 12-ти часовым "световым днем"
#define lightMaxHours       19  // Максимальное время "светового дня", в часах
#define lightDeltaMinutes   20  // На сколько минут увеличивается световой день, каждый день
bool isLightOn = false;