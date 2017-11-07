/*****< ndcs.c >***************************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  NDCS - Bluetooth Stack Next DST Change Service (GATT Based) for           */
/*         Stonestreet One Bluetooth Protocol Stack.                          */
/*                                                                            */
/*  Author:  Ajay Parashar                                                    */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   06/28/12  A. Parashar    Initial creation.                               */
/*   06/09/16  R. McCord      Added Suspend/Resume support.                   */
/******************************************************************************/
#include "SS1BTPS.h"        /* Bluetooth Stack API Prototypes/Constants.      */
#include "SS1BTGAT.h"       /* Bluetooth Stack GATT APIPrototypes/Constants.  */
#include "SS1BTNDC.h"       /* Bluetooth NDCS API Prototypes/Constants.       */

#include "BTPSKRNL.h"       /* BTPS Kernel Prototypes/Constants.              */
#include "NDCS.h"           /* Bluetooth NDCS Prototypes/Constants.           */

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   #include "BTPSALIGN.h"   /* Alignment Related Constants.                   */
#endif

   /* The following controls the number of supported NDCS instances.    */
#define NDCS_MAXIMUM_SUPPORTED_INSTANCES                 (BTPS_CONFIGURATION_NDCS_MAXIMUM_SUPPORTED_INSTANCES)

   /* The following structure defines the Instance Data that must be    */
   /* unique for each NDCS service registered (Only 1 per Bluetooth     */
   /* Stack).                                                           */
typedef __PACKED_STRUCT_BEGIN__ struct _tagNDCS_Instance_Data_t
{
  NonAlignedWord_t     Time_With_Dst_Length;
  NDCS_Time_With_Dst_t Time_With_Dst;

} __PACKED_STRUCT_END__ NDCS_Instance_Data_t;

#define NDCS_INSTANCE_DATA_SIZE                          (sizeof(NDCS_Instance_Data_t))

   /* The following define the instance tags for each NDCS service data */
   /* that is unique per registered service.                            */
#define NDCS_NEXT_DST_CHANGE_INSTANCE_TAG                (BTPS_STRUCTURE_OFFSET(NDCS_Instance_Data_t,Time_With_Dst_Length ))

   /*********************************************************************/
   /**               Next DST Change Service Table                      */
   /*********************************************************************/

   /* The Next_DST_Change_Time Service Declaration UUID.                */
static BTPSCONST GATT_Primary_Service_16_Entry_t NDCS_Service_UUID =
{
   NDCS_SERVICE_BLUETOOTH_UUID_CONSTANT
};

   /* The Next_DST_Change_Time Characteristic Declaration.              */
static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t NDCS_Next_DST_Change_Time_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_READ,
   NDCS_TIME_WITH_DST_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Next_DST_Change_Time Characteristic Value.                    */
static BTPSCONST GATT_Characteristic_Value_16_Entry_t NDCS_Next_DST_Change_Time_Value =
{
   NDCS_TIME_WITH_DST_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   NDCS_NEXT_DST_CHANGE_INSTANCE_TAG,
   NULL
};

   /* The following defines the Health Thermometer service that is      */
   /* registered with the GATT_Register_Service function call.          */
   /* * NOTE * This array will be registered with GATT in the call to   */
   /*          GATT_Register_Service.                                   */
BTPSCONST GATT_Service_Attribute_Entry_t Next_DST_Time_Service[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService16,            (Byte_t *)&NDCS_Service_UUID},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration16, (Byte_t *)&NDCS_Next_DST_Change_Time_Declaration},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicValue16,       (Byte_t *)&NDCS_Next_DST_Change_Time_Value},

};

#define NEXT_DST_CHANGE_TIME_SERVICE_ATTRIBUTE_COUNT                      (sizeof(Next_DST_Time_Service)/sizeof(GATT_Service_Attribute_Entry_t))

#define NDSC_TIME_WITH_DST_ATTRIBUTE_OFFSET              (2)

   /*********************************************************************/
   /**                    END OF SERVICE TABLE                         **/
   /*********************************************************************/

   /* The following type defines a union large enough to hold all events*/
   /* dispatched by this module.                                        */
typedef union
{
   NDCS_Read_Time_With_DST_Request_Data_t Read_DST_Time_Request_Data ;
} NDCS_Event_Data_Buffer_t;

#define NDCS_EVENT_DATA_BUFFER_SIZE                      (sizeof(NDCS_Event_Data_Buffer_t))


   /* NDCS Service Instance Block.  This structure contains All         */
   /* information associated with a specific Bluetooth Stack ID (member */
   /* is present in this structure).                                    */
typedef struct _tagNDCSServerInstance_t
{
   unsigned int                  BluetoothStackID;
   unsigned int                  ServiceID;
   NDCS_Event_Callback_t         EventCallback;
   unsigned long                 CallbackParameter;


#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
#endif
} NDCSServerInstance_t;

#define NDCS_SERVER_INSTANCE_DATA_SIZE                    (sizeof(NDCSServerInstance_t))

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   /* The following structure contains the information that is          */
   /* stored/restored in NDCS Service Instance when a suspend/resume     */
   /* procedure is performed.                                           */
typedef struct NDCSSuspendInstanceInfo_s
{
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
   unsigned int                  InstanceID;
   NDCS_Event_Callback_t         EventCallback;
   unsigned long                 CallbackParameter;
   Byte_t                        VariableData[1];
} NDCSSuspendInstanceInfo_t;

