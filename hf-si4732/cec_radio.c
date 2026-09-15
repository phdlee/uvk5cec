#include "driver/cec_si4735.h"
#include "driver/patch_ssb_compressed.h"    // Compressed SSB patch version (saving almost 1KB)

#include "ceccommon.h"
#include "ui/helper.h"
#include "ui/status.h"
#include "driver/eeprom.h"
#include "helper/battery.h"

#include "bsp/dp32g030/pwmplus.h"
#include "bsp/dp32g030/portcon.h"
const uint16_t size_content = sizeof ssb_patch_content; // See ssb_patch_content.h
const uint16_t cmd_0x15_size = sizeof cmd_0x15;         // Array of lines where the 0x15 command occurs in the patch content.


#define FM_FUNCTION 0
#define AM_FUNCTION 1
#define RESET_PIN 12
// #define RESET_PIN 9


#define LSB 1
#define USB 2
#define MAX_BANDWIDTH_COUNT 6
#define DEFAULT_BANDWIDTH_INDEX 2
#define DEFAULT_BANDWIDTH_INDEX_CW 5

int currentBFO = 0;
uint8_t currentBFOStep = 10;


/*
SSB Audio Bandwidth:
0 = 1.2 kHz low-pass filter*1. (default)
1 = 2.2 kHz low-pass filter*1.
2 = 3.0 kHz low-pass filter.
3 = 4.0 kHz low-pass filter.
4 = 500 Hz band-pass filter for receiving CW signal, i.e. [250 Hz, 750 Hz]
with center frequency at 500 Hz when USB is selected or [-250 Hz, -750
Hz] with center frequency at -500Hz when LSB is selected*1.
5 = 1 kHz band-pass filter for receiving CW signal, i.e. [500 Hz, 1500 Hz]
with center frequency at 1 kHz when USB is selected or [-500 Hz, -1500
Hz] with center frequency at -1kHz when LSB is selected*1.
Other values = reserved.
Note:
1. If audio bandwidth selected is about 2 kHz or below, it is recommended to set SBCUTFLT[3:0] to 0 to enable the band pass filter for better high cut performance on the wanted side band. Otherwise, set it to 1.
*/

//SSB 14.074
//uint16_t currentRadioFreq = 14074;
uint8_t bandwidthIdx = DEFAULT_BANDWIDTH_INDEX;
uint8_t restoreBandwidthIdx;
uint8_t isAutoSave = 0;

//const char *bandwitdth[] = {"6", "4", "3", "2", "1", "1.8", "2.5"};
const char *bandwitdth[] = {"1.2", "2.2", "3.0", "4.0", "0.5", "1.0"};

uint8_t currentAGCAtt = 0;


//============ SCREEN DESIGN (Version 0.1 ~ 0.2 )==================
// MODE Switch Change AM ->  LSB -> USB
// USB    7.075 KHz
// AGC ON      STEP:1   <-- Change when STEP Switch 
// BFO: 0Hz   St : 25
// BW : 3.0Khz  30  S:2 0%

//Switch BAND+, BAND-
//STEP , MODE, BW : 0.5, 1.0, 1.2, 2.2  3.0, 4.0 
//===============================================
const char modeName [5][4] = {"FM", "AM", "LSB", "USB", "CW"};	//CW IS USB AND SHIFT 700hz
#define MODE_FM  0
#define MODE_AM  1
#define MODE_LSB 2
#define MODE_USB 3
#define MODE_CW  4

uint8_t NowMode = 0;	//0:FM, 1:AM, 2:LSB, 3:USB, 4: CW (USB - 1KHz)
uint16_t CurrentFMFreq = 9310;
uint16_t CurrentAMFreq = 14074;
//uint16_t CurrentSSBFreq = 14074;
//uint8_t CurrentBandIndex = 0;
uint8_t AMFreqStep = 2;	//10
uint8_t SSBFreqStep = 0; //1
uint8_t FMFreqStep = 1;	//10
bool disableAgc = true;

uint8_t CurrentFunctinIndex = 0;	//BAND, STEP, LNA, BW, BFO


#define BAND_BCAST 0
#define BAND_HAM   1
/*
#define MOD_AM  0
#define MOD_LSB 1
#define MOD_USB 2
#define MOD_CW 3
*/

typedef struct {
  uint8_t   BandType;
  uint16_t  minimumFreq;
  uint16_t  maximumFreq;
//  uint16_t  currentFreq;
  uint8_t   currentStep;  //STEP
  uint8_t   currentMode;  //0 : AM, 1:LSB, 2:USB
  int16_t   adjBFO;
  char bandName[5];
} Band;

//23 Band (0.3HF Version)
/*
Band swband[] = {
  {BAND_HAM,    150,   471,   200,  1, MOD_LSB, 0, "L.F"},
  {BAND_HAM,    472,   479,   475,  1, MOD_LSB, 0, "630M"},
  {BAND_BCAST,  480,  1799,  1000,  1, MOD_AM,  0, "AMMF"},
  {BAND_HAM,   1800,  2000,  1820,  1, MOD_LSB, 0, "160M"},
  {BAND_BCAST, 2001,  3499,  2500,  1, MOD_AM,  0, "120M"},
  {BAND_HAM,   3500,  4000,  3800,  1, MOD_LSB, 0, "80M"},
  {BAND_BCAST, 4001,  5200,  4700,  1, MOD_AM,  0, "60M"},
  {BAND_BCAST, 5201,  6999,  5700,  1, MOD_AM,  0, "49M"},
  {BAND_HAM,   7000,  7300,  7070,  1, MOD_LSB, 0, "40M"},
  {BAND_BCAST, 7301,  9999,  7500,  1, MOD_AM,  0, "35M"},
  {BAND_HAM,  10000, 10500, 10299,  1, MOD_LSB, 0, "30M"},
  {BAND_BCAST,10501, 12999, 12499,  1, MOD_AM,  0, "25M"},
  {BAND_BCAST,13000, 13999, 13500,  1, MOD_AM,  0, "22M"},
  {BAND_HAM,  14000, 14499, 14050,  1, MOD_USB, 0, "20M"},
  {BAND_BCAST,14500, 16999, 15000,  1, MOD_AM,  0, "19M"},
  {BAND_BCAST,17000, 17999, 17500,  1, MOD_AM,  0, "18M"},
  {BAND_HAM,  18000, 18199, 18050,  1, MOD_USB, 0, "17M"},
  {BAND_BCAST,18200, 20999, 18500,  1, MOD_AM,  0, "16M"},
  {BAND_HAM,  21000, 21499, 21050,  1, MOD_USB, 0, "15M"},
  {BAND_BCAST,21500, 24799, 22500,  1, MOD_AM,  0, "13M"},
  {BAND_HAM,  24800, 25000, 24900,  1, MOD_USB, 0, "12M"},
  {BAND_HAM,  25001, 27999, 27125,  1, MOD_AM,  0, "11M"},
  {BAND_HAM,  28000, 30000, 28200,  1, MOD_USB, 0, "10M"},
};
*/

