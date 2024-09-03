#include <cvidef.h>
#include <ansi_c.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>

////////////////////////////
/////Define Commandes///////
#define Request_to_List_Sensors 0x01
#define Request_Sensor_Data 0x02
#define Start_Data_Stream 0x03
#define Stop_Data_Stream 0x04
#define List_of_Sensors_Response 0xA1
#define Sensor_Data_Response 0xA2
#define Data_Stream_Response 0xA3
////////////////////////////////
/////Define Comunication protocal//////
#define Baud_Rate 256000
#define Parity 0
#define Data_Bits 8
#define Stop_Bits 1
#define SEND_BUFFER_SIZE 32767
#define MAX_WAIT_TIME 1000 //maximun wait if the massage didnt get /r in mile sec
///////////////////////////