#define NDCS_SUSPEND_INSTANCE_INFO_SIZE(_x)              (BTPS_STRUCTURE_OFFSET(NDCSSuspendInstanceInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

   /* The following structure contains the information that is          */
   /* stored/restored in NDCS Service when a suspend/resume procedure is */
   /* performed.                                                        */
typedef struct NDCSSuspendInfo_s
{
   unsigned int NumberOfSuspendedInstances;
   Byte_t       VariableData[1];
} NDCSSuspendInfo_t;

#define NDCS_SUSPEND_INFO_SIZE(_x)                       (BTPS_STRUCTURE_OFFSET(NDCSSuspendInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

#endif

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the compiler*/
   /* as part of standard C/C++).                                       */

static NDCS_Instance_Data_t InstanceData[NDCS_MAXIMUM_SUPPORTED_INSTANCES];
                                            /* Variable which holds all */
                                            /* data that is unique for  */
                                            /* each service instance.   */

static NDCSServerInstance_t InstanceList[NDCS_MAXIMUM_SUPPORTED_INSTANCES];
                                            /* Variable which holds the */
                                            /* service instance data.   */

static Boolean_t InstanceListInitialized;   /* Variable that flags that */
                                            /* is used to denote that   */
                                            /* this module has been     */
                                            /* successfully initialized.*/

   /* The following are the prototypes of local functions.              */
static Boolean_t InitializeModule(void);
static void CleanupModule(void);

static int FormatDSTTime(NDCS_Time_With_Dst_Data_t *DST_Time, unsigned int BufferLength, Byte_t *Buffer);

static Boolean_t InstanceRegisteredByStackID(unsigned int BluetoothStackID);
static NDCSServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID);

static NDCS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, NDCS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, unsigned int *TransactionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR);

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer);
   static int ResumeServiceInstance(unsigned int BluetoothStackID, unsigned long *BufferSize, Byte_t **Buffer);
#endif

static int NDCSRegisterService(unsigned int BluetoothStackID, NDCS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID);

   /* Bluetooth Event Callbacks.                                        */
static void BTPSAPI GATT_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);

   /* The following function is a utility function that is used to      */
   /* reduce the ifdef blocks that are needed to handle the difference  */
   /* between module initialization for Threaded and NonThreaded stacks.*/
static Boolean_t InitializeModule(void)
{
   /* All we need to do is flag that we are initialized.                */
   if(!InstanceListInitialized)
   {
      InstanceListInitialized = TRUE;

      BTPS_MemInitialize(InstanceList, 0, sizeof(InstanceList));
   }

   return(TRUE);
}

   /* The following function is a utility function that exists to       */
   /* perform stack specific (threaded versus nonthreaded) cleanup.     */
static void CleanupModule(void)
{
   /* Flag that we are no longer initialized.                           */
   InstanceListInitialized = FALSE;
}

static int FormatDSTTime(NDCS_Time_With_Dst_Data_t *Next_Dst_Change_Time, unsigned int BufferLength, Byte_t *Buffer)
{
  int ret_val = 0;

  if((Next_Dst_Change_Time) && (BufferLength >= NDCS_TIME_WITH_DST_DATA_SIZE && (Buffer)))
  {
     /* Assign the NextDSTChange time for the specified instance.       */
     ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((NDCS_Time_With_Dst_t*)Buffer)->Date_Time.Year),    Next_Dst_Change_Time->Date_Time.Year);
     ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((NDCS_Time_With_Dst_t*)Buffer)->Date_Time.Month),   Next_Dst_Change_Time->Date_Time.Month);
     ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((NDCS_Time_With_Dst_t*)Buffer)->Date_Time.Day),     Next_Dst_Change_Time->Date_Time.Day);
     ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((NDCS_Time_With_Dst_t*)Buffer)->Date_Time.Hours),   Next_Dst_Change_Time->Date_Time.Hours);
     ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((NDCS_Time_With_Dst_t*)Buffer)->Date_Time.Minutes), Next_Dst_Change_Time->Date_Time.Minutes);
     ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((NDCS_Time_With_Dst_t*)Buffer)->Date_Time.Seconds), Next_Dst_Change_Time->Date_Time.Seconds);

     /* Assign Offset                                                   */
     ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((NDCS_Time_With_Dst_t*)Buffer)->Dst_Offset),        Next_Dst_Change_Time->Dst_Offset);
  }
  else
     ret_val = NDCS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is a utility function that exists to format*/
   /* a NDCS Event into the specified buffer.                           */
   /* * NOTE * TransactionID is optional and may be set to NULL.        */
   /* * NOTE * BD_ADDR is NOT optional and may NOT be set to NULL.      */
static NDCS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, NDCS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, unsigned int *TransactionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR)
{
   NDCS_Event_Data_t *EventData = NULL;

   if((BufferLength >= (NDCS_EVENT_DATA_SIZE + NDCS_EVENT_DATA_BUFFER_SIZE)) && (Buffer) && (BD_ADDR))
   {
      /* Format the header of the event, that is data that is common to */
      /* all events.                                                    */
      BTPS_MemInitialize(Buffer, 0, BufferLength);
      EventData                                                                  =  (NDCS_Event_Data_t *)Buffer;
      EventData->Event_Data_Type                                                 =  EventType;
      EventData->Event_Data.NDCS_Read_Time_With_DST_Request_Data                 =  (NDCS_Read_Time_With_DST_Request_Data_t *)(((Byte_t *)EventData) + NDCS_EVENT_DATA_SIZE);
      EventData->Event_Data.NDCS_Read_Time_With_DST_Request_Data->InstanceID     =  InstanceID;
      EventData->Event_Data.NDCS_Read_Time_With_DST_Request_Data->ConnectionID   =  ConnectionID;
      EventData->Event_Data.NDCS_Read_Time_With_DST_Request_Data->TransactionID  = *TransactionID;
      EventData->Event_Data.NDCS_Read_Time_With_DST_Request_Data->ConnectionType =  ConnectionType;
      EventData->Event_Data.NDCS_Read_Time_With_DST_Request_Data->RemoteDevice   = *BD_ADDR;
   }

   /* Finally return the result to the caller.                          */
   return(EventData);
}

   /* The following function is a utility function that exists to check */
   /* to see if an instance has already been registered for a specified */
   /* Bluetooth Stack ID.                                               */
   /* * NOTE * Since this is an internal function no check is done on   */
   /*          the input parameters.                                    */
