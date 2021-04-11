#include <Arduino.h>
#include <Wire.h>
#include "global.h"

// насос высокого давления
uint32_t pumpHighTS_start = 0;
uint32_t pumpHighTS_end = 0;

// Свет
bool isLightOn = false;

void processConsoleCommand();