/*****< bas.c >****************************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BAS - Bluetooth Stack Battery Service (GATT Based) for Stonestreet One    */
/*        Bluetooth Protocol Stack.                                           */
/*                                                                            */
/*  Author:  Zahid Khan                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   06/28/12  Z. Khan        Initial creation.                               */
/*   06/09/16  R. McCord      Added Suspend/Resume support.                   */
/******************************************************************************/
#include "SS1BTPS.h"        /* Bluetooth Stack API Prototypes/Constants.      */
#include "SS1BTGAT.h"       /* Bluetooth Stack GATT API Prototypes/Constants. */
#include "SS1BTBAS.h"       /* Bluetooth BAS API Prototypes/Constants.        */

#include "BTPSKRNL.h"       /* BTPS Kernel Prototypes/Constants.              */
#include "BAS.h"            /* Bluetooth BAS Prototypes/Constants.            */

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   #include "BTPSALIGN.h"   /* Alignment Related Constants.                   */
#endif

   /* The following controls the number of supported BAS instances.     */
#define BAS_MAXIMUM_SUPPORTED_INSTANCES                  (BTPS_CONFIGURATION_BAS_MAXIMUM_SUPPORTED_INSTANCES)

   /* The following defines are used to set the Format and Unit fields  */
   /* of the Characteristic Presentation Format Descriptor that goes    */
   /* with the Battery Level if multiple instances are supported.  These*/
   /* are constants as the Format and the Unit of the Battery Level     */
   /* Characteristic are defined by the Battery Service Specification   */
   /* and thus will be the same for all instances.                      */
#define BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_FORMAT_UNSIGNED_BYTE_INTEGER   4
#define BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_UNIT_PERCENTAGE                0x27AD

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

   /* The following structure defines the Instance Data that must be    */
   /* unique for each BAS service registered (Only 1 per Bluetooth      */
   /* Stack).                                                           */
typedef __PACKED_STRUCT_BEGIN__ struct _tagBAS_Instance_Data_t
{
   NonAlignedWord_t Battery_Level_Presentation_Format_Length;
   BAS_Presentation_Format_t Battery_Level_Presentation_Format;
} __PACKED_STRUCT_END__ BAS_Instance_Data_t;

#define BAS_INSTANCE_DATA_SIZE                              (sizeof(BAS_Instance_Data_t))

   /* The following define the instance tags for each BAS service data  */
   /* that is unique per registered service.                            */
#define BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_INSTANCE_TAG  (BTPS_STRUCTURE_OFFSET(BAS_Instance_Data_t, Battery_Level_Presentation_Format_Length))

#endif

   /*********************************************************************/
   /**                      Battery Service Table                      **/
   /*********************************************************************/

   /* The Battery Service Declaration UUID.                             */
static BTPSCONST GATT_Primary_Service_16_Entry_t BAS_Service_UUID =
{
   BAS_SERVICE_BLUETOOTH_UUID_CONSTANT
};

   /* The Battery Level Characteristic Declaration.                     */
static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t BAS_Battery_Level_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY),
   BAS_BATTERY_LEVEL_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Battery Level Characteristic Value.                           */
static BTPSCONST GATT_Characteristic_Value_16_Entry_t  BAS_Battery_Level_Value =
{
   BAS_BATTERY_LEVEL_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};


#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

   /* The Characteristic Presentation Format Descriptor.                */
static GATT_Characteristic_Descriptor_16_Entry_t Characteristic_Presentation_Format_Descriptor =
{
   GATT_CHARACTERISTIC_PRESENTATION_FORMAT_BLUETOOTH_UUID_CONSTANT,
   BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_INSTANCE_TAG,
   NULL
};

#endif

   /* Client Characteristic Configuration Descriptor.                   */
static GATT_Characteristic_Descriptor_16_Entry_t Client_Characteristic_Configuration =
{
   GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
   GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
   NULL
};

   /* The following defines the Battery Service that is registered with */
   /* the GATT_Register_Service function call.                          */
   /* * NOTE * This array will be registered with GATT in the call to   */
   /*          GATT_Register_Service.                                   */
BTPSCONST GATT_Service_Attribute_Entry_t Battery_Service[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService16,            (Byte_t *)&BAS_Service_UUID},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration16, (Byte_t *)&BAS_Battery_Level_Declaration},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicValue16,       (Byte_t *)&BAS_Battery_Level_Value},
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,  (Byte_t *)&Client_Characteristic_Configuration},

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDescriptor16, (Byte_t *)&Characteristic_Presentation_Format_Descriptor},
#endif
};

#define BATTERY_SERVICE_ATTRIBUTE_COUNT               (sizeof(Battery_Service)/sizeof(GATT_Service_Attribute_Entry_t))

#define BAS_BATTERY_LEVEL_ATTRIBUTE_OFFSET                     2
#define BAS_CLIENT_CHARACTERISTIC_CONFIGURATION_OFFSET         3

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)
#define BAS_PRESENTATION_FORMAT_DESCRIPTOR_ATTRIBUTE_OFFSET    4
#endif

   /*********************************************************************/
   /**                    END OF SERVICE TABLE                         **/
   /*********************************************************************/

   /* The following type defines a union large enough to hold all events*/
   /* dispatched by this module.                                        */
