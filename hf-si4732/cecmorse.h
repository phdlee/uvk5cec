/*************************************************************************
  KD8CEC's Memory Keyer for HAM
  
  This source code is written for All amateur radio operator, 
  I have not had amateur radio communication for a long time. CW has been 
  around for a long time, and I do not know what kind of keyer and keying 
  software is fashionable. So I implemented the functions I need mainly.

  To minimize the use of memory space, we used bitwise operations.
  For the alphabet, I put Morsecode in 1 byte. The front 4Bit is the length 
  and the 4Bit is the Morse code. Because the number is fixed in length, 
  there is no separate length information. The 5Bit on the right side is 
  the Morse code.

  I wrote this code myself, so there is no license restriction. 
  So this code allows anyone to write with confidence.
  But keep it as long as the original author of the code.
  DE Ian KD8CEC
-----------------------------------------------------------------------------
   This program is free software: you can redistribute it and/or modify
   it under the terms of the GNU General Public License as published by
   the Free Software Foundation, either version 3 of the License, or
   (at your option) any later version.

   This program is distributed in the hope that it will be useful,
   but WITHOUT ANY WARRANTY; without even the implied warranty of
   MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
   GNU General Public License for more details.

   You should have received a copy of the GNU General Public License
   along with this program.  If not, see <http://www.gnu.org/licenses/>.

**************************************************************************/

#ifndef CECMORSE_H
#define CECMORSE_H

//#include <avr/pgmspace.h>
#include <stdint.h>
#include "driver/system.h"
#include "driver\backlight.h"

#include "driver\uart.h"

#define CW_KEY_IDL           -1
#define CW_KEY_DIT            1
#define CW_KEY_DAH            2
#define CW_KEY_BOTH           3

#define CW_DECODE_ERROR      -1
#define CW_DECODE_SPACE       0
#define CW_DECODE_ALPHA       1
#define CW_DEOCDE_NUM         2
#define CW_DECODE_SYMBOLE     3
#define CW_DECODE_MULTI       4


//PULLDOWN으로 처리한것 (10K, 20K)
extern uint16_t CW_DECODE_INTERVAL;	//10mili second
extern uint16_t SPACE_INTERVAL;	//10mili second

//For Decode CW
extern uint32_t lastPressKeyTime;
extern uint8_t arPressKey[10];	//
extern uint8_t pressKeyIndex;
extern uint8_t spaceCheck;

#define CW_LR_MODE + //ICOM STYLE, - : YAESU STYLE (uBITX : CWL, CWR)


//27 + 10 + 18 + 1(SPACE) = //56 
static const uint8_t cwAZTable[27] = {0b00100100 , 0b01001000 , 0b01001010 , 0b00111000 , 0b00010000, 0b01000010, 0b00111100, 0b01000000 , //A ~ H
0b00100000, 0b01000111 ,0b00111010, 0b01000100, 0b00101100, 0b00101000 , 0b00111110, 0b01000110, 0b01001101, 0b00110100, //I ~ R
0b00110000, 0b00011000, 0b00110010, 0b01000001, 0b00110110, 0b01001001, 0b01001011, 0b01001100};  //S ~ Z

static const uint8_t cw09Table[27] = {0b00011111, 0b00001111, 0b00000111, 0b00000011, 0b00000001, 0b00000000, 0b00010000, 0b00011000, 0b00011100, 0b00011110};

//# : AR, ~:BT, [:AS, ]:SK, ^:KN
static const  uint8_t cwSymbolIndex[] =  {'.',         ',',        '?',         '"',       '!',         '/',      '(',       ')',        '&',        ':',        ';',         '=',        '+',        '-',        '_',        '\'',       '@',          '#',         '~',        '[',        ']',        '^' };

static const uint8_t cwSymbolTable[]  = {0b11010101, 0b11110011, 0b11001100, 0b11011110, 0b11101011, 0b10100100, 0b10101100, 0b11101101, 0b10010000, 0b11111000, 0b11101010, 0b10100010, 0b10010100, 0b11100001, 0b11001101, 0b11010010,  0b11011010,  0b10010100, 0b10100010, 0b10010000, 0b11000101, 0b10101100};
////const PROGMEM uint8_t cwSymbolLength[] = {6,          6,          6,         6,           6,          5,          5,          6,          5,          6,          6,          5,          5,          6,          6,          6,         6,         5,          5,          5,           6,          5};

// ":(Start"),   ':(End "), >: My callsign, <:QSO Callsign (Second Callsign), #:AR, ~:BT, [:AS, ]:SK

uint16_t GetCWADC(void);
void CWKeyDown(void);
void CWKeyUp(void);

void delay_background(int delayTime, int fromType);
char DecodeMorseData(uint8_t *srcBuff, int dataLength, int * _decodeType);
void sendCWChar(char cwKeyChar);
//void InitCWMode(uint8_t _cwMode, uint8_t _cwKeyType, uint8_t _cwWPM, uint16_t _cwTone, uint8_t _cwSideTone, uint16_t _key1AdcFrom, uint16_t _key1AdcTo,  uint16_t _key2AdcFrom, uint16_t _key2AdcTo,  uint16_t _key3AdcFrom, uint16_t _key3AdcTo);
void InitCWMode(uint8_t _cwMode );
//void CWTXStart(uint8_t isDecoding, void (*_cWDecodedChar)(char , int, int, int), uint8_t justStart);
void CWTXStart(uint8_t isDecoding, uint8_t justStart);
uint8_t GetCWKeyStatus();

#define CWKEY_STATUS_IDLE 0
#define CWKEY_STATUS_DIT  1
#define CWKEY_STATUS_DAH  2
#define CWKEY_STATUS_BOTH 3

__inline uint16_t CW_SCALE_TONE(const uint16_t freq)
{
	return (((uint32_t)freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}


#endif