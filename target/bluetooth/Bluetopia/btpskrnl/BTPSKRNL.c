/*****< btpskrnl.c >***********************************************************/
/*      Copyright 2000 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTPSKRNL - Stonestreet One Bluetooth Stack Kernel Implementation for      */
/*             Thread-X.                                                      */
/*                                                                            */
/*  Author:  Tim Thomas                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   03/14/01  T. Thomas      Initial creation.                               */
/******************************************************************************/
#include "qcom/qcom_mem.h"
#include "qcom/qcom_utils.h"
#include "tx_api.h"        /* API Kernel Prototypes/Constants for ThreadX     */

#include "BTPSKRNL.h"      /* BTPS Kernel Prototypes/Constants.               */
#include "BTTypes.h"       /* BTPS internal data types.                       */
#include "sprintf.h"

   /* The following constant represents the number of bytes that are    */
   /* displayed on an individual line when using the DumpData()         */
   /* function.                                                         */
#define MAXIMUM_BYTES_PER_ROW                           (16)

   /* Defines the maximum number of event bit masks supported           */
   /* simultaneously by this module.                                    */
#define MAXIMUM_EVENT_MASK_COUNT                        (31)

#define TICKS_PER_SECOND                                (1000)

   /* The following MACRO maps Timeouts (specified in Milliseconds) to  */
   /* System Ticks that are required by the Operating System Timeout    */
   /* functions (Waiting only).                                         */
#define MILLISECONDS_TO_TICKS(_x)                       ((_x * TICKS_PER_SECOND) / 1000)

   /* Denotes the priority of the thread being created using the thread */
   /* create function.                                                  */
#define DEFAULT_THREAD_PRIORITY                         (16)
#define DEFAULT_THREAD_TIME_SLICE                       (4)

   /* The following structure is a container structure that exists to   */
   /* map the OS Thread Function to the BTPS Kernel Thread Function (it */
   /* doesn't totally map in a one to one relationship/mapping).        */
typedef struct _tagThreadWrapperInfo_t
{
   TX_THREAD      Thread;
   char           ThreadName[8];
   Thread_t       ThreadFunction;
   void          *ThreadParameter;
   unsigned char  ThreadStack[1];
} ThreadWrapperInfo_t;

   /* The following MACRO determines the size of an instance of a       */
   /* ThreadWrapperInfo_t structure given the size of the ThreadStack.  */
#define THREAD_WRAPPER_INFO_SIZE(_x)    (BTPS_STRUCTURE_OFFSET(ThreadWrapperInfo_t, ThreadStack) + (_x))

   /* The following type declaration represents the entire state        */
   /* information for a Mailbox.  This structure is used with all of the*/
   /* Mailbox functions contained in this module.                       */
typedef struct _tagMailboxHeader_t
{
   Event_t       Event;
   Mutex_t       Mutex;
   unsigned int  HeadSlot;
   unsigned int  TailSlot;
   unsigned int  OccupiedSlots;
   unsigned int  NumberSlots;
   unsigned int  SlotSize;
   void         *Slots;
} MailboxHeader_t;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */
static unsigned long                DebugZoneMask;

   /* The following variables are used for event handling.  They include*/
   /* a variable to hold the Event Flags Group Handle, the currently    */
   /* used Event Flags Group Mask, and a mutex to guard access.         */
static Mutex_t                      EventMutex;
static TX_EVENT_FLAGS_GROUP         EventFlagsGroup;
static unsigned long                EventFlagsGroupMask;

   /* Internal Function Prototypes.                                     */
static void ThreadWrapper(DWord_t UserData);
static void TheadExitNotification(TX_THREAD *Thread, unsigned int Status);

   /* The following function wraps the thread created with              */
   /* BTPS_CreateThread to be compatible with ThreadX.  This function   */
   /* calls the function for the newly created thread and frees the     */
   /* memory used by the Thread info structure before returning.        */
static void ThreadWrapper(ULONG UserData)
{
   ThreadWrapperInfo_t *ThreadWrapperInfo;

   if(UserData)
   {
      ThreadWrapperInfo = (ThreadWrapperInfo_t *)UserData;

      (*(ThreadWrapperInfo->ThreadFunction))(ThreadWrapperInfo->ThreadParameter);

      /* Register for a notification when the thread terminates.        */
      tx_thread_entry_exit_notify(&(ThreadWrapperInfo->Thread), TheadExitNotification);
   }
}

   /* The following function is called when a thread craeated by        */
   /* BTPS_CreateThread().  is terminated.  It will free the memory     */
   /* allocated for the thread.                                         */
static void TheadExitNotification(TX_THREAD *Thread, unsigned int Status)
{
   /* Confirm the parameters and status indicates the thread has        */
   /* terminated.                                                       */
   if((Thread) && (Status == TX_THREAD_EXIT))
   {
      tx_thread_delete(Thread);

      /* Free the memory for the thread.                                */
      BTPS_FreeMemory((ThreadWrapperInfo_t *)(Thread->tx_thread_entry_parameter));
   }
}

   /* The following function is responsible for delaying the current    */
   /* task for the specified duration (specified in Milliseconds).      */
   /* * NOTE * Very small timeouts might be smaller in granularity than */
   /*          the system can support !!!!                              */
void BTPSAPI BTPS_Delay(unsigned long MilliSeconds)
{
   ULONG Ticks;

   if((Ticks = MILLISECONDS_TO_TICKS(MilliSeconds)) == 0)
      Ticks = 1;

   tx_thread_sleep(Ticks);
}

   /* The following function is responsible for creating an actual      */
   /* Mutex (Binary Semaphore).  The Mutex is unique in that if a       */
   /* Thread already owns the Mutex, and it requests the Mutex again    */
   /* it will be granted the Mutex.  This is in Stark contrast to a     */
   /* Semaphore that will block waiting for the second acquisition of   */
   /* the Semaphore.  This function accepts as input whether or not     */
   /* the Mutex is initially Signalled or not.  If this input parameter */
   /* is TRUE then the caller owns the Mutex and any other threads      */
   /* waiting on the Mutex will block.  This function returns a NON-NULL*/
   /* Mutex Handle if the Mutex was successfully created, or a NULL     */
   /* Mutex Handle if the Mutex was NOT created.  If a Mutex is         */
   /* successfully created, it can only be destroyed by calling the     */
   /* BTPS_CloseMutex() function (and passing the returned Mutex        */
   /* Handle).                                                          */
