/*****< ias.c >****************************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  IAS  - Bluetooth Stack Immediate Alert Service (GATT Based) for           */
/*        Stonestreet One Bluetooth Protocol Stack.                           */
/*                                                                            */
/*  Author:  Ajay Parashar                                                    */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   08/24/12  A. parashar    Initial creation.                               */
/*   06/09/16  R. McCord      Added Suspend/Resume support.                   */
/******************************************************************************/
#include "SS1BTPS.h"        /* Bluetooth Stack API Prototypes/Constants.      */
#include "SS1BTGAT.h"       /* Bluetooth Stack GATT API Prototypes/Constants. */
#include "SS1BTIAS.h"       /* Bluetooth IAS API Prototypes/Constants.        */
#include "BTPSKRNL.h"       /* BTPS Kernel Prototypes/Constants.              */
#include "IAS.h"            /* Bluetooth IAS Prototypes/Constants.            */

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   #include "BTPSALIGN.h"   /* Alignment Related Constants.                   */
#endif

  /* The following controls the number of supported IAS instances.      */
#define IAS_MAXIMUM_SUPPORTED_INSTANCES                 (BTPS_CONFIGURATION_IAS_MAXIMUM_SUPPORTED_INSTANCES)

   /* IAS Service Instance Block.  This structure contains All          */
   /* information associated with a specific Bluetooth Stack ID (member */
   /* is present in this structure).                                    */
typedef struct _tagIASServerInstance_t
{
   unsigned int                  BluetoothStackID;
   unsigned int                  ServiceID;
   IAS_Event_Callback_t          EventCallback;
   unsigned long                 CallbackParameter;

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
#endif

} IASServerInstance_t;

#define IAS_SERVER_INSTANCE_DATA_SIZE                   (sizeof(IASServerInstance_t))

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   /* The following structure contains the information that is          */
   /* stored/restored in IAS Service Instance when a suspend/resume     */
   /* procedure is performed.                                           */
typedef struct IASSuspendInstanceInfo_s
{
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
   unsigned int                  InstanceID;
   IAS_Event_Callback_t          EventCallback;
   unsigned long                 CallbackParameter;
   Byte_t                        VariableData[1];
} IASSuspendInstanceInfo_t;

#define IAS_SUSPEND_INSTANCE_INFO_SIZE(_x)               (BTPS_STRUCTURE_OFFSET(IASSuspendInstanceInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

   /* The following structure contains the information that is          */
   /* stored/restored in IAS Service when a suspend/resume procedure is */
   /* performed.                                                        */
typedef struct IASSuspendInfo_s
{
   unsigned int NumberOfSuspendedInstances;
   Byte_t       VariableData[1];
} IASSuspendInfo_t;

#define IAS_SUSPEND_INFO_SIZE(_x)                        (BTPS_STRUCTURE_OFFSET(IASSuspendInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

#endif

   /*********************************************************************/
   /**               Immediate Alert Service Table                     **/
   /*********************************************************************/

   /* The Immediate Alert Service Declaration UUID.                     */
static BTPSCONST GATT_Primary_Service_16_Entry_t IAS_Service_UUID =
{
   IAS_SERVICE_BLUETOOTH_UUID_CONSTANT
};

   /* The Alert Level Characteristic Declaration.                       */
static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t IAS_Alert_Level_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_WRITE_WITHOUT_RESPONSE,
   IAS_ALERT_LEVEL_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Alert Level Characteristic Value.                             */
static BTPSCONST GATT_Characteristic_Value_16_Entry_t  IAS_Alert_Level_Value =
{
   IAS_ALERT_LEVEL_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};

   /* The following defines the SImmediate Alert Service that is        */
   /* registered with the GATT_Register_Service function call.          */
   /* * NOTE * This array will be registered with GATT in the call to   */
   /*          GATT_Register_Service.                                   */
BTPSCONST GATT_Service_Attribute_Entry_t Immediate_Alert_Service[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService16,            (Byte_t *)&IAS_Service_UUID},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration16, (Byte_t *)&IAS_Alert_Level_Declaration},
   {GATT_ATTRIBUTE_FLAGS_WRITABLE,          aetCharacteristicValue16,       (Byte_t *)&IAS_Alert_Level_Value},
};

#define IMMEDIATE_ALERT_SERVICE_ATTRIBUTE_COUNT          (sizeof(Immediate_Alert_Service)/sizeof(GATT_Service_Attribute_Entry_t))

   /*********************************************************************/
   /**                    END OF SERVICE TABLE                         **/
   /*********************************************************************/

   /* The following type defines a union large enough to hold all events*/
   /* dispatched by this module.                                        */
