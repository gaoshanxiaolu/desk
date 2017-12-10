//
/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef _WX_HEART_PACK_H_
#define _WX_HEART_PACK_H_

struct MAGNET_DATA
{
	char signature[6];
	char MACAddress[6];
	int  xValue;
	int  yValue;
	int  zValue;
	int  ID;
};

#define SEND_BUFFER_SIZE 28
#define RECV_BUFFER_SIZE 24
#define MAX_QUEUE_LEN (2)

typedef struct
{
	A_CHAR frame[30];
	A_UINT16 len;
}FRAME_FORMAT;

typedef struct
{
	FRAME_FORMAT frames[MAX_QUEUE_LEN];
	A_UINT8 read_index;
	A_UINT8 write_index;
	A_UINT8 is_init;
}FRAME_QUEUE;


A_INT32 start_desk_socket_app(A_INT32 argc, A_CHAR *argv[]);
A_INT32 desk_test(A_INT32 argc, A_CHAR *argv[]);

#endif

