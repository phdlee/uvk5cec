#include <stdio.h>
#include "cecgps.h"
#include "cecaprs.h"
#include "ceccommon.h"

//#define GPS_SIMULATE 1

//VARIABLE FOR GPS INFORMATION

uint32_t lastGPSInfo_Time = 0;
uint32_t lastAPRSsend_Time = 0;
struct minmea_float lastGPSInfo_Latitude;
struct minmea_float lastGPSInfo_Longitude;
int lastGPSInfo_UTC_Hour = 0;
int lastGPSInfo_UTC_Min = 0;
int lastGPSInfo_UTC_Sec = 0;
int lastGPSInfo_UTC_Mi = 0;

uint16_t aprsInteral = 7000; //1초간격 (1msec단위)

//Using by APRS Module
uint8_t GetLastPosition(char* latBuff, char* longBuff)
{
    //latBuff, char* longBuff
    sprintf(latBuff,  "%3u.%05u", lastGPSInfo_Latitude.value / 100 / lastGPSInfo_Latitude.scale, (lastGPSInfo_Latitude.value /100) % lastGPSInfo_Latitude.scale);
    sprintf(longBuff, "%3u.%05u", lastGPSInfo_Longitude.value / 100 / lastGPSInfo_Longitude.scale, (lastGPSInfo_Longitude.value /100) % lastGPSInfo_Longitude.scale);
}


void UI_DisplayPosition(const char *string, uint8_t X, uint8_t Y, bool center)
{
	const unsigned int char_width  = 13;
	uint8_t           *pFb0        = gFrameBuffer[Y] + X;
	uint8_t           *pFb1        = pFb0 + 128;
	bool               bCanDisplay = false;

	uint8_t len = strlen(string);
	for(int i = 0; i < len; i++) {
		char c = string[i];
		if(c=='-') c = '9' + 1;
		if (bCanDisplay || c != ' ')
		{
			bCanDisplay = true;
			if(c>='0' && c<='9' + 1) {
				memcpy(pFb0 + 2, gFontBigDigits[c-'0'],                  char_width - 3);
				memcpy(pFb1 + 2, gFontBigDigits[c-'0'] + char_width - 3, char_width - 3);
			}
			else if(c=='.') {
				*pFb1 = 0x60; pFb0++; pFb1++;
				*pFb1 = 0x60; pFb0++; pFb1++;
				*pFb1 = 0x60; pFb0++; pFb1++;
				continue;
			}
			
		}
		else if (center) {
			pFb0 -= 6;
			pFb1 -= 6;
		}
		pFb0 += char_width;
		pFb1 += char_width;
	}
}
const uint8_t BITMAP_GPS[8] =
{	
	0b00011100,
	0b00100010,
	0b01011001,
	0b10100101,
	0b10100101,
	0b01011001,
	0b00100010,
	0b00011100,
};

//
extern uint32_t APRS_Freq;
uint32_t receiveCount = 0;
//DrawScreen
void DrawGPSInfo(uint8_t drawType)
{
    uint8_t strbuff[32];
    //================= 테스트 데이터
    //테스트 데이터
    /*
    lastGPSInfo_Latitude.value = 350900224;
    lastGPSInfo_Latitude.scale = 100000;

    lastGPSInfo_Longitude.value = 1264598480;
    lastGPSInfo_Longitude.scale = 100000;

    lastGPSInfo_UTC_Hour = 13;
    lastGPSInfo_UTC_Min = 32;
    lastGPSInfo_UTC_Sec = 55;
    lastGPSInfo_UTC_Mi = 775;
    */
    //================== 테스트 데이터

    //Clear Screen
	//memset(gStatusLine,  0, sizeof(gStatusLine));
	memset(gFrameBuffer, 0, sizeof(gFrameBuffer));

    //if ((millis10() - lastGPSInfo_Time) < 20)
        memmove(gFrameBuffer + 0, BITMAP_GPS, sizeof(BITMAP_GPS));

    //APRS
    sprintf(strbuff, lastAPRSsend_Time < 100 ? "SEND" : "APRS");
    UI_PrintStringSmallNormal(strbuff, 7, 44, 0);
    
    //Draw Time 
    sprintf(strbuff, "UTC %02d:%02d:%02d", lastGPSInfo_UTC_Hour, lastGPSInfo_UTC_Min, lastGPSInfo_UTC_Sec);
	UI_PrintStringSmallNormal(strbuff, 44, 127, 0);

    //Draw APRS CHECK
    sprintf(strbuff, "%3u.%05u", lastGPSInfo_Latitude.value / 100 / lastGPSInfo_Latitude.scale, (lastGPSInfo_Latitude.value /100) % lastGPSInfo_Latitude.scale);
    UI_DisplayFrequency(strbuff, 5, 1, false);
    UI_PrintString("'N", 112, 128, 1, 8);

    sprintf(strbuff, "%3u.%05u", lastGPSInfo_Longitude.value / 100 / lastGPSInfo_Longitude.scale, (lastGPSInfo_Longitude.value /100) % lastGPSInfo_Longitude.scale);
    UI_DisplayFrequency(strbuff, 5, 3, false);
    UI_PrintString("'E", 112, 128, 3, 8);

    UI_DrawLineBuffer(gFrameBuffer, 42, 0, 42, 8, true);    
    UI_DrawLineBuffer(gFrameBuffer, 43, 0, 43, 8, true);    
    UI_DrawLineBuffer(gFrameBuffer, 0, 8, 127, 8, true);    
    UI_DrawLineBuffer(gFrameBuffer, 0, 40, 127, 40, true);    

    //Draw GPS Position
   	//sprintf(strbuff, "%u, %u", chkValue, voice_amp);
   	//GUI_DisplaySmallest(strbuff, 0, 1, false, true);
	//ST7565_FillScreen(0xFF);
	//UI_PrintString(strbuff, 2, 127, 0, 8);

    //sprintf(strbuff, "Rcv:%03u By KD8CEC", receiveCount);
    //UI_PrintStringSmallNormal(strbuff, 0, 127, 5);

    //Left Time
    int leftTime = aprsInteral - (millis10() - lastAPRSsend_Time);
    //남은시간 20초중 5초면 128 - 
    //interval을 200으로 만들려면 ?
    //aprsInterval
    // leftTime /   (248 / 128)
    //  120 / 2 = 60
    leftTime = 128 - (leftTime / (aprsInteral / 128));
    UI_DrawLineBuffer(gFrameBuffer, 0, 43, leftTime, 43, true);    
    UI_DrawLineBuffer(gFrameBuffer, 0, 44, leftTime, 44, true);    
    UI_DrawLineBuffer(gFrameBuffer, 0, 45, leftTime, 45 , true);    

    sprintf(strbuff, "APRS:%3u.%03u [%u]", APRS_Freq / 100000, (APRS_Freq / 100) % 1000, receiveCount);
    UI_PrintStringSmallNormal(strbuff, 0, 127, 6);

    //Redraw Screen
    ST7565_BlitFullScreen();
}

