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
#include "cecsstv1.h"
#include "cecmorse.h"
#include "bsp\dp32g030\portcon.h"
#include "bsp\dp32g030\syscon.h"
#include "bsp\dp32g030\saradc.h"
#include "driver/adc.h"
#include "radio.h"
#include "driver\st7565.h"
#include "misc.h"
#include "settings.h"
#include "driver\keyboard.h"
#include "functions.h"
#include "audio.h"



//KD8CEC GLOBAL VARIABLE LIST -----------------------------------------------
//uint8_t CW_Tone = 70;    //*10 Hz, because BK4819 inc/dec 10Hz
//uint8_t CW_SPEED = 10;
uint8_t SSTV_LCD_Start_Timer = 0;
uint8_t CEC_LiveSeekMode = 2;   //0:NONE, 1:LIVE, 2:LIVE+1, 3:LIVE+2

uint8_t TXViaUART = 0;  //prevent highvoltage error, check error with uart

#ifdef MENU_SSB_FLT
uint8_t CEC_SSB_FLT = 0; 
#else
uint8_t CEC_SSB_FLT = 5;  //Default Value is 2K+ (Best Performance)
#endif

//================================= For Reduce Use Memory ====================
//gMR_ChannelAttributes[207]; -> function
//resize to 7 byte and Move to ceccommon.c 
ChannelAttributes_t MR_ChannelAttributes(int _channelIndex)
{
    if (_channelIndex < _MAX_READ_CH_ATTRIBUTES)
        return gMR_ChannelAttributes[_channelIndex];
        
    uint8_t _ChannelAttributes = 0x00;
    EEPROM_ReadBuffer(0x0D60 + _channelIndex, &_ChannelAttributes, 1);
    ChannelAttributes_t att = (ChannelAttributes_t)_ChannelAttributes;
    if(att.__val == 0xff){
        att.__val = 0;
        att.band = 0xf;
    }
    return att;
}

