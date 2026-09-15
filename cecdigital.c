#include "cecmorse.h"
#include <string.h>
#include <stdio.h>     // NULL
#include "driver/bk4819.h"
#include "bsp/dp32g030/portcon.h"
#include "bsp/dp32g030/saradc.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/adc.h"
#include "radio.h"
#include "driver/st7565.h"
#include "misc.h"
#include "settings.h"
#include "driver/keyboard.h"
#include "functions.h"
#include "audio.h"
#include "ceccommon.h"
#include "cecswuart.h"
#include "ui/helper.h"
#include "app/uart.h"
#include "cecdigital.h"

#define BK4819_REG_40 0x40U
#define BK4819_REG_40_SHIFT_ENABLE_DEVIATION 12
#define BK4819_REG_40_SHIFT_TX_DEVIATION 0

#define ISPTTPRESS  (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))

uint32_t BasicFreq = 0;
uint8_t nowMode = 0;    //0 : FT8, 1: APRS, 2:WSPR
uint8_t ScreenDelayTime = 0;
uint8_t ScreenLockMode = 0;
uint8_t ScreenControlFromDSP = 0;
uint32_t digitBeforeFreq = 0;

void ChangeFreq()
{
    BK4819_SetFrequency(BasicFreq);
    BK4819_RX_TurnOn();
}

//void BK4819_WriteRegister(BK4819_REGISTER_t Register, uint16_t Data);

//Read From Memory to CommBuff
uint8_t ReadWSPRData(uint8_t readIndex)
{
    //8바이트식 6번 읽으면 된다.
    //0x58 0x57  subFreq WSPRDATA 41byte 40 * 4 + 2 = 162
    /*
        110.6 sec continuous wave
        Frequency shifts among 4 tones every 0.683 sec
        Tones are separated by 1.46 Hz
        Total bandwidth is about 6 Hz
        50 bits of information are packaged into a 162 bit message with FEC
        Transmissions always begin 1 sec after even minutes (UTC)    
        There are 162 ((50 + K − 1) * 2) possible symbols.    
    */
    #define WSPR_DATA_LEN 48
    uint8_t startAddr = 0x0970 + (WSPR_DATA_LEN * readIndex); 
    EEPROM_ReadBuffer(startAddr, CommBuff, 48);
    return (CommBuff[0] == 0x58 && CommBuff[1] == 0x57);
}

void DisplayDigitalMode()
{
    uint8_t digiName[3][5] = {"FT8", "APRS", "WSPR"};
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    //UI_PrintStringSmallNormal("Digital Mode", 0, 0, 0);

    sprintf(strBuff, "%3u.%05u", BasicFreq / 100000, BasicFreq % 100000);
    UI_PrintStringSmallNormal(strBuff+ 7, 113, 0, 1);
    strBuff[7] = 0;
    // show the main large frequency digits
    UI_DisplayFrequency(strBuff, 32, 0, false);
    UI_PrintStringSmallNormal(digiName[nowMode], 0, 0, 1);

    //WSPR MODE DRAW CHANNEL INFO
    if (nowMode == 2)   //WSPR MODE
    {
        for (int i = 0; i < 3; i++)
        {
            //ReadWSPRData(i);
            if (ReadWSPRData(i))
            {
                //DRAW Channel Info
                uint8_t subFreq = CommBuff[2];
                //SETTINGS_FetchChannelName(CommBuff, 151 + 3 * i);
                EEPROM_ReadBuffer(0x0F50 + ((151 + 3 * i) * 16), CommBuff, 10);
                sprintf(strBuff, "%d.%-10s%3dHz", i + 1, CommBuff, subFreq * 10);

                //sprintf(strBuff, "%d - %X %X %X %X %X", i + 1, CommBuff[0], CommBuff[1],CommBuff[2],CommBuff[3],CommBuff[4]);
                UI_PrintStringSmallNormal(strBuff, 0, 0, 3 + i);
            }
        }
    }
    else if (DigitalMode)
    {
        if (ScreenControlFromDSP)
                UI_PrintStringSmallNormal("0: TIME SET", 0, 0, 3);
                //UI_PrintStringSmallNormal("1: TIME SET", 0, 0, 4);

    }


    //UI_PrintStringSmallNormal("Exit (PTT)", 0, 0, 6);

    ST7565_BlitFullScreen();
    memset(gStatusLine, 0, sizeof(gStatusLine));

    /*
    uint8_t progressBarIndex = 0;
    for (; progressBarIndex < 128 - symbol_count -5; progressBarIndex++)
    {
        gStatusLine[progressBarIndex] = 0b00011100;
    }
    */
    uint8_t     *line = gStatusLine;
    UI_PrintStringSmallBufferNormal("Ready", line);
    ST7565_BlitStatusLine();
}

