#ifndef CECRTTY_H
#define CECRTTY_H

//#include <avr/pgmspace.h>
#include <stdint.h>
#include "driver/system.h"
#include "driver\backlight.h"
#include "driver\bk4819-regs.h"
#include "driver\bk4819.h"
#include "driver\keyboard.h"

#define delay(delayTime) SYSTEM_DelayMs(delayTime)
typedef uint8_t byte;
void rttyTest();


#endif