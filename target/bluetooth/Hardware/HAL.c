/*
  * Copyright (c) 2014 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */
#include "HAL.h"
#include "BTPSKRNL.h"
#include "qcom_common.h"
#include "qcom_uart.h"
#include "select_api.h"

   /* The following defines are used as utilities to validate           */
   /* characters.                                                       */
#define IsDecChar(_x)               (((_x) >= '0') && ((_x) <= '9'))
#define IsHexChar(_x)               ((((_x) >= '0') && ((_x) <= '9')) || ((((_x) | 0x20) >= 'a') && (((_x) | 0x20) <= 'f')))
#define ToInt(_x)                   (((_x) > 0x39)?(((_x) & 0x0F)+9):(((_x) & 0x0F)))

   /* The following function locates a match of the specified characeter*/
   /* in the character string supplied.  The function returns TRUE if   */
   /* the character is located within the Allowed string.               */
A_BOOL ValidInputCh(A_CHAR ch, A_CHAR *Allowed)
{
   A_INT32 ret_val;

   /* Scan the string looking for a match.                              */
   ret_val = 0;
   while(Allowed[ret_val])
   {
      /* Check for a match.                                             */
      if(Allowed[ret_val] == ch)
         break;

      /* Advance to the next character in the string.                   */
      ret_val++;
   }

   /* Return TRUE if non-NULL.                                          */
   return((A_BOOL)Allowed[ret_val]);
}

   /* The following function is used to get input from the used.  The   */
   /* function allow the input of a Hex string, Decimal string or ASCII */
   /* string.  The buffer passed in must be pre-initialized with a      */
   /* default string.  The user may backspace to change the default     */
   /* value and can only exit the routine when a <CR> or <LF> is        */
   /* received while at the end of the string.  The string is fixed an  */
   /* is defined by buf_len.  The type of input allowed is defined by   */
   /* the Type parameter.  The function returns a pointer to a NULL     */
   /* terminated string on success and a NULL pointer on failure.       */
static A_CHAR *FGetInput(A_CHAR *buf, A_UINT32 buf_len, A_UINT32 Type, A_CHAR *Allowed, A_INT32 fd)
{
   A_INT32   i;
   A_CHAR    ch;
   A_UINT32  len;
   A_CHAR   *ret_val;

   /* Verify that the parameters passed in appear valid.                */
   if((buf) && (buf_len > 1) && (fd >= 0) && ((Type != CUSTOM_INPUT) || ((Type == CUSTOM_INPUT) && (Allowed))))
   {
      /* Make sure that the string is NULL terminated.                  */
      buf[buf_len--] = 0;
      len = buf_len;

      /* Display the default input.                                     */
      qcom_uart_write(fd, buf, &len);

      /* Process any input characters.                                  */
      i = buf_len;
      while(i <= buf_len)
      {
         /* Read the next character from the stream.                    */
         ch = (char)Fgetc(fd);

         /* Check to see if the character is a <CR> or <LF>.            */
         if((ch == '\r') || (ch == '\n'))
         {
            if(i == buf_len)
            {
               len = 2;
               qcom_uart_write(fd, "\r\n", &len);
               break;
            }
            else
               continue;
         }

         /* See if the character is a <BS> and we have at least 1       */
         /* character in the input buffer.                              */
         if((ch == '\b') && (i > 0))
         {
            /* Back the input buffer by 1 and terminate the string.     */
            i--;
            buf[i] = 0;

            /* Erase the character from the display.                    */
            len    = 3;
            qcom_uart_write(fd, (char *)"\b \b", &len);
            continue;
         }

         /* Don't go past the end of the buffer.                        */
         if(i < buf_len)
         {
            if((Type == HEX_INPUT) && (!IsHexChar(ch)))
               continue;
            if((Type == DEC_INPUT) && (!IsDecChar(ch)))
               continue;
            if((Type == CUSTOM_INPUT) && (!ValidInputCh(ch, Allowed)))
               continue;

            /* Add the character to the input buffer.                */
            buf[i++] = ch;
            len      = 1;
            qcom_uart_write(fd, &ch, &len);
         }
      }

      /* Return a pointer to the input buffer.                          */
      ret_val = buf;
   }
   else
      ret_val = NULL;

   return(ret_val);
}

   /* The following is an implementation of 'fgets'.  The function takes*/
   /* as its input a file descriptor that identifies the input stream.  */
   /* The function returns a positive number on success and a negative  */
   /* number on failure.                                                */