static Boolean_t InstanceRegisteredByStackID(unsigned int BluetoothStackID)
{
   Boolean_t    ret_val = FALSE;
   unsigned int Index;

   for(Index=0;Index<NDCS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
   {
      if((InstanceList[Index].BluetoothStackID == BluetoothStackID) && (InstanceList[Index].ServiceID))
      {
         ret_val = TRUE;
         break;
      }
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is a utility function that exists to       */
   /* acquire a specified service instance.                             */
   /* * NOTE * Since this is an internal function no check is done on   */
   /*          the input parameters.                                    */
   /* * NOTE * If InstanceID is set to 0, this function will return the */
   /*          next free instance.                                      */
static NDCSServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID)
{
   unsigned int          LocalInstanceID;
   unsigned int          Index;
   NDCSServerInstance_t *ret_val = NULL;

   /* Lock the Bluetooth Stack to gain exclusive access to this         */
   /* Bluetooth Protocol Stack.                                         */
   if(!BSC_LockBluetoothStack(BluetoothStackID))
   {
      /* Acquire the BSC List Lock while we are searching the instance  */
      /* list.                                                          */
      if(BSC_AcquireListLock())
      {
         /* Store a copy of the passed in InstanceID locally.           */
         LocalInstanceID = *InstanceID;

         /* Verify that the Instance ID is valid.                       */
         if((LocalInstanceID) && (LocalInstanceID <= NDCS_MAXIMUM_SUPPORTED_INSTANCES))
         {
            /* Decrement the LocalInstanceID (to access the InstanceList*/
            /* which is 0 based).                                       */
            --LocalInstanceID;

            /* Verify that this Instance is registered and valid.       */
            if((InstanceList[LocalInstanceID].BluetoothStackID == BluetoothStackID) && (InstanceList[LocalInstanceID].ServiceID))
            {
               /* Return a pointer to this instance.                    */
               ret_val = &InstanceList[LocalInstanceID];
            }
         }
         else
         {
            /* Verify that we have been requested to find the next free */
            /* instance.                                                */
            if(!LocalInstanceID)
            {
               /* Try to find a free instance.                          */
               for(Index=0;Index<NDCS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
               {
                  /* Check to see if this instance is being used.       */
                  if(!(InstanceList[Index].ServiceID))
                  {
                     /* Return the InstanceID AND a pointer to the      */
                     /* instance.                                       */
                     *InstanceID = Index+1;
                     ret_val     = &InstanceList[Index];
                     break;
                  }
               }
            }
         }

         /* Release the previously acquired list lock.                  */
         BSC_ReleaseListLock();
      }

      /* If we failed to acquire the instance then we should un-lock the*/
      /* previously acquired Bluetooth Stack.                           */
      if(!ret_val)
         BSC_UnLockBluetoothStack(BluetoothStackID);
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   /* The following function is used to suspend (or calculate memory    */
   /* required for suspend) a given NDCS instance.  It returns the      */
   /* number of bytes used (or required for) a suspend.                 */
   /* * NOTE * Internal function no check needed on input parameters.   */
static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer)
{
   unsigned int               Offset;
   unsigned long              RequiredMemory = 0;
   NDCSSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Instance ID is always +1 to be always positive.                   */
   InstanceID--;

   /* Calculate the memory required for a successful suspend.           */

   /* Include the size of the NDCS Instance Data.                       */
   RequiredMemory += NDCS_INSTANCE_DATA_SIZE;

   /* Check to see if we need to save the required instance.            */
   if(BufferSize)
   {
      /* Verify that the buffer is big enough.                          */
      if((Buffer) && (BufferSize >= (NDCS_SUSPEND_INSTANCE_INFO_SIZE(RequiredMemory) + BTPS_ALIGNMENT_SIZE)))
      {
         /* Get an aligned pointer to where we will store the suspend   */
         /* information.                                                */
         SuspendInstanceInfo                      = (NDCSSuspendInstanceInfo_t *)(Buffer + BTPS_ALIGNMENT_OFFSET(Buffer, NDCSSuspendInstanceInfo_t));

         /* Store the registered GATT Handle Range so we can restore the*/
         /* service in the same location on resume.                     */
         SuspendInstanceInfo->InstanceHandleRange = InstanceList[InstanceID].InstanceHandleRange;

         /* Save the Instance ID.                                       */
         /* * NOTE * We need to increment the instance ID since we      */
         /*          previously decremented it.                         */
         SuspendInstanceInfo->InstanceID          = InstanceID + 1;

         /* Save the user Event Callback.                               */
         SuspendInstanceInfo->EventCallback       = InstanceList[InstanceID].EventCallback;

         /* Save the user Callback Parameter..                          */
         SuspendInstanceInfo->CallbackParameter   = InstanceList[InstanceID].CallbackParameter;

         /* Set the initialize offset to 0.                             */
         Offset                                   = 0;

         /* Copy the data into the suspend info.                        */
         BTPS_MemCopy(&(SuspendInstanceInfo->VariableData[Offset]), &(InstanceData[InstanceID]), NDCS_INSTANCE_DATA_SIZE);
         Offset += NDCS_INSTANCE_DATA_SIZE;

         /* Calculate the actual required amount of memory used.        */
         RequiredMemory = (NDCS_SUSPEND_INSTANCE_INFO_SIZE(Offset) + BTPS_ALIGNMENT_OFFSET(Buffer, NDCSSuspendInstanceInfo_t));

         /* Mark the instance entry as being free.                      */
         BTPS_MemInitialize(&(InstanceList[InstanceID]), 0, NDCS_SERVER_INSTANCE_DATA_SIZE);
      }
      else
         RequiredMemory = 0;
   }
   else
   {
      /* Add the memory for the header plus the alignment size so we can*/
      /* align the received buffer.                                     */
      RequiredMemory += (NDCS_SUSPEND_INSTANCE_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE);
   }

   /* Return the used required memory to the caller.                    */
   return(RequiredMemory);
}

   /* The following function is used to resume a service instance.  It  */
   /* returns 0 on success or a negative errror code.                   */
   /* * NOTE * Internal function no check needed on input parameters.   */
static int ResumeServiceInstance(unsigned int BluetoothStackID, unsigned long *BufferSize, Byte_t **Buffer)
{
   int                        ret_val;
   Byte_t                    *TempBuffer      = *Buffer;
   unsigned int               AlignmentOffset;
   unsigned int               ServiceID;
   unsigned int               InstanceID;
   unsigned long              TempBufferSize  = *BufferSize;
   NDCSSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Make sure we have enough memory for an instance header.           */
   if((TempBuffer) && (TempBufferSize >= NDCS_SUSPEND_INSTANCE_INFO_SIZE(0)))
   {
      /* Calculate the alignment offset for the buffer.                 */
      AlignmentOffset = BTPS_ALIGNMENT_OFFSET(TempBuffer, NDCSSuspendInstanceInfo_t);

      /* Make sure there is enough buffer space to align the buffer.    */
      if(TempBufferSize >= (NDCS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset))
      {
         /* Align the temp buffer pointer.                              */
         SuspendInstanceInfo  = (NDCSSuspendInstanceInfo_t *)(TempBuffer + AlignmentOffset);

         /* Set temp buffer to the variable data.                       */
         TempBuffer           = SuspendInstanceInfo->VariableData;

         /* Subtract the header from the buffer size.                   */
         TempBufferSize      -= (NDCS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset);

         /* Verify that the requested InstanceID is not being used      */
         /* currently.                                                  */
         if((SuspendInstanceInfo->InstanceID) && (SuspendInstanceInfo->InstanceID <= NDCS_MAXIMUM_SUPPORTED_INSTANCES) && (!(InstanceList[SuspendInstanceInfo->InstanceID-1].ServiceID)))
         {
            /* Attempt to re-initialize the NDCS service.                */
            ret_val = NDCSRegisterService(BluetoothStackID, SuspendInstanceInfo->EventCallback, SuspendInstanceInfo->CallbackParameter, &ServiceID, &(SuspendInstanceInfo->InstanceHandleRange), &(SuspendInstanceInfo->InstanceID));
            if(ret_val > 0)
            {
               /* Save the initialized Instance ID.                     */
               InstanceID = (unsigned int)ret_val;

               /* Initialize the return value to indicate success.      */
               ret_val = 0;

               /*********************************************************/
               /* Service is registered, now restore all of the Instance*/
               /* Data.                                                 */
               /*********************************************************/

               /* Verify that we have enough memory for the NDCS Instance*/
               /* Data.                                                 */
               if(TempBufferSize >= NDCS_INSTANCE_DATA_SIZE)
               {
                  /* Copy the suspended instance data into the NDCS      */
                  /* Instance Data (it is already formatted).           */
                  BTPS_MemCopy(&(InstanceData[InstanceID - 1]), TempBuffer, NDCS_INSTANCE_DATA_SIZE);

                  /* Update the pointers and the buffersize.            */
                  TempBufferSize -= NDCS_INSTANCE_DATA_SIZE;
                  TempBuffer     += NDCS_INSTANCE_DATA_SIZE;
               }
               else
                  ret_val = NDCS_ERROR_MALFORMATTED_DATA;

               /* Check to see if an error occurred.                    */
               if(!ret_val)
               {
                  /* Update the caller's buffer size and point for any  */
                  /* further instances.                                 */
                  *BufferSize = TempBufferSize;
                  *Buffer     = TempBuffer;
               }
               else
               {
                  /* Some failure occurred so cleanup the service.      */
                  NDCS_Cleanup_Service(BluetoothStackID, InstanceID);
               }
            }
         }
         else
            ret_val = NDCS_ERROR_INVALID_PARAMETER;
      }
      else
         ret_val = NDCS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = NDCS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

#endif

   /* The following function is a utility function which is used to     */
   /* register an NDCS Service.  This function returns the positive,    */
   /* non-zero, Instance ID on success or a negative error code.        */
static int NDCSRegisterService(unsigned int BluetoothStackID, NDCS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID)
{
   int                   ret_val;
   unsigned int          InstanceID;
   NDCSServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (EventCallback) && (ServiceID))
   {
      /* Verify that no instance is registered to this Bluetooth Stack. */
      if(!InstanceRegisteredByStackID(BluetoothStackID))
      {
         /* Acquire a free NDCS Instance.                               */
         InstanceID = 0;
         if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
         {

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

            /* If the Instance ID of the initialized service doesn't    */
            /* match the expected (which we know is free) we need update*/
            /* the Instance ID to the old value so we restore the       */
            /* suspended service instance information to the expected   */
            /* location.                                                */
            if((PreviousInstanceID) && (*PreviousInstanceID != InstanceID) && (!(InstanceList[*PreviousInstanceID-1].ServiceID)))
            {
               /* Clear the memory for the other entry.                 */
               BTPS_MemInitialize(ServiceInstance, 0, NDCS_SERVER_INSTANCE_DATA_SIZE);

               /* Store pointer to new instance.                        */
               ServiceInstance = &(InstanceList[*PreviousInstanceID - 1]);

               /* Save the InstanceID we should use from now on.        */
               InstanceID      = *PreviousInstanceID;
            }

#endif

            /* Call GATT to register the NDCS service.                  */
            ret_val = GATT_Register_Service(BluetoothStackID, NDCS_SERVICE_FLAGS, NEXT_DST_CHANGE_TIME_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)Next_DST_Time_Service, ServiceHandleRange, GATT_ServerEventCallback, InstanceID);
            if(ret_val > 0)
            {
               /* Save the Instance information.                        */
               ServiceInstance->BluetoothStackID    = BluetoothStackID;
               ServiceInstance->ServiceID           = (unsigned int)ret_val;
               ServiceInstance->EventCallback       = EventCallback;
               ServiceInstance->CallbackParameter   = CallbackParameter;
               *ServiceID                           = (unsigned int)ret_val;

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

               /* Store the handle range that the service was saved in. */
               ServiceInstance->InstanceHandleRange = *ServiceHandleRange;

#endif

               /* Intilize the Instance Data for this instance.         */
               BTPS_MemInitialize(&InstanceData[InstanceID-1], 0, NDCS_INSTANCE_DATA_SIZE);

               /* Return the NDCS Instance ID.                          */
               ret_val = (int)InstanceID;
            }

            /* UnLock the previously locked Bluetooth Stack.            */
            BSC_UnLockBluetoothStack(BluetoothStackID);
         }
         else
            ret_val = NDCS_ERROR_INSUFFICIENT_RESOURCES;
      }
      else
         ret_val = NDCS_ERROR_SERVICE_ALREADY_REGISTERED;
   }
   else
      ret_val = NDCS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is the GATT Server Event Callback that     */
   /* handles all requests made to the NDCS Service for all registered  */
   /* instances.                                                        */
static void BTPSAPI GATT_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
   Word_t                AttributeOffset;
   Byte_t                Event_Buffer[NDCS_EVENT_DATA_SIZE + NDCS_EVENT_DATA_BUFFER_SIZE];
   unsigned int          TransactionID;
   unsigned int          InstanceID;
   NDCS_Event_Data_t    *EventData;
   NDCSServerInstance_t *ServiceInstance;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_ServerEventData) && (CallbackParameter))
   {
      /* The Instance ID is always registered as the callback parameter.*/
      InstanceID = (unsigned int)CallbackParameter;

      /* Acquire the Service Instance for the specified service.        */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         switch(GATT_ServerEventData->Event_Data_Type)
         {
            case etGATT_Server_Read_Request:
               if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data)
               {
                  /* Verify that the Event Data is valid.               */
                  AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset;
                  TransactionID   = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;

                  /* Verify that they are not trying to write with an   */
                  /* offset or using preprared writes.                  */
                  if(!(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeValueOffset))
                  {
                     if(AttributeOffset == NDSC_TIME_WITH_DST_ATTRIBUTE_OFFSET)
                     {
                        EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etNDCS_Server_Read_Current_Time_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice));
                        if(EventData)
                        {
                           /* Format the rest of the event.             */
                           EventData->Event_Data_Size = NDCS_READ_TIME_WITH_DST_REQUEST_DATA_SIZE;

                           /* Dispatch the event.                       */
                           __BTPSTRY
                           {
                              (*ServiceInstance->EventCallback)(ServiceInstance->BluetoothStackID, EventData, ServiceInstance->CallbackParameter);
                           }
                           __BTPSEXCEPT(1)
                           {
                              /* Do Nothing.                            */
                           }
                        }
                     }
                  }
                  else
                     GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
               }
               break;
            default:
               /* Do nothing, as this is just here to get rid of        */
               /* warnings that some compilers flag when not all cases  */
               /* are handled in a switch off of a enumerated value.    */
               break;
         }

         BSC_UnLockBluetoothStack(BluetoothStackID);
      }
   }
}

   /* The following function is responsible for making sure that the    */
   /* Bluetooth Stack NDCS Module is Initialized correctly.  This       */
   /* function *MUST* be called before ANY other Bluetooth Stack NDCS   */
   /* function can be called.  This function returns non-zero if the    */
   /* Module was initialized correctly, or a zero value if there was an */
   /* error.                                                            */
   /* * NOTE * Internally, this module will make sure that this function*/
   /*          has been called at least once so that the module will    */
   /*          function.  Calling this function from an external        */
   /*          location is not necessary.                               */