void SetMR_ChannelAttributes(int _channelIndex, ChannelAttributes_t _channelAtt)
{
    if (_channelIndex < _MAX_READ_CH_ATTRIBUTES)
        gMR_ChannelAttributes[_channelIndex] = _channelAtt;
}

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
uint8_t DigitalMode = 0;  //OFF


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
    uint8_t tmpBuff[16];
    sprintf(tmpBuff, "%3u.%03u", _frequency / 100000, (_frequency / 100) % 1000);
    //UI_PrintStringSmallLeft(strBuff, _startX, _startX +  _Length, _lineNumber);
    memset(gFrameBuffer[_lineNumber], 0, 128);
    UI_PrintStringSmallNormal(tmpBuff, _startX, 0, _lineNumber);
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

        if (_drawYValue > 8)
            _drawYValue = 8;
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

    if (SSTV_LCD_Start_Timer > 0)
    {
        if (--SSTV_LCD_Start_Timer == 0)
            StartSSTVM1(2);	//LCD SCREEN
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


void UART_SendByte(uint8_t _sendByte)
{
	UART1->TDR = _sendByte;
	while ((UART1->IF & UART_IF_TXFIFO_FULL_MASK) != UART_IF_TXFIFO_FULL_BITS_NOT_SET) {
	}
}


void StoreCWMemory(int channelIndex, char * _srcBuff, uint8_t srcLen);

char MaskingChar(char srcChar)
{
    return srcChar == 0x00 || srcChar >= 0xF0 ? ' ' : '*';
}

#define CWTEXT_MAIN_LENGTH   		 30	//memory length
#define CWTEXT_MAIN_DISPLAY_LENGTH   15	//display length
int CWTextMainIdex = 0;
char CWTextMainBuff[CWTEXT_MAIN_LENGTH + 1] = {0};
void WriteCharDisplay(char writeChar, int decodeType, int option1, int option2)
{
	unsigned int activeTxVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;  
	const unsigned int _lineNumber       = (activeTxVFO == 0) ? 3 : 0;

	//const unsigned int _lineNumber       = 3;
	memset(gFrameBuffer[_lineNumber], 0, 128);
	memset(gFrameBuffer[_lineNumber + 1], 0, 128);
	memset(gFrameBuffer[_lineNumber + 2], 0, 128);
	memset(gFrameBuffer[_lineNumber + 3], 0, 128);

	if (decodeType == 100)	//All Clear
	{
		memset(CWTextMainBuff, 0, sizeof(CWTextMainBuff));
		CWTextMainIdex = 0;
	}
	//else if (decodeType >= 50 && decodeType <= 90)
	else if (decodeType == 50)  // && decodeType <= 90) Change Range to Just 1 QSO Data Store with CW Message Mode (input by Menu)
	{   //Quick Button (DX CALL)
		//Stored Memory
		//StoreCWMemory(decodeType - 50, CWTextMainBuff, CWTextMainIdex);
        SETTINGS_SaveChannelName(175, CWTextMainBuff);
	}
	else if (decodeType == 51)  // QSO Temp Data Save (Just One)
	{   //Quick Button (DX CALL)
		//Stored Memory
		//StoreCWMemory(decodeType - 50, CWTextMainBuff, CWTextMainIdex);
        CWTextMainBuff[CWTextMainIdex] = 0;
        //CWTextMainBuff[CWTextMainIdex] = 0;
        for (int i = 0; i < 4; i++)
            EEPROM_WriteBuffer(CEC_EEPROM_CWQSODATA + (i * 8),  CWTextMainBuff + (i * 8));
        //text    data     bss     dec     hex filename
        //57872      20    2728   60620    eccc firmware
        /*
        //same size
	    EEPROM_WriteBuffer(CEC_EEPROM_CWQSODATA + 0,  CWTextMainBuff + 0);
	    EEPROM_WriteBuffer(CEC_EEPROM_CWQSODATA + 8,  CWTextMainBuff + 8);
	    EEPROM_WriteBuffer(CEC_EEPROM_CWQSODATA + 16, CWTextMainBuff + 16);
	    EEPROM_WriteBuffer(CEC_EEPROM_CWQSODATA + 24, CWTextMainBuff + 24);
        */
	}
	else if (decodeType < 100)	//ignore more than 100
	{
		if (CWTextMainIdex >= CWTEXT_MAIN_LENGTH)
		{
			for (int i = 0; i < CWTEXT_MAIN_LENGTH-1; i++)
				CWTextMainBuff[i] = CWTextMainBuff[i + 1];
			CWTextMainBuff[CWTEXT_MAIN_LENGTH -1] = writeChar;
		}
		else
		{
			CWTextMainBuff[CWTextMainIdex++] = writeChar;
		}
	}	

	//CWTextMainIdex
	//UI_PrintStringSmallLeft(CWTextSecondBuff, 0, 127, _lineNumber);
	//12345678901234567890
	//     |
	UI_PrintString(&CWTextMainBuff[CWTextMainIdex <= CWTEXT_MAIN_DISPLAY_LENGTH ? 0 :  CWTextMainIdex - CWTEXT_MAIN_DISPLAY_LENGTH], 0, 0, _lineNumber + 0, 8);

	if (_lineNumber == 3)
    	UI_DrawLineBuffer(gFrameBuffer, 0, 23, 127, 23, true);
	else
		UI_DrawLineBuffer(gFrameBuffer, 0, 32, 127, 32, true);

    if (option2 == 1)  //Display Quick Send Menu
    {
        uint8_t existsData[9] = {0};

        for (int i = 0; i < 9; i++)
            EEPROM_ReadBuffer(CEC_EEPROM_CWMSG0 + (i * 16), existsData + i, 1);            

/*
        //STYLE #1
        EEPROM_ReadBuffer((EEPROM_CHANNELNAME + RIGINFO_MSG_DXCALL * 16), existsData + 10, 1);            
        EEPROM_ReadBuffer((EEPROM_CHANNELNAME + RIGINFO_MSG_MYCALL * 16), existsData + 11, 1);            

        sprintf(tmpBuff, "1%c 2%c 3%c 4%c 5%c <%c", MaskingChar(existsData[1]), MaskingChar(existsData[2]), 
            MaskingChar(existsData[3]), MaskingChar(existsData[4]), MaskingChar(existsData[5]), MaskingChar(existsData[10]));
        UI_PrintStringSmallLeft(tmpBuff, 0, 127, _lineNumber + 2);

        sprintf(tmpBuff, "6%c 7%c 8%c 9%c 0%c @%c", MaskingChar(existsData[6]), MaskingChar(existsData[7]), 
            MaskingChar(existsData[8]), MaskingChar(existsData[9]) , MaskingChar(existsData[0]), MaskingChar(existsData[10]));
        UI_PrintStringSmallLeft(tmpBuff, 0, 127, _lineNumber + 3);
*/
        //STYLE #2
        //[DXCALL]        
        //12345678 1* 2* 3* 4*
        //4* 5* 6* 7* 8* 9* 0*
        uint8_t dxCallBuff[10];

        SETTINGS_FetchChannelName(dxCallBuff, RIGINFO_MSG_DXCALL);
        dxCallBuff[9] = 0x00;   //substring
        sprintf(strBuff, "%-9s0%c 1%c 2%c", dxCallBuff, MaskingChar(existsData[0]), MaskingChar(existsData[1]), 
            MaskingChar(existsData[2]));
        UI_PrintStringSmallNormal(strBuff, 0, 0, _lineNumber + 2);

        sprintf(strBuff, "3%c 4%c 5%c 6%c 7%c 8%c", MaskingChar(existsData[3]), MaskingChar(existsData[4]), 
            MaskingChar(existsData[5]), MaskingChar(existsData[6]) , MaskingChar(existsData[7]), MaskingChar(existsData[8]));
        UI_PrintStringSmallNormal(strBuff, 0, 0, _lineNumber + 3);
    }
    else
    {
        UI_PrintStringSmallNormal("1:DXC 2:CLR 3:MEM", 0, 0, _lineNumber + 2);
        if (option1 == 0)	//Space Auto
            UI_PrintStringSmallNormal("4.HLD 5:SPC 6:PLY", 0, 0, _lineNumber + 3);
        else
            UI_PrintStringSmallNormal("4:HLD 5.SPC*6:PLY", 0, 0, _lineNumber + 3);
    }

	ST7565_BlitFullScreen();
}

void CWDecodedChar(char _decodedChar, int decodeType, int _option1, int _option2)
{
	if ((decodeType >= CW_DECODE_SPACE  && decodeType <= CW_DECODE_SYMBOLE) || decodeType > 30)
		WriteCharDisplay(_decodedChar, decodeType, _option1, _option2);

}



//O : GPIO MODE
//1 : UART MODE (SYSTEM DEFAULT)
//2 : ADC MODE FOR CW PADDEL AND KEY
//3 : GPS MODE BAUD 9600
void SetRX1Mode(int rx1Mode)
{
  //when digitalMode, Only using RS232 Highspeed 1152K UART Mode
  if (DigitalMode)
    return;

  if (rx1Mode == 0) //GPIO
  {
    //*************************************************************************************************
    //Test OK  (by Ianlee) #2	//RX -> GPIO OUT
    PORTCON_PORTA_SEL1 &= ~(PORTCON_PORTA_SEL1_A8_MASK);
    PORTCON_PORTA_SEL1 |= PORTCON_PORTA_SEL1_A8_BITS_GPIOA8;
    GPIOA->DIR |= GPIO_DIR_8_BITS_OUTPUT;
    PORTCON_PORTA_IE &= ~(0 | PORTCON_PORTA_IE_A8_MASK);
    PORTCON_PORTA_PU &= ~(0 | PORTCON_PORTA_PU_A8_MASK);
    PORTCON_PORTA_PD &= ~(0 | PORTCON_PORTA_PD_A8_MASK);
    PORTCON_PORTA_OD &= ~(0 | PORTCON_PORTA_OD_A8_MASK);

    //GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);	//flash light
    GPIO_SetBit(&GPIOA->DATA, 8);
    //GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);	//flash light
    GPIO_ClearBit(&GPIOA->DATA, 8);

    //End of Test for GPIO  ***********************************************************************
  }
  else if (rx1Mode == 1)  //UART 38400
  {
    //SYSTEM DEFAULT
    UART_Init(UART_BAUD_38400_CLOCK_DIV);
  }
  else if (rx1Mode == 2)  //ADC MODE FOR CWKEY
  {
      // *************************************************************************************************
      //Test by Ianlee (OK) #1	//RX -> ADC로 (Success)
      PORTCON_PORTA_SEL1 |= 0
        // UART1 RX, wasn't cleared in previous step / relying on default value!
        //| PORTCON_PORTA_SEL1_A8_BITS_UART1_RX
        | PORTCON_PORTA_SEL1_A8_BITS_SARADC_CH3
        // Battery voltage, wasn't cleared in previous step / relying on default value!
        | PORTCON_PORTA_SEL1_A9_BITS_SARADC_CH4
        // Key pad + I2C
        | PORTCON_PORTA_SEL1_A10_BITS_GPIOA10
        // Key pad + I2C
        | PORTCON_PORTA_SEL1_A11_BITS_GPIOA11
        // Key pad + Voice chip
        | PORTCON_PORTA_SEL1_A12_BITS_GPIOA12
        // Key pad + Voice chip
        | PORTCON_PORTA_SEL1_A13_BITS_GPIOA13
        // Battery Current, wasn't cleared in previous step / relying on default value!
        | PORTCON_PORTA_SEL1_A14_BITS_SARADC_CH9
        ;

      ADC_Config_t Config;

      Config.CLK_SEL            = SYSCON_CLK_SEL_W_SARADC_SMPL_VALUE_DIV2;
      Config.CH_SEL             = ADC_CH4 | ADC_CH9 | ADC_CH3;
      Config.AVG                = SARADC_CFG_AVG_VALUE_8_SAMPLE;
      Config.CONT               = SARADC_CFG_CONT_VALUE_SINGLE;
      Config.MEM_MODE           = SARADC_CFG_MEM_MODE_VALUE_CHANNEL;
      Config.SMPL_CLK           = SARADC_CFG_SMPL_CLK_VALUE_INTERNAL;
      Config.SMPL_WIN           = SARADC_CFG_SMPL_WIN_VALUE_15_CYCLE;
      Config.SMPL_SETUP         = SARADC_CFG_SMPL_SETUP_VALUE_1_CYCLE;
      Config.ADC_TRIG           = SARADC_CFG_ADC_TRIG_VALUE_CPU;
      Config.CALIB_KD_VALID     = SARADC_CALIB_KD_VALID_VALUE_YES;
      Config.CALIB_OFFSET_VALID = SARADC_CALIB_OFFSET_VALID_VALUE_YES;
      Config.DMA_EN             = SARADC_CFG_DMA_EN_VALUE_DISABLE;
      Config.IE_CHx_EOC         = SARADC_IE_CHx_EOC_VALUE_NONE;
      Config.IE_FIFO_FULL       = SARADC_IE_FIFO_FULL_VALUE_DISABLE;
      Config.IE_FIFO_HFULL      = SARADC_IE_FIFO_HFULL_VALUE_DISABLE;

      ADC_Configure(&Config);
      ADC_Enable();
      ADC_SoftReset();


      //PULL DOWN (already Hardware Pull Up and 3.5mm Connect without GND, so h/w pullup and h/w pull down -> almost 2.5 Volt, we using half range)
      //PORTCON_PORTA_SEL1 &= ~(PORTCON_PORTA_SEL1_A8_MASK);
      //PORTCON_PORTA_SEL1 |= PORTCON_PORTA_SEL1_A8_BITS_GPIOA8;
      //GPIOA->DIR |= GPIO_DIR_8_BITS_INPUT;
      //PORTCON_PORTA_IE &= ~(0 | PORTCON_PORTA_IE_A8_MASK);
      PORTCON_PORTA_PU &= ~(0 | PORTCON_PORTA_PU_A8_MASK);
      //PORTCON_PORTA_PU &= 0 | PORTCON_PORTA_PU_A8_BITS_ENABLE;
      //PORTCON_PORTA_PD &= ~(0 | PORTCON_PORTA_PD_A8_MASK);
      PORTCON_PORTA_PD |= 0 | PORTCON_PORTA_PD_A8_BITS_ENABLE;
      //PORTCON_PORTA_OD &= ~(0 | PORTCON_PORTA_OD_A8_MASK);

      //GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);	//flash light
      //GPIO_SetBit(&GPIOA->DATA, 8);
      //GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_FLASHLIGHT);	//flash light
      //GPIO_ClearBit(&GPIOA->DATA, 8);

      //End of ADC Test ***********************************************************************
  }
  else if (rx1Mode == 3) //GPS MODE BAUD 9600
  {
    //UART_Init9600();
    UART_Init(UART_BAUD_9600_CLOCK_DIV);
  }
}

void BackLightBlink(int _cnt)
{
  for (int i = 0; i < _cnt; i++)
  {
    BACKLIGHT_TurnOn();
    SYSTEM_DelayMs(200);
    BACKLIGHT_TurnOff();
    SYSTEM_DelayMs(200);
  }
}

//READY TX MODE 
void PrepareSWFSKTX()
{
    //LED OFF
  BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, false);

  RADIO_PrepareTX();
	//BK4819_SetFrequency(frequency);
  BK4819_PrepareTransmit();
	//BK4819_EnterTxMute();
	//BK4819_SetAF(BK4819_AF_MUTE);
  //below 2 line added for sending with side tone

#ifdef ENABLE_SSTV_APRS_SIDETOME  
  AUDIO_AudioPathOn();
  BK4819_SetAF(BK4819_AF_BEEP);
#endif

  #define FM_MODE_GAIN_LEVEL 100
  BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((FM_MODE_GAIN_LEVEL & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
  BK4819_EnableTXLink();
  BK4819_ExitTxMute();  
}

//using CW, SSTV -> Recovery Transceive Mode
void RestoreReceiveMode()
{
  BK4819_WriteRegister(0x40, 0x4D0 | (0x01U << BK4819_REG_40_SHIFT_ENABLE_DEVIATION));
  RADIO_SelectVfos();
	gUpdateStatus   = true;

#ifdef ENABLE_NOAA
	RADIO_ConfigureNOAA();
#endif
  gMonitor = false;
	RADIO_SetupRegisters(true);
	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
  gUpdateDisplay = true; 
  CECTimer0Disable();
}


double StrToFloat(const char *s)
{
  // This function stolen from either Rolf Neugebauer or Andrew Tolmach. 
  // Probably Rolf.
  double a = 0.0;
  int e = 0;
  int c;
  /*
  while ((c = *s++) != '\0' && isdigit(c)) {
    a = a*10.0 + (c - '0');
  }
  if (c == '.') {
    while ((c = *s++) != '\0' && isdigit(c)) {
      a = a*10.0 + (c - '0');
      e = e-1;
    }
  }
  */
  while ((c = *s++) != '\0' && c != '.') {
    a = a*10.0 + (c - '0');
  }
  if (c == '.') {
    while ((c = *s++) != '\0') {
      a = a*10.0 + (c - '0');
      e = e-1;
    }
  }

  /*
  if (c == 'e' || c == 'E') {
    int sign = 1;
    int i = 0;
    c = *s++;
    if (c == '+')
      c = *s++;
    else if (c == '-') {
      c = *s++;
      sign = -1;
    }
    while (isdigit(c)) {
      i = i*10 + (c - '0');
      c = *s++;
    }
    e += i*sign;
  }
  */
 /*
  while (e > 0) {
    a *= 10.0;
    e--;
  }
*/  
  while (e < 0) {
    a *= 0.1;
    e++;
  }
  return a;
}

/* DATA TYPE  : !
   * LAT        : ddmm.ssN or ddmm.ssS
   * LON        : dddmm.ssE or dddmm.ssW
*/
//GPS Location Value to APRS Format
//Very Large Program memory (perhaps using float? ) using 5~7kbyte
void GPSToAPRS2(char * _gpsSTR, bool _isLat)
{
	char _ns;
	//27008
	char * gpsSTR = _gpsSTR;
	if (gpsSTR[0] == '-')
	{
		_ns = _isLat ? 'S' : 'W';
		gpsSTR++;
	}
	else
	{
		_ns = _isLat ? 'N' : 'E';
	}

	float gpsPosF = StrToFloat(gpsSTR);

	uint16_t tmpDeg = (int)gpsPosF;	//Int Section
	//gpsPosF = (gpsPosF - tmpDeg) * 10000 * (60.0 / 10000.0);
	gpsPosF = (gpsPosF - tmpDeg) * 60.0;	//decrease program memory 27004 -> 26840
	uint16_t tmpMin = gpsPosF;	//(int)(gpsPosF) <-- increase   27356 -> 26840
	gpsPosF = gpsPosF - tmpMin;
	//uint16_t tmpMinSub = (int)(gpsPosF * 100 + 0.5);	//round
	uint16_t tmpMinSub = (int)(gpsPosF * 100);	//floor  ()  increase 128 byte program memory, so ignore

	sprintf(_gpsSTR, _isLat ? "%02d%02d.%02d%c" : "%03d%02d.%02d%c", tmpDeg, tmpMin, tmpMinSub, _ns);	//because not supprt %f char
}


//by KD8CEC, just using integer type 
void CEC_GPSToAPRS(char * _gpsSTR, bool _isLat)
{
	char _ns;
	//27008
	char * gpsSTR = _gpsSTR;
	if (gpsSTR[0] == '-')
	{
		_ns = _isLat ? 'S' : 'W';
		gpsSTR++;
	}
	else
	{
		_ns = _isLat ? 'N' : 'E';
	}

	int pointPosition = -1;
	int strLength = -1;
	uint16_t tmpDeg = 0;	//Int Section
	uint32_t tmpMin = 0;
	int8_t posEE = 0;

	for (int i = 0; i < 11; i++)
	{
		char __c = gpsSTR[i];
		if (__c == '.')
		{
			pointPosition = i;
		}
		else if (__c == '\0')
		{
			break;
		}
		else
		{
			/*
			tmpDeg  = tmpDeg * 10 + (__c - '0');
			if (pointPosition >= 0)
			{
				positonEE--;
			}
			*/

			if (pointPosition < 0)
			{
				tmpDeg  = tmpDeg * 10 + (__c - '0');
			}
			else
			{
				tmpMin  = tmpMin * 10 + (__c - '0');
				//point
				posEE++;
			}
		}
	}


	tmpMin = tmpMin * 6;	//4062
	sprintf(strBuff, "%u", tmpMin);
	int tmpLength = strlen(strBuff);
	char tmpBuff[4] = {'0', '0', '0', '0'};
	int writeIndex = posEE - tmpLength + 1;

	for (int i = 0; i < tmpLength; i++)
	{
		tmpBuff[writeIndex++] = strBuff[i];
		if (writeIndex > 3)
			break;
	}
	char tmpDegStr[4];
	sprintf(tmpDegStr, _isLat ? "%02d" : "%03d", tmpDeg);
	sprintf(_gpsSTR, "%s%c%c.%c%c%c", tmpDegStr, tmpBuff[0], tmpBuff[1], tmpBuff[2], tmpBuff[3], _ns);	//because not supprt %f char
	//sprintf(_gpsSTR, _isLat ? "%02d%02d.%02d%c" : "%03d%02d.%02d%c", tmpDeg, tmpMin, tmpMinSub, _ns);	//because not supprt %f char
}


void CEC_ReverseScreen(uint8_t * _srcBuff, int _buffSize)
{
    //reverse
    for (int i = 0; i < _buffSize; i++)
        _srcBuff[i] = ~_srcBuff[i];
}

void CEC_DisplaySmallest(const char *pString, uint8_t x, uint8_t y, bool statusbar, bool fill)
{
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
    
  /*
  uint8_t c;
  uint8_t pixels;
  const uint8_t *p = (const uint8_t *)pString;
  while ((c = *p++) && c != '\0') 
  {
    //c -= 0x20;
    if (c >= '.')
    {
      c -= '.';
      c *= 3;
      const uint8_t *gFont3x5_part = &gFontBig[90][0];
      gFont3x5_part += c;
      for (int i = 0; i < 3; ++i) 
      {
        //pixels = gFont3x5_part[c][i];
        pixels = gFont3x5_part[i];
          
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
  */
}



#define     __IM     volatile const      /*! Defines 'read only' structure member permissions */
#define     __OM     volatile            /*! Defines 'write only' structure member permissions */
#define     __IOM    volatile            /*! Defines 'read / write' structure member permissions */

typedef struct
{
  __IOM uint32_t CTRL;                   /*!< Offset: 0x000 (R/W)  SysTick Control and Status Register */
  __IOM uint32_t LOAD;                   /*!< Offset: 0x004 (R/W)  SysTick Reload Value Register */
  __IOM uint32_t VAL;                    /*!< Offset: 0x008 (R/W)  SysTick Current Value Register */
  __IM  uint32_t CALIB;                  /*!< Offset: 0x00C (R/ )  SysTick Calibration Register */
} SysTick_Type;

#define SCS_BASE            (0xE000E000UL)                            /*!< System Control Space Base Address */
#define SysTick_BASE        (SCS_BASE +  0x0010UL)                    /*!< SysTick Base Address */
#define NVIC_BASE           (SCS_BASE +  0x0100UL)                    /*!< NVIC Base Address */
#define SCB_BASE            (SCS_BASE +  0x0D00UL)                    /*!< System Control Block Base Address */

#define SysTick             ((SysTick_Type   *)     SysTick_BASE  )   /*!< SysTick configuration struct */

void SYSTICK_DelayUs_HS(uint32_t Delay)
{
	const uint32_t ticks    = Delay * 2;
	uint32_t       i        = 0;
	uint32_t       Start    = SysTick->LOAD;
	uint32_t       Previous = SysTick->VAL;
	do {
		uint32_t Current;
		uint32_t Delta;
		while ((Current = SysTick->VAL) == Previous) {}
		Delta    = (Current < Previous) ? -Current : Start - Current;
		i       += Delta + Previous;
		Previous = Current;
	} while (i < ticks);
}

//#define SYSTICK_DelayUs_HS SYSTICK_DelayUs 

static uint16_t BK4819_ReadU16_HS(void)
{
	unsigned int i;
	uint16_t     Value;

	PORTCON_PORTC_IE = (PORTCON_PORTC_IE & ~PORTCON_PORTC_IE_C2_MASK) | PORTCON_PORTC_IE_C2_BITS_ENABLE;
	GPIOC->DIR = (GPIOC->DIR & ~GPIO_DIR_2_MASK) | GPIO_DIR_2_BITS_INPUT;
	SYSTICK_DelayUs_HS(1);

	Value = 0;
	for (i = 0; i < 16; i++)
	{
		Value <<= 1;
		Value |= GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
		GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
		SYSTICK_DelayUs_HS(1);
		GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
		SYSTICK_DelayUs_HS(1);
	}
	PORTCON_PORTC_IE = (PORTCON_PORTC_IE & ~PORTCON_PORTC_IE_C2_MASK) | PORTCON_PORTC_IE_C2_BITS_DISABLE;
	GPIOC->DIR = (GPIOC->DIR & ~GPIO_DIR_2_MASK) | GPIO_DIR_2_BITS_OUTPUT;

	return Value;
}

void BK4819_WriteU16_HS(uint16_t Data)
{
	unsigned int i;

	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
	for (i = 0; i < 16; i++)
	{
		if ((Data & 0x8000) == 0)
			GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
		else
			GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);

		SYSTICK_DelayUs_HS(1);
		GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);

		Data <<= 1;

		SYSTICK_DelayUs_HS(1);
		GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
		SYSTICK_DelayUs_HS(1);
	}
}

