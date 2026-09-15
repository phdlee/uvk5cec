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
#include "ceccommon.h"
#include "bitmaps.h"
#include "driver/bk1080-regs.h"
#include "bsp/dp32g030/gpio.h"
#include "driver/gpio.h"
#include "driver/i2c.h"
#include "driver/system.h"
#include "misc.h"

//================================================================================
// Source from bk1080.c and more, FOR reduce program memory
//--------------------------------------------------------------------------------
uint16_t BK1080_ReadRegister(BK1080_Register_t Register)
{
	uint8_t Value[2];

	I2C_Start();
	I2C_Write(0x80);
	I2C_Write((Register << 1) | I2C_READ);
	I2C_ReadBuffer(Value, sizeof(Value));
	I2C_Stop();

	return (Value[0] << 8) | Value[1];
}

void BK1080_WriteRegister(BK1080_Register_t Register, uint16_t Value)
{
	I2C_Start();
	I2C_Write(0x80);
	I2C_Write((Register << 1) | I2C_WRITE);
	Value = ((Value >> 8) & 0xFF) | ((Value & 0xFF) << 8);
	I2C_WriteBuffer(&Value, sizeof(Value));
	I2C_Stop();
}

#define CEC_FM_STARTUP      0
#define CEC_FM_STOP         1
#define CEC_FM_SOUNDONOFF   2
#define CEC_FM_SET_FREQ     3
#define CEC_FM_GET_DEVATION 4

uint8_t BK1080_Status = 0;  //0 default > 1, init
void BK_1080Control(uint8_t _controlCmd, uint16_t _controlData)
{
const uint16_t BK1080_RegisterTable[] =
{
	0x0008, 0x1080, 0x0201, 0x0000, 0x40C0, 0x0A1F, 0x002E, 0x02FF,
	0x5B11, 0x0000, 0x411E, 0x0000, 0xCE00, 0x0000, 0x0000, 0x1000,
	0x3197, 0x0000, 0x13FF, 0x9852, 0x0000, 0x0000, 0x0008, 0x0000,
	0x51E1, 0xA8BC, 0x2645, 0x00E4, 0x1CD8, 0x3A50, 0xEAE0, 0x3000,
	0x0200, 0x0000,
};

    if (_controlCmd == CEC_FM_STARTUP)
    {
		GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BK1080);

		if (BK1080_Status < 1)
		{
			for (unsigned int i = 0; i < ARRAY_SIZE(BK1080_RegisterTable); i++)
				BK1080_WriteRegister(i, BK1080_RegisterTable[i]);

			SYSTEM_DelayMs(250);

			BK1080_WriteRegister(BK1080_REG_25_INTERNAL, 0xA83C);
			BK1080_WriteRegister(BK1080_REG_25_INTERNAL, 0xA8BC);
			SYSTEM_DelayMs(60);

			BK1080_Status = true;
		}
		else
		{
			BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x0201);
		}

		BK1080_WriteRegister(BK1080_REG_05_SYSTEM_CONFIGURATION2, 0x0A5F);
		BK1080_WriteRegister(BK1080_REG_03_CHANNEL, _controlData - 760);

		SYSTEM_DelayMs(10);
		BK1080_WriteRegister(BK1080_REG_03_CHANNEL, (_controlData - 760) | 0x8000);        
    }
    else if (_controlCmd == CEC_FM_STOP)
    {
		BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, 0x0241);
		GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BK1080);
    }
    else if (_controlCmd == CEC_FM_SOUNDONOFF)
    {
	    BK1080_WriteRegister(BK1080_REG_02_POWER_CONFIGURATION, _controlData ? 0x0201 : 0x4201);
    }
    else if (_controlCmd == CEC_FM_SET_FREQ)
    {
        BK1080_WriteRegister(BK1080_REG_03_CHANNEL, _controlData - 760);
        SYSTEM_DelayMs(10);
        BK1080_WriteRegister(BK1080_REG_03_CHANNEL, (_controlData - 760) | 0x8000);
    }
    else if (_controlCmd == CEC_FM_GET_DEVATION)
    {
        //BK1080_BaseFrequency      = Frequency;
        //BK1080_FrequencyDeviation = BK1080_ReadRegister(BK1080_REG_07) / 16;
    }

}
//--------------------------------------------------------------------------------
// END OF Source from bk1080.c
//================================================================================

