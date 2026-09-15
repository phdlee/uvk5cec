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
#include "driver\systick.h"
#include "cecgps.h"

#define DELAY_TIME_FOR_1200BPS 565  //540 ~ 580

// Defines the Square Wave Output Pin
#define _1200   1
#define _2400   0

#define _FLAG       0x7e
#define _CTRL_ID    0x03
#define _PID        0xf0
#define _DT_EXP     ','
#define _DT_STATUS  '>'
#define _DT_POS     '!'

#define _GPRMC          1
#define _FIXPOS         2
#define _FIXPOS_STATUS  3
#define _STATUS         4
#define _BEACON         5

// Defines the Dorji Control PIN
#define _PTT      7
#define _PD       6
#define _POW      5

#define DRJ_TXD 10
#define DRJ_RXD 11

__inline uint16_t scale_freq(const uint16_t freq)
{
//	return (((uint32_t)freq * 1032444u) + 50000u) / 100000u;   // with rounding
	return (((uint32_t)freq * 1353245u) + (1u << 16)) >> 17;   // with rounding
}


void set_nada(bool nada);

void send_char_NRZI(unsigned char in_byte, bool enBitStuff);
void send_string_len(const char *in_string, int len);

void calc_crc(bool in_bit);
void send_crc(void);

void send_packet(char packet_type);
void send_flag(unsigned char flag_len);
void send_header(char msg_type);
void send_payload(char type);

char rx_gprmc(void);
char parse_gprmc(void);

void set_io(void);
void print_code_version(void);
void print_debug(char type);


#endif