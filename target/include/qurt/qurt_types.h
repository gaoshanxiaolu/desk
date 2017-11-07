#ifndef QURT_TYPES_H
#define QURT_TYPES_H
/*=======================================================================

FILE:         qurt_types.h

DESCRIPTION:  definition of basic types, constants, preprocessor macros


  Copyright (c) 2015-2017 Qualcomm Technologies Incorporated.
  All Rights Reserved.
  Qualcomm Confidential and Proprietary

$Header: //components/rel/core.ioe/1.0/v2/rom/release/api/kernel/qurt/qurt_types.h#3 $

=========================================================================*/

#include "qurt_stddef.h"

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/
/** @addtogroup chapter_data_types
@{ */
#define QURT_TIME_NO_WAIT       0x00000000 /**< Return immediately without
any waiting. */
#define QURT_TIME_WAIT_FOREVER  0xFFFFFFFF /**< Block until the operation is successful. */

/*=============================================================================
                        TYPEDEFS
=============================================================================*/


typedef uint32 qurt_time_t; /**<QuRT time unit types.*/

/* QuRT time unit types.*/
typedef enum {
  QURT_TIME_TICK,                  /**< Return time in ticks. */
  QURT_TIME_MSEC,                  /**< Return time in milliseconds. */
  QURT_TIME_NONE=0x7FFFFFF        /**< Identifier to use if no particular return type is needed */ 
}qurt_time_unit_t;
/** @} */ /* end_addtogroup chapter_data_types */
#endif /* QURT_TYPES_H */
