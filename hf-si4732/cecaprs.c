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

bool nada = _2400;

const bool play_speaker = true;
const unsigned int tone1_Hz = 1200;
const unsigned int tone2_Hz = 2200;
const unsigned int level	= 250;
uint32_t APRS_Freq = 14439000;

/*
 * This strings will be used to generate AFSK signals, over and over again.
 */
const char mycall[8] = "KD8CEC";
char myssid = 1;

const char dest[8] = "APZ";
const char dest_beacon[8] = "BEACON";

const char digi[8] = "WIDE2";
char digissid = 2;

const char comment[128] = "www.hamsky.com (kd8cec)";
const char mystatus[128] = "APRS Test for UV-K5";

char latBuff[10];
char lonBuff[10];
int coord_valid;
const char sym_ovl = 'H';
const char sym_tab = 'a';

unsigned int tx_delay = 5000;
unsigned int str_len = 400;

char bit_stuff = 0;
unsigned short crc=0xffff;

char rmc[100];
char rmc_stat;

const uint8_t gain   = (1u << 3) | (0u << 0);
const uint8_t enable = 1;

void WriteAFSK1200(uint8_t writeBit)
{
    /*
  if(writeBit == 1)
    BK4819_WriteRegister(BK4819_REG_71, scale_freq(tone1_Hz));  
  else
    BK4819_WriteRegister(BK4819_REG_71, scale_freq(tone2_Hz));  
    */
  BK4819_WriteRegister(BK4819_REG_71, scale_freq(writeBit == 1 ? tone1_Hz : tone2_Hz));
  SYSTICK_DelayUs(DELAY_TIME_FOR_1200BPS); //540 ~ 582까지 가능한데 560~570이 가장 성능이 좋다.
}

/*
 * This function will calculate CRC-16 CCITT for the FCS (Frame Check Sequence)
 * as required for the HDLC frame validity check.
 * 
 * Using 0x1021 as polynomial generator. The CRC registers are initialized with
 * 0xFFFF
 */
void calc_crc(bool in_bit)
{
  unsigned short xor_in;
  
  xor_in = crc ^ in_bit;
  crc >>= 1;

  if(xor_in & 0x01)
    crc ^= 0x8408;
}

void send_crc(void)
{
  unsigned char crc_lo = crc ^ 0xff;
  unsigned char crc_hi = (crc >> 8) ^ 0xff;

  send_char_NRZI(crc_lo, true);
  send_char_NRZI(crc_hi, true);
}

void send_header(char msg_type)
{
  char temp;

  /*
   * APRS AX.25 Header 
   * ........................................................
   * |   DEST   |  SOURCE  |   DIGI   | CTRL FLD |    PID   |
   * --------------------------------------------------------
   * |  7 bytes |  7 bytes |  7 bytes |   0x03   |   0xf0   |
   * --------------------------------------------------------
   * 
   * DEST   : 6 byte "callsign" + 1 byte ssid
   * SOURCE : 6 byte your callsign + 1 byte ssid
   * DIGI   : 6 byte "digi callsign" + 1 byte ssid
   * 
   * ALL DEST, SOURCE, & DIGI are left shifted 1 bit, ASCII format.
   * DIGI ssid is left shifted 1 bit + 1
   * 
   * CTRL FLD is 0x03 and not shifted.
   * PID is 0xf0 and not shifted.
   */

  /********* DEST ***********/
  if(msg_type == _BEACON)
  {
    temp = strlen(dest_beacon);
    for(int j=0; j<temp; j++)
      send_char_NRZI(dest_beacon[j] << 1, true);
  }
  else
  {
    temp = strlen(dest);
    for(int j=0; j<temp; j++)
      send_char_NRZI(dest[j] << 1, true);
  }
  if(temp < 6)
  {
    for(int j=0; j<(6 - temp); j++)
      send_char_NRZI(' ' << 1, true);
  }
  send_char_NRZI('0' << 1, true);
  

  
  /********* SOURCE *********/
  temp = strlen(mycall);
  for(int j=0; j<temp; j++)
    send_char_NRZI(mycall[j] << 1, true);
  if(temp < 6)
  {
    for(int j=0; j<(6 - temp); j++)
      send_char_NRZI(' ' << 1, true);
  }
  send_char_NRZI((myssid + '0') << 1, true);

  
  /********* DIGI ***********/
  temp = strlen(digi);
  for(int j=0; j<temp; j++)
    send_char_NRZI(digi[j] << 1, true);
  if(temp < 6)
  {
    for(int j=0; j<(6 - temp); j++)
      send_char_NRZI(' ' << 1, true);
  }
  send_char_NRZI(((digissid + '0') << 1) + 1, true);

  /***** CTRL FLD & PID *****/
  send_char_NRZI(_CTRL_ID, true);
  send_char_NRZI(_PID, true);
}

