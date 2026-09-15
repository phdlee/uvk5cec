#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include "driver/bk4819.h"
#include "driver/keyboard.h"
#include "audio.h"
#include "string.h"
#include <stdint.h>
#include <string.h>
#include "external/printf/printf.h"
//#include "gpsinfo.h"
#include "driver/eeprom.h"
#include "misc.h"
#include "radio.h"
#include "driver/system.h"
#include "driver/st7565.h"
#include "settings.h"
#include "driver/systick.h"
#include "ui/helper.h"
#include "ui/ui.h"
//#include "ui\main.h"
//#include "font.h"
//#include "functions.h"
#include "app/app.h"
#include "ui/helper.h"
//#include "driver\uart.h"
#include "bsp/dp32g030/uart.h"
#include "bsp/dp32g030/dma.h"
#include "bsp/dp32g030/syscon.h"
#include "ceccommon.h"
#include "bitmaps.h"
#include "am_fix.h"

uint16_t beforeReg = 0;
static void SetFreqSpectrum(uint32_t f, bool applyFilter) 
{
  //if APPLY _HS THEN REDUCE 70msec for 1 time
  //BK4819_SetFrequency(f);
	BK4819_WriteRegister(BK4819_REG_38, (f >>  0) & 0xFFFF);
	BK4819_WriteRegister(BK4819_REG_39, (f >> 16) & 0xFFFF);

  //reduce 50~70 msec for 1 time (128)
  if (applyFilter)
  {
    BK4819_PickRXFilterPathBasedOnFrequency(f);
  }

  BK4819_WriteRegister(BK4819_REG_30, 0);
  BK4819_WriteRegister(BK4819_REG_30, beforeReg); //reduce 90msec for 1 time
}


/*
uint16_t GetRssi_spetcrum() 
{
  //uint16_t rssi = BK4819_GetRSSI();
  //return rssi;
  //return (BK4819_ReadRegister_HS(0x61) & 0xFF) * 10- 10;
  //return (BK4819_ReadRegister(BK4819_REG_67) & 0x01FF) * 3 - 30; //  - 30;
  //return (BK4819_ReadRegister(BK4819_REG_67) & 0x01FF) * 3; //  - 30;
  return (BK4819_ReadRegister(BK4819_REG_67) & 0x01FF); //  - 30;
}
*/

#define CENTER_POSITION 63
#define MaxYPosition 127  //MAX X POSITION


//61164
uint8_t GetRSSILevel(uint8_t _targetOffset)
{
  //SYSTICK_DelayUs(5000);
  SYSTICK_DelayUs(4500);  //not full charging adc value but little diff, useful
  //delay(2);
  //nowRssi = Rssi2PX2(GetRssi_spetcrum(), 0, 50);// / 2;

  uint16_t nowCurrentRssi = (BK4819_ReadRegister(BK4819_REG_67) & 0x01FF);
  if (nowCurrentRssi <  _targetOffset)
    nowCurrentRssi = 0;
  else
    nowCurrentRssi -=  _targetOffset;

  return Rssi2PX2(nowCurrentRssi, 0, 50);// / 2;
}