int InitializeNDCSModule(void)
{
   return((int)InitializeModule());
}

   /* The following function is responsible for instructing the         */
   /* Bluetooth Stack NDCS Module to clean up any resources that it has */
   /* allocated.  Once this function has completed, NO other Bluetooth  */
   /* Stack NDCS Functions can be called until a successful call to the */
   /* InitializeNDCSModule() function is made.  The parameter to this   */
   /* function specifies the context in which this function is being    */
   /* called.  If the specified parameter is TRUE, then the module will */
   /* make sure that NO functions that would require waiting/blocking on*/
   /* Mutexes/Events are called.  This parameter would be set to TRUE if*/
   /* this function was called in a context where threads would not be  */
   /* allowed to run.  If this function is called in the context where  */
   /* threads are allowed to run then this parameter should be set to   */
   /* FALSE.                                                            */
void CleanupNDCSModule(Boolean_t ForceCleanup)
{
   /* Check to make sure that this module has been initialized.         */
   if(InstanceListInitialized)
   {
      /* Wait for access to the NDCS Context List.                      */
      if((ForceCleanup) || ((!ForceCleanup) && (BSC_AcquireListLock())))
      {
         /* Cleanup the Instance List.                                  */
         BTPS_MemInitialize(InstanceList, 0, sizeof(InstanceList));

         if(!ForceCleanup)
            BSC_ReleaseListLock();
      }

      /* Cleanup the module.                                            */
      CleanupModule();
   }
}

   /* NDCS Server API.                                                  */

   /* The following function is responsible for opening a NDCS Server.  */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is a user-defined callback parameter*/
   /* that will be passed to the callback function with each event.  The*/
   /* final parameter is a pointer to store the GATT Service ID of the  */
   /* registered NDCS service.  This can be used to include the service */
   /* registered by this call.  This function returns the positive,     */
   /* non-zero, Instance ID or a negative error code.                   */
   /* * NOTE * Only 1 NDCS Server may be open at a time, per Bluetooth  */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
