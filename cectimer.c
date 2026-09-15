#include <string.h>
#include <stdio.h>     // NULL

#ifdef ENABLE_AM_FIX
    #include "am_fix.h"
#endif
#include "app/app.h"
#include "app/dtmf.h"
#include "audio.h"
#include "bsp/dp32g030/gpio.h"
#include "bsp/dp32g030/syscon.h"
#include "board.h"
#include "driver/backlight.h"
#include "driver/bk4819.h"
#include "driver/gpio.h"
#include "driver/system.h"
#include "driver/systick.h"
#include "driver/uart.h"
#include "helper/battery.h"
#include "helper/boot.h"
#include "misc.h"
#include "radio.h"
#include "settings.h"
#include "ui/lock.h"
#include "ui/welcome.h"
#include "ui/menu.h"
#include "version.h"
#include "app/uart.h"


#include "bsp/dp32g030/portcon.h"
#include "bsp/dp32g030/saradc.h"
#include "bsp/dp32g030/syscon.h"
#include "driver/adc.h"

#include "font.h"
#include "driver/st7565.h"
#include "ui/helper.h"
#include <stdbool.h>
#include <stdint.h>
#include <string.h>
#include "external/printf/printf.h"

#include "ARMCM0.h"
#include "driver/systick.h"
#include "misc.h"

#include "bsp\dp32g030\irq.h"
#include "cectimer.h"
#include "ceccommon.h"

//Timer Handler
uint32_t timeIncVal = 0;
void HandlerTIMER_BASE0(void)
{
    /*
    if (timeIncVal % 2 == 0)
        BACKLIGHT_TurnOff();
    else
        BACKLIGHT_TurnOn();
    */
    //TIMERBASE0_HIGHCNT = 0;
    if (TIMERBASE0_IF & (1U << 1))  //HIGH CHECK
    {
        TIMERBASE0_IF |= 1U << 1;
        timeIncVal++;
    }
    
    //Not use
    /*
    if (TIMERBASE0_IF & (1U << 0))  //LOW CHECK
    {
        TIMERBASE0_IF |= 1U << 0;
        timeIncVal2++;
    }
    */
}

//interrupt 
void CECTimer0Enable(uint8_t timerType)
{
    //milisecnd and increase timeIncVal, uint32_t range is 4,294,967,295 / 1000 (sec) / 60 (min) / 60 / (housr) / 24 (day) is about 50day, enough
    TIMERBASE0_DIV = 48;    //1us단위로 tick 발생
    if (timerType == CEC_TIMER_MSEC)    //Milisecond
    {
        //for test 1sencd
        TIMERBASE0_HIGHLOAD = 1000;  //1000; //1mm sec단위 (16BIT : max 65535)
    }
    else if (timerType == CEC_TIMER_APRS) //1000(msec) / 1200 (1200bps)  = 0.833 (with process time)
    {
        //for test 1sencd
        //TIMERBASE0_HIGHLOAD = 832;  //1000; //1mm sec단위 (16BIT : max 65535)
        TIMERBASE0_HIGHLOAD = 820;  //1000; //1mm sec단위 (16BIT : max 65535) 795부터 가능 833
    }
    else if (timerType == CEC_TIMER_FT8) //FT8 0.16
    {
        //for ft8 160msec
        TIMERBASE0_HIGHLOAD = 4;  //1000; //1mm sec단위 (16BIT : max 65535)
    }

    TIMERBASE0_IE |= 1U << 1;   //인터럽트 발생
    TIMERBASE0_EN |= 1U << 1;   //enabled high count
    NVIC_EnableIRQ((IRQn_Type)DP32_TIMER_BASE0_IRQn);
}
void CECTimer0Disable()
{
    TIMERBASE0_EN &= ~(1U << 1);   //diabled high count
    NVIC_DisableIRQ((IRQn_Type)DP32_TIMER_BASE0_IRQn);
}