/*

void CEC_ReverseScreen(uint8_t * _srcBuff, int _buffSize)
{
    //reverse
    for (int i = 0; i < _buffSize; i++)
        _srcBuff[i] = ~_srcBuff[i];
}

static void PutPixel(uint8_t x, uint8_t y, bool fill) {
  //UI_DrawPixelBuffer(gFrameBuffer, x, y, fill);
}
static void PutPixelStatus(uint8_t x, uint8_t y, bool fill) {
  //UI_DrawPixelBuffer(&gStatusLine, x, y, fill);
}

static void CEC_DisplaySmallest(const char *pString, uint8_t x, uint8_t y,
                                bool statusbar, bool fill) {
  uint8_t c;
  uint8_t pixels;
  const uint8_t *p = (const uint8_t *)pString;
  while ((c = *p++) && c != '\0') 
  {
    if (c >= 0x2A)
    {
        c -= 0x2B;

        for (int i = 0; i < 3; ++i) 
        {
        pixels = gFont3x5[c][i];
            
        for (int j = 0; j < 6; ++j) 
        {
            if (pixels & 1) 
            {
            if (statusbar)
                UI_DrawPixelBuffer(&gStatusLine, x + i, y + j, fill);
                //PutPixelStatus(x + i, y + j, fill);
            else
                UI_DrawPixelBuffer(gFrameBuffer, x + i, y + j, fill);
                //PutPixel(x + i, y + j, fill);
            }
            pixels >>= 1;
        }
        }
    }
    x += 4;
  }
}

*/

#define FM_FREQ_MIN   760
#define FM_FREQ_MAX  1159
#define CENTER_POSITION 64

/*
//16
start : smeter 4 lessthan (1,2,3,4 (si4732 great than zero so not check -14, -8 )), 4,5,6,7,8,9, + 20 + 30 + 60
//CNT:  1  2  3  4  5  6  7  8   9    10   11   12  13   14  15 16
//VAL:  1  2  4 10 16 22 28 34  44   54    64   74  84   94 104 114 over
//S  : u1 u2  4  5  6  7  8  9 +10 9+20  9+30  +40 +50  +60 +70 +80
IARU Signal μV vs S-Meter reading : HF
Received Voltage	Received Power (Zin = 50 Ω)	Signal Strength (S-Value)
-14.0 dBμV	0.2 μV	-121 dBm	1
-8.0 dBμV	0.4 μV	-115 dBm	2
-2.0 dBμV	0.8 μV	-109 dBm	3
4.0 dBμV	1.6 μV	-103 dBm	4
10.0 dBμV	3.2 μV	-97 dBm	5
16.0 dBμV	6.3 μV	-91 dBm	6
22.0 dBμV	12.6 μV	-85 dBm	7
28.0 dBμV	25.1 μV	-79 dBm	8
34.0 dBμV	50.1 μV	-73 dBm	9
40.0 dBμV	99.9 μV	-67 dBm	9 +6
44.0 dBμV	158.3 μV	-63 dBm	9 +10
46.0 dBμV	199.3 μV	-61 dBm	9 +12
52.0 dBμV	397.6 μV	-55 dBm	9 +18
54.0 dBμV	500.6 μV	-53 dBm	9 +20
58.0 dBμV	793.4 μV	-49 dBm	9 +24
64.0 dBμV	1.6 mV	-43 dBm	9 +30
74.0 dBμV	5.0 mV	-33 dBm	9 +40
84.0 dBμV	15.8 mV	-23 dBm	9 +50
94.0 dBμV	50.1 mV	-13 dBm	9 +60
*/
//dBuV to 0 ~ 15
uint8_t dBuVToSignalLength15(uint8_t _srcValue)
{
    //                    4Under 1 and 2  4  5   6    7  8   9   +10   +20 +30  +40  +50   +60  +70       +80
    //uint8_t _baseMeterValue[] = {1, 2,  4, 10, 16, 22, 28, 34,  44,   54, 64,  74,  84,   94, 104}; //, 114};
    //CHANGED MAX 50 (SI4732 MAX 50)
    //                    4Under 1  4  5   6    7  8   9   +3  +5  +7  +10  +15  +20  +25  +30          + 40
    uint8_t _baseMeterValue[] = {2, 4, 10, 16, 22, 28, 34, 37, 39, 42,  44,  48,  54,  59,  64}; //,  74}; //, 114};
    for (int i = sizeof(_baseMeterValue) - 1; i >= 0; i--)
    {
        if (_srcValue >= _baseMeterValue[i])
            return i + 1;
    }
    
    return 0;
}

