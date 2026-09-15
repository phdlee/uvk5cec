#include "cecmorse.h"
#include <string.h>
#include <stdio.h>     // NULL
#include "driver\bk4819.h"
#include "bsp/dp32g030/portcon.h"
#include "bsp/dp32g030/saradc.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/adc.h"
#include "radio.h"
#include "driver\st7565.h"
#include "misc.h"
#include "settings.h"
#include "driver\keyboard.h"
#include "functions.h"
#include "audio.h"
#include "cecswuart.h"
#include "external/printf/printf.h"
#include "driver\systick.h"
#include "ceccommon.h"
#include "driver\crc.h"

void WriteFMLog3(char *writeMessage, char *writeMessage2, int delayMS)
{
	memset(gStatusLine,  0, sizeof(gStatusLine));
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
	UI_PrintString(writeMessage, 0, 0, 0, 8);
	UI_PrintString(writeMessage2, 0, 0, 2, 8);
	ST7565_BlitFullScreen();
	delay(delayMS);
}

uint16_t GetUartADC(void)
{
	//uint16_t readBuff[5] = {0};
	uint16_t maxReadValue = 0;
	uint16_t nowReadValue = 0;
	int i;
	ADC_SoftReset();
	ADC_Start();
	while (!ADC_CheckEndOfConversion(ADC_CH3)) {}
    nowReadValue = ADC_GetValue(ADC_CH3);
    /*
	for (i = 0; i < 5; i++)
	{
		nowReadValue = ADC_GetValue(ADC_CH3);;
		if (nowReadValue > maxReadValue)
			maxReadValue = nowReadValue;
		//SYSTEM_DelayMs(1);
    SYSTICK_DelayUs(500); //3 * 5= 2.5ms
	}
    */

	return nowReadValue;
}
//9600 에서는 19만큼 빼주니까 좋다.
#define baud 9600
//#define baud 19200
//#define UART_RX (GetUartADC() > 4020)
#define UART_RX (GetUartADC() > 3900)
#define ADC_MEAUSRE_TIME 19
#define OnebitDelay ( (1000000 / baud))
#define Databitcount 8
uint16_t CECSWUart_ReadTimeOut = 30000;
uint8_t CECSWUart_LastError = 0;


uint8_t CECSWUartReadByte()
{
    uint16_t timeOutcheck = 0;
    uint8_t DataValue = 0;
    while (UART_RX == 1)
    {
        if (++timeOutcheck > CECSWUart_ReadTimeOut)
        {
            CECSWUart_LastError = 1;    //time out
            return 0;
        }

        //SYSTICK_DelayUs(3);
    }
    CECSWUart_LastError = 0;

    //Wait Start bit
    SYSTICK_DelayUs(OnebitDelay - ADC_MEAUSRE_TIME);
    SYSTICK_DelayUs(OnebitDelay / 2);

    for (int i = 0; i < Databitcount; i++)
    {
        if (UART_RX == 1)
        {
            DataValue += (1 << i);
        }

        SYSTICK_DelayUs(OnebitDelay - ADC_MEAUSRE_TIME);
    }
    //Check Stop Bit
    if (UART_RX == 1)
    {
        SYSTICK_DelayUs(OnebitDelay / 2);
        return DataValue;
    }
    else
    {
        SYSTICK_DelayUs(OnebitDelay / 2);
        //return 0x00;
        //BackLightBlink(1);
        return DataValue;

    }
}

//9600Fixed
void CECSWUartInit()
{
    //Init ADC Mode
    SetRX1Mode(2);
}

uint8_t CECHWUartReadByte(int _timeoutCount);
//uint8_t CECHWUartClearBuffer();
//Receive Command
uint8_t CECUartCommand(uint8_t _option1)
{
    int _recvIndex = 0;
    uint8_t _nowRecvChar = 0x00;
    uint8_t _dataLength = 0;
    uint8_t _uartReadStep = 0;  //0 : READY, 1:STX1, 2:STX2, LEN1, LEN2

    if (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))
        return 1;

    while(1)
    {
#ifdef ENABLE_CEC_INTERFACE_CABLE        
        //SOFTWARE CABLE USING ADC
        _nowRecvChar = CECSWUartReadByte();
#else
        //UART CABLE
        _nowRecvChar = CECHWUartReadByte(CECSWUart_ReadTimeOut);
#endif        
        //time out check
        if (CECSWUart_LastError == 1)
        {
            break;
        }

        CommBuff[_recvIndex++] = _nowRecvChar;

        //DataRead
        if (_uartReadStep == 4)
        {
            if (_recvIndex >= _dataLength)
                break;
        }
        else if (_uartReadStep == 3)
        {
            //if (_nowRecvChar >= 10)   //PROTOCOL :  STX1, STX2, LENGTH [DATA MIN 4] CRC1,CRC2 ETX1
            //{
                _dataLength += _nowRecvChar * 256;
                _uartReadStep = 4;
            //}
        }
        else if (_uartReadStep == 2) //checked stx, data in
        {
            _dataLength = _nowRecvChar;
            _uartReadStep = 3;
        }
        else if (_uartReadStep == 1)    //step1
        {
            if (_nowRecvChar == 0x57)
                _uartReadStep = 2;      // Ready Data In
            else
                break;
        }
        else
        {
            //step 0 
            if (_nowRecvChar == 0x59)
                _uartReadStep = 1;
            else
                break;
        }

    }
    if (CECSWUart_LastError == 1)
        return 3;

    if (_uartReadStep < 4)
        return 4;               //occured error


    //0     1    2    3(10)  4     5     6     7    8   9
    //STX, STX, LEN, DATA, DATA, DATA, DATA, CRC, CRC, ETX

    uint16_t rcvCRC = (CommBuff[_dataLength - 3] +  CommBuff[_dataLength - 2] * 256);
    uint16_t crcValue = CRC_Calculate(CommBuff,  _dataLength - 3);   //without crc1, crc2, etx
    //buff[16] = 0x00;
    //sprintf(strBuff, "ADC:%u, %u", _rVal, uartValue);
    //WriteFMLog(strBuff, 0);

    if (rcvCRC == crcValue)
    {
        //sprintf(strBuff, "%d:%u,%x,%d", _dataLength - 6, cntVal++, crcValue, errCount);
        //WriteFMLog3(strBuff, &buff[3], 0);
        return  _dataLength;
    }
    else
    {
        /*
        errCount++;
        sprintf(strBuff, "ERR%x:%u, %x", rcvCRC, cntVal++, crcValue);
        WriteFMLog3(strBuff, &buff[3], 0);
        */
       return 2;
    }

    return 0;
}

