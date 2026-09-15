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
#include "ceccommon.h"


uint8_t CW_Mode       = 0;      //0 : None, 1 : CWN, 2:CW-FM (MCW OR F2A), 3:CW (A1A), 4:MCW A2A
//uint8_t CW_Tone       = 70;     //[EEPROM] *10 Hz, because BK4819 inc/dec 10Hz, Move to ceccommon.c
uint8_t CW_WPM        = 17;     //[EEPROM] 5~ 50 
uint8_t CW_TXDelay    = 20;     //[EEPROM] * 100 milisecond
uint8_t CW_KeyType    = CW_KEYTYPE_PADDLE;  //[EEPROM]

uint8_t CW_SPEED      = 10;     // MILISECOND
uint8_t CW_SideTone   = 1;    //[EEPROM but Not yet decided, will not use it, will use it as a volume]1: Enabled,  0: Disabled




struct ST_CW_ADC CW_ADC;

		// radio 1 .. 04 00 46 00 50 00 2C 0E
		// radio 2 .. 05 00 46 00 50 00 2C 0E
	//	EEPROM_ReadBuffer(0x1F88, &Misc, 8);


/*
uint16_t CWKKEY_DIT_AdcFrom  = 3390;  //[EEPROM]
uint16_t CWKKEY_DAH_AdcFrom  = 3580;  //[EEPROM]
uint16_t CWKKEY_BOTH_AdcFrom = 3680;  //[EEPROM]
uint16_t CWKKEY_BOTH_AdcTo   = 3750;  //[EEPROM]
*/
//)
//uint16_t CWKKEY_DAH_AdcTo = 3650;
//uint16_t CWKKEY_DIT_AdcTo = 3535;

uint16_t CW_DECODE_INTERVAL;	//10mili second
uint16_t SPACE_INTERVAL;	//10mili second


void delay_background(int delayTime, int fromType)
{
  delay(delayTime);
}

char DecodeMorseData(uint8_t *srcBuff, int dataLength, int * _decodeType)
{
  //Find with A~Z
  *_decodeType = -1;
  uint8_t morseBinData = 0;
  for (int i = 0; i < dataLength; i++)
  {
    morseBinData = morseBinData << 1;
    morseBinData |= srcBuff[i];
  }

  uint8_t shiftLengthData = dataLength << 4;
  if (dataLength <= 4)  //Alphabet
  {
    //0b00100100  4Bit(Length), 4Bit(Data)
    morseBinData = morseBinData << (4 - dataLength);
    morseBinData |= (dataLength << 4);

    for (int i = 0; i < 27; i++)
    {
      if (cwAZTable[i] == morseBinData)
      {
        *_decodeType = 1;
        return 'A' + i;
      }
    }

    return -1;
  }

  else if (dataLength == 5)  //Number check , (Length 5 is Number and some symbols)
  {
    for (unsigned int i = 0; i < 10; i++)
      if (cw09Table[i] == morseBinData)
      {
        *_decodeType = 2;
        return '0' + i;
      }

    //== Check Symbol Length 5
    morseBinData = morseBinData << 1; //SHIFT 1
    morseBinData |= 0b10000000;

    for (unsigned int i = 0; i < sizeof(cwSymbolTable); i++)
      if (cwSymbolTable[i] == morseBinData)
      {
        *_decodeType = 3;
        return cwSymbolIndex[i];
      }
  }
  else if (dataLength == 6) //symbole
  {
    morseBinData |= 0b11000000;
    for (unsigned int i = 0; i < sizeof(cwSymbolTable); i++)
      if (cwSymbolTable[i] == morseBinData)
      {
        *_decodeType = 3;
        return cwSymbolIndex[i];
      }
  }
  else if (dataLength == 7 & morseBinData == 0b00001001)  //$
  {
        *_decodeType = 3;
    return '$';
  }
  else
  {
        //*_decodeType = 4;
    //SPECIAL 
    //// ":(Start"),   ':(End "), >: My callsign, <:QSO Callsign (Second Callsign), #:AR, ~:BT, [:AS, ]:SK
  }

  return 0;
}


