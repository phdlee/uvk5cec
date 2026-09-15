#include "cecrtty.h"
#include "ceccommon.h"

// RTTY Output Frequency
#define RTTY_TXF  14510000

// RTTY Offset Frequency
//#define RTTY_OSET 170
#define RTTY_OSET 17

// DDS/Arduino Connections
//#define DDS_LOAD  8
//#define DDS_CLOCK 9
//#define DDS_DATA  10
//#define LED       13

#include <stdint.h>

#define ARRAY_LEN 32
#define LETTERS_SHIFT 31
#define FIGURES_SHIFT 27
#define LINEFEED 2
#define CARRRTN  8
 
#define is_lowercase(ch)    ((ch) >= 'a' && (ch) <= 'z')
#define is_uppercase(ch)    ((ch) >= 'A' && (ch) <= 'Z')
 
//unsigned long nmtime;
 
char letters_arr[33] = "\000E\nA SIU\rDRJNFCKTZLWHYPQOBG\000MXV\000";
char figures_arr[33] = "\0003\n- \a87\r$4',!:(5\")2#6019?&\000./;\000";

uint32_t beforeFreq = 0;

//#define GFSK_SIMULATE 1
int maxPower = 50;

void frequency(uint32_t targetFreq) 
{
    /*
  unsigned long tuning_word = (frequency * pow(2, 32)) / DDS_REF;
  digitalWrite (DDS_LOAD, LOW); // take load pin low

  for(int i = 0; i < 32; i++) {
    if ((tuning_word & 1) == 1)
      outOne();
    else
      outZero();
    tuning_word = tuning_word >> 1;
  }
  byte_out(0x09);
  digitalWrite (DDS_LOAD, HIGH); // Take load pin high again
  */
    const uint8_t gain   = (1u << 3) | (0u << 0);
    const uint8_t enable = 1;

    if (beforeFreq != targetFreq)
    {
        BK4819_WriteRegister(BK4819_REG_38, (targetFreq >>  0) & 0xFFFF);
        BK4819_WriteRegister(BK4819_REG_39, (targetFreq >> 16) & 0xFFFF);

#ifdef GFSK_SIMULATE
        BK4819_WriteRegister(BK4819_REG_36, (10 << 8) | (enable << 7) | (gain << 0));
        //BK4819_WriteRegister(BK4819_REG_36, (0 << 8) | (0 << 7) | (0 << 0));
        //BK4819_WriteRegister(BK4819_REG_36, (10 << 8) | (enable << 7) | (gain << 0));

        for (int i = 3;  i > 0; i--)
        {
            BK4819_WriteRegister(BK4819_REG_36, (i << 8) | (enable << 7) | (gain << 0));
            SYSTEM_DelayMs(1);
        }
#endif


        uint16_t tmp1 = 0xC1FE;
        tmp1 &= ~(1 << 15);	//VCO Calibration Enable  (이것만 해줘도 주파수 변경됨) - 0xC1FE로 다시 전송한경우 주파수 변경됨
        //tmp1 &= ~(0xF << 4);	//드드드 소리큼
        BK4819_WriteRegister(BK4819_REG_30, tmp1);
        BK4819_WriteRegister(BK4819_REG_30, 0xC1FE);

#ifdef GFSK_SIMULATE
			for (int i = 1;  i <= 5; i++)
			{
				BK4819_WriteRegister_HS(BK4819_REG_36, (i << 8) | (enable << 7) | (gain << 0));
				SYSTEM_DelayMs(1);
			}

        	//BK4819_WriteRegister_HS(BK4819_REG_36, ( 2 << 8) | (enable << 7) | (gain << 0));
        	BK4819_WriteRegister_HS(BK4819_REG_36, ( maxPower << 8) | (enable << 7) | (gain << 0));
        delay(13);
#else
        delay(22);
#endif

    }
    else
    {
        delay(22);
    }
}


