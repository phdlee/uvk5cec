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
uint8_t CEC_LiveSeekMode = 0;   //0:NONE, 1:LIVE, 2:LIVE+1, 3:LIVE+2
uint8_t CW_Tone       = 70;     //[EEPROM] *10 Hz, because BK4819 inc/dec 10Hz, Move from cecmorese.c
uint8_t HF_DualWatch = 0;
uint8_t HF_DualVol = 63;

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
#ifdef ENABLE_CEC_SSTV
    if (SSTV_LCD_Start_Timer > 0)
    {
        if (--SSTV_LCD_Start_Timer == 0)
            StartSSTVM1(2);	//LCD SCREEN
    }    
#endif    
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



void UART_Init9600(void)
{
	uint32_t Delta;
	uint32_t Positive;
	uint32_t Frequency;

	UART1->CTRL = (UART1->CTRL & ~UART_CTRL_UARTEN_MASK) | UART_CTRL_UARTEN_BITS_DISABLE;
	Delta = SYSCON_RC_FREQ_DELTA;
	Positive = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_SIG_MASK) >> SYSCON_RC_FREQ_DELTA_RCHF_SIG_SHIFT;
	Frequency = (Delta & SYSCON_RC_FREQ_DELTA_RCHF_DELTA_MASK) >> SYSCON_RC_FREQ_DELTA_RCHF_DELTA_SHIFT;
	if (Positive) {
		Frequency += 48000000U;
	} else {
		Frequency = 48000000U - Frequency;
	}

	//UART1->BAUD = Frequency / 39053U;
	UART1->BAUD = Frequency / 9763;	//19526U;
	UART1->CTRL = UART_CTRL_RXEN_BITS_ENABLE | UART_CTRL_TXEN_BITS_ENABLE | UART_CTRL_RXDMAEN_BITS_ENABLE;
	UART1->RXTO = 4;
	UART1->FC = 0;
	UART1->FIFO = UART_FIFO_RF_LEVEL_BITS_8_BYTE | UART_FIFO_RF_CLR_BITS_ENABLE | UART_FIFO_TF_CLR_BITS_ENABLE;
	UART1->IE = 0;

	DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_DISABLE;

	DMA_CH0->MSADDR = (uint32_t)(uintptr_t)&UART1->RDR;
	DMA_CH0->MDADDR = (uint32_t)(uintptr_t)UART_DMA_Buffer;
	DMA_CH0->MOD = 0
		// Source
		| DMA_CH_MOD_MS_ADDMOD_BITS_NONE
		| DMA_CH_MOD_MS_SIZE_BITS_8BIT
		| DMA_CH_MOD_MS_SEL_BITS_HSREQ_MS1
		// Destination
		| DMA_CH_MOD_MD_ADDMOD_BITS_INCREMENT
		| DMA_CH_MOD_MD_SIZE_BITS_8BIT
		| DMA_CH_MOD_MD_SEL_BITS_SRAM
		;
	DMA_INTEN = 0;
	DMA_INTST = 0
		| DMA_INTST_CH0_TC_INTST_BITS_SET
		| DMA_INTST_CH1_TC_INTST_BITS_SET
		| DMA_INTST_CH2_TC_INTST_BITS_SET
		| DMA_INTST_CH3_TC_INTST_BITS_SET
		| DMA_INTST_CH0_THC_INTST_BITS_SET
		| DMA_INTST_CH1_THC_INTST_BITS_SET
		| DMA_INTST_CH2_THC_INTST_BITS_SET
		| DMA_INTST_CH3_THC_INTST_BITS_SET
		;
	DMA_CH0->CTR = 0
		| DMA_CH_CTR_CH_EN_BITS_ENABLE
		| ((0xFF << DMA_CH_CTR_LENGTH_SHIFT) & DMA_CH_CTR_LENGTH_MASK)
		| DMA_CH_CTR_LOOP_BITS_ENABLE
		| DMA_CH_CTR_PRI_BITS_MEDIUM
		;
	UART1->IF = UART_IF_RXTO_BITS_SET;

	DMA_CTR = (DMA_CTR & ~DMA_CTR_DMAEN_MASK) | DMA_CTR_DMAEN_BITS_ENABLE;

	UART1->CTRL |= UART_CTRL_UARTEN_BITS_ENABLE;
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
  else if (rx1Mode == 1)  //UART
  {
    //SYSTEM DEFAULT
    UART_Init();
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
    UART_Init9600();
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
    //                    4Under 1  4  5   6    7  8   9   +3  +5  +10                   +20  +30
    uint8_t _baseMeterValue[] = {2, 4, 10, 16, 22, 28, 34, 37, 39, 44, 45, 47,  48,  50, 54}; //, 114};
    for (int i = sizeof(_baseMeterValue) - 1; i >= 0; i--)
    {
        if (_srcValue >= _baseMeterValue[i])
            return i + 1;
    }
    
    return 0;
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
    c -= 0x20;
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
    x += 4;
  }
}
