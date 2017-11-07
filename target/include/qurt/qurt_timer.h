#ifndef QURT_TIMER_H
#define QURT_TIMER_H
/**
  @file qurt_timer.h
  @brief  Prototypes of qurt_timer API 
  Qurt Timer APIs allows two different mechanism for user notification on timer
  expiry. signal and callback. A user can choose one of them but not both.

EXTERNAL FUNCTIONS
   None.

INITIALIZATION AND SEQUENCING REQUIREMENTS
   None.

Copyright (c) 2015-2017  by Qualcomm Technologies, Inc.  All Rights Reserved.
Confidential and Proprietary - Qualcomm Technologies, Inc.


=============================================================================*/

#include "qurt_types.h"
#include "qurt_signal.h"

/*=============================================================================
                        CONSTANTS AND MACROS
=============================================================================*/

/** @addtogroup chapter_data_types
@{ */
/**
   QuRT timer options                                                       
 */
#define QURT_TIMER_ONESHOT              0x01   /**< One-shot timer.*/
#define QURT_TIMER_PERIODIC             0x02   /**< Periodic timer.*/
#define QURT_TIMER_NO_AUTO_START        0x04   /**< No auto activate.*/
#define QURT_TIMER_AUTO_START           0x08   /**< Default, auto activate. */
#define QURT_TIMER_OBJ_SIZE_BYTES       128    /**< On QuRT, object size is 40 bytes. */


/*=============================================================================
                        TYPEDEFS
=============================================================================*/

/** QuRT timer types.*/
typedef unsigned long  qurt_timer_t;

/** QuRT timer callback function types. */
typedef void (*qurt_timer_callback_func_t)( void *);
/** @} */ /* end_addtogroup chapter_data_types */
/**@cond */
typedef struct qurt_timer_attr  /* 8 byte aligned. */
{
   unsigned long long _bSpace[QURT_TIMER_OBJ_SIZE_BYTES/sizeof(unsigned long long)];
}qurt_timer_attr_t; /*< QuRT timer attributes types.*/
/**@endcond */


/*=============================================================================
                        FUNCTIONS
=============================================================================*/
/**@ingroup func_qurt_timer_attr_init
  Initializes the timer attribute object.

  @datatypes
  qurt_timer_attr_t

  @param[in,out] attr Pointer to the destination structure for the timer attributes.

  @return
  None.

  @dependencies
  None.
  
 */
void qurt_timer_attr_init(qurt_timer_attr_t *attr);

/**@ingroup func_qurt_timer_attr_set_duration
  Sets the timer duration in the specified timer attribute structure.\n

  The timer duration specifies the interval (in timer ticks) between the activation of the
  timer object and the generation of the corresponding timer event.
  
  @note1hang If the timer is activated during creation, the duration specified is the interval
  (in timer ticks) between the creation and the generation of the corresponding timer event.

  @datatypes
  qurt_timer_attr_t \n
  #qurt_time_t

  @param[in,out] attr    Pointer to the timer attribute structure.
  @param[in] duration    Timer duration (in timer ticks).

  @return
  None.

  @dependencies
  None.
  
 */
void qurt_timer_attr_set_duration(qurt_timer_attr_t *attr, qurt_time_t duration);

/**@ingroup func_qurt_timer_attr_set_callback
  Sets the timer callback function and callback context in the specified timer attribute structure. \n

  The callback function is invoked on timer expiry with callback context passed by the user while creating the timer. 

  @note1hang The application should not make any blocking calls from a callback.

  @datatypes
  qurt_timer_attr_t \n
  #qurt_timer_callback_func_t

  @param[in] attr     Pointer to the timer attribute object.
  @param[in] cbfunc   Pointer to the timer callback function.
  @param[in] cbctxt   Pointer to the timer callback context.

  @return
  None.

  @dependencies
  None.
  
 */
void qurt_timer_attr_set_callback(qurt_timer_attr_t *attr, qurt_timer_callback_func_t cbfunc, void *cbctxt);

/**@ingroup func_qurt_timer_attr_set_signal
  Sets the signal object and mask in the specified timer attribute structure. \n
  Signal masks are set on timer expiry.
  
  @datatypes
  qurt_timer_attr_t \n

  @param[in] attr    Pointer to the timer attribute object. 
  @param[in] signal  Pointer to the signal object. 
  @param[in] mask    Signal mask to be set when the timer expires. 

  @return
  None.

  @dependencies
  None.
  
 */