void SetFreqCEC(uint16_t xtalFreq, uint32_t vfoFreq)
{
    BK4819_WriteRegister(BK4819_REG_3B, xtalFreq);
    BK4819_WriteRegister(BK4819_REG_38, (vfoFreq >>  0) & 0xFFFF);
    BK4819_WriteRegister(BK4819_REG_39, (vfoFreq >> 16) & 0xFFFF);
}

/*
void SetReceiveStart()
{
  gTxVfo->freq_config_RX.Frequency = BasicFreq;
  //gTxVfo->Modulation = MODULATION_SSB;  //MODULATION_FM;   //MODULATION_SSB;
  //gTxVfo->Modulation = nowMode == 1 ? MODULATION_FM : MODULATION_SSB;
  RADIO_SetupRegisters(true);
  APP_StartListening(FUNCTION_MONITOR);
}
*/
//20240417. 64048kb
void SetReceiveStart()
{
    gTxVfo->freq_config_RX.Frequency = BasicFreq;
    
    /*
    if (nowMode == 1)   //APRS MODE
    {
//        gTxVfo->freq_config_RX.Frequency = beforeFreq;  //original Freq
        gTxVfo->Modulation = MODULATION_FM;
    }
    else    //OTHER DIGITAL MODE
    {
//        gTxVfo->freq_config_RX.Frequency = BasicFreq;
        gTxVfo->Modulation = MODULATION_SSB;
  
    }
*/

  //gTxVfo->Modulation = MODULATION_SSB;  //MODULATION_FM;   //MODULATION_SSB;
  //gTxVfo->Modulation = nowMode == 1 ? MODULATION_FM : MODULATION_SSB;
  RADIO_SetupRegisters(true);
  APP_StartListening(FUNCTION_MONITOR);
}

// Mode defines
#define JT9_DELAY               576          // Delay value for JT9-1
#define JT65_DELAY              371          // Delay in ms for JT65A
#define JT4_DELAY               229          // Delay value for JT4A
//#define WSPR_DELAY              683          // Delay value for WSPR   or -1
//#define WSPR_DELAY              (683 -12)     //-6 ~ -18 decoded (Good)

//#define WSPR_DELAY              (671)     //-6 ~ -18 decoded (Good)
//#define WSPR_DELAY              (669)     //TESTING  (670까지)  (BEST)
#define WSPR_DELAY              (670)     //TESTING  (670까지)  (BEST)
//     665   667  668   670   671      675      678     679   680
//5R   no   ok(?)             ok       ok(!)    ok(?)   no    no
//5K              no   ok(?)  ok

//28Mhz
//   60 61 62 663 664 665                      655 673 674 675 676 677  678  679   680
//5R ok ok(?) ok   ok                              ok   ok  no no   no                     (667)
//5K          no ok(?) ok          ok                       ok ok  ok(?)   no              (671)

#define FSQ_2_DELAY             500          // Delay value for 2 baud FSQ
#define FSQ_3_DELAY             333          // Delay value for 3 baud FSQ
#define FSQ_4_5_DELAY           222          // Delay value for 4.5 baud FSQ
#define FSQ_6_DELAY             167          // Delay value for 6 baud FSQ
#define FT8_DELAY               157          // Delay value for FT8 159 158 156 ok  ( 155 ~ 159)
//#define FT8_DELAY               160          // Delay value for FT8 (not some ) 154 some 153x
#define FT4_DELAY               47           // Delay value for FT4

// ft8 test 
//    153  154 155   156 157 158  159  160 161
// 5r no   ok  ok                  ok  no   no
// k5 no   no  ok(?)               ok  ok   no


#define JT65_SYMBOL_COUNT                   126
#define JT9_SYMBOL_COUNT                    85
#define JT4_SYMBOL_COUNT                    207
#define WSPR_SYMBOL_COUNT                   162
#define FT8_SYMBOL_COUNT                    79
#define FT4_SYMBOL_COUNT                    103

uint8_t wsprBandIndex = 0;
#define WSPR_MAX_BAND_CNT 3
#define FT8_MAX_BAND_CNT  4
const uint32_t wsprBandFreq[WSPR_MAX_BAND_CNT] = 
{
/*
WSPR
28.1246 MHz
50.2930 MHz (Region 2, 3)
144.4890 MHz
432.3000 MHz
*/    
    2812460, 5029300, 14448900
};

/*
const uint32_t FT8BandFreq[FT8_MAX_BAND_CNT] = 
{
    2807400, 5031300, 14417400, 44206500
};
*/