//Send 1 char
void sendCWChar(char cwKeyChar)
{
  byte sendBuff[7];
  byte i, j, charLength;
  byte tmpChar;

  //For Macrofunction
  //replace > and  < to My callsign, qso callsign, use recursive function call
  /*
  if (cwKeyChar == '>' || cwKeyChar == '<')
  {
    uint16_t callsignStartIndex = 0;
    uint16_t callsignEndIndex = 0;
    
    if (cwKeyChar == '>') //replace my callsign
    {
      if (userCallsignLength > 0)
      {
        callsignStartIndex = 0;
        callsignEndIndex = userCallsignLength;
      }
    }
    else if (cwKeyChar == '<')  //replace qso callsign
    {
      //ReadLength
      callsignEndIndex = EEPROM.read(CW_STATION_LEN);
      if (callsignEndIndex > 0)
      {
        callsignStartIndex = CW_STATION_LEN - callsignEndIndex - USER_CALLSIGN_DAT;
        callsignEndIndex = callsignStartIndex + callsignEndIndex;
      }
    }

    if (callsignStartIndex == 0 && callsignEndIndex == 0)
      return;

    for (uint16_t i = callsignStartIndex; i <= callsignEndIndex; i++)
    {
      sendCWChar(EEPROM.read(USER_CALLSIGN_DAT + i));
      autoSendPTTCheck(); //for reserve and cancel next CW Text
      if (changeReserveStatus == 1)
      {
        changeReserveStatus = 0;
        updateDisplay();
      }
      
      if (i < callsignEndIndex) delay_background(CW_SPEED * 3, 4); //
    }
    
    return;
  }
  else */ 
  
    //Lower case to Uppercase
  if(cwKeyChar >= 'a' && cwKeyChar <= 'z') 
    cwKeyChar -=32;
  //else if (cwKeyChar == '_')
  //  return;
  
   if (cwKeyChar >= 'A' && cwKeyChar <= 'Z')  //Encode Char by KD8CEC
  {
    tmpChar = (cwAZTable[(cwKeyChar - 'A')]);
    charLength = (tmpChar >> 4) & 0x0F;
    for (i = 0; i < charLength; i++)
      sendBuff[i] = (tmpChar << i) & 0x08;
  }
  else if (cwKeyChar >= '0' && cwKeyChar <= '9')
  {
    charLength = 5;
    for (i = 0; i < charLength; i++)
      sendBuff[i] = (   (cw09Table[(cwKeyChar - '0')])    << i) & 0x10;
  }
  else if (cwKeyChar == ' ')
  {
    charLength = 0;
    delay_background(CW_SPEED * 4, 4); //7 -> basic interval is 3
  }
  else if (cwKeyChar == '$')  //7 digit
  {
    charLength = 7;
    for (i = 0; i < 7; i++)
      sendBuff[i] = (0b00010010 << i) & 0x80; //...1..1
  }
/*else if (cwKeyChar == '*')  //8 digit delete  //Not use
  {
    charLength = 7;
    for (i = 0; i < 8; i++)
      sendBuff[i] = (0b00000000 << i) & 0x80; //...1..1
  }
*/  
  else
  {
    //symbol
    for (i = 0; i < 22; i++)
    {
      //  cwSymbolIndex
      if (cwSymbolIndex[i] == cwKeyChar)
      {
        tmpChar = (cwSymbolTable[i]);
        charLength = ((tmpChar >> 6) & 0x03) + 3;
        
        for (j = 0; j < charLength; j++)
          sendBuff[j] = (tmpChar << (j + 2)) & 0x80;

        break;
      }
      else
      {
        charLength = 0;
      }
    }
  }

  for (i = 0; i < charLength; i++)
  {
    CWKeyDown();
    if (sendBuff[i] == 0)
      delay_background(CW_SPEED, 4);
    else
      delay_background(CW_SPEED * 3, 4);
    CWKeyUp();
    if (i != charLength -1)
      delay_background(CW_SPEED, 4);
  }
}


void SendMorseString(const char* sendBuff, int startIndex, int _Length)
{
  if (_Length == -1)
    _Length = strlen(sendBuff);
  for (int i = startIndex; i < startIndex + _Length; i++)
  {
    sendCWChar(sendBuff[i]);
    delay_background(CW_SPEED * 3, 4);
  }

}

//For Decode CW
uint32_t lastPressKeyTime = 0;
uint8_t arPressKey[10] = {0};	//
uint8_t pressKeyIndex = 0;
uint8_t spaceCheck = 0;

uint8_t TXStartByKey = 1; //0:Disabled , 1:Enabled