typedef union _tagBAS_Event_Data_Buffer_t
{
   BAS_Read_Client_Configuration_Data_t   Read_Client_Configuration_Data;
   BAS_Client_Configuration_Update_Data_t Client_Configuration_Update_Data;
   BAS_Read_Battery_Level_Data_t          BAS_Read_Battery_Level_Data;
} BAS_Event_Data_Buffer_t;

#define BAS_EVENT_DATA_BUFFER_SIZE                      (sizeof(BAS_Event_Data_Buffer_t))

   /* BAS Service Instance Block.  This structure contains All          */
   /* information associated with a specific Bluetooth Stack ID (member */
   /* is present in this structure).                                    */
typedef struct _tagBASServerInstance_t
{
   unsigned int                  BluetoothStackID;
   unsigned int                  ServiceID;
   BAS_Event_Callback_t          EventCallback;
   unsigned long                 CallbackParameter;

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
#endif

} BASServerInstance_t;

#define BAS_SERVER_INSTANCE_DATA_SIZE                    (sizeof(BASServerInstance_t))

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   /* The following structure contains the information that is          */
   /* stored/restored in BAS Service Instance when a suspend/resume     */
   /* procedure is performed.                                           */
typedef struct BASSuspendInstanceInfo_s
{
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
   unsigned int                  InstanceID;
   BAS_Event_Callback_t          EventCallback;
   unsigned long                 CallbackParameter;
   Byte_t                        VariableData[1];
} BASSuspendInstanceInfo_t;

#define BAS_SUSPEND_INSTANCE_INFO_SIZE(_x)               (BTPS_STRUCTURE_OFFSET(BASSuspendInstanceInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

   /* The following structure contains the information that is          */
   /* stored/restored in BAS Service when a suspend/resume procedure is */
   /* performed.                                                        */
typedef struct BASSuspendInfo_s
{
   unsigned int NumberOfSuspendedInstances;
   Byte_t       VariableData[1];
} BASSuspendInfo_t;

#define BAS_SUSPEND_INFO_SIZE(_x)                        (BTPS_STRUCTURE_OFFSET(BASSuspendInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

#endif

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

static  BAS_Instance_Data_t InstanceData[BAS_MAXIMUM_SUPPORTED_INSTANCES];
                                            /* Variable which holds all */
                                            /* data that is unique for  */
                                            /* each service instance.   */

#endif

static BASServerInstance_t InstanceList[BAS_MAXIMUM_SUPPORTED_INSTANCES];
                                            /* Variable which holds the */
                                            /* service instance data.   */

static Boolean_t InstanceListInitialized;   /* Variable that flags that */
                                            /* is used to denote that   */
                                            /* this module has been     */
                                            /* successfully initialized.*/

   /* The following are the prototypes of local functions.              */
static Boolean_t InitializeModule(void);
static void CleanupModule(void);
static int DecodeClientConfigurationValue(unsigned int BufferLength, Byte_t *Buffer, Boolean_t *Notify);
static BAS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, BAS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, unsigned int *TransactionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR);
static Boolean_t InstanceExceeded(unsigned int BluetoothStackID);
static BASServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID);

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer);
   static int ResumeServiceInstance(unsigned int BluetoothStackID, unsigned long *BufferSize, Byte_t **Buffer);
#endif

   /* BAS Register Service                                              */
static int BASRegisterService(unsigned int BluetoothStackID, BAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID);

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

   /* The following function is a utility function that exists to decode*/
   /* an Client Configuration value into a user specified boolean value.*/
   /* This function returns the zero if successful or a negative error  */
   /* code.                                                             */