typedef union
{
   IAS_Alert_Level_Control_Point_Command_Data_t  IAS_Alert_Level_Control_Point_Command_Data;
} IAS_Event_Data_Buffer_t;

#define IAS_EVENT_DATA_BUFFER_SIZE                      (sizeof(IAS_Event_Data_Buffer_t))

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

static IASServerInstance_t InstanceList[IAS_MAXIMUM_SUPPORTED_INSTANCES];
                                            /* Variable which holds the */
                                            /* service instance data.   */

static Boolean_t InstanceListInitialized;   /* Variable that flags that */
                                            /* is used to denote that   */
                                            /* this module has been     */
                                            /* successfully initialized.*/

   /* The following are the prototypes of local functions.              */
static Boolean_t InitializeModule(void);
static void CleanupModule(void);
static Boolean_t InstanceRegisteredByStackID(unsigned int BluetoothStackID);
static IASServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID);

static IAS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, IAS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR);

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer);
   static int ResumeServiceInstance(unsigned int BluetoothStackID, unsigned long *BufferSize, Byte_t **Buffer);
#endif

static int IASRegisterService(unsigned int BluetoothStackID, IAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID);

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

  /* The following function is a utility function that exists to        */
  /* perform stack specific (threaded versus nonthreaded) cleanup.      */
static void CleanupModule(void)
{
  /* Flag that we are no longer initialized.                            */
  InstanceListInitialized = FALSE;
}

   /* The following function is responsible for making sure that the    */
   /* Bluetooth Stack IAS Module is Initialized correctly.  This        */
   /* function *MUST* be called before ANY other Bluetooth Stack IAS    */
   /* function can be called.  This function returns non-zero if the    */
   /* Module was initialized correctly, or a zero value if there was an */
   /* error.                                                            */
   /* * NOTE * Internally, this module will make sure that this function*/
   /*          has been called at least once so that the module will    */
   /*          function.  Calling this function from an external        */
   /*          location is not necessary.                               */
int InitializeIASModule(void)
{
   return((int)InitializeModule());
}

   /* The following function is a utility function that exists to format*/
   /* a IAS Event into the specified buffer.                            */
   /* * NOTE * TransactionID is optional and may be set to NULL.        */
   /* * NOTE * BD_ADDR is NOT optional and may NOT be set to NULL.      */