void send_payload(char type)
{
  /*
   * APRS AX.25 Payloads
   * 
   * TYPE : POSITION
   * ........................................................
   * |DATA TYPE |    LAT   |SYMB. OVL.|    LON   |SYMB. TBL.|
   * --------------------------------------------------------
   * |  1 byte  |  8 bytes |  1 byte  |  9 bytes |  1 byte  |
   * --------------------------------------------------------
   * 
   * DATA TYPE  : !
   * LAT        : ddmm.ssN or ddmm.ssS
   * LON        : dddmm.ssE or dddmm.ssW
   * 
   * 
   * TYPE : STATUS
   * ..................................
   * |DATA TYPE |    STATUS TEXT      |
   * ----------------------------------
   * |  1 byte  |       N bytes       |
   * ----------------------------------
   * 
   * DATA TYPE  : >
   * STATUS TEXT: Free form text
   * 
   * 
   * TYPE : POSITION & STATUS
   * ..............................................................................
   * |DATA TYPE |    LAT   |SYMB. OVL.|    LON   |SYMB. TBL.|    STATUS TEXT      |
   * ------------------------------------------------------------------------------
   * |  1 byte  |  8 bytes |  1 byte  |  9 bytes |  1 byte  |       N bytes       |
   * ------------------------------------------------------------------------------
   * 
   * DATA TYPE  : !
   * LAT        : ddmm.ssN or ddmm.ssS
   * LON        : dddmm.ssE or dddmm.ssW
   * STATUS TEXT: Free form text
   * 
   * 
   * All of the data are sent in the form of ASCII Text, not shifted.
   * 
   */
  if(type == _GPRMC)
  {
    send_char_NRZI('$', true);
    send_string_len(rmc, strlen(rmc)-1);
  }
  else if(type == _FIXPOS)
  {
    //Get Position Info from GPS Maanage Code
    GetLastPosition(latBuff, lonBuff);    
    send_char_NRZI(_DT_POS, true);
    send_string_len(latBuff, strlen(latBuff));
    send_char_NRZI(sym_ovl, true);
    send_string_len(lonBuff, strlen(lonBuff));
    send_char_NRZI(sym_tab, true);
  }
  else if(type == _STATUS)
  {
    send_char_NRZI(_DT_STATUS, true);
    send_string_len(mystatus, strlen(mystatus));
  }
  else if(type == _FIXPOS_STATUS)
  {
    send_char_NRZI(_DT_POS, true);
    send_string_len(latBuff, strlen(latBuff));
    send_char_NRZI(sym_ovl, true);
    send_string_len(lonBuff, strlen(lonBuff));
    send_char_NRZI(sym_tab, true);

    send_string_len(comment, strlen(comment));
  }
  else
  {
    send_string_len(mystatus, strlen(mystatus));
  }
}

/*
 * This function will send one byte input and convert it
 * into AFSK signal one bit at a time LSB first.
 * 
 * The encode which used is NRZI (Non Return to Zero, Inverted)
 * bit 1 : transmitted as no change in tone
 * bit 0 : transmitted as change in tone
 */
