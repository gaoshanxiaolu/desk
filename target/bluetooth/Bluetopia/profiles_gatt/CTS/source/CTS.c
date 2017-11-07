/*****< cts.c >****************************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  CTS - Bluetooth Stack Current Time Service (GATT Based) for Stonestreet   */
/*        One Bluetooth Protocol Stack.                                       */
/*                                                                            */
/*  Author:  Ajay Parashar                                                    */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   06/28/12  A. Parashar    Initial creation.                               */
/*   11/29/12  T. Cook        Fixed naming conventions and comments.          */
/*   06/09/16  R. McCord      Added Suspend/Resume support.                   */
/******************************************************************************/
#include "SS1BTPS.h"        /* Bluetooth Stack API Prototypes/Constants.      */
#include "SS1BTGAT.h"       /* Bluetooth Stack GATT API Prototypes/Constants. */
#include "SS1BTCTS.h"       /* Bluetooth CTS API Prototypes/Constants.        */

#include "BTPSKRNL.h"       /* BTPS Kernel Prototypes/Constants.              */
#include "CTS.h"            /* Bluetooth CTS Prototypes/Constants.            */

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   #include "BTPSALIGN.h"   /* Alignment Related Constants.                   */
#endif

 /* The following controls the number of supported CTS instances.       */
#define CTS_MAXIMUM_SUPPORTED_INSTANCES                 (BTPS_CONFIGURATION_CTS_MAXIMUM_SUPPORTED_INSTANCES)

   /* The following structure defines the Instance Data that must be    */
   /* unique for each CTS service registered (Only 1 per Bluetooth      */
   /* Stack).                                                           */
typedef __PACKED_STRUCT_BEGIN__ struct _tagCTS_Instance_Data_t
{
  NonAlignedWord_t   Current_Time_Length;
  CTS_Current_Time_t Current_Time;

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

  NonAlignedWord_t             Local_Time_Information_Length;
  CTS_Local_Time_Information_t Local_Time_Information;

#endif

} __PACKED_STRUCT_END__ CTS_Instance_Data_t;

#define CTS_INSTANCE_DATA_SIZE                           (sizeof(CTS_Instance_Data_t))

   /* The following define the instance tags for each CTS service data  */
   /* that is unique per registered service.                            */
#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION
   #define CTS_LOCAL_TIME_INFORMATION_INSTANCE_TAG       (BTPS_STRUCTURE_OFFSET(CTS_Instance_Data_t, Local_Time_Information_Length))
#endif

   /*********************************************************************/
   /**               Current Time Service Table                         */
   /*********************************************************************/

   /* The Current Time Service Declaration UUID.                        */
static BTPSCONST GATT_Primary_Service_16_Entry_t CTS_Service_UUID =
{
   CTS_SERVICE_BLUETOOTH_UUID_CONSTANT
};

   /* The Current Time Characteristic Declaration.                      */
static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t CTS_Current_Time_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY),
   CTS_CURRENT_TIME_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t CTS_Current_Time_Declaration_ReadWrite =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ | GATT_CHARACTERISTIC_PROPERTIES_WRITE | GATT_CHARACTERISTIC_PROPERTIES_NOTIFY),
   CTS_CURRENT_TIME_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Current Time Characteristic Value.                            */
static BTPSCONST GATT_Characteristic_Value_16_Entry_t  CTS_Current_Time_Value =
{
   CTS_CURRENT_TIME_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};

   /* Client Characteristic Configuration Descriptor.                   */
static GATT_Characteristic_Descriptor_16_Entry_t Client_Characteristic_Configuration =
{
   GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_BLUETOOTH_UUID_CONSTANT,
   GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH,
   NULL
};

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

  /* The Local Time Information Characteristic Declaration.             */
static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t CTS_Local_Time_Information_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_READ,
   CTS_LOCAL_TIME_INFORMATION_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t CTS_Local_Time_Information_Declaration_ReadWrite =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ | GATT_CHARACTERISTIC_PROPERTIES_WRITE),
   CTS_LOCAL_TIME_INFORMATION_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Local Time Information Characteristic Value.                  */
static BTPSCONST GATT_Characteristic_Value_16_Entry_t  CTS_Local_Time_Information_Value =
{
   CTS_LOCAL_TIME_INFORMATION_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   CTS_LOCAL_TIME_INFORMATION_INSTANCE_TAG,
   NULL
};

#endif

#if BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION

  /* The Reference Time Information Characteristic Declaration.         */
static BTPSCONST GATT_Characteristic_Declaration_16_Entry_t CTS_Reference_Time_Information_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_READ,
   CTS_REFERENCE_TIME_INFORMATION_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT
};

   /* The Reference Time Information Characteristic Value.              */
static BTPSCONST GATT_Characteristic_Value_16_Entry_t  CTS_Reference_Time_Information_Value =
{
   CTS_REFERENCE_TIME_INFORMATION_CHARACTERISTIC_BLUETOOTH_UUID_CONSTANT,
   0,
   NULL
};

#endif

 /* The following base offsets are used to calculate the correct        */
 /* attribute even if certain features are compiled out of the module.  */
#define ATTRIBUTE_OFFSET_BASE_0                          0
#define ATTRIBUTE_OFFSET_BASE_1                          0

  /* The following defines the Current Time Service that is registered  */
  /* with the GATT_Register_Service function call.                      */
  /* * NOTE * This array will be registered with GATT in the call to    */
  /*          GATT_Register_Service.                                    */
static GATT_Service_Attribute_Entry_t Current_Time_Service[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService16,            (Byte_t *)&CTS_Service_UUID},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration16, (Byte_t *)&CTS_Current_Time_Declaration},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicValue16,       (Byte_t *)&CTS_Current_Time_Value},
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,  (Byte_t *)&Client_Characteristic_Configuration},

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration16, (Byte_t *)&CTS_Local_Time_Information_Declaration},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicValue16,       (Byte_t *)&CTS_Local_Time_Information_Value},

   /* If we are including support for the Local time                    */
   /* characteristic we must define the Attribute Offset Base 0 to be 2 */
   /* so that the attribute offset defines are not thrown off.          */
#undef ATTRIBUTE_OFFSET_BASE_0
#define ATTRIBUTE_OFFSET_BASE_0             2

#endif

#if BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION

   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration16, (Byte_t *)&CTS_Reference_Time_Information_Declaration},
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicValue16,       (Byte_t *)&CTS_Reference_Time_Information_Value},

   /* If we are including support for the Local time                    */
   /* characteristic we must define the Attribute Offset Base 0 to be 2 */
   /* so that the attribute offset defines are not thrown off.          */
   #undef ATTRIBUTE_OFFSET_BASE_1

   #if   BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION
      #define ATTRIBUTE_OFFSET_BASE_1             4
   #else
      #define ATTRIBUTE_OFFSET_BASE_1             2
   #endif

#endif

};

#define CURRENT_TIME_SERVICE_ATTRIBUTE_COUNT                      (sizeof(Current_Time_Service)/sizeof(GATT_Service_Attribute_Entry_t))

#define CTS_CURRENT_TIME_ATTRIBUTE_OFFSET                         2
#define CTS_CURRENT_TIME_CCD_ATTRIBUTE_OFFSET                     3

#ifdef BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION
   #define CTS_LOCAL_TIME_INFORMATION_ATTRIBUTE_OFFSET            (3 + ATTRIBUTE_OFFSET_BASE_0)
#endif

#if ((BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION) && (BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION))
   #define CTS_REFERENCE_TIME_INFORMATION_ATTRIBUTE_OFFSET        (3 + ATTRIBUTE_OFFSET_BASE_1)
#elif BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION
   #define CTS_REFERENCE_TIME_INFORMATION_ATTRIBUTE_OFFSET        (3 + ATTRIBUTE_OFFSET_BASE_1)
#endif

   /*********************************************************************/
   /**                    END OF SERVICE TABLE                         **/
   /*********************************************************************/

   /* The following MACRO is a utility MACRO that exists to valid that a*/
   /* specified Week Day is valid.  The only parameter to this function */
   /* is the CTS_Week_Day_Type_t enumerated type to valid.  This MACRO  */
   /* returns TRUE if the Day of week is valid or FALSE otherwise.      */
#define CTS_CURRENT_TIME_VALID_DAY_OF_WEEK(_x)                                      (((_x) >= wdUnknown) && ((_x) <= wdSunday))

   /* The following MACRO is a utility MACRO that exists to valid that a*/
   /* specified Current Time (CTS_Current_Time_Data_t structure) is     */
   /* valid.  The only parameter to this MACRO is the                   */
   /* CTS_Current_Time_Data_t structure to valid.  This MACRO returns   */
   /* TRUE if the Current Time is valid or FALSE otherwise.             */
#define CTS_CURRENT_TIME_VALID(_x)                                                  ((CTS_CURRENT_TIME_VALID_DAY_OF_WEEK((_x).Exact_Time.Day_Date_Time.Day_Of_Week)) && (CTS_DATE_TIME_VALID((_x).Exact_Time.Day_Date_Time.Date_Time)))

   /* The following MACRO is a utility MACRO that exists to determine if*/
   /* the Time Zone is valid.                                           */
#define CTS_LOCAL_TIME_INFORMATION_TIME_ZONE_VALID(_x)                              ((((_x) >= tzUTCMinus1200) && ((_x) <= tzUTCPlus1400)) || ((_x) == tzUTCUnknown))

   /* The following MACRO is a UTILITY MACRO that exists to determine if*/
   /* the Local Time Information DST Offset is valid.                   */
