#ifndef CEC_DIGITAL_H
#define CEC_DIGITAL_H



#define CEC_DIGIT_CAT_HEADER_SIZE 4

#define CEC_PROTOCOL_CMD_TYPE   4
#define CEC_PROTOCOL_CMD_LENGTH 5
#define CEC_PROTOCOL_DAATA1     7
#define CEC_PROTOCOL_DAATA2     8
#define CEC_PROTOCOL_DAATA3     9
#define CEC_PROTOCOL_DAATA4    10
#define CEC_PROTOCOL_DAATA5    11
#define CEC_PROTOCOL_PAYLOAD   16

//================ MAIN COMMAND TYPE (CMD_TYPE : OFFSET 4)

#define CEC_COMMAND 0x21
//STX 4,  DATA 8 (TYPE, LEN1, LEN2, DATA1, DATA2, DATA3, DATA4, DATA5)    ETX 4

//================ SUB COMMAND =================
#define CEC_CMD_REQMODE         0x01	//Request FT8 or APRS
#define CEC_CMD_RESMODE         0x02	//Request FT8 or APRS

//PROCESS
//K5 -> DSP (START)    DSP->K5 (SYNC... with Display)  K5 -> DSP (SET TIME, 0, 15, 30 ...)
#define CEC_CMD_TIMESTART       0x03	//TIME SYNC START (UV-5K) then, Display Screen with 0x04 send
#define CEC_CMD_TIMESYNC        0x04    //TIME SYNC WAIT FOR FT8 (DSP -> UV-5K)
//#define CEC_CMD_TIMESET       0x05    //TIME SET FOR FT8 (UV-5K -> DSP) Keypress
//CEC_CMD_READYTIMESET
#define CEC_CMD_BACKLIGHT       0x10    //BACK Light Control
#define CEC_CMD_READEEPROM      0x15    //READEEPROM

#define CEC_CMD_FT8READY        0xA0    //FT8 READY MODE
#define CEC_CMD_FT8RECV         0xA1    //FT8 READY MODE
#define CEC_CMD_FT8DECODED      0xA2    //FT8 READY MODE

#define CEC_CMD_REBOOT          0xB1

#define CEC_CMD_KEY_0  200
#define CEC_CMD_KEY_1  201




#endif