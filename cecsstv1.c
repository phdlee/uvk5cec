#include "ceccommon.h"
#include "cecsstv1.h"
#include "cecmorse.h"


uint8_t SSTV_Protocol = 0;  //0: MARTIN 1, 1 : SCOTTIE1
uint8_t SSTV_SendCW = 0;  //0: MARTIN 1, 1 : SCOTTIE1


//const bool play_speaker1 = true;
//uint32_t SSTV_Freq = 14439000;

#define BIT_SET_TONE_FREQ 2300

void toneWait(uint16_t freq, long timer) 
{
    BK4819_WriteRegister(BK4819_REG_71, scale_freq_sstv(freq));
    SYSTEM_DelayMs(timer);
}

void toneWaitU(uint16_t freq, long timer) 
{
    BK4819_WriteRegister(BK4819_REG_71, scale_freq_sstv(freq));
    SYSTICK_DelayUs(timer);
}

/*
static inline void sstvToneGen(uint16_t freq) 
{
    BK4819_WriteRegister(BK4819_REG_71, scale_freq_sstv(freq));
}
*/

bool parityCalc(int code) 
{
     unsigned int v;       // word value to compute the parity of
     bool parity = false;  // parity will be the parity of v

    while (code)
    {
       parity = !parity;
       code = code & (code - 1);
    }

    return parity;
}

void SSTVVISCode(int code) 
{
    if (code == MARTIN1)
    {
        toneWait(1900,300);
        toneWait(1200,10);
        toneWait(1900,300);
        toneWait(1200,30);
    for(int x = 0; x < 7; x++) { 
        if(code&(1<<x)) { toneWait(1100,30); } else { toneWait(1300,30); } 
        } 
        if(parityCalc(code)) { toneWait(1300,30); } else { toneWait(1100,30); } 
        toneWait(1200,30);
    }
    else if (code == SCOTTIE1)
    {
        /** VOX TONE (OPTIONAL) **/
        toneWait(1900, 100);
        toneWait(1500, 100);
        toneWait(1900, 100);
        toneWait(1500, 100);
        toneWait(2300, 100);
        toneWait(1500, 100);
        toneWait(2300, 100);
        toneWait(1500, 100);

        /** CALIBRATION HEADER **/

        toneWait(1900, 300);
        toneWait(1200, 10);
        toneWait(1900, 300);
        toneWait(1200, 30);
        toneWait(1300, 30);    // 0
        toneWait(1300, 30);    // 0
        toneWait(1100, 30);    // 1
        toneWait(1100, 30);    // 1
        toneWait(1100, 30);    // 1
        toneWait(1100, 30);    // 1
        toneWait(1300, 30);    // 0
        toneWait(1300, 30);    // Even parity
        toneWait(1200, 30);    // VIS stop bit
    }
    return;
}



// void MARTIN1_HOR_LINE(int lineHeight)
// {
//     //Title Line
//     for (int retryY = 0; retryY < 3; retryY++)
//     {
//         toneWaitU(1200,4862);              // sync pulse (4862 uS)
//         toneWaitU(1500,572);               // sync porch (572 uS)

//         /* Green Channel - 146.432ms a line (we are doing 144ms) */

// /*
//         for (int i = 0; i < 70; i++)
//         {
//             if (lineBuff[i] & 0x01 == 0x01)
//                 toneWaitU(BIT_SET_TONE_FREQ, 1800);  //1.125
//             else
//                 toneWaitU(1500, 1800);
//         }
// */        
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);

//         toneWaitU(1500,572);               // color separator pulse (572 uS)

//         /* Blue Channel - 146.432ms a line (we are doing 144ms) */
//         //24 * 6 = 
//         toneWait(1500,24);
//         toneWait(1500,24);
//         toneWait(1500,24);
//         toneWait(1500,24);
//         toneWait(1500,24);
//         toneWait(1500,24);  

//         toneWaitU(1500,572);               // color separator pulse (572 uS)

//         /* Red Channel - 146.432ms a line (we are doing 144ms) */

//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);
//         toneWait(BIT_SET_TONE_FREQ,24);

//         toneWaitU(1500,572);               // color separator pulse (572 uS)
//     }
    
// }



#define LINE_COLOR_BLACK  0
#define LINE_COLOR_GREEN  1
#define LINE_COLOR_BLUE   2
#define LINE_COLOR_RED    3
#define LINE_COLOR_WHITE  4