/*
//reduce 23 -> 20 for using FM Radio Frequency EEPROM
Band swband[] = {
  {BAND_HAM,    150,   471,   200,  1, MOD_LSB, 0, "L.F"},
  {BAND_HAM,    472,   479,   475,  1, MOD_LSB, 0, "630M"},
  {BAND_BCAST,  480,  1799,  1000,  1, MOD_AM,  0, "AMMF"},
  {BAND_HAM,   1800,  2000,  1820,  1, MOD_LSB, 0, "160M"},
  {BAND_BCAST, 2001,  3499,  2500,  1, MOD_AM,  0, "120M"},
  {BAND_HAM,   3500,  4000,  3800,  1, MOD_LSB, 0, "80M"},
//  {BAND_BCAST, 4001,  6999,  4700,  1, MOD_AM,  0, "60M"},
  {BAND_BCAST, 4001,  6999,  5500,  1, MOD_AM,  0, "60M"},
//  {BAND_BCAST, 5201,  6999,  5700,  1, MOD_AM,  0, "49M"},
  {BAND_HAM,   7000,  7300,  7070,  1, MOD_LSB, 0, "40M"},
  {BAND_BCAST, 7301,  9999,  7500,  1, MOD_AM,  0, "35M"},
  {BAND_HAM,  10000, 10500, 10299,  1, MOD_LSB, 0, "30M"},
  {BAND_BCAST,10501, 13999, 12499,  1, MOD_AM,  0, "25M"},
//  {BAND_BCAST,13000, 13999, 13500,  1, MOD_AM,  0, "22M"},
  {BAND_HAM,  14000, 14499, 14050,  1, MOD_USB, 0, "20M"},
  {BAND_BCAST,14500, 17999, 15000,  1, MOD_AM,  0, "19M"},
//  {BAND_BCAST,17000, 17999, 17500,  1, MOD_AM,  0, "18M"},
  {BAND_HAM,  18000, 18199, 18050,  1, MOD_USB, 0, "17M"},
  {BAND_BCAST,18200, 20999, 18500,  1, MOD_AM,  0, "16M"},
  {BAND_HAM,  21000, 21499, 21050,  1, MOD_USB, 0, "15M"},
  {BAND_BCAST,21500, 24799, 22500,  1, MOD_AM,  0, "13M"},
  {BAND_HAM,  24800, 25000, 24900,  1, MOD_USB, 0, "12M"},
  {BAND_HAM,  25001, 27999, 27125,  1, MOD_AM,  0, "11M"},
  {BAND_HAM,  28000, 30000, 28200,  1, MOD_USB, 0, "10M"},
};
*/

//reduce 23 -> 20 for using FM Radio Frequency EEPROM, Split Current Freq
Band swband[] = {
  {BAND_HAM,    150,   471,   1, MODE_LSB, 0, "L.F"},
  {BAND_HAM,    472,   479,   1, MODE_LSB, 0, "630M"},
  {BAND_BCAST,  480,  1799,   1, MODE_AM,  0, "AMMF"},
  {BAND_HAM,   1800,  2000,   1, MODE_LSB, 0, "160M"},
  {BAND_BCAST, 2001,  3499,   1, MODE_AM,  0, "120M"},
  {BAND_HAM,   3500,  4000,   1, MODE_LSB, 0, "80M"},
//  {BAND_BCAST, 4001,  6999,  4700,  1, MOD_AM,  0, "60M"},
  {BAND_BCAST, 4001,  6999,   1, MODE_AM,  0, "60M"},
//  {BAND_BCAST, 5201,  6999,  5700,  1, MOD_AM,  0, "49M"},
  {BAND_HAM,   7000,  7300,   1, MODE_LSB, 0, "40M"},
  {BAND_BCAST, 7301,  9999,   1, MODE_AM,  0, "35M"},
  {BAND_HAM,  10000, 10500,   1, MODE_LSB, 0, "30M"},
  {BAND_BCAST,10501, 13999,   1, MODE_AM,  0, "25M"},
//  {BAND_BCAST,13000, 13999, 13500,  1, MOD_AM,  0, "22M"},
  {BAND_HAM,  14000, 14499,   1, MODE_USB, 0, "20M"},
  {BAND_BCAST,14500, 17999,   1, MODE_AM,  0, "19M"},
//  {BAND_BCAST,17000, 17999, 17500,  1, MOD_AM,  0, "18M"},
  {BAND_HAM,  18000, 18199,   1, MODE_USB, 0, "17M"},
  {BAND_BCAST,18200, 20999,   1, MODE_AM,  0, "16M"},
  {BAND_HAM,  21000, 21499,   1, MODE_USB, 0, "15M"},
  {BAND_BCAST,21500, 24799,   1, MODE_AM,  0, "13M"},
  {BAND_HAM,  24800, 25000,   1, MODE_USB, 0, "12M"},
  {BAND_HAM,  25001, 27999,   1, MODE_AM,  0, "11M"},
  {BAND_HAM,  28000, 30000,   1, MODE_USB, 0, "10M"},
};

uint16_t CurrentFreqs[20];

#define SI4735_RADIO_EEPROM 0x0F20
void LoadSaveConfig(bool isLoad);

int8_t CEC_FindBandIndex(uint16_t srcFreq)
{
    for (int i = 0; i < 20; i++)
    {
		if (srcFreq >= swband[i].minimumFreq && srcFreq <= swband[i].maximumFreq)
			return i;
    }

	return -1;
}