void qurt_timer_attr_set_signal(qurt_timer_attr_t *attr, qurt_signal_t *signal, uint32 mask);

/**@ingroup func_qurt_timer_attr_set_reload
  Sets the timer reload time in the specified timer attribute structure.\n

  The timer reload specifies the interval (in timer ticks) for all timer 
  expiration after first. A zero value indicates that the timer is a one-shot timer.

  @datatypes
  qurt_timer_attr_t \n
  #qurt_time_t

  @param[in]  attr         Pointer to the timer attribute object.
  @param[out] reload_time  Timer reload (in timer ticks).

  @return
  #QURT_EOK -- Timer set reload success. \n
  #QURT_EINVALID - Invalid argument. \n
  #QURT_EFAILED -- Timer set option failed. \n

  @dependencies
  None.
 */
void qurt_timer_attr_set_reload(qurt_timer_attr_t *attr, qurt_time_t reload_time);

/**@ingroup func_qurt_timer_attr_set_option
  Sets the timer option in the specified timer attribute object. 
  
  The timer type specifies the functional behavior of the timer: \n
  - A one-shot timer (#QURT_TIMER_ONESHOT) waits for the specified timer duration
      and then generates a single timer event. After this the timer is nonfunctional. \n
  - A periodic timer (#QURT_TIMER_PERIODIC) repeatedly waits for the specified
     timer duration and then generates a timer event. The result is a series of timer
     events with interval equal to the timer duration. \n
  - An auto activate timer option (#QURT_TIMER_AUTO_START) activates the timer when it is created. \n
  - A no auto activate timer option (#QURT_TIMER_NO_AUTO_START) creates the timer in a non-active state. \n
  
   @note1hang   QURT_TIMER_ONESHOT and QURT_TIMER_PERIODIC are mutually exclusive.\n
   @note1cont   QURT_TIMER_AUTO_START and QURT_TIMER_NO_AUTO_START are mutually exclusive.\n
   @note1cont   Options can be OR'ed. \n
   
   @datatypes 
   qurt_timer_attr_t \n
   
   @param[in,out]  attr  Pointer to the timer attribute object.
   @param[in]      option  Timer option. Values are: \n
                   - QURT_TIMER_ONESHOT - One time timer \n
                   - QURT_TIMER_PERIODIC - Periodic timer \n
                   - QURT_TIMER_AUTO_START -- Auto enable \n
                   - QURT_TIMER_NO_AUTO_START -- Auto disable @tablebulletend

   @return
   None.

   @dependencies
   None.
   
 */
void qurt_timer_attr_set_option(qurt_timer_attr_t *attr, uint32 option);

/**@ingroup func_qurt_timer_create
  Creates a timer.
 
  A QuRT timer can be created with a signal or a callback as a notification mechanism on timer expiry.\n
  The option is mutually exclusive and defined via attribute APIs. See  
  qurt_timer_attr_set_callback() and qurt_timer_attr_set_signal().
  
  A timer can be started at the time of creation or started explicitly later based on options
  set by users.
  
  @note1hang A timer signal handler must be defined to wait on the specified signal 
             to handle the timer signal.

  @datatypes
  #qurt_timer_t \n
  qurt_timer_attr_t \n

  @param[out] timer   Pointer to the created timer object.
  @param[in]  attr    Pointer to the timer attribute structure.

  @return
  #QURT_EOK -- Success. \n
  #QURT_EFAILED -- Failed to create timer. \n
  
  @dependencies
  None.
  
 */
int qurt_timer_create (qurt_timer_t *timer, const qurt_timer_attr_t *attr); 
                  
/**@ingroup func_qurt_timer_start
  Activates the specified application timer. The timer to be activated should 
  not be active.

  @datatypes
  #qurt_timer_t 

  @param[in] timer  Created timer object.

  @return
  #QURT_EOK -- Timer activated successfully. \n
  #QURT_EFAILED -- Timer activation failed. \n
  #QURT_EINVALID -- Invalid argument. \n
  
  @dependencies
  None.
  
 */
int qurt_timer_start(qurt_timer_t timer);

