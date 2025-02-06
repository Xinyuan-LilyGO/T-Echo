/**
 * @file      display.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */

#pragma once

#include <GxEPD2_BW.h>
#include <Fonts/FreeMono9pt7b.h>
#include <Fonts/FreeMonoBold9pt7b.h>
#include <Fonts/FreeMonoBold12pt7b.h>
#include <Fonts/FreeMonoBold18pt7b.h>
#include <Fonts/FreeMonoBold24pt7b.h>
#include <Fonts/FreeMono12pt7b.h>
#include <Fonts/FreeMonoOblique9pt7b.h>
#include "FreePuhuiti7pt7b.h"
#include "utilities.h"

#define PPS_LED                 GreenLed_Pin
#define SENDER_LED              RedLed_Pin
#define RECV_LED                BlueLed_Pin
#define FAILED_LED              0xFF
#define CONNECT_LED             BlueLed_Pin + 1
#define DISCONNECT_LED          BlueLed_Pin + 2
#define WHITE_LED               BlueLed_Pin + 3
#define OFF_LED                 0X7C

#define DEFAULT_FONT            &FreeMono9pt7b
#define DEFAULT_FONT_HEIGHT     FreeMono9pt7b.yAdvance

extern GxEPD2_BW<GxEPD2_154_D67, GxEPD2_154_D67::HEIGHT> display;

void setupDisplay();

void drawBox(uint16_t x, uint16_t y, uint16_t w, uint16_t h, uint16_t bgColor);

void adjustBacklight();
