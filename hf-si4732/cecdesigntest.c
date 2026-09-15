#include "driver/cec_si4735.h"
#include "driver/patch_ssb_compressed.h"    // Compressed SSB patch version (saving almost 1KB)

#include "ceccommon.h"
#include "ui\helper.h"
#include "ui\status.h"
#include "driver\eeprom.h"
#include "helper\battery.h"

#include "bsp\dp32g030\pwmplus.h"
#include "bsp\dp32g030\portcon.h"



#define BAND_BCAST 0
#define BAND_HAM   1
#define MOD_AM  0
#define MOD_LSB 1
#define MOD_USB 2

typedef struct {
  uint8_t   BandType;
  uint16_t  minimumFreq;
  uint16_t  maximumFreq;
  uint16_t  currentFreq;
  uint8_t   currentStep;  //STEP
  uint8_t   currentMode;  //0 : AM, 1:LSB, 2:USB
  int16_t   adjBFO;
  char bandName[5];
} Band;


Band swband[] = {
  {BAND_HAM,    472,   479,   475,  1, MOD_LSB, 0, "630M"},
  {BAND_BCAST,  480,  1790,  1000,  9, MOD_AM,  0, "AMMF"},
  {BAND_HAM,   1800,  2000,  1820,  1, MOD_LSB, 0, "160M"},
  {BAND_BCAST, 2001,  3499,  2500, 10, MOD_AM,  0, "120M"},
  {BAND_HAM,   3500,  4000,  3800,  1, MOD_LSB, 0, "80M"},
  {BAND_BCAST, 4001,  5200,  4700, 10, MOD_AM,  0, "60M"},
  {BAND_BCAST, 5201,  6999,  5700, 10, MOD_AM,  0, "49M"},
  {BAND_HAM,   7000,  7299,  7070,  1, MOD_LSB, 0, "40M"},
  {BAND_BCAST, 7300,  9999,  7500,  5, MOD_AM,  0, "35M"},
  {BAND_HAM,  10000, 10500, 10299,  1, MOD_LSB, 0, "30M"},
  {BAND_BCAST,10300, 12199, 12499,  5, MOD_AM,  0, "25M"},
  {BAND_BCAST,13000, 13999, 13500,  5, MOD_AM,  0, "22M"},
  {BAND_HAM,  14000, 14499, 14050,  1, MOD_USB, 0, "20M"},
  {BAND_BCAST,14500, 16999, 15000,  5, MOD_AM,  0, "19M"},
  {BAND_BCAST,17000, 17999, 17500,  5, MOD_AM,  0, "19M"},
  {BAND_HAM,  18000, 18199, 18050,  1, MOD_USB, 0, "17M"},
  {BAND_BCAST,18200, 20999, 18500,  5, MOD_AM,  0, "16M"},
  {BAND_HAM,  21000, 21499, 21050,  1, MOD_USB, 0, "15M"},
  {BAND_BCAST,21500, 24799, 22500,  5, MOD_AM,  0, "13M"},
  {BAND_HAM,  24800, 25000, 24900,  1, MOD_USB, 0, "12M"},
  {BAND_HAM,  25000, 27999, 27125,  1, MOD_AM,  0, "11M"},
  {BAND_HAM,  28000, 29999, 28200,  1, MOD_USB, 0, "10M"},
};



void CEC_ReverseScreen(uint8_t * _srcBuff, int _buffSize)
{
    //reverse
    for (int i = 0; i < _buffSize; i++)
        _srcBuff[i] = ~_srcBuff[i];
}

static void PutPixel(uint8_t x, uint8_t y, bool fill) {
  UI_DrawPixelBuffer(gFrameBuffer, x, y, fill);
}
static void PutPixelStatus(uint8_t x, uint8_t y, bool fill) {
  UI_DrawPixelBuffer(&gStatusLine, x, y, fill);
}

static void CEC_DisplaySmallest(const char *pString, uint8_t x, uint8_t y,
                                bool statusbar, bool fill) {
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
            PutPixelStatus(x + i, y + j, fill);
        else
            PutPixel(x + i, y + j, fill);
        }
        pixels >>= 1;
      }
    }
    x += 4;
  }
}

