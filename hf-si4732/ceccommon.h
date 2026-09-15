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
#include "driver\bk4819.h"
#include "driver\keyboard.h"
#include "audio.h"
#include "string.h"
#include <stdint.h>
#include <string.h>
#include "external/printf/printf.h"
//#include "gpsinfo.h"
#include "driver\eeprom.h"
#include "misc.h"
#include "radio.h"
#include "driver\system.h"
#include "driver\st7565.h"
#include "settings.h"
#include "driver\systick.h"
#include "ui\helper.h"
#include "ui\ui.h"
#include "ui\main.h"
#include "font.h"
#include "functions.h"
#include "app\app.h"
#include "ui\helper.h"
#include "driver\uart.h"
#include "bsp\dp32g030\uart.h"
#include "bsp\dp32g030\dma.h"
#include "bsp\dp32g030\syscon.h"

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
#define EEPROM_CHANNELNAME      0x0F50    //10byte  left 6 byte always set zero
#define EEPROM_CONFIGSTART  
#define CEC_EEPROM_CALLSIGN EEPROM_WELCOMESTRING1   //CALLSIGN SPACE GRID, ex)KD8CEC EM37, KD8CEC/TEST EM37
#define CEC_EEPROM USERNAME EEPROM_WELCOMESTRING1   //NAME STATUS using APRS 
#define CEC_EEPROM_CWQSODATA    0x1BE0    //TEMPDATA

//1:SAVE DX CALL
//2:CLEAR SCREEN
//3:SAVE QSO STRING

//BY KD8CEC
#define CEC_EEPROM_RIGINFO   0x2680    //RIGINFO EEPROM ADDRESS
#define RIGINFO_MSG_MYCALL   (0  + RIGINFO_FIRST) //CHANNEL 170
#define RIGINFO_MSG_MYNAME   (1  + RIGINFO_FIRST) //CHANNEL 171
#define RIGINFO_MSG_MYGRID   (2  + RIGINFO_FIRST) //CHANNEL 172
#define RIGINFO_MSG_GPSLAT   (3  + RIGINFO_FIRST) //CHANNEL 173
#define RIGINFO_MSG_GPSLON   (4  + RIGINFO_FIRST) //CHANNEL 174
#define RIGINFO_MSG_DXCALL   (5  + RIGINFO_FIRST) //CHANNEL 175 HOTKEY ON CW TXSTATUS
#define RIGINFO_MSG_DXNAME   (6  + RIGINFO_FIRST) //CHANNEL 176
#define RIGINFO_MSG_APRSMSG  (7  + RIGINFO_FIRST) //CHANNEL 177
#define RIGINFO_MSG_SSTVMSG1 (8  + RIGINFO_FIRST) //CHANNEL 178
#define RIGINFO_MSG_SSTVMSG2 (9  + RIGINFO_FIRST) //CHANNEL 179
#define RIGINFO_MSG_RESERVE  (10 + RIGINFO_FIRST) //CHANNEL 180

#define CEC_EEPROM_CWMSG0    (EEPROM_CHANNELNAME + RIGINFO_MSG_CWMSG0 * 16)    //CWMSG0
#define RIGINFO_MSG_CWMSG0   (11 + RIGINFO_FIRST) //CHANNEL 181
#define RIGINFO_MSG_CWMSG1   (12 + RIGINFO_FIRST) //CHANNEL 182
#define RIGINFO_MSG_CWMSG2   (13 + RIGINFO_FIRST) //CHANNEL 183
#define RIGINFO_MSG_CWMSG3   (14 + RIGINFO_FIRST) //CHANNEL 184
#define RIGINFO_MSG_CWMSG4   (15 + RIGINFO_FIRST) //CHANNEL 185
#define RIGINFO_MSG_CWMSG5   (16 + RIGINFO_FIRST) //CHANNEL 186
#define RIGINFO_MSG_CWMSG6   (17 + RIGINFO_FIRST) //CHANNEL 187
#define RIGINFO_MSG_CWMSG7   (18 + RIGINFO_FIRST) //CHANNEL 188
#define RIGINFO_MSG_CWMSG8   (19 + RIGINFO_FIRST) //CHANNEL 189
#define RIGINFO_MSG_CWMSG9   (20 + RIGINFO_FIRST) //CHANNEL 190

#define CWMODE_NONE           0
#define CWMODE_CWN            1
#define CWMODE_CWFM           2
#define CWMODE_CW             3
#define CWMODE_CWAM           4