int BTPSAPI NDCS_Initialize_Service(unsigned int BluetoothStackID, NDCS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID)
{
   GATT_Attribute_Handle_Group_t ServiceHandleRange;

    /* Initialize the Service Handle Group to 0.                        */
   ServiceHandleRange.Starting_Handle = 0;
   ServiceHandleRange.Ending_Handle   = 0;

   return(NDCSRegisterService(BluetoothStackID, EventCallback, CallbackParameter, ServiceID, &ServiceHandleRange, NULL));
}

   /* The following function is responsible for opening a NDCS Server.  */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The fourth parameter is a     */
   /* pointer to store the GATT Service ID of the registered NDCS       */
   /* service.  This can be used to include the service registered by   */
   /* this call.  The final parameter is a pointer, that on input can be*/
   /* used to control the location of the service in the GATT database, */
   /* and on ouput to store the service handle range.  This function    */
   /* returns the positive, non-zero, Instance ID or a negative error   */
   /* code.                                                             */
   /* * NOTE * Only 1 NDCS Server may be open at a time, per Bluetooth  */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
int BTPSAPI NDCS_Initialize_Service_Handle_Range(unsigned int BluetoothStackID, NDCS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange)
{
   return(NDCSRegisterService(BluetoothStackID, EventCallback, CallbackParameter, ServiceID, ServiceHandleRange, NULL));
}

   /* The following function is responsible for closing a previously    */
   /* opened NDCS Server.  The first parameter is the Bluetooth Stack ID*/
   /* on which to close the server.  The second parameter is the        */
   /* InstanceID that was returned from a successful call to            */
   /* NDCS_Initialize_Service().  This function returns a zero if       */
   /* successful or a negative return error code if an error occurs.    */