#define DELAY_TIME_DRAWTIME 886  //line width 70 -> 1800
//88 at 1500
//#define DELAY_TIME_DRAWTIME 1500  //line width 70 -> 1800
#define MARTIN1_PIXEL_TIME 877  //877 or 878
//#define SCOTTIE_PIXEL_TIME 1036   //for 88width
#define SCOTTIE_PIXEL_TIME 786  //org : 822 790 -> 780
//#define SSTV_LINE_WIDTH 88
#define SSTV_LINE_WIDTH 128


void SSTV_SEND(int SSTV_Codec,  uint8_t * lineColors) 
{ 
    uint16_t pixelDelayTime = 0;
    uint16_t colorPerLineLength = 0;
    uint16_t colorSPtime = 0;
    SSTVVISCode(SSTV_Codec);

    if (SSTV_Codec == MARTIN1)
    {
        pixelDelayTime = MARTIN1_PIXEL_TIME;
        colorPerLineLength = 146;
        colorSPtime = 572;
        //MARTIN1_HOR_LINE(3);
        //MARTIN1_MARK(1);
    }
    else
    {
        pixelDelayTime = SCOTTIE_PIXEL_TIME;
        colorPerLineLength = 138;
        colorSPtime = 1050;
        /** STARTING SYNC PULSO (FIRST LINE ONLY)  **/
        toneWait(1200, 9);
    }

    for (int line1 = 0; line1 < 7; line1++)
    {
        uint8_t *lineBuff = gFrameBuffer[line1];//gFrameBuffer[Line][Column];
        uint8_t aColor = lineColors[line1];
        for(int x = 0; x < 8; x++)      //
        {
            for (int retryY = 0; retryY < 3; retryY++)
            {
                toneWaitU(1200,4862);              // sync pulse (4862 uS)
                toneWaitU(1500,572);               // sync porch (572 uS)

                /* Green Channel - 146.432ms a line (we are doing 144ms) */
                if (aColor == LINE_COLOR_GREEN || aColor == LINE_COLOR_WHITE)
                {
                    for (int i = 0; i < SSTV_LINE_WIDTH; i++)
                    {
                        if (lineBuff[i] & 0x01 == 0x01)
                            toneWaitU(BIT_SET_TONE_FREQ, pixelDelayTime);  //1.125
                        else
                            toneWaitU(1500, pixelDelayTime);
                    }
                }
                else
                {
                    toneWait(1500, colorPerLineLength);
                    /*
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    */
                }

                toneWaitU(1500,colorSPtime);               // color separator pulse (572 uS)

                /* Blue Channel - 146.432ms a line (we are doing 144ms) */
                /* Green Channel - 146.432ms a line (we are doing 144ms) */
                if (aColor == LINE_COLOR_BLUE || aColor == LINE_COLOR_WHITE)
                {
                    for (int i = 0; i < SSTV_LINE_WIDTH; i++)
                    {
                        if (lineBuff[i] & 0x01 == 0x01)
                            toneWaitU(BIT_SET_TONE_FREQ, pixelDelayTime);  //1.125
                        else
                            toneWaitU(1500, pixelDelayTime);
                    }
                }
                else
                {
                    toneWait(1500,colorPerLineLength);
                }

                if (SSTV_Codec == SCOTTIE1)
                    toneWait(1200, 9);

                toneWaitU(1500, colorSPtime);               // color separator pulse (572 uS)

                /* Red Channel - 146.432ms a line (we are doing 144ms) */
                if (aColor == LINE_COLOR_RED || aColor == LINE_COLOR_WHITE)
                {
                    for (int i = 0; i < SSTV_LINE_WIDTH; i++)
                    //for (int i =  SSTV_LINE_WIDTH - 1; i >= 0; i--)
                    {
                        if (lineBuff[i] & 0x01 == 0x01)
                            toneWaitU(BIT_SET_TONE_FREQ, pixelDelayTime);  //1.125
                        else
                            toneWaitU(1500, pixelDelayTime);
                    }
                }
                else
                {
                    toneWait(1500,colorPerLineLength);
                }

                if (SSTV_Codec == MARTIN1)
                    toneWaitU(1500,572);               // color separator pulse (572 uS)
            }
            for (int i = 0; i < SSTV_LINE_WIDTH; i++)
                lineBuff[i] = lineBuff[i] >> 1;
        }
    }

    if (SSTV_Codec == MARTIN1)
    {
        //MARTIN1_MARK(0);
        //MARTIN1_HOR_LINE(3);
    }
}