#define CW_KEYTYPE_KEYPAD_PDL 0   //COMBINATION PTT AND MENU KEY
#define CW_KEYTYPE_KEYPAD_ST  1   //KEYPAD STRAIGHT PTT OR MENU KEY BUT TERRIBLE PERFORMANCE JUST TOY I CONCIDER FOR REMOVE THIS MENU
#define CW_KEYTYPE_PADDLE     2   //IAMBIC.A WITH 2 REGISTER
#define CW_KEYTYPE_STRAIGHT   3   //EXTERAL STRAIHGT KEY WITH 1 REGISTER
#define CW_KEYTYPE_PC         4   //FOR USING PC PROGRAM AS FLDIGI


#define CW_LR_MODE + //ICOM STYLE, - : YAESU STYLE (uBITX : CWL, CWR)

#define GET_FREQ_OFFSET(__rxmode__) ( CW_LR_MODE (__rxmode__ == MODULATION_CW ||__rxmode__ == MODULATION_CWN ? ( CW_Tone ) : 0))


typedef uint8_t byte;
struct ST_CW_ADC
{
  uint16_t CWKKEY_DIT_AdcFrom;
  uint16_t CWKKEY_DAH_AdcFrom;
  uint16_t CWKKEY_BOTH_AdcFrom;
  uint16_t CWKKEY_BOTH_AdcTo;
} __attribute__((packed));

extern struct ST_CW_ADC CW_ADC;

//For Reduce Memory, reading at use time
extern ChannelAttributes_t gMR_ChannelAttributes[_MAX_READ_CH_ATTRIBUTES];
extern uint8_t CommBuff[COMBUFF_LENGTH];  //for Common Use for 
extern uint8_t CommBuffUsingType;
extern uint32_t CommBuffLastUseTime;

extern uint8_t CW_Tone;   //Hz Default 700Hz
extern uint8_t CW_Mode;     //0 : None, 1 : CWN, 2:CW-FM (MCW OR F2A), 3:CW (A1A), 4:MCW A2A
extern uint8_t CW_SideTone; //1: Enabled,  0: Disabled
extern uint8_t CW_KeyType;  //
extern uint8_t CW_WPM;     //[EEPROM] 5~ 50 
extern uint8_t CW_TXDelay;     //[EEPROM] * 100 milisecond
extern uint8_t CW_SPEED;
extern uint8_t HF_DualWatch;
extern uint8_t HF_DualVol;

extern uint8_t CEC_LiveSeekMode;   //0:NONE, 1:LIVE, 2:LIVE+1, 3:LIVE+2
extern uint8_t SSTV_LCD_Start_Timer;

extern uint8_t strBuff[32];     //For sprintf
extern bool isNeed4732Init;         //SI4732 INIT CHECK

#define LIVESEEK_NONE           0   //NONE
#define LIVESEEK_RCV            1   //SPEAKER ONLY
#define LIVESEEK_RCV_SPECTRUM1  2   //SPECTRUM SMALL
#define LIVESEEK_RCV_SPECTRUM2  3   //SPECTRUM LARGE (Not Use, Reserve)

#define delay(delayTime) SYSTEM_DelayMs(delayTime)

ChannelAttributes_t MR_ChannelAttributes(int _channelIndex);
void SetMR_ChannelAttributes(int channel, ChannelAttributes_t att);
void CEC_ApplyChangeRXFreq(int _applyOption);
void CEC_TimeSlice500ms(void);
void DrawCommBuffToSpectrum(void);
uint32_t millis10();    //scheduler.c

void UART_SendByte(uint8_t _sendByte);
void UART_Init9600(void);
void SetRX1Mode(int rx1Mode);
void InitRX1Mode(void);
void BackLightBlink(int _cnt);
void CWDecodedChar(char _decodedChar, int decodeType, int _option1, int _option2);
void CEC_Radio_Work(void);

void CEC_DESIGN_TEST(void);
uint8_t dBuVToSignalLength15(uint8_t _srcValue);
void CEC_DisplaySmallest(const char *pString, uint8_t x, uint8_t y, bool statusbar, bool fill);
void CEC_ReverseScreen(uint8_t * _srcBuff, int _buffSize);
void SI4735_setVolume(uint8_t volume);  //dupulicated it,   from cec_si4735.h
#endif
