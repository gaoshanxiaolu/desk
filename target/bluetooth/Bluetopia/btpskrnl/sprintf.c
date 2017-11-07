/*****< printf.c >************************************************************/
/*      Copyright 2000, 2001 Stonestreet One, Inc.                           */
/*      All Rights Reserved.                                                 */
/*                                                                           */
/*  printf - gives printf capabilities out a serial port.                    */
/*                                                                           */
/*  Author:  Tim Thomas                                                      */
/*                                                                           */
/*** MODIFICATION HISTORY ****************************************************/
/*                                                                           */
/*   mm/dd/yy  F. Lastname    Description of Modification                    */
/*   --------  -----------    -----------------------------------------------*/
/*   08/03/10  T. Thomas      Initial creation.                              */
/*****************************************************************************/

#include "sprintf.h"

   /* Flag bit patterns                                                 */
#define FFORMAT   0x0100 /* Saw a Format Flag                           */
#define FZERO     0x0200 /* PAD WITH '0'                                */
#define FPADLEFT  0x0400 /* LEFT PADDING POSSIBILY REQUIRED             */
#define FUNSIGNED 0x0800 /* if writing an unsigned num                  */
#define FLONG     0x1000 /* if writing a long varible                   */
#define FLLONG    0x2000 /* if writing a long long varible              */
#define FUPPER    0x4000 /* Upper case Hex values                       */

#define PF_8         signed char
#define PF_U8        unsigned char
#define PF_16        short
#define PF_U16       unsigned short
#define PF_32        long
#define PF_U32       unsigned long
#define PF_64        long long
#define PF_U64       unsigned long long

static const char HexTable[]  = "0123456789abcdef";
static const char UHexTable[] = "0123456789ABCDEF";


static int NumDigits(unsigned long long val, int Radix);
static unsigned int LtoA(PF_U64  val, PF_8 *buffer, PF_U16 flags);
static unsigned int LtoH(PF_U64  val, PF_8 *buffer, PF_U16 flags);

   /* The following function is used to determine the number of digits  */
   /* that will need to be printed to print specified value.            */
static int NumDigits(unsigned long long val, int Radix)
{
   int ret_val= 1;

   while(val /= Radix)
      ret_val++;

   return(ret_val);
}

   /* The following function is used to determine the number of chars   */
   /* that will need to be printed to print specified value.            */
static int NumChars(char *ptr)
{
   int ret_val = 0;

   while(*ptr++)
      ret_val++;

   return(ret_val);
}

   /* The following function converts a 32 bit value to a string        */
   /* representation of the value.  The function receives the value to  */
   /* be processed as its first parameter.  The second parameter is a   */
   /* pointer to a buffer to receive the formatted string.  The last    */
   /* parameter contains flags that control to format of the string.    */
   /* The function returns the number of characters that were placed in */
   /* the output buffer.                                                */
static unsigned int LtoA(PF_U64 val, PF_8 *bufptr, PF_U16 flags)
{
   unsigned int ret_val;
   int          negative;
   char         fill_char;
   unsigned int fill_cnt;

   /* Check to see if the value is negative.                            */
   negative = 0;

   /* Save the nunmber of padding bytes.                                */
   fill_cnt = (flags & 0xFF);

   /* Check to see if the value is negative                             */
   if((!(flags & FUNSIGNED)) && ((PF_64)val < 0))
   {
      negative = 1;
      val      = (PF_U64)(0-val);
   }

   /* Truncate the value if it is not specified as Long Long.           */
   if(!(flags & FLLONG))
      val &= 0xFFFFFFFF;

   /* If the number of digits required is greater than or equal to the  */
   /* padding, then just adjust the pointer.  If the nmber is negative, */
   /* then add an extra byte to the required size to handle the minus   */
   /* sign.                                                             */
   ret_val = NumDigits(val, 10);
   if(negative)
      ret_val++;

   /* Check to see if there has been any padding specified.             */
   if(ret_val >= fill_cnt)
      bufptr += ret_val;
   else
   {
      /* Fill the output buffer with the requested amount of fill       */
      /* characterrs.                                                   */
      ret_val = fill_cnt;

      fill_char = (char)((flags & FZERO)?'0':' ');
      while(fill_cnt--)
      {
         *(bufptr++) = fill_char;
      }
   }

   /* Make sure string is NULL terminated                               */
   *bufptr = 0;

   /* Build string 'backwards'                                          */
   do
   {
      *(--bufptr) = (char)((val % 10) + '0');
   }
   while(val /= 10);
   if(negative)
      *(--bufptr) = '-';

   return(ret_val);
}

   /* The following function converts a 32 bit value to a Hex string    */
   /* representation of the value.  The function receives the value to  */
   /* be processed as its first parameter.  The second parameter is a   */
   /* pointer to a buffer to receive the formatted string.  The last    */
   /* parameter contains flags that control to format of the string.    */
   /* The function returns the number of characters that were placed in */
   /* the output buffer.                                                */