int BTPSAPI NDCS_Cleanup_Service(unsigned int BluetoothStackID, unsigned int InstanceID)
{
   int                   ret_val;
   NDCSServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID))
   {
      /* Acquire the specified NDCS Instance.                           */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Verify that the service is actually registered.             */
         if(ServiceInstance->ServiceID)
         {
            /* Call GATT to un-register the service.                    */
            GATT_Un_Register_Service(BluetoothStackID, ServiceInstance->ServiceID);

            /* mark the instance entry as being free.                   */
            BTPS_MemInitialize(ServiceInstance, 0, NDCS_SERVER_INSTANCE_DATA_SIZE);

            /* return success to the caller.                            */
            ret_val = 0;
         }
         else
            ret_val = NDCS_ERROR_INVALID_PARAMETER;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(BluetoothStackID);
      }
      else
         ret_val = NDCS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = NDCS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is used to perform a suspend of the        */
   /* Bluetooth stack.  This function accepts as input the Bluetooth    */
   /* Stack ID of the Bluetooth Stack that the Device is associated     */
   /* with.  The final two parameters are the buffer size and buffer    */
   /* that Bluetopia is to use to collapse it's state information into. */
   /* This function can be called with BufferSize and Buffer set to 0   */
   /* and NULL, respectively.  In this case this function will return   */
   /* the number of bytes that must be passed to this function in order */
   /* to successfully perform a suspend (or 0 if an error occurred, or  */
   /* this functionality is not supported).  If the BufferSize and      */
   /* Buffer parameters are NOT 0 and NULL, this function will attempt  */
   /* to perform a suspend of the stack.  In this case, this function   */
   /* will return the amount of memory that was used from the provided  */
   /* buffers for the suspend (or zero otherwise).                      */