void CEC_Radio_DrawRSSI(bool _isDraw)
{
	uint8_t tmpBuff[20];
	SI4735_getCurrentReceivedSignalQuality(0);
	memset(gFrameBuffer[3], 0, 128 * 2);		//2Line
/*
	uint8_t RSSI;    //!<  RESP4 - Contains the current receive signal strength (0–127 dBμV).
	uint8_t SNR;     //!<  RESP5 - Contains the current SNR metric (0–127 dB).
*/
	uint8_t displayRSSI = dBuVToSignalLength15(SI4735_getCurrentRSSI()) ;	//MAX 20

	//tmpBuff[0] = 'S';
	memset(tmpBuff, 0, sizeof(tmpBuff));
	sprintf(tmpBuff, "S%1u%c", displayRSSI < 10 ? displayRSSI : 9, displayRSSI < 10 ? ' ' : '+');
	for (int i = 0; i < 15; i++)
	{
		tmpBuff[i + 3] = displayRSSI > i ? '}' : '{';
	}

	UI_PrintStringSmallNormal(tmpBuff, 0, 0, 3);

	sprintf(tmpBuff, "RSSI %2ddBuV", SI4735_getCurrentRSSI());
	//sprintf(tmpBuff, "RSSI TEST TEST %d", 0);
	CEC_DisplaySmallest(tmpBuff, 1,  33, false, true);

	sprintf(tmpBuff, "SNR %2ddB", SI4735_getCurrentSNR());
	CEC_DisplaySmallest(tmpBuff, 95,  33, false, true);

	if (_isDraw)
	{
    	ST7565_BlitFullScreen();
	}
}

void CEC_Radio_DrawScreen(void)
{
	uint8_t tmpBuff[16];
//    uint16_t tmpFreq = 14074;
    uint16_t freqScaleDiv =  1000;
	uint8_t dispStep =  0;

	uint16_t tmpFreq = SI4735_getFrequency();
	gAskToSave	= false;
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
	else	//LSB, USB, CW
	{
		CurrentAMFreq = tmpFreq;
		dispStep = SSBFreqStep;
	}
	/*
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
	*/



	memset(gStatusLine, 0, sizeof(gStatusLine));
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));	

    uint8_t     *line = gStatusLine;

	//sprintf(tmpBuff, "%u", tmpFreq);
	//sprintf(tmpBuff, NowMode == MODE_FM ? "%3u.%02u" : "%3u.%03u %u %u", tmpFreq / freqScaleDiv, (tmpFreq) % freqScaleDiv, tmpFreq, freqScaleDiv);
	//currentStep
	int nowBandIndex = CEC_FindBandIndex(tmpFreq);
	char * bandName;
	char * bandTypeName;
	if (NowMode == MODE_FM)
	{
		nowBandIndex = -1;
		bandName = "FM";
		bandTypeName = "BROAD";
	}
	//else if (nowBandIndex == -1)
	//{
	//	bandName = "";
	//	bandTypeName = "";
	//}
	else
	{
		bandName = swband[nowBandIndex].bandName;
		bandTypeName = swband[nowBandIndex].BandType == BAND_BCAST ? "BROAD" : "HAM";
	}

	uint8_t tmpLnaStr[10];
	if (!disableAgc)
	{
		sprintf(tmpLnaStr, "AGC ON");
	}
	else
	{
		sprintf(tmpLnaStr, "ATT %2d", currentAGCAtt);
	}

	sprintf(tmpBuff, "%-5s %4uKhz %s %3sK",bandTypeName,  currentStep, tmpLnaStr, bandwitdth[bandwidthIdx] );
    CEC_DisplaySmallest(tmpBuff, 3, 1, true, true);
    CEC_DisplaySmallest("H0.5", 107, 1, true, true);
    CEC_ReverseScreen(gStatusLine, sizeof(gStatusLine));
    ST7565_BlitStatusLine();


	int startBandIndex = nowBandIndex - 3;
	if (nowBandIndex != -1)
	{
		if (startBandIndex < 0)
			startBandIndex = 0;
		else if (startBandIndex > 13)
			startBandIndex = 13;

		for (int i = 0; i < 7; i++)
		{
			CEC_DisplaySmallest(swband[startBandIndex + i].bandName, 2 + i * 18, 1, false, true);
		}


		uint8_t bandIndex = nowBandIndex - startBandIndex;
		uint8_t drawStartX = (18 * bandIndex);
		UI_DrawRectangleBuffer(gFrameBuffer, 0 + drawStartX , 6, 17 + drawStartX, 6, true);

	}

	sprintf(tmpBuff, NowMode == MODE_FM ? "%4u.%02u" : "%3u.%03u", tmpFreq / freqScaleDiv, (tmpFreq) % freqScaleDiv);
    UI_DisplayFrequency(tmpBuff, 20, 1, false);
	UI_PrintStringSmallNormal("Mhz", 105, 0, 2);
	UI_PrintStringSmallNormal(modeName[NowMode], 1, 0, 2);

	uint8_t drawStepUnderLineX = 46;
	/*
	if (dispStep == 0)
		drawStepUnderLineX = 88;
	if (dispStep == 1)
		drawStepUnderLineX = 74;
	else if (dispStep == 2)
		drawStepUnderLineX = 61;
		*/

	switch (dispStep)
	{
		case 0 : 
			drawStepUnderLineX = 88;
			break;
		case 1 : 
			drawStepUnderLineX = 81;
			break;
		case 2 : 
			drawStepUnderLineX = 74;
			break;
		case 3 : 
			drawStepUnderLineX = 67;
			break;
		case 4 : 
			drawStepUnderLineX = 61;
			break;
		case 5 : 
			drawStepUnderLineX = 53;
			break;
	}


	//else
	//	drawStepUnderLineX = 30;

    UI_DrawRectangleBuffer(gFrameBuffer, drawStepUnderLineX , 22, drawStepUnderLineX + 13, 23, true);

	//With 13
	//dispStep

	sprintf(tmpBuff, "%03d", currentBFO);
    CEC_DisplaySmallest(tmpBuff, 105, 9, false, true);

	CEC_Radio_DrawRSSI(0);
	//UI_PrintStringSmallNormal("S5}}}}}}{{{{{{{{{{", 0, 0, 3);
	//CEC_DisplaySmallest("RSSI 35dBuV", 3,  33, false, true);
	//CEC_DisplaySmallest("SNR 35", 95,  33, false, true);


    //UI_DrawRectangleBuffer(gFrameBuffer, 46 , 23, 59, 23, true);

 	//UI_PrintString("Mhz", 100, 0, 1, 7);