//uint8_t strBuff[32];
 int errCount = 0;

//CRC를 테스트 한다.
void uartTest()
{
    //ADC 값을 읽어서 화면에 표시한다.
    WriteFMLog3("Hello World", "", 0);
    CECSWUartInit();

    uint16_t _rVal = 0;
    uint8_t uartValue = 0;
    uint8_t buff[256] = { 10, 20, 30, 40, 50, 60, 70, 80, 90, 95};
    //10개씩 읽어서 써보자

    for (int i = 0; i < 256; i++)
        buff[i] = i;

    uint16_t crcValue = CRC_Calculate(buff, 256);

    sprintf(strBuff, "CRC:%u, %x", crcValue, crcValue);
    WriteFMLog3(strBuff, "", 0);

    //delay(5000);

    BackLightBlink(3);
    int cntVal = 0;
    uint8_t _uartReadStep = 0;  //0 : READY, 1:STX1, 2:STX2,

    while(1)
    {

        /*
        //ADC를 읽어서 화면에 표시한다.
        //_rVal = GetUartADC();
        for (int i = 0;  i < 64; i++)
        {
            buff[i] = CECSWUartReadByte();
            if (CECSWUart_LastError == 1)
            {
                break;
            }
        }
        buff[64] = 0x00;
        if (CECSWUart_LastError == 1)
            continue;
        WriteFMLog(&buff[52], 0);
        continue;
    */
  
  /*

        //대용량  256 Byte 읽어서 CRC체크하는 루틴
        for (int i = 0;  i < 16; i++)
        {
            buff[i] = CECSWUartReadByte();
            //time out check
            if (CECSWUart_LastError == 1)
            {
                //DataAllReset Write
                //BackLightBlink(1);
                break;
            }
        }
        if (CECSWUart_LastError == 1)
            continue;

        crcValue = CRC_Calculate(buff, 16);
        buff[16] = 0x00;
        */


        //프로토콜화 해서 읽어본다. 처음59두번째57이들어와야 STX

        //for (int i = 0;  i < 16; i++)
        int _recvIndex = 0;
        uint8_t _nowRecvChar = 0x00;
        uint8_t _dataLength = 0;
        _uartReadStep = 0;
        while(1)
        {
            _nowRecvChar = CECSWUartReadByte();
            //time out check
            if (CECSWUart_LastError == 1)
            {
                break;
            }

            buff[_recvIndex++] = _nowRecvChar;
            //DataRead
            if (_uartReadStep == 3)
            {
                if (_recvIndex >= _dataLength)
                    break;
            }
            else if (_uartReadStep == 2) //checked stx, data in
            {
                if (_nowRecvChar >= 10)   //PROTOCOL :  STX1, STX2, LENGTH [DATA MIN 4] CRC1,CRC2 ETX1
                {
                    _dataLength = _nowRecvChar;
                    _uartReadStep = 3;
                }
            }
            else if (_uartReadStep == 1)    //step1
            {
                if (_nowRecvChar == 0x57)
                    _uartReadStep = 2;      // Ready Data In
                else
                    break;
            }
            else
            {
                //step 0 
                if (_nowRecvChar == 0x59)
                    _uartReadStep = 1;
                else
                    break;
            }

        }
        if (CECSWUart_LastError == 1)
            continue;

        if (_uartReadStep < 3)
            continue;   //occured error


        //0     1    2    3(10)  4     5     6     7    8   9
        //STX, STX, LEN, DATA, DATA, DATA, DATA, CRC, CRC, ETX

        uint16_t rcvCRC = (buff[_dataLength - 3] +  buff[_dataLength - 2] * 256);
        crcValue = CRC_Calculate(buff,  _dataLength - 3);   //without crc1, crc2, etx
        buff[16] = 0x00;


        //sprintf(strBuff, "ADC:%u, %u", _rVal, uartValue);
        //WriteFMLog(strBuff, 0);
        
         if (rcvCRC == crcValue)
         {
            sprintf(strBuff, "%d:%u,%x,%d", _dataLength - 6, cntVal++, crcValue, errCount);
            WriteFMLog3(strBuff, &buff[3], 0);
         }
         else
         {
            errCount++;
            sprintf(strBuff, "ERR%x:%u, %x", rcvCRC, cntVal++, crcValue);
            WriteFMLog3(strBuff, &buff[3], 0);
         }

        //delay(5);
    }


}