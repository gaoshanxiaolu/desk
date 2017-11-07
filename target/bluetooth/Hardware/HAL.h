/*
  * Copyright (c) 2014 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#ifndef __APP_START__
#define __APP_START__

#include "qcom_common.h"

   /* The following define the type of input that can be allowed when   */
   /* Gtting User INput.                                                */
#define CHAR_INPUT                                             0
#define DEC_INPUT                                              1
#define HEX_INPUT                                              3
#define CUSTOM_INPUT                                           4

   /* The following is an implementation of 'fgets'.  The function takes*/
   /* as its input a file descriptor that identifies the input stream.  */
   /* The function returns a positive number on success and a negative  */
   /* number on failure.                                                */
A_INT32 Fgetc(A_INT32 fd);

   /* The following is an implementation of 'fgets'.  The function takes*/
   /* as its input a pointer to a buffer to receive the data, the size  */
   /* of the buffer and a file descriptor that identifies the input     */
   /* stream.  The function will echo each character and will manage the*/
   /* <BS> characters.  The input will end on a <CR> or <LF> or when the*/
   /* input buffer is full and will NOT include the <CR> or <LF> in the */
   /* returned string.  Other than the mentioned control characters, the*/
   /* function will only accept printable ASCII characters.  The        */
   /* function returns a pointer to a NULL terminated string on success */
   /* and a NULL pointer on failure.                                    */
A_CHAR *Fgets(A_CHAR *buf, A_UINT32 buf_len, A_INT32 fd);

   /* The foillowing function is used to bring the Bluetooth controller */
   /* out of reset.                                                     */
void EnableBluetooth(void);

   /* The foillowing function is used to place the Bluetooth controller */
   /* out of reset.                                                     */
void DisableBluetooth(void);

A_INT32 GetUserInput(A_INT32 fd, A_CHAR *Prompt, A_CHAR *buffer, A_UINT32 buf_len, A_UINT32 Type, A_CHAR *Allowed);

   /* Prompt the user for the UAP-NAP portion of the Bluetooth Address. */
   /* This function returns the integer value of the data entered.      */
A_INT32 GetBD_ADDR(A_INT32 fd, A_UINT32 *BluetoothAddress);

#endif