// void MARTIN1_BW_SEND(uint8_t * lineColors) 
// { 
//     SSTVVISCode(MARTIN1);

//     MARTIN1_HOR_LINE(3);
//     MARTIN1_MARK(1);
//     for (int line1 = 0; line1 < 7; line1++)
//     {
//         uint8_t *lineBuff = gFrameBuffer[line1];//gFrameBuffer[Line][Column];
//         uint8_t aColor = lineColors[line1];
//         for(int x = 0; x < 8; x++)      //
//         {
//             for (int retryY = 0; retryY < 3; retryY++)
//             {
//                 toneWaitU(1200,4862);              // sync pulse (4862 uS)
//                 toneWaitU(1500,572);               // sync porch (572 uS)

//                 /* Green Channel - 146.432ms a line (we are doing 144ms) */
//                 if (aColor == LINE_COLOR_GREEN || aColor == LINE_COLOR_WHITE)
//                 {
//                     for (int i = 0; i < SSTV_LINE_WIDTH; i++)
//                     {
//                         if (lineBuff[i] & 0x01 == 0x01)
//                             toneWaitU(BIT_SET_TONE_FREQ, DELAY_TIME_DRAWTIME);  //1.125
//                         else
//                             toneWaitU(1500, DELAY_TIME_DRAWTIME);
//                     }
//                 }
//                 else
//                 {
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                 }

//                 toneWaitU(1500,572);               // color separator pulse (572 uS)

//                 /* Blue Channel - 146.432ms a line (we are doing 144ms) */
//                 /* Green Channel - 146.432ms a line (we are doing 144ms) */
//                 if (aColor == LINE_COLOR_BLUE || aColor == LINE_COLOR_WHITE)
//                 {
//                     for (int i = 0; i < SSTV_LINE_WIDTH; i++)
//                     {
//                         if (lineBuff[i] & 0x01 == 0x01)
//                             toneWaitU(BIT_SET_TONE_FREQ, DELAY_TIME_DRAWTIME);  //1.125
//                         else
//                             toneWaitU(1500, DELAY_TIME_DRAWTIME);
//                     }
//                 }
//                 else
//                 {
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                 }

//                 toneWaitU(1500,572);               // color separator pulse (572 uS)

//                 /* Red Channel - 146.432ms a line (we are doing 144ms) */
//                 if (aColor == LINE_COLOR_RED || aColor == LINE_COLOR_WHITE)
//                 {
//                     for (int i = 0; i < SSTV_LINE_WIDTH; i++)
//                     {
//                         if (lineBuff[i] & 0x01 == 0x01)
//                             toneWaitU(BIT_SET_TONE_FREQ, DELAY_TIME_DRAWTIME);  //1.125
//                         else
//                             toneWaitU(1500, DELAY_TIME_DRAWTIME);
//                     }
//                 }
//                 else
//                 {
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                     toneWait(1500,24);
//                 }

//                 toneWaitU(1500,572);               // color separator pulse (572 uS)
//             }
//             for (int i = 0; i < 70; i++)
//                 lineBuff[i] = lineBuff[i] >> 1;
//         }
//     }
//     MARTIN1_MARK(0);
//     MARTIN1_HOR_LINE(3);
// }



// void SCOTTIE_SEND(uint8_t * lineColors) 
// { 


//     /** STARTING SYNC PULSO (FIRST LINE ONLY)  **/
//     toneWait(1200, 9);

//     for (int line1 = 0; line1 < 7; line1++)
//     {
//         uint8_t *lineBuff = gFrameBuffer[line1];//gFrameBuffer[Line][Column];
//         uint8_t aColor = lineColors[line1];
//         for(int x = 0; x < 8; x++)      //
//         {
//             for (int retryY = 0; retryY < 3; retryY++)
//             {
//                 // Separator pulse
//                 toneWaitU(1500, 1050);