Mutex_t BTPSAPI BTPS_CreateMutex(Boolean_t CreateOwned)
{
   Mutex_t ret_val;

   /* Before proceeding allocate memory for the mutex header and verify */
   /* that the allocate was successful.                                 */
   if((ret_val = (Mutex_t)BTPS_AllocateMemory(sizeof(TX_MUTEX))) != NULL)
   {
      if(tx_mutex_create((TX_MUTEX *)ret_val, "BT Mutex", TX_INHERIT) == TX_SUCCESS)
      {
         /* If the Mutex needs to created owned, try to take it now.    */
         if((CreateOwned) && (!(BTPS_WaitMutex(ret_val, 0))))
         {
            /* Failed to obtain on the Mutex.                           */
            BTPS_FreeMemory(ret_val);
            ret_val = NULL;
         }
      }
      else
      {
         BTPS_FreeMemory(ret_val);
         ret_val = NULL;
      }
   }
   else
      ret_val = NULL;

   if(!ret_val)
      DBG_MSG(DBG_ZONE_BTPSKRNL,("CreateMutex Error\n"));

   return((Mutex_t)ret_val);
}

   /* The following function is responsible for waiting for the         */
   /* specified Mutex to become free.  This function accepts as input   */
   /* the Mutex Handle to wait for, and the Timeout (specified in       */
   /* Milliseconds) to wait for the Mutex to become available.  This    */
   /* function returns TRUE if the Mutex was successfully acquired and  */
   /* FALSE if either there was an error OR the Mutex was not           */
   /* acquired in the specified Timeout.  It should be noted that       */
   /* Mutexes have the special property that if the calling Thread      */
   /* already owns the Mutex and it requests access to the Mutex again  */
   /* (by calling this function and specifying the same Mutex Handle)   */
   /* then it will automatically be granted the Mutex.  Once a Mutex    */
   /* has been granted successfully (this function returns TRUE), then  */
   /* the caller MUST call the BTPS_ReleaseMutex() function.            */
   /* * NOTE * There must exist a corresponding BTPS_ReleaseMutex()     */
   /*          function call for EVERY successful BTPS_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
Boolean_t BTPSAPI BTPS_WaitMutex(Mutex_t Mutex, unsigned long Timeout)
{
   ULONG     WaitTime;
   Boolean_t ret_val;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(Mutex)
   {
      /* The parameters appear to be semi-valid.  Next determine the    */
      /* amount of time to wait.                                        */
      if(Timeout == BTPS_INFINITE_WAIT)
         WaitTime = TX_WAIT_FOREVER;
      else
         WaitTime = MILLISECONDS_TO_TICKS(Timeout);

      /* Simply call the os supplied wait function.                     */
      if(tx_mutex_get((TX_MUTEX *)Mutex, WaitTime) == TX_SUCCESS)
         ret_val = TRUE;
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   return(ret_val);
}

   /* The following function is responsible for releasing a Mutex that  */
   /* was successfully acquired with the BTPS_WaitMutex() function.     */
   /* This function accepts as input the Mutex that is currently        */
   /* owned.                                                            */
   /* * NOTE * There must exist a corresponding BTPS_ReleaseMutex()     */
   /*          function call for EVERY successful BTPS_WaitMutex()      */
   /*          function call or a deadlock will occur in the system !!! */
void BTPSAPI BTPS_ReleaseMutex(Mutex_t Mutex)
{
   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(Mutex)
   {
      /* The parameters appear to be semi-valid.  Simply attempt to use */
      /* the os supplied release function.                              */
      if(tx_mutex_put((TX_MUTEX *)Mutex) != TX_SUCCESS)
         DBG_MSG(DBG_ZONE_BTPSKRNL,("Release Mutex error\n"));
   }
}

   /* The following function is responsible for destroying a Mutex that */
   /* was created successfully via a successful call to the             */
   /* BTPS_CreateMutex() function.  This function accepts as input the  */
   /* Mutex Handle of the Mutex to destroy.  Once this function is      */
   /* completed the Mutex Handle is NO longer valid and CANNOT be       */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitMutex() functions to fail with an error.                 */
void BTPSAPI BTPS_CloseMutex(Mutex_t Mutex)
{
   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if(Mutex)
   {
      /* Attempt to take the Mutex before deleting it.                  */
      BTPS_WaitMutex(Mutex, 1000);

      /* The parameters appear to be semi-valid.  Now attempt to cleanup*/
      /* the resource associated with this mutex.                       */
      if(tx_mutex_delete((TX_MUTEX *)Mutex) == TX_SUCCESS)
         BTPS_FreeMemory(Mutex);
      else
         DBG_MSG(DBG_ZONE_BTPSKRNL,("Close Mutex error\n"));
   }
}

   /* The following function is responsible for creating an actual      */
   /* Event.  The Event is unique in that it only has two states.  These*/
   /* states are Signalled and Non-Signalled.  Functions are provided   */
   /* to allow the setting of the current state and to allow the        */
   /* option of waiting for an Event to become Signalled.  This function*/
   /* accepts as input whether or not the Event is initially Signalled  */
   /* or not.  If this input parameter is TRUE then the state of the    */
   /* Event is Signalled and any BTPS_WaitEvent() function calls will   */
   /* immediately return.  This function returns a NON-NULL Event       */
   /* Handle if the Event was successfully created, or a NULL Event     */
   /* Handle if the Event was NOT created.  If an Event is successfully */
   /* created, it can only be destroyed by calling the BTPS_CloseEvent()*/
   /* function (and passing the returned Event Handle).                 */