/**@ingroup func_qurt_timer_restart
  @xreflabel{sec:qurt_timer_restart}
  Restarts a stopped timer with the specified duration.
  The timer must be a one-shot timer.

  @note1hang Timers stop after they have expired or after they are explicitly
             stopped with the timer stop operation, see Section @xref{sec:qurt_timer_stop}.
  
  @note1hang Timer expires after the duration passed as a user parameter from the time this API is
             invoked.
  
  @datatypes
  #qurt_timer_t \n
  #qurt_time_t

  @param[in] timer        Timer object. 
  @param[in] duration     Timer duration (in timer ticks) before the restarted timer
                          expires again.
  @return             
  #QURT_EOK -- Success. \n
  #QURT_EINVALID -- Invalid timer ID or duration value. \n
  #QURT_ENOTALLOWED -- Timer is not a one-shot timer. \n
  #QURT_EMEM --  Out-of-memory error. \n

  @dependencies
  None.
  
 */
int qurt_timer_restart (qurt_timer_t timer, qurt_time_t duration);

/**@ingroup func_qurt_timer_get_attr
  Gets the timer attributes of the specified timer.

  @note1hang After a timer is created, its attributes cannot be changed.

  @datatypes
  #qurt_timer_t \n
  qurt_timer_attr_t

  @param[in] timer  Timer object.
  @param[out] attr  Pointer to the destination structure for timer attributes.

  @return
  #QURT_EOK -- Success. \n
  #QURT_EVAL -- Argument passed is not a valid timer.

  @dependencies
  None.
 */
int qurt_timer_get_attr(qurt_timer_t timer, qurt_timer_attr_t *attr);

/**@ingroup func_qurt_timer_attr_get_duration
  Gets the timer duration from the specified timer attribute structure.
  
  @note1hang A call to the API returns the duration of timer that was
  originally set for timer to expire. This API does not return the remaining time
  of the active timer, see Section @xref{sec:qurt_timer_attr_get_remaining}.
  
  @datatypes
  qurt_timer_attr_t \n
  #qurt_time_t

  @param[in]  attr       Pointer to the timer attributes object
  @param[out] duration   Pointer to the destination variable for timer duration.

  @return
  #QURT_EOK -- Timer get duration success. \n
  #QURT_EINVALID -- Invalid argument. \n
  #QURT_EFAILED -- Timer get duration failed. \n

  @dependencies
  None.
  
 */
int qurt_timer_attr_get_duration(qurt_timer_attr_t *attr, qurt_time_t *duration);

/**@ingroup func_qurt_timer_attr_get_remaining
  @xreflabel{sec:qurt_timer_attr_get_remaining} 
  Gets the timer remaining duration from the specified timer attribute structure. \n

  The timer remaining duration indicates (in timer ticks) how much time remains before
  the generation of the next timer event on the corresponding timer.

  @note1hang This attribute is read-only and thus cannot be set.

  @datatypes
  qurt_timer_attr_t \n
  #qurt_time_t

  @param[in] attr          Pointer to the timer attribute object.
  @param[out] remaining    Pointer to the destination variable for remaining time.

  @return
  #QURT_EOK -- Timer get remaining time success. \n
  #QURT_EINVALID -- Invalid argument. \n
  #QURT_EFAILED -- Timer get duration failed. \n

  @dependencies
  None.
  
 */
int qurt_timer_attr_get_remaining(qurt_timer_attr_t *attr, qurt_time_t *remaining);

/**@ingroup func_qurt_timer_attr_get_option
  Gets the timer option from the specified timer attribute object.

  @datatypes
  qurt_timer_attr_t \n

  @param[in]  attr  Pointer to the timer attribute object.
  @param[out] option  Pointer to the destination variable for the timer option.

  @return
  #QURT_EOK -- Timer get option success. \n
  #QURT_EINVALID -- Invalid argument. \n
  #QURT_EFAILED -- Timer get duration failed. \n

  @dependencies
  None.
 */
int qurt_timer_attr_get_option(qurt_timer_attr_t *attr, uint32  *option);

