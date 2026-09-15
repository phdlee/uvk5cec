#include "ceccommon.h"
#include "cecsstv1.h"

const bool play_speaker1 = true;
const unsigned int level1	= 250;
uint32_t SSTV_Freq = 14439000;

void toneWait(uint16_t freq, long timer) 
{
    BK4819_WriteRegister(BK4819_REG_71, scale_freq_sstv(freq));
	//BK4819_ExitTxMute();

    SYSTEM_DelayMs(timer);
	//BK4819_EnterTxMute();
    //BK4819_WriteRegister(BK4819_REG_71, scale_freq_sstv(freq));

/*
    HStone(hs_mic_pin ,freq);//,timer);
    HSdelay(timer);
    HSnoTone(hs_mic_pin);
*/    
}

/* wait microseconds for tone to complete */

void toneWaitU(uint16_t freq, long timer) 
{
    /*
    if(freq < 16383) 
    { 
        HStone(hs_mic_pin,freq);
        HSdelayMicroseconds(timer); 
        HSnoTone(hs_mic_pin); 
        return;
    }
    HStone(hs_mic_pin,freq);
    HSdelay(timer / 1000); HSnoTone(hs_mic_pin); return;
    */
    BK4819_WriteRegister(BK4819_REG_71, scale_freq_sstv(freq));
	//BK4819_ExitTxMute();

    SYSTICK_DelayUs(timer);
	//BK4819_EnterTxMute();
}

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
    toneWait(1900,300);
    toneWait(1200,10);
    toneWait(1900,300);
    toneWait(1200,30);
        for(int x = 0; x < 7; x++) { 
           if(code&(1<<x)) { toneWait(1100,30); } else { toneWait(1300,30); } 
        } 
        if(parityCalc(code)) { toneWait(1300,30); } else { toneWait(1100,30); } 
        toneWait(1200,30);
        return;
}



void SSTVTestPattern(int code) 
{ 
       SSTVVISCode(code);
       if(code == MARTIN1) 
       {
            for(int x = 0; x < 257; x++)
            {

                toneWaitU(1200,4862);              // sync pulse (4862 uS)
                toneWaitU(1500,572);               // sync porch (572 uS)

                /* Green Channel - 146.432ms a line (we are doing 144ms) */
 
                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(1500,24); 

                toneWaitU(1500,572);               // color separator pulse (572 uS)

                /* Blue Channel - 146.432ms a line (we are doing 144ms) */
 
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(1500,24);
                toneWait(2400,24);  

                toneWaitU(1500,572);               // color separator pulse (572 uS)

                /* Red Channel - 146.432ms a line (we are doing 144ms) */

                toneWait(2400,24);
                toneWait(2400,24);
                toneWait(1500,24);
                toneWait(1500,24);
                toneWait(2400,24);
                toneWait(1500,24); 
 
                toneWaitU(1500,572);               // color separator pulse (572 uS)
             }
     }
}

void MARTIN1_HOR_LINE(int lineHeight)
{
    //Title Line
    for (int retryY = 0; retryY < 3; retryY++)
    {
        toneWaitU(1200,4862);              // sync pulse (4862 uS)
        toneWaitU(1500,572);               // sync porch (572 uS)

        /* Green Channel - 146.432ms a line (we are doing 144ms) */

/*
        for (int i = 0; i < 70; i++)
        {
            if (lineBuff[i] & 0x01 == 0x01)
                toneWaitU(2400, 1800);  //1.125
            else
                toneWaitU(1500, 1800);
        }
*/        
        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);

        toneWaitU(1500,572);               // color separator pulse (572 uS)

        /* Blue Channel - 146.432ms a line (we are doing 144ms) */
        //24 * 6 = 
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);  

        toneWaitU(1500,572);               // color separator pulse (572 uS)

        /* Red Channel - 146.432ms a line (we are doing 144ms) */

        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);
        toneWait(2400,24);

        toneWaitU(1500,572);               // color separator pulse (572 uS)
    }
    
}