static IAS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, IAS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR)
{
   IAS_Event_Data_t *EventData = NULL;

   if((BufferLength >= (IAS_EVENT_DATA_SIZE + IAS_EVENT_DATA_BUFFER_SIZE)) && (Buffer) && (BD_ADDR))
   {
      /* Format the header of the event, that is data that is common to */
      /* all events.                                                    */
      BTPS_MemInitialize(Buffer, 0, BufferLength);
      EventData                                                                        = (IAS_Event_Data_t *)Buffer;
      EventData->Event_Data_Type                                                       = EventType;
      EventData->Event_Data.IAS_Alert_Level_Control_Point_Command_Data                 = (IAS_Alert_Level_Control_Point_Command_Data_t *)(((Byte_t *)EventData) + IAS_EVENT_DATA_SIZE);
      EventData->Event_Data.IAS_Alert_Level_Control_Point_Command_Data->InstanceID     = InstanceID;
      EventData->Event_Data.IAS_Alert_Level_Control_Point_Command_Data->ConnectionID   = ConnectionID;
      EventData->Event_Data.IAS_Alert_Level_Control_Point_Command_Data->ConnectionType = ConnectionType;
      EventData->Event_Data.IAS_Alert_Level_Control_Point_Command_Data->RemoteDevice   = *BD_ADDR;
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

   for(Index=0;Index<IAS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
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
static IASServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID)
{
   unsigned int         LocalInstanceID;
   unsigned int         Index;
   IASServerInstance_t *ret_val = NULL;

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
         if((LocalInstanceID) && (LocalInstanceID <= IAS_MAXIMUM_SUPPORTED_INSTANCES))
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
               for(Index=0;Index<IAS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
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
   /* required for suspend) a given IAS instance.  It returns the number*/
   /* of bytes used (or required for) a suspend.                        */
   /* * NOTE * Internal function no check needed on input parameters.   */
static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer)
{
   unsigned int              Offset;
   unsigned long             RequiredMemory = 0;
   IASSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Instance ID is always +1 to be always positive.                   */
   InstanceID--;

   /* Check to see if we need to save the required instance.            */
   if(BufferSize)
   {
      /* Verify that the buffer is big enough.                          */
      if((Buffer) && (BufferSize >= (IAS_SUSPEND_INSTANCE_INFO_SIZE(RequiredMemory) + BTPS_ALIGNMENT_SIZE)))
      {
         /* Get an aligned pointer to where we will store the suspend   */
         /* information.                                                */
         SuspendInstanceInfo                      = (IASSuspendInstanceInfo_t *)(Buffer + BTPS_ALIGNMENT_OFFSET(Buffer, IASSuspendInstanceInfo_t));

         /* Store the registered GATT Handle Range so we can restore the*/
         /* service in the same location on resume.                     */
         SuspendInstanceInfo->InstanceHandleRange = InstanceList[InstanceID].InstanceHandleRange;

         /* Save the Instance ID.                                       */
         /* * NOTE * We need to increment the instance ID since we      */
         /*          previously decremented it.                         */
         SuspendInstanceInfo->InstanceID          = InstanceID + 1;

         /* Save the user Event Callback.                               */
         SuspendInstanceInfo->EventCallback       = InstanceList[InstanceID].EventCallback;

         /* Save the user Callback Parameter.                           */
         SuspendInstanceInfo->CallbackParameter   = InstanceList[InstanceID].CallbackParameter;

         /* Set the initialize offset to 0.                             */
         Offset                                   = 0;

         /* Calculate the actual required amount of memory used.        */
         RequiredMemory = (IAS_SUSPEND_INSTANCE_INFO_SIZE(Offset) + BTPS_ALIGNMENT_OFFSET(Buffer, IASSuspendInstanceInfo_t));

         /* Mark the instance entry as being free.                      */
         BTPS_MemInitialize(&(InstanceList[InstanceID]), 0, IAS_SERVER_INSTANCE_DATA_SIZE);
      }
      else
         RequiredMemory = 0;
   }
   else
   {
      /* Add the memory for the header plus the alignment size so we can*/
      /* align the received buffer.                                     */
      RequiredMemory += (IAS_SUSPEND_INSTANCE_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE);
   }

   /* Return the used required memory to the caller.                    */
   return(RequiredMemory);
}

   /* The following function is used to resume a service instance.  It  */
   /* returns 0 on success or a negative errror code.                   */
   /* * NOTE * Internal function no check needed on input parameters.   */
static int ResumeServiceInstance(unsigned int BluetoothStackID, unsigned long *BufferSize, Byte_t **Buffer)
{
   int                       ret_val;
   Byte_t                   *TempBuffer      = *Buffer;
   unsigned int              AlignmentOffset;
   unsigned int              ServiceID;
   unsigned long             TempBufferSize  = *BufferSize;
   IASSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Make sure we have enough memory for an instance header.           */
   if((TempBuffer) && (TempBufferSize >= IAS_SUSPEND_INSTANCE_INFO_SIZE(0)))
   {
      /* Calculate the alignment offset for the buffer.                 */
      AlignmentOffset = BTPS_ALIGNMENT_OFFSET(TempBuffer, IASSuspendInstanceInfo_t);

      /* Make sure there is enough buffer space to align the buffer.    */
      if(TempBufferSize >= (IAS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset))
      {
         /* Align the temp buffer pointer.                              */
         SuspendInstanceInfo  = (IASSuspendInstanceInfo_t *)(TempBuffer + AlignmentOffset);

         /* Set temp buffer to the variable data.                       */
         TempBuffer           = SuspendInstanceInfo->VariableData;

         /* Subtract the header from the buffer size.                   */
         TempBufferSize      -= (IAS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset);

         /* Verify that the requested InstanceID is not being used      */
         /* currently.                                                  */
         if((SuspendInstanceInfo->InstanceID) && (SuspendInstanceInfo->InstanceID <= IAS_MAXIMUM_SUPPORTED_INSTANCES) && (!(InstanceList[SuspendInstanceInfo->InstanceID-1].ServiceID)))
         {
            /* Attempt to re-initialize the IAS service.                */
            ret_val = IASRegisterService(BluetoothStackID, SuspendInstanceInfo->EventCallback, SuspendInstanceInfo->CallbackParameter, &ServiceID, &(SuspendInstanceInfo->InstanceHandleRange), &(SuspendInstanceInfo->InstanceID));
            if(ret_val > 0)
            {
               /* Initialize the return value to indicate success.      */
               ret_val = 0;

               /*********************************************************/
               /* Service is registered, now restore all of the Instance*/
               /* Data.                                                 */
               /*********************************************************/

               /* Update the caller's buffer size and point for any     */
               /* further instances.                                    */
               *BufferSize = TempBufferSize;
               *Buffer     = TempBuffer;
            }
         }
         else
            ret_val = IAS_ERROR_INVALID_PARAMETER;
      }
      else
         ret_val = IAS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = IAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

#endif

   /* The following function is a utility function which is used to     */
   /* register an IAS Service.  This function returns the positive,     */
   /* non-zero, Instance ID on success or a negative error code.        */
static int IASRegisterService(unsigned int BluetoothStackID, IAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID)
{
   int                  ret_val;
   unsigned int         InstanceID;
   IASServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (EventCallback) && (ServiceID))
   {
      /* Verify that no instance is registered to this Bluetooth Stack. */
      if(!InstanceRegisteredByStackID(BluetoothStackID))
      {
         /* Acquire a free IAS Instance.                                */
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
               BTPS_MemInitialize(ServiceInstance, 0, IAS_SERVER_INSTANCE_DATA_SIZE);

               /* Store pointer to new instance.                        */
               ServiceInstance = &(InstanceList[*PreviousInstanceID - 1]);

               /* Save the InstanceID we should use from now on.        */
               InstanceID      = *PreviousInstanceID;
            }

#endif

            /* Call GATT to register the IAS service.                   */
            ret_val = GATT_Register_Service(BluetoothStackID, IAS_SERVICE_FLAGS, IMMEDIATE_ALERT_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)Immediate_Alert_Service, ServiceHandleRange, GATT_ServerEventCallback, InstanceID);
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

               /* Return the IAS Instance ID.                           */
               ret_val = (int)InstanceID;
            }
            /* UnLock the previously locked Bluetooth Stack.            */
            BSC_UnLockBluetoothStack(BluetoothStackID);
         }
         else
            ret_val = IAS_ERROR_INSUFFICIENT_RESOURCES;
      }
      else
         ret_val = IAS_ERROR_SERVICE_ALREADY_REGISTERED;
   }
   else
      ret_val = IAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is the GATT Server Event Callback that     */
   /* handles all requests made to the IAS Service for all registered   */
   /* instances.                                                        */
static void BTPSAPI GATT_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
   Word_t               AttributeOffset;
   Word_t               ValueLength;
   Byte_t               Event_Buffer[IAS_EVENT_DATA_SIZE + IAS_EVENT_DATA_BUFFER_SIZE];
   unsigned int         TransactionID;
   unsigned int         InstanceID;
   IAS_Event_Data_t    *EventData;
   IASServerInstance_t *ServiceInstance;

    /* Verify that all parameters to this callback are Semi-Valid.      */
   if((BluetoothStackID) && (GATT_ServerEventData) && (CallbackParameter))
   {
      /* The Instance ID is always registered as the callback parameter.*/
      InstanceID = (unsigned int)CallbackParameter;

      /* Acquire the Service Instance for the specified service.        */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         switch(GATT_ServerEventData->Event_Data_Type)
         {
              case etGATT_Server_Write_Request:
               /* Verify that the Event Data is valid.                  */
               if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data)
               {
                  /* Store the needed members for use later.            */
                  AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;
                  TransactionID   = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID;
                  ValueLength     = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;

                  /* Verify that the parameters look correct.           */
                  if((ValueLength == IAS_ALERT_LEVEL_CONTROL_POINT_VALUE_LENGTH) && (!(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset)) && (!(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->DelayWrite)))
                  {
                     /* IAS defines that the Write Without Response     */
                     /* procedure is used to write to this attribute,   */
                     /* therefore we will just accept the write request */
                     /* here to allow GATT to clean up the resources.   */
                     GATT_Write_Response(BluetoothStackID, TransactionID);

                     /* Format and Dispatch the event.                  */
                     EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etIAS_Server_Alert_Level_Control_Point_Command, InstanceID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice));
                     if(EventData)
                     {
                        /* Format the rest of the event.                */
                        EventData->Event_Data_Size                                                = IAS_ALERT_LEVEL_CONTROL_POINT_COMMAND_DATA_SIZE;
                        EventData->Event_Data.IAS_Alert_Level_Control_Point_Command_Data->Command = (IAS_Control_Point_Command_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);

                        __BTPSTRY
                        {
                           (*ServiceInstance->EventCallback)(ServiceInstance->BluetoothStackID, EventData, ServiceInstance->CallbackParameter);
                        }
                        __BTPSEXCEPT(1)
                        {
                           /* Do Nothing.                               */
                        }
                     }
                  }
                  else
                     GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_REQUEST_NOT_SUPPORTED);
               }
               break;
            default:
               /* Do nothing, as this is just here to get rid of        */
               /* warnings that some compilers flag when not all cases  */
               /* are handled in a switch off of a enumerated value.    */
               break;
         }

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
   }
}

   /* The following function is responsible for instructing the         */
   /* Bluetooth Stack IAS Module to clean up any resources that it has  */
   /* allocated. Once this function has completed, NO other Bluetooth   */
   /* Stack IAS Functions can be called until a successful call to the  */
   /* InitializeIASModule() function is made.  The parameter to this    */
   /* function specifies the context in which this function is being    */
   /* called.  If the specified parameter is TRUE, then the module will */
   /* make sure that NO functions that would require waiting/blocking on*/
   /* Mutexes/Events are called.  This parameter would be set to TRUE if*/
   /* this function was called in a context where threads would not be  */
   /* allowed to run.  If this function is called in the context where  */
   /* threads are allowed to run then this parameter should be set to   */
   /* FALSE.                                                            */