static int DecodeClientConfigurationValue(unsigned int BufferLength, Byte_t *Buffer, Boolean_t *Notify)
{
   int    ret_val = BAS_ERROR_MALFORMATTED_DATA;
   Word_t ClientConfiguration;

   /* Verify that the input parameters are valid.                       */
   if(((BufferLength == NON_ALIGNED_BYTE_SIZE) || (BufferLength == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH)) && (Buffer) && (Notify))
   {
      /* Read the requested Client Configuration.                       */
      if(BufferLength == NON_ALIGNED_BYTE_SIZE)
         ClientConfiguration = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(Buffer);
      else
         ClientConfiguration = READ_UNALIGNED_WORD_LITTLE_ENDIAN(Buffer);

      if(ClientConfiguration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
      {
         *Notify = TRUE;
         ret_val = 0;
      }
      else
      {
         if(!ClientConfiguration)
         {
            *Notify = FALSE;
            ret_val = 0;
         }
      }
   }
   else
   {
      if(BufferLength == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH)
         ret_val = BAS_ERROR_INVALID_PARAMETER;
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is a utility function that exists to format*/
   /* a BAS Event into the specified buffer.                            */
   /* * NOTE * TransactionID is optional and may be set to NULL.        */
   /* * NOTE * BD_ADDR is NOT optional and may NOT be set to NULL.      */
static BAS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, BAS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, unsigned int *TransactionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR)
{
   BAS_Event_Data_t *EventData = NULL;

   if((BufferLength >= (BAS_EVENT_DATA_SIZE + BAS_EVENT_DATA_BUFFER_SIZE)) && (Buffer) && (BD_ADDR))
   {
      /* Format the header of the event, that is data that is common to */
      /* all events.                                                    */
      BTPS_MemInitialize(Buffer, 0, BufferLength);

      EventData                                                              = (BAS_Event_Data_t *)Buffer;
      EventData->Event_Data_Type                                             = EventType;
      EventData->Event_Data.BAS_Read_Client_Configuration_Data               = (BAS_Read_Client_Configuration_Data_t *)(((Byte_t *)EventData) + BAS_EVENT_DATA_SIZE);
      EventData->Event_Data.BAS_Read_Client_Configuration_Data->InstanceID   = InstanceID;
      EventData->Event_Data.BAS_Read_Client_Configuration_Data->ConnectionID = ConnectionID;

      if(TransactionID)
      {
         EventData->Event_Data.BAS_Read_Client_Configuration_Data->TransactionID  = *TransactionID;
         EventData->Event_Data.BAS_Read_Client_Configuration_Data->ConnectionType = ConnectionType;
         EventData->Event_Data.BAS_Read_Client_Configuration_Data->RemoteDevice   = *BD_ADDR;
      }
      else
      {
         EventData->Event_Data.BAS_Client_Configuration_Update_Data->ConnectionType = ConnectionType;
         EventData->Event_Data.BAS_Client_Configuration_Update_Data->RemoteDevice   = *BD_ADDR;
      }
   }

   /* Finally return the result to the caller.                          */
   return(EventData);
}

   /* The following function is a utility function that exists to check */
   /* to see if number of instances registered is exceeding the maximum */
   /* allowed instances.                                                */
   /* * NOTE * Since this is an internal function no check is done on   */
   /*          the input parameters.                                    */
static Boolean_t InstanceExceeded(unsigned int BluetoothStackID)
{
   Boolean_t    ret_val = TRUE;
   unsigned int Index;

   for(Index=0;Index<BAS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
   {
      if((InstanceList[Index].BluetoothStackID == 0) || ((InstanceList[Index].BluetoothStackID == BluetoothStackID) && (InstanceList[Index].ServiceID == 0)))
      {
         ret_val = FALSE;
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
static BASServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID)
{
   unsigned int         LocalInstanceID;
   unsigned int         Index;
   BASServerInstance_t *ret_val = NULL;

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
         if((LocalInstanceID) && (LocalInstanceID <= BAS_MAXIMUM_SUPPORTED_INSTANCES))
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
               for(Index=0;Index<BAS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
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
   /* required for suspend) a given BAS instance.  It returns the number*/
   /* of bytes used (or required for) a suspend.                        */
   /* * NOTE * Internal function no check needed on input parameters.   */
static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer)
{
   unsigned int              Offset;
   unsigned long             RequiredMemory = 0;
   BASSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Instance ID is always +1 to be always positive.                   */
   InstanceID--;

   /* Calculate the memory required for a successful suspend.           */

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

   /* Include the size of the BAS Instance Data.                        */
   RequiredMemory += BAS_INSTANCE_DATA_SIZE;

#endif

   /* Check to see if we need to save the required instance.            */
   if(BufferSize)
   {
      /* Verify that the buffer is big enough.                          */
      if((Buffer) && (BufferSize >= (BAS_SUSPEND_INSTANCE_INFO_SIZE(RequiredMemory) + BTPS_ALIGNMENT_SIZE)))
      {
         /* Get an aligned pointer to where we will store the suspend   */
         /* information.                                                */
         SuspendInstanceInfo                      = (BASSuspendInstanceInfo_t *)(Buffer + BTPS_ALIGNMENT_OFFSET(Buffer, BASSuspendInstanceInfo_t));

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

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

         /* Copy the data into the suspend info.                        */
         BTPS_MemCopy(&(SuspendInstanceInfo->VariableData[Offset]), &(InstanceData[InstanceID]), BAS_INSTANCE_DATA_SIZE);
         Offset += BAS_INSTANCE_DATA_SIZE;

#endif

         /* Calculate the actual required amount of memory used.        */
         RequiredMemory = (BAS_SUSPEND_INSTANCE_INFO_SIZE(Offset) + BTPS_ALIGNMENT_OFFSET(Buffer, BASSuspendInstanceInfo_t));

         /* Mark the instance entry as being free.                      */
         BTPS_MemInitialize(&(InstanceList[InstanceID]), 0, BAS_SERVER_INSTANCE_DATA_SIZE);
      }
      else
         RequiredMemory = 0;
   }
   else
   {
      /* Add the memory for the header plus the alignment size so we can*/
      /* align the received buffer.                                     */
      RequiredMemory += (BAS_SUSPEND_INSTANCE_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE);
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
   unsigned int              InstanceID;
   unsigned long             TempBufferSize  = *BufferSize;
   BASSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Make sure we have enough memory for an instance header.           */
   if((TempBuffer) && (TempBufferSize >= BAS_SUSPEND_INSTANCE_INFO_SIZE(0)))
   {
      /* Calculate the alignment offset for the buffer.                 */
      AlignmentOffset = BTPS_ALIGNMENT_OFFSET(TempBuffer, BASSuspendInstanceInfo_t);

      /* Make sure there is enough buffer space to align the buffer.    */
      if(TempBufferSize >= (BAS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset))
      {
         /* Align the temp buffer pointer.                              */
         SuspendInstanceInfo  = (BASSuspendInstanceInfo_t *)(TempBuffer + AlignmentOffset);

         /* Set temp buffer to the variable data.                       */
         TempBuffer           = SuspendInstanceInfo->VariableData;

         /* Subtract the header from the buffer size.                   */
         TempBufferSize      -= (BAS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset);

         /* Verify that the requested InstanceID is not being used      */
         /* currently.                                                  */
         if((SuspendInstanceInfo->InstanceID) && (SuspendInstanceInfo->InstanceID <= BAS_MAXIMUM_SUPPORTED_INSTANCES) && (!(InstanceList[SuspendInstanceInfo->InstanceID-1].ServiceID)))
         {
            /* Attempt to re-initialize the BAS service.                */
            ret_val = BASRegisterService(BluetoothStackID, SuspendInstanceInfo->EventCallback, SuspendInstanceInfo->CallbackParameter, &ServiceID, &(SuspendInstanceInfo->InstanceHandleRange), &(SuspendInstanceInfo->InstanceID));
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

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

               /* Verify that we have enough memory for the BAS Instance*/
               /* Data.                                                 */
               if(TempBufferSize >= BAS_INSTANCE_DATA_SIZE)
               {
                  /* Copy the suspended instance data into the BAS      */
                  /* Instance Data (it is already formatted).           */
                  BTPS_MemCopy(&(InstanceData[InstanceID - 1]), TempBuffer, BAS_INSTANCE_DATA_SIZE);

                  /* Update the pointers and the buffersize.            */
                  TempBufferSize -= BAS_INSTANCE_DATA_SIZE;
                  TempBuffer     += BAS_INSTANCE_DATA_SIZE;
               }
               else
                  ret_val = BAS_ERROR_MALFORMATTED_DATA;

#endif


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
                  BAS_Cleanup_Service(BluetoothStackID, InstanceID);
               }
            }
         }
         else
            ret_val = BAS_ERROR_INVALID_PARAMETER;
      }
      else
         ret_val = BAS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

#endif

   /* The following function is a utility function which is used to     */
   /* register an BAS Service.  This function returns the positive,     */
   /* non-zero, Instance ID on success or a negative error code.        */
static int BASRegisterService(unsigned int BluetoothStackID, BAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID)
{
   int                  ret_val;
   unsigned int         InstanceID;
   BASServerInstance_t *ServiceInstance = NULL;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (EventCallback) && (ServiceID))
   {
      /* Verify that number of service instances registered not         */
      /* exceeding the maximum number of instances allowed.             */
      if(!InstanceExceeded(BluetoothStackID))
      {
         /* Acquire a free BAS Instance.                                */
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
               BTPS_MemInitialize(ServiceInstance, 0, BAS_SERVER_INSTANCE_DATA_SIZE);

               /* Store pointer to new instance.                        */
               ServiceInstance = &(InstanceList[*PreviousInstanceID - 1]);

               /* Save the InstanceID we should use from now on.        */
               InstanceID      = *PreviousInstanceID;
            }

#endif

            /* Call GATT to register the BAS service.                   */
            ret_val = GATT_Register_Service(BluetoothStackID, BAS_SERVICE_FLAGS, BATTERY_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)Battery_Service, ServiceHandleRange, GATT_ServerEventCallback, InstanceID);
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

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

               /* Intilize the Instance Data for this instance.         */
               BTPS_MemInitialize(&InstanceData[InstanceID-1], 0, BAS_INSTANCE_DATA_SIZE);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format_Length), BAS_PRESENTATION_FORMAT_SIZE);

               /* Assign the constant fields of the Battery Level       */
               /* Characteristic Presentation Format Descriptor.        */
               ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Format),      BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_FORMAT_UNSIGNED_BYTE_INTEGER);
               ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Exponent),    0);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Unit),        BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_UNIT_PERCENTAGE);

