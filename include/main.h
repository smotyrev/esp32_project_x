#include <Arduino.h>
#include <Wire.h>
#include "global.h"
#include "DFRobot_ESP_PH.h"

// насос высокого давления
uint32_t pumpHighTS_start = 0;
uint32_t pumpHighTS_end = 0;

// насос высокого давления 2
uint32_t pumpHigh2TS_start = 0;
uint32_t pumpHigh2TS_end = 0;

// Свет
bool isLightOn = false;

// БОКС вентилятор
bool isBoxVentOn = false;

// БОКС увлажнитель
bool isBoxHumidOn = false;

void processConsoleCommand();
void loopPh();