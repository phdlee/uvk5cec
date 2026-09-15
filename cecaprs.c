/*
    Modified by KD8CEC for Cortex-M0 with TONE Generator
    
 *  Copyright (C) 2018 - Handiko Gesang - www.github.com/handiko
 *  
 *  This program is free software: you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation, either version 3 of the License, or
 *  (at your option) any later version.
 *  
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *  
 *  You should have received a copy of the GNU General Public License
 *  along with this program.  If not, see <https://www.gnu.org/licenses/>.
 */

#include "cecaprs.h"
#include "settings.h"

uint8_t aprs_MYSSID;
//uint8_t aprs_DIGISSID;

unsigned short _AprsCRC = 0xffff;

void WriteAFSK1200(uint8_t writeBit)
{
  while(timeIncVal == 0)
    SYSTICK_DelayUs(1);
  timeIncVal = 0;
  BK4819_WriteRegister(BK4819_REG_71, scale_freq(writeBit == 1 ? 1200 : 2200));
}

void AX25_Send_Header(char _AprsDataType, uint8_t * _fromCall, uint8_t * _digi1, uint8_t * _digi2, bool _isSendSecondDigi)
{
  char temp;

  //FIXED DEST From Version 0.2D
  #define _FIXED_DST_LEN (6 + 1)  //not change DST + SSID 1 (MUST 0)
  //char _FIXED_DST[] = "APZK5C"; //Experimental tag
  char _FIXED_DST[] = "APK5C10"; //MUST 6 CHAR + 0 ssid

  //FIXED DEST
  for(int j = 0; j <_FIXED_DST_LEN; j++)
    AX25_SendChar(_FIXED_DST[j] << 1, true);
//  AX25_SendChar('0' << 1, true);  // Added _FIXED_DST FOR REDUCE PROGRAM 8BYTE

  //FROM CALL SIGN  
  for(int i = 0; i < 6; i++)
    AX25_SendChar(_fromCall[i] << 1, true);
  AX25_SendChar((aprs_MYSSID + '0') << 1, true);


 
  for(int i = 0; i < 7; i++)
    AX25_SendChar((_digi1[i] << 1) + ( ((i == 6) && (! _isSendSecondDigi)) ? 1 : 0), true);


  //62172 byte
/*
  for(int i = 0; i < 6; i++)
    AX25_SendChar(_digi1[i] << 1, true);
  AX25_SendChar(((_digi1[6]) << 1) + (_isSendSecondDigi ? 0 : 1), true);
*/



  //DIGI#2 (EXPERT MODE ) MUST 7 CHAR, IF NEED SSID SSID SET AT INDEX 7(6) ex) WIDE2 1
  if (_isSendSecondDigi)
  {
    for(int i = 0; i < 7; i++)
      AX25_SendChar((_digi2[i] << 1) + (i == 6 ? 1 : 0) , true);

  }

  //END OF HEAD
  AX25_SendChar(_CTRL_ID, true);
  AX25_SendChar(_PID, true);
}