#define CTS_LOCAL_TIME_INFORMATION_VALID_DST_OFFSET(_x)                             (((_x) == doStandardTime) || ((_x) == doHalfAnHourDaylightTime) || ((_x) == doDaylightTime) || ((_x) == doDoubleDaylightTime) || ((_x) == doUnknown))

   /* The following MACRO is a utility MACRO that exists to valid that a*/
   /* specified Local Time Information                                  */
   /* (CTS_Local_Time_Information_Data_t structure) is valid.  The only */
   /* parameter to this MACRO is the CTS_Local_Time_Information_Data_t  */
   /* structure to valid.  This MACRO returns TRUE if the Local Time is */
   /* valid or FALSE otherwise.                                         */
#define CTS_LOCAL_TIME_INFORMATION_VALID(_x)                                        ((CTS_LOCAL_TIME_INFORMATION_TIME_ZONE_VALID(((_x)).Time_Zone)) && (CTS_LOCAL_TIME_INFORMATION_VALID_DST_OFFSET(((_x)).Daylight_Saving_Time)))

   /* The following MACRO is a utility MACRO that exists to determine if*/
   /* the Reference Time Information Source is valid.                   */
#define CTS_REFERENCE_TIME_INFORMATION_VALID_REFERENCE_TIME_SOURCE(_x)              (((_x) >= tsUnknown) && ((_x) <= tsCellularNetwork))

   /* The following MACRO is a utility MACRO that exists to determine if*/
   /* the Reference Time Information Hours Since Update field is valid. */
#define CTS_REFERENCE_TIME_INFORMATION_VALID_REFERENCE_TIME_HOURS_SINCE_UPDATE(_x)  ((((_x) <= 23)) || ((_x) == 255))

   /* The following MACRO is a utility MACRO that exists to valid that a*/
   /* specified Reference Time Information                              */
   /* (CTS_Reference_Time_Information_Data_t structure) is valid.  The  */
   /* only parameter to this MACRO is the                               */
   /* CTS_Reference_Time_Information_Data_t structure to valid.  This   */
   /* MACRO returns TRUE if the Local Time is valid or FALSE otherwise. */
#define CTS_REFERENCE_TIME_INFORMATION_VALID(_x)                                    ((CTS_REFERENCE_TIME_INFORMATION_VALID_REFERENCE_TIME_SOURCE(((_x)).Source)) && (CTS_REFERENCE_TIME_INFORMATION_VALID_REFERENCE_TIME_HOURS_SINCE_UPDATE(((_x)).Hours_Since_Update)))

   /* The following type defines a union large enough to hold all events*/
   /* dispatched by this module.                                        */
typedef union
{
   CTS_Read_Client_Configuration_Data_t            Read_Client_Data;
   CTS_Client_Configuration_Update_Data_t          Client_Configuration_Update_Data;
   CTS_Reference_Time_Information_Data_t           CTS_Reference_Time_Information_Data;
   CTS_Read_Current_Time_Request_Data_t            CTS_Read_Current_Time_Request_Data;
   CTS_Write_Current_Time_Request_Data_t           CTS_Write_Current_Time_Request_Data;
   CTS_Write_Local_Time_Information_Request_Data_t CTS_Write_Local_Time_Information_Request_Data;
} CTS_Event_Data_Buffer_t;

#define CTS_EVENT_DATA_BUFFER_SIZE                      (sizeof(CTS_Event_Data_Buffer_t))

   /* CTS Service Instance Block.  This structure contains All          */
   /* information associated with a specific Bluetooth Stack ID (member */
   /* is present in this structure).                                    */
typedef struct _tagCTSServerInstance_t
{
   unsigned int                  BluetoothStackID;
   unsigned int                  ServiceID;
   CTS_Event_Callback_t          EventCallback;
   unsigned long                 CallbackParameter;

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
#endif

#if BTPS_CONFIGURATION_GATT_SUPPORT_BR_EDR
   DWord_t                       SDPServiceRecordHandle;
#endif

} CTSServerInstance_t;

#define CTS_SERVER_INSTANCE_DATA_SIZE                    (sizeof(CTSServerInstance_t))

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   /* The following structure contains the information that is          */
   /* stored/restored in CTS Service Instance when a suspend/resume     */
   /* procedure is performed.                                           */
typedef struct CTSSuspendInstanceInfo_s
{
   GATT_Attribute_Handle_Group_t InstanceHandleRange;
   unsigned int                  InstanceID;
   CTS_Event_Callback_t          EventCallback;
   unsigned long                 CallbackParameter;
   Byte_t                        VariableData[1];
} CTSSuspendInstanceInfo_t;

#define CTS_SUSPEND_INSTANCE_INFO_SIZE(_x)               (BTPS_STRUCTURE_OFFSET(CTSSuspendInstanceInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

   /* The following structure contains the information that is          */
   /* stored/restored in CTS Service when a suspend/resume procedure is */
   /* performed.                                                        */
typedef struct CTSSuspendInfo_s
{
   unsigned int NumberOfSuspendedInstances;
   Byte_t       VariableData[1];
} CTSSuspendInfo_t;

#define CTS_SUSPEND_INFO_SIZE(_x)                        (BTPS_STRUCTURE_OFFSET(CTSSuspendInfo_t, VariableData) + ((_x)*sizeof(Byte_t)))

#endif

 /* Internal Variables to this Module (Remember that all variables      */
 /* declared static are initialized to 0 automatically by the compiler  */
 /* as part of standard C/C++).                                         */

static CTS_Instance_Data_t InstanceData[CTS_MAXIMUM_SUPPORTED_INSTANCES];
                                            /* Variable which holds all */
                                            /* data that is unique for  */
                                            /* each service instance.   */

static CTSServerInstance_t InstanceList[CTS_MAXIMUM_SUPPORTED_INSTANCES];
                                            /* Variable which holds the */
                                            /* service instance data.   */

static Boolean_t InstanceListInitialized;   /* Variable that flags that */
                                            /* is used to denote that   */
                                            /* this module has been     */
                                            /* successfully initialized.*/
   /* The following are the prototypes of local functions.              */
static Boolean_t InitializeModule(void);
static void CleanupModule(void);

static int DecodeClientConfigurationValue(unsigned int BufferLength, Byte_t *Buffer, Word_t *ClientConfiguration);
static int FormatCurrentTime(CTS_Current_Time_Data_t *Current_Time, unsigned int BufferLength, Byte_t *Buffer);
static int FormatReferenceTimeInformation(CTS_Reference_Time_Information_Data_t *Reference_Time, unsigned int BufferLength, Byte_t *Buffer);
static CTS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, CTS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, unsigned int *TransactionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR);

static Boolean_t InstanceRegisteredByStackID(unsigned int BluetoothStackID);
static CTSServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID);

#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME
   static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer);
   static int ResumeServiceInstance(unsigned int BluetoothStackID, unsigned long *BufferSize, Byte_t **Buffer);
#endif

static int CTSRegisterService(unsigned int BluetoothStackID, unsigned int Flags, CTS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID);

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
  /* Flag that we are no longer initialized.                            */
  InstanceListInitialized = FALSE;
}

   /* The following function is a utility function that exists to decode*/
   /* an Client Configuration value into a user specified boolean value.*/
   /* This function returns the zero if successful or a negative error  */
   /* code.                                                             */
static int DecodeClientConfigurationValue(unsigned int BufferLength, Byte_t *Buffer, Word_t *ClientConfiguration)
{
   int ret_val = CTS_ERROR_MALFORMATTED_DATA;

   /* Verify that the input parameters are valid.                       */
   if(((BufferLength == NON_ALIGNED_BYTE_SIZE) || (BufferLength == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH)) && (Buffer) && (ClientConfiguration))
   {
      /* Read the requested Client Configuration.                       */
      if(BufferLength == NON_ALIGNED_BYTE_SIZE)
         *ClientConfiguration = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(Buffer);
      else
         *ClientConfiguration = READ_UNALIGNED_WORD_LITTLE_ENDIAN(Buffer);

      ret_val              = 0;
   }
   else
   {
      if(BufferLength == GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH)
         ret_val = CTS_ERROR_INVALID_PARAMETER;
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is a utility function that exists to       */
   /* format a Current Time Command into a user specified buffer.       */
static int FormatCurrentTime(CTS_Current_Time_Data_t *Current_Time, unsigned int BufferLength, Byte_t *Buffer)
{
   int ret_val;

   /* Verify that the input parameters are valid.                       */
   if((Current_Time) && (CTS_CURRENT_TIME_VALID(*Current_Time)) && (BufferLength >= CTS_CURRENT_TIME_SIZE) && (Buffer))
   {
      /* Format the Date Time of the Current Time information.          */
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Day_Date_Time.Date_Time.Year),    Current_Time->Exact_Time.Day_Date_Time.Date_Time.Year)
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Day_Date_Time.Date_Time.Month),   Current_Time->Exact_Time.Day_Date_Time.Date_Time.Month);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Day_Date_Time.Date_Time.Day),     Current_Time->Exact_Time.Day_Date_Time.Date_Time.Day);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Day_Date_Time.Date_Time.Hours),   Current_Time->Exact_Time.Day_Date_Time.Date_Time.Hours);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Day_Date_Time.Date_Time.Minutes), Current_Time->Exact_Time.Day_Date_Time.Date_Time.Minutes);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Day_Date_Time.Date_Time.Seconds), Current_Time->Exact_Time.Day_Date_Time.Date_Time.Seconds);

      /* Format the Day of the Week field of the Current Time.          */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Day_Date_Time.Day_Of_Week), Current_Time->Exact_Time.Day_Date_Time.Day_Of_Week);

      /* Format the Fractions 1/256 seconds field of the Current Time.  */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Exact_Time.Fractions256), Current_Time->Exact_Time.Fractions256);

      /* Finally, format the Adjust Reason Mask of the Current Time.    */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Current_Time_t*)Buffer)->Adjust_Reason_Mask), Current_Time->Adjust_Reason_Mask);

      /* Return success to the caller.                                  */
      ret_val = 0;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is a utility function that exists to       */
   /* format a Reference Time Information into a user specified buffer. */
