/* Copyright 2023 KD8CEC
 * https://github.com/kd8cec
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

#include "ceccommon.h"
typedef struct 
{
    uint8_t LOW_VALUE;
    uint8_t HIGH_VALUE;
} TADC_RANGE;

uint8_t CW_TONE          = 70;    //*10 Hz, because BK4819 using 10Hz Step, as YAESU : PITCH, ICOM,KENWOOD : TONE
uint8_t CW_SPEED         = 10;    //WPM 5 ~ 50
uint8_t CW_KEYTYPE       =  0;   
uint8_t CEC_LiveSeekMode = 0;   //0:NONE, 1:LIVE, 2:LIVE+1, 3:LIVE+2
TADC_RANGE CW_KEY_ADC[3];

//================================= For Reduce Use Memory ====================
//gMR_ChannelAttributes[207]; -> function
//resize to 7 byte and Move to ceccommon.c 
uint16_t CEC_GetRssi() 
{
  int _wait_count = 0;
  while (((BK4819_ReadRegister(0x63) & 0b11111111) >= 255 ) && _wait_count++ < 100)  
    SYSTICK_DelayUs(100);

  return BK4819_GetRSSI();
}

void DisplayIntLog(char* displayMessage, int _logInt1, int _logInt2)
{
	uint8_t tmpBuff[32];

	memset(gStatusLine,  0, sizeof(gStatusLine));
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

	sprintf(tmpBuff, "%s : %d", displayMessage, _logInt1);
	UI_PrintString(tmpBuff, 2, 127, 0, 8);

	sprintf(tmpBuff, "1: %u", _logInt1);
	UI_PrintString(tmpBuff, 2, 127, 2, 8);

	sprintf(tmpBuff, "2: %u", _logInt2);
	UI_PrintString(tmpBuff, 2, 127, 4, 8);

	ST7565_BlitFullScreen();
}

//-----------------------------------------------------------------------------------------
//=================== Variables shared and used in various places , To Rduce memory usage
uint8_t CommBuff[COMBUFF_LENGTH];  //for Common Use for 
uint8_t CommBuffUsingType = 0;
uint32_t CommBuffLastUseTime = 0;   //10milisec using millis10() function
uint8_t CommValue1 = 0;             //temp varaible
uint8_t CommValue2 = 0;
uint8_t CommValue3 = 0;
uint8_t strBuff[32];    //For sprintf
//-----------------------------------------------------------------------------------------
//=================== End of Common Variables =============================================

uint8_t lastSeekDirection = 0;
uint32_t rssiStartFreq = 0;
uint32_t addRssiCount = 0;


void UI_PrintStringSmallLeft(const char *pString, uint8_t Start, uint8_t End, uint8_t Line)
{
	const size_t Length = strlen(pString);
	size_t       i;

	const unsigned int char_width   = ARRAY_SIZE(gFontSmall[0]);
	const unsigned int char_spacing = char_width + 1;

	uint8_t            *pFb         = gFrameBuffer[Line] + Start;
	for (i = 0; i < Length; i++)
	{
		if (pString[i] > ' ')
		{
			const unsigned int index = (unsigned int)pString[i] - ' ' - 1;
			if (index < ARRAY_SIZE(gFontSmall))
				memmove(pFb + (i * char_spacing) + 1, &gFontSmall[index], char_width);
		}
	}
}

/*
void DrawFrequencySmall(uint32_t _frequency, int _startX, int _Length, int _lineNumber)
{
    sprintf(strBuff, "%3u.%03u", _frequency / 100000, (_frequency / 100) % 1000);
    UI_PrintStringSmallLeft(strBuff, _startX, _startX +  _Length, _lineNumber);
}
*/
void DrawFrequencySmall(uint32_t _frequency, int _startX, int _Length, int _lineNumber)
{
    //  58096      48    3284   61428    eff4 firmware
    //  58036      48    3284   61368    efb8 firmware
    //sprintf(strBuff, "%1u.%03u", (_frequency / 100000) % 10, (_frequency / 100) % 1000);
    sprintf(strBuff, "%3u.%03u", _frequency / 100000, (_frequency / 100) % 1000);
    //UI_PrintStringSmallLeft(strBuff, _startX, _startX +  _Length, _lineNumber);
    memset(gFrameBuffer[_lineNumber], 0, 128);
    UI_PrintStringSmallNormal(strBuff, _startX, 0, _lineNumber);
}