void AX25_Send_Payload(char _AprsDataType, uint8_t *aprsMsg1, uint8_t *aprsMsg2, uint8_t *_dxCall)
{
  int coord_valid;
  const char sym_ovl = '/';
  const char sym_tab = 'D';

  //****************************   DATA TYPE CHAR  **********************
  //GPS POSTION NMEA $
  //TELEMETRY Report Format 'T'
  //Message Format  ':'  ADDRESS9 ':' Message Text, '{' Mesage No

  if(_AprsDataType == APRS_DATA_FIXPOS) //GPS or SOTRED POSITION
  {
    AX25_SendChar('!', true); //		//PAGE 104 ! or =
    AX25_SendString(aprsMsg1, strlen(aprsMsg1));
    AX25_SendChar(sym_ovl, true);
    AX25_SendString(aprsMsg2, strlen(aprsMsg2));
    AX25_SendChar(sym_tab, true);
  }
  else if(_AprsDataType == APRS_DATA_STATUS)
  {
    AX25_SendChar('>', true);     //Manual Page, 130
    AX25_SendString(aprsMsg1, strlen(aprsMsg1));
  }
  else if(_AprsDataType == APRS_DATA_MESSAGE || _AprsDataType == APRS_DATA_MESSAGE_CWMSG) //Manual Page 110
  {
    //Message Format  ':'  ADDRESS9 ':' Message Text, '{' Mesage No
    AX25_SendChar(':', true); //		//PAGE 104 ! or =
    AX25_SendString(_dxCall, 9);
    //_dxCall

    AX25_SendChar(':', true); //		//PAGE 104 ! or =
    AX25_SendString(aprsMsg1, strlen(aprsMsg1));
    AX25_SendChar('{', true); //		//PAGE 104 ! or =
    AX25_SendChar('1', true); //		//PAGE 104 ! or =
  }
  /*
  else if(_AprsDataType == APRS_DATA_MESSAGE_CWMSG) //Manual Page 110
  {
    //Message Format  ':'  ADDRESS9 ':' Message Text, '{' Mesage No
    AX25_SendChar(':', true); //		//PAGE 104 ! or =
    AX25_SendString("DS2BPX ", 7);
    //AX25_SendString(_dxCall, 9);
    //_dxCall

    AX25_SendChar(':', true); //		//PAGE 104 ! or =
    AX25_SendString(aprsMsg1, strlen(aprsMsg1));
    AX25_SendChar('{', true); //		//PAGE 104 ! or =
    AX25_SendChar('1', true); //		//PAGE 104 ! or =
  }
  */

  /*
  if(type == _GPRMC)
  {
    AX25_SendChar('$', true);
    AX25_SendString(rmc, strlen(rmc)-1);
  }
  else if(type == _FIXPOS)
  {
    //Get Position Info from GPS Maanage Code
    GetLastPosition(latBuff, lonBuff);    
    AX25_SendChar(_DT_POS, true);
    AX25_SendString(latBuff, strlen(latBuff));
    AX25_SendChar(sym_ovl, true);
    AX25_SendString(lonBuff, strlen(lonBuff));
    AX25_SendChar(sym_tab, true);
  }
  else if(type == _STATUS)
  {
    AX25_SendChar(_DT_STATUS, true);
    AX25_SendString(mystatus, strlen(mystatus));
  }
  else if(type == _FIXPOS_STATUS)
  {
    AX25_SendChar(_DT_POS, true);
    AX25_SendString(latBuff, strlen(latBuff));
    AX25_SendChar(sym_ovl, true);
    AX25_SendString(lonBuff, strlen(lonBuff));
    AX25_SendChar(sym_tab, true);

    AX25_SendString(comment, strlen(comment));
  }
  else
  {
    AX25_SendString(mystatus, strlen(mystatus));
  }
  */
}

//control byte 0x73 (0111 1110) <-- just continue send 1 bits (5 time)
//chkBitStuff always true except control data
void AX25_SendChar(uint8_t _srcChar, bool chkContinueTrueBit)
{
  static bool _AX25_SENT_BIT = false;
  static uint8_t trueSentCount = 0; //Check Continue True Bit
  uint8_t tempCRCValue;
  bool _currentBit;
  
  for(int i = 0; i < 8; i++)
  {
    _currentBit = _srcChar & 0x01;

    tempCRCValue = (_AprsCRC ^ _currentBit) & 0x01;
    _AprsCRC >>= 1;
    if(tempCRCValue)
      _AprsCRC ^= 0x8408;

    if(_currentBit)
    {
      WriteAFSK1200(_AX25_SENT_BIT);
      trueSentCount++;

      if((chkContinueTrueBit) && (trueSentCount == 5)) //if continue sent 1 (5 times), sent reverse bit 
      {
        _AX25_SENT_BIT ^= 1;
        WriteAFSK1200(_AX25_SENT_BIT);
        trueSentCount = 0;
      }
    }
    else
    {
      _AX25_SENT_BIT ^= 1;
      WriteAFSK1200(_AX25_SENT_BIT);

      trueSentCount = 0;
    }

    _srcChar >>= 1;
  }
}

void AX25_SendString(const char *in_string, int len)
{
  for(int i = 0; i < len; i++)
    AX25_SendChar(in_string[i], true);
}

//Send 0x7E (STX AND ETX of APRS, Manual P.12)
void Send_Flag_0x7E(uint8_t _sendCount)
{
  for(int i = 0; i < _sendCount; i++)
    AX25_SendChar(0x7E, 0); 
}

//static void ProcessKey(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
/*
 * In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
 * Standard APRS FLAG is 0x7e character sent over and over again as a packet
 * delimiter. In this example, 100 flags is used the preamble and 3 flags as
 * the postamble.
 */