static unsigned int LtoH(PF_U64 val, PF_8 *bufptr, PF_U16 flags)
{
   unsigned int  ret_val;
   char         *hextable;
   char          fill_char;
   unsigned int  fill_cnt;

   /* Save the nunmber of padding bytes.                                */
   fill_cnt = (flags & 0xFF);

   /* Truncate the value if it is not specified as Long Long.           */
   if(!(flags & FLLONG))
      val &= 0xFFFFFFFF;

   /* Calculate the number of digits that result will produce.          */
   ret_val = NumDigits(val, 16);

   /* If the number of digits required is greater than or equal to the  */
   /* padding, then just adjust the pointer.                            */
   if(ret_val >= fill_cnt)
      bufptr += ret_val;
   else
   {
      ret_val   = fill_cnt;
      fill_char = (char)((flags & FZERO)?'0':' ');
      while(fill_cnt--)
      {
         *(bufptr++) = fill_char;
      }
   }

   /* Make sure string is NULL terminated                               */
   *bufptr = 0;
   bufptr--;

   hextable = (char *)((flags & FUPPER)?UHexTable:HexTable);

   /* Build string 'backwards'                                          */
   do
   {
      *(bufptr--) = hextable[(unsigned char)(val & 0x0F)];
   }
   while(val >>= 4);

   return(ret_val);
}

   /* The following function is used to format an output sting based on */
   /* a provided format.  This function should be called from a function*/
   /* that receives a variable argument list.  The function takes as its*/
   /* first parameter a pointer to a buffer that will receive the       */
   /* formatted output.  The second parameters is a pointer to a string */
   /* that defines the format of the output.  The last parameter is a   */
   /* variable argument list.  The function returns the number if       */
   /* characters that weere placed in the output buffer.                */
int vSprintF(char *buffer, const char *format, va_list ap)
{
   unsigned int       flags;           /* flags that control conversion */
   unsigned long long value;           /* value read                    */
   char               *p;              /* temp char pointer             */
   char               *bufptr;
   char               ch;

   /* go through format until the end                                   */
   flags   = 0;
   bufptr  = buffer;

   /* Verify that the pointer to the output buffer is valid.            */
   if(buffer)
   {
      while((ch = *(format++)))
      {
         /* Check to see if we are working on a format.                 */
         if(!flags)
         {
            /* copy non '%' chars into result string                    */
            if(ch != '%')
            {
               *(bufptr++) = ch;
            }
            else
               flags = FFORMAT;

            /* Continue with the next character.                        */
            continue;
         }

         /* Check to see if the last character was a '%'.               */
         if(flags == FFORMAT)
         {
            /* Must have hit a '%', handle possible '%%'                */
            if(ch == '%')
            {
               /* Clear the format flag.                                */
               flags = 0;

               /* Hit 2 '%'s, print one out                             */
               *(bufptr++) = ch;
               continue;
            }
         }

         /* process flags                                               */
         if(ch == '0')
         {
            flags |= FZERO;
            continue;
         }

         /* Check for a Long or Long Long specifier                     */
         if(ch == 'l')
         {
            if(flags & FLONG)
            {
               flags &= ~FLONG;
               flags |= FLLONG;
            }
            else
               flags |= FLONG;
            continue;
         }

         /* check for possible field width                              */
         if((ch > '1') && (ch <= '9'))
         {
            flags |= (FPADLEFT + (ch & 0x0F));
            continue;
         }

         if(ch == '*')
         {
            value = va_arg(ap, int);
            flags |= (unsigned int)(FPADLEFT + (value & 0xFF));
            continue;
         }

         switch(ch)
         {
            case 'u':
               flags |= FUNSIGNED;
            case 'd':
               if(flags & FLLONG)
                  value = va_arg(ap, long long);
               else
                  value = va_arg(ap, int);
               bufptr += LtoA((PF_64)value, (PF_8 *)bufptr, (PF_16)flags);
               break;
            case 'p':
               flags |= (FZERO + (sizeof(void *) << 1));
            case 'X':
               flags |= FUPPER;
            case 'x':
               if(flags & FLLONG)
                  value = va_arg(ap, long long);
               else
                  value = va_arg(ap, int);
               bufptr += LtoH((PF_U64)value, (PF_8 *)bufptr, (PF_16)flags);
               break;
            case 'c':
               value       = va_arg(ap, int);
               *(bufptr++) = (char)value;
               break;
            case 's':
               p = va_arg(ap, char *);
               if(p)
               {
                  if(flags & FPADLEFT)
                  {
                     ch = (char)((flags & 0xFF) - NumChars(p));
                     while((unsigned int)ch--)
                        *(bufptr++) = ' ';
                  }
                  while(*p)
                    *(bufptr++) = *(p++);
               }
               break;
         }
         flags = 0;
      }

      /* Null Terminate the string.                                     */
      *bufptr = 0x00;
   }

   return(int)(bufptr-buffer);
}

   /* The following function formats a string and places the formatted  */
   /* string into an output buffer.  The function takes as its first    */
   /* parameter a pointer to the output buffer.  The second parameter is*/
   /* a pointer to a string that represents how to format the data.  The*/
   /* remaining data are the parameters specified by the format of the  */
   /* output string.  The function return the number of characters that */
   /* were placed in the output buffer..                                */
int SprintF(char *buffer, const char *format, ...)
{
   int     ret_val;
   va_list ap;                         /* argument pointer              */

   /* init the variable number argument pointer                         */
   va_start(ap, format);

   ret_val = vSprintF(buffer, format ,ap);

   va_end(ap);

   return(ret_val);
}