void SendDigitalMode(uint8_t dataType, uint16_t subDigFreq, uint8_t *txBuffer)
{
	//uint8_t swingHz[8] = {0, 1, 2, 3, 4, 5, 6, 7};  //8 : 6.25 * 8FSK

    //Added 28Mhz, 50Mhz 2024.05.20
    //28.075 Mhz
    const uint16_t adjFT8_28074[8][2] = 
    {
        {  0,  0},
        {116, 62},
        { 19,  9},
        { 22, 10},
        { 38, 18},
        {141, 73},
        { 94, 47},
        { 47, 21},
    };

    //Added 50Mhz, 50Mhz 2024.05.20
    //50.314 Mhz
    /*
    const uint16_t adjFT8_50313[8][2] = 
    {
        {  0,   0},
        {104, 100},
        { 23,  21},
        {158, 151},
        { 46,  42},
        { 27,  23},
        {162, 153},
        { 50,  44},
    };
    */
    const uint16_t adjFT8_50313[8][2] = 
    {
        {  0,   0},
        {104, 100},
        { 23,  21},
        {158, 151},
//        { 46,  42},
//        { 77,  72}, //745.03 - 25 = 720
        { 293,  281}, //2835.000 -25 2810
        { 27,  23},
        {162, 153},
        { 50,  44},
    };


    //Remarked 2024.05.20
    //144120000
    /*
    const uint16_t adjFT8_144120[8][2] = 
    //uint16_t adjXtalFreq[8][2] = 
    {
        {  0,   0},
        {343, 950},
        {314, 869},
        {250, 691},
        {221, 610},
        {157, 432},
        {128, 351},
        { 64, 173},
    };
    */

    //144174000
    const uint16_t adjFT8_144170[8][2] = 
    {
        {  0,   0},
        { 94, 260},
        {166, 459},
        {260, 719},
        { 11,  28},
        { 83, 227},
        {133, 365},
        {249, 686},
    };

    const uint16_t adjFT8_144460[8][2] = 
    {
        {  0,   0},
        {218, 605},
        { 89, 246},
        {307, 851},
        {178, 492},
        {396, 1097},
        {267, 738},
        {129, 354},
    };

    //432065000
    const uint16_t adjFT8_432065[8][2] = 
    {
        { 0,   0},
        {70, 581},
        {17, 140},
        {87, 721},
        {34, 280},
        {36, 296},
        {51, 420},
        {53, 436},
    };

    const uint16_t adjFT8_1296065[8][2] = 
    {
        {  0,    0},
        { 71, 1769},
        { 76, 1893},
        {147, 3662},
        { 33,  820},
        {223, 5555},
        {109, 2713},
        {114, 2837},
    };
/*
    //432175000
    const uint16_t adjFT8_432175[8][2] = 
    {
        {  0,    0},
        {346, 2875},
        {303, 2517},
        {260, 2159},
        { 37,  305},
        {174, 1443},
        { 41,  337},
        {433,  353},
    };
*/

    const uint16_t adjWSPRTable[WSPR_MAX_BAND_CNT][4][2] = 
    {
        //28Mhz Band 28.1246 MHz
        {
            {  0,   0},
            {285, 154},
            {154,  83},
            { 23,  12}
        },

        //50Mhz Band 50.2930 MHz (Region 2, 3)
        {
            {  0,    0},
            { 26,  25},
            { 52,  50},
            { 78,  75}
        },

        //144Mhz Band  144.4890 MHz
        {
            {  0,    0},
            { 40,  111},
            { 80,  222},
            {504, 1400}
        },
    };

    //int (*pointer)[2];
    const uint16_t (*adjXtalFreq)[2];
	uint8_t symbol_count;
	uint16_t tone_delay;
    uint8_t maxPower = gTxVfo->TXP_CalculatedSetting;  //25;
    //Read Original X-Tal AdjustValue
    uint16_t orgXTalValue = BK4819_ReadRegister(BK4819_REG_3B);

    //adjXtalFreq = adjFT8_144120;
    //dataType
    //if (dataType)
    if (dataType == 0)  //FT8
    {
        symbol_count = FT8_SYMBOL_COUNT; // From the library defines
        tone_delay = FT8_DELAY;

        //Changed 2024.05.20
/*
        if (BasicFreq >= 43217000)
            adjXtalFreq = adjFT8_432175;
        else if (BasicFreq >= 43200000)
            adjXtalFreq = adjFT8_432065;
        else if (BasicFreq >= 14417000)
            adjXtalFreq = adjFT8_144170;
        else if (BasicFreq >= 14410000)
            adjXtalFreq = adjFT8_144120;
        else
        {
            //BackLightBlink(3);
            return;
        }
*/

//        if (BasicFreq >= 100000000)
//            adjXtalFreq = adjFT8_1296065;
        if (BasicFreq >= 40000000)
            adjXtalFreq = adjFT8_432065;
        else if (BasicFreq >= 14440000)
            adjXtalFreq = adjFT8_144460;
        else if (BasicFreq >= 14400000)
            adjXtalFreq = adjFT8_144170;
        else if (BasicFreq >= 5000000)
            adjXtalFreq = adjFT8_50313;
        else if (BasicFreq >= 280000)
            adjXtalFreq = adjFT8_28074;
        else
        {
            //BackLightBlink(3);
            return;
        }

    }
    else if (dataType == 2) //WSPR
    {
        symbol_count = WSPR_SYMBOL_COUNT; // From the library defines
        tone_delay = WSPR_DELAY;
        adjXtalFreq = adjWSPRTable[wsprBandIndex];
    }

    TXViaUART = 1;
	RADIO_PrepareTX();
    TXViaUART = 0;
	
    unsigned int activeTxVFO = gRxVfoIsActive ? gEeprom.RX_VFO : gEeprom.TX_VFO;
    if (VfoState[activeTxVFO] == VFO_STATE_NORMAL)  //Check TX Enable Added Version0.2P, modified Version 0.2R
    {
        BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, true);

        #define BK4819_REG_40 0x40U
        #define BK4819_REG_40_SHIFT_ENABLE_DEVIATION 12
        #define BK4819_REG_40_SHIFT_TX_DEVIATION 0
        DisplayDigitalMode();

        //set disabled DEVIATION 
        uint16_t regTemp1 = BK4819_ReadRegister(BK4819_REG_40);
        BK4819_WriteRegister(0x40, 0);
        BK4819_SetAF(BK4819_AF_MUTE);

        BK4819_SetupPowerAmplifier(0, 0);
        BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);

        const uint8_t gain   = (1u << 3) | (0u << 0);
        const uint8_t enable = 1;
        uint32_t startFreq = BasicFreq + subDigFreq;

        BK4819_WriteRegister(BK4819_REG_36, ( maxPower << 8) | (enable << 7) | (gain << 0));

        CECTimer0Enable(0); //msec
    #define GFSK_SIMULATE_FT8 1

        #ifdef GFSK_SIMULATE_FT8
            BK4819_WriteRegister(BK4819_REG_36, (0 << 8) | (enable << 7) | (gain << 0));
        #else
            BK4819_WriteRegister(BK4819_REG_36, (maxPower << 8) | (enable << 7) | (gain << 0));
        #endif

        uint8_t swingCoeff = txBuffer[0];
        SetFreqCEC(orgXTalValue - adjXtalFreq[swingCoeff][0], startFreq - adjXtalFreq[swingCoeff][1]);    

        #ifdef GFSK_SIMULATE_FT8
            BK4819_WriteRegister(BK4819_REG_36, (0 << 8) | (enable << 7) | (gain << 0));
        #else
            BK4819_WriteRegister(BK4819_REG_36, (maxPower << 8) | (enable << 7) | (gain << 0));
        #endif

        //timeIncVal = 0;
        uint8_t nowByteIndex = 0;
        uint8_t nowBitIndex = 0;
        for(int i = 0; i < symbol_count; i++)
        {
            timeIncVal = 0;
            if (dataType == 2)  //WSPR
            {
                swingCoeff = (txBuffer[nowByteIndex] >> nowBitIndex) & 0x03;
                nowBitIndex += 2;
                if (nowBitIndex > 6)
                {
                    nowBitIndex = 0;
                    nowByteIndex++;
                }
            }
            else
            {
                swingCoeff = txBuffer[i];
            }
           
           SetFreqCEC(orgXTalValue - adjXtalFreq[swingCoeff][0], startFreq - adjXtalFreq[swingCoeff][1]);

#ifdef GFSK_SIMULATE_FT8
			BK4819_WriteRegister(BK4819_REG_36, (10 << 8) | (enable << 7) | (gain << 0));

			for (int i = 3;  i > 0; i--)
			{
				BK4819_WriteRegister(BK4819_REG_36, (i << 8) | (enable << 7) | (gain << 0));
				SYSTEM_DelayMs(1);
			}
#endif
			uint16_t tmp1 = 0xC1FE;
			tmp1 &= ~((1 << 15) | 0x02);	//VCO Calibration Enable and dsp 
			//tmp1 &= ~(0xF << 4);	//Loud Pop sound
			BK4819_WriteRegister(BK4819_REG_30, tmp1);
			BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);