static int FormatReferenceTimeInformation(CTS_Reference_Time_Information_Data_t *Reference_Time, unsigned int BufferLength, Byte_t *Buffer)
{
   int ret_val;

   /* Verify that the input parameters are valid.                       */
   if((Reference_Time) && (BufferLength >= CTS_REFERENCE_TIME_INFORMATION_SIZE) && (Buffer))
   {
      /* Assign the Reference Time Information for the specified        */
      /* instance.                                                      */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Reference_Time_Information_t *)Buffer)->Source), (Byte_t)Reference_Time->Source);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Reference_Time_Information_t *)Buffer)->Accuracy), Reference_Time->Accuracy);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Reference_Time_Information_t *)Buffer)->Days_Since_Update), Reference_Time->Days_Since_Update);
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Reference_Time_Information_t *)Buffer)->Hours_Since_Update), Reference_Time->Hours_Since_Update);

      /* Return success to the caller.                                  */
      ret_val = 0;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is a utility function that exists to format*/
   /* a CTS Event into the specified buffer.                            */
   /* * NOTE * TransactionID is optional and may be set to NULL.        */
   /* * NOTE * BD_ADDR is NOT optional and may NOT be set to NULL.      */
static CTS_Event_Data_t *FormatEventHeader(unsigned int BufferLength, Byte_t *Buffer, CTS_Event_Type_t EventType, unsigned int InstanceID, unsigned int ConnectionID, unsigned int *TransactionID, GATT_Connection_Type_t ConnectionType, BD_ADDR_t *BD_ADDR)
{
   CTS_Event_Data_t *EventData = NULL;

   if((BufferLength >= (CTS_EVENT_DATA_SIZE + CTS_EVENT_DATA_BUFFER_SIZE)) && (Buffer) && (BD_ADDR))
   {
      /* Format the header of the event, that is data that is common to */
      /* all events.                                                    */
      BTPS_MemInitialize(Buffer, 0, BufferLength);

      EventData                                                              = (CTS_Event_Data_t *)Buffer;
      EventData->Event_Data_Type                                             = EventType;
      EventData->Event_Data.CTS_Read_Client_Configuration_Data               = (CTS_Read_Client_Configuration_Data_t *)(((Byte_t *)EventData) + CTS_EVENT_DATA_SIZE);
      EventData->Event_Data.CTS_Read_Client_Configuration_Data->InstanceID   = InstanceID;
      EventData->Event_Data.CTS_Read_Client_Configuration_Data->ConnectionID = ConnectionID;

      if(TransactionID)
      {
         EventData->Event_Data.CTS_Read_Client_Configuration_Data->TransactionID  = *TransactionID;
         EventData->Event_Data.CTS_Read_Client_Configuration_Data->ConnectionType = ConnectionType;
         EventData->Event_Data.CTS_Read_Client_Configuration_Data->RemoteDevice   = *BD_ADDR;
      }
      else
      {
         EventData->Event_Data.CTS_Client_Configuration_Update_Data->ConnectionType = ConnectionType;
         EventData->Event_Data.CTS_Client_Configuration_Update_Data->RemoteDevice   = *BD_ADDR;
      }
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

   for(Index=0;Index<CTS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
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
static CTSServerInstance_t *AcquireServiceInstance(unsigned int BluetoothStackID, unsigned int *InstanceID)
{
   unsigned int         LocalInstanceID;
   unsigned int         Index;
   CTSServerInstance_t *ret_val = NULL;

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
         if((LocalInstanceID) && (LocalInstanceID <= CTS_MAXIMUM_SUPPORTED_INSTANCES))
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
               for(Index=0;Index<CTS_MAXIMUM_SUPPORTED_INSTANCES;Index++)
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
   /* required for suspend) a given CTS instance.  It returns the number*/
   /* of bytes used (or required for) a suspend.                        */
   /* * NOTE * Internal function no check needed on input parameters.   */
static unsigned long SuspendServiceInstance(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned long BufferSize, Byte_t *Buffer)
{
   unsigned int              Offset;
   unsigned long             RequiredMemory = 0;
   CTSSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Instance ID is always +1 to be always positive.                   */
   InstanceID--;

   /* Calculate the memory required for a successful suspend.           */

   /* Include the size of the CTS Current Time.                         */
   RequiredMemory += (NON_ALIGNED_WORD_SIZE + CTS_CURRENT_TIME_SIZE);

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

   /* Include the size of the CTS Local Time.                           */
   RequiredMemory += (NON_ALIGNED_WORD_SIZE + CTS_LOCAL_TIME_INFORMATION_SIZE);

#endif

   /* Check to see if we need to save the required instance.            */
   if(BufferSize)
   {
      /* Verify that the buffer is big enough.                          */
      if((Buffer) && (BufferSize >= (CTS_SUSPEND_INSTANCE_INFO_SIZE(RequiredMemory) + BTPS_ALIGNMENT_SIZE)))
      {
         /* Get an aligned pointer to where we will store the suspend   */
         /* information.                                                */
         SuspendInstanceInfo                      = (CTSSuspendInstanceInfo_t *)(Buffer + BTPS_ALIGNMENT_OFFSET(Buffer, CTSSuspendInstanceInfo_t));

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

         /* Copy the CTS Current Time into the suspend info.            */
         BTPS_MemCopy(&(SuspendInstanceInfo->VariableData[Offset]), &(InstanceData[InstanceID]), (NON_ALIGNED_WORD_SIZE + CTS_CURRENT_TIME_SIZE));
         Offset += (NON_ALIGNED_WORD_SIZE + CTS_CURRENT_TIME_SIZE);

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

         /* Copy the CTS Local Time into the suspend info.              */
         BTPS_MemCopy(&(SuspendInstanceInfo->VariableData[Offset]), &(InstanceData[InstanceID].Local_Time_Information_Length), (NON_ALIGNED_WORD_SIZE + CTS_LOCAL_TIME_INFORMATION_SIZE));
         Offset += (NON_ALIGNED_WORD_SIZE + CTS_LOCAL_TIME_INFORMATION_SIZE);

#endif

         /* Calculate the actual required amount of memory used.        */
         RequiredMemory = (CTS_SUSPEND_INSTANCE_INFO_SIZE(Offset) + BTPS_ALIGNMENT_OFFSET(Buffer, CTSSuspendInstanceInfo_t));

         /* Mark the instance entry as being free.                      */
         BTPS_MemInitialize(&(InstanceList[InstanceID]), 0, CTS_SERVER_INSTANCE_DATA_SIZE);
      }
      else
         RequiredMemory = 0;
   }
   else
   {
      /* Add the memory for the header plus the alignment size so we can*/
      /* align the received buffer.                                     */
      RequiredMemory += (CTS_SUSPEND_INSTANCE_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE);
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
   CTSSuspendInstanceInfo_t *SuspendInstanceInfo;

   /* Make sure we have enough memory for an instance header.           */
   if((TempBuffer) && (TempBufferSize >= CTS_SUSPEND_INSTANCE_INFO_SIZE(0)))
   {
      /* Calculate the alignment offset for the buffer.                 */
      AlignmentOffset = BTPS_ALIGNMENT_OFFSET(TempBuffer, CTSSuspendInstanceInfo_t);

      /* Make sure there is enough buffer space to align the buffer.    */
      if(TempBufferSize >= (CTS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset))
      {
         /* Align the temp buffer pointer.                              */
         SuspendInstanceInfo  = (CTSSuspendInstanceInfo_t *)(TempBuffer + AlignmentOffset);

         /* Set temp buffer to the variable data.                       */
         TempBuffer           = SuspendInstanceInfo->VariableData;

         /* Subtract the header from the buffer size.                   */
         TempBufferSize      -= (CTS_SUSPEND_INSTANCE_INFO_SIZE(0) + AlignmentOffset);

         /* Verify that the requested InstanceID is not being used      */
         /* currently.                                                  */
         if((SuspendInstanceInfo->InstanceID) && (SuspendInstanceInfo->InstanceID <= CTS_MAXIMUM_SUPPORTED_INSTANCES) && (!(InstanceList[SuspendInstanceInfo->InstanceID-1].ServiceID)))
         {
            /* Attempt to re-initialize the CTS service.                */
            ret_val = CTSRegisterService(BluetoothStackID, CTS_SERVICE_FLAGS_LE, SuspendInstanceInfo->EventCallback, SuspendInstanceInfo->CallbackParameter, &ServiceID, &(SuspendInstanceInfo->InstanceHandleRange), &(SuspendInstanceInfo->InstanceID));
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

               /* Verify that we have enough memory for the CTS Instance*/
               /* Data.                                                 */
               if(TempBufferSize >= (NON_ALIGNED_WORD_SIZE + CTS_CURRENT_TIME_SIZE))
               {
                  /* Copy the CTS Current Time data into the CTS        */
                  /* Instance Data (it is already formatted).           */
                  BTPS_MemCopy(&(InstanceData[InstanceID - 1]), TempBuffer, (NON_ALIGNED_WORD_SIZE + CTS_CURRENT_TIME_SIZE));

                  /* Update the pointers and the buffersize.            */
                  TempBufferSize -= (NON_ALIGNED_WORD_SIZE + CTS_CURRENT_TIME_SIZE);
                  TempBuffer     += (NON_ALIGNED_WORD_SIZE + CTS_CURRENT_TIME_SIZE);
               }
               else
                  ret_val = CTS_ERROR_MALFORMATTED_DATA;

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

               /* Verify that we have enough memory for the CTS Instance*/
               /* Data.                                                 */
               if(TempBufferSize >= (NON_ALIGNED_WORD_SIZE + CTS_LOCAL_TIME_INFORMATION_SIZE))
               {
                  /* Copy the suspended instance data into the CTS      */
                  /* Instance Data (it is already formatted).           */
                  BTPS_MemCopy(&(InstanceData[InstanceID - 1].Local_Time_Information_Length), TempBuffer, (NON_ALIGNED_WORD_SIZE + CTS_LOCAL_TIME_INFORMATION_SIZE));

                  /* Update the pointers and the buffersize.            */
                  TempBufferSize -= (NON_ALIGNED_WORD_SIZE + CTS_LOCAL_TIME_INFORMATION_SIZE);
                  TempBuffer     += (NON_ALIGNED_WORD_SIZE + CTS_LOCAL_TIME_INFORMATION_SIZE);
               }
               else
                  ret_val = CTS_ERROR_MALFORMATTED_DATA;

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
                  CTS_Cleanup_Service(BluetoothStackID, InstanceID);
               }
            }
         }
         else
            ret_val = CTS_ERROR_INVALID_PARAMETER;
      }
      else
         ret_val = CTS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

#endif

   /* The following function is a utility function which is used to     */
   /* register an CTS Service.  This function returns the positive,     */
   /* non-zero, Instance ID on success or a negative error code.        */
static int CTSRegisterService(unsigned int BluetoothStackID, unsigned int Flags, CTS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange, unsigned int *PreviousInstanceID)
{
   int                  ret_val;
   unsigned int         InstanceID;
   CTSServerInstance_t *ServiceInstance;

#if BTPS_CONFIGURATION_GATT_SUPPORT_BR_EDR

   GATT_SDP_Service_Record_t      SDPServiceRecord;
   SDP_UUID_Entry_t               SDPUUID;

#endif

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (EventCallback) && (ServiceID))
   {
      /* Verify that no instance is registered to this Bluetooth Stack. */
      if(!InstanceRegisteredByStackID(BluetoothStackID))
      {
         /* Acquire a free CTS Instance.                                */
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
               BTPS_MemInitialize(ServiceInstance, 0, CTS_SERVER_INSTANCE_DATA_SIZE);

               /* Store pointer to new instance.                        */
               ServiceInstance = &(InstanceList[*PreviousInstanceID - 1]);

               /* Save the InstanceID we should use from now on.        */
               InstanceID      = *PreviousInstanceID;
            }

#endif

            /* Determine if we need to update the CTS Service Table     */
            /* before registering it.  We may need to do this if GATT   */
            /* Write is supported by the application for the Current    */
            /* Time and Local Time Characteristics.  (CTS 1.1).         */

            if(Flags & CTS_FLAGS_SUPPORT_CURRENT_TIME_GATT_WRITE)
            {
               /* The characteristic declaration will be one off from   */
               /* the attribute offset.                                 */
               Current_Time_Service[CTS_CURRENT_TIME_ATTRIBUTE_OFFSET-1].Attribute_Value = (Byte_t *)&CTS_Current_Time_Declaration_ReadWrite;

               /* Now update the characteristic value.                  */
               Current_Time_Service[CTS_CURRENT_TIME_ATTRIBUTE_OFFSET].Attribute_Flags   = GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE;
            }

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

            if(Flags & CTS_FLAGS_SUPPORT_LOCAL_TIME_GATT_WRITE)
            {
               /* The characteristic declaration will be one off from   */
               /* the attribute offset.                                 */
               Current_Time_Service[CTS_LOCAL_TIME_INFORMATION_ATTRIBUTE_OFFSET-1].Attribute_Value = (Byte_t *)&CTS_Local_Time_Information_Declaration_ReadWrite;

               /* Now update the characteristic value.                  */
               Current_Time_Service[CTS_LOCAL_TIME_INFORMATION_ATTRIBUTE_OFFSET].Attribute_Flags   = GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE;
            }

#endif

            /* Call GATT to register the CTS service.                   */
            /* * NOTE * We will only allow GATT Service Flags for LE or */
            /*          BR/EDR to be passed to this function.           */
            ret_val = GATT_Register_Service(BluetoothStackID, (Byte_t)(Flags & CTS_SERVICE_FLAGS_DUAL_MODE), CURRENT_TIME_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)Current_Time_Service, ServiceHandleRange, GATT_ServerEventCallback, InstanceID);
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
               BTPS_MemInitialize(&InstanceData[InstanceID-1], 0, CTS_INSTANCE_DATA_SIZE);

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(InstanceData[InstanceID-1].Local_Time_Information_Length), CTS_LOCAL_TIME_INFORMATION_SIZE);
#endif

#if BTPS_CONFIGURATION_GATT_SUPPORT_BR_EDR

               /* Check to make sure the application supports BR/EDR.   */
               if(Flags & CTS_SERVICE_FLAGS_BR_EDR)
               {
                  SDPServiceRecord.NumberServiceClassUUID = 1;

                  SDPUUID.SDP_Data_Element_Type           = deUUID_16;
                  CTS_ASSIGN_CTS_SERVICE_SDP_UUID_16(SDPUUID.UUID_Value.UUID_16);

                  SDPServiceRecord.SDPUUIDEntries         = &SDPUUID;

                  /* Register the SDP Record.                           */
                  if((ret_val = GATT_Register_Service_SDP_Record(BluetoothStackID, &SDPServiceRecord, ServiceHandleRange, &(ServiceInstance->SDPServiceRecordHandle))) == 0)
                  {
                     /* Return the CTS Instance ID.                     */
                     ret_val = (int)InstanceID;
                  }
                  else
                  {
                     /* De-register the service since an error occured. */
                     GATT_Un_Register_Service(BluetoothStackID, ServiceInstance->ServiceID);
                  }
               }
               else
                  ret_val    = (int)InstanceID;

#else

               /* Return the CTS Instance ID.                           */
               ret_val       = (int)InstanceID;

#endif

            }

            /* UnLock the previously locked Bluetooth Stack.            */
            BSC_UnLockBluetoothStack(BluetoothStackID);
         }
         else
            ret_val = CTS_ERROR_INSUFFICIENT_RESOURCES;
      }
      else
         ret_val = CTS_ERROR_SERVICE_ALREADY_REGISTERED;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is the GATT Server Event Callback that     */
   /* handles all requests made to the CTS Service for all registered   */
   /* instances.                                                        */
static void BTPSAPI GATT_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
   Byte_t               ErrorCode = 0;
   Word_t               AttributeOffset;
   Word_t               ValueLength;
   Byte_t              *Value;
   Byte_t               Event_Buffer[CTS_EVENT_DATA_SIZE + CTS_EVENT_DATA_BUFFER_SIZE];
   unsigned int         TransactionID;
   unsigned int         InstanceID;
   CTS_Event_Data_t    *EventData;
   CTSServerInstance_t *ServiceInstance;

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

   Word_t               InstanceTag;

#endif

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
                     /* Check to see if the Read is for a Descriptor    */
                     /* (CCCD is the only descriptor in the table).     */
                     if(Current_Time_Service[AttributeOffset].Attribute_Entry_Type == aetCharacteristicDescriptor16)
                     {
                        /* Format the event header.                     */
                        EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etCTS_Server_Read_Client_Configuration_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice));
                        if(EventData)
                        {
                           /* Format the rest of the event.             */
                           EventData->Event_Data_Size = CTS_READ_CLIENT_CONFIGURATION_DATA_SIZE;

                           /* Determine the Correct Client Configuration*/
                           /* Type.                                     */
                           if(AttributeOffset == CTS_CURRENT_TIME_CCD_ATTRIBUTE_OFFSET)
                              EventData->Event_Data.CTS_Read_Client_Configuration_Data->ClientConfigurationType = ctCurrentTime;

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
                        if(AttributeOffset == CTS_CURRENT_TIME_ATTRIBUTE_OFFSET)
                        {
                           EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etCTS_Server_Read_Current_Time_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice));
                           if(EventData)
                           {
                              /* Format the rest of the event.          */
                              EventData->Event_Data_Size = CTS_READ_CURRENT_TIME_REQUEST_DATA_SIZE;

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
                        else
                        {

#if BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION

                           if(AttributeOffset == CTS_REFERENCE_TIME_INFORMATION_ATTRIBUTE_OFFSET)
                           {
                              EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etCTS_Server_Read_Reference_Time_Information_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice));
                              if(EventData)
                              {
                                 /* Format the rest of the event.       */
                                 EventData->Event_Data_Size = CTS_READ_REFERENCE_TIME_INFORMATION_REQUEST_DATA_SIZE;

                                 /* Dispatch the event.                 */
                                 __BTPSTRY
                                 {
                                    (*ServiceInstance->EventCallback)(ServiceInstance->BluetoothStackID, EventData, ServiceInstance->CallbackParameter);
                                 }
                                 __BTPSEXCEPT(1)
                                 {
                                    /* Do Nothing.                      */
                                 }

                              }
                           }
                           else
                           {

#endif

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

                              /* Get the instance tag for the Local Time*/
                              /* Characteristic.                        */
                              InstanceTag = (Word_t)(((GATT_Characteristic_Value_16_Entry_t *)Current_Time_Service[AttributeOffset].Attribute_Value)->Characteristic_Value_Length);
                              ValueLength = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((Byte_t *)(&InstanceData[InstanceID-1]))[InstanceTag]));
                              Value       = (Byte_t *)(&(((Byte_t *)(&InstanceData[InstanceID-1]))[InstanceTag + WORD_SIZE]));

                              /* Respond with the data.                 */
                              GATT_Read_Response(BluetoothStackID, TransactionID, (unsigned int)ValueLength, Value);

