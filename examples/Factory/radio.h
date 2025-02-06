/**
 * @file      radio.h
 * @author    Lewis He (lewishe@outlook.com)
 * @license   MIT
 * @copyright Copyright (c) 2024  ShenZhen XinYuan Electronic Technology Co., Ltd
 * @date      2024-05-04
 *
 */

#include <RadioLib.h>

#define USE_SX1262


#if   defined(USE_SX1262)
#define RADIO_TYPE      "SX1262"
#elif defined(USE_SX1268)
#define RADIO_TYPE      "SX1268"
#endif

bool setupLoRa();
void sleepLoRa();

void wakeupLoRa();
void loopSender();
void loopReceiver();
void drawReceiver();
void drawSender();

#ifndef LORA_FREQ
#define LORA_FREQ       920.0
#endif

#ifndef LORA_BW
#define LORA_BW         125.0
#endif

#ifndef LORA_TX_POWER
#define LORA_TX_POWER   22
#endif

#ifndef LORA_SF
#define LORA_SF         9
#endif