unsigned long BTPSAPI NDCS_Suspend(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   Byte_t            *TempPtr         = NULL;
   Boolean_t          PerformSuspend;
   unsigned int       Index;
   unsigned long      MemoryUsed;
   unsigned long      TotalMemoryUsed = 0;
   NDCSSuspendInfo_t *SuspendInfo     = NULL;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize == 0) || ((Buffer) && (BufferSize >= (NDCS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE))))
   {
      /* Attempt to attain a lock on the Bluetooth Stack.               */
      if(!BSC_LockBluetoothStack(BluetoothStackID))
      {
         /* See if we are performing a suspend or calculating the memory*/
         /* required to perform a suspend.                              */
         if(BufferSize)
         {
            /* Get an aligned pointer to the memory to store the suspend*/
            /* information.                                             */
            TempPtr                                  = (Byte_t *)(Buffer);
            SuspendInfo                              = (NDCSSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, NDCSSuspendInfo_t));

            /* Calculated the memory used for the header.               */
            MemoryUsed                               = (NDCS_SUSPEND_INFO_SIZE(0) + (((Byte_t *)SuspendInfo) - ((Byte_t *)Buffer)));

            /* Adjust the counts.                                       */
            TotalMemoryUsed                         += MemoryUsed;
            BufferSize                              -= MemoryUsed;

            /* Set a pointer to the memory location for the service     */
            /* instances.                                               */
            TempPtr                                  = SuspendInfo->VariableData;

            /* Set the number of suspended instances to 0.              */
            SuspendInfo->NumberOfSuspendedInstances  = 0;

            /* Flag that we should suspend all registered services.     */
            PerformSuspend                           = TRUE;
         }
         else
         {
            /* Calculated the memory used for the header.               */
            TotalMemoryUsed += (NDCS_SUSPEND_INFO_SIZE(0));

            /* Flag that we are not performing the suspend, but instead */
            /* calculating the memory required for a suspend.           */
            PerformSuspend   = FALSE;
         }

         /* Loop and suspend all active services.                       */
         for(Index = 0; Index < (unsigned int)NDCS_MAXIMUM_SUPPORTED_INSTANCES; Index++)
         {
            /* Check to see if this instance is currently registered.   */
            if((InstanceList[Index].BluetoothStackID == BluetoothStackID) && (InstanceList[Index].ServiceID))
            {
               /* Check to see if we are performing a suspend.          */
               if(PerformSuspend)
               {
                  /* Verify that we have memory left to perform the     */
                  /* suspend.                                           */
                  if(BufferSize >= NDCS_SUSPEND_INSTANCE_INFO_SIZE(0))
                  {
                     /* Suspend this service.                           */
                     /* * NOTE * Instance ID's are positive non-zero.   */
                     MemoryUsed = SuspendServiceInstance(BluetoothStackID, (Index+1), BufferSize, TempPtr);

                     /* Verify that we didn't use more memory then      */
                     /* given.                                          */
                     if(MemoryUsed <= BufferSize)
                     {
                        /* If something was stored in the suspend info  */
                        /* go ahead and increment count of services     */
                        /* stored.                                      */
                        if(MemoryUsed)
                        {
                           /* Adjust the buffer size and count and      */
                           /* adjust total memory used.                 */
                           BufferSize                              -= MemoryUsed;
                           TempPtr                                 += MemoryUsed;
                           TotalMemoryUsed                         += MemoryUsed;

                           /* Increment the stored instance count.      */
                           SuspendInfo->NumberOfSuspendedInstances += 1;
                        }
                     }
                     else
                     {
                        /* Used more memory than given exit loop.       */
                        break;
                     }
                  }
                  else
                  {
                     /* Used all of the memory so just exit the loop.   */
                     break;
                  }
               }
               else
               {
                  /* Simply calculate the required number of bytes for a*/
                  /* successful suspend.                                */
                  TotalMemoryUsed += SuspendServiceInstance(BluetoothStackID, (Index+1), 0, NULL);
               }
            }
         }

         /* Release the previously acquired Bluetooth Stack Lock.       */
         BSC_UnLockBluetoothStack(BluetoothStackID);
      }
   }

   /* Finally return the result to the caller.                          */
   return(TotalMemoryUsed);

#else

   /* Feature not supported so just return 0.                           */
   return(0);

#endif
}

   /* The following function is used to perform a resume of the         */
   /* Bluetooth stack after a successful suspend has been performed (see*/
   /* NDCS_Suspend()).  This function accepts as input the Bluetooth    */
   /* Stack ID of the Bluetooth Stack that the Device is associated     */
   /* with.  The final two parameters are the buffer size and buffer    */
   /* that contains the memory that was used to collapse Bluetopia      */
   /* context into with a successfull call to NDCS_Suspend().  This     */
   /* function returns ZERO on success or a negative error code.        */
int BTPSAPI NDCS_Resume(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   int                ret_val;
   Byte_t            *TempPtr;
   unsigned int       Index;
   NDCSSuspendInfo_t *SuspendInfo;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize) && ((TempPtr = (Byte_t *)Buffer) != NULL))
   {
      /* Verify that the buffer size is large enough for NDCS.           */
      if(BufferSize >= (NDCS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, NDCSSuspendInfo_t)))
      {
         /* Subtract the header and alignment offset from the buffer    */
         /* size.                                                       */
         BufferSize  -= (NDCS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, NDCSSuspendInfo_t));

         /* Align the buffer for the NDCSSuspendInfo_t structrue.        */
         SuspendInfo  = (NDCSSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, NDCSSuspendInfo_t));

         /* Attempt to attain a lock on the Bluetooth Stack.            */
         if((ret_val = BSC_LockBluetoothStack(BluetoothStackID)) == 0)
         {
            /* Set the temp pointer to point to the variable data.      */
            TempPtr = SuspendInfo->VariableData;

            /* Loop and resume all suspended services.                  */
            for(Index = 0; (Index < SuspendInfo->NumberOfSuspendedInstances) && (!ret_val); Index++)
            {
               /* Attempt to resume all previously suspended services.  */
               ret_val = ResumeServiceInstance(BluetoothStackID, &BufferSize, &TempPtr);
            }

            /* Release the previously acquired Bluetooth Stack Lock.    */
            BSC_UnLockBluetoothStack(BluetoothStackID);
         }
      }
      else
         ret_val = NDCS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = NDCS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   /* Feature not supported so just return 0.                           */
   return(0);