#else

                              /* Respond with error since we received an*/
                              /* unsupported attribute.                 */
                              GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_FOUND);

#endif

#if BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION

                           }

#endif

                        }
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
                     if(Current_Time_Service[AttributeOffset].Attribute_Entry_Type == aetCharacteristicDescriptor16)
                     {
                        /* Begin formatting the Client Configuration    */
                        /* Update event.                                */
                        EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etCTS_Server_Update_Client_Configuration_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID, NULL, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice));
                        if(EventData)
                        {
                           /* Format the rest of the event.             */
                           EventData->Event_Data_Size = CTS_CLIENT_CONFIGURATION_UPDATE_DATA_SIZE;

                           /* Determine the Correct Client Configuration*/
                           /* Type.                                     */
                           if(AttributeOffset == CTS_CURRENT_TIME_CCD_ATTRIBUTE_OFFSET)
                              EventData->Event_Data.CTS_Client_Configuration_Update_Data->ClientConfigurationType = ctCurrentTime;

                           /* Attempt to decode the request Client      */
                           /* Configuration.                            */
                           if(!DecodeClientConfigurationValue(ValueLength, Value, &(EventData->Event_Data.CTS_Client_Configuration_Update_Data->ClientConfiguration)))
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
                     {
                        /* Determine the Correct Attribute Offset for   */
                        /* writing.                                     */
                        switch(AttributeOffset)
                        {
                           case CTS_CURRENT_TIME_ATTRIBUTE_OFFSET:
                              EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etCTS_Server_Write_Current_Time_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice));
                              if(EventData)
                              {
                                 /* Format the rest of the event.       */
                                 EventData->Event_Data_Size = CTS_WRITE_CURRENT_TIME_REQUEST_DATA_SIZE;

                                 /* If the current time is malformatted */
                                 /* then we will go ahead and ignore the*/
                                 /* written time.                       */
                                 if(CTS_Decode_Current_Time(ValueLength, Value, &(EventData->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime)))
                                    ErrorCode = CTS_ERROR_CODE_DATA_FIELD_IGNORED;

                                 /* Verify that the processing of the   */
                                 /* request was successful.             */
                                 if(!ErrorCode)
                                 {
                                    /* Dispatch the event.              */
                                    __BTPSTRY
                                    {
                                       (*ServiceInstance->EventCallback)(BluetoothStackID, EventData, ServiceInstance->CallbackParameter);
                                    }
                                    __BTPSEXCEPT(1)
                                    {
                                       /* Do Nothing.                   */
                                    }
                                 }
                                 else
                                    GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ErrorCode);
                              }
                              else
                                 ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;
                              break;

