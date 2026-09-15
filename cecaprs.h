#ifndef CECAPRS_H
#define CECAPRS_H

#include <stdbool.h>
#include <math.h>
#include <stdio.h>
#include "driver\bk4819.h"
#include "driver\keyboard.h"
#include "audio.h"
#include "string.h"
#include <stdint.h>
#include <string.h>
#include "external/printf/printf.h"
//#include "gpsinfo.h"
#include "ceccommon.h"

#define DELAY_TIME_FOR_1200BPS 565  //540 ~ 580

//#define _FLAG       0x7E	//Manual P.22, Fixed 0x7E Start APRS Protocol 
#define _CTRL_ID    0x03	//Manual P.22, Fixed 0x03
#define _PID        0xF0	//Manual P.22, Fixed 0xF0, End of Head


__inline uint16_t scale_freq(const uint16_t freq)
{
//	return (((uint32_t)freq * 1032444u) + 50000u) / 100000u;   // with rounding
	return (((uint32_t)freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}

void WriteAFSK1200(uint8_t writeBit);
void AX25_SendChar(uint8_t _srcChar, bool chkContinueTrueBit);
void AX25_SendString(const char *in_string, int len);
void Send_Flag_0x7E(uint8_t _sendCount);
void AX25_Send_Header(char _AprsDataType, uint8_t * _fromCall, uint8_t * _digi1, uint8_t * _digi2, bool _isSendSecondDigi);
void AX25_Send_Payload(char _AprsDataType, uint8_t *aprsMsg1, uint8_t *aprsMsg2, uint8_t *_dxCall);
void CEC_APRS_SEND(char _aprsDataType);



#endif