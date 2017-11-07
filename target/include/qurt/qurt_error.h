#ifndef QURT_ERROR_H
#define QURT_ERROR_H

/*=======================================================================
  @file qurt_error.h 
  @brief Error results- QuRT defines a set of standard symbols for the error result values. This file lists the
  symbols and their corresponding values.

 EXTERNALIZED FUNCTIONS
  none

 INITIALIZATION AND SEQUENCING REQUIREMENTS
  none

 Copyright (c) 2015 - 2017  by Qualcomm Technologies, Inc.  All Rights Reserved.
 Confidential and Proprietary - Qualcomm Technologies, Inc..
 ======================================================================*/

/** @addtogroup chapter_error
@{ */

/*=====================================================================
Constants and macros
======================================================================*/
#define QURT_EOK                             0  /**< Operation successfully performed. */
#define QURT_EFATAL                         -1  /**< Fatal error, which should never happen. */
#define QURT_EVAL                           -2  /**< Wrong values for parameters; the specified page does not exist. */
#define QURT_EMEM                           -3  /**< Not enough memory to perform the operation.*/

#define QURT_EINVALID                       -4  /**< Invalid argument value, invalid key. */ 
#define QURT_EFAILED_TIMEOUT                -5  /**< Time out. */
#define QURT_EUNKNOWN                       -6  /**< Defined but never used in BLAST. */
#define QURT_EFAILED                        -7  /**< Operation failed. */
#define QURT_ENOMSGS                        -8  /**< Message queue empty. */

#define QURT_ENOTALLOWED                    -9  /**< Operation not allowed. */
/** @} */ /* end_addtogroup chapter_error */
#endif /* QURT_ERROR_H */