#endif

               /* Return the BAS Instance ID.                           */
               ret_val = (int)InstanceID;
            }

            /* UnLock the previously locked Bluetooth Stack.            */
            BSC_UnLockBluetoothStack(BluetoothStackID);
         }
         else
            ret_val = BAS_ERROR_INSUFFICIENT_RESOURCES;
      }
      else
         ret_val = BAS_ERROR_MAXIMUM_NUMBER_OF_INSTANCES_REACHED;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is the GATT Server Event Callback that     */
   /* handles all requests made to the BAS Service for all registered   */
   /* instances.                                                        */
static void BTPSAPI GATT_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
   Word_t               AttributeOffset;
   Word_t               ValueLength;

#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

   Word_t               InstanceTag;

#endif

   Byte_t              *Value;
   Byte_t               Event_Buffer[BAS_EVENT_DATA_SIZE + BAS_EVENT_DATA_BUFFER_SIZE];
   unsigned int         TransactionID;
   unsigned int         InstanceID;
   BAS_Event_Data_t    *EventData;
   BASServerInstance_t *ServiceInstance;

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
               /* Verify that the Event Data is valid.                  */
               if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data)
               {
                  AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset;
                  TransactionID   = GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID;

                  /* Verify that they are not trying to write with an   */
                  /* offset or using preprared writes.                  */
                  if(!(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeValueOffset))
                  {
                     if((Battery_Service[AttributeOffset].Attribute_Entry_Type == aetCharacteristicDescriptor16) && (AttributeOffset == BAS_CLIENT_CHARACTERISTIC_CONFIGURATION_OFFSET))
                     {
                        EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etBAS_Server_Read_Client_Configuration_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice));
                        if(EventData)
                        {
                           /* Format the rest of the event.             */
                           EventData->Event_Data_Size                                                        = BAS_READ_CLIENT_CONFIGURATION_DATA_SIZE;
                           EventData->Event_Data.BAS_Read_Client_Configuration_Data->ClientConfigurationType = ctBatteryLevel;

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
                        else
                           GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR);

                     }
                     else
                     {
                        if(AttributeOffset == BAS_BATTERY_LEVEL_ATTRIBUTE_OFFSET)
                        {
                           EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etBAS_Server_Read_Battery_Level_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice));
                           if(EventData)
                           {
                              /* Format the rest of the event.          */
                              EventData->Event_Data_Size = BAS_READ_BATTERY_LEVEL_DATA_SIZE;

                              /* Dispatch the event.                    */
                              __BTPSTRY
                              {
                                 (*ServiceInstance->EventCallback)(ServiceInstance->BluetoothStackID, EventData, ServiceInstance->CallbackParameter);
                              }
                              __BTPSEXCEPT(1)
                              {
                                 /* Do Nothing.                         */
                              }

                           }

                        }