//61144 - 59516 = 1628
void CEC_Spectrum_WithWaterFall()
{
  uint8_t _FilterWidth[3][5] =
  {
    "25K", "12K", "6.5K"
  };
  uint8_t _ScanType[3][5] =
  {
    "SCAN", "RECV", "AUTO"
  };
  uint8_t tmpBuff[16];
  uint8_t nowRssi = 0;
  uint8_t isBarLock = 1;
  //uint32_t tmpCTCSS = 0;
  //uint8_t tmpCTCSS_15 = 0;
  //uint8_t WScanMode = 0;  //SCAN, RECV, AUTO
  uint8_t nowSideKeyFunction = 0;  //0 : * 10 Move, 1:BandWidth, 2 : Offset Position, 3 : WaterFall Level, 4: Scan Mode, 5: Auto Level
  //uint32_t startFreq = 14512345; //145Mhz
  //uint32_t spStep = pVfo->StepFrequency;
  uint16_t spStep = gTxVfo->StepFrequency;
  uint32_t centerFreq = gTxVfo->freq_config_RX.Frequency;
  uint32_t startFreq = 0;
  uint32_t endFreq = 0;
  uint32_t targetFreq = 0;
  uint32_t markedFreq = centerFreq;

  //uint8_t centerPosition = 61;
  uint8_t centerPosition = CENTER_POSITION;
  uint8_t maxLevel = 0;
  uint8_t minLevel = 0;
  uint8_t maxPosition = 0;
  uint8_t receiveStartRSSI = 0;

  uint8_t isKeyUpCheck = 0;

  //uint8_t WAutoLevel = 25;
  bool isScanMode = true; //SCAN OR SPEAKER
  bool isRecalculatedStartEnd = true;
  uint8_t tmpBuffRssi[128];

  struct
  {
    uint8_t RSSIOffset;
    uint8_t WaterFallLevel;
    uint8_t WBandWidth; //0 : WIDE, NARROW, NARROWER
    uint8_t WAutoLevel;
    uint8_t Padding[4];
  }  SPECTRUM;

  //CURRENT  Freq
  EEPROM_ReadBuffer(CEC_EEPROM_SPECTRUM, &SPECTRUM, 8);

  SPECTRUM.RSSIOffset =  SPECTRUM.RSSIOffset < 200 ? SPECTRUM.RSSIOffset  : 15;
  SPECTRUM.WaterFallLevel =  SPECTRUM.WaterFallLevel < 100 ? SPECTRUM.WaterFallLevel  : 5;
  SPECTRUM.WBandWidth =  SPECTRUM.WBandWidth < 3 ? SPECTRUM.WBandWidth  : 2;
  SPECTRUM.WAutoLevel =  SPECTRUM.WAutoLevel < 30 ? SPECTRUM.WAutoLevel  : 25;

  uint8_t WScanMode = 0;  //SCAN, RECV, AUTO
//  CalculateFrequency(0, centerFreq, centerPosition, &startFreq, &endFreq, spStep);
//  8byte reduce 

  beforeReg = BK4819_ReadRegister(BK4819_REG_30);

  //Clear Full
  memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

  CEC_ReceiveMode(false);
  BK4819_SetFilterBandwidth(SPECTRUM.WBandWidth, MODULATION_FM, true);

  bool isSideKeyWork = false;
  uint8_t processStep = 1; // <-- SPECTRUM SPLIT

  while (1)
  {
    processStep = ! processStep;  //2 Section 
    
    //Wait KEY UP
    if (isKeyUpCheck)
    {
      CEC_WaitKeyRelease();
      isKeyUpCheck = false;
    }

    //isKeyUpCheck = 0;
    KEY_Code_t akey = KEYBOARD_Poll();

    isSideKeyWork = false;
    if (akey == KEY_SIDE1 || akey == KEY_SIDE2)
    {
      if (nowSideKeyFunction == 0)
      {
        akey = akey == KEY_SIDE1 ? KEY_UP : KEY_DOWN; ///functionShortKeys[nowSideKeyFunction][akey - KEY_SIDE2];
        isSideKeyWork = true;
      }
      else if (nowSideKeyFunction == 1)
      {
        if (++(SPECTRUM.WBandWidth) > 2)
          SPECTRUM.WBandWidth = 0;
        BK4819_SetFilterBandwidth(SPECTRUM.WBandWidth, MODULATION_FM, true);  //MODULATION_FM <-- Dont' worry, Not changed Modulation
        isKeyUpCheck = true;
      }
      else if (nowSideKeyFunction == 2)
      {
        SPECTRUM.RSSIOffset += akey ==  KEY_SIDE1 ? 1 : -1;
      }
      else if (nowSideKeyFunction == 3)
      {
        SPECTRUM.WaterFallLevel += akey == KEY_SIDE1 ? 1 : -1;
      }
      else if (nowSideKeyFunction == 4)
      {
        if (++WScanMode > 2)
          WScanMode = 0;

        isScanMode = WScanMode != 1;
        SetFreqSpectrum(centerFreq, 1);
        CEC_ReceiveMode(! isScanMode);
        isKeyUpCheck = true;
      }
      else if (nowSideKeyFunction == 5)
      {
        SPECTRUM.WAutoLevel += (akey == KEY_SIDE1 ? 1 : -1);
        if (SPECTRUM.WAutoLevel > 30)
          SPECTRUM.WAutoLevel = 30;
      }
    }

    //uint8_t nowSideKeyFunction = 0;  //0 : * 10 Move, 1:BandWidth, 2 : Offset Position, 3 : WaterFall Level, 4: Scan Mode
    if (akey == KEY_MENU)
    {
      if (++nowSideKeyFunction > 5)
        nowSideKeyFunction = 0;

      isKeyUpCheck = true;
    }
    else if (akey == KEY_UP || akey == KEY_DOWN )
    {
      if (isBarLock)
      {
        centerFreq += (akey == KEY_UP ? spStep : -spStep) * (isSideKeyWork ? 10 : 1);
        isRecalculatedStartEnd = true;
        //startFreq = centerFreq - (spStep * centerPosition);
        //endFreq = centerFreq + (spStep * (MaxYPosition - centerPosition));
      }
      else
      {
        centerPosition += akey == KEY_UP ? 1 : -1;
        //if (centerPosition < 0) //for Reduce program memory, -1 -> 255 so changed Max YPosition
        //  centerPosition = 0;
        //else 
        if (centerPosition > MaxYPosition)
          centerPosition = MaxYPosition;

        centerFreq = startFreq + (centerPosition * spStep);
      }

      SetFreqSpectrum(centerFreq, 1);
    }
    else if (akey == KEY_F)
    {
      isBarLock = ! isBarLock;
      isKeyUpCheck = true;
    }
    else if (akey == KEY_EXIT || akey == KEY_7)
    {
      if (akey == KEY_EXIT)
        gTxVfo->freq_config_RX.Frequency = centerFreq;

      EEPROM_WriteBuffer(CEC_EEPROM_SPECTRUM, &SPECTRUM);
      RestoreReceiveMode();
      SETTINGS_SaveSettings();
      return;
    }
    else if (akey == KEY_1)
    {
      //MARKING Top Value
      uint8_t _xPos = maxPosition > 97 ? 97 : maxPosition;
      markedFreq = startFreq + (maxPosition * spStep);
      memset(&gFrameBuffer[5][_xPos], 0, 29);
      CEC_DisplayFreqSmallst(markedFreq, _xPos, 41);
      isKeyUpCheck = true;
    }
    else if (akey == KEY_2 || akey == KEY_3)
    {
      //if (markedFreq >= startFreq && markedFreq <= endFreq)
      //if (markedFreq > 0)
      centerFreq = markedFreq;
      if (isBarLock)
      {
        if (akey == KEY_3)
          centerPosition = CENTER_POSITION;

        isRecalculatedStartEnd = true;
      }
      else
      {
        centerPosition = (markedFreq - startFreq)  / spStep;
        //if (centerPosition < 0) //for Reduce program memory, -1 -> 255 so changed Max YPosition
        //  centerPosition = 0;
        //else 
        //if (centerPosition > MaxYPosition)
        //  centerPosition = MaxYPosition;

        //centerFreq = startFreq + (centerPosition * spStep);
      }
    }
    else if (akey == KEY_STAR)
    {
      isScanMode = !isScanMode;
      SetFreqSpectrum(centerFreq, 1);
      CEC_ReceiveMode(! isScanMode);
      isKeyUpCheck = true;
    }
    else if (akey == KEY_0)
    {
      //Atuo Setup
      //get low field
      minLevel = 250;
      isKeyUpCheck = 3;  //All Scan and Auto Setup
    }
    else if (akey == KEY_8)
    {
      BACKLIGHT_TurnOn();
    }
    else if (akey == KEY_9)
    {
      BACKLIGHT_TurnOff();
    }

    if (isRecalculatedStartEnd)
    {
      isRecalculatedStartEnd = false;
      startFreq = centerFreq - (spStep * centerPosition);
      endFreq = centerFreq + (spStep * (MaxYPosition - centerPosition));
    }

    //==============================================================
    //DISPLAY STATUS
    //==============================================================
    memset(gStatusLine, 0, sizeof(gStatusLine));

    CEC_DisplaySmallest(_FilterWidth[SPECTRUM.WBandWidth], 2,  1, true, true);
    CEC_DisplayValueSmallst("P:%2u", SPECTRUM.RSSIOffset, 26, 1, true);
    CEC_DisplayValueSmallst("W:%2u", SPECTRUM.WaterFallLevel, 50, 1, true);
    CEC_DisplaySmallest(_ScanType[WScanMode], 74,  1, true, true); //RECEIVE ON / OFF
    CEC_DisplayValueSmallst("L:%2u", SPECTRUM.WAutoLevel, 100, 1, true);

    // KEY-LOCK indicator
    if (isBarLock) 
      memcpy(&gStatusLine[120], BITMAP_KeyLock, sizeof(BITMAP_KeyLock));

    //SELECT CURRENT FUNCTION
    if (nowSideKeyFunction > 0)
    {
      CEC_ReverseScreen(&gStatusLine[(nowSideKeyFunction -1) * 24],  23);
    }

    UI_DrawLineBuffer(&gStatusLine, 0, 7, 128, 7, true);    
    ST7565_BlitStatusLine();

    //===========================================================================
    //DISPLAY MAIN
    //===========================================================================
    DrawFrequencySmall(centerFreq, 33, 0, 0);
    if (centerFreq < 100000000)
    {
      sprintf(tmpBuff, "%02u", (centerFreq % 100));
      CEC_DisplaySmallest(tmpBuff, 84,  2, false, true);

      CEC_DisplayFreqSmallst(startFreq, 2, 2);
      CEC_DisplayFreqSmallst(endFreq, 95, 2);
    }

    CEC_ReverseScreen(gFrameBuffer[0], 128);
    ST7565_BlitFullScreen();

    //===========================================================================
    //DRAW SPECTRUM
    //===========================================================================
    if (isScanMode)
    {
      for (int i = 0; i < 64; i++)
      {
        for (int j = 0; j < 5; j++)
          gFrameBuffer[j][i * 2 + processStep] = 0x00;
      }

      //When First Step => Init Value
      if (! processStep)
      {
        //memset(gFrameBuffer, 0, 128 * 5);
        maxLevel = 0;
        //minLevel = 250;
      }

      for (int i = 0; i < 128; i++)
      {
        if (isKeyUpCheck != 3 &&  (i % 2) != processStep) //this time skip
        //if ((i % 2) != processStep) //this time skip /  for reduce program
          continue;

        targetFreq = startFreq + (i * spStep);
        SetFreqSpectrum(targetFreq, i == 0);

        nowRssi = GetRSSILevel(SPECTRUM.RSSIOffset);

        if (i > (centerPosition -3) && i < (centerPosition + 3))
        {
          //Selected Range
          //if (nowRssi > 21)
          //UI_DrawLineBuffer(gFrameBuffer, i, 20, i, 38 - nowRssi, true);
          UI_DrawLineBuffer(gFrameBuffer, i, 20, i, 38 - nowRssi, true);    
        }
        else
          UI_DrawLineBuffer(gFrameBuffer, i, 38, i, 38 - nowRssi, true);

        //gFrameBuffer[6][i] = gFrameBuffer[6][i] << 1 | ((gFrameBuffer[5][i]) >> 7);
        //gFrameBuffer[5][i] = gFrameBuffer[5][i] << 1 | ((nowRssi > SPECTRUM.WaterFallLevel ? 1 : 0));
        tmpBuffRssi[i] =((nowRssi > SPECTRUM.WaterFallLevel ? 1 : 0));

        if (nowRssi > maxLevel)
        {
          maxLevel = nowRssi;
          maxPosition = i;
        }

        if (minLevel > nowRssi)
          minLevel = nowRssi;
      }

      if (processStep)
      {
        for (int i = 0; i < 128; i++)
        {
          gFrameBuffer[6][i] = gFrameBuffer[6][i] << 1 | ((gFrameBuffer[5][i]) >> 7);
          gFrameBuffer[5][i] = gFrameBuffer[5][i] << 1 | tmpBuffRssi[i];
        }
      }


      if (isKeyUpCheck == 3)
      {
        //AutoSetMode
        //SPECTRUM.RSSIOffset = minLevel; // > 5 ? minLevel - 3 : 3;
        SPECTRUM.WaterFallLevel = minLevel + 3; //(millis10() % 4);
      }
     /*
     //Not Improve so not use
      SetFreqSpectrum(startFreq, 1);

      for (int i = 0; i < 128; i++)
      {
        //Read RSSI Level
        nowRssi = GetRSSILevel(SPECTRUM.RSSIOffset);

        //Set Frequency
        targetFreq = startFreq + ((i + 1) * spStep);  //Next Frequency Set for reduce adc charging time
        SetFreqSpectrum(targetFreq, 0);

        if (i >= (centerPosition -3) && i < (centerPosition + 3))
        {
          if (nowRssi > 21)
            UI_DrawLineBuffer(gFrameBuffer, i, 21, i, 38 - nowRssi, true);

          UI_DrawLineBuffer(gFrameBuffer, i, 20, i, 38 - nowRssi, true);    
        }
        else
          UI_DrawLineBuffer(gFrameBuffer, i, 38, i, 38 - nowRssi, true);

        gFrameBuffer[6][i] = gFrameBuffer[6][i] << 1 | ((gFrameBuffer[5][i]) >> 7);
        gFrameBuffer[5][i] = gFrameBuffer[5][i] << 1 | ((nowRssi > SPECTRUM.WaterFallLevel ? 1 : 0));


        if (nowRssi > maxLevel)
        {
          maxLevel = nowRssi;
          maxPosition = i;
        }
      }
      */

    }
    else
    {
        //RECEIVE MODE
//AMFIX MODE
#ifdef ENABLE_AM_FIX
      if (gRxVfo->Modulation == MODULATION_AM) 
      {
        AM_fix_10ms(gEeprom.RX_VFO);
      }
#endif 

    //200 -> 50 reduce for AMFIX (org : amfix process 10msec)
      delay(50);
    }

    //AUTO DETECT SCAN OR RECVIE CHANGE MODE (0 : SCAN, 1 : MONITOR 2:AUTO SCAN)
    //  WScanMode
    if (WScanMode == 2) //increase 12 byte  //AUTO MODE
    {
      if (isScanMode) //Not Receive mode
      {
        for (int i = 0; i < 128; i += 4)
          UI_DrawPixelBuffer(gFrameBuffer, i, 38 - SPECTRUM.WAutoLevel, 1);

        if (maxLevel > SPECTRUM.WAutoLevel)
        {
          //Receive Mode Change
          targetFreq = startFreq + (maxPosition * spStep);
          memset(gFrameBuffer[1], 0, 128);

          //DrawFrequencySmall(targetFreq, maxPosition > 77  ? 77 : maxPosition , 0, 1);
          //CEC_DisplayFreqSmallst(targetFreq*10, maxPosition > 97  ? 97 : maxPosition , 9);
          CEC_DisplayFreqSmallst(targetFreq, maxPosition, 9);

          SetFreqSpectrum(targetFreq, true);
          isScanMode = false;
          CEC_ReceiveMode(! isScanMode);
          receiveStartRSSI = 0;
        }
      }
      else
      {
        /*
        #define RECEIVE_CUTOFF_THRES 2
        //Over Program Memory, so Receive off -> Manual
        //Check Cutoff Signal 
        //uint16_t nowCurrentRssi = (BK4819_ReadRegister(BK4819_REG_67) & 0x01FF);
        uint8_t tmpRSSI = (BK4819_ReadRegister(BK4819_REG_67) & 0x01FF) / 2;// GetRSSILevel(SPECTRUM.RSSIOffset);
        //if (receiveStartRSSI  tmpRSSI)
        if (receiveStartRSSI < tmpRSSI)
          receiveStartRSSI = tmpRSSI;
        else if (receiveStartRSSI - RECEIVE_CUTOFF_THRES > tmpRSSI)
        {
          isScanMode = true;
          CEC_ReceiveMode(! isScanMode);

        }
        */

      }
    } //end of if WScanMode ==

  } //end of while
}