//                 //// .4320ms/pixel //160 milisecnd
//                 if (aColor == LINE_COLOR_GREEN || aColor == LINE_COLOR_WHITE)
//                 {
//                     for (int i = 0; i < SSTV_LINE_WIDTH; i++)
//                     {
//                         //transmit_micro(1500 + 3 * buffG[i], 1306);    // .4320ms/pixel //160 milisecnd                        
//                         if (lineBuff[i] & 0x01 == 0x01)
//                             toneWaitU(BIT_SET_TONE_FREQ, SCOTTIE_PIXEL_TIME);  //1.125
//                         else
//                             toneWaitU(1500, SCOTTIE_PIXEL_TIME);
//                     }
//                 }
//                 else
//                 {
//                     toneWait(1500, 138);
//                 }

//                 toneWaitU(1500, 1050);        

//                //// .4320ms/pixel //160 milisecnd
//                 if (aColor == LINE_COLOR_BLUE || aColor == LINE_COLOR_WHITE)
//                 {
//                     for (int i = 0; i < SSTV_LINE_WIDTH; i++)
//                     {
//                         if (lineBuff[i] & 0x01 == 0x01)
//                             toneWaitU(BIT_SET_TONE_FREQ, SCOTTIE_PIXEL_TIME);  //1.125
//                         else
//                             toneWaitU(1500, SCOTTIE_PIXEL_TIME);
//                     }
//                 }
//                 else
//                 {
//                     toneWait(1500, 138);
//                 }

//                 toneWait(1200, 9);               // color separator pulse (572 uS)
//                 toneWaitU(1500, 1050);               // color separator pulse (572 uS)

//                //// .4320ms/pixel //160 milisecnd
//                 if (aColor == LINE_COLOR_RED || aColor == LINE_COLOR_WHITE)
//                 {
//                     for (int i = 0; i < SSTV_LINE_WIDTH; i++)
//                     {
//                         if (lineBuff[i] & 0x01 == 0x01)
//                             toneWaitU(BIT_SET_TONE_FREQ, SCOTTIE_PIXEL_TIME);  //1.125
//                         else
//                             toneWaitU(1500, SCOTTIE_PIXEL_TIME);
//                     }
//                 }
//                 else
//                 {
//                     toneWait(1500, 138);
//                 }

//             }

//             for (int i = 0; i < 70; i++)
//                 lineBuff[i] = lineBuff[i] >> 1;
//         }
//     }
//     //MARTIN1_MARK(0);
//     //MARTIN1_HOR_LINE(3);
// }

// void SSTV_MARTIN_Send(int SSTVTYPE)
// {
//     /*
//     memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
//     UI_PrintStringLeft("HELLO WORLD", 3, 127, 0, 8);
//     UI_PrintStringLeft("KD8CEC", 3, 127, 2, 8);
//     UI_PrintStringLeft("FROM KOREA", 3, 127, 4, 8);
//     UI_PrintStringSmallLeft("by UV-K5 KD8CE", 3, 70, 6);
//     uint8_t lineColors[7] = {LINE_COLOR_BLUE, LINE_COLOR_BLUE, LINE_COLOR_RED, LINE_COLOR_RED, LINE_COLOR_WHITE, LINE_COLOR_WHITE, LINE_COLOR_RED};
//     */
//     memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
//     UI_PrintString("CQ SSTV !", 2, 0, 0, 8);
//     UI_PrintString("DE KD8CEC", 2, 0, 2, 8);
//     UI_PrintString(" @gmail.com", 2, 0, 4, 8);
//     UI_PrintStringSmallNormal("HAPPY NEW YEAR", 2, 0, 6);
//     //UI_PrintStringSmallLeft("by UV-K5 KD8CE", 3, 70, 6);
//     uint8_t lineColors[7] = {LINE_COLOR_RED, LINE_COLOR_RED, LINE_COLOR_BLUE, LINE_COLOR_BLUE, LINE_COLOR_GREEN, LINE_COLOR_GREEN, LINE_COLOR_RED};


// 	AUDIO_AudioPathOff();
// 	//gEnableSpeaker = false;
// 	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
// 	BK4819_SetFrequency(SSTV_Freq);
// 	BK4819_PrepareTransmit();
// 	SYSTEM_DelayMs(10);
// 	BK4819_PickRXFilterPathBasedOnFrequency(SSTV_Freq);
// 	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
// 	SYSTEM_DelayMs(5);
// 	BK4819_SetupPowerAmplifier(25, SSTV_Freq);  //
// 	SYSTEM_DelayMs(10);

//     BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level1 & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
//     BK4819_EnableTXLink();

// 	BK4819_ExitTxMute();
//     MARTIN1_BW_SEND(lineColors);
// 	BK4819_EnterTxMute();