/*
	CEC_DisplaySmallest("MODE", 2,  36, false, true);
	CEC_DisplaySmallest("STEP+", 2, 42, false, true);
	CEC_DisplaySmallest("STEP-", 2, 48, false, true);
    UI_DrawRectangleBuffer(gFrameBuffer, 0 , 34, 23, 55, true);
*/
    UI_PrintStringSmallNormal("BND STP LNA BW BFO", 0, 0, 5);
	
    uint8_t * startPT = &gFrameBuffer[5][0];
    startPT += (27 * CurrentFunctinIndex);
    for (int i = 0; i < 24; i++)
    {
        startPT[i] = ~startPT[i];

    }

    memset(gFrameBuffer[6], 0, 128);

	if (CurrentFunctinIndex == 0)
	{
		sprintf(tmpBuff, "BAND:%5s", bandName);
    	UI_PrintStringSmallNormal(tmpBuff, 0, 0, 6);
	}
	else if (CurrentFunctinIndex == 1)
	{
		//STEP
		sprintf(tmpBuff, "STEP:%5dKhz", currentStep);
    	UI_PrintStringSmallNormal(tmpBuff, 0, 0, 6);
	}
	else if (CurrentFunctinIndex == 2)
	{
		//STEP
		if (! disableAgc)
		{
    		UI_PrintStringSmallNormal("AGC ON", 0, 0, 6);
		}
		else
		{
			sprintf(tmpBuff, "ATT:%3d", currentAGCAtt);
    		UI_PrintStringSmallNormal(tmpBuff, 0, 0, 6);
		}
	}
	else if (CurrentFunctinIndex == 3)
	{
		//STEP
		sprintf(tmpBuff, "BW:%3sKhz", bandwitdth[bandwidthIdx]);
    	UI_PrintStringSmallNormal(tmpBuff, 0, 0, 6);
	}
	else if (CurrentFunctinIndex == 4)
	{
		//STEP
		sprintf(tmpBuff, "BFO:%5d", currentBFO);
    	UI_PrintStringSmallNormal(tmpBuff, 0, 0, 6);
	}

    ST7565_BlitFullScreen();

	if (isAutoSave == 1)
	{
		LoadSaveConfig(false);	//SAVE
		//BackLightBlink(2);
		isAutoSave = 0;
	}

}

/*
//STEP
void AppllyStep(uint8_t _nowStep)
{
	currentStep = 1;
	for (int i = 0; i < _nowStep; i++)
		currentStep *= 10;
}
*/

void SetSI4735ChangeStep(uint8_t isIncrease)
{
	uint8_t nowStep = 0;

	if (NowMode == 0)
		nowStep = FMFreqStep;
	else if (NowMode == 1)
		nowStep = AMFreqStep;
	else	//SSB
		nowStep = SSBFreqStep;


	if (isIncrease == 1)	//Increase
	{
		if (++nowStep > 6)
			nowStep = 0;
	}
	else if (isIncrease == 0)	//Decrease
	{
		if (nowStep == 0)
			nowStep = 6;
		else 
			nowStep--;
	}

	//other skip
	uint16_t lstStep[] = {1, 5, 10, 50, 100, 500, 1000};
	//SI4735_setFrequencyStep(nowStep);
	//AppllyStep(nowStep);
	//currentStep = 1;
	//for (int i = 0; i < nowStep; i++)
	//	currentStep *= 10;
	currentStep = lstStep[nowStep];

	uint16_t tmpFreq = SI4735_getFrequency();

	if (NowMode == 0)
	{
		FMFreqStep = nowStep;
		CurrentFMFreq = tmpFreq;
	}
	else if (NowMode == 1)
	{
		AMFreqStep = nowStep;
		CurrentAMFreq = tmpFreq;
	}
	else	//SSB
	{
		SSBFreqStep = nowStep;
		CurrentAMFreq = tmpFreq;
	}


	//if (NowMode)
	/*
	uint16_t tmpFreq = SI4735_getFrequency();
	tmpFreq = (tmpFreq / nowStep) * nowStep;

	setFrequency(tmpFreq);
	*/

}

#define AMBAND_FREQ_LOW  150
#define AMBAND_FREQ_HIGH 30000



void SendMixSignal()
{

	return;
  	gCurrentVfo->pTX->Frequency = 2510000;

	RADIO_PrepareTX();
  	BK4819_PrepareTransmit();

	BK4819_WriteRegister(0x40, 0);
	BK4819_SetupPowerAmplifier(0, 0);
	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);

	const uint8_t gain   = (3u << 3) | (1u << 0);
	const uint8_t enable = 1;

	uint32_t startFreq = gRxVfo->pRX->Frequency;
	BK4819_WriteRegister(BK4819_REG_36, (100 << 8) | (enable << 7) | (gain << 0));

	GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BK1080);
	//while(1);
	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, true);
	AUDIO_AudioPathOn();

	//BK4819_SetFrequency(startFreq);
	//잘된다.
	/*
	BK4819_WriteRegister(BK4819_REG_38, (startFreq >>  0) & 0xFFFF);
	BK4819_WriteRegister(BK4819_REG_39, (startFreq >> 16) & 0xFFFF);

	uint16_t tmp1 = 0xC1FE;
	tmp1 &= ~(1 << 15);	//VCO Calibration Enable  (이것만 해줘도 주파수 변경됨) - 0xC1FE로 다시 전송한경우 주파수 변경됨
	//tmp1 &= ~(0xF << 4);	//드드드 소리큼
	BK4819_WriteRegister(BK4819_REG_30, tmp1);
	BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
	*/
}

void WaitMomentDisplay()
{
	memset(gFrameBuffer[1] + 40, 0, 50);
	memset(gFrameBuffer[2] + 40, 0, 50);
				
	UI_PrintString("Wait", 50, 0, 1, 8);
	CEC_ReverseScreen(gFrameBuffer[1] + 40, 50);
	CEC_ReverseScreen(gFrameBuffer[2] + 40, 50);
	ST7565_BlitFullScreen();
}