const char BandNames [4][4] = {"LF", "", "LSB", "USB"};


void CEC_DESIGN_TEST(void)
{
    uint16_t tmpFreq = 14074;
    uint8_t freqScaleDiv =  1000;

	memset(gStatusLine, 0, sizeof(gStatusLine));
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));	

    uint8_t     *line = gStatusLine;

    CEC_DisplaySmallest(" 10M  AGC-ON  100Hz  3khz", 0, 1, true, true);
    //UI_PrintStringSmallBufferNormal35(" 10M  AGCON  100Hz  3khz", line);
    CEC_ReverseScreen(gStatusLine, sizeof(gStatusLine));
    ST7565_BlitStatusLine();


    /*
    CEC_DisplaySmallest("LF  MF  160M 80M 40M 20M 10M", 5, 3, false, true);
    */
    //void UI_DrawLineBuffer(uint8_t (*buffer)[128], int16_t x1, int16_t y1, int16_t x2, int16_t y2, bool black)

    for (int i = 0; i < 7; i++)
    {
        CEC_DisplaySmallest(swband[i].bandName, 2 + i * 18, 2, false, true);
    }


	//sprintf(strBuff, "%-3s  %3u.%02uMhz", "USB", tmpFreq / freqScaleDiv, (tmpFreq) % freqScaleDiv);
	sprintf(strBuff, "%3u.%02u", tmpFreq / freqScaleDiv, (tmpFreq) % freqScaleDiv);
 	//UI_PrintString(strBuff, 0, 0, 1, 8);
    //sprintf(strbuff, "%3u.%05u", lastGPSInfo_Latitude.value / 100 / lastGPSInfo_Latitude.scale, (lastGPSInfo_Latitude.value /100) % lastGPSInfo_Latitude.scale);
    UI_DisplayFrequency(strBuff, 20, 1, false);
	UI_PrintStringSmallNormal("S5}}}}}}{{{{{{{{{{", 0, 0, 3);

    UI_DrawRectangleBuffer(gFrameBuffer, 46 , 8, 59, 23, true);
    //UI_DrawRectangleBuffer(gFrameBuffer, 46 , 23, 59, 23, true);

    CEC_DisplaySmallest("-3700", 105, 9, false, true);
	UI_PrintStringSmallNormal("Mhz", 105, 0, 2);
	UI_PrintStringSmallNormal("USB", 1, 0, 2);
 	//UI_PrintString("Mhz", 100, 0, 1, 7);

	//CEC_DisplaySmallest("RSSI 35dBuV", 3,  33, false, true);
	CEC_DisplaySmallest("SNR 35", 95,  33, false, true);

/*
	CEC_DisplaySmallest("MODE", 2,  36, false, true);
	CEC_DisplaySmallest("STEP+", 2, 42, false, true);
	CEC_DisplaySmallest("STEP-", 2, 48, false, true);
    UI_DrawRectangleBuffer(gFrameBuffer, 0 , 34, 23, 55, true);
*/
    UI_PrintStringSmallNormal("BND STP LNA BW BFO", 0, 0, 5);
	
    uint8_t * startPT = &gFrameBuffer[5][0];
    startPT += (27 * 4);
    for (int i = 0; i < 24; i++)
    {
        startPT[i] = ~startPT[i];

    }

    memset(gFrameBuffer[6], 0, 128);	

    //UI_PrintStringSmallNormal("BFO", 0, 0, 6);
    UI_PrintStringSmallNormal("BAND : 160M", 0, 0, 6);
    //UI_DrawRectangleBuffer(gFrameBuffer, 23 , 47, 127, 55, true);
    //UI_PrintStringSmallNormal("BSTP BFO  BW  CAP", 0, 0, 6);
    //CEC_DisplaySmallest("80M", 2, 48, false, true);
	//UI_PrintStringSmallNormal("SNR 35dBuV", 30, 0, 4);
	//UI_PrintStringSmallNormal("LNA 3 ", 30, 0, 6);

    uint8_t bandIndex = 3;
    uint8_t drawStartX = (18 * bandIndex);
    UI_DrawRectangleBuffer(gFrameBuffer, 0 + drawStartX , 1, 18 + drawStartX, 7, true);





	/*
	//UI_PrintString("This is Test", 0, 127, 3);	
	//UI_PrintStringSmallNormal(strBuff, 0, 0, _lineNumber + 2);
	UI_PrintStringSmallNormal("this is test", 0, 0, 1);
 	UI_PrintString("'this is test", 1, 128, 3, 8);
	*/

    //for (int i = 0; i < sizeof(gFrameBuffer); i++)
    //    gFrameBuffer[i] = ~gFrameBuffer[i];
    //CEC_ReverseScreen(&gFrameBuffer[0][0], sizeof(gFrameBuffer));

    ST7565_BlitFullScreen();    