#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)
                        else
                        {
                           if(AttributeOffset == BAS_PRESENTATION_FORMAT_DESCRIPTOR_ATTRIBUTE_OFFSET)
                           {
                              /* Get the instance tag for the           */
                              /* characteristic.                        */
                              InstanceTag = (Word_t)(((GATT_Characteristic_Value_16_Entry_t *)Battery_Service[AttributeOffset].Attribute_Value)->Characteristic_Value_Length);
                              ValueLength = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((Byte_t *)(&InstanceData[InstanceID-1]))[InstanceTag]));
                              Value       = (Byte_t *)(&(((Byte_t *)(&InstanceData[InstanceID-1]))[InstanceTag + WORD_SIZE]));

                              /* Respond with the data.                 */
                              GATT_Read_Response(BluetoothStackID, TransactionID, (unsigned int)ValueLength, Value);
                           }
                           else
                              GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET);
                        }
#else
                        else
                           GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET);
#endif
                     }
                  }
                  else
                     GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
               }
               break;
            case etGATT_Server_Write_Request:
               /* Verify that the Event Data is valid.                  */
               if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data)
               {
                  AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;
                  TransactionID   = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID;
                  ValueLength     = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;
                  Value           = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue;

                  if((!(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset)) && (!(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->DelayWrite)))
                  {
                     if(Battery_Service[AttributeOffset].Attribute_Entry_Type == aetCharacteristicDescriptor16)
                     {
                        /* Begin formatting the Client Configuration    */
                        /* Update event.                                */
                        EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etBAS_Server_Client_Configuration_Update, InstanceID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID, NULL, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice));
                        if(EventData)
                        {
                           /* Format the rest of the event.             */
                           EventData->Event_Data_Size                                                          = BAS_CLIENT_CONFIGURATION_UPDATE_DATA_SIZE;
                           EventData->Event_Data.BAS_Client_Configuration_Update_Data->ClientConfigurationType = ctBatteryLevel;

                           /* Attempt to decode the request Client      */
                           /* Configuration.                            */
                           if(!DecodeClientConfigurationValue(ValueLength, Value, &(EventData->Event_Data.BAS_Client_Configuration_Update_Data->Notify)))
                           {
                              /* Go ahead and accept the write request  */
                              /* since we have decoded the Client       */
                              /* Configuration Value successfully.      */
                              GATT_Write_Response(BluetoothStackID, TransactionID);

                              /* Dispatch the event.                    */
                              __BTPSTRY
                              {
                                 (*ServiceInstance->EventCallback)(ServiceInstance->BluetoothStackID, EventData, ServiceInstance->CallbackParameter);
                              }
                              __BTPSEXCEPT(1)
                              {
                                 /* Do Nothing.                         */
                              }
                           }
                           else
                              GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_REQUEST_NOT_SUPPORTED);
                        }
                        else
                           GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR);
                     }
                     else
                        GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_WRITE_NOT_PERMITTED);
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

   /* The following function is responsible for making sure that the    */
   /* Bluetooth Stack BAS Module is Initialized correctly.  This        */
   /* function *MUST* be called before ANY other Bluetooth Stack BAS    */
   /* function can be called.  This function returns non-zero if the    */
   /* Module was initialized correctly, or a zero value if there was an */
   /* error.                                                            */
   /* * NOTE * Internally, this module will make sure that this function*/
   /*          has been called at least once so that the module will    */
   /*          function.  Calling this function from an external        */
   /*          location is not necessary.                               */
int InitializeBASModule(void)
{
   return((int)InitializeModule());
}

   /* The following function is responsible for instructing the         */
   /* Bluetooth Stack BAS Module to clean up any resources that it has  */
   /* allocated.  Once this function has completed, NO other Bluetooth  */
   /* Stack BAS Functions can be called until a successful call to the  */
   /* InitializeBASModule() function is made.  The parameter to this    */
   /* function specifies the context in which this function is being    */
   /* called.  If the specified parameter is TRUE, then the module will */
   /* make sure that NO functions that would require waiting/blocking on*/
   /* Mutexes/Events are called.  This parameter would be set to TRUE if*/
   /* this function was called in a context where threads would not be  */
   /* allowed to run.  If this function is called in the context where  */
   /* threads are allowed to run then this parameter should be set to   */
   /* FALSE.                                                            */