void SetSSBConfig()
{
	SI4735_setAvcAmMaxGain(90); // Sets the maximum gain for automatic volume control on AM/SSB mode (from 12 to 90dB)
	if (NowMode < 2)
		return;
	SI4735_setSSBBfo2(-currentBFO + (NowMode == 4 ? (CW_Tone * -10) : 0) );
	SI4735_setTuneFrequencyAntennaCapacitor(1); // Set antenna tuning capacitor for SW. 0 : auto, max for sw is 6143
	SI4735_setAutomaticGainControl(disableAgc, currentAGCAtt);

	//bandwidthIdx
	//SI4735_setSSBAudioBandwidth(NowMode == 4 ? 4 : bandwidthIdx);	//CW Mode : Fixed 0.5Khz Bandwidth
	SI4735_setSSBAudioBandwidth(bandwidthIdx);	//CW Mode : Fixed 0.5Khz Bandwidth

	//TEST AM CHANNEL FILTER
	//SI4735_setBandwidth(4, 1);


	//1. If audio bandwidth selected is about 2 kHz or below, it is recommended to set SBCUTFLT[3:0] to 0 to enable the band pass filter for better high cut performance on the wanted side band. Otherwise, set it to 1.
	if (bandwidthIdx == 0 || bandwidthIdx == 4 || bandwidthIdx == 5)
		SI4735_setSBBSidebandCutoffFilter(0);
	else
		SI4735_setSBBSidebandCutoffFilter(1);
}

void SetSI4735Mode(uint8_t newMode)
{
	//SendMixSignal();

	if (newMode == 0)	//FM
		SI4735_setFM(7000, 10800,  CurrentFMFreq, FMFreqStep);
	else if (newMode == 1)	//AM
		SI4735_setAM(AMBAND_FREQ_LOW, AMBAND_FREQ_HIGH,  CurrentAMFreq, AMFreqStep);
	else if (newMode == 2 || newMode == 3 || newMode == 4)	//LSB, USB, CW
	{
		if (NowMode < 2)
		{
			//2024.04.06
			WaitMomentDisplay();
			//end of 2024.04.06

			//XOSCEN_CRYSTAL
			//setRefClock(32570);
			//setRefClockPrescaler(1, 0);
			SI4735_setup2(0, AM_FUNCTION);
			//SI4735_setup1(0, -1, AM_FUNCTION, SI473X_ANALOG_AUDIO, XOSCEN_RCLK, 0); // XOSCEN_RCLK means: external clock source setup
			//SI4735_setup1(resetPin, 0, defaultFunction, SI473X_ANALOG_AUDIO, XOSCEN_CRYSTAL, 0);

			delay(10);
			SI4735_loadCompressedPatch(ssb_patch_content, size_content, cmd_0x15, cmd_0x15_size, 1);
			delay(10);

		}

		//setRefClock(32570);
		//setRefClockPrescaler(1, 0);
		SI4735_setSSB(AMBAND_FREQ_LOW, AMBAND_FREQ_HIGH, CurrentAMFreq, SSBFreqStep, newMode == 3 ? 2 : 1);	//USB, LSB(CW)

		//if ()
		//currentRadioFreq = SI4735_getFrequency();
	}

/*
SI4735_setAM2
        //SI4735_setAvcAmMaxGain(currentAvcAmMaxGain); // Set AM Automatic Volume Gain (default value is DEFAULT_CURRENT_AVC_AM_MAX_GAIN)
        //SI4735_setVolume(volume);                    // Set to previus configured volume
*/

	if (NowMode != MODE_CW && newMode == MODE_CW)		//OTHER MODE TO CW (Save Bandwidth)
	{
		restoreBandwidthIdx = bandwidthIdx;
		bandwidthIdx = DEFAULT_BANDWIDTH_INDEX_CW;
	}
	else if (NowMode == MODE_CW && newMode != MODE_CW)	//CW TO OTHER MODE (Restore Bandwidth)
	{
		bandwidthIdx = restoreBandwidthIdx;
	}
	//AppllyStep(currentStep);	//Change currnetStep
	NowMode = newMode;
	//SI4735_setVolume(63);
	SI4735_setSSBAutomaticVolumeControl(false);

	SetSSBConfig();
	SetSI4735ChangeStep(2);
	SI4735_setVolume(63);
}

KEY_Code_t GetKeyValue()
{
	KEY_Code_t rst = KEYBOARD_Poll();
	if (rst == KEY_INVALID)
		return KEY_INVALID;

	delay(250);
	return rst;
}


uint8_t GetKeyPressTime(uint8_t maxDelayTime)
{
	uint8_t i = 0;
	for (i = 0; i < maxDelayTime; i++)
	{
		SYSTEM_DelayMs(10);
		if (KEYBOARD_Poll() == KEY_INVALID)
			return i;
	}

	return i;
}


void WaitKeyUp()
{
	while (KEYBOARD_Poll() != KEY_INVALID)
	{
		delay(10);
	}
}