//return;
while(1);

	//char nowModeStr[4];
	//uint8_t dispStep =  0;
	//uint16_t freqScaleDiv = 1000;
	//uint16_t tmpFreq = SI4735_getFrequency();
    /*
	if (NowMode == 0)	//FMSI47
	{
		CurrentFMFreq = tmpFreq;
		freqScaleDiv = 100;
		dispStep = FMFreqStep;
	}
	else if (NowMode == 1)	//AM
	{
		CurrentAMFreq = tmpFreq;
		dispStep = AMFreqStep;
	}
	else if (NowMode == 2) //LSB
	{
		CurrentAMFreq = tmpFreq;
		dispStep = SSBFreqStep;
	}
	else if (NowMode == 3) //USB
	{
		CurrentAMFreq = tmpFreq;
		dispStep = SSBFreqStep;
	}
	//UI_DisplayStatus();
	memset(gStatusLine, 0, sizeof(gStatusLine));	
	uint8_t     *line = gStatusLine;
	//UI_PrintStringSmallBufferNormal("UV-K5 HF AllBand", line);
	//ST7565_BlitStatusLine();
  	//memset(gStatusLine, 0, sizeof(gStatusLine));
	BATTERY_GetReadings(false);
  	UI_DisplayStatus();
  	//ST7565_BlitStatusLine();

	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));	
	//LINE #1
	//sprintf(strBuff, "%5s ", aVal++);

	//gStatusLine

//	if (NowMode == 0)	//FM
	sprintf(strBuff, NowMode == MODE_FM ? "%-3s  %3u.%02uMhz" : "%-3s %3u.%03uMhz ", modeName[NowMode], tmpFreq / freqScaleDiv, (tmpFreq) % freqScaleDiv);
 	UI_PrintString(strBuff, 0, 0, 0, 8);

	sprintf(strBuff, "AGC(2): %3s ST*:%d", disableAgc ? "OFF" : "ON", dispStep );
	UI_PrintStringSmallNormal(strBuff, 0, 0, 2);
	sprintf(strBuff, "BFO(4):%4d ATT(5)",  currentBFO);
	UI_PrintStringSmallNormal(strBuff, 0, 0, 3);

	sprintf(strBuff, "BW (1):%3sKhz %2d",  bandwitdth[bandwidthIdx], currentAGCAtt );
	//UI_PrintStringSmall(strBuff, 0, 127, 3);
	UI_PrintStringSmallNormal(strBuff, 0, 0, 4);

	UI_PrintStringSmallNormal("UV-K5 HF V0.2", 0, 0, 6);
	//UI_PrintStringSmallBufferNormal("UV-K5 HF AllBand", line);
*/

	/*
	//UI_PrintString("This is Test", 0, 127, 3);	
	//UI_PrintStringSmallNormal(strBuff, 0, 0, _lineNumber + 2);
	UI_PrintStringSmallNormal("this is test", 0, 0, 1);
 	UI_PrintString("'this is test", 1, 128, 3, 8);
	*/
    ST7565_BlitFullScreen();    
}