Event_t BTPSAPI BTPS_CreateEvent(Boolean_t CreateSignalled)
{
   Event_t       ret_val;
   unsigned long EventMask;

   if(!EventMutex)
   {
      /* The kernel mutex has not yet been created so initialize the    */
      /* kernel.                                                        */
      if((EventMutex = BTPS_CreateMutex(FALSE)) != NULL)
      {
         if(tx_event_flags_create(&EventFlagsGroup, "BTPS Events") != TX_SUCCESS)
         {
            /* Failed to initialized the event flags so close the mutex.*/
            /* This will also cause this function to return an error.   */
            BTPS_CloseMutex(EventMutex);
            EventMutex = NULL;
         }
      }
   }

   /* Make sure the events were initialized successfully and aquire the */
   /* event mutex.                                                      */
   if((EventMutex) && (BTPS_WaitMutex(EventMutex, BTPS_INFINITE_WAIT)))
   {

      /* Determine the next available event mask.                       */
      EventMask = 1;
      ret_val   = NULL;

      /* Loop until an available event mask is found or the entire mask */
      /* has been checked.                                              */
      while((EventMask) && (!ret_val))
      {
         if(EventFlagsGroupMask & EventMask)
            EventMask <<= 1;
         else
            ret_val = (Event_t)EventMask;
      }

      /* If a free event was found, make sure that it is in the correct */
      /* initial state.                                                 */
      if(ret_val)
      {
         /* Flag that the event is in use.                              */
         EventFlagsGroupMask |= (unsigned long)ret_val;

         if(CreateSignalled)
            BTPS_SetEvent(ret_val);
         else
            BTPS_ResetEvent(ret_val);
      }

     BTPS_ReleaseMutex(EventMutex);
   }
   else
      ret_val = NULL;

   if(!ret_val)
      DBG_MSG(DBG_ZONE_BTPSKRNL,("Create Event error\n"));

   return(ret_val);
}

   /* The following function is responsible for waiting for the         */
   /* specified Event to become Signalled.  This function accepts as    */
   /* input the Event Handle to wait for, and the Timeout (specified    */
   /* in Milliseconds) to wait for the Event to become Signalled.  This */
   /* function returns TRUE if the Event was set to the Signalled       */
   /* State (in the Timeout specified) or FALSE if either there was an  */
   /* error OR the Event was not set to the Signalled State in the      */
   /* specified Timeout.  It should be noted that Signals have a        */
   /* special property in that multiple Threads can be waiting for the  */
   /* Event to become Signalled and ALL calls to BTPS_WaitEvent() will  */
   /* return TRUE whenever the state of the Event becomes Signalled.    */