uint16_t BK4819_ReadRegister_HS(BK4819_REGISTER_t Register)
{
	uint16_t Value;

	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);

	SYSTICK_DelayUs_HS(1);

	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
	BK4819_WriteU8(Register | 0x80);
	Value = BK4819_ReadU16_HS();
	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);

	SYSTICK_DelayUs_HS(1);

	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);

	return Value;
}



void BK4819_WriteU8_HS(uint8_t Data)
{
	unsigned int i;

	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
	for (i = 0; i < 8; i++)
	{
		if ((Data & 0x80) == 0)
			GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
		else
			GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);

		SYSTICK_DelayUs_HS(1);
		GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
		SYSTICK_DelayUs_HS(1);

		Data <<= 1;

		GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
	}
}


void BK4819_WriteRegister_HS(BK4819_REGISTER_t Register, uint16_t Data)
{
	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);

	//SYSTICK_DelayUs_HS(1);

	GPIO_ClearBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);
	BK4819_WriteU8_HS(Register);

	//SYSTICK_DelayUs_HS(1);

	BK4819_WriteU16_HS(Data);

	//SYSTICK_DelayUs_HS(1);

	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCN);

	//SYSTICK_DelayUs_HS(1);

	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SCL);
	GPIO_SetBit(&GPIOC->DATA, GPIOC_PIN_BK4819_SDA);
}