#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

                           case CTS_LOCAL_TIME_INFORMATION_ATTRIBUTE_OFFSET:
                              EventData = FormatEventHeader(sizeof(Event_Buffer), Event_Buffer, etCTS_Server_Write_Local_Time_Information_Request, InstanceID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionID, &TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->ConnectionType, &(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice));
                              if(EventData)
                              {
                                 /* Format the rest of the event.       */
                                 EventData->Event_Data_Size = CTS_WRITE_LOCAL_TIME_INFORMATION_REQUEST_DATA_SIZE;

                                 /* If the local time information is    */
                                 /* malformatted then we will go ahead  */
                                 /* and ignore the written time.        */
                                 if(CTS_Decode_Local_Time_Information(ValueLength, Value, &(EventData->Event_Data.CTS_Write_Local_Time_Information_Request_Data->LocalTime)))
                                    ErrorCode = CTS_ERROR_CODE_DATA_FIELD_IGNORED;

                                 /* Verify that the processing of the   */
                                 /* request was successful.             */
                                 if(!ErrorCode)
                                 {
                                    /* Dispatch the event.              */
                                    __BTPSTRY
                                    {
                                       (*ServiceInstance->EventCallback)(BluetoothStackID, EventData, ServiceInstance->CallbackParameter);
                                    }
                                    __BTPSEXCEPT(1)
                                    {
                                       /* Do Nothing.                   */
                                    }
                                 }
                                 else
                                    GATT_Error_Response(BluetoothStackID, TransactionID, AttributeOffset, ErrorCode);
                              }
                              else
                                 ErrorCode = ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR;

                              break;

#endif

                           default:
                              ErrorCode = ATT_PROTOCOL_ERROR_CODE_REQUEST_NOT_SUPPORTED;
                              break;
                        }
                     }
                  }
               }
               break;
            default:
               /* Do nothing, as this is just here to get rid of        */
               /* warnings that some compilers flag when not all cases  */
               /* are handled in a switch off of a enumerated value.    */
               break;
         }

          /* UnLock the previously locked Bluetooth Stack.              */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
   }
}

   /* The following function is responsible for making sure that the    */
   /* Bluetooth Stack CTS Module is Initialized correctly.  This        */
   /* function *MUST* be called before ANY other Bluetooth Stack CTS    */
   /* function can be called.  This function returns non-zero if the    */
   /* Module was initialized correctly, or a zero value if there was an */
   /* error.                                                            */
   /* * NOTE * Internally, this module will make sure that this function*/
   /*          has been called at least once so that the module will    */
   /*          function.  Calling this function from an external        */
   /*          location is not necessary.                               */
int InitializeCTSModule(void)
{
   return((int)InitializeModule());
}

   /* The following function is responsible for instructing the         */
   /* Bluetooth Stack CTS Module to clean up any resources that it has  */
   /* allocated.  Once this function has completed, NO other Bluetooth  */
   /* Stack CTS Functions can be called until a successful call to the  */
   /* InitializeCTSModule() function is made.  The parameter to this    */
   /* function specifies the context in which this function is being    */
   /* called.  If the specified parameter is TRUE, then the module will */
   /* make sure that NO functions that would require waiting/blocking on*/
   /* Mutexes/Events are called.  This parameter would be set to TRUE if*/
   /* this function was called in a context where threads would not be  */
   /* allowed to run.  If this function is called in the context where  */
   /* threads are allowed to run then this parameter should be set to   */
   /* FALSE.                                                            */