#define APRS_FREQ_CH1_MSG  168
#define APRS_FREQ_CH2_GPS  169

/*
void CEC_CHANNEL_TO_DIGI(int readAddress, uint8_t * targetBuff)
{
  uint8_t _tmpData[11];  
  SETTINGS_FetchChannelName(_tmpData, readAddress);
  //uint8_t rstlength = strlen(_tmpData);
  //  _sendSecondDigi = strlen(_tmpData) > 2; //WIDE1-1

  sprintf(targetBuff, "%-7s", _tmpData);
  for (int i = 0; i < 7; i++)
    if (targetBuff[i] == '-')
      targetBuff[i] = ' ';

  //return rstlength;
}
*/

//Reduce 62204 -> 62196
void CEC_CHANNEL_TO_DIGI(int readAddress, uint8_t * targetBuff)
{
  uint8_t _tmpData[11];  
  SETTINGS_FetchChannelName(_tmpData, readAddress);
  uint8_t rstlength = strlen(_tmpData);

  //memset(targetBuff, ' ', 7);
  for (int i = 0; i < 7; i++)
  {
    //targetBuff[i] = i < rstlength ? _tmpData[i] : ' ';
    if (i < rstlength && _tmpData[i] != '-')
      targetBuff[i] = _tmpData[i];
    else
      targetBuff[i] = ' ';
  }
  //  _sendSecondDigi = strlen(_tmpData) > 2; //WIDE1-1

/*
  sprintf(targetBuff, "%-7s", _tmpData);
  for (int i = 0; i < 7; i++)
    if (targetBuff[i] == '-')
      targetBuff[i] = ' ';
*/
  //return rstlength;
}

/*
0	"STS",	//STATUS BROADCAST
1 "MSG",	//MESSAGE TO DX CALL
2	"CWM",	//CW Message CONTINUE
3	"POS"	//PSOTION
58 : RS-232 DATA SEND (any)
*/