void GPSInfoScreen()
{
    while (1)
    {
        DrawGPSInfo(0);  //테스트, 나중에는 스케쥴러에 의해서 그려야됨

        // scan the hardware keys
        KEY_Code_t Key = KEYBOARD_Poll();

        if (Key == KEY_EXIT)
        {
            break;
        }
        else if (Key == KEY_PTT)
        {
            //Send APRS
            CheckAPRSSendTime(1);
        }
        else if (Key == KEY_1)
        {
            //Send APRS
            if (BACKLIGHT_IsOn())
                BACKLIGHT_TurnOff();
            else
                BACKLIGHT_TurnOn();
        }


        SYSTEM_DelayMs(300);
    }

		RADIO_SelectVfos();

#ifdef ENABLE_NOAA
		RADIO_ConfigureNOAA();
#endif
		RADIO_SetupRegisters(true);
  //BackLightBlink(1);
}

void CheckAPRSSendTime(uint8_t nowSendAprs)
{
    if (nowSendAprs == 1 || (millis10() - lastAPRSsend_Time > aprsInteral))
    {

        if (lastGPSInfo_Latitude.value  != 0)
        {
            //send_packet(1);    
            //SYSTEM_DelayMs(1000);
            //send_packet(2);    
            //SYSTEM_DelayMs(1000);
            CEC_APRS_SEND(3);    
            //SYSTEM_DelayMs(1000);
            //send_packet(4);    
            SYSTEM_DelayMs(500);
            CEC_APRS_SEND(5);    
            //SYSTEM_DelayMs(1000);

        }
        lastAPRSsend_Time = millis10();
    }
}

void GPSReceveHandler(uint8_t *gpsBuff, int len)
{
	//자세한건 여기를 참고하고 필요한 좌표와 시간만 구한다.
	//https://github.com/kosma/minmea/blob/master/example.c
	//char strBuff[128];
#ifdef	GPS_SIMULATE
static int gpsSimulateIndex = 0;
		gpsBuff = valid_sentences_checksum[gpsSimulateIndex++];
		if (gpsSimulateIndex > 50)
			gpsSimulateIndex = 0;
#endif		
		
	int newgpsFrameID = minmea_sentence_id(gpsBuff, false);

	if (newgpsFrameID == MINMEA_SENTENCE_RMC)
	{
		struct minmea_sentence_rmc frame;
		if (minmea_parse_rmc(&frame, gpsBuff)) 
		{
            lastGPSInfo_Latitude.value = frame.latitude.value;
            lastGPSInfo_Latitude.scale = frame.latitude.scale;

            lastGPSInfo_Longitude.value = frame.longitude.value;
            lastGPSInfo_Longitude.scale = frame.longitude.scale;

            lastGPSInfo_UTC_Hour = frame.time.hours;
            lastGPSInfo_UTC_Min = frame.time.minutes;
            lastGPSInfo_UTC_Sec = frame.time.seconds;
            lastGPSInfo_UTC_Mi = frame.time.microseconds;
            lastGPSInfo_Time = millis10();

            CheckAPRSSendTime(0);
		}
	}
	else if (newgpsFrameID == MINMEA_SENTENCE_GGA)
	{
		struct minmea_sentence_gga frame;
		if (minmea_parse_gga(&frame, gpsBuff)) 
		{
            lastGPSInfo_Latitude.value = frame.latitude.value;
            lastGPSInfo_Latitude.scale = frame.latitude.scale;

            lastGPSInfo_Longitude.value = frame.longitude.value;
            lastGPSInfo_Longitude.scale = frame.longitude.scale;

            lastGPSInfo_UTC_Hour = frame.time.hours;
            lastGPSInfo_UTC_Min = frame.time.minutes;
            lastGPSInfo_UTC_Sec = frame.time.seconds;
            lastGPSInfo_UTC_Mi = frame.time.microseconds;

			//sprintf(strBuff, "$GGA:%d(%d), %d(%d), q:%d, %d:%d:%d", frame.latitude.value, frame.latitude.scale, frame.longitude.value, frame.longitude.scale, frame.fix_quality, frame.time.hours, frame.time.minutes, frame.time.seconds);
            //DrawGPSInfo(0);  //테스트, 나중에는 스케쥴러에 의해서 그려야됨
            if (receiveCount++ > 999)
                receiveCount = 1;

            lastGPSInfo_Time = millis10();

			//WriteLogInt(strBuff, len, 0, 0);
            CheckAPRSSendTime(0);
		}
	}
}