void LoadSaveConfig(bool isLoad)
{
	//STEP
	struct
	{
		uint16_t fmFreq;
		uint16_t amFreq;
		uint8_t  agcInfo;	//0 : agc on/off,  7~1 : AGC Value
		uint8_t  bandwidth;	//0~10
		uint8_t amStep;
		uint8_t ssbStep;
	} __attribute__((packed)) SI4735_ROM1;

	struct
	{
		uint16_t xtalAdjVal;	//0, 1
		uint8_t temp1_1;			//2
		uint8_t temp1_2;			//3
		uint8_t  temp2;			//4
		uint8_t  temp3;			//5
		uint8_t temp4;			//6
		uint8_t temp5;			//7
	} __attribute__((packed)) SI4735_ROM2;

	if (isLoad)
	{
		//Load Stored Frequency 
        EEPROM_ReadBuffer(0x0E40, CurrentFreqs, sizeof(CurrentFreqs));
        for (int i = 0; i < 20; i++)
        {
            if (CurrentFreqs[i] > swband[i].maximumFreq || CurrentFreqs[i] < swband[i].minimumFreq)
                CurrentFreqs[i] = swband[i].minimumFreq + ((swband[i].maximumFreq - swband[i].minimumFreq) / 2);
        }

		//Load Setting values
		EEPROM_ReadBuffer(SI4735_RADIO_EEPROM, &SI4735_ROM1, 8);
		//AMBAND_FREQ_LOW, AMBAND_FREQ_HIGH
		//gEeprom.FM_LowerLimit = 700;
		//gEeprom.FM_UpperLimit = 1080;
		if (SI4735_ROM1.fmFreq < 7000 || SI4735_ROM1.fmFreq > 10800)
			CurrentFMFreq = 9700;
		else
			CurrentFMFreq = SI4735_ROM1.fmFreq;

		if (SI4735_ROM1.amFreq < AMBAND_FREQ_LOW || SI4735_ROM1.amFreq > AMBAND_FREQ_HIGH)
			CurrentAMFreq = 21000;
		else
			CurrentAMFreq = SI4735_ROM1.amFreq;

		bandwidthIdx =   SI4735_ROM1.bandwidth < MAX_BANDWIDTH_COUNT ? SI4735_ROM1.bandwidth : DEFAULT_BANDWIDTH_INDEX;

		disableAgc = SI4735_ROM1.agcInfo & 0x01;
		currentAGCAtt = (SI4735_ROM1.agcInfo >> 1) & 0x7F;
		if (currentAGCAtt > 30)
		{
			currentAGCAtt = 0;
			disableAgc = 1;
		}

		AMFreqStep = SI4735_ROM1.amStep < 7 ? SI4735_ROM1.amStep : 2;		//10Khz
		SSBFreqStep = SI4735_ROM1.ssbStep < 7 ? SI4735_ROM1.ssbStep : 0;	//1Khz
	}
	else
	{
		//AMBAND_FREQ_LOW, AMBAND_FREQ_HIGH
		//gEeprom.FM_LowerLimit = 700;
		//gEeprom.FM_UpperLimit = 1080;
        //Save Freq
        for (int i = 0; i < 5; i++)
            EEPROM_WriteBuffer(0x0E40 + (i * 8), &CurrentFreqs[i * 4]);     

		SI4735_ROM1.fmFreq = CurrentFMFreq;
		SI4735_ROM1.amFreq = CurrentAMFreq;
		SI4735_ROM1.bandwidth = bandwidthIdx;
		SI4735_ROM1.agcInfo = (currentAGCAtt << 1) | (disableAgc & 0x01);
		SI4735_ROM1.amStep = AMFreqStep;
		SI4735_ROM1.ssbStep = SSBFreqStep;

		EEPROM_WriteBuffer(SI4735_RADIO_EEPROM, &SI4735_ROM1);
	}
}