void CleanupCTSModule(Boolean_t ForceCleanup)
{
   /* Check to make sure that this module has been initialized.         */
   if(InstanceListInitialized)
   {
      /* Wait for access to the CTS Context List.                       */
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

   /* CTS Server API.                                                   */

   /* The following function is responsible for opening a CTS Server.   */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The final parameter is a      */
   /* pointer to store the GATT Service ID of the registered CTS        */
   /* service.  This can be used to include the service registered by   */
   /* this call.  This function returns the positive, non-zero, Instance*/
   /* ID or a negative error code.                                      */
   /* * NOTE * Only 1 CTS Server may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
   /* * NOTE * This function does not support CTS 1.1 and will not allow*/
   /*          the optional writing of the Current Time and Local Time  */
   /*          Information Characteristics.                             */
   /* * NOTE * This function does not support CTS 1.1 and will not allow*/
   /*          registering CTS for the BR/EDR transport.                */
   /* * NOTE * If CTS 1.1 is needed, use CTS_Initialize_Service_Flags().*/
int BTPSAPI CTS_Initialize_Service(unsigned int BluetoothStackID, CTS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID)
{
   GATT_Attribute_Handle_Group_t ServiceHandleRange;

   /* Initialize the Service Handle Group to 0.                         */
   ServiceHandleRange.Starting_Handle = 0;
   ServiceHandleRange.Ending_Handle   = 0;

   /* Simply wrap the internal utility function to register the service */
   /* with the GATT layer.                                              */
   return(CTSRegisterService(BluetoothStackID, CTS_SERVICE_FLAGS_LE, EventCallback, CallbackParameter, ServiceID, &ServiceHandleRange, NULL));
}

   /* The following function is responsible for opening a CTS Server.   */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The fourth parameter is a     */
   /* pointer to store the GATT Service ID of the registered CTS        */
   /* service.  The final parameter is a pointer, that on input can be  */
   /* used to control the location of the service in the GATT database, */
   /* and on ouput to store the service handle range.  This can be used */
   /* to include the service registered by this call.  This function    */
   /* returns the positive, non-zero, Instance ID or a negative error   */
   /* code.                                                             */
   /* * NOTE * Only 1 CTS Server may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
   /* * NOTE * This function does not support CTS 1.1 and will not allow*/
   /*          the optional writing of the Current Time and Local Time  */
   /*          Information Characteristics.                             */
   /* * NOTE * This function does not support CTS 1.1 and will not allow*/
   /*          registering CTS for the BR/EDR transport.                */
   /* * NOTE * If CTS 1.1 is needed, use CTS_Initialize_Service_Flags().*/
int BTPSAPI CTS_Initialize_Service_Handle_Range(unsigned int BluetoothStackID, CTS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange)
{
   /* Simply wrapper the internal utility function to register the      */
   /* service with the GATT layer.                                      */
   return(CTSRegisterService(BluetoothStackID, CTS_SERVICE_FLAGS_LE, EventCallback, CallbackParameter, ServiceID, ServiceHandleRange, NULL));
}

   /* The following function is responsible for opening a CTS Server.   */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter will specify the CTS Flags          */
   /* (CTS_FLAGS_XXX).  These flags MUST be used to register GATT for   */
   /* LE, BR/EDR, or both.  These flags may also be used to enable      */
   /* optional service features (CTS 1.1) needed by the application.    */
   /* The third parameter is the Callback function to call when an event*/
   /* occurs on this Server Port.  The fourth parameter is a            */
   /* user-defined callback parameter that will be passed to the        */
   /* callback function with each event.  The fifth parameter is a      */
   /* pointer to store the GATT Service ID of the registered CTS        */
   /* service.  This can be used to include the service registered by   */
   /* this call.  The final parameter is an optional pointer, that on   */
   /* input can be used to control the location of the service in the   */
   /* GATT database, and on ouput to store the service handle range.    */
   /* This function returns the positive, non-zero, Instance ID or a    */
   /* negative error code.                                              */
   /* * NOTE * Only 1 CTS Server may be open at a time, per Bluetooth   */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
   /* * NOTE * This function supports CTS 1.1 and will allow the        */
   /*          optional writing of the Current Time and Local Time      */
   /*          Information Characteristics if the                       */
   /*          CTS_FLAGS_SUPPORT_CURRENT_TIME_GATT_WRITE or             */
   /*          CTS_FLAGS_SUPPORT_LOCAL_TIME_GATT_WRITE are set.         */
   /* * NOTE * If the last parameter is excluded it MUST be set to NULL.*/
int BTPSAPI CTS_Initialize_Service_Flags(unsigned int BluetoothStackID, unsigned int Flags, CTS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange)
{
   GATT_Attribute_Handle_Group_t LocalServiceHandleRange;

   /* Initialize the service handle range to either the user specified  */
   /* parameters or the default.                                        */
   if(!ServiceHandleRange)
   {
      /* Initialize the default handle range.                           */
      LocalServiceHandleRange.Starting_Handle = 0;
      LocalServiceHandleRange.Ending_Handle   = 0;

      /* Set the pointer to the default handle range.                   */
      ServiceHandleRange                      = &LocalServiceHandleRange;
   }

   /* Simply wrap the internal utility function to register the service */
   /* with the GATT layer.                                              */
   return(CTSRegisterService(BluetoothStackID, Flags, EventCallback, CallbackParameter, ServiceID, ServiceHandleRange, NULL));
}

   /* The following function is responsible for closing a previously    */
   /* opened CTS Server.  The first parameter is the Bluetooth Stack ID */
   /* on which to close the server.  The second parameter is the        */
   /* InstanceID that was returned from a successful call to            */
   /* CTS_Initialize_Service().  This function returns a zero if        */
   /* successful or a negative return error code if an error occurs.    */
int BTPSAPI CTS_Cleanup_Service(unsigned int BluetoothStackID, unsigned int InstanceID)
{
   int                  ret_val;
   CTSServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID))
   {
      /* Acquire the specified CTS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Verify that the service is actually registered.             */
         if(ServiceInstance->ServiceID)
         {
            /* Call GATT to un-register the service.                    */
            GATT_Un_Register_Service(BluetoothStackID, ServiceInstance->ServiceID);

#if BTPS_CONFIGURATION_GATT_SUPPORT_BR_EDR

            /* Call GATT to un-register the SDP Record.                 */
            GATT_Un_Register_SDP_Record(BluetoothStackID, ServiceInstance->SDPServiceRecordHandle);

#endif

            /* mark the instance entry as being free.                   */
            BTPS_MemInitialize(ServiceInstance, 0, CTS_SERVER_INSTANCE_DATA_SIZE);

            /* return success to the caller.                            */
            ret_val = 0;
         }
         else
            ret_val = CTS_ERROR_INVALID_PARAMETER;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(BluetoothStackID);
      }
      else
         ret_val = CTS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

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
unsigned long BTPSAPI CTS_Suspend(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   Byte_t           *TempPtr         = NULL;
   Boolean_t         PerformSuspend;
   unsigned int      Index;
   unsigned long     MemoryUsed;
   unsigned long     TotalMemoryUsed = 0;
   CTSSuspendInfo_t *SuspendInfo     = NULL;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize == 0) || ((Buffer) && (BufferSize >= (CTS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_SIZE))))
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
            SuspendInfo                              = (CTSSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, CTSSuspendInfo_t));

            /* Calculated the memory used for the header.               */
            MemoryUsed                               = (CTS_SUSPEND_INFO_SIZE(0) + (((Byte_t *)SuspendInfo) - ((Byte_t *)Buffer)));

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
            TotalMemoryUsed += (CTS_SUSPEND_INFO_SIZE(0));

            /* Flag that we are not performing the suspend, but instead */
            /* calculating the memory required for a suspend.           */
            PerformSuspend   = FALSE;
         }

         /* Loop and suspend all active services.                       */
         for(Index = 0; Index < (unsigned int)CTS_MAXIMUM_SUPPORTED_INSTANCES; Index++)
         {
            /* Check to see if this instance is currently registered.   */
            if((InstanceList[Index].BluetoothStackID == BluetoothStackID) && (InstanceList[Index].ServiceID))
            {
               /* Check to see if we are performing a suspend.          */
               if(PerformSuspend)
               {
                  /* Verify that we have memory left to perform the     */
                  /* suspend.                                           */
                  if(BufferSize >= CTS_SUSPEND_INSTANCE_INFO_SIZE(0))
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
   /* CTS_Suspend()).  This function accepts as input the Bluetooth     */
   /* Stack ID of the Bluetooth Stack that the Device is associated     */
   /* with.  The final two parameters are the buffer size and buffer    */
   /* that contains the memory that was used to collapse Bluetopia      */
   /* context into with a successfully call to CTS_Suspend().  This     */
   /* function returns ZERO on success or a negative error code.        */
int BTPSAPI CTS_Resume(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer)
{
#if BTPS_CONFIGURATION_GATT_SUPPORT_SUSPEND_RESUME

   int               ret_val;
   Byte_t           *TempPtr;
   unsigned int      Index;
   CTSSuspendInfo_t *SuspendInfo;

   /* Verify that the input parameters are semi-valid.                  */
   if((BufferSize) && ((TempPtr = (Byte_t *)Buffer) != NULL))
   {
      /* Verify that the buffer size is large enough for CTS.           */
      if(BufferSize >= (CTS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, CTSSuspendInfo_t)))
      {
         /* Subtract the header and alignment offset from the buffer    */
         /* size.                                                       */
         BufferSize  -= (CTS_SUSPEND_INFO_SIZE(0) + BTPS_ALIGNMENT_OFFSET(TempPtr, CTSSuspendInfo_t));

         /* Align the buffer for the CTSSuspendInfo_t structrue.        */
         SuspendInfo  = (CTSSuspendInfo_t *)(TempPtr + BTPS_ALIGNMENT_OFFSET(TempPtr, CTSSuspendInfo_t));

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
         ret_val = CTS_ERROR_INVALID_PARAMETER;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   /* Feature not supported so just return 0.                           */
   return(0);

#endif
}

   /* The following function is responsible for querying the number of  */
   /* attributes that are contained in the CTS Service that is          */
   /* registered with a call to CTS_Initialize_Service().  This function*/
   /* returns the non-zero number of attributes that are contained in a */
   /* CTS Server or zero on failure.                                    */
unsigned int BTPSAPI CTS_Query_Number_Attributes(void)
{
   /* Simply return the number of attributes that are contained in a CTS*/
   /* service.                                                          */
   return(CURRENT_TIME_SERVICE_ATTRIBUTE_COUNT);
}

   /* The following function is responsible for responding to a CTS Read*/
   /* Current Time Request.The first parameter is the Bluetooth Stack ID*/
   /* of the Bluetooth Device.The second parameter is the Transaction ID*/
   /* of the request. The final parameter contains the Current Time that*/
   /* send to the remote device.This function returns a zero if         */
   /* successful or a negative return error code if an error occurs.    */
int BTPSAPI CTS_Current_Time_Read_Request_Response(unsigned int BluetoothStackID, unsigned int TransactionID, CTS_Current_Time_Data_t *Current_Time)
{
   int    ret_val;
   Byte_t Value[CTS_CURRENT_TIME_SIZE];

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID) && (Current_Time))
   {
      /* Format the received data.                                      */
      if((ret_val = FormatCurrentTime(Current_Time, CTS_CURRENT_TIME_SIZE, Value)) == 0)
      {
         /* Send the current time read response.                        */
         ret_val = GATT_Read_Response(BluetoothStackID, TransactionID, (unsigned int)CTS_CURRENT_TIME_SIZE, Value);
      }
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for responding to a CTS Read*/
   /* Current Time Request when an error occured. The first parameter is*/
   /* the Bluetooth Stack ID of the Bluetooth Device.The second         */
   /* parameter is the Transaction ID of the request.The final parameter*/
   /* contains the Error which occured during the Read operation. This  */
   /* function returns a zero if successful or a negative return error  */
   /* code if an error occurs.                                          */
int BTPSAPI CTS_Current_Time_Read_Request_Error_Response(unsigned int BluetoothStackID, unsigned int TransactionID, Byte_t ErrorCode)
{
  int ret_val;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID))
      ret_val = GATT_Error_Response(BluetoothStackID, TransactionID, CTS_CURRENT_TIME_ATTRIBUTE_OFFSET, ErrorCode);
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

  return(ret_val);
}

   /* The following function is responsible for responding to a CTS     */
   /* Write Current Time Request.  The first parameter is the Bluetooth */
   /* Stack ID of the Bluetooth Device.  The second parameter is the    */
   /* Transaction ID of the request.  The final parameter contains the  */
   /* Error which occured during the Write operation.  If this parameter*/
   /* is non-zero then then error response will be sent.  This function */
   /* returns a zero if successful or a negative return error code if an*/
   /* error occurs.                                                     */
int BTPSAPI CTS_Current_Time_Write_Request_Response(unsigned int BluetoothStackID, unsigned int TransactionID, Byte_t ErrorCode)
{
   int ret_val;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID))
   {
      /* Send the correct response.                                     */
      if(!ErrorCode)
         ret_val = GATT_Write_Response(BluetoothStackID, TransactionID);
      else
         ret_val = GATT_Error_Response(BluetoothStackID, TransactionID, CTS_CURRENT_TIME_ATTRIBUTE_OFFSET, ErrorCode);
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for setting the Local Time  */
   /* Information on the specified CTS Instance.  The first parameter is*/
   /* the Bluetooth Stack ID of the Bluetooth Device.  The second       */
   /* parameter is the InstanceID returned from a successful call to    */
   /* CTS_Initialize_Server().  The final parameter is the Local Time to*/
   /* set for the specified CTS Instance.  This function returns a zero */
   /* a zero if successful or a negative return error code if an error  */
   /* occurs.                                                           */
int BTPSAPI CTS_Set_Local_Time_Information(unsigned int BluetoothStackID, unsigned int InstanceID, CTS_Local_Time_Information_Data_t *Local_Time)
{
#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

   int                  ret_val;
   CTSServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID) && (Local_Time) && (CTS_LOCAL_TIME_INFORMATION_VALID(*Local_Time)))
   {
      /* Acquire the specified CTS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Assign the Local Time Zone Information for the specified    */
         /* instance.                                                   */
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(InstanceData[InstanceID-1].Local_Time_Information.Time_Zone), Local_Time->Time_Zone);

         /* Assign the Local Time DST Information for the specified     */
         /* instance.                                                   */
         ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(InstanceData[InstanceID-1].Local_Time_Information.Daylight_Saving_Time), Local_Time->Daylight_Saving_Time);

         /* Return success to the caller.                               */
         ret_val = 0;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
      else
         ret_val = CTS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}

   /* The following function is responsible for querying the Local time */
   /* on the specified CTS Instance.The first parameter is the Bluetooth*/
   /* Stack ID of the Bluetooth Device.  The second parameter is the    */
   /* InstanceID returned from a successful call to                     */
   /* CTS_Initialize_Server() The final parameter is a pointer to return*/
   /* the Local time for the specified CTS Instance.  This function     */
   /* returns a zero if successful or a negative return error code if an*/
   /* error occurs.                                                     */