void CleanupIASModule(Boolean_t ForceCleanup)
{
   /* Check to make sure that this module has been initialized.         */
   if(InstanceListInitialized)
   {
      /* Wait for access to the IAS Context List.                       */
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

   /* The following function is responsible for opening a IAS Server.   */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The final parameter is a      */
   /* pointer to store the GATT Service ID of the registered IAS        */
   /* service.  This can be used to include the service registered by   */
   /* this call.  This function returns the positive, non-zero, Instance*/
   /* ID or a negative error code.                                      */
   /* * NOTE * Only 1 IAS Server may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
int BTPSAPI IAS_Initialize_Service(unsigned int BluetoothStackID, IAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID)
{
   GATT_Attribute_Handle_Group_t ServiceHandleRange;

    /* Initialize the Service Handle Group to 0.                        */
   ServiceHandleRange.Starting_Handle = 0;
   ServiceHandleRange.Ending_Handle   = 0;

   return(IASRegisterService(BluetoothStackID, EventCallback, CallbackParameter, ServiceID, &ServiceHandleRange, NULL));
}

   /* The following function is responsible for opening a IAS Server.   */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The fourth parameter is a     */
   /* pointer to store the GATT Service ID of the registered IAS        */
   /* service.  This can be used to include the service registered by   */
   /* this call.  The final parameter is a pointer, that on input can be*/
   /* used to control the location of the service in the GATT database, */
   /* and on ouput to store the service handle range.  This function    */
   /* returns the positive, non-zero, Instance ID or a negative error   */
   /* code.                                                             */
   /* * NOTE * Only 1 IAS Server may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
int BTPSAPI IAS_Initialize_Service_Handle_Range(unsigned int BluetoothStackID, IAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange)
{
   return(IASRegisterService(BluetoothStackID, EventCallback, CallbackParameter, ServiceID, ServiceHandleRange, NULL));
}

   /* The following function is responsible for closing a previously    */
   /* opened IAS Server.  The first parameter is the Bluetooth Stack ID */
   /* on which to close the server.  The second parameter is the        */
   /* InstanceID that was returned from a successful call to            */
   /* IAS_Initialize_Service().  This function returns a zero if        */
   /* successful or a negative return error code if an error occurs.    */
int BTPSAPI IAS_Cleanup_Service(unsigned int BluetoothStackID, unsigned int InstanceID)
{
   int                  ret_val;
   IASServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID))
   {
      /* Acquire the specified IAS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Verify that the service is actually registered.             */
         if(ServiceInstance->ServiceID)
         {
            /* Call GATT to un-register the service.                    */
            GATT_Un_Register_Service(BluetoothStackID, ServiceInstance->ServiceID);

            /* mark the instance entry as being free.                   */
            BTPS_MemInitialize(ServiceInstance, 0, IAS_SERVER_INSTANCE_DATA_SIZE);

            /* return success to the caller.                            */
            ret_val = 0;
         }
         else
            ret_val = IAS_ERROR_INVALID_PARAMETER;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(BluetoothStackID);
      }
      else
         ret_val = IAS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = IAS_ERROR_INVALID_PARAMETER;

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
unsigned long BTPSAPI IAS_Suspend(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   Byte_t           *TempPtr         = NULL;
   Boolean_t         PerformSuspend;
   unsigned int      Index;
   unsigned long     MemoryUsed;
   unsigned long     TotalMemoryUsed = 0;
   IASSuspendInfo_t *SuspendInfo     = NULL;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize == 0) || ((Buffer) && (BufferSize >= (IAS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE))))
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
            SuspendInfo                              = (IASSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, IASSuspendInfo_t));

            /* Calculated the memory used for the header.               */
            MemoryUsed                               = (IAS_SUSPEND_INFO_SIZE(0) + (((Byte_t *)SuspendInfo) - ((Byte_t *)Buffer)));

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
            TotalMemoryUsed += (IAS_SUSPEND_INFO_SIZE(0));

            /* Flag that we are not performing the suspend, but instead */
            /* calculating the memory required for a suspend.           */
            PerformSuspend   = FALSE;
         }

         /* Loop and suspend all active services.                       */
         for(Index = 0; Index < (unsigned int)IAS_MAXIMUM_SUPPORTED_INSTANCES; Index++)
         {
            /* Check to see if this instance is currently registered.   */
            if((InstanceList[Index].BluetoothStackID == BluetoothStackID) && (InstanceList[Index].ServiceID))
            {
               /* Check to see if we are performing a suspend.          */
               if(PerformSuspend)
               {
                  /* Verify that we have memory left to perform the     */
                  /* suspend.                                           */
                  if(BufferSize >= IAS_SUSPEND_INSTANCE_INFO_SIZE(0))
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
   /* IAS_Suspend()).  This function accepts as input the Bluetooth     */
   /* Stack ID of the Bluetooth Stack that the Device is associated     */
   /* with.  The final two parameters are the buffer size and buffer    */
   /* that contains the memory that was used to collapse Bluetopia      */
   /* context into with a successfull call to IAS_Suspend().  This      */
   /* function returns ZERO on success or a negative error code.        */
int BTPSAPI IAS_Resume(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   int               ret_val;
   Byte_t           *TempPtr;
   unsigned int      Index;
   IASSuspendInfo_t *SuspendInfo;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize) && ((TempPtr = (Byte_t *)Buffer) != NULL))
   {
      /* Verify that the buffer size is large enough for IAS.           */
      if(BufferSize >= (IAS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, IASSuspendInfo_t)))
      {
         /* Subtract the header and alignment offset from the buffer    */
         /* size.                                                       */
         BufferSize  -= (IAS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, IASSuspendInfo_t));

         /* Align the buffer for the IASSuspendInfo_t structrue.        */
         SuspendInfo  = (IASSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, IASSuspendInfo_t));

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
         ret_val = IAS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = IAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   /* Feature not supported so just return 0.                           */
   return(0);

#endif
}

   /* The following function is responsible for querying the number of  */
   /* attributes that are contained in the IAS Service that is          */
   /* registered with a call to IAS_Initialize_Service().  This function*/
   /* returns the non-zero number of attributes that are contained in an*/
   /* IAS Server or zero on failure.                                    */