uint16_t GetCWADC(void)
{
	//uint16_t readBuff[5] = {0};
	uint16_t maxReadValue = 0;
	uint16_t nowReadValue = 0;
	int i;
	ADC_SoftReset();
	ADC_Start();
	while (!ADC_CheckEndOfConversion(ADC_CH3)) {}
	for (i = 0; i < 5; i++)
	{
		nowReadValue = ADC_GetValue(ADC_CH3);;
		if (nowReadValue > maxReadValue)
			maxReadValue = nowReadValue;
		//SYSTEM_DelayMs(1);
    SYSTICK_DelayUs(500); //3 * 5= 2.5ms
	}

	return maxReadValue;
}

#define KEYBOARD_POLLING_MIN_INTERVAL 1 //ms
uint8_t GetCWKeyStatus()
{
  if (CW_KeyType == CW_KEYTYPE_KEYPAD_PDL || CW_KeyType == CW_KEYTYPE_KEYPAD_ST)
  {
    uint8_t isMenuPress = KEYBOARD_Poll() == KEY_MENU;
    uint8_t isPttPress = !GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT);
    SYSTEM_DelayMs(KEYBOARD_POLLING_MIN_INTERVAL);

      if (isMenuPress && isPttPress)
        return CWKEY_STATUS_BOTH;
      else if (isMenuPress)
        return CWKEY_STATUS_DAH;
      else if (isPttPress)
        return CWKEY_STATUS_DIT;
      else
        return CWKEY_STATUS_IDLE;
  }
  else
  {
      uint16_t readVal = GetCWADC();
    /*
    uint16_t CWKKEY_DIT_AdcFrom = 3390;
    uint16_t CWKKEY_DIT_AdcTo = 3535;
    uint16_t CWKKEY_DAH_AdcFrom = 3610;
    uint16_t CWKKEY_DAH_AdcTo = 3650;
    uint16_t CWKKEY_BOTH_AdcFrom = 3700;
    uint16_t CWKKEY_BOTH_AdcTo = 3750;
    */
      if (CW_ADC.CWKKEY_DIT_AdcFrom < readVal && readVal < CW_ADC.CWKKEY_DAH_AdcFrom)
        return CWKEY_STATUS_DIT;
      //else if (3580 < readVal && readVal < 3680)
      else if (CW_ADC.CWKKEY_DAH_AdcFrom < readVal && readVal < CW_ADC.CWKKEY_BOTH_AdcFrom)
        return CWKEY_STATUS_DAH;
      //else if (3680 < readVal && readVal < CWKKEY_BOTH_AdcTo)
      else if (CW_ADC.CWKKEY_BOTH_AdcFrom < readVal && readVal < CW_ADC.CWKKEY_BOTH_AdcTo) //actually just need CWKKEY_BOTH_AdcFrom, but using 2 adc value for safty
        return CWKEY_STATUS_BOTH;
      else
        return CWKEY_STATUS_IDLE;
  }
}


void CWKeyDown(void)
{
  if (CW_Mode == CWMODE_CW)
  {
			BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
			BK4819_SetupPowerAmplifier(gCurrentVfo->TXP_CalculatedSetting, gCurrentVfo->pTX->Frequency);
			BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, true);
  }
  BK4819_ExitTxMute();  
}

void CWKeyUp(void)
{
  if (CW_Mode == CWMODE_CW)
  {
			BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
			BK4819_SetupPowerAmplifier(0, 0);
			BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
  }
	BK4819_EnterTxMute();
  //isSendSignal = 0;
}

void InitRX1Mode(void)
{
  if (CW_KeyType == CW_KEYTYPE_PADDLE || CW_KeyType == CW_KEYTYPE_STRAIGHT )
    SetRX1Mode(2);   //0: GPIO, 1:UART, 2:ADC, 3 : UART 9600 FOR GPS
  else  //DEFAULT
    SetRX1Mode(1);   //0: GPIO, 1:UART, 2:ADC, 3 : UART 9600 FOR GPS

  /*
  if (CW_KeyType == CW_KEYTYPE_PADDLE || CW_KeyType == CW_KEYTYPE_KEYPAD_ST )
    SetRX1Mode(2);   //0: GPIO, 1:UART, 2:ADC, 3 : UART 9600 FOR GPS
  else  //DEFAULT
    SetRX1Mode(1);   //0: GPIO, 1:UART, 2:ADC, 3 : UART 9600 FOR GPS
  */
}

