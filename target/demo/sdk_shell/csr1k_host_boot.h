/*
    NXP LPC210x CSR1000/CSR1001 host SPI boot definitions file

    NOTE:  This code was "seeded" from WINARM and/or Olimex sources.

    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
*/

#ifndef CSR1K_HOST_BOOT_H
#define CSR1K_HOST_BOOT_H
 
#include <stdlib.h>
#include <string.h>
#include "qcom/qcom_common.h"

/* Use of these typedefs might help port this to other platforms */
typedef unsigned int                uint32;
typedef int                         int32;
typedef unsigned short              uint16;
typedef short                       int16;
typedef unsigned char               uint8;
typedef char                        int8;

#define timeDelayInMS(_d)      qcom_thread_msleep(_d)  

extern void CSR1Kboot(void);
void NODE_CSR1Kboot(void);

#endif  /* CSR1K_HOST_BOOT_H */