static int clamp2(int v, int min, int max) {
  return v <= min ? min : (v >= max ? max : v);
}

uint8_t Rssi2PX2(uint16_t rssi, uint8_t pxMin, uint8_t pxMax)
{
  //const int DB_MIN = -130 << 1;
  //const int DB_MAX = -50 << 1;
  const int DB_MIN = -260;
  const int DB_MAX = -100;
  const int DB_RANGE = DB_MAX - DB_MIN;

  const uint8_t PX_RANGE = pxMax - pxMin;

  int dbm = clamp2(rssi - (160 << 1), DB_MIN, DB_MAX);

  //return ((dbm - DB_MIN) * PX_RANGE + DB_RANGE / 2) / DB_RANGE + pxMin;
  return ((dbm - DB_MIN) * PX_RANGE + DB_RANGE / 2) / DB_RANGE + pxMin;
}

void CEC_ReceiveMode(bool _isReceive)
{
  BK4819_ToggleGpioOut(BK4819_GPIO6_PIN2_GREEN, _isReceive);
  if (_isReceive)
  {
    APP_StartListening(FUNCTION_MONITOR);
    //delay(STOP_RSSI_TIME);
    //RADIO_SetupRegisters(true);
  }
  else
  {
    AUDIO_AudioPathOff();
  }
}