#ifdef GFSK_SIMULATE_FT8
			for (int i = 1;  i <= 5; i++)
			{
				BK4819_WriteRegister(BK4819_REG_36, (i << 8) | (enable << 7) | (gain << 0));
				SYSTEM_DelayMs(1);
			}

        	BK4819_WriteRegister(BK4819_REG_36, ( maxPower << 8) | (enable << 7) | (gain << 0));
#endif

            //SYSTEM_DelayMs(tone_delay-9);

            memset(gStatusLine, 0, sizeof(gStatusLine));
            uint8_t     *line = gStatusLine;
            uint8_t progressPercent = (i * 100) / symbol_count;
            sprintf(strBuff, "%2d%%", progressPercent);
            UI_PrintStringSmallBufferNormal(strBuff, line);

            for (int kk = 0; kk < progressPercent; kk++)
            {
                gStatusLine[kk + 27] = 0b00011100;
            }
            ST7565_BlitStatusLine();
            //gStatusLine[progressBarIndex++] = 0b00011100;
            //ST7565_BlitStatusLine();

            while (timeIncVal < tone_delay)  //-1 : X, -2:X  -3:  -4:X  -5:O   -10:O     -15:O,  -17: O  -18:  -20:X
            {
                SYSTICK_DelayUs(1);
            }
		}
    }
  //Restore orgXtalValue and BaiscFreq
  SetFreqCEC(orgXTalValue, BasicFreq);
  CECTimer0Disable();
  //BK4819_WriteRegister(BK4819_REG_36, (0 << 8) | (enable << 7) | (gain << 0));
  //BK4819_WriteRegister(0x40, 0x4D0 | (0x01U << BK4819_REG_40_SHIFT_ENABLE_DEVIATION));
  SetReceiveStart();

  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);  
}