void DrawCommBuffToSpectrum(void)
{
    //Low Value
    int _lowValue = 999;
    int drawYPosition = 53;

    if (CEC_LiveSeekMode < LIVESEEK_RCV_SPECTRUM1)
        return;


    if (addRssiCount < 3)
        return;

    for (int i = 0; i < COMBUFF_LENGTH; i++)    
    {
        if (CommBuff[i] > 0 && _lowValue > CommBuff[i])
            _lowValue = CommBuff[i];
    }

    //DrawFrequencyKhzSmall(gTxVfo->freq_config_RX.Frequency, lastSeekDirection == 12 ? 100 : 0, 55, 3);

    //DrawFrequencyKhzSmall(gTxVfo->freq_config_RX.Frequency + (gTxVfo->StepFrequency * 64) * (lastSeekDirection == 12 ? -1 : 1), 
    //    57, 55, 3);

    //DrawFrequencyKhzSmall(gTxVfo->freq_config_RX.Frequency + (gTxVfo->StepFrequency * 127) * (lastSeekDirection == 12 ? -1 : 1), 
    //    lastSeekDirection == 12 ? 0 : 100, 55, 3);
    memset(gFrameBuffer[6], 0, 128);    //Clear Last Line

    for (int i = 0; i < COMBUFF_LENGTH; i++)
    {
        int _drawXPosition = (lastSeekDirection == 10 ? 127 - i : i);
        int _drawYValue = CommBuff[i] - _lowValue;
        if (_drawYValue > 50)
            _drawYValue = 50;
        else if (_drawYValue < 0)
            _drawYValue = 0;
        UI_DrawLineBuffer(gFrameBuffer, _drawXPosition, drawYPosition, _drawXPosition, (drawYPosition - _drawYValue), true);
    }
    uint32_t _drawFreq = rssiStartFreq;
    int _drawTextPosition = 0;
    //int _drawTextPosition = lastSeekDirection == 12 ? 127 - addRssiCount: 0 + addRssiCount;
    if (lastSeekDirection == 12)
    {
        _drawTextPosition = 127 - addRssiCount;
        if (_drawTextPosition > 73)
            _drawTextPosition = 73;
        else if (_drawTextPosition < 0)
        {
            _drawTextPosition = 0;
            _drawFreq = gTxVfo->freq_config_RX.Frequency - (gTxVfo->StepFrequency * 127);
        }
    }
    else
    {
        _drawTextPosition = addRssiCount - 55;

        if (_drawTextPosition < 0)
            _drawTextPosition = 0;
        else if (_drawTextPosition > 73)
        {
            _drawTextPosition = 73;
            _drawFreq = gTxVfo->freq_config_RX.Frequency + (gTxVfo->StepFrequency * 127);
        }
    }

    DrawFrequencySmall(_drawFreq,  _drawTextPosition, 55, 3);
    ST7565_BlitFullScreen();
}

//500msec interval execute function
void CEC_TimeSlice500ms(void)
{
    if (CommBuffUsingType == COMBUFF_USE_SEEK_RSSI && (millis10() -CommBuffLastUseTime > 70))
    {
        //Clear
        CommBuffUsingType = COMBUFF_USE_SEEK_NONE;
        if (gScreenToDisplay == DISPLAY_MAIN)
        {
            gMonitor = false;

            if (addRssiCount > 3)
            {
                if (CommValue1 != FUNCTION_MONITOR)
                    RADIO_SetupRegisters(true);

                UI_DisplayMain();
            }
        }

    }

}

//Frequency Apply Receive Mode
//DEFAULT 
//_applyOption 0 : No Delaytime
#define STOP_RSSI_LIMIT 50
#define STOP_RSSI_TIME 500

void CEC_ApplyChangeRXFreq(int _applyOption)
{
    if (CEC_LiveSeekMode == LIVESEEK_NONE)
        return;

    BK4819_SetFrequency(gTxVfo->freq_config_RX.Frequency);
    BK4819_RX_TurnOn();

    //BK4819_SetFrequency(frequency);
    //APP_StartListeningLive(FUNCTION_MONITOR, false);
    //if (gEeprom.SQUELCH_LEVEL == 0)
    //    AUDIO_AudioPathOn();
    int32_t tmpRssi = CEC_GetRssi() / 3;
    
    tmpRssi = (tmpRssi < 0 ? 0 : tmpRssi);

    if (_applyOption >= 10 && _applyOption <= 12)    //Save RSSI Result, 10 : Direction -1,  12 : Direction : 1
    {
        CommBuffUsingType = COMBUFF_USE_SEEK_RSSI;
        if (lastSeekDirection != _applyOption || (millis10() -CommBuffLastUseTime > 70))
        {
            memset(CommBuff, 0, sizeof(CommBuff));
            rssiStartFreq = gTxVfo->freq_config_RX.Frequency;
            addRssiCount = 0;
            CommValue1 = gCurrentFunction;
        }

        addRssiCount++;
        lastSeekDirection = _applyOption;

        int _insertIndex = COMBUFF_LENGTH -1;
        if (addRssiCount < COMBUFF_LENGTH /2)
        {
            _insertIndex = COMBUFF_LENGTH /2 + addRssiCount;
        }
        else
        {
            //for (int i = 0; i < COMBUFF_LENGTH -1; i++)
            //    CommBuff[i] = CommBuff[i + 1];
            memmove(&CommBuff[0], &CommBuff[1], sizeof(CommBuff) - 1);            
        }
        CommBuff[_insertIndex] = tmpRssi > 255 ? 255 : tmpRssi;
        CommBuffLastUseTime = millis10();
    }


    if (gEeprom.SQUELCH_LEVEL == 0)
    {
        if (addRssiCount > 2)
            APP_StartListening(FUNCTION_MONITOR);
        //delay(200);
    }
    else if (tmpRssi > STOP_RSSI_LIMIT)
    {
        APP_StartListening(FUNCTION_MONITOR);
        delay(STOP_RSSI_TIME);
        RADIO_SetupRegisters(true);
    }
}