void CEC_WaitKeyRelease(void)
{
  while (KEYBOARD_Poll() != KEY_INVALID)
    delay(10);
}

void CEC_DisplayFreqSmallst(uint32_t _srcFreq, uint8_t _xPosition, uint8_t _yPosition)
{
  uint8_t tmpBuff[12];
  sprintf(tmpBuff, "%u.%03u", _srcFreq / 100000, (_srcFreq / 100) % 1000);
  CEC_DisplaySmallest(tmpBuff, _xPosition > 97  ? 97 : _xPosition, _yPosition, false, true);  
  //CEC_DisplayFreqSmallst(targetFreq*10, maxPosition > 97  ? 97 : maxPosition , 9);
}

void CEC_DisplayValueSmallst(char * _dispStr, uint8_t _dispValue, uint8_t _xPosition, uint8_t _yPosition, bool _isStatus)
{
  uint8_t tmpBuff[12];
  sprintf(tmpBuff, _dispStr, _dispValue);
  CEC_DisplaySmallest(tmpBuff, _xPosition, _yPosition, _isStatus, true);  
}



const uint8_t STXBUFF[] = { 0x59, 0x57, 0x58, 0x73 };
const uint8_t ETXBUFF[] = { 0x95, 0x75, 0x85, 0x37 };