//startMode : 0 - PC CONTROL
//            1 - Stand Alone (WSPR) and other (wish todo)
uint16_t CRC_Calculate(const void *pBuffer, uint16_t Size);

//uint8_t DigitalModeType = 0x01; //1:APRS, 2:FT8, 3:SignalGenerator for HFBAND, 4 : Realtime Spectrum, 5 : CW Decoder 
uint8_t IsFT8Mode = 0; //1:APRS, 2:FT8, 3:SignalGenerator for HFBAND, 4 : Realtime Spectrum, 5 : CW Decoder 

//CallType : 0 : Main, 1: DititalMode
uint8_t ProcessRemoteControl(int _callType)
{
    //int StartIndex = CEC_ReceiveRemoteCommand(&CMD_TYPE, &CMD_DATALEN);
    int StartIndex = CEC_ReceiveRemoteCommand();
    //if (StartIndex < 0 && !needDisplay)
    //    continue;
    if (StartIndex >= 0)
    {
        //076
        uint8_t * tmpBuff = &CommBuff[StartIndex];
        uint8_t cmd1      = tmpBuff[CEC_PROTOCOL_CMD_TYPE];
        uint16_t cmdLength = (uint16_t)((tmpBuff[CEC_PROTOCOL_CMD_LENGTH + 1] << 8) + tmpBuff[CEC_PROTOCOL_CMD_LENGTH]);    //tmpBuff[CEC_PROTOCOL_CMD_LENGTH]; 

        if (cmdLength == 0)
        {
            if (cmd1 == CEC_COMMAND)    //CEC_COMMAND)
            {
                //COMMAND
                uint8_t subCommand  = tmpBuff[7];
                uint8_t subCmdData1 = tmpBuff[8];
                uint8_t subCmdData2 = tmpBuff[9];

                if (subCommand == CEC_CMD_REQMODE)    //Response Working Mode
                {
                    CEC_SendRemoteData(0x21, CEC_CMD_RESMODE, IsFT8Mode ? 0x02 : 0x01, 0, 0);
                }
                /*
                else if (subCommand == CEC_CMD_TIMESYNC)
                {
                    //WriteLog("Set Time Sync Press Key1");

                }
                */
                else if (subCommand == CEC_CMD_BACKLIGHT)
                {
                    if (subCmdData1 == 0)
                        BACKLIGHT_TurnOff();
                    else if (subCmdData1 == 1)
                        BACKLIGHT_TurnOn();

                    return 0;
                   //BackLightBlink(5);
                    //WriteLog("Set Time Sync Press Key1");

                }
                /*
                else if (subCommand == CEC_CMD_READEEPROM)
                {
                    cmdLength = *(uint16_t *)&tmpBuff[8];
                    EEPROM_ReadBuffer(cmdLength, CommBuff, 128);
                    CEC_SendRemoteData(0x21, CEC_CMD_READEEPROM, 0, CommBuff, 128);
                }
                */
                else if (subCommand == CEC_CMD_FT8READY)
                {
                    ScreenControlFromDSP = true;
                    //BackLightBlink(2);
                    //delay(500);
                    //WriteLog("Currnet Mode : FT8 READY");
                    //Ready Mode
                }

            }
        }



        //From Digital Screen Mode
        if (_callType == 1)
        {
            if (cmd1 == 0x31)   //Change  mode
            {
                nowMode = tmpBuff[CEC_PROTOCOL_DAATA5];
                if (nowMode == 1)
                {
                    BasicFreq = digitBeforeFreq;
                }
                //if APRS
                //if (nowMode != 1)
                //    BasicFreq = (tmpBuff[CEC_PROTOCOL_DAATA1]) + (tmpBuff[CEC_PROTOCOL_DAATA2] << 8) + (tmpBuff[CEC_PROTOCOL_DAATA3] << 16) + (tmpBuff[CEC_PROTOCOL_DAATA4] << 24);

                //SetReceiveStart();
                return 2;
            }
            else if (cmd1 == 0x32)   //Change Frequency
            {
                //nowMode = tmpBuff[CEC_PROTOCOL_DAATA5];
                //if (nowMode == 1)
                //{
                //    BasicFreq = digitBeforeFreq;
                //}
                //if APRS
                //if (nowMode != 1)
                BasicFreq = (tmpBuff[CEC_PROTOCOL_DAATA1]) + (tmpBuff[CEC_PROTOCOL_DAATA2] << 8) + (tmpBuff[CEC_PROTOCOL_DAATA3] << 16) + (tmpBuff[CEC_PROTOCOL_DAATA4] << 24);

                //SetReceiveStart();
                return 2;
            }
            else if (cmd1 == 0x35)  //FT8 SEND SYMDATA
            {
                //nowMode = CommBuff[CEC_DIGIT_CAT_HEADER_SIZE + 1];
                ///uint8_t symLength = CommBuff[CEC_DIGIT_CAT_HEADER_SIZE + 2];
                uint16_t subFreq = (tmpBuff[CEC_PROTOCOL_DAATA1] << 8) + tmpBuff[CEC_PROTOCOL_DAATA2];
                nowMode = tmpBuff[CEC_PROTOCOL_DAATA5];
                //TXViaUART = 1;
                SendDigitalMode(nowMode, subFreq, &CommBuff[StartIndex + CEC_PROTOCOL_PAYLOAD]);
                //TXViaUART = 0;

                //return 2;   //Refresh 
                //SetReceiveStart();
            }

        }

/*
        if (cmdLength == 0 || cmd1 > 0xA0)
        {
            //IF NEED SOME PROCESS, WORK HERE
            return 0;
        }        
*/

        if (cmd1 == 0x38)  //WSPRDATA
        {
            /*
            //nowMode = CommBuff[CEC_DIGIT_CAT_HEADER_SIZE + 1];
            ///uint8_t symLength = CommBuff[CEC_DIGIT_CAT_HEADER_SIZE + 2];
            uint16_t subFreq = (tmpBuff[CEC_PROTOCOL_DAATA1] << 8) + tmpBuff[CEC_PROTOCOL_DAATA2];
            nowMode = tmpBuff[CEC_PROTOCOL_DAATA5];
            SendDigitalMode(nowMode, subFreq, &CommBuff[StartIndex + CEC_PROTOCOL_PAYLOAD]);
            SetReceiveStart();
            */

            //WSPR SAVE ST ST LEN  CMD, DIGIMODE, LEN, LEN
            #define WSPR_DATA_LEN 48  //3Channel 0x58 + 41DATA
            //uint8_t wsprIndex = (cmd1 - 151);
            uint8_t wsprIndex = tmpBuff[CEC_PROTOCOL_DAATA1];   //INDEX
            uint8_t startAddr = 0x0970 + (WSPR_DATA_LEN * wsprIndex);
            uint8_t startChannel = 151 + (3 * wsprIndex);

    #ifdef CEC_COMM_CRC_CHECK
            //CRC CHECK (BEFORE 63620)
            uint16_t rcvCRC = (CommBuff[StartIndex + 16 + 54] +  (CommBuff[StartIndex + 16 + 55] << 8));
            uint16_t crcValue = CRC_Calculate(&CommBuff[16],  54);   //without crc1, crc2, etx

            if (rcvCRC != crcValue)
            {
                UI_PrintStringSmallNormal("CRC ERROR", 0, 0, 2);
                ST7565_BlitFullScreen();
                delay(5000);
                return;
            }
    #endif
        
        //#define WSPR_DATA_SRC_OFFSET (CEC_DIGIT_CAT_HEADER_SIZE + 5 + 10) //HEADER 5, NAME 10, (MAGICKEY 2+SUBTONE1) <-- Saved Data
            #define WSPR_DATA_SRC_OFFSET (16 + 10) //HEADER 16, NAME 10, (MAGICKEY 2+SUBTONE1) <-- Saved Data
            for (int i = 0; i < 6; i++)
            {
                EEPROM_WriteBuffer(startAddr + i * 8, &CommBuff[i * 8 + WSPR_DATA_SRC_OFFSET + StartIndex]);
            }
            for (int i = 0; i < 3; i++)
            {
                SETTINGS_SaveChannelName(startChannel + i, &CommBuff[16 + StartIndex]);
            }

            memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
            UI_PrintStringSmallNormal("WSPR SAVED", 0, 0, 2);
            UI_PrintStringSmallNormal(&CommBuff[16 + StartIndex], 0, 0, 3);
            ST7565_BlitFullScreen();
            delay(2000);

            //return 2;   //Refresh 
        }
        else if (cmd1 == 0x39)
        {
            //CommBuff[CommBuffUsingType], CommValue1); <-- PAYLOAD
            //SET APRS PAYLOAD LENGTH
            CommValue1 = cmdLength;
            CommBuffUsingType = StartIndex + 16;    //Start Message Pointer
            //nowMode = 1;
            CEC_APRS_SEND(APRS_DATA_FROM_UART);
            //return 3;
        }
        else if (cmd1 > 0x40)
        {
            //DISPLAY COMMAND
            ScreenLockMode = tmpBuff[10];  //100msec
            ScreenDelayTime = tmpBuff[11];

            memset(gFrameBuffer, 0, sizeof(gFrameBuffer));


            if (cmd1 == 0x41)
            {
                uint16_t readDataCount = 16;
                while (readDataCount < cmdLength + 16)
                {
                    UI_PrintStringSmallNormal(&tmpBuff[readDataCount + 3], 0, 0, tmpBuff[readDataCount]);
                    readDataCount += tmpBuff[readDataCount + 2] + 3;
                }
            }
            else if (cmd1 == 0x51 || cmd1 == 0x52)  //MAIN SCREEN (EXCEPT STATUSLINE) DRAW SCREEN
            {
                uint8_t * writePointer = cmd1 == 0x51 ? gFrameBuffer[0] : gStatusLine;
                for (int k = 0; k < cmdLength; k++)   //with null
                {
                    writePointer[k] = CECHWUartReadByte(3000);
                    if (CECSWUart_LastError == 1) break;
                }

                if (cmd1 == 0x52)
                {
                    ST7565_BlitStatusLine();
                    return 1;   //Not Display in from function
                }
            }

            ST7565_BlitFullScreen();
            return 1;   //Not Display in from function
        }


        return 2;
    }                                        //                0      1   2     3    4
    else
        return 0;

}