unsigned int BTPSAPI IAS_Query_Number_Attributes(void)
{
   /* Simply return the number of attributes that are contained in an   */
   /* IAS service.                                                      */
   return(IMMEDIATE_ALERT_SERVICE_ATTRIBUTE_COUNT);
}

   /* The following function is responsible for formatting an Alert     */
   /* Level Control Point Command into a user specified buffer. The     */
   /* first parameter is the command to format.  The final two          */
   /* parameters contain the length of the buffer, and the buffer, to   */
   /* format the command into. This function returns a zero if          */
   /* successful or a negative return error code if an error occurs.    */
   /* * NOTE * The BufferLength and Buffer parameter must point to a    */
   /*          buffer of at least                                       */
   /*          IAS_ALERT_LEVEL_CONTROL_POINT_VALUE_LENGTH in size.      */
int BTPSAPI IAS_Format_Control_Point_Command(IAS_Control_Point_Command_t Command, unsigned int BufferLength, Byte_t *Buffer)
{
   int ret_val;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BufferLength >= IAS_ALERT_LEVEL_CONTROL_POINT_VALUE_LENGTH) && (Buffer))
   {
      /* Assign the command into the user specified buffer.             */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(Buffer, Command);

      /* Return success to the caller.                                  */
      ret_val = 0;
   }
   else
      ret_val = IAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}