int BTPSAPI CTS_Query_Local_Time_Information(unsigned int BluetoothStackID, unsigned int InstanceID, CTS_Local_Time_Information_Data_t *Local_Time)
{
#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

   int                  ret_val;
   CTSServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID) && (Local_Time))
   {
      /* Acquire the specified CTS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Retrieve the Local Time Information from the specified      */
         /* instance.                                                   */
         Local_Time->Time_Zone            = (CTS_Time_Zone_Type_t)((SByte_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(InstanceData[InstanceID-1].Local_Time_Information.Time_Zone)));
         Local_Time->Daylight_Saving_Time = (CTS_DST_Offset_Type_t)((SByte_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(InstanceData[InstanceID-1].Local_Time_Information.Daylight_Saving_Time)));

         /* Return success to the caller.                               */
         ret_val                          = 0;

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
      else
         ret_val = CTS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}

   /* The following function is responsible for responding to a CTS     */
   /* Write Current Time Request.  The first parameter is the Bluetooth */
   /* Stack ID of the Bluetooth Device.  The second parameter is the    */
   /* Transaction ID of the request.  The final parameter contains the  */
   /* Error which occured during the Write operation.  If this parameter*/
   /* is non-zero then then error response will be sent.  This function */
   /* returns a zero if successful or a negative return error code if an*/
   /* error occurs.                                                     */
int BTPSAPI CTS_Local_Time_Information_Write_Request_Response(unsigned int BluetoothStackID, unsigned int TransactionID, Byte_t ErrorCode)
{
   int ret_val;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID))
   {
      /* Send the correct response.                                     */
      if(!ErrorCode)
         ret_val = GATT_Write_Response(BluetoothStackID, TransactionID);
      else
         ret_val = GATT_Error_Response(BluetoothStackID, TransactionID, CTS_LOCAL_TIME_INFORMATION_ATTRIBUTE_OFFSET, ErrorCode);
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for responding to a CTS Read*/
   /* Reference Time Information Request.  The first parameter is the   */
   /* Bluetooth Stack ID of the Bluetooth Device.  The second parameter */
   /* is the Transaction ID of the request.  The final parameter        */
   /* contains the Reference Time Information to send to the remote     */
   /* device.  This function returns a zero if successful or a negative */
   /* return error code if an error occurs.                             */
int BTPSAPI CTS_Reference_Time_Information_Read_Request_Response(unsigned int BluetoothStackID, unsigned int TransactionID, CTS_Reference_Time_Information_Data_t *Reference_Time)
{
#if BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION

   int    ret_val;
   Byte_t Value[CTS_REFERENCE_TIME_INFORMATION_SIZE];

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID) && (Reference_Time) && (CTS_REFERENCE_TIME_INFORMATION_VALID(*Reference_Time)))
   {
      /* Format the received data.                                      */
      if((ret_val = FormatReferenceTimeInformation(Reference_Time, CTS_REFERENCE_TIME_INFORMATION_SIZE, Value)) == 0)
      {
         /* Send the current time read response.                        */
         ret_val = GATT_Read_Response(BluetoothStackID, TransactionID, (unsigned int)CTS_REFERENCE_TIME_INFORMATION_SIZE, Value);
      }
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}

   /* The following function is responsible for responding to a CTS Read*/
   /* Reference Time Information Request when an error occured.  The    */
   /* first parameter is the Bluetooth Stack ID of the Bluetooth Device.*/
   /* The second parameter is the Transaction ID of the request.        */
   /* The final parameter contains the Error which occured during the   */
   /* Read operation.  This function returns a zero if successful or a  */
   /* negative return error code if an error occurs.                    */
int BTPSAPI CTS_Reference_Time_Information_Read_Request_Error_Response(unsigned int BluetoothStackID, unsigned int TransactionID, Byte_t ErrorCode)
{
#if BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION

   int ret_val;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (TransactionID))
      ret_val = GATT_Error_Response(BluetoothStackID, TransactionID, CTS_REFERENCE_TIME_INFORMATION_ATTRIBUTE_OFFSET, ErrorCode);
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}

   /* The following function is responsible for responding to a CTS Read*/
   /* Client Configuration Request.  The first parameter is the         */
   /* Bluetooth Stack ID of the Bluetooth Device.  The second parameter */
   /* is the InstanceID returned from a successful call to              */
   /* CTS_Initialize_Server().  The third is the Transaction ID of the  */
   /* request.  The final parameter contains the Client Configuration to*/
   /* send to the remote device.  This function returns a zero if       */
   /* successful or a negative return error code if an error occurs.    */
int BTPSAPI CTS_Read_Client_Configuration_Response(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int TransactionID, Word_t Client_Configuration)
{
   int              ret_val;
   Word_t           ValueLength;
   NonAlignedWord_t ClientConfiguration;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID) && (TransactionID))
   {
      /* Format the Read Response.                                      */
      ValueLength = GATT_CLIENT_CHARACTERISTIC_CONFIGURATION_LENGTH;
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&ClientConfiguration, Client_Configuration);

      /* Send the response.                                             */
      ret_val = GATT_Read_Response(BluetoothStackID, TransactionID, (unsigned int)ValueLength, (Byte_t *)&ClientConfiguration);
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for sending an Current      */
   /* Time notification to a specified remote device.  The first        */
   /* parameter is the Bluetooth Stack ID of the Bluetooth Device.  The */
   /* second parameter is the InstanceID returned from a successful call*/
   /* to CTS_Initialize_Server().  The third parameter is the           */
   /* ConnectionID of the remote device to send the notification to.    */
   /* The final parameter is the Current Time  data to notify.          */
   /* This function returns a zero if successful or a negative          */
   /* return error code if an error occurs.                             */