void send_char_NRZI(unsigned char in_byte, bool enBitStuff)
{
  bool bits;
  
  for(int i = 0; i < 8; i++)
  {
    bits = in_byte & 0x01;

    calc_crc(bits);

    if(bits)
    {
      WriteAFSK1200(nada);
      bit_stuff++;

      if((enBitStuff) && (bit_stuff == 5))
      {
        nada ^= 1;
        WriteAFSK1200(nada);
        
        bit_stuff = 0;
      }
    }
    else
    {
      nada ^= 1;
      WriteAFSK1200(nada);

      bit_stuff = 0;
    }

    in_byte >>= 1;
  }
}

void send_string_len(const char *in_string, int len)
{
  for(int j=0; j<len; j++)
    send_char_NRZI(in_string[j], true);
}

void send_flag(unsigned char flag_len)
{
  for(int j=0; j<flag_len; j++)
    send_char_NRZI(_FLAG, 0); 
}

//static void ProcessKey(KEY_Code_t Key, bool bKeyPressed, bool bKeyHeld);
/*
 * In this preliminary test, a packet is consists of FLAG(s) and PAYLOAD(s).
 * Standard APRS FLAG is 0x7e character sent over and over again as a packet
 * delimiter. In this example, 100 flags is used the preamble and 3 flags as
 * the postamble.
 */
void send_packet(char packet_type)
{
  //digitalWrite(LED_BUILTIN, HIGH);
  //digitalWrite(_PTT, HIGH);

  //SYSTEMDelayMs(100);
  //For Delay Time Test 
  /*
    delayTime = delayTime - 2;
    if (delayTime < 300)
        delayTime = 800;
    uint8_t strBuff[32];
    sprintf(strBuff, " DELAY[ %u ]  ", delayTime);
	UI_PrintString(strBuff, 2, 127, 0, 8);
	ST7565_BlitFullScreen();
*/        
    
	AUDIO_AudioPathOff();
	//gEnableSpeaker = false;
	BK4819_ToggleGpioOut(BK4819_GPIO0_PIN28_RX_ENABLE, false);
	BK4819_SetFrequency(APRS_Freq);
	BK4819_PrepareTransmit();
	SYSTEM_DelayMs(10);
	BK4819_PickRXFilterPathBasedOnFrequency(APRS_Freq);
	BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);
	SYSTEM_DelayMs(5);
	BK4819_SetupPowerAmplifier(25, APRS_Freq);  //APRS 출력은 설정값으로 저장한다.
	SYSTEM_DelayMs(10);

    BK4819_WriteRegister(BK4819_REG_70, BK4819_REG_70_ENABLE_TONE1 | ((level & 0x7f) << BK4819_REG_70_SHIFT_TONE1_TUNING_GAIN));
    BK4819_EnableTXLink();
	//uint16_t reg = BK4819_ReadRegister(BK4819_REG_30);
	//BK4819_WriteRegister(BK4819_REG_30, 0x0000);
	//BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);


  /*
   * AX25 FRAME
   * 
   * ........................................................
   * |  FLAG(s) |  HEADER  | PAYLOAD  | FCS(CRC) |  FLAG(s) |
   * --------------------------------------------------------
   * |  N bytes | 22 bytes |  N bytes | 2 bytes  |  N bytes |
   * --------------------------------------------------------
   * 
   * FLAG(s)  : 0x7e
   * HEADER   : see header
   * PAYLOAD  : 1 byte data type + N byte info
   * FCS      : 2 bytes calculated from HEADER + PAYLOAD
   */
  
    send_flag(200);
    crc = 0xffff;
    send_header(packet_type);
    send_payload(packet_type);
    send_crc();
    send_flag(3);


	if (play_speaker)
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

  //BackLightBlink(1);   
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