int CEC_SendRemoteData(uint8_t _cmdType, uint8_t _cmdData1, uint8_t _cmdData2, uint8_t * _sndData, uint16_t _sendLength)
{
    uint8_t tmpBuff[16];    //Max 16 Byte

  //uint8_t * tmpBuff = strBuff;
    //STX4, COMMAND, LENGTH, LENGTH,          ___ ETX4
    tmpBuff[4] = _cmdType;
    tmpBuff[5] = (_sendLength & 0xFF);
    tmpBuff[6] = ((_sendLength >> 8) & 0xFF);
    tmpBuff[7] = _cmdData1;
    tmpBuff[8] = _cmdData2;

    memcpy(tmpBuff, STXBUFF, 4);
    memcpy(&tmpBuff[12], ETXBUFF, 4);

    //UART_Send(STXBUFF, 4);
    //UART_Send(tmpBuff, 8);
    UART_Send(tmpBuff, 16);
    //UART_Send(ETXBUFF, 4);

    if (_sendLength > 0)
    {
        UART_Send(_sndData, _sendLength);
    }
}


/*
0x50 이하는 데이터 최대 크기가 110Byte이고 CommBuff에 모두 저장되어 리턴된다.
//==========================================
//전체읽어서 리턴
0x31 : SET FREQ, MOD (DATA SIZE : 0)
0x32 : 



0x41 : LCD BY TEXT DRAW PROTOCOL

//==========================================
//COMMAND만 읽는다.
0x51 : LCD MAIN SCREEN
0X52 : LCD STATUS SCREEN 

*/