//uint8_t _cwKeyType, uint8_t _cwWPM, uint16_t _cwTone, uint8_t _cwSideTone, uint16_t _keyDahFrom, uint16_t _keyDahTo,  uint16_t _keyDitFrom, uint16_t _keyDitTo,  uint16_t _keyBothFrom, uint16_t _keyBothTo
void InitCWMode(uint8_t _cwMode)
{
  //if same value is return;
  if (CW_Mode == _cwMode)
    return;

  //Set CW Mode
  CW_Mode            = _cwMode;

  memset(arPressKey, 5, sizeof(arPressKey));
/*
#define CW_KEYTYPE_KEYPAD_PDL 0   //COMBINATION PTT AND MENU KEY
#define CW_KEYTYPE_KEYPAD_ST  1   //KEYPAD STRAIGHT PTT OR MENU KEY BUT TERRIBLE PERFORMANCE JUST TOY I CONCIDER FOR REMOVE THIS MENU
#define CW_KEYTYPE_PADDLE     2   //IAMBIC.A WITH 2 REGISTER
#define CW_KEYTYPE_STRAIGHT   3   //EXTERAL STRAIHGT KEY WITH 1 REGISTER
#define CW_KEYTYPE_PC         4   //FOR USING PC PROGRAM AS FLDIGI
*/

   InitRX1Mode();
/*
  //if (CWKeyType == CW_KEYTYPE_STRAIGHT || CWKeyType == CW_KEYTYPE_PADDLE)
  if (CW_Mode == CWMODE_NONE || CW_KeyType == CW_KEYTYPE_PC || CW_KeyType == CW_KEYTYPE_KEYPAD_PDL || CW_KeyType == CW_KEYTYPE_KEYPAD_ST )  //IF WINKEY -> INTERFACE WITH PC
    SetRX1Mode(1);     //0: GPIO, 1:UART, (DEFAULT), 2:ADC, 3 : UART 9600 FOR GPS
  else
      SetRX1Mode(2);   //0: GPIO, 1:UART, 2:ADC, 3 : UART 9600 FOR GPS
*/      
  //else
  //  SetRX1Mode(3);     //0: GPIO, 1:UART, 2:ADC, 3 : UART 9600 FOR GPS

  if (CW_Mode == CWMODE_CWN)
  {

  }

}


extern bool gRxIdleMode;

//void (*CWDecodedChar)(char _decodedChar, int decodeType, int option1, int option2);