Boolean_t BTPSAPI BTPS_WaitEvent(Event_t Event, unsigned long Timeout)
{
   Boolean_t ret_val;
   ULONG     WaitTime;
   ULONG     ActualMask;

   /* Verify that the parameter passed in appears valid.                */
   if(Event)
   {
      /* At least one bit is set in the bit mask to wait for.  Next     */
      /* determine the amount of time to wait.                          */
      if(Timeout == BTPS_INFINITE_WAIT)
      {
         WaitTime = TX_WAIT_FOREVER;
      }
      else
      {
         WaitTime = MILLISECONDS_TO_TICKS(Timeout);
      }

      /* Now attempt to wait for one of the events to be set.           */
      if(tx_event_flags_get(&EventFlagsGroup, (ULONG)Event, TX_AND, &ActualMask, WaitTime) == TX_SUCCESS)
      {
         /* Check to see if this event was signaled.                 */
         ret_val = (Boolean_t)((ActualMask & (ULONG)Event) != 0);
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   return(ret_val);
}

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Non-Signalled State.  Once the Event   */
   /* is in this State, ALL calls to the BTPS_WaitEvent() function will */
   /* block until the State of the Event is set to the Signalled State. */
   /* This function accepts as input the Event Handle of the Event to   */
   /* set to the Non-Signalled State.                                   */
void BTPSAPI BTPS_ResetEvent(Event_t Event)
{
   /* Reset the event at the specified location in the event flags      */
   /* group.                                                            */
   tx_event_flags_set(&EventFlagsGroup, ~((ULONG)Event), TX_AND);
}

   /* The following function is responsible for changing the state of   */
   /* the specified Event to the Signalled State.  Once the Event is in */
   /* this State, ALL calls to the BTPS_WaitEvent() function will       */
   /* return.  This function accepts as input the Event Handle of the   */
   /* Event to set to the Signalled State.                              */
void BTPSAPI BTPS_SetEvent(Event_t Event)
{
   /* Set the event at the specified location in the event flags group. */
   tx_event_flags_set(&EventFlagsGroup, ((ULONG)Event), TX_OR);
}

   /* The following function is responsible for destroying an Event that*/
   /* was created successfully via a successful call to the             */
   /* BTPS_CreateEvent() function.  This function accepts as input the  */
   /* Event Handle of the Event to destroy.  Once this function is      */
   /* completed the Event Handle is NO longer valid and CANNOT be       */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitEvent() functions to fail with an error.                 */
void BTPSAPI BTPS_CloseEvent(Event_t Event)
{
   /* Set the event to force any outstanding waiting threads to return. */
   BTPS_SetEvent(Event);

   if(BTPS_WaitMutex(EventMutex, BTPS_INFINITE_WAIT))
   {
     /* Clear the flag signifying the event is in use.                    */
      EventFlagsGroupMask &= ~((unsigned long)Event);

      BTPS_ReleaseMutex(EventMutex);
   }
}

   /* The following function is provided to allow a mechanism to        */
   /* actually allocate a Block of Memory (of at least the specified    */
   /* size).  This function accepts as input the size (in Bytes) of the */
   /* Block of Memory to be allocated.  This function returns a NON-NULL*/
   /* pointer to this Memory Buffer if the Memory was successfully      */
   /* allocated, or a NULL value if the memory could not be allocated.  */
void *BTPSAPI BTPS_AllocateMemory(unsigned long MemorySize)
{
   void *ret_val;

   ret_val = qcom_mem_alloc(MemorySize);

   if(!ret_val)
      DBG_MSG(DBG_ZONE_BTPSKRNL, ("Malloc Failed: %d.\r\n", MemorySize));

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for de-allocating a Block   */
   /* of Memory that was successfully allocated with the                */
   /* BTPS_AllocateMemory() function.  This function accepts a NON-NULL */
   /* Memory Pointer which was returned from the BTPS_AllocateMemory()  */
   /* function.  After this function completes the caller CANNOT use    */
   /* ANY of the Memory pointed to by the Memory Pointer.               */
void BTPSAPI BTPS_FreeMemory(void *MemoryPointer)
{
   if(MemoryPointer)
      qcom_mem_free(MemoryPointer);
}

   /* The following function is responsible for copying a block of      */
   /* memory of the specified size from the specified source pointer    */
   /* to the specified destination memory pointer.  This function       */
   /* accepts as input a pointer to the memory block that is to be      */
   /* Destination Buffer (first parameter), a pointer to memory block   */
   /* that points to the data to be copied into the destination buffer, */
   /* and the size (in bytes) of the Data to copy.  The Source and      */
   /* Destination Memory Buffers must contain AT LEAST as many bytes    */
   /* as specified by the Size parameter.                               */
   /* * NOTE * This function does not allow the overlapping of the      */
   /*          Source and Destination Buffers !!!!                      */
void BTPSAPI BTPS_MemCopy(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcpy() function.                     */
   memcpy(Destination, Source, Size);
}

   /* The following function is responsible for moving a block of       */
   /* memory of the specified size from the specified source pointer    */
   /* to the specified destination memory pointer.  This function       */
   /* accepts as input a pointer to the memory block that is to be      */
   /* Destination Buffer (first parameter), a pointer to memory block   */
   /* that points to the data to be copied into the destination buffer, */
   /* and the size (in bytes) of the Data to copy.  The Source and      */
   /* Destination Memory Buffers must contain AT LEAST as many bytes    */
   /* as specified by the Size parameter.                               */
   /* * NOTE * This function DOES allow the overlapping of the          */
   /*          Source and Destination Buffers.                          */
void BTPSAPI BTPS_MemMove(void *Destination, BTPSCONST void *Source, unsigned long Size)
{
   /* Simply wrap the C Run-Time memmove() function.                    */
   memmove(Destination, Source, Size);
}

   /* The following function is provided to allow a mechanism to fill   */
   /* a block of memory with the specified value.  This function accepts*/
   /* as input a pointer to the Data Buffer (first parameter) that is   */
   /* to filled with the specified value (second parameter).  The       */
   /* final parameter to this function specifies the number of bytes    */
   /* that are to be filled in the Data Buffer.  The Destination        */
   /* Buffer must point to a Buffer that is AT LEAST the size of        */
   /* the Size parameter.                                               */
void BTPSAPI BTPS_MemInitialize(void *Destination, unsigned char Value, unsigned long Size)
{
   /* Simply wrap the C Run-Time memset() function.                     */
   memset(Destination, Value, Size);
}

   /* The following function is provided to allow a mechanism to        */
   /* Compare two blocks of memory to see if the two memory blocks      */
   /* (each of size Size (in bytes)) are equal (each and every byte up  */
   /* to Size bytes).  This function returns a negative number if       */
   /* Source1 is less than Source2, zero if Source1 equals Source2, and */
   /* a positive value if Source1 is greater than Source2.              */
int BTPSAPI BTPS_MemCompare(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   /* Simply wrap the C Run-Time memcmp() function.                     */
   return(memcmp(Source1, Source2, Size));
}

   /* The following function is provided to allow a mechanism to Compare*/
   /* two blocks of memory to see if the two memory blocks (each of size*/
   /* Size (in bytes)) are equal (each and every byte up to Size bytes) */
   /* using a Case-Insensitive Compare.  This function returns a        */
   /* negative number if Source1 is less than Source2, zero if Source1  */
   /* equals Source2, and a positive value if Source1 is greater than   */
   /* Source2.                                                          */
int BTPSAPI BTPS_MemCompareI(BTPSCONST void *Source1, BTPSCONST void *Source2, unsigned long Size)
{
   int           ret_val = 0;
   unsigned char Byte1;
   unsigned char Byte2;
   unsigned int  Index;

   /* Simply loop through each byte pointed to by each pointer and check*/
   /* to see if they are equal.                                         */
   for(Index = 0; ((Index<Size) && (!ret_val)); Index++)
   {
      /* Note each Byte that we are going to compare.                   */
      Byte1 = ((unsigned char *)Source1)[Index];
      Byte2 = ((unsigned char *)Source2)[Index];

      /* If the Byte in the first array is lower case, go ahead and make*/
      /* it upper case (for comparisons below).                         */
      if((Byte1 >= 'a') && (Byte1 <= 'z'))
         Byte1 = Byte1 - ('a' - 'A');

      /* If the Byte in the second array is lower case, go ahead and    */
      /* make it upper case (for comparisons below).                    */
      if((Byte2 >= 'a') && (Byte2 <= 'z'))
         Byte2 = Byte2 - ('a' - 'A');

      /* If the two Bytes are equal then there is nothing to do.        */
      if(Byte1 != Byte2)
      {
         /* Bytes are not equal, so set the return value accordingly.   */
         if(Byte1 < Byte2)
            ret_val = -1;
         else
            ret_val = 1;
      }
   }

   /* Simply return the result of the above comparison(s).              */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism to        */
   /* copy a source NULL Terminated ASCII (character) String to the     */
   /* specified Destination String Buffer.  This function accepts as    */
   /* input a pointer to a buffer (Destination) that is to receive the  */
   /* NULL Terminated ASCII String pointed to by the Source parameter.  */
   /* This function copies the string byte by byte from the Source      */
   /* to the Destination (including the NULL terminator).               */
void BTPSAPI BTPS_StringCopy(char *Destination, BTPSCONST char *Source)
{
   /* Simply wrap the C Run-Time strcpy() function.                     */
   strcpy(Destination, Source);
}

   /* The following function is provided to allow a mechanism to        */
   /* determine the Length (in characters) of the specified NULL        */
   /* Terminated ASCII (character) String.  This function accepts as    */
   /* input a pointer to a NULL Terminated ASCII String and returns     */
   /* the number of characters present in the string (NOT including     */
   /* the terminating NULL character).                                  */
unsigned int BTPSAPI BTPS_StringLength(BTPSCONST char *Source)
{
   /* Simply wrap the C Run-Time strlen() function.                     */
   return(strlen(Source));
}

   /* The following function is provided to allow a mechanism for a C   */
   /* Run-Time Library sprintf() function implementation.  This function*/
   /* accepts as its imput the output buffer, a format string and a     */
   /* variable number of arguments determined by the format string.     */
int BTPSAPI BTPS_SprintF(char *Buffer, BTPSCONST char *Format, ...)
{
   int     ret_val;
   va_list args;

   va_start(args, Format);
   ret_val = vSprintF(Buffer, Format, args);
   va_end(args);

   return(ret_val);
}

   /* The following function is provided to allow a means for the       */
   /* programmer to create a separate thread of execution.  This        */
   /* function accepts as input the Function that represents the        */
   /* Thread that is to be installed into the system as its first       */
   /* parameter.  The second parameter is the size of the Threads       */
   /* Stack (in bytes) required by the Thread when it is executing.     */
   /* The final parameter to this function represents a parameter that  */
   /* is to be passed to the Thread when it is created.  This function  */
   /* returns a NON-NULL Thread Handle if the Thread was successfully   */
   /* created, or a NULL Thread Handle if the Thread was unable to be   */
   /* created.  Once the thread is created, the only way for the Thread */
   /* to be removed from the system is for the Thread function to run   */
   /* to completion.                                                    */
   /* * NOTE * There does NOT exist a function to Kill a Thread that    */
   /*          is present in the system.  Because of this, other means  */
   /*          needs to be devised in order to signal the Thread that   */
   /*          it is to terminate.                                      */
ThreadHandle_t BTPSAPI BTPS_CreateThread(Thread_t ThreadFunction, unsigned int StackSize, void *ThreadParameter)
{
   static int           ThreadCount = 0;
   ThreadHandle_t       ret_val;
   ThreadWrapperInfo_t *ThreadWrapperInfo;

   /* Wrap the OS Thread Creation function.                             */
   /* * NOTE * Becuase the OS Thread function and the BTPS Thread       */
   /*          function are not necessarily the same, we will wrap the  */
   /*          BTPS Thread within the real OS thread.                   */
   if(ThreadFunction)
   {
      /* First we need to allocate memory for a ThreadWrapperInfo_t     */
      /* structure to hold the data we are going to pass to the thread. */
      if((ThreadWrapperInfo = (ThreadWrapperInfo_t *)BTPS_AllocateMemory(THREAD_WRAPPER_INFO_SIZE((StackSize)))) != NULL)
      {
         /* Populate the structure with the correct information.        */
         ThreadCount++;
         SprintF(ThreadWrapperInfo->ThreadName, "BT%d", ThreadCount);

         ThreadWrapperInfo->ThreadFunction  = ThreadFunction;
         ThreadWrapperInfo->ThreadParameter = ThreadParameter;

         /* Next attempt to create a thread using the default priority. */
         if(tx_thread_create(&(ThreadWrapperInfo->Thread), ThreadWrapperInfo->ThreadName, ThreadWrapper, (DWord_t)ThreadWrapperInfo, ThreadWrapperInfo->ThreadStack, StackSize, DEFAULT_THREAD_PRIORITY, DEFAULT_THREAD_PRIORITY, DEFAULT_THREAD_TIME_SLICE, TX_AUTO_START) == TX_SUCCESS)
            ret_val = (Thread_t)(&(ThreadWrapperInfo->Thread));
         else
         {
            /* Failed to create the thread.                             */
            BTPS_FreeMemory(ThreadWrapperInfo);
            ret_val = NULL;
         }
      }
      else
         ret_val = NULL;
   }
   else
      ret_val = NULL;

   if(!ret_val)
      DBG_MSG(DBG_ZONE_BTPSKRNL,("Failed to create thread.\r\n"));

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a mechanism to        */
   /* retrieve the handle of the thread which is currently executing.   */
   /* This function require no input parameters and will return a valid */
   /* ThreadHandle upon success.                                        */
ThreadHandle_t BTPSAPI BTPS_CurrentThreadHandle(void)
{
   ThreadHandle_t ret_val;

   /* Simply return the Current Thread Handle that is executing.        */
   ret_val = (ThreadHandle_t)tx_thread_identify();

  return(ret_val);
}

   /* The following function is provided to allow a mechanism to create */
   /* a Mailbox.  A Mailbox is a Data Store that contains slots (all of */
   /* the same size) that can have data placed into (and retrieved      */
   /* from).  Once Data is placed into a Mailbox (via the               */
   /* BTPS_AddMailbox() function, it can be retrieved by using the      */
   /* BTPS_WaitMailbox() function.  Data placed into the Mailbox is     */
   /* retrieved in a FIFO method.  This function accepts as input the   */
   /* Maximum Number of Slots that will be present in the Mailbox and   */
   /* the Size of each of the Slots.  This function returns a NON-NULL  */
   /* Mailbox Handle if the Mailbox is successfully created, or a NULL  */
   /* Mailbox Handle if the Mailbox was unable to be created.           */
Mailbox_t BTPSAPI BTPS_CreateMailbox(unsigned int NumberSlots, unsigned int SlotSize)
{
   Mailbox_t        ret_val;
   MailboxHeader_t *MailboxHeader;

   /* Before proceeding any further we need to make sure that the       */
   /* parameters that were passed to us appear semi-valid.              */
   if((NumberSlots) && (SlotSize))
   {
      /* Parameters appear semi-valid, so now let's allocate enough     */
      /* Memory to hold the Mailbox Header AND enough space to hold all */
      /* requested Mailbox Slots.                                       */
      if((MailboxHeader = (MailboxHeader_t *)BTPS_AllocateMemory(sizeof(MailboxHeader_t)+(NumberSlots*SlotSize))) != NULL)
      {
         /* Memory successfully allocated so now let's create an Event  */
         /* that will be used to signal when there is Data present in   */
         /* the Mailbox.                                                */
         if((MailboxHeader->Event = BTPS_CreateEvent(FALSE)) != NULL)
         {
            /* Event created successfully, now let's create a Mutex that*/
            /* will guard access to the Mailbox Slots.                  */
            if((MailboxHeader->Mutex = BTPS_CreateMutex(FALSE)) != NULL)
            {
               /* Everything successfully created, now let's initialize */
               /* the state of the Mailbox such that it contains NO     */
               /* Data.                                                 */
               MailboxHeader->NumberSlots   = NumberSlots;
               MailboxHeader->SlotSize      = SlotSize;
               MailboxHeader->HeadSlot      = 0;
               MailboxHeader->TailSlot      = 0;
               MailboxHeader->OccupiedSlots = 0;
               MailboxHeader->Slots         = ((unsigned char *)MailboxHeader) + sizeof(MailboxHeader_t);

               /* All finished, return success to the caller (the       */
               /* Mailbox Header).                                      */
               ret_val                      = (Mailbox_t)MailboxHeader;
            }
            else
            {
               /* Error creating the Mutex, so let's free the Event we  */
               /* created, Free the Memory for the Mailbox itself and   */
               /* return an error to the caller.                        */
               BTPS_CloseEvent(MailboxHeader->Event);

               BTPS_FreeMemory(MailboxHeader);

               ret_val = NULL;
            }
         }
         else
         {
            /* Error creating the Data Available Event, so let's free   */
            /* the Memory for the Mailbox itself and return an error to */
            /* the caller.                                              */
            BTPS_FreeMemory(MailboxHeader);

            ret_val = NULL;
         }
      }
      else
         ret_val = NULL;
   }
   else
      ret_val = NULL;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a means to Add data to*/
   /* the Mailbox (where it can be retrieved via the BTPS_WaitMailbox() */
   /* function.  This function accepts as input the Mailbox Handle of   */
   /* the Mailbox to place the data into and a pointer to a buffer that */
   /* contains the data to be added.  This pointer *MUST* point to a    */
   /* data buffer that is AT LEAST the Size of the Slots in the Mailbox */
   /* (specified when the Mailbox was created) and this pointer CANNOT  */
   /* be NULL.  The data that the MailboxData pointer points to is      */
   /* placed into the Mailbox where it can be retrieved via the         */
   /* BTPS_WaitMailbox() function.                                      */
   /* * NOTE * This function copies from the MailboxData Pointer the    */
   /*          first SlotSize Bytes.  The SlotSize was specified when   */
   /*          the Mailbox was created via a successful call to the     */
   /*          BTPS_CreateMailbox() function.                           */
Boolean_t BTPSAPI BTPS_AddMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Since we are going to change the Mailbox we need to acquire the*/
      /* Mutex that is guarding access to it.                           */
      if(BTPS_WaitMutex(((MailboxHeader_t *)Mailbox)->Mutex, BTPS_INFINITE_WAIT))
      {
         /* Before adding the data to the Mailbox, make sure that the   */
         /* Mailbox is not already full.                                */
         if(((MailboxHeader_t *)Mailbox)->OccupiedSlots < ((MailboxHeader_t *)Mailbox)->NumberSlots)
         {
            /* Mailbox is NOT full, so add the specified User Data to   */
            /* the next available free Mailbox Slot.                    */
            BTPS_MemCopy(&(((unsigned char *)(((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->HeadSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), MailboxData, ((MailboxHeader_t *)Mailbox)->SlotSize);

            /* Update the Next available Free Mailbox Slot (taking into */
            /* account wrapping the pointer).                           */
            if(++(((MailboxHeader_t *)Mailbox)->HeadSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
               ((MailboxHeader_t *)Mailbox)->HeadSlot = 0;

            /* Update the Number of occupied slots to signify that there*/
            /* was additional Mailbox Data added to the Mailbox.        */
            ((MailboxHeader_t *)Mailbox)->OccupiedSlots++;

            /* Signal the Event that specifies that there is indeed Data*/
            /* present in the Mailbox.                                  */
            BTPS_SetEvent(((MailboxHeader_t *)Mailbox)->Event);

            /* Finally, return success to the caller.                   */
            ret_val = TRUE;
         }
         else
            ret_val = FALSE;

         /* All finished with the Mailbox, so release the Mailbox       */
         /* Mutex that we currently hold.                               */
         BTPS_ReleaseMutex(((MailboxHeader_t *)Mailbox)->Mutex);
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is provided to allow a means to retrieve   */
   /* data from the specified Mailbox.  This function will block until  */
   /* either Data is placed in the Mailbox or an error with the Mailbox */
   /* was detected.  This function accepts as its first parameter a     */
   /* Mailbox Handle that represents the Mailbox to wait for the data   */
   /* with.  This function accepts as its second parameter, a pointer to*/
   /* a data buffer that is AT LEAST the size of a single Slot of the   */
   /* Mailbox (specified when the BTPS_CreateMailbox() function was     */
   /* called).  The MailboxData parameter CANNOT be NULL.  This function*/
   /* will return TRUE if data was successfully retrieved from the      */
   /* Mailbox or FALSE if there was an error retrieving data from the   */
   /* Mailbox.  If this function returns TRUE then the first SlotSize   */
   /* bytes of the MailboxData pointer will contain the data that was   */
   /* retrieved from the Mailbox.                                       */
   /* * NOTE * This function copies to the MailboxData Pointer the data */
   /*          that is present in the Mailbox Slot (of size SlotSize).  */
   /*          The SlotSize was specified when the Mailbox was created  */
   /*          via a successful call to the BTPS_CreateMailbox()        */
   /*          function.                                                */
Boolean_t BTPSAPI BTPS_WaitMailbox(Mailbox_t Mailbox, void *MailboxData)
{
   Boolean_t ret_val;

   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* and the MailboxData pointer that was specified appears semi-valid.*/
   if((Mailbox) && (MailboxData))
   {
      /* Next, we need to block until there is at least one Mailbox Slot*/
      /* occupied by waiting on the Data Available Event.               */
      if(BTPS_WaitEvent(((MailboxHeader_t *)Mailbox)->Event, BTPS_INFINITE_WAIT))
      {
         /* Since we are going to update the Mailbox, we need to acquire*/
         /* the Mutex that guards access to the Mailox.                 */
         if(BTPS_WaitMutex(((MailboxHeader_t *)Mailbox)->Mutex, BTPS_INFINITE_WAIT))
         {
            /* Let's double check to make sure that there does indeed   */
            /* exist at least one slot with Mailbox Data present in it. */
            if(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
            {
               /* Flag success to the caller.                           */
               ret_val = TRUE;

               /* Now copy the Data into the Memory Buffer specified by */
               /* the caller.                                           */
               BTPS_MemCopy(MailboxData, &((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]), ((MailboxHeader_t *)Mailbox)->SlotSize);

               /* Now that we've copied the data into the Memory Buffer */
               /* specified by the caller we need to mark the Mailbox   */
               /* Slot as free.                                         */
               if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
                  ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

               ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;

               /* If there is NO more data available in this Mailbox    */
               /* then we need to flag it as such by Resetting the      */
               /* Mailbox Data available Event.                         */
               if(!((MailboxHeader_t *)Mailbox)->OccupiedSlots)
                  BTPS_ResetEvent(((MailboxHeader_t *)Mailbox)->Event);
            }
            else
            {
               /* Internal error, flag that there is NO Data present in */
               /* the Mailbox (by resetting the Data Available Event)   */
               /* and return failure to the caller.                     */
               BTPS_ResetEvent(((MailboxHeader_t *)Mailbox)->Event);

               ret_val = FALSE;
            }

            /* All finished with the Mailbox, so release the Mailbox    */
            /* Mutex that we currently hold.                            */
            BTPS_ReleaseMutex(((MailboxHeader_t *)Mailbox)->Mutex);
         }
         else
            ret_val = FALSE;
      }
      else
         ret_val = FALSE;
   }
   else
      ret_val = FALSE;

   /* Return the result to the caller.                                  */
   return(ret_val);
}

   /* The following function is responsible for destroying a Mailbox    */
   /* that was created successfully via a successful call to the        */
   /* BTPS_CreateMailbox() function.  This function accepts as input the*/
   /* Mailbox Handle of the Mailbox to destroy.  Once this function is  */
   /* completed the Mailbox Handle is NO longer valid and CANNOT be     */
   /* used.  Calling this function will cause all outstanding           */
   /* BTPS_WaitMailbox() functions to fail with an error.  The final    */
   /* parameter specifies an (optional) callback function that is called*/
   /* for each queued Mailbox entry.  This allows a mechanism to free   */
   /* any resources that might be associated with each individual       */
   /* Mailbox item.                                                     */
void BTPSAPI BTPS_DeleteMailbox(Mailbox_t Mailbox, BTPS_MailboxDeleteCallback_t MailboxDeleteCallback)
{
   /* Before proceeding any further make sure that the Mailbox Handle   */
   /* that was specified appears semi-valid.                            */
   if(Mailbox)
   {
      /* Mailbox appears semi-valid, so let's free all Events and       */
      /* Mutexes, perform any mailbox deletion callback actions, and    */
      /* finally free the Memory associated with the Mailbox itself.    */
      if(((MailboxHeader_t *)Mailbox)->Event)
         BTPS_CloseEvent(((MailboxHeader_t *)Mailbox)->Event);

      if(((MailboxHeader_t *)Mailbox)->Mutex)
         BTPS_CloseMutex(((MailboxHeader_t *)Mailbox)->Mutex);

      /* Check to see if a Mailbox Delete Item Callback was specified.  */
      if(MailboxDeleteCallback)
      {
         /* Now loop though all of the occupied slots and call the      */
         /* callback with the slot data.                                */
         while(((MailboxHeader_t *)Mailbox)->OccupiedSlots)
         {
            __BTPSTRY
            {
               (*MailboxDeleteCallback)(&((((unsigned char *)((MailboxHeader_t *)Mailbox)->Slots))[((MailboxHeader_t *)Mailbox)->TailSlot*((MailboxHeader_t *)Mailbox)->SlotSize]));
            }
            __BTPSEXCEPT(1)
            {
               /* Do Nothing.                                           */
            }

            /* Now that we've called back with the data, we need to     */
            /* advance to the next slot.                                */
            if(++(((MailboxHeader_t *)Mailbox)->TailSlot) == ((MailboxHeader_t *)Mailbox)->NumberSlots)
               ((MailboxHeader_t *)Mailbox)->TailSlot = 0;

            /* Flag that there is one less occupied slot.               */
            ((MailboxHeader_t *)Mailbox)->OccupiedSlots--;
         }
      }

      /* All finished cleaning up the Mailbox, simply free all memory   */
      /* that was allocated for the Mailbox.                            */
      BTPS_FreeMemory(Mailbox);
   }
}

   /* The following function is used to initialize the Platform module. */
   /* The Platform module relies on some static variables that are used */
   /* to coordinate the abstraction.  When the module is initially      */
   /* started from a cold boot, all variables are set to the proper     */
   /* state.  If the Warm Boot is required, then these variables need to*/
   /* be reset to their default values.  This function sets all static  */
   /* parameters to their default values.                               */
   /* * NOTE * The implementation is free to pass whatever information  */
   /*          required in this parameter.                              */
void BTPSAPI BTPS_Init(void *UserParam)
{
   /* Initialize the static variables for this module.                  */
   DebugZoneMask            = DEBUG_ZONES;

   DBG_MSG(DBG_ZONE_BTPSKRNL,("BTPS Initialized\n"));
}

   /* The following function is used to cleanup the Platform module.    */
void BTPSAPI BTPS_DeInit(void)
{
}

static char DebugMsgBuffer[256];

   /* Write out the specified NULL terminated Debugging String to the   */
   /* Debug output.                                                     */
void BTPSAPI BTPS_OutputMessage(BTPSCONST char *DebugString, ...)
{
   va_list args;

   va_start(args, DebugString);
   vSprintF(DebugMsgBuffer, DebugString, args);
   qcom_printf(DebugMsgBuffer);
   va_end(args);
}

   /* The following function is used to set the Debug Mask that controls*/
   /* which debug zone messages get displayed.  The function takes as   */
   /* its only parameter the Debug Mask value that is to be used.  Each */
   /* bit in the mask corresponds to a debug zone.  When a bit is set,  */
   /* the printing of that debug zone is enabled.                       */
void BTPSAPI BTPS_SetDebugMask(unsigned long DebugMask)
{
   DebugZoneMask = DebugMask;
}

   /* The following function is a utility function that can be used to  */
   /* determine if a specified Zone is currently enabled for debugging. */
int BTPSAPI BTPS_TestDebugZone(unsigned long Zone)
{
   return(DebugZoneMask & Zone);
}

   /* The following function is responsible for displaying binary debug */
   /* data.  The first parameter to this function is the length of data */
   /* pointed to by the next parameter.  The final parameter is a       */
   /* pointer to the binary data to be  displayed.                      */
int BTPSAPI BTPS_DumpData(unsigned int DataLength, BTPSCONST unsigned char *DataPtr)
{
   int                    ret_val;
   char                   Buffer[80];
   char                  *BufPtr;
   char                  *HexBufPtr;
   Byte_t                 DataByte;
   unsigned int           Index;
   static BTPSCONST char  HexTable[] = "0123456789ABCDEF\r\n";
   static BTPSCONST char  Header1[]  = "       00 01 02 03 04 05 06 07 08 09 0A 0B 0C 0D 0E 0F  ";
   static BTPSCONST char  Header2[]  = " ------------------------------------------------------\r\n";

   /* Before proceeding any further, lets make sure that the parameters */
   /* passed to us appear semi-valid.                                   */
   if((DataLength > 0) && (DataPtr != NULL))
   {
      /* The parameters which were passed in appear to be at least      */
      /* semi-valid, next write the header out to the file.             */
      BTPS_OutputMessage((char *)Header1);
      BTPS_OutputMessage((char *)HexTable);
      BTPS_OutputMessage((char *)Header2);

      /* Limit the number of bytes that will be displayed in the dump.  */
      if(DataLength > MAX_DBG_DUMP_BYTES)
      {
         DataLength = MAX_DBG_DUMP_BYTES;
      }

      /* Now that the Header is written out, let's output the Debug     */
      /* Data.                                                          */
      BTPS_MemInitialize(Buffer, ' ', sizeof(Buffer));
      BufPtr    = Buffer + BTPS_SprintF(Buffer," %05X ", 0);
      HexBufPtr = Buffer + sizeof(Header1)-1;
      for(Index = 1; Index <= DataLength; Index ++)
      {
         DataByte     = *DataPtr++;
         *BufPtr++    = HexTable[(Byte_t)(DataByte >> 4)];
         *BufPtr      = HexTable[(Byte_t)(DataByte & 0x0F)];
         BufPtr      += 2;

         /* Check to see if this is a printable character and not a     */
         /* special reserved character.  Replace the non-printable      */
         /* characters with a period.                                   */
         *HexBufPtr++ = (char)(((DataByte >= ' ') && (DataByte <= '~') && (DataByte != '\\') && (DataByte != '%'))?DataByte:'.');
         if(((Index % MAXIMUM_BYTES_PER_ROW) == 0) || (Index == DataLength))
         {
            /* We are at the end of this line, so terminate the data    */
            /* compiled in the buffer and send it to the output device. */
            *HexBufPtr++ = '\r';
            *HexBufPtr++ = '\n';
            *HexBufPtr   = 0;
            BTPS_OutputMessage(Buffer);

            if(Index != DataLength)
            {
               /* We have more to process, so prepare for the next line.*/
               BTPS_MemInitialize(Buffer, ' ', sizeof(Buffer));
               BufPtr    = Buffer + BTPS_SprintF(Buffer," %05X ", Index);
               HexBufPtr = Buffer + sizeof(Header1)-1;
            }
            else
            {
               /* Flag that there is no more data.                      */
               HexBufPtr = NULL;
            }
         }
      }

      /* Check to see if there is partial data in the buffer.           */
      if(HexBufPtr)
      {
         /* Terminate the buffer and output the line.                   */
         *HexBufPtr++ = '\r';
         *HexBufPtr++ = '\n';
         *HexBufPtr   = 0;
         BTPS_OutputMessage(Buffer);
      }

      BTPS_OutputMessage("\r\n");

      /* Finally, set the return value to indicate success.             */
      ret_val = 0;
   }
   else
   {
      /* An invalid parameter was enterer.                              */
      ret_val = -1;
   }

   return(ret_val);
}
