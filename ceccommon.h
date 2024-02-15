/* Copyright 2023 KD8CEC
 * https://github.com/kd8cec
 *
 * This is the source code used to minimize memory usage.
 *  Global variables among the source code written by KD8CEC will be shared and used.
 * Most source code written by KD8CEC will INCLUDE this file.
 * 
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 *     Unless required by applicable law or agreed to in writing, software
 *     distributed under the License is distributed on an "AS IS" BASIS,
 *     WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 *     See the License for the specific language governing permissions and
 *     limitations under the License.
 */

#ifndef CEC_COMMON_H
#define CEC_COMMON_H

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include "driver/bk4819.h"
#include "driver/keyboard.h"
#include "audio.h"
#include "string.h"
#include <stdint.h>
#include <string.h>
#include "external/printf/printf.h"
//#include "gpsinfo.h"
#include "driver/eeprom.h"
#include "misc.h"
#include "radio.h"
#include "driver/system.h"
#include "driver/st7565.h"
#include "settings.h"
#include "driver/systick.h"
#include "ui/helper.h"
#include "ui/ui.h"
#include "ui/main.h"
#include "font.h"
#include "functions.h"
#include "app/app.h"

#define _MAX_READ_CH_ATTRIBUTES 7
#define COMBUFF_USE_SEEK_RSSI   01
#define COMBUFF_USE_SEEK_NONE   99
#define COMBUFF_LENGTH          128
#define CEC_EEPROM_START1       0x1D50   //0x1D50 ~ 0x1DFF
#define CEC_EEPROM_START2       0x1D00   //0x1D00 ~ 0x1D4F
#define CEC_EEPROM_START3       0x1BD0   //0x1BD0 ~ 0x1BFF  (48Byte)
#define CEC_EEPROM_START4       0x1F90   //0x1F90 ~ 0x1FFF  (96Byte)
#define CEC_EEPROM_DATA1        0x0F5B   //5 0x0F5A ~ 0x0F5F (6Byte, but 0x0F5A is buffer with Channel Name) + (index * 16) index Count =  20, 5byte * 20 = 100byte
#define CEC_EEPROM_DATA2        0x1E0A   //7 0x1E0A ~ 0x1E0A (7byte) + (index * 16) index Count = 12, 7byte * 12 = 84byte
#define EEPROM_WELCOMESTRING1   0x0EB0    //16Byte
#define EEPROM_WELCOMESTRING2   0x0EC0    //16Byte
#define EEPROM_CONFIGSTART  
#define CEC_EEPROM_CALLSIGN EEPROM_WELCOMESTRING1   //CALLSIGN SPACE GRID, ex)KD8CEC EM37, KD8CEC/TEST EM37
#define CEC_EEPROM USERNAME EEPROM_WELCOMESTRING1   //NAME STATUS using APRS 

#define CW_LR_MODE + //ICOM STYLE, - : YAESU STYLE (uBITX : CWL, CWR)
//extern ChannelAttributes_t gMR_ChannelAttributes[_MAX_READ_CH_ATTRIBUTES];
extern uint8_t CommBuff[COMBUFF_LENGTH];  //for Common Use for 
extern uint8_t CommBuffUsingType;
extern uint32_t CommBuffLastUseTime;

extern uint8_t CW_TONE;             //Hz Default 700Hz
extern uint8_t CW_SPEED;
extern uint8_t CW_KEYTYPE;
extern uint8_t CEC_LiveSeekMode;   //0:NONE, 1:LIVE, 2:LIVE+1, 3:LIVE+2
#define LIVESEEK_NONE           0   //NONE
#define LIVESEEK_RCV            1   //SPEAKER ONLY
#define LIVESEEK_RCV_SPECTRUM1  2   //SPECTRUM SMALL
#define LIVESEEK_RCV_SPECTRUM2  3   //SPECTRUM LARGE (Not Use, Reserve)

#define GET_FREQ_OFFSET(__rxmode__) ( CW_LR_MODE (__rxmode__ == MODULATION_CW ||__rxmode__ == MODULATION_CWN ? (CWTone) : 0))
#define delay(delayTime) SYSTEM_DelayMs(delayTime)

ChannelAttributes_t MR_ChannelAttributes(int _channelIndex);
void SetMR_ChannelAttributes(int channel, ChannelAttributes_t att);
void CEC_ApplyChangeRXFreq(int _applyOption);
void CEC_TimeSlice500ms(void);
void DrawCommBuffToSpectrum(void);
uint32_t millis10();    //scheduler.c
#endif