void CleanupBASModule(Boolean_t ForceCleanup)
{
   /* Check to make sure that this module has been initialized.         */
   if(InstanceListInitialized)
   {
      /* Wait for access to the BAS Context List.                       */
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

   /* BAS Server API.                                                   */

   /* The following function is responsible for opening a BAS Server.   */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The final parameter is a      */
   /* pointer to store the GATT Service ID of the registered BAS        */
   /* service.  This can be used to include the service registered by   */
   /* this call.  This function returns the positive, non-zero, Instance*/
   /* ID or a negative error code.                                      */
   /* * NOTE * Only 1 BAS Server may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
int BTPSAPI BAS_Initialize_Service(unsigned int BluetoothStackID, BAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID)
{
   GATT_Attribute_Handle_Group_t ServiceHandleRange;

    /* Initialize the Service Handle Group to 0.                        */
   ServiceHandleRange.Starting_Handle = 0;
   ServiceHandleRange.Ending_Handle   = 0;

   /* Simply wrap the internal function to do all of the work.          */
   return(BASRegisterService(BluetoothStackID, EventCallback, CallbackParameter, ServiceID, &ServiceHandleRange, NULL));
}

   /* The following function is responsible for opening a BAS Server.   */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The fourth parameter is a     */
   /* pointer to store the GATT Service ID of the registered BAS        */
   /* service.  This can be used to include the service registered by   */
   /* this call.  The final parameter is a pointer, that on input can be*/
   /* used to control the location of the service in the GATT database, */
   /* and on ouput to store the service handle range.  This function    */
   /* returns the positive, non-zero, Instance ID or a negative error   */
   /* code.                                                             */
   /* * NOTE * Only 1 BAS Server may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
int BTPSAPI BAS_Initialize_Service_Handle_Range(unsigned int BluetoothStackID, BAS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange)
{
   return(BASRegisterService(BluetoothStackID, EventCallback, CallbackParameter, ServiceID, ServiceHandleRange, NULL));
}

   /* The following function is responsible for closing a previous BAS  */
   /* Server.  The first parameter is the Bluetooth Stack ID on which to*/
   /* close the server.  The second parameter is the InstanceID that was*/
   /* returned from a successful call to BAS_Initialize_Service().  This*/
   /* function returns a zero if successful or a negative return error  */
   /* code if an error occurs.                                          */
int BTPSAPI BAS_Cleanup_Service(unsigned int BluetoothStackID, unsigned int InstanceID)
{
   int                  ret_val;
   BASServerInstance_t *ServiceInstance = NULL;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID))
   {
      /* Acquire the specified BAS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Verify that the service is actually registered.             */
         if(ServiceInstance->ServiceID)
         {
            /* Call GATT to un-register the service.                    */
            GATT_Un_Register_Service(BluetoothStackID, ServiceInstance->ServiceID);

            /* mark the instance entry as being free.                   */
            BTPS_MemInitialize(ServiceInstance, 0, BAS_SERVER_INSTANCE_DATA_SIZE);

            /* return success to the caller.                            */
            ret_val = 0;
         }
         else
            ret_val = BAS_ERROR_INVALID_PARAMETER;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(BluetoothStackID);
      }
      else
         ret_val = BAS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

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
unsigned long BTPSAPI BAS_Suspend(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   Byte_t           *TempPtr         = NULL;
   Boolean_t         PerformSuspend;
   unsigned int      Index;
   unsigned long     MemoryUsed;
   unsigned long     TotalMemoryUsed = 0;
   BASSuspendInfo_t *SuspendInfo     = NULL;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize == 0) || ((Buffer) && (BufferSize >= (BAS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE))))
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
            SuspendInfo                              = (BASSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, BASSuspendInfo_t));

            /* Calculated the memory used for the header.               */
            MemoryUsed                               = (BAS_SUSPEND_INFO_SIZE(0) + (((Byte_t *)SuspendInfo) - ((Byte_t *)Buffer)));

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
            TotalMemoryUsed += (BAS_SUSPEND_INFO_SIZE(0));

            /* Flag that we are not performing the suspend, but instead */
            /* calculating the memory required for a suspend.           */
            PerformSuspend   = FALSE;
         }

         /* Loop and suspend all active services.                       */
         for(Index = 0; Index < (unsigned int)BAS_MAXIMUM_SUPPORTED_INSTANCES; Index++)
         {
            /* Check to see if this instance is currently registered.   */
            if((InstanceList[Index].BluetoothStackID == BluetoothStackID) && (InstanceList[Index].ServiceID))
            {
               /* Check to see if we are performing a suspend.          */
               if(PerformSuspend)
               {
                  /* Verify that we have memory left to perform the     */
                  /* suspend.                                           */
                  if(BufferSize >= BAS_SUSPEND_INSTANCE_INFO_SIZE(0))
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
   /* BAS_Suspend()).  This function accepts as input the Bluetooth     */
   /* Stack ID of the Bluetooth Stack that the Device is associated     */
   /* with.  The final two parameters are the buffer size and buffer    */
   /* that contains the memory that was used to collapse Bluetopia      */
   /* context into with a successfully call to BAS_Suspend().  This     */
   /* function returns ZERO on success or a negative error code.        */
int BTPSAPI BAS_Resume(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   int               ret_val;
   Byte_t           *TempPtr;
   unsigned int      Index;
   BASSuspendInfo_t *SuspendInfo;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize) && ((TempPtr = (Byte_t *)Buffer) != NULL))
   {
      /* Verify that the buffer size is large enough for BAS.           */
      if(BufferSize >= (BAS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, BASSuspendInfo_t)))
      {
         /* Subtract the header and alignment offset from the buffer    */
         /* size.                                                       */
         BufferSize  -= (BAS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, BASSuspendInfo_t));

         /* Align the buffer for the BASSuspendInfo_t structrue.        */
         SuspendInfo  = (BASSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, BASSuspendInfo_t));

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
         ret_val = BAS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   /* Feature not supported so just return 0.                           */
   return(0);

#endif
}

   /* The following function is responsible for querying the number of  */
   /* attributes that are contained in the BAS Service that is          */
   /* registered with a call to BAS_Initialize_Service().  This function*/
   /* returns the non-zero number of attributes that are contained in a */
   /* BAS Server or zero on failure.                                    */