// 	if (play_speaker1)
// 	{
// 		AUDIO_AudioPathOff();
// 		BK4819_SetAF(BK4819_AF_MUTE);
// 	}

// 	BK4819_ExitTxMute();
// //TX STOP
//     RADIO_SelectVfos();
// #ifdef ENABLE_NOAA
//     RADIO_ConfigureNOAA();
// #endif
//     RADIO_SetupRegisters(true);
// }

// void SSTV_Test()
// {
//     BACKLIGHT_TurnOff();
//     while(1)
//     {
    
//         KEY_Code_t tmpKey = KEYBOARD_Poll();

//         if (tmpKey == KEY_1)
//         {
//             SSTV_MARTIN_Send(MARTIN1);

//         }
//         /*
//         else if (tmpKey == KEY_2)
//         {
//             SSTV_PD120_Send(ROBOT8BW);
//         }
//         else if (tmpKey == KEY_4)
//         {
//             SendTestScott();
//         }
//         else if (tmpKey == KEY_5)
//         {
//             pd90_loop();
//         }
//         else if (tmpKey == KEY_3)
//         {


//             AUDIO_AudioPathOff();
//             //gEnableSpeaker = false;
//             BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
//             BK4819_SetFrequency(SSTV_Freq);
//             BK4819_PrepareTransmit();
//             SYSTEM_DelayMs(10);
//             BK4819_PickRXFilterPathBasedOnFrequency(SSTV_Freq);
//             BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
//             SYSTEM_DelayMs(5);
//             BK4819_SetupPowerAmplifier(25, SSTV_Freq);  //APRS 출력은 설정값으로 저장한다.
//             SYSTEM_DelayMs(10);

//             BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level1 & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
//             BK4819_EnableTXLink();

//             BK4819_ExitTxMute();


//             sSq = 1;    //Start
//             while (KEYBOARD_Poll() != KEY_1)
//             {
//                 Start_BW();
//             }
            
//             BK4819_EnterTxMute();

//             if (play_speaker1)
//             {
//                 AUDIO_AudioPathOff();
//                 BK4819_SetAF(BK4819_AF_MUTE);
//             }

//             BK4819_ExitTxMute();
//         //TX STOP
//             RADIO_SelectVfos();
//         #ifdef ENABLE_NOAA
//             RADIO_ConfigureNOAA();
//         #endif
//             RADIO_SetupRegisters(true);            
//         }
//         SYSTEM_DelayMs(300);
//             */
//     }
// }

