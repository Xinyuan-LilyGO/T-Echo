/**
 * @file      gps.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */

#pragma once



bool setupGPS();
void loopGPS();

void drawGPS();
void sleepGPS();
void wakeupGPS();

extern String gpsVersion;