int BTPSAPI CTS_Notify_Current_Time(unsigned int BluetoothStackID, unsigned int InstanceID, unsigned int ConnectionID, CTS_Current_Time_Data_t *Current_Time)
{
   int                  ret_val;
   Byte_t               NotificationData[CTS_CURRENT_TIME_SIZE];
   CTSServerInstance_t *ServiceInstance;

   /* Make sure the parameters passed to us are semi-valid.             */
   if((BluetoothStackID) && (InstanceID) && (ConnectionID) && (Current_Time))
   {
      /* Acquire the specified CTS Instance.                            */
      if((ServiceInstance = AcquireServiceInstance(BluetoothStackID, &InstanceID)) != NULL)
      {
         /* Format the notification.                                    */
         if((ret_val = FormatCurrentTime(Current_Time, CTS_CURRENT_TIME_SIZE, NotificationData)) == 0)
         {
            /* Attempt to send the notification.                        */
            ret_val = GATT_Handle_Value_Notification(ServiceInstance->BluetoothStackID, ServiceInstance->ServiceID, ConnectionID, CTS_CURRENT_TIME_ATTRIBUTE_OFFSET, CTS_CURRENT_TIME_SIZE, (Byte_t *)NotificationData);
            if(ret_val > 0)
               ret_val = 0;
         }

         /* UnLock the previously locked Bluetooth Stack.               */
         BSC_UnLockBluetoothStack(ServiceInstance->BluetoothStackID);
      }
      else
         ret_val = CTS_ERROR_INVALID_INSTANCE_ID;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* CTS Client API.                                                   */

   /* The following function is responsible for formatting the Current  */
   /* Time into a buffer.  The first parameter is a pointer that        */
   /* contains the Current Time that will be formatted.  The second     */
   /* parameter is the length of the buffer that will hold the formatted*/
   /* data.  The final parameter is a pointer to the buffer that will   */
   /* contain the formatted data.  This function returns a zero if      */
   /* successful or a negative return error code if an error occurs.    */
int BTPSAPI CTS_Format_Current_Time(CTS_Current_Time_Data_t *Current_Time, unsigned int BufferLength, Byte_t *Buffer)
{
   return(FormatCurrentTime(Current_Time, BufferLength, Buffer));
}

   /* The following function is responsible for parsing a value received*/
   /* from a remote CTS Server interpreting it as a Current Time        */
   /* characteristic.  The first parameter is the length of             */
   /* the value returned by the remote CTS Server.  The second parameter*/
   /* is a pointer to the data returned by the remote CTS Server.  The  */
   /* final parameter is a pointer to store the parsed Current Time     */
   /* Measurement value.  This function returns a zero if successful or */
   /* a negative return error code if an error occurs.                  */
int BTPSAPI CTS_Decode_Current_Time(unsigned int ValueLength, Byte_t *Value, CTS_Current_Time_Data_t *Current_Time)
{
   int ret_val = CTS_ERROR_MALFORMATTED_DATA;

   /* Verify that the input parameters are valid.                       */
   if((ValueLength == CTS_CURRENT_TIME_SIZE) && (Value) && (Current_Time))
   {
      /* Read the packed Current Time Data.                             */
      Current_Time->Exact_Time.Day_Date_Time.Date_Time.Year    = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Day_Date_Time.Date_Time.Year));
      Current_Time->Exact_Time.Day_Date_Time.Date_Time.Month   = (CTS_Month_Of_Year_Type_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Day_Date_Time.Date_Time.Month));
      Current_Time->Exact_Time.Day_Date_Time.Date_Time.Day     = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Day_Date_Time.Date_Time.Day));
      Current_Time->Exact_Time.Day_Date_Time.Date_Time.Hours   = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Day_Date_Time.Date_Time.Hours));
      Current_Time->Exact_Time.Day_Date_Time.Date_Time.Minutes = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Day_Date_Time.Date_Time.Minutes));
      Current_Time->Exact_Time.Day_Date_Time.Date_Time.Seconds = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Day_Date_Time.Date_Time.Seconds));
      Current_Time->Exact_Time.Day_Date_Time.Day_Of_Week       = (CTS_Week_Day_Type_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Day_Date_Time.Day_Of_Week));
      Current_Time->Exact_Time.Fractions256                    = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Exact_Time.Fractions256));
      Current_Time->Adjust_Reason_Mask                         = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Current_Time_t *)Value)->Adjust_Reason_Mask));

      /* Verify that the Current Time Information is valid.             */
      if(CTS_CURRENT_TIME_VALID(*Current_Time))
      {
         /* Return success to the caller.                               */
         ret_val       = 0;
      }
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for formatting the Local    */
   /* Time Information into a buffer.  The first parameter is a pointer */
   /* to the Local Time Information that will be formatted.  The second */
   /* parameter is the length of the buffer that will hold the Local    */
   /* Time Information.  The final parameter is a pointer to the buffer */
   /* that will contain the formatted data.  This function returns a    */
   /* zero if successful or a negative return error code if an error    */
   /* occurs.                                                           */
int BTPSAPI CTS_Format_Local_Time_Information(CTS_Local_Time_Information_Data_t *Local_Time, unsigned int BufferLength, Byte_t *Buffer)
{
   int ret_val;

   /* Verify that the input parameters are valid.                       */
   if((Local_Time) && (CTS_LOCAL_TIME_INFORMATION_VALID(*Local_Time)) && (BufferLength >= CTS_LOCAL_TIME_INFORMATION_SIZE) && (Buffer))
   {
      /* Format the Time Zone.                                          */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Local_Time_Information_t*)Buffer)->Time_Zone), Local_Time->Time_Zone);

      /* Format the DST.                                                */
      ASSIGN_HOST_BYTE_TO_LITTLE_ENDIAN_UNALIGNED_BYTE(&(((CTS_Local_Time_Information_t*)Buffer)->Daylight_Saving_Time), Local_Time->Daylight_Saving_Time);

      /* Return success to the caller.                                  */
      ret_val = 0;
   }
   else
      ret_val = CTS_ERROR_INVALID_PARAMETER;

   /* Finally return the result to the caller.                          */
   return(ret_val);
}

   /* The following function is responsible for parsing a value received*/
   /* from a remote CTS Server interpreting it as a Local Time          */
   /* information characteristic.  The first parameter is the length of */
   /* the value returned by the remote CTS Server.  The second parameter*/
   /* is a pointer to the data returned by the remote CTS Server.The    */
   /* final parameter is a pointer to store the parsed Local Time       */
   /* information value.This function returns a zero if successful or a */
   /* negative return error code if an error occurs.                    */
int BTPSAPI CTS_Decode_Local_Time_Information(unsigned int ValueLength, Byte_t *Value, CTS_Local_Time_Information_Data_t *Local_Time)
{
#if BTPS_CONFIGURATION_CTS_SUPPORT_LOCAL_TIME_INFORMATION

   int ret_val = CTS_ERROR_MALFORMATTED_DATA;

   /* Verify that the input parameters are valid.                       */
   if((ValueLength == CTS_LOCAL_TIME_INFORMATION_SIZE) && (Value) && (Local_Time))
   {
      /* Read the packed Local Time Information Data.                   */
      Local_Time->Time_Zone            = (CTS_Time_Zone_Type_t)((SByte_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Local_Time_Information_t *)Value)->Time_Zone)));
      Local_Time->Daylight_Saving_Time = (CTS_DST_Offset_Type_t)((SByte_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Local_Time_Information_t *)Value)->Daylight_Saving_Time)));

      /* Verify that the decoded Local Time Information was valid.      */
      if(CTS_LOCAL_TIME_INFORMATION_VALID(*Local_Time))
      {
         /* Return success to the caller.                               */
         ret_val     = 0;
      }
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}

   /* The following function is responsible for parsing a value received*/
   /* from a remote CTS Server interpreting it as a Reference Time      */
   /* Information characteristic.  The first parameter is the length of */
   /* the value returned by the remote CTS Server.  The second parameter*/
   /* is a pointer to the data returned by the remote CTS Server.The    */
   /* final parameter is a pointer to store the parsed Reference Time   */
   /* Information value.  This function returns a zero if successful or */
   /* a negative return error code if an error occurs.                  */
int BTPSAPI CTS_Decode_Reference_Time_Information(unsigned int ValueLength, Byte_t *Value, CTS_Reference_Time_Information_Data_t *Reference_Time)
{
#if BTPS_CONFIGURATION_CTS_SUPPORT_REFERENCE_TIME_INFORMATION

   int                                   ret_val = CTS_ERROR_MALFORMATTED_DATA;
   CTS_Reference_Time_Information_Data_t ReferenceTimeInfo;

   /* Verify that the input parameters are valid.                       */
   if((ValueLength == CTS_REFERENCE_TIME_INFORMATION_SIZE) && (Value) && (Reference_Time))
   {
      /* Read the packed Local Time Information Data.                   */
      ReferenceTimeInfo.Source             = (CTS_Time_Source_Type_t)READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Reference_Time_Information_t *)Value)->Source));
      ReferenceTimeInfo.Accuracy           = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Reference_Time_Information_t *)Value)->Accuracy));
      ReferenceTimeInfo.Days_Since_Update  = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Reference_Time_Information_t *)Value)->Days_Since_Update));
      ReferenceTimeInfo.Hours_Since_Update = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(&(((CTS_Reference_Time_Information_t *)Value)->Hours_Since_Update));

      /* Verify that the decoded data is correctly formatted.           */
      if(CTS_REFERENCE_TIME_INFORMATION_VALID(ReferenceTimeInfo))
      {
         /* Return the successfully decoded Reference Time Information  */
         /* to the caller.                                              */
         *Reference_Time = ReferenceTimeInfo;

         /* Return success to the caller.                               */
         ret_val         = 0;
      }
   }

   /* Finally return the result to the caller.                          */
   return(ret_val);

#else

   return(BTPS_ERROR_FEATURE_NOT_AVAILABLE);

#endif
}