void CEC_FM_DrawRSSI(bool _isDraw)
{
    uint8_t tmpBuff[20];
	memset(gFrameBuffer[5], 0, 128 * 2);		//2Line
    uint8_t readedRSSI = BK1080_ReadRegister(0x0A);
	uint16_t readRSSI =  (readedRSSI & 0xFF); //dBuVToSignalLength15(SI4735_getCurrentRSSI()) ;	//MAX 20
    //0XFF IS 75dbUv
    //displayRSSI = dBuVToSignalLength15((displayRSSI * 10) / 34);
    //readRSSI = (readRSSI * 10) / 34;
    uint8_t displayRSSI = dBuVToSignalLength15(readRSSI);
    uint8_t displayST = (readedRSSI >> 8) & 0x01;

	memset(tmpBuff, 0, sizeof(tmpBuff));
	sprintf(tmpBuff, "S%1u%c", displayRSSI < 10 ? displayRSSI : 9, displayRSSI < 10 ? ' ' : '+');
	for (int i = 0; i < 15; i++)
	{
		tmpBuff[i + 3] = displayRSSI > i ? '}' : '{';
	}

	UI_PrintStringSmallNormal(tmpBuff, 0, 0, 5);

    //removed lowercase @ -> d, ; -> u
	sprintf(tmpBuff, "RSSI %2d@B;V", readRSSI);
	//sprintf(tmpBuff, "RSSI TEST TEST %d", 0);
	CEC_DisplaySmallest(tmpBuff, 1,  48, false, true);
    //uint16_t readedSNR = BK1080_ReadRegister(0x07);
    //uint16_t freqDevation = (readedSNR >> 4);

/*
	sprintf(strBuff, "DEV %4d", freqDevation);
	CEC_DisplaySmallest(strBuff, 55,  33, false, true);

	sprintf(strBuff, "SNR %2d", readedSNR & 0x0F);
	CEC_DisplaySmallest(strBuff, 100,  33, false, true);
*/
	CEC_DisplaySmallest("7  9 +10  +20 ", 59,  48, false, true);    //wrong signal value, but easy reading

	if (_isDraw)
	{
    	ST7565_BlitFullScreen();
	}
}

                         //0   1    2    3    4    5    6        7  8  9
//    uint16_t fmMem[10] = {891, 931, 959, 973, 885, 1070, 940, 1043, 0, 0};
uint16_t fmMem[10];// = {0};   //write 3 set 3*8 = 12, but only 10 channel uses

