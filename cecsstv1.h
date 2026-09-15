#ifndef CECSSTV1_H

#define CECSSTV1_H

#include "ceccommon.h"
//#include "gpsinfo.h"

#define delay(delayTime) SYSTEM_DelayMs(delayTime)
typedef uint8_t byte;

__inline uint16_t scale_freq_sstv(const uint16_t freq)
{
//	return (((uint32_t)freq * 1032444u) + 50000u) / 100000u;   // with rounding
	return (((uint32_t)freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}

#define MARTIN1  0
#define SCOTTIE1 1

void StartSSTVM1(int startType);
extern uint8_t SSTV_LCD_Start_Timer;
#endif