unsigned int BTPSAPI BAS_Query_Number_Attributes(void)
{
   /* Simply return the number of attributes that are contained in a BAS*/
   /* service.                                                          */
   return(BATTERY_SERVICE_ATTRIBUTE_COUNT);
}

   /* The following function is responsible for responding to a BAS Read*/
   /* Battery Level Request.  The first parameter is the Bluetooth      */
   /* Stack ID of the Bluetooth Device.  The second parameter is the    */
   /* Transaction ID of the request.  The final parameter contains the  */
   /* Battery Level to send to the remote device.  This function returns*/
   /* a zero if successful or a negative return error code if an error  */
   /* occurs.                                                           */
int BTPSAPI BAS_Battery_Level_Read_Request_Response(unsigned int BluetoothStackID, unsigned int TransactionID, Byte_t BatteryLevel)
{
   int              ret_val;
   NonAlignedByte_t Battery_Level;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID))
   {
      /* Format the received data.                                      */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&Battery_Level, BatteryLevel);

      /* Send the battery level read response.                          */
      ret_val = GATT_Read_Response(BluetoothStackID, TransactionID, (unsigned int)NON_ALIGNED_BYTE_SIZE, &Battery_Level);
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for responding to a BAS Read*/
   /* Battery Level Request when an error occured.  The first parameter */
   /* is the Bluetooth Stack ID of the Bluetooth Device. The second     */
   /* parameter is the Transaction ID of the request.The final parameter*/
   /* contains the Error which occured during the Read operation.  This */
   /* function returns a zero if successful or a negative return error  */
   /* code if an error occurs.                                          */
int BTPSAPI BAS_Battery_Level_Read_Request_Error_Response(unsigned int BluetoothStackID, unsigned int TransactionID, Byte_t ErrorCode)
{
  int ret_val;

   /* Make sure the parameters passed to us are semi-valid.             */
  if((BluetoothStackID) && (TransactionID))
     ret_val = GATT_Error_Response(BluetoothStackID, TransactionID, BAS_BATTERY_LEVEL_ATTRIBUTE_OFFSET, ErrorCode);
  else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
  return (ret_val);
}

   /* The following function is responsible for getting the presentation*/
   /* format of Device battery level on the specified BAS Instance.     */
   /* The first parameter is the Bluetooth Stack ID of the Bluetooth    */
   /* Device.The second parameter is the InstanceID returned from a     */
   /* successful call to BAS_Initialize_Server().The final parameter is */
   /* a pointer to store the Battery Level presentation format of the   */
   /* specified BAS Instance.This function returns zero if successful or*/
   /* a negative return error code if an error occurs.                  */
int BTPSAPI BAS_Query_Characteristic_Presentation_Format(unsigned int BluetoothStackID, unsigned int InstanceID, BAS_Presentation_Format_Data_t *CharacteristicPresentationFormat)
{
#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

   int                  ret_val;
   BASServerInstance_t *ServiceInstance = NULL;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID) && (CharacteristicPresentationFormat))
   {
      /* Acquire the specified BAS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Read the battery level presentation format from the         */
         /* specified instance.                                         */
         CharacteristicPresentationFormat->Format      = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Format));
         CharacteristicPresentationFormat->Exponent    = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Exponent));
         CharacteristicPresentationFormat->Unit        = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Unit));
         CharacteristicPresentationFormat->NameSpace   = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.NameSpace));
         CharacteristicPresentationFormat->Description = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Description));

         /* Return success to the caller.                               */
         ret_val = 0;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
      else
         ret_val = BAS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}

   /* The following function is responsible for setting the presentation*/
   /* format of Device battery level on the specified BAS Instance.     */
   /* The first parameter is the Bluetooth Stack ID of the Bluetooth    */
   /* Device.The second parameter is the InstanceID returned from a     */
   /* successful call to BAS_Initialize_Server().The final parameter is */
   /* Battery Level presentation format to be set on the specified      */
   /* BAS Instance.This function returns zero if successful or          */
   /* a negative return error code if an error occurs.                  */