void StartSSTVM1(int startType)
{

   uint8_t tmpBuff[16];
   uint8_t lineColors[7];
    unsigned int level1	= 250;

    //Load From Memory Info
    typedef union {
        struct {
            uint8_t magickey1 : 8;
            uint8_t magickey2 : 8;
            uint16_t serialNumber : 16;
            uint8_t sColor1 : 2;
            uint8_t sColor2 : 2;
            uint8_t sColor3 : 2;
            uint8_t sColor4 : 2;
            uint8_t sColor5 : 2;
            uint8_t sColor6 : 2;
            uint8_t sColor7 : 2;
            uint8_t sColor8 : 2;
            uint8_t reserve1 : 8;
            uint8_t reserve2 : 8;
        };
        uint8_t __tmpBuff[8];
    } stvConfi_t;

    stvConfi_t sstvConfig;

	EEPROM_ReadBuffer(CEC_EEPROM_SSTVCONFIG, sstvConfig.__tmpBuff, 8);
    if (sstvConfig.magickey1 != 0x57 || sstvConfig.magickey2 != 0x59)   //Check Key
    {
        sstvConfig.magickey1 = 0x57;
        sstvConfig.magickey2 = 0x59;
        sstvConfig.serialNumber = 0;

        //{LINE_COLOR_RED, LINE_COLOR_RED, LINE_COLOR_BLUE, LINE_COLOR_BLUE, LINE_COLOR_GREEN, LINE_COLOR_GREEN, LINE_COLOR_RED}
        sstvConfig.sColor1 = LINE_COLOR_RED;
        sstvConfig.sColor2 = LINE_COLOR_RED;
        sstvConfig.sColor3 = LINE_COLOR_BLUE;
        sstvConfig.sColor4 = LINE_COLOR_BLUE;
        sstvConfig.sColor5 = LINE_COLOR_GREEN;
        sstvConfig.sColor6 = LINE_COLOR_GREEN;
        sstvConfig.sColor7 = LINE_COLOR_RED;
    }
    lineColors[0] = sstvConfig.sColor1;
    lineColors[1] = sstvConfig.sColor2;
    lineColors[2] = sstvConfig.sColor3;
    lineColors[3] = sstvConfig.sColor4;
    lineColors[4] = sstvConfig.sColor5;
    lineColors[5] = sstvConfig.sColor6;
    lineColors[6] = sstvConfig.sColor7;

/*
    //READY SEND 
    RADIO_PrepareTX();
    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level1 & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_EnableTXLink();
	BK4819_ExitTxMute();
*/
    PrepareSWFSKTX();

    if (startType != 2)
    {
        memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
        if (startType == 0) //CQ
        {
            UI_PrintString("CQ SSTV", 5, 0, 0, 8);
        }
        else 
        {
            //if QSO Increase SSTV Number
            SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_DXCALL);
            UI_PrintString(tmpBuff, 4, 0, 0, 8);

            //SEND OP NAME IF QSO 
            SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_MYNAME);
            tmpBuff[7] = 0;
            UI_PrintStringSmallNormal(tmpBuff, 73, 0, 6);
        }
        SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_SSTVMSG1);
        UI_PrintString(tmpBuff, 4, 0, 4, 8);
        SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_SSTVMSG2);
        UI_PrintStringSmallNormal(tmpBuff, 4, 0, 6);

        //UI_PrintStringLeft(" From UV-K5", 2, 127, 4, 8);
    }

    uint8_t tmpName1[11];
    SETTINGS_FetchChannelName(tmpName1, RIGINFO_MSG_MYCALL);

    //For reduce program memory, not support dx callsign on lcd screen mode
    /*
    if (startType == 2)
    {
        uint8_t tmpName2[11];
        SETTINGS_FetchChannelName(tmpName2, RIGINFO_MSG_DXCALL);
        sprintf(tmpBuff, "%s %s", tmpName1, tmpName2);
    }
    else
    */
        sprintf(tmpBuff, "DE %s", tmpName1);

    memset(gFrameBuffer[2], 0, 128);
    UI_PrintString(tmpBuff, 2, 125, 2, 8);

    sprintf(tmpBuff, "%05d", sstvConfig.serialNumber);
    UI_PrintStringSmallNormal(tmpBuff, 88, 0, startType == 2 ? 6 : 0);

    //MARK LINE DRAW
    if (startType == 0)
    {
        UI_DrawLineBuffer(gFrameBuffer, 0, 1, 3, 1, true);    
        UI_DrawLineBuffer(gFrameBuffer, 1, 0, 1, 14, true);    
        UI_DrawLineBuffer(gFrameBuffer, 65, 0, 65, 14, true);    
        UI_DrawLineBuffer(gFrameBuffer, 0, 13, 67, 13, true);    
    }
    UI_DrawLineBuffer(gFrameBuffer, 0, 55, 125, 55, true);    



    ST7565_BlitFullScreen();


    //CECTimer0Enable(1);
    //MARTIN1_BW_SEND(lineColors);
    //SSTV_SEND(MARTIN1, lineColors);
    SSTV_SEND(SSTV_Protocol, lineColors);
    //SCOTTIE_SEND(lineColors);
    //SSTVTestPattern(MARTIN1);

    //CECTimer0Disable();
    //Write
    if (startType != 0)
    {
        sstvConfig.serialNumber++;
    }
    EEPROM_WriteBuffer(CEC_EEPROM_SSTVCONFIG, sstvConfig.__tmpBuff);    

    if (SSTV_SendCW)
    {
        delay(1000);
        //CW_SPEED           = 1200 / CW_WPM;
        CW_SPEED = 90;
        PlayCWMemory(RIGINFO_MSG_MYCALL);
    }
//	BK4819_EnterTxMute();

/*
	if (play_speaker1)
	{
		AUDIO_AudioPathOff();
		BK4819_SetAF(BK4819_AF_MUTE);
	}
*/

//	BK4819_ExitTxMute();

    RestoreReceiveMode();

    /*
//TX STOP
    RADIO_SelectVfos();
#ifdef ENABLE_NOAA
    RADIO_ConfigureNOAA();
#endif
    RADIO_SetupRegisters(true);
    */
        
}