A_INT32 Fgetc(A_INT32 fd)
{
   A_INT32  ch = -1;
   A_INT32  ret_val;
   q_fd_set fd_set;
   A_UINT32 len;

   /* Verify that the File Descriptor appears valid.                    */
   if(fd >= 0)
   {
      /* Read 1 character from the input stream.                        */
      FD_ZERO(&fd_set);
      FD_SET(fd, &fd_set);
      ret_val = qcom_select(1, &fd_set, NULL, NULL, NULL);
      /* Verify that we broke on input.                                 */
      if((ret_val) && FD_ISSET(fd, &fd_set))
      {
         /* Read the character from the input stream.                   */
         len = 1;
         qcom_uart_read(fd, (char *)&ch, &len);
      }
   }

   return(ch);
}

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
A_CHAR *Fgets(A_CHAR *buf, A_UINT32 buf_len, A_INT32 fd)
{
   A_INT32   i;
   A_CHAR    ch;
   A_UINT32  len;
   A_CHAR   *ret_val;

   /* Verify that the parameters passed in appear valid.                */
   if((buf) && (buf_len > 1) && (fd >=0))
   {
      /* Initialize the buffer to zero and reduce the buffer size by 1  */
      /* to account for the NULL terminator.                            */
      memset(buf, 0, buf_len);
      buf_len--;

      /* Process any input characters.                                  */
      i = 0;
      while(i < buf_len)
      {
         /* Read the next character from the stream.                    */
         ch = (char)Fgetc(fd);

         /* Check to see if the character is a <CR> or <LF>.            */
         if((ch == '\r') || (ch == '\n'))
            break;

         /* See if the character is a <BS> and we have at least 1       */
         /* character in the input buffer.                              */
         if((ch == '\b') && (i > 0))
         {
            /* Back the input buffer by 1 and terminate the string.     */
            i--;
            buf[i] = 0;

            /* Erase the character from the display.                    */
            len    = 3;
            qcom_uart_write(fd, (char *)"\b \b", &len);
            continue;
         }

         /* Check to see if the character is a printable character.     */
         if((ch >= ' ') && (ch <= '~'))
         {
            /* Add the character to the input buffer.                   */
            buf[i++] = ch;
            len      = 1;
            qcom_uart_write(fd, &ch, &len);
         }
      }

      /* Return a pointer to the input buffer.                          */
      ret_val = buf;
   }
   else
      ret_val = NULL;

   return(ret_val);
}

   /* The foillowing function is used to bring the Bluetooth controller */
   /* out of reset.                                                     */
void EnableBluetooth(void)
{
   /* Hold Reset Low.                                                   */
   DisableBluetooth();
   BTPS_Delay(10);

   /* Bring the controller out of reset.                                */
   *((unsigned int *)0x14004) = (unsigned int)(1 << 14);
   BTPS_Delay(10);

}

   /* The foillowing function is used to place the Bluetooth controller */
   /* out of reset.                                                     */
void DisableBluetooth(void)
{
   /* Setup BT_RESET GPIO                                               */
   *((unsigned int *)0x14010) = (unsigned int)(1 << 14);

   /* Hold Reset Low.                                                   */
   *((unsigned int *)0x14008) = (unsigned int)(1 << 14);
}

A_INT32 GetUserInput(A_INT32 fd, A_CHAR *Prompt, A_CHAR *buffer, A_UINT32 buf_len, A_UINT32 Type, A_CHAR *Allowed)
{
   A_INT32 ret_val = -1;

   /* Verify that the parameter passed in appears valid.                */
   if((Prompt) && (buffer) && (buf_len) && ((Type != CUSTOM_INPUT) || ((Type == CUSTOM_INPUT) && (Allowed))))
   {
      /* Prompt for the Bluetooth Address Info.                         */
      printf("%s", Prompt);

      /* Let the user update the default.                               */
      if(FGetInput(buffer, buf_len, Type, Allowed, fd))
         ret_val = 0;
   }

   return(ret_val);
}


   /* Prompt the user for the UAP-NAP portion of the Bluetooth Address. */
   /* This function returns the integer value of the data entered.      */
A_INT32 GetBD_ADDR(A_INT32 fd, A_UINT32 *BluetoothAddress)
{
   A_UINT32 ndx;
   A_INT32  ret_val = -1;
   A_CHAR   LAP[7];

   /* Verify that the parameter passed in appears valid.                */
   if(BluetoothAddress)
   {
      /* Initialize the default value.                                  */
      BTPS_SprintF(LAP, "000000");

      ret_val = GetUserInput(fd, "Specify Bluetooth Address: 00025B", LAP, sizeof(LAP), HEX_INPUT, NULL);
      if(!ret_val)
      {
         /* Convert the string to an integer value.                     */
         ndx     = 0;
         while(ndx < 6)
         {
            ret_val = (A_INT32)((ret_val * 16) + ToInt(LAP[ndx]));
            ndx++;
         }

         /* Return the result to the user.                              */
         *BluetoothAddress = ret_val;
         ret_val           = 0;
      }
   }

   return(ret_val);
}