uint8_t CECSWUartReadByte();

//Read to CommBuff  (Return CommBuff Start Index)
//int CEC_ReceiveRemoteCommand(uint8_t * _cmdType, uint16_t * _dataLength)
int CEC_ReceiveRemoteCommand()
{
  uint8_t cmdIndex = 0;


#ifdef ENABLE_CEC_INTERFACE_CABLE
//empty check of swuart
#else
  if (IsUartEmpty())
  {
     //BackLightBlink(1);
    return -1;
  }
#endif

  while(1)
  {   //msec

#ifdef ENABLE_CEC_INTERFACE_CABLE
        //SOFTWARE CABLE USING ADC
      CommBuff[cmdIndex++] =  CECSWUartReadByte();
#else
      //_nowRecvChar = CECHWUartReadByte(5000); //5msec
      CommBuff[cmdIndex++] = CECHWUartReadByte(1000); //5msec
#endif      

      //time out checkm, Not Received Data
      if (CECSWUart_LastError == 1) return -1;

      //Arrived Data (one more) Added Buffer
      //_nowRecvChar;

      //BackLightBlink(1);

      if (cmdIndex < 5)
          continue;

      //Check ETX
      if (ETXBUFF[3] == CommBuff[cmdIndex - 1] && ETXBUFF[2] == CommBuff[cmdIndex - 2]
          && ETXBUFF[1] == CommBuff[cmdIndex - 3] && ETXBUFF[0] == CommBuff[cmdIndex - 4])
      {
          //ETX FOUND BUT WRONG DATA (Short data)
          if (cmdIndex < 15)
          {
              //cmdIndex = 0;
              return -1;
          }

          //Check STX -> CORRECT COMMAND DATA
          if (STXBUFF[3] == CommBuff[cmdIndex - 13] && STXBUFF[2] == CommBuff[cmdIndex - 14]
              && STXBUFF[1] == CommBuff[cmdIndex - 15] && STXBUFF[0] == CommBuff[cmdIndex - 16])
          {
            ////0x50 이하는 데이터를 그대로 읽는다.
            //*_cmdType = CommBuff[cmdIndex - 12];
            //*_dataLength = (uint16_t)((CommBuff[cmdIndex - 10] << 8) + CommBuff[cmdIndex - 11]);
            //CMD_DATALEN = CommBuff[cmdIndex - 10];
            //CMD_DATALEN = 
            //if ()
            //CommBuffUsingType = CommBuff[cmdIndex - 12];
            //CommBuffLastUseTime = (uint16_t)((CommBuff[cmdIndex - 10] << 8) + CommBuff[cmdIndex - 11]);
            //data Maximum 110 byte 0x50 Protocol
            //uint8_t tmpLength = CommBuff[cmdIndex - 11];
            if (CommBuff[cmdIndex - 12] < 0x50)
            {
              for (int i = 0; i < CommBuff[cmdIndex - 11]; i++)
              {

#ifdef ENABLE_CEC_INTERFACE_CABLE        
        //SOFTWARE CABLE USING ADC
                CommBuff[cmdIndex + i] = CECSWUartReadByte();
#else

                CommBuff[cmdIndex + i] = CECHWUartReadByte(5000); //5msec
#endif                
                if (CECSWUart_LastError == 1) return -1;
              }
            }

//uint8_t CommBuff[COMBUFF_LENGTH];  //for Common Use for 
//uint8_t CommBuffUsingType = 0;
//uint32_t CommBuffLastUseTime = 0;   //10milisec using millis10() function
            return (cmdIndex - 16);
          }
          else
            return -1;
      }
      else if (cmdIndex > 15)
      {
        return -1;
      }
    }


}