//1584 Byte
void CEC_FMRadio(void)
{
    //uint16_t fmRadioFreq = 959;
    uint8_t tmpBuff[16];
    //uint16_t fmMem[12];// = {0};   //write 3 set 3*8 = 12, but only 10 channel uses
    //uint16_t fmRadioFreq = 973;
    //uint16_t *fmMem;    // = {0};   //write 3 set 3*8 = 12, but only 10 channel uses
    //fmMem = (uint16_t * )&strBuff[0];

    //LOAD CHANNEL INFORMATION
	// 0E88..0E8F
    uint8_t backLightTime = 255;

    struct
    {
        uint16_t SelectedFrequency;
        uint8_t  SelectedChannel;
        uint8_t  IsMrMode;
        uint8_t  Padding[8];
    } __attribute__((packed)) FM;

    //CURRENT  Freq
    EEPROM_ReadBuffer(0x0E88, &FM, 8);
    if (FM.SelectedFrequency < FM_FREQ_MIN || FM.SelectedFrequency > FM_FREQ_MAX)
        FM.SelectedFrequency = 960;

    //Mem Freq just 10 Channel
	// 0E40..0E67
    EEPROM_ReadBuffer(0x0E40, fmMem, sizeof(fmMem));

	RADIO_SetupRegisters(true);
    BK_1080Control(CEC_FM_STARTUP, FM.SelectedFrequency);
	AUDIO_AudioPathOn();

    bool isNeedDraw = 1;
    uint8_t drawSpctrumCnt = 0;
    bool isCheckKeyRelease = 1;
    
    //CEC_WaitKeyRelease();

    while (1)
    {
        if (isNeedDraw)
        {
            isNeedDraw = 0;
            memset(gStatusLine, 0, sizeof(gStatusLine));
            memset(gFrameBuffer, 0, sizeof(gFrameBuffer));	

            sprintf(tmpBuff, "RSSI - %s", FM.SelectedChannel != 0x00 ? "ALWAYS" : "CHANGED");
            CEC_DisplaySmallest(tmpBuff, 2,  1, true, true);

            ST7565_BlitStatusLine();
            //DRAW DIAL ========================================================
            //UI_DrawRectangleBuffer(gFrameBuffer, 0 , 0, 127, 21, true);
            //UI_DrawLineBuffer(gFrameBuffer, 0, 0, 127, 0, true);
            //UI_DrawLineBuffer(gFrameBuffer, 0, 4, 127, 4, true);
            UI_DrawLineBuffer(gFrameBuffer, 0, 4, 127, 4, true);

            //128 /
            //310

            //Get Start Frequency
            uint16_t startFrequency = FM.SelectedFrequency - CENTER_POSITION;


            for (int i = 0; i < 128; i++)
            {
                uint16_t drawPosFreq = startFrequency + i;
                if ((drawPosFreq % 20) == 0)
                {
                    UI_DrawLineBuffer(gFrameBuffer, i, 2, i, 8, true);

                    if (i > 5 && i < 120)
                    {
	                    sprintf(tmpBuff, "%2u", drawPosFreq / 10);
	                    CEC_DisplaySmallest(tmpBuff, i - 5,  9, false, true);
                    }
                }
                else if ((drawPosFreq % 2) == 0)
                {
                    UI_DrawLineBuffer(gFrameBuffer, i, 3, i, 5, true);
                }

                //memory Check
                for (int j = 0; j < 10; j++)
                {
                    if (fmMem[j] == drawPosFreq)
                    {
	                    sprintf(tmpBuff, "M%u", j);
	                    CEC_DisplaySmallest(tmpBuff, i - 2,  15, false, true);
                    }
                }
                
            }

            //Current Freq Niddle
            UI_DrawLineBuffer(gFrameBuffer, 64, 0, 64, 21, true);
            UI_DrawLineBuffer(gFrameBuffer, 0, 21, 127, 21, true);

            //END OF DRAW DIAL =================================================



            sprintf(tmpBuff, "%3u.%1u", FM.SelectedFrequency / 10, (FM.SelectedFrequency) % 10);
            UI_DisplayFrequency(tmpBuff, 30, 3, false);
            UI_PrintStringSmallNormal("Mhz", 105, 0, 4);
            UI_PrintStringSmallNormal("FM", 1, 0, 4);

            CEC_FM_DrawRSSI(false);

            ST7565_BlitFullScreen();
        }

        if (isCheckKeyRelease)
        {
            isCheckKeyRelease = false;
            CEC_WaitKeyRelease();
        }
        //Check Buttons
		KEY_Code_t chkKey = KEYBOARD_Poll();
		if (chkKey != KEY_INVALID)
        {
            BACKLIGHT_TurnOn();
            //backLightTime = 50; //10 sec
            if (chkKey == KEY_UP || chkKey == KEY_DOWN || chkKey == KEY_SIDE1 || chkKey == KEY_SIDE2)
            {
                //62100
                uint16_t tmpFreq = FM.SelectedFrequency;
			    if (chkKey == KEY_UP)
				    tmpFreq++;
			    else if (chkKey == KEY_DOWN)
				    tmpFreq--;
			    else if (chkKey == KEY_SIDE1)
				    tmpFreq += 10;
			    else if (chkKey == KEY_SIDE2)
				    tmpFreq -= 10;

               if (tmpFreq >= FM_FREQ_MIN && tmpFreq <= FM_FREQ_MAX)
               {
                FM.SelectedFrequency = tmpFreq;
                BK_1080Control(CEC_FM_SET_FREQ, FM.SelectedFrequency);
               }
            }
            else if (chkKey <= KEY_9)
            {
                //CHECK SHORT OR LONG PRESS
                bool isLongPress = true;
                for (int i = 0; i < 20; i++)
                {
                    if(KEYBOARD_Poll() == KEY_INVALID)
                    {
                        isLongPress = false;
                        break;
                    }
                    delay(100); //Wait 2Sec  100 * 20
                }

                if (isLongPress)
                {
                    //Save memory
                    fmMem[chkKey - KEY_0] = FM.SelectedFrequency;
                    //for (i = 0; i < 5; i++)
                    for (int i = 0; i < 3; i++) //just using 10 channel 
                        EEPROM_WriteBuffer(0x0E40 + (i * 8), &fmMem[i * 4]);    //Last 4 Byte gabage data saved (ignore that)
                }
                else if (fmMem[chkKey - KEY_0] >= FM_FREQ_MIN && fmMem[chkKey - KEY_0] <= FM_FREQ_MAX)
                {
                    FM.SelectedFrequency = fmMem[chkKey - KEY_0];
                    BK_1080Control(CEC_FM_SET_FREQ, FM.SelectedFrequency);
                    isCheckKeyRelease = true;
                }
            }
            else if (chkKey == KEY_EXIT)
            {
                //SAVE AND EXIT
                break;
            }
            else if (chkKey == KEY_F)
            {
                FM.SelectedChannel = ! FM.SelectedChannel;
                isCheckKeyRelease = true;
            }
            isNeedDraw = true;
        }


        delay(200);
        if (++drawSpctrumCnt > 5)
        {
            drawSpctrumCnt = 0;
            if (FM.SelectedChannel)
                CEC_FM_DrawRSSI(true);
        }

        //idle 0 -> 255, so useless process, but reduce program memory (not if using)
        //50 sec
        if (--backLightTime < 1)
        {
            BACKLIGHT_TurnOff();
        }
    }   //end of while

    EEPROM_WriteBuffer(0x0E88, &FM);
    BK_1080Control(CEC_FM_STOP, 0);
    AUDIO_AudioPathOff();
}