bool isNeed4732Init = true;
void CEC_Radio_Work(void)
{
    //SI4735Test_SSB();
    //DRAW MAIN SCREEN 
	uint16_t refreshCount = 0;
	uint8_t keyPressTime = 0;

	WaitMomentDisplay();

	RADIO_SetupRegisters(true);
	GPIO_ClearBit(&GPIOB->DATA, GPIOB_PIN_BK1080);
	delay(100);	

	if (isNeed4732Init)
	{
		//Generat32Khz();
		LoadSaveConfig(true);
		gMonitor = false;
		//SI4735_setDeviceI2CAddress(0);
		int16_t si4735Addr = 0x11;

		//for External 32Khz X-Tal  (Default)
		//
		//setRefClock(32500);
		//setRefClockPrescaler(1, 0);
		SI4735_setup2(0, FM_FUNCTION);

/*
		//Belore Test Working with Signal Generator amp 1volt, bias 1.5 volt
		//* @param refclk The allowed REFCLK frequency range is between 31130 and 34406 Hz (32768 ±5%), or 0 (to disable AFC). : 32500 = OK
		setRefClock(32500);          // Ref = 32.5kHz

		//@param prescale  Prescaler for Reference Clock value; Between 1 and 4095 in 1 unit steps. Default is 1. : 800  = OK
		//@param rclk_sel  0 = RCLK pin is clock source (default); 1 = DCLK pin is clock source : 0 : Connect clock with RCLK pin  = OK
		setRefClockPrescaler(800, 0);   // prescaler = 800 ==> 32500 x 800 = 26000000 (26MHz)
		//rx.setup(RESET_PIN, 0, POWER_UP_AM, SI473X_ANALOG_AUDIO, XOSCEN_RCLK);
		//NOT USE RESET PIN, INT 0, 

		SI4735_setup1(0, 0, FM_FUNCTION, SI473X_ANALOG_AUDIO, XOSCEN_RCLK, 0);
*/

/*
	//USING PWM
		setRefClock(32120);          // Ref = 32.5kHz
		setRefClockPrescaler(0, 0);   // prescaler = 800 ==> 32500 x 800 = 26000000 (26MHz)
		SI4735_setup1(0, 0, FM_FUNCTION, SI473X_ANALOG_AUDIO, XOSCEN_RCLK, 0);
*/

/*
		//delay(1000);
		delay(100);
		//SI4735_setVolume(0);
		SetSI4735Mode(MODE_FM);
		delay(100);
		SetSI4735Mode(MODE_AM);
		delay(70);
*/		
	}

	if ((! HF_DualWatch) || isNeed4732Init)
	{
		delay(100);
		//SI4735_setVolume(0);
		SetSI4735Mode(MODE_FM);
		delay(300);
		SetSI4735Mode(MODE_AM);
		delay(100);
	}


	//SI4735_setVolume(63);
	//setSSBAudioBandwidth(bandwidthIdx);
	AUDIO_AudioPathOn();
	SI4735_setVolume(63);
	
	isNeed4732Init = true;	//using 2 way
    gAskToSave = true;  //use need redrawScreen
	bool isNeedKeyUpCheck = true;
    while (1)
    {
        if (gAskToSave)
            CEC_Radio_DrawScreen();

		if (isNeed4732Init)
		{
			isNeed4732Init = false;
		}

		if (isNeedKeyUpCheck)
		{
			isNeedKeyUpCheck = false;
			WaitKeyUp();
		}

        //Check Buttons
		KEY_Code_t chkKey = GetKeyValue();
		if (chkKey != KEY_INVALID)
		{
			BACKLIGHT_TurnOn();

			if (chkKey == KEY_MENU)
			{
				if (++CurrentFunctinIndex > 4)
					CurrentFunctinIndex = 0;

				isNeedKeyUpCheck = true;
			}
			else if (chkKey == KEY_0)
			{
				int indexOffset = 0;

				//if want reduce memory, remark below 1line and CEC_ReverseScreen Command
				int nowBandIndex = CEC_FindBandIndex(currentWorkFrequency);
				CurrentFreqs[nowBandIndex] = currentWorkFrequency;
				isNeedKeyUpCheck = 1;
 				while(1)
                {
					if (isNeedKeyUpCheck)
					{
						memset(gFrameBuffer, 0, sizeof(gFrameBuffer));
						for (int i = 0; i < 7; i++)
						{
							int drawOffset = indexOffset +i;
							sprintf(strBuff, "%d %-4s%2u.%03u %s", i + 1, swband[drawOffset].bandName, 
								CurrentFreqs[drawOffset] / 1000, (CurrentFreqs[drawOffset]) % 1000,
								(swband[drawOffset].BandType == BAND_BCAST ? "BROAD" : "HAM"));
							UI_PrintStringSmallNormal(strBuff, 2, 0, i);

							if (nowBandIndex == drawOffset)
							{
								CEC_ReverseScreen(gFrameBuffer[i], 128);
							}
							//ianlee
						}
						ST7565_BlitFullScreen();

						WaitKeyUp();
						isNeedKeyUpCheck = 0;
					}
					//delay(200);
					//WaitKeyUp();
					chkKey = GetKeyValue();
					if (chkKey != KEY_INVALID)
					{
						isNeedKeyUpCheck = true;

						if (chkKey == 0)
						{
							if (indexOffset == 0)
								indexOffset = 7;
							else if (indexOffset == 7)
								indexOffset = 13;
							else if (indexOffset == 13)
								break;
						}
						else if (chkKey < 8)
						{
							//if (NowMode == 0)	//if fm, change AM
							//{
							SetSI4735Mode(swband[indexOffset + chkKey - 1].currentMode);
							//}

							SI4735_setFrequency(CurrentFreqs[indexOffset + chkKey - 1]);  //swband[_newIndex].currentFreq);
							isAutoSave = 1;
							break;
						}
						else if (chkKey == KEY_EXIT)
						{
							//isNeedKeyUpCheck = true;
							break;
						}
					}

                }
			}
			else if (chkKey == KEY_F)
			{
				uint8_t newMode = 0;
				//Check Long Press
				keyPressTime = GetKeyPressTime(130);
				if (keyPressTime > 120)	//Change AM,FM <-> LSB, USB
				{
					if (NowMode < 2)	//FM, AM Mode
						newMode = 3;	//CHANGE TO LSB
					else
						newMode = 1;	//CHAGNE TO AM
				}
				else
				{
					//FM,AM / LSB/USB Toggle
					//0, 1    2   3
					switch (NowMode)
					{
						case 0 :
							newMode = 1;
							break;
						case 1 :
							newMode = 0;
							break;
						case 2 :
							newMode = 3;
							break;
						case 3 :
							newMode = 4;
							break;
						case 4 :
							newMode = 2;
							break;
					}
				}
				SetSI4735Mode(newMode);

				//WaitKeyUp();
				isNeedKeyUpCheck = true;
			}
			else if (chkKey == KEY_UP)
				SI4735_frequencyUp();
			else if (chkKey == KEY_DOWN)
				SI4735_frequencyDown();
			else if (chkKey == KEY_STAR)
			{
				SetSI4735ChangeStep(0);
				//WaitKeyUp();
				isNeedKeyUpCheck = true;
			}
			else if (chkKey == KEY_SIDE1 || chkKey == KEY_SIDE2)
			{
				//UP 1Mhz
				if (NowMode == 0)	//FM MODE SKIP
					return;

				//BAND Change
				if (CurrentFunctinIndex == 0)
				{
					int32_t tmpFreq = SI4735_getFrequency();
					int _bandIndex = CEC_FindBandIndex(tmpFreq);
					int _newIndex = -1;
					if (_bandIndex < 0)
						return;

 					CurrentFreqs[_bandIndex] = tmpFreq;
					if (chkKey == KEY_SIDE2 && _bandIndex > 0)
						_newIndex = _bandIndex - 1;
					else if (chkKey == KEY_SIDE1 && _bandIndex < 19)
						_newIndex = _bandIndex + 1;

					if (_newIndex != -1)
					{
						SI4735_setFrequency(CurrentFreqs[_newIndex]);  //swband[_newIndex].currentFreq);
						isAutoSave = 1;
						//LoadSaveConfig(false);	//SAVE
					}
						//SI4735_setFrequency(swband[_newIndex].currentFreq);
				}
				else if (CurrentFunctinIndex == 1)
				{
					SetSI4735ChangeStep(chkKey == KEY_SIDE1);
				}
				else if (CurrentFunctinIndex == 2)
				{
					//STEP
					if (disableAgc)	//LNA STATUS
					{
						//same changed by any key
						//if (chkKey == KEY_SIDE1)
						{
							if (currentAGCAtt == 0)
								currentAGCAtt = 1;
							else if (currentAGCAtt == 1)
								currentAGCAtt = 5;
							else if (currentAGCAtt == 5)
								currentAGCAtt = 15;
							else if (currentAGCAtt == 15)
								currentAGCAtt = 26;
							else
								disableAgc = 0;	//AGC
						}
						/*
						else
						{
							if (currentAGCAtt == 0)
								disableAgc = 0;	//AGC
							else if (currentAGCAtt == 1)
								currentAGCAtt = 0;
							else if (currentAGCAtt == 5)
								currentAGCAtt = 1;
							else if (currentAGCAtt == 15)
								currentAGCAtt = 5;
							else
								currentAGCAtt = 26;	//AGC

						}
						*/
					}
					else	//AGC
					{
						disableAgc = 1;		//AGC DIABLE
						//currentAGCAtt = (chkKey == KEY_SIDE1 ? 0 : 26);
						currentAGCAtt = 0;
					}
					//SI4735_setAutomaticGainControl(disableAgc, currentAGCAtt);
				}
				else if (CurrentFunctinIndex == 3)
				{
					//STEP
					/*
					int newBWIndex = bandwidthIdx + (chkKey == KEY_SIDE1 ? -1 : 1);
					if (newBWIndex >= 0 && newBWIndex < 6)
						bandwidthIdx = newBWIndex;
					*/
					bandwidthIdx += (chkKey == KEY_SIDE1 ? 1 : -1);
					if (bandwidthIdx == 255)
						bandwidthIdx = 5;
					else if (bandwidthIdx == 6)
					    bandwidthIdx = 0;

				}
				else if (CurrentFunctinIndex == 4)
				{
					//STEP
					currentBFO += (chkKey == KEY_SIDE1 ? 5 : -5);
				}


				isNeedKeyUpCheck = (CurrentFunctinIndex != 4);
				//WaitKeyUp();
			}
			/*
			else if (chkKey == KEY_1)
			{
				bandwidthIdx++;
				if (bandwidthIdx > 5)
					bandwidthIdx = 0;
				SI4735_setSSBAudioBandwidth(bandwidthIdx);

      			if (bandwidthIdx == 0 || bandwidthIdx == 4 || bandwidthIdx == 5)
        			SI4735_setSBBSidebandCutoffFilter(0);
      			else
        			SI4735_setSBBSidebandCutoffFilter(1);

				WaitKeyUp();
			}
			*/
			
			/*
			else if (chkKey == KEY_2)
			{
      			disableAgc = !disableAgc;
				SI4735_setAutomaticGainControl(disableAgc, currentAGCAtt);
				WaitKeyUp();
			}
			else if (chkKey == KEY_5)
			{
				if (currentAGCAtt == 0)
					currentAGCAtt = 1;
				else if (currentAGCAtt == 1)
					currentAGCAtt = 5;
				else if (currentAGCAtt == 5)
					currentAGCAtt = 15;
				else if (currentAGCAtt == 15)
					currentAGCAtt = 26;
				else
					currentAGCAtt = 0;
      			// siwtch on/off ACG; AGC Index = 0. It means Minimum attenuation (max gain)
				disableAgc = 1;
				SI4735_setAutomaticGainControl(disableAgc, currentAGCAtt);
				WaitKeyUp();
			}
			*/
			else if (chkKey == KEY_1 || chkKey == KEY_4)
			{
      			currentBFO += currentBFOStep * (chkKey == KEY_4 ? -1 : 1 );
				//SI4735_setSSBBfo2(currentBFO);
			}
			else if (chkKey == KEY_EXIT)
			{
				keyPressTime = GetKeyPressTime(80);
				SI4735_setVolume(HF_DualWatch ? HF_DualVol : 0);

				if (! HF_DualWatch)
				{
					GPIO_SetBit(&GPIOB->DATA, GPIOB_PIN_BK1080);
					AUDIO_AudioPathOff();
				}
				LoadSaveConfig(false);	//SAVE
				return;
			}
			//2024.04.05  VERSION 0.44
			else if (chkKey <= 9)
			{
				//DIRECT KEY PAD INPUT
				uint32_t _timeOutValue = millis10();
				//28075//
				//19532
				uint8_t arFreq[7] = {"------"};
				uint8_t inputIndex = 0;
				uint8_t isChanged = 1;

				//uint8_t NowMode = 0;	//0:FM, 1:AM, 2:LSB, 3:USB, 4: CW (USB - 1KHz)
				//arFreq[7]
				if (NowMode == 0)
					arFreq[3] = '.';
				else
					arFreq[2] = '.';

				while(millis10() < (_timeOutValue + 1000))
				{
					//sprintf(tmpBuff, NowMode == MODE_FM ? "%4u.%02u" : "%3u.%03u", tmpFreq / freqScaleDiv, (tmpFreq) % freqScaleDiv);
					//UI_DisplayFrequency(tmpBuff, 20, 1, false);
					if (isChanged)
					{
						memset(gFrameBuffer[1] + 33, 0, 70);
						memset(gFrameBuffer[2] + 33, 0, 70);

						UI_DisplayFrequency(arFreq, 33, 1, false);
						ST7565_BlitFullScreen();
						//delay(300);
						WaitKeyUp();
						isChanged = 0;
					}

					KEY_Code_t tmpKey = KEYBOARD_Poll();
					if (tmpKey != KEY_INVALID)
					{
						if (tmpKey <= KEY_9)
						{
							if (arFreq[inputIndex] == '.')
								inputIndex++;

							arFreq[inputIndex++] = tmpKey + '0';
						}

						/*
						else if (tmpKey == KEY_STAR && inputIndex < 2)
						{
							if (inputIndex == 0)
								arFreq[1] = '0';
							else if (inputIndex == 1)
								arFreq[1] = arFreq[0];

							arFreq[0] = '0';

							inputIndex = 3;
						}
						*/
						else if (tmpKey == KEY_EXIT)
						{
							/*
							if (inputIndex == 0)
							{
								isNeedKeyUpCheck = 1;
								break;
							}
							else
							{
								arFreq[--inputIndex] = '-';
							}
							*/
							isNeedKeyUpCheck = 1;
							break;
						}

						_timeOutValue = millis10();
						isChanged = 1;

						if (inputIndex == 6)	//Complete Input
						{
							//7000, 10800
							//uint8_t arFreq[7] = {"--.---"};
							//memcpy(arFreq +2, arFreq + 3, 3);
							//arFreq[5] = 0;
							//64036
							uint16_t textFreq = (arFreq[0] - '0') * 10000 + (arFreq[1] - '0') * 1000 + (arFreq[NowMode == 0 ? 2 : 3] - '0') * 100 + (arFreq[4] - '0') * 10 + (arFreq[5] - '0');
							//uint16_t textFreq = (arFreq[0] - '0') * 10000 + (arFreq[1] - '0') * 1000;

							//64056
							/*
							if (NowMode == 0)
								textFreq += (arFreq[2] - '0') * 100 + (arFreq[4] - '0') * 10 + (arFreq[5] - '0');
							else
								textFreq += (arFreq[3] - '0') * 100 + (arFreq[4] - '0') * 10 + (arFreq[5] - '0');
								*/

							SI4735_setFrequency(textFreq);  //swband[_newIndex].currentFreq);
							gAskToSave = 1;
							//isAutoSave = 1;
							isNeedKeyUpCheck = 1;

							break;
						}

					}
					delay(20);
				}
			}

			//END OF 2024.04.04 VERSION 0.44



			SetSSBConfig();
			refreshCount = 0;
			gAskToSave = true;	//Redraw
		}	//end of key invalid check

		if (++refreshCount > 3000)
		{
			//BACKLIGHT_TurnOff();
			refreshCount = 0;
			gAskToSave = true;	//Redraw
		}

		/*
		if ((refreshCount % 100) == 0)
		{
			CEC_Radio_DrawRSSI(1);
		}
		*/
		if ((refreshCount % 50) == 0)
		{
			if (gBacklightCountdown_500ms > 0 && --gBacklightCountdown_500ms == 0)
			{
				BACKLIGHT_TurnOff();
			}
		}
		delay(10);
    }
}