int BTPSAPI BAS_Set_Characteristic_Presentation_Format(unsigned int BluetoothStackID, unsigned int InstanceID, BAS_Presentation_Format_Data_t *CharacteristicPresentationFormat)
{
#if (BAS_MAXIMUM_SUPPORTED_INSTANCES > 1)

   int                  ret_val;
   BASServerInstance_t *ServiceInstance = NULL;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID))
   {
      /* Acquire the specified BAS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Assign the battery level presentation format to the         */
         /* specified instance.                                         */
         /* * NOTE * The Format/Exponent/Unit fields of the Battery     */
         /*          Level Characteristic Presentation Format descriptor*/
         /*          are constants that will be the same for all        */
         /*          instances of Battery Service.  Therefore we will   */
         /*          not allow the user to change these values.         */
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Format),      BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_FORMAT_UNSIGNED_BYTE_INTEGER);
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Exponent),    0);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Unit),        BAS_BATTERY_LEVEL_PRESENTATION_FORMAT_UNIT_PERCENTAGE);
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.NameSpace),   CharacteristicPresentationFormat->NameSpace);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(InstanceData[InstanceID-1].Battery_Level_Presentation_Format.Description), CharacteristicPresentationFormat->Description);

         /* Return success to the caller.                               */
         ret_val = 0;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
      else
         ret_val = BAS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}

   /* The following function is responsible for responding to a BAS Read*/
   /* Client Configuration Request.  The first parameter is the         */
   /* Bluetooth Stack ID of the Bluetooth Device.  The second parameter */
   /* is the InstanceID returned from a successful call to              */
   /* BAS_Initialize_Server().  The third is the Transaction ID of the  */
   /* request.  The final parameter contains the Client Configuration to*/
   /* send to the remote device.  This function returns a zero if       */
   /* successful or a negative return error code if an error occurs.    */
int BTPSAPI BAS_Read_Client_Configuration_Response(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int TransactionID, Word_t Client_Configuration)
{
   int                  ret_val;
   Word_t               ValueLength;
   NonAlignedWord_t     ClientConfiguration;
   BASServerInstance_t *ServiceInstance = NULL;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID) && (TransactionID))
   {
      /* Acquire the specified BAS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Format the Read Response.                                   */
         ValueLength = GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH;
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&ClientConfiguration, Client_Configuration);

         /* Send the response.                                          */
         ret_val = GATT_Read_Response(ServiceInstance->BluetoothStackID, TransactionID, (unsigned int)ValueLength, (Byte_t *)&ClientConfiguration);

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
      else
         ret_val = BAS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for sending a Battery Level */
   /* Status notification to a specified remote device.  The first      */
   /* parameter is the Bluetooth Stack ID of the Bluetooth Device.  The */
   /* second parameter is the InstanceID returned from a successful call*/
   /* to BAS_Initialize_Server().  The third parameter is the           */
   /* ConnectionID of the remote device to send the notification to.    */
   /* The final parameter contains the Battery Level to send to the     */
   /* remote device. This function returns a zero if successful or a    */
   /* negative return error code if an error occurs.                    */
int BTPSAPI BAS_Notify_Battery_Level(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int ConnectionID, Byte_t BatteryLevel)
{
   int                  ret_val;
   NonAlignedByte_t     Battery_Level;
   BASServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID) && (ConnectionID))
   {
      /* Acquire the specified BAS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Format the received data.                                   */
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&Battery_Level, BatteryLevel);

         /* Send the notification.                                      */
         ret_val = GATT_Handle_Value_Notification(ServiceInstance->BluetoothStackID, ServiceInstance->ServiceID, ConnectionID, BAS_BATTERY_LEVEL_ATTRIBUTE_OFFSET, NON_ALIGNED_BYTE_SIZE, &Battery_Level);
         if(ret_val == NON_ALIGNED_BYTE_SIZE)
            ret_val = 0;
         else
         {
            if(ret_val >= 0)
               ret_val = BAS_ERROR_UNKNOWN_ERROR;
         }

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
      else
         ret_val = BAS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = BAS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* BAS Client API                                                    */

   /* The following function is responsible for parsing a value received*/
   /* from a remote BAS Server interpreting it as characteristic        */
   /* persentation format of Battery Level. The first parameter is the  */
   /* length of the value returned by the remote BAS Server. The second */
   /* parameter is a pointer to the data returned by the remote BAS     */
   /* Server.The final parameter is a pointer to store the parsed       */
   /* Battery Level Presentation Format. This function returns a zero   */
   /* if successful or a negative return error code if an error occurs. */
int BTPSAPI BAS_Decode_Characteristic_Presentation_Format(unsigned int ValueLength, Byte_t *Value, BAS_Presentation_Format_Data_t *CharacteristicPresentationFormat)
{
   int ret_val = BAS_ERROR_MALFORMATTED_DATA;

   /* Verify that the input parameters are valid.                       */
   if((ValueLength == BAS_PRESENTATION_FORMAT_SIZE) && (Value) && (CharacteristicPresentationFormat))
   {
      /* Read the packed battery level presentation format Data.        */
      CharacteristicPresentationFormat->Format      = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((BAS_Presentation_Format_t *)Value)->Format));
      CharacteristicPresentationFormat->Exponent    = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((BAS_Presentation_Format_t *)Value)->Exponent));
      CharacteristicPresentationFormat->Unit        = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((BAS_Presentation_Format_t *)Value)->Unit));
      CharacteristicPresentationFormat->NameSpace   = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((BAS_Presentation_Format_t *)Value)->NameSpace));
      CharacteristicPresentationFormat->Description = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((BAS_Presentation_Format_t *)Value)->Description));

      /* Return success to the caller.                                  */
      ret_val = 0;
   }
   else
   {
      if(ValueLength == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH)
         ret_val = BAS_ERROR_INVALID_PARAMETER;
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}