// Transmit a bit as a mark or space
void rtty_txbit (int bit) {
  if (bit) 
  {
    frequency(RTTY_TXF + RTTY_OSET);
  } 
  else 
  {
    frequency(RTTY_TXF);
  }
  
  // Delay appropriately - tuned to 45.45 baud.
  //delay(22); //sets the baud rate
  //delayMicroseconds(250);
}

uint8_t char_to_baudot(char c, char *array)
{
  int i;
  for (i = 0; i < ARRAY_LEN; i++)
  {
    if (array[i] == c)
      return i;
  }
 
  return 0;
}
 
void rtty_txbyte(uint8_t b)
{
  int8_t i;
 
  rtty_txbit(0);
 
  /* TODO: I don't know if baudot is MSB first or LSB first */
  /* for (i = 4; i >= 0; i--) */
  for (i = 0; i < 5; i++)
  {
    if (b & (1 << i))
      rtty_txbit(1);
    else
      rtty_txbit(0);
  }
 
  rtty_txbit(1);
}
 
enum baudot_mode
{
  NONE,
  LETTERS,
  FIGURES


};
 

 void rtty_txstring(char *str)
{
  enum baudot_mode current_mode = NONE;
  char c;
  uint8_t b;
 
  while (*str != '\0')
  {
    c = *str;
    /* some characters are available in both sets */
    if (c == '\n')
    {
      rtty_txbyte(LINEFEED);
    }
    else if (c == '\r')
    {
      rtty_txbyte(CARRRTN);
    }
    else if (is_lowercase(*str) || is_uppercase(*str))
    {
      if (is_lowercase(*str))
      {
        c -= 32;
      }
 
      if (current_mode != LETTERS)
      {
        rtty_txbyte(LETTERS_SHIFT);
        current_mode = LETTERS;
      }
 
      rtty_txbyte(char_to_baudot(c, letters_arr));
    }
    else
    {
      b = char_to_baudot(c, figures_arr);
 
      if (b != 0 && current_mode != FIGURES)
      {
        rtty_txbyte(FIGURES_SHIFT);
        current_mode = FIGURES;
      }
 
      rtty_txbyte(b);
    }
 
    str++;
  }
}

void rttyTest()
{
    RADIO_PrepareTX();
    #define BK4819_REG_40 0x40U
    #define BK4819_REG_40_SHIFT_ENABLE_DEVIATION 12
    #define BK4819_REG_40_SHIFT_TX_DEVIATION 0

    //DEVIATION을 막는다. FM 변조 x)
    uint16_t regTemp1 = BK4819_ReadRegister(BK4819_REG_40);
    //WriteLogInt("REG1:", regTemp1, 0, 0);
    regTemp1 &= ~(0x01U << BK4819_REG_40_SHIFT_ENABLE_DEVIATION);
    //regTemp1 |= (0x01U << BK4819_REG_40_SHIFT_ENABLE_DEVIATION);
    regTemp1 &= ~(0x0FFFU << 0);
    //WriteLogInt("REG2:", regTemp1, 0, 0);
    BK4819_WriteRegister(0x40, 0);

    BK4819_SetupPowerAmplifier(0, 0);
    BK4819_ToggleGpioOut(BK4819_GPIO1_PIN29_PA_ENABLE, true);

    const uint8_t gain   = (1u << 3) | (0u << 0);
    const uint8_t enable = 1;


    while (1)
    {
        //Wait for KeyPress
        KEY_Code_t btn = KEYBOARD_Poll();
        if (btn != KEY_3)
        {
            SYSTEM_DelayMs(100);
            continue;
        }

        int maxPower = 50;

        BK4819_WriteRegister(BK4819_REG_36, (maxPower << 8) | (enable << 7) | (gain << 0));
        beforeFreq = 0;
        rtty_txstring("Hello World");
        BK4819_WriteRegister(BK4819_REG_36, (0 << 8) | (enable << 7) | (gain << 0));
    }
}