/**
 * @file      sensor.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 * 
 */
#pragma once

#include <SensorPCF8563.hpp>

bool setupSensor();
void sleepSensor();
void wakeupSensor();
void loopSensor();
bool setupRTC();
void drawSensor();
void drawSensor();
extern SensorPCF8563   rtc;