void DigitalModeStart(uint8_t startMode)
{
//    return;
  uint8_t needDisplay = 1;
  nowMode = startMode;
  //TX Start at Main Screen
  //if (gScreenToDisplay != DISPLAY_MAIN )
  // return;

/*
  //모드를 선택받는다.
  if (startMode == 99)
  {
    while(1)
    {
        memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

        UI_PrintStringSmallNormal("Select Function", 0, 0, 0);
        UI_PrintStringSmallNormal("1)WSPR SEND", 0, 0, 1);
        UI_PrintStringSmallNormal("2)PC Control", 0, 0, 2);

        UI_PrintStringSmallNormal("Select Function", 0, 0, 0);
        UI_PrintStringSmallNormal("1)WSPR SEND", 0, 0, 1);
        UI_PrintStringSmallNormal("2)PC Control", 0, 0, 2);


        ST7565_BlitFullScreen();

        uint8_t aKey = KEYBOARD_Poll();

        if (aKey != KEY_INVALID)
        {
            if (aKey == KEY_1)
            {
                startMode = 2;
                break;
            }
            else if (aKey == KEY_2)
            {
                startMode = 0;
                break;
            }
            else
            {
                UI_PrintStringSmallNormal("Please 1 or 2", 0, 0, 5);
                ST7565_BlitFullScreen();
                delay(5000);
            }
        }

        delay(100);
    }
  }

//  CEC_WaitKeyRelease();
*/


  //int beforeMode = gTxVfo->Modulation;
  digitBeforeFreq = gTxVfo->freq_config_RX.Frequency;
  
  if (digitBeforeFreq < 2900000)
      wsprBandIndex = 0;
  else if (gTxVfo->Band == BAND1_50MHz)
      wsprBandIndex = 1;
  else
      wsprBandIndex = 2;

  //WSPR SEND MODE
  if (startMode == 2)   //Stand alone (WSPR AND MORE)
  {
/*
28.1246 MHz
50.2930 MHz (Region 2, 3)
144.4890 MHz
432.3000 MHz
*/    
/*
    //Get Frequency
    if (beforeFreq < 2900000)
        wsprBandIndex = 0;
    else if (gTxVfo->Band == BAND1_50MHz)
        wsprBandIndex = 1;
    else if (gTxVfo->Band == BAND3_137MHz)
        wsprBandIndex = 2;
    else
    {
        //Display Error Message , 3Second
        return;
    }
   */
    BasicFreq = wsprBandFreq[wsprBandIndex];
  }
  else  //================================== FT8 MODE
  {
    /*
    //Get Frequency
    if (beforeFreq < 2900000)
        wsprBandIndex = 0;
    else if (gTxVfo->Band == BAND1_50MHz)
        wsprBandIndex = 1;
    else
        wsprBandIndex = 2;

    else if (gTxVfo->Band == BAND3_137MHz)
        wsprBandIndex = 2;
    else
    {
        //Display Error Message , 3Second
        return;
    }
    */

   //Change Set Frequency from Remote  (2024.05.21)
    //BasicFreq = FT8BandFreq[wsprBandIndex];
    BasicFreq = digitBeforeFreq;    //Default : VFO Frequency 

    //BasicFreq = 14412000;0021
   //BasicFreq = 14417400;
    IsFT8Mode = 1;
    ScreenControlFromDSP = false;

#ifdef ENABLE_CEC_INTERFACE_CABLE
    CECSWUartInit();
#else    
    //UART_Init(UART_BAUD_1152K_CLOCK_DIV);
    UART_Init(UART_BAUD_57600_CLOCK_DIV);
#endif    

  }

//byte tmpBuff1[80] = { 3, 1, 4, 0, 6, 5, 2, 0, 0, 0, 0, 0, 0, 0, 0, 1, 1, 1, 2, 1, 7, 0, 2, 7, 4, 2, 0, 5, 0, 7, 6, 5, 3, 0, 2, 1, 3, 1, 4, 0, 6, 5, 2, 7, 0, 2, 5, 4, 5, 1, 2, 6, 6, 2, 5, 7, 7, 0, 2, 4, 0, 2, 7, 0, 7, 3, 2, 1, 5, 1, 3, 7, 3, 1, 4, 0, 6, 5, 2 };

    //CLEAR BUFFER
#ifdef ENABLE_CEC_INTERFACE_CABLE
    //CLEAR BUFFER FOR S/W UART
#else
    //CLEAR BUFFER FOR HW UART
  CECHWUartClearBuffer();
#endif  
 
     //if Digital Mode
    if (DigitalMode)
    {
        //Reboot DSP Module for Changed Mode to FT8 from APRS (common live Mode)
        CEC_SendRemoteData(0x21, CEC_CMD_REBOOT, 1, 0, 0);  //1 : AT START, 0 : AT END 
        //delay(200);
    }

    KEY_Code_t beforeKey = KEY_INVALID;
//    uint8_t timeInterval500msec = 0;    //5msec * 100 = 500msec

  //uint32_t lastReceivedTime = 0;
    while (1)
    {
        //if (needDisplay && (! ScreenControlFromDSP))
        if (needDisplay)
        {
            needDisplay = 0;
            DisplayDigitalMode();
            SetReceiveStart();
            CEC_WaitKeyRelease();
        }

        KEY_Code_t aKey = KEYBOARD_Poll();

        if (aKey == KEY_EXIT)
        {
            break;
        }        

        if (startMode == 2)  //WSPR MODE (Stand alone)
        {
            if (aKey >= KEY_1 && aKey <= KEY_3)
            {
//                BACKLIGHT_TurnOn();
                uint8_t wsprIndex = aKey - KEY_1;
                if (ReadWSPRData(wsprIndex))
                {
                    //STX1, STX2, SUBFREQ, DATA...
                    uint8_t tmpWSPRBuff[41];
                    memcpy(tmpWSPRBuff, &CommBuff[3], 41);
                    SendDigitalMode(2, CommBuff[2], tmpWSPRBuff);
                    needDisplay = 1;
                    //SetReceiveStart();
                }
            }
        }
        else                //PC CONNECT MODE
        {
            if (beforeKey != aKey)
            {
                beforeKey = aKey;
                if (aKey != KEY_INVALID)
                {
//                    BACKLIGHT_TurnOn();
                    /*
                    else if (aKey == KEY_MENU)
                    {
                        //REQUEST TIME SYNC 
                        //SendCommand(CEC_CMD_TIMESTART, 0); //
                        CEC_SendRemoteData(0x21, CEC_CMD_TIMESTART, 0, 0, 0);
                        //BackLightBlink(1);
                        delay(500);
                    }
                    */
                    CEC_SendRemoteData(0x21, 200 + aKey, 0x02, 0, 0);   //press
                    //delay(50);
                }
            }

            //needDisplay = ProcessRemoteControl();
            if (ProcessRemoteControl(1) > 1)
            {
                needDisplay = 1;
//                BACKLIGHT_TurnOn();
            }

        }   //end of else

/*
        if (++timeInterval500msec > 100)
        {
            timeInterval500msec = 0;
            if (gBacklightCountdown_500ms > 0 && --gBacklightCountdown_500ms == 0)
            {
                BACKLIGHT_TurnOff();
            }
        }
*/        
        
        delay(5);
    }


//Restore
    BK4819_WriteRegister(0x40, 0x4D0 | (0x01U << BK4819_REG_40_SHIFT_ENABLE_DEVIATION));
    //gTxVfo->Modulation = beforeMode;
    gTxVfo->freq_config_RX.Frequency = digitBeforeFreq;
	RADIO_SelectVfos();
	gUpdateStatus   = true;

#ifdef ENABLE_NOAA
	RADIO_ConfigureNOAA();
#endif
	RADIO_SetupRegisters(true);

  //BackLightBlink(1);
  //RADIO_SetupRegisters(false);
  gUpdateDisplay = true;  
  CEC_SendRemoteData(0x21, CEC_CMD_REBOOT, 0, 0, 0);
  //SendCommand(CEC_CMD_REBOOT, 0); //  
  IsFT8Mode = 0;
}

/*
void RemoteCommunicationMode()
{

    while (1)
    {
        //SYSTICK_DelayUs(700);
        delay(1);

        KEY_Code_t aKey = KEYBOARD_Poll();
        if (aKey != KEY_INVALID)
        {
            if (aKey == KEY_EXIT)
            {
                break;
            }



        }



		if (DigitalMode)
			ProcessRemoteControl();

        

    }


    }
}

*/