__inline uint16_t scale_freq(const uint16_t freq)
{
//	return (((uint32_t)freq * 1032444u) + 50000u) / 100000u;   // with rounding
	return (((uint32_t)freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}

#define MEMORY_ITEM_COUNT  5
#define MEMORY_LENGTH     35
char CWMEM[MEMORY_ITEM_COUNT][MEMORY_LENGTH] = {0};

/*
void StoreCWMemory(int channelIndex, char * _srcBuff, uint8_t srcLen)
{
//  if (channelIndex >= MEMORY_ITEM_COUNT)
//    return;

//  if (srcLen > MEMORY_LENGTH -2)
//  {
//    srcLen = MEMORY_LENGTH -2;
//  }
  //int startIndex = channelIndex == 0 ? 

  for (int i = 0; i < srcLen; i++)
    CWMEM[channelIndex][i] = _srcBuff[i];

  CWMEM[channelIndex][MEMORY_LENGTH -1] = srcLen;
  CWMEM[channelIndex][MEMORY_LENGTH -2] = 0x57; //  MAGIC CODE
}
*/

//cwMessageIndex : 0~ 9 and 100 (CW QSO DATA)
void PlayCWMemory(int cwMessageIndex)
{
  /*
  if (channelIndex >= MEMORY_ITEM_COUNT)
    return;

  int _Length = CWMEM[channelIndex][MEMORY_LENGTH-1];
*/
/*
Before Modifiy
arm-none-eabi-size firmware
   text    data     bss     dec     hex filename
  58528      16    2720   61264    ef50 firmware

After Modified
arm-none-eabi-size firmware
   text    data     bss     dec     hex filename
  58480      16    2720   61216    ef20 firmware  
*/

  uint8_t cwBuff1[32];
  uint8_t cwBuff2[10];

  uint16_t startIndex = 0;  //share mainData and subData
  uint8_t playLength = 0;
  uint8_t readedChar = 0;
  int8_t subLength = 0; 

  if (cwMessageIndex == 100)  //tempdata Call
  {
    playLength = 31;
    //Read CW Buffer
    EEPROM_ReadBuffer(CEC_EEPROM_CWQSODATA, cwBuff1, playLength);
  }
  else if (cwMessageIndex >= RIGINFO_MSG_MYCALL)
  {
    SETTINGS_FetchChannelName(cwBuff1, cwMessageIndex);
    playLength = strlen(cwBuff1);
  }
  //else
  //{
    //startIndex = EEPROM_CHANNELNAME + ((RIGINFO_MSG_CWMSG0 + cwMessageIndex) * 16);
    //SETTINGS_FetchChannelName(cwBuff1, RIGINFO_MSG_CWMSG0 + cwMessageIndex);
    //playLength = 10;
  //}

  
  for (int i = 0; i < playLength; i++)
  {
    if (cwBuff1[i] == '<' || cwBuff1[i] == '@'|| cwBuff1[i] == '#')  //'<' : DX Call, '@' : MY CALL, '#' : MY NAME
    {
      /*
      if (cwBuff1[i] == '<')
        startIndex = EEPROM_CHANNELNAME + (RIGINFO_MSG_DXCALL * 16);
      else if (cwBuff1[i] == '@')
        startIndex = EEPROM_CHANNELNAME + (RIGINFO_MSG_MYCALL * 16);
      else
        startIndex = EEPROM_CHANNELNAME + (RIGINFO_MSG_MYNAME * 16);
      */
      uint8_t subChannelIndex = 0;
      if (cwBuff1[i] == '<')
        subChannelIndex = RIGINFO_MSG_DXCALL;
      else if (cwBuff1[i] == '@')
        subChannelIndex = RIGINFO_MSG_MYCALL;
      else
        subChannelIndex = RIGINFO_MSG_MYNAME;

      SETTINGS_FetchChannelName(cwBuff2, subChannelIndex);

      for (uint16_t j = 0; j < sizeof(cwBuff2); j++)
      {
        //if (cwBuff2[j] == 0x00)
        //  break;
        sendCWChar(cwBuff2[j]);
        delay_background(CW_SPEED * 3, 4);
      }
    }
    else
    {
      sendCWChar(cwBuff1[i]);
      delay_background(CW_SPEED * 3, 4);
    }
  }
}

uint8_t autoSpaceCheck = 1;
uint8_t isHoldMode = 0;
//isDecoding = 0 : Disable
//             1 : User Function
//isJustStart =0 : Normal Mode
//             1 : Start end Exit
//             3 : Normal Mode + isHoldMode is 1 
//void CWTXStart(uint8_t isDecoding, void (*_cWDecodedChar)(char , int, int, int), uint8_t justStart)
void CWTXStart(uint8_t isDecoding, uint8_t justStart)
{
  //TX Start at Main Screen
  if (gScreenToDisplay != DISPLAY_MAIN )
    return;

  //Check Invalid Key Setup 
  if (millis10() < 400)
  {
    for (int i = 0; i < 10; i++)
      AUDIO_PlayBeep(BEEP_880HZ_60MS_TRIPLE_BEEP);
    gTxVfo->Modulation = MODULATION_FM;
    RADIO_SetupRegisters(true);    
    return;
  }

  uint8_t cwTXScreenMode = 0; //0: Current TX Mode, 1 : Quick Select Mode

  isHoldMode = 0;
  if (justStart == 3)
  {
    justStart = 0;
    isHoldMode = 1;
    //WAIT FOR PTT RELEASE
    while (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))
      SYSTEM_DelayMs(100);

    CWDecodedChar(' ', 200, autoSpaceCheck, cwTXScreenMode); //refresh      
  }

  //INIT VALUES
  //Calculate WPM to SPEED (milisecond)
  CW_SPEED           = 1200 / CW_WPM;
  CW_DECODE_INTERVAL = (CW_SPEED * 3) / 10;	//10mili second
  SPACE_INTERVAL     = (CW_SPEED * 7) / 10;	//10mili second


  //INIT
  if (CW_Mode == CWMODE_CWN)
  {
	  AUDIO_AudioPathOff();

	  if (gCurrentFunction == FUNCTION_POWER_SAVE && gRxIdleMode)
		  BK4819_RX_TurnOn();

	  //BK4819_PlayTone(CWTone, true);
    uint16_t ToneConfig = BK4819_REG_70_ENABLE_TONE1;

    BK4819_EnterTxMute();
    BK4819_SetAF(BK4819_AF_BEEP);
/*
    if (bTuningGainSwitch == 0)
      ToneConfig |=  96u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN;
    else
      ToneConfig |= 28u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN;
*/      
    ToneConfig |= 120u << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN;
    BK4819_WriteRegister(BK4819_REG_70, ToneConfig);

    BK4819_WriteRegister(BK4819_REG_30, 0);
    BK4819_WriteRegister(BK4819_REG_30, BK4819_REG_30_ENABLE_AF_DAC | BK4819_REG_30_ENABLE_DISC_MODE | BK4819_REG_30_ENABLE_TX_DSP);

    BK4819_WriteRegister(BK4819_REG_71, scale_freq(CW_Tone * 10));
	  AUDIO_AudioPathOn();
  }
  else if (CW_Mode == CWMODE_CWFM)
  {
 	  RADIO_PrepareTX();
	  BK4819_EnterTxMute();
	
	  if (CW_SideTone)
	  {
		  AUDIO_AudioPathOn();
		  BK4819_SetAF(BK4819_AF_BEEP);
	  }
	  else
		  BK4819_SetAF(BK4819_AF_MUTE);

    #define FM_MODE_GAIN_LEVEL 250
    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((FM_MODE_GAIN_LEVEL & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));

    BK4819_EnableTXLink();
    //SYSTEM_DelayMs(50);
    BK4819_WriteRegister(BK4819_REG_71, CW_SCALE_TONE(CW_Tone * 10));
  }
  else if (CW_Mode == CWMODE_CW)
  {
 	  RADIO_PrepareTX();
    BK4819_EnterTxMute();

	  #define BK4819_REG_40 0x40U
	  #define BK4819_REG_40_SHIFT_ENABLE_DEVIATION 12
	  #define BK4819_REG_40_SHIFT_TX_DEVIATION 0

    /*
	  uint16_t regTemp1 = BK4819_ReadRegister(BK4819_REG_40);
	  WriteLogInt("REG1:", regTemp1, 0, 0);
	  regTemp1 &= ~(0x01U << BK4819_REG_40_SHIFT_ENABLE_DEVIATION);
	  WriteLogInt("REG2:", regTemp1, 0, 0);
    BK4819_WriteRegister(0x40, regTemp1);
    */
	  BK4819_WriteRegister(0x40, 0);
			
	  if (CW_SideTone)
	  {
		  AUDIO_AudioPathOn();
		  BK4819_SetAF(BK4819_AF_BEEP);
	  }
	  else
		  BK4819_SetAF(BK4819_AF_MUTE);

    #define FM_MODE_GAIN_LEVEL 250
    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((FM_MODE_GAIN_LEVEL & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_EnableTXLink();
    

	  BK4819_ToggleGpioOut(BK4819_GPIO5_PIN1_RED, false);
	  BK4819_SetupPowerAmplifier(0, 0);
	  BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, false);
	  BK4819_EnterTxMute();

    BK4819_WriteRegister(BK4819_REG_71, CW_SCALE_TONE(CW_Tone * 10));
  }

  if (justStart == 1)
    return;

  CWDecodedChar(' ', 200, autoSpaceCheck, cwTXScreenMode); //refresh

  //Option : Paddle Start 
  int _chkTimeOut = 0;
  while (GetCWKeyStatus() != CWKEY_STATUS_IDLE &&  ++_chkTimeOut < 1000)
    SYSTEM_DelayMs(1);


	lastPressKeyTime = millis10();

  uint8_t isRunning = 1;
  //LOOP (txDelayTime)
  uint8_t keyStatus = 0;
  uint32_t lastKeyPressTime = 0;
	while (isRunning)
	{
    //Check Keyboard (key. function) 
    if (millis10() - lastKeyPressTime > 100)
    {
      KEY_Code_t Key = KEYBOARD_Poll();
      SYSTEM_DelayMs(KEYBOARD_POLLING_MIN_INTERVAL);  //BY IANLEE SET MINIMUM INTERVAL TIME
      if (Key != KEY_INVALID && Key != KEY_MENU)
      {
        lastKeyPressTime = millis10();
        /*
        while(KEYBOARD_Poll() != Key)
        {
          SYSTEM_DelayMs(50);
        }
        */
        SYSTEM_DelayMs(500);

        //Change TX Screen Mode
        if (Key == KEY_STAR)
        {
          cwTXScreenMode = ! cwTXScreenMode;
          //Refresh
           CWDecodedChar(' ', cwTXScreenMode ? 101 : 200, autoSpaceCheck, cwTXScreenMode); //refresh
        }
        else if (cwTXScreenMode == 1 && (Key <= KEY_9))  //Memory Quick Send Mode
        {
          //CW MESSAGE 0 ~ 9 ONE CLICK TO SEND
            //PlayCWMemory(Key);
            PlayCWMemory(RIGINFO_MSG_CWMSG0 + Key);
        }
        else
        {
            //SYSTEM_DelayMs(500);
          if (Key == KEY_1)   //Quick Save DX.Call for CW Message 0~ 10
          {
            CWDecodedChar(' ', 50, autoSpaceCheck, cwTXScreenMode);
          }
          else if (Key == KEY_2)  //Clear Screen
          {
            CWDecodedChar(' ', 100, autoSpaceCheck, cwTXScreenMode);  
            //SendMorseString("KD8CEC", 0, -1);
          }
          else if (Key == KEY_3)  //Save QSO Data Maximum 31 Byte
          {
            //Store Memory
            //SendMorseString("HELLO WORLD THIS TEXT IS MEMORY 1", 0, -1);
            CWDecodedChar(' ', 51, autoSpaceCheck, cwTXScreenMode);  //Select Number

          //Change at Version 0.1P  (5 CW Memory => Quick Save DX Call (10byte) and One QSO Data (31 Byte) Save)
          /*
            while (1)
            {
              KEY_Code_t key1 = KEYBOARD_Poll();
              while (KEYBOARD_Poll() == key1);
              if (key1 == KEY_3)
              {
                //SYSTEM_DelayMs(500);
                break;
              }
              else if (key1 == KEY_INVALID)
              {
                //SYSTEM_DelayMs(100);
                continue;
              }
              else if (key1 == KEY_1) //HOT KEY : STORE DX CALL
              {

              }
              
              if (key1 >= KEY_6 && key1 <= KEY_9 || key1 == KEY_0)
              {
                //Key 0(0), 1(6), 2(7), 3(8), 4(9)
                int newIndex = 0;
                if (key1 >= KEY_6)
                  newIndex = key1 - KEY_5; 
                CWDecodedChar(' ', 50 + newIndex, autoSpaceCheck, 0);  //Select Number
                //SYSTEM_DelayMs(1000);
                break;
              }
            }
            */

          }
          else if (Key == KEY_4)
          {
            isHoldMode = ! isHoldMode;
          }
          else if (Key == KEY_5)
          {
            if (autoSpaceCheck)
            {
              autoSpaceCheck = 0;  
              CWDecodedChar(' ', 200, autoSpaceCheck, cwTXScreenMode); //refresh
            }
            else
            {
              //LONG CHANGE MODE
              //SHORT SPACE INPUT
              //while (KEYBOARD_Poll() == KEY_5)
              SYSTEM_DelayMs(500);

              if (millis10() - lastKeyPressTime > 150)
              {
                //LONG
                autoSpaceCheck = 1;
                CWDecodedChar(' ', 200, autoSpaceCheck, cwTXScreenMode); //refresh
              }
              else
              {
                CWDecodedChar(' ', 0, autoSpaceCheck, cwTXScreenMode);
              }

            }
          }
          else if (Key == KEY_6)  //
          {
            //PLAY CURRENT QSO DATA
            PlayCWMemory(100);
          }
          else if (Key == KEY_UP)  //
          {
            //PLAY CURRENT QSO DATA
            PlayCWMemory(RIGINFO_MSG_DXCALL);
          }
          else if (Key == KEY_DOWN)  //
          {
            //PLAY CURRENT QSO DATA
            PlayCWMemory(RIGINFO_MSG_MYCALL);
          }

          else if (isHoldMode && Key == KEY_EXIT)
          {
            break;
          }          
        }
      }

      /*  //Using KEY PAD
      else
      {
        if (isHoldMode && (! GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT)))
        {
          while (!GPIO_CheckBit(&GPIOC->DATA, GPIOC_PIN_PTT))
            SYSTEM_DelayMs(100);
          break;
        }
      }
      */
    }

    keyStatus = GetCWKeyStatus();
    if (CW_KeyType == CW_KEYTYPE_STRAIGHT || CW_KeyType == CW_KEYTYPE_KEYPAD_ST )
    {
      if (keyStatus == CW_KEY_DAH || keyStatus == CW_KEY_DIT || keyStatus == CW_KEY_BOTH)
      {
        CWKeyDown();
        lastPressKeyTime = millis10();
      }
      else
      {
        CWKeyUp();
      }

    }
    else if (CW_KeyType == CW_KEYTYPE_PADDLE || CW_KeyType == CW_KEYTYPE_KEYPAD_PDL)
    {
      if (keyStatus == CW_KEY_DAH || keyStatus == CW_KEY_BOTH)
      {
        CWKeyDown();
        SYSTEM_DelayMs(CW_SPEED * 3);
        CWKeyUp();

        lastPressKeyTime = millis10();

        if (isDecoding == 1)      
          arPressKey[pressKeyIndex++] = 1;

        SYSTEM_DelayMs(CW_SPEED);
      }

      keyStatus = GetCWKeyStatus();
      if (keyStatus == CW_KEY_DIT || keyStatus == CW_KEY_BOTH)
      {
        CWKeyDown();
        SYSTEM_DelayMs(CW_SPEED);
        CWKeyUp();
        lastPressKeyTime = millis10();
        
        if (isDecoding == 1)      
          arPressKey[pressKeyIndex++] = 0;

        SYSTEM_DelayMs(CW_SPEED);
      }
      
      //Just CWN Mode 
      if (isDecoding == 1 /* && CWMode == CWMODE_CWN */)
      {
        if (pressKeyIndex > 0 &&  lastPressKeyTime + CW_DECODE_INTERVAL < millis10())
        {
          //sprintf(logBuff, "%u%u%u%u%u%u%u%u%u%u", arPressKey[0], arPressKey[1], arPressKey[2], arPressKey[3], arPressKey[4], arPressKey[5], 
          //arPressKey[6], arPressKey[7], arPressKey[8], arPressKey[9]);
          //WriteLogDisplay(logBuff, 0, 3);

          int decodeType = 0;
          char decodedChar = DecodeMorseData(arPressKey, pressKeyIndex, &decodeType);
          if (isDecoding == 1)
            CWDecodedChar(decodedChar, decodeType, autoSpaceCheck, cwTXScreenMode);

          spaceCheck = 1;
          pressKeyIndex = 0;
          memset(arPressKey, 5, sizeof(arPressKey));
        }
        else if (spaceCheck == 1 && pressKeyIndex == 0 && lastPressKeyTime + SPACE_INTERVAL < millis10())
        {
          //SPACE
          spaceCheck = 0;
          if (autoSpaceCheck)
            CWDecodedChar(' ', 0, autoSpaceCheck, cwTXScreenMode);
        }
        else if (pressKeyIndex > (int)sizeof(arPressKey))
        {
          pressKeyIndex = 0;
        }
      }
    }

    if ((! isHoldMode) && (millis10() - lastPressKeyTime >  (CW_TXDelay * 10) )) //MAX 250 * 10 = 2.5Secnd
    {
      if (KEYBOARD_Poll() == KEY_INVALID)
      {
        isRunning = 0;
        break;
      }
    }

		
	}   //end of while(1)

  //deinit

  if (CW_Mode == CWMODE_CWFM)
  {
    AUDIO_AudioPathOff();
    BK4819_SetAF(BK4819_AF_MUTE);
    
    BK4819_WriteRegister(BK4819_REG_70, 0x0000);
    BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);
    BK4819_ExitTxMute();
  }
  else if (CW_Mode == CWMODE_CW)
  {
	  BK4819_WriteRegister(0x40, 0x4D0 | (0x01U << BK4819_REG_40_SHIFT_ENABLE_DEVIATION));
  }

	RADIO_SelectVfos();

	gUpdateStatus   = true;

#ifdef ENABLE_NOAA
		RADIO_ConfigureNOAA();
#endif
		RADIO_SetupRegisters(true);

  //BackLightBlink(1);
  //RADIO_SetupRegisters(false);
  gUpdateDisplay = true;  

}