void CEC_APRS_SEND(char _aprsDataType)
{
  //packet_type 4 //STATUS #define _STATUS    Message
  //packet-type 2 //GPS

  const uint32_t frequency = SETTINGS_FetchChannelFrequency(_aprsDataType == 3 ? APRS_FREQ_CH2_GPS : APRS_FREQ_CH1_MSG);

  rssiStartFreq = gCurrentVfo->pTX->Frequency;
  gCurrentVfo->pTX->Frequency = frequency;

  TXViaUART = 1;
  PrepareSWFSKTX();
  TXViaUART = 0;

  
  //Common Field
  uint8_t _tmpData[11];
  uint8_t aprs_call[7];
  //const char digi[8] = "WIDE1";
  uint8_t aprs_digi1[7];
  uint8_t aprs_digi2[7];
  uint8_t aprs_dxCall[10];
  uint8_t aprsMsg1[51];
  uint8_t aprsMsg2[11];

  //uint8_t aprs_dxCall2[10];

  bool _sendSecondDigi = false;
    
  /*
  SETTINGS_FetchChannelName(_tmpData, RIGINFO_MSG_MYCALL);
  sprintf(aprs_call, "%-6s", _tmpData);
  */
  CEC_CHANNEL_TO_DIGI(RIGINFO_MSG_MYCALL, aprs_call);

  CEC_CHANNEL_TO_DIGI(RIGINFO_MSG_APRSDIG1, aprs_digi1);
  CEC_CHANNEL_TO_DIGI(RIGINFO_MSG_APRSDIG2, aprs_digi2);
  //_sendSecondDigi = CEC_CHANNEL_TO_DIGI(RIGINFO_MSG_APRSDIG2, aprs_digi2) > 2;
  
  _sendSecondDigi = aprs_digi2[0] != ' ';


  SETTINGS_FetchChannelName(_tmpData, RIGINFO_MSG_DXCALL);
  sprintf(aprs_dxCall, "%-9s", _tmpData);
  

  //Status Message Field (4)
  if (_aprsDataType == APRS_DATA_STATUS || _aprsDataType == APRS_DATA_MESSAGE) //STATUS
  {
    SETTINGS_FetchChannelName(aprsMsg1, RIGINFO_MSG_APRSMSG);
  }
  else if (_aprsDataType == APRS_DATA_FIXPOS) //POSTION DATA
  {
    SETTINGS_FetchChannelName(aprsMsg1, RIGINFO_MSG_GPSLAT); 
    SETTINGS_FetchChannelName(aprsMsg2, RIGINFO_MSG_GPSLON); 

    ////Increase 336 byte, using float -> 7kbyte -> reduce 336byte (only int)
    CEC_GPSToAPRS(aprsMsg1, true);    
    CEC_GPSToAPRS(aprsMsg2, false);
  }
  else if (_aprsDataType == APRS_DATA_MESSAGE_CWMSG) //CW MESSAGE 1~5
  {
    uint8_t _writeIndex = 0;
    //RIGINFO_MSG_CWMSG0
    uint8_t isFindEnded = 0;
    for (int i = 0; i < 5; i++)
    {
      SETTINGS_FetchChannelName(_tmpData, RIGINFO_MSG_CWMSG0 + i );
      uint8_t tmpLen = strlen(_tmpData);
      //if (tmpLen == 0)
      //  break;
      for (int i = 0; i < tmpLen; i++)
      {
        if (_tmpData[i] == '#')
        {
          isFindEnded = true;
          break;
        }

        aprsMsg1[_writeIndex++] = _tmpData[i];
      }
      if (isFindEnded)
        break;
    }
    aprsMsg1[_writeIndex] = 0x00;
  }
  //Timer Start for MicroSecond Control
  CECTimer0Enable(CEC_TIMER_APRS);


  //Test by KD8CEC
  Send_Flag_0x7E(130);//630 -> 1000sec
  //send_flag(200);
  _AprsCRC = 0xFFFF;
  //_aprsDataType not use in AX25_Send_Header, but remain because reserved
  AX25_Send_Header(_aprsDataType, aprs_call, aprs_digi1, aprs_digi2, _sendSecondDigi);

  if (_aprsDataType == APRS_DATA_FROM_UART)
    AX25_SendString(&CommBuff[CommBuffUsingType], CommValue1);
  else
    AX25_Send_Payload(_aprsDataType, aprsMsg1, aprsMsg2, aprs_dxCall);

  //===========================
  //SEND LAST DATA OF APRS (CRC + FLAG)
  unsigned char _crc_lo = _AprsCRC ^ 0xFF;
  unsigned char _crc_hi = (_AprsCRC >> 8) ^ 0xFF;
  AX25_SendChar(_crc_lo, true);
  AX25_SendChar(_crc_hi, true);
  Send_Flag_0x7E(3);

  gCurrentVfo->pTX->Frequency = rssiStartFreq; //tmp Before Frequency, not rssiStartFreq
  RestoreReceiveMode();
}

/*
 * Function to randomized the value of a variable with defined low and hi limit value.
 * Used to create random AFSK pulse length.
 */
/*
void randomize(unsigned int &var, unsigned int low, unsigned int high)
{
  randomSeed(analogRead(A0));
  var = random(low, high);
}
*/
/*
 * 
 */
/*
char rx_gprmc(void)
{
  char temp;
  int c=0;

  for(int i=0;i<100;i++)
    rmc[i]=0;

  do
  {
    if(gps.available()>0)
      temp = gps.read(); 
  }
  while(temp!='$');

  do
  {
    if(gps.available()>0)
    {
      temp = gps.read();
      rmc[c] = temp;
      c++;
    }

    if(c==5)
    {
      if(rmc[4]!='C')
      {
        return 0;

        goto esc;
      }
    }
  }
  while((temp!=10)&&(temp!=13));

  c--;

  return c;

  esc:
  ;
}
*/

/*
char parse_gprmc(void)
{
  gps.begin(9600);

  rmc_stat = 0;
  rmc_stat = rx_gprmc();

  gps.flush();
  gps.end();

  if(rmc_stat > 10)
    return rmc_stat;
  else
    return 0;
}
*/

/*
int get_coord(void)
{
  //
  for(int i=0;i<7;i++)
  {
    lati[i] = rmc[i+18];
  }

  lati[7]=rmc[29];

  //
  for(int i=0;i<8;i++)
  {
    lon[i] = rmc[i+31];
  }

  lon[8]=rmc[43];

  if(rmc[16]=='A')
    return 1;
  else if(rmc[16]=='V')
    return 0;
  else
    return -1;
}

*/