#endif
}

   /* The following function is responsible for querying the number of  */
   /* attributes that are contained in the NDCS Service that is         */
   /* registered with a call to NDCS_Initialize_Service().  This        */
   /* function returns the non-zero number of attributes that are       */
   /* contained in a NDCS Server or zero on failure.                    */
unsigned int BTPSAPI NDCS_Query_Number_Attributes(void)
{
   /* Simply return the number of attributes that are contained in a    */
   /* NDCS service.                                                     */
   return(NEXT_DST_CHANGE_TIME_SERVICE_ATTRIBUTE_COUNT);
}

   /* The following function is responsible for sending the Next DST    */
   /* Change Time read request response.  The first parameter is        */
   /* Bluetooth Stack ID of the Bluetooth Device.  The second parameter */
   /* is the Transaction ID.  The final parameter is the value of       */
   /* NDCS_Time_With_Dst_Data_t structure.  This function returns a zero*/
   /* if successful or a negative return error code if an error occurs. */
int BTPSAPI NDCS_Time_With_DST_Read_Request_Response(unsigned int BluetoothStackID, unsigned int TransactionID,  NDCS_Time_With_Dst_Data_t *Next_Dst_Change_Time)
{
   int    ret_val = NDCS_ERROR_MALFORMATTED_DATA;
   Byte_t Value[NDCS_TIME_WITH_DST_DATA_SIZE];

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID) && (Next_Dst_Change_Time))
   {
      /* Format the received data.                                      */
      if((ret_val = FormatDSTTime(Next_Dst_Change_Time, NDCS_TIME_WITH_DST_DATA_SIZE, Value)) == 0)
      {
         /* Send the current time read response.                        */
         ret_val = GATT_Read_Response(BluetoothStackID, TransactionID, (unsigned int)NDCS_TIME_WITH_DST_DATA_SIZE, Value);
      }
   }
   else
      ret_val = NDCS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for responding to a Next DST*/
   /* Change Time read request with an error response.  The first       */
   /* parameter is Bluetooth Stack ID of the Bluetooth Device.  The     */
   /* second parameter is the Transaction ID.  The final parameter is   */
   /* the GATT error code to respond to the request with.  This function*/
   /* returns a zero if successful or a negative return error code if an*/
   /* error occurs.                                                     */
int BTPSAPI NDCS_Time_With_DST_Read_Request_Error_Response(unsigned int BluetoothStackID, unsigned int TransactionID, Byte_t ErrorCode)
{
   int ret_val = NDCS_ERROR_MALFORMATTED_DATA;

  /* Make sure the parameters passed to us are semi-valid.              */
  if((BluetoothStackID) && (TransactionID))
     ret_val = GATT_Error_Response(BluetoothStackID, TransactionID, NDSC_TIME_WITH_DST_ATTRIBUTE_OFFSET, ErrorCode);
  else
     ret_val = NDCS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* NDCS Client API.                                                  */

   /* The following function is responsible for parsing a value received*/
   /* from a remote NDCS Server interpreting it as a NextDST Change Time*/
   /* characteristic.  The first parameter is the length of the value   */
   /* returned by the remote NDCS Server.  The second parameter is a    */
   /* pointer to the data returned by the remote NDCS Server.  The final*/
   /* parameter is a pointer to store the parsed NextDST Change Time    */
   /* Measurement value.  This function returns a zero if successful or */
   /* a negative return error code if an error occurs.                  */
int BTPSAPI NDCS_Decode_Time_With_Dst(unsigned int ValueLength, Byte_t *Value, NDCS_Time_With_Dst_Data_t *Next_Dst_Change_Time)
{
   int ret_val = NDCS_ERROR_MALFORMATTED_DATA;

   /* Verify that the input parameters are valid.                       */
   if((ValueLength == NDCS_TIME_WITH_DST_DATA_SIZE) && (Value) && (Next_Dst_Change_Time))
   {
      /* Read the packed Next Dst change Time Data.                     */
      Next_Dst_Change_Time->Date_Time.Year    = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((NDCS_Time_With_Dst_t *)Value)->Date_Time.Year));
      Next_Dst_Change_Time->Date_Time.Month   = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((NDCS_Time_With_Dst_t *)Value)->Date_Time.Month));
      Next_Dst_Change_Time->Date_Time.Day     = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((NDCS_Time_With_Dst_t *)Value)->Date_Time.Day));
      Next_Dst_Change_Time->Date_Time.Hours   = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((NDCS_Time_With_Dst_t *)Value)->Date_Time.Hours));
      Next_Dst_Change_Time->Date_Time.Minutes = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((NDCS_Time_With_Dst_t *)Value)->Date_Time.Minutes));
      Next_Dst_Change_Time->Date_Time.Seconds = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((NDCS_Time_With_Dst_t *)Value)->Date_Time.Seconds));

      /* Retrive the DST_OFFSET from specific instance                  */
      Next_Dst_Change_Time->Dst_Offset = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((NDCS_Time_With_Dst_t *)Value)->Dst_Offset));

      ret_val = 0;
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}