/**@ingroup func_qurt_timer_attr_get_reload
  Gets the timer reload time from the specified timer attribute structure.\n
  
  The timer reload specifies the interval (in timer ticks) for all timer 
  expiration after first. A zero value indicates that the timer is a one-shot timer.
  
  @datatypes
  qurt_timer_attr_t \n
  #qurt_time_t 
  
  @param[in]  attr  Pointer to the timer attribute object.
  @param[out] reload_time  Timer reload (in timer ticks).

  @return
  #QURT_EOK -- Timer get reload success. \n
  #QURT_EINVALID -- Invalid argument. \n
  #QURT_EFAILED -- Timer get reload failed. \n

  @dependencies
  None.
 */
int qurt_timer_attr_get_reload(qurt_timer_attr_t *attr, qurt_time_t * reload_time);

/**@ingroup func_qurt_timer_delete
  Deletes the timer.\n
  Destroys the specified timer and deallocates the timer object.

  @datatypes
  #qurt_timer_t
  
  @param[in] timer  Timer object.

  @return       
  #QURT_EOK -- Timer delete success. \n
  #QURT_EFAILED -- Timer delete failed. \n
  #QURT_EVAL -- Argument passed is not a valid timer. 

  @dependencies
  None.
  
 */
int qurt_timer_delete(qurt_timer_t timer);

/**@ingroup func_qurt_timer_stop
  @xreflabel{sec:qurt_timer_stop}  
  Stops a running timer.
 
  @note1hang The timer restart/activate operation can restart stopped timers,
             see Section @xref{sec:qurt_timer_restart}. 

  @datatypes
  #qurt_timer_t
  
  @param[in] timer    Timer object. 

  @return
  #QURT_EOK -- Success. \n
  #QURT_EINVALID -- Invalid timer ID or duration value. \n
  #QURT_ENOTALLOWED -- Timer is not a one-shot timer. \n
  #QURT_EMEM -- Out of memory error.

  @dependencies
  None.
  
 */
int qurt_timer_stop (qurt_timer_t timer);

/*   
  Gets the timer remaining duration from the QuRT library timer service.\n
 
  The timer remaining duration indicates (in system ticks) how much time remains before 
  the generation of the next timer event on any active timer in the QuRT application system.
  
  @note1hang This API must only be used when the QuRT application system is running 
  in STM mode with all interrupts disabled. Otherwise, it results in undefined 
  behavior, and can have side effects.
  
  @note1hang Sleep is the primary user of this API. 

  @datatypes
  #qurt_time_t
  
  @return
  Number of system ticks until the next timer event.\n
  #QURT_TIME_WAIT_FOREVER --- No pending timer events.
   
  @dependencies
  None.
  
 */
qurt_time_t qurt_timer_get_remaining ( void );
 
/**@ingroup func_qurt_timer_get_ticks
   Gets current ticks. The ticks are accumulated since the RTOS has started.
   
   @datatypes
   #qurt_time_t

   @return             
   Ticks since the system started.
   
   @dependencies
   None.
   
 */
qurt_time_t qurt_timer_get_ticks (void);

/**@ingroup func_qurt_timer_convert_time_to_ticks   
  Converts to tick count the time value expressed in the specified time units.
  
  If the time in milliseconds is not a multiple of systick duration in milliseconds,
  the API rounds up the return ticks.

  @datatypes
  #qurt_time_t \n
  #qurt_time_unit_t
   
  @param[in] time         Time value to convert. 
  @param[in] time_unit    Time units that value is expressed in.
  
  @return
  Tick count in system ticks -- Success. \n
  #QURT_TIME_WAIT_FOREVER - Conversion failed.
   
  @dependencies
  None.
  
 */
qurt_time_t qurt_timer_convert_time_to_ticks(qurt_time_t time, qurt_time_unit_t time_unit );

/**@ingroup func_qurt_timer_convert_ticks_to_time   
  Converts tick count to the time value expressed in the specified time units.

  @datatypes
  #qurt_time_t \n
  #qurt_time_unit_t
  
  @param[in] ticks        Tick count to convert. 
  @param[in] time_unit    Time units that the return value is expressed in.  
  
  @return
  Time value expressed in the specified time units -- Success. \n
  #QURT_TIME_WAIT_FOREVER - Conversion failed.
   
  @dependencies
  None.
  
 */
qurt_time_t qurt_timer_convert_ticks_to_time(qurt_time_t ticks, qurt_time_unit_t time_unit);

#endif /* QURT_TIMER_H */
