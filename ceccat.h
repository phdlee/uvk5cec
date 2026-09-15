#ifndef CECCAT_H
#define CECCAT_H

//#include <avr/pgmspace.h>
#include <stdint.h>
#include "driver/system.h"
#include "driver\backlight.h"


//for broken protocol
#define CAT_RECEIVE_TIMEOUT 500

#define CAT_MODE_LSB            0x00
#define CAT_MODE_USB            0x01
#define CAT_MODE_CW             0x02
#define CAT_MODE_CWR            0x03
#define CAT_MODE_AM             0x04
#define CAT_MODE_FM             0x08
#define CAT_MODE_DIG            0x0A
#define CAT_MODE_PKT            0x0C
#define CAT_MODE_FMN            0x88

#define ACK 0


//#define delay(delayTime) SYSTEM_DelayMs(delayTime / 10)
typedef uint8_t byte;

void Check_817Cat(uint8_t * recvBuff, byte fromType);

#endif