const uint8_t BITMAP_MARKING[3] =
{	// "RX"
	0b00000100,
	0b11111111,
	0b00000100,
};

void MARTIN1_START_MARK_CHAR(int lineIndex, int isLeft)
{
    //TOTAL 24 msec
    if (! isLeft)
        toneWait(1500, 11);  //

    for (int i = 0; i < 3; i++)
    {
        if ((BITMAP_MARKING[i] >> lineIndex) & 0x01 == 0x01)
            toneWait(2400,4);
        else
            toneWait(1500,4);
    }
    
    if (isLeft)
        toneWait(1500, 11);  //
}

void MARTIN1_MARK(uint8_t isLeft)
{
    //Title Line
    for (int i = 0; i < 8; i++)
    {
        toneWaitU(1200,4862);              // sync pulse (4862 uS)
        toneWaitU(1500,572);               // sync porch (572 uS)

        //toneWait(1500,24);
        if (isLeft)
            MARTIN1_START_MARK_CHAR(i, isLeft);
        else
            toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        if (! isLeft)
            MARTIN1_START_MARK_CHAR(i, isLeft);
        else
            toneWait(1500,24);

        toneWaitU(1500,572);               // color separator pulse (572 uS)

        /* Blue Channel - 146.432ms a line (we are doing 144ms) */
        //24 * 6 = 
        if (isLeft)
            MARTIN1_START_MARK_CHAR(i, isLeft);
        else
            toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        if (! isLeft)
            MARTIN1_START_MARK_CHAR(i, isLeft);
        else
            toneWait(1500,24);

        toneWaitU(1500,572);               // color separator pulse (572 uS)

        /* Red Channel - 146.432ms a line (we are doing 144ms) */

        if (isLeft)
            MARTIN1_START_MARK_CHAR(i, isLeft);
        else
            toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        toneWait(1500,24);
        if (! isLeft)
            MARTIN1_START_MARK_CHAR(i, isLeft);
        else
            toneWait(1500,24);

        toneWaitU(1500,572);               // color separator pulse (572 uS)
    }
    
}

#define LINE_COLOR_BLACK  0
#define LINE_COLOR_GREEN  1
#define LINE_COLOR_BLUE   2
#define LINE_COLOR_RED    3
#define LINE_COLOR_WHITE  4

#define DELAY_TIME_DRAWTIME 1500  //line width 70 -> 1800
#define SSTV_LINE_WIDTH 88

void MARTIN1_BW_SEND(uint8_t * lineColors) 
{ 
    SSTVVISCode(MARTIN1);

    MARTIN1_HOR_LINE(3);
    MARTIN1_MARK(1);
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
                            toneWaitU(2400, DELAY_TIME_DRAWTIME);  //1.125
                        else
                            toneWaitU(1500, DELAY_TIME_DRAWTIME);
                    }
                }
                else
                {
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                }

                toneWaitU(1500,572);               // color separator pulse (572 uS)

                /* Blue Channel - 146.432ms a line (we are doing 144ms) */
                /* Green Channel - 146.432ms a line (we are doing 144ms) */
                if (aColor == LINE_COLOR_BLUE || aColor == LINE_COLOR_WHITE)
                {
                    for (int i = 0; i < SSTV_LINE_WIDTH; i++)
                    {
                        if (lineBuff[i] & 0x01 == 0x01)
                            toneWaitU(2400, DELAY_TIME_DRAWTIME);  //1.125
                        else
                            toneWaitU(1500, DELAY_TIME_DRAWTIME);
                    }
                }
                else
                {
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                }

                toneWaitU(1500,572);               // color separator pulse (572 uS)

                /* Red Channel - 146.432ms a line (we are doing 144ms) */
                if (aColor == LINE_COLOR_RED || aColor == LINE_COLOR_WHITE)
                {
                    for (int i = 0; i < SSTV_LINE_WIDTH; i++)
                    {
                        if (lineBuff[i] & 0x01 == 0x01)
                            toneWaitU(2400, DELAY_TIME_DRAWTIME);  //1.125
                        else
                            toneWaitU(1500, DELAY_TIME_DRAWTIME);
                    }
                }
                else
                {
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                    toneWait(1500,24);
                }

                toneWaitU(1500,572);               // color separator pulse (572 uS)
            }
            for (int i = 0; i < 70; i++)
                lineBuff[i] = lineBuff[i] >> 1;
        }
    }
    MARTIN1_MARK(0);
    MARTIN1_HOR_LINE(3);
}

void SSTV_MARTIN_Send(int SSTVTYPE)
{
    /*
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    UI_PrintStringLeft("HELLO WORLD", 3, 127, 0, 8);
    UI_PrintStringLeft("KD8CEC", 3, 127, 2, 8);
    UI_PrintStringLeft("FROM KOREA", 3, 127, 4, 8);
    UI_PrintStringSmallLeft("by UV-K5 KD8CE", 3, 70, 6);
    uint8_t lineColors[7] = {LINE_COLOR_BLUE, LINE_COLOR_BLUE, LINE_COLOR_RED, LINE_COLOR_RED, LINE_COLOR_WHITE, LINE_COLOR_WHITE, LINE_COLOR_RED};
    */
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    UI_PrintString("CQ SSTV !", 2, 0, 0, 8);
    UI_PrintString("DE KD8CEC", 2, 0, 2, 8);
    UI_PrintString(" @gmail.com", 2, 0, 4, 8);
    UI_PrintStringSmallNormal("HAPPY NEW YEAR", 2, 0, 6);
    //UI_PrintStringSmallLeft("by UV-K5 KD8CE", 3, 70, 6);
    uint8_t lineColors[7] = {LINE_COLOR_RED, LINE_COLOR_RED, LINE_COLOR_BLUE, LINE_COLOR_BLUE, LINE_COLOR_GREEN, LINE_COLOR_GREEN, LINE_COLOR_RED};


	AUDIO_AudioPathOff();
	//gEnableSpeaker = false;
	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
	BK4819_SetFrequency(SSTV_Freq);
	BK4819_PrepareTransmit();
	SYSTEM_DelayMs(10);
	BK4819_PickRXFilterPathBasedOnFrequency(SSTV_Freq);
	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
	SYSTEM_DelayMs(5);
	BK4819_SetupPowerAmplifier(25, SSTV_Freq);  //
	SYSTEM_DelayMs(10);

    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level1 & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_EnableTXLink();

	BK4819_ExitTxMute();
    MARTIN1_BW_SEND(lineColors);
	BK4819_EnterTxMute();

	if (play_speaker1)
	{
		AUDIO_AudioPathOff();
		BK4819_SetAF(BK4819_AF_MUTE);
	}

	BK4819_ExitTxMute();
//TX STOP
    RADIO_SelectVfos();
#ifdef ENABLE_NOAA
    RADIO_ConfigureNOAA();
#endif
    RADIO_SetupRegisters(true);
}

void SSTV_Test()
{
    BACKLIGHT_TurnOff();
    while(1)
    {
    
        KEY_Code_t tmpKey = KEYBOARD_Poll();

        if (tmpKey == KEY_1)
        {
            SSTV_MARTIN_Send(MARTIN1);

        }
        /*
        else if (tmpKey == KEY_2)
        {
            SSTV_PD120_Send(ROBOT8BW);
        }
        else if (tmpKey == KEY_4)
        {
            SendTestScott();
        }
        else if (tmpKey == KEY_5)
        {
            pd90_loop();
        }
        else if (tmpKey == KEY_3)
        {


            AUDIO_AudioPathOff();
            //gEnableSpeaker = false;
            BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
            BK4819_SetFrequency(SSTV_Freq);
            BK4819_PrepareTransmit();
            SYSTEM_DelayMs(10);
            BK4819_PickRXFilterPathBasedOnFrequency(SSTV_Freq);
            BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
            SYSTEM_DelayMs(5);
            BK4819_SetupPowerAmplifier(25, SSTV_Freq);  //APRS 출력은 설정값으로 저장한다.
            SYSTEM_DelayMs(10);

            BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level1 & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
            BK4819_EnableTXLink();

            BK4819_ExitTxMute();


            sSq = 1;    //Start
            while (KEYBOARD_Poll() != KEY_1)
            {
                Start_BW();
            }
            
            BK4819_EnterTxMute();

            if (play_speaker1)
            {
                AUDIO_AudioPathOff();
                BK4819_SetAF(BK4819_AF_MUTE);
            }

            BK4819_ExitTxMute();
        //TX STOP
            RADIO_SelectVfos();
        #ifdef ENABLE_NOAA
            RADIO_ConfigureNOAA();
        #endif
            RADIO_SetupRegisters(true);            
        }
        SYSTEM_DelayMs(300);
            */
    }
}

void StartSSTVM1(int startType)
{

    /*
    memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
    UI_PrintStringLeft("HELLO WORLD", 3, 127, 0, 8);
    UI_PrintStringLeft("KD8CEC", 3, 127, 2, 8);
    UI_PrintStringLeft("FROM UVK5", 3, 127, 4, 8);
    UI_PrintStringSmallLeft("by UV-K5 KD8CE", 3, 70, 6);
    uint8_t lineColors[7] = {LINE_COLOR_BLUE, LINE_COLOR_BLUE, LINE_COLOR_RED, LINE_COLOR_RED, LINE_COLOR_WHITE, LINE_COLOR_WHITE, LINE_COLOR_RED};
    */
   uint8_t tmpBuff[32];

/*
	AUDIO_AudioPathOff();
	//gEnableSpeaker = false;
	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
	BK4819_SetFrequency(SSTV_Freq);
	BK4819_PrepareTransmit();
	SYSTEM_DelayMs(10);
	BK4819_PickRXFilterPathBasedOnFrequency(SSTV_Freq);
	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
	SYSTEM_DelayMs(5);
	BK4819_SetupPowerAmplifier(25, SSTV_Freq);  //
	SYSTEM_DelayMs(10);

    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level1 & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_EnableTXLink();
*/

    RADIO_PrepareTX();
    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level1 & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_EnableTXLink();
	BK4819_ExitTxMute();

    if (startType != 2)
    {
        memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
        if (startType == 0) //CQ
            UI_PrintString("CQ SSTV", 2, 0, 0, 8);
        else 
        {
            SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_DXCALL);
            UI_PrintString(tmpBuff, 2, 0, 0, 8);
        }
        SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_SSTVMSG1);
        UI_PrintString(tmpBuff, 2, 0, 4, 8);
        //UI_PrintStringLeft(" From UV-K5", 2, 127, 4, 8);
        SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_SSTVMSG2);
        UI_PrintStringSmallNormal(tmpBuff, 2, 0, 6);
    }

    UI_PrintString("DE", 2, 0, 2, 8);
    SETTINGS_FetchChannelName(tmpBuff, RIGINFO_MSG_MYCALL);
    UI_PrintString(tmpBuff, 22, 0, 2, 8);

    uint8_t lineColors[7] = {LINE_COLOR_RED, LINE_COLOR_RED, LINE_COLOR_BLUE, LINE_COLOR_BLUE, LINE_COLOR_GREEN, LINE_COLOR_GREEN, LINE_COLOR_RED};


    MARTIN1_BW_SEND(lineColors);
	BK4819_EnterTxMute();

    //전송종료
	if (play_speaker1)
	{
		AUDIO_AudioPathOff();
		BK4819_SetAF(BK4819_AF_MUTE);
	}

	BK4819_ExitTxMute();
//TX STOP
    RADIO_SelectVfos();
#ifdef ENABLE_NOAA
    RADIO_ConfigureNOAA();
#endif
    RADIO_SetupRegisters(true);
        
}