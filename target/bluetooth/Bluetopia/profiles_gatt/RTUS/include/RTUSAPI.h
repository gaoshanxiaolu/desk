/*****< rtusapi.h >************************************************************/
/*      Copyright 2012 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  RTUSAPI - Stonestreet One Bluetooth Reference Time Update Service (GATT   */
/*           based) API Type Definitions, Constants, and Prototypes.          */
/*                                                                            */
/*  Author:  Zahid Khan                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   07/11/12  Z. Khan        Initial creation.                               */
/******************************************************************************/
#ifndef __RTUSAPIH__
#define __RTUSAPIH__

#include "SS1BTPS.h"       /* Bluetooth Stack API Prototypes/Constants.       */
#include "SS1BTGAT.h"      /* Bluetooth Stack GATT API Prototypes/Constants.  */
#include "RTUSType.h"      /* Reference Time Update Service Types/Constants.  */

   /* Error Return Codes.                                               */

   /* Error Codes that are smaller than these (less than -1000) are     */
   /* related to the Bluetooth Protocol Stack itself (see BTERRORS.H).  */
#define RTUS_ERROR_INVALID_PARAMETER                      (-1000)
#define RTUS_ERROR_INVALID_BLUETOOTH_STACK_ID             (-1001)
#define RTUS_ERROR_INSUFFICIENT_RESOURCES                 (-1002)
#define RTUS_ERROR_SERVICE_ALREADY_REGISTERED             (-1003)
#define RTUS_ERROR_INVALID_INSTANCE_ID                    (-1004)
#define RTUS_ERROR_MALFORMATTED_DATA                      (-1005)
#define RTUS_ERROR_UNKNOWN_ERROR                          (-1006)

   /* The following enumerated type represents all of the valid commands*/
   /* that may be received in an                                        */
   /* etRTUS_Server_Time_Update_Control_Point_Command Server event OR   */
   /* that may be written to a remote RTUS Server.                      */
typedef enum
{
   cpGet_Reference_Update    = RTUS_TIME_UPDATE_CONTROL_POINT_GET_REFERENCE_UPDATE,
   cpCancel_Reference_Update = RTUS_TIME_UPDATE_CONTROL_POINT_CANCEL_REFERENCE_UPDATE
} RTUS_Time_Update_Control_Command_t;

   /* The following enumeration covers all the events generated by the  */
   /* RTUS Service.  These are used to determine the type of each event */
   /* generated, and to ensure the proper union element is accessed for */
   /* the RTUS_Event_Data_t structure.                                  */
typedef enum
{
   etRTUS_Server_Time_Update_Control_Point_Command
} RTUS_Event_Type_t;

   /* The following RTUS Service Event is dispatched to a RTUS Server   */
   /* when a RTUS Client has sent a Time Update Control Point Command.  */
   /* The ConnectionID, ConnectionType, and RemoteDevice specifiy the   */
   /* Client that is making the update.  The final parameter specifies  */
   /* the Time Update Control Point command that the client sent.       */
typedef struct _tagRTUS_Time_Update_Control_Command_Data_t
{
   unsigned int                       InstanceID;
   unsigned int                       ConnectionID;
   GATT_Connection_Type_t             ConnectionType;
   BD_ADDR_t                          RemoteDevice;
   RTUS_Time_Update_Control_Command_t Command;
} RTUS_Time_Update_Control_Command_Data_t;

#define RTUS_TIME_UPDATE_CONTROL_COMMAND_DATA_SIZE         (sizeof(RTUS_Time_Update_Control_Command_Data_t))

   /* The following structure represents the container structure for    */
   /* holding all RTUS Service Event Data.  This structure is received  */
   /* for each event generated.  The Event_Data_Type member is used to  */
   /* determine the appropriate union member element to access the      */
   /* contained data.  The Event_Data_Size member contains the total    */
   /* size of the data contained in this event.                         */
typedef struct _tagRTUS_Event_Data_t
{
   RTUS_Event_Type_t Event_Data_Type;
   Word_t            Event_Data_Size;
   union
   {
      RTUS_Time_Update_Control_Command_Data_t *RTUS_Time_Update_Control_Command_Data;
   } Event_Data;
} RTUS_Event_Data_t;

#define RTUS_EVENT_DATA_SIZE                             (sizeof(RTUS_Event_Data_t))

   /* The following structure contains the Handles that will need to be */
   /* cached by a RTUS client in order to only do service discovery once*/
typedef struct _tagRTUS_Client_Information_t
{
   Word_t Control_Point;
   Word_t Time_Update_State;
} RTUS_Client_Information_t;

#define RTUS_CLIENT_INFORMATION_DATA_SIZE     (sizeof(RTUS_Client_Information_t))

   /* The following structure contains all of the per Client data that  */
   /* will need to be stored by a RTUS Server.                          */
typedef struct _tagRTUS_Server_Information_t
{
   Word_t Control_Point;
} RTUS_Server_Information_t;

#define RTUS_SERVER_INFORMATION_DATA_SIZE     (sizeof(RTUS_Server_Information_t))

   /* The following enum defines the valid Current State of Time Update */
   /* State Characteristic Attribute                                    */
typedef enum
{
   csIdle          = RTUS_CURRENT_STATE_IDLE,
   csUpdatePending = RTUS_CURRENT_STATE_UPDATE_PENDING
} RTUS_Time_Update_State_Current_State_t;

   /* The following enum defines the valid Result of Time Update State  */
   /* Characteristic Attribute                                          */
typedef enum
{
   reSuccessful                   = RTUS_RESULT_SUCCESSFUL,
   reCanceled                     = RTUS_RESULT_CANCELED,
   reNoConnectionToReference      = RTUS_RESULT_NO_CONNECTION_TO_REFERENCE,
   reReferenceRespondedWithError  = RTUS_RESULT_REFERENCE_RESPONDED_WITH_AN_ERROR,
   reTimeout                      = RTUS_RESULT_TIMEOUT,
   reUpdateNotAttemptedAfterReset = RTUS_RESULT_UPDATE_NOT_ATTEMPTED_AFTER_RESET
} RTUS_Time_Update_State_Result_t;

   /* The following defines the format of a Time Update State data that */
   /* may be set to or queried from RTUS Server                         */
typedef struct _tagRTUS_Time_Update_State_Data_t
{
   RTUS_Time_Update_State_Current_State_t CurrentState;
   RTUS_Time_Update_State_Result_t        Result;
} RTUS_Time_Update_State_Data_t;

#define RTUS_TIME_UPDATE_STATE_DATA_SIZE                (sizeof(RTUS_Time_Update_State_Data_t))

   /* The following declared type represents the Prototype Function for */
   /* a RTUS Service Event Receive Data Callback.  This function will be*/
   /* called whenever an RTUS Service Event occurs that is associated   */
   /* with the specified Bluetooth Stack ID.  This function passes to   */
   /* the caller the Bluetooth Stack ID, the RTUS Event Data that       */
   /* occurred and the RTUS Service Event Callback Parameter that was   */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the RTUS Service Event Data ONLY in the       */
   /* context of this callback.  If the caller requires the Data for a  */
   /* longer period of time, then the callback function MUST copy the   */
   /* data into another Data Buffer This function is guaranteed NOT to  */
   /* be invoked more than once simultaneously for the specified        */
   /* installed callback (i.e.  this function DOES NOT have be          */
   /* re-entrant).  It needs to be noted however, that if the same      */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.Therefore, processing in this function should be*/
   /* as efficient as possible (this argument holds anyway because      */
   /* another RTUS Service Event will not be processed while this       */
   /* function call is outstanding).                                    */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving RTUS Service Event  */
   /*            Packets.  A Deadlock WILL occur because NO RTUS Event  */
   /*            Callbacks will be issued while this function is        */
   /*            currently outstanding.                                 */
typedef void (BTPSAPI *RTUS_Event_Callback_t)(unsigned int BluetoothStackID, RTUS_Event_Data_t *RTUS_Event_Data, unsigned long CallbackParameter);

   /* RTUS Server API.                                                  */

   /* The following function is responsible for opening a RTUS Server.  */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The final parameter is a      */
   /* pointer to store the GATT Service ID of the registered RTUS       */
   /* service.  This can be used to include the service registered by   */
   /* this call.  This function returns the positive, non-zero, Instance*/
   /* ID or a negative error code.                                      */
   /* * NOTE * Only 1 RTUS Server may be open at a time, per Bluetooth  */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Initialize_Service(unsigned int BluetoothStackID, RTUS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Initialize_Service_t)(unsigned int BluetoothStackID, RTUS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID);
#endif

   /* The following function is responsible for opening a RTUS Server.  */
   /* The first parameter is the Bluetooth Stack ID on which to open the*/
   /* server.  The second parameter is the Callback function to call    */
   /* when an event occurs on this Server Port.  The third parameter is */
   /* a user-defined callback parameter that will be passed to the      */
   /* callback function with each event.  The fourth parameter is a     */
   /* pointer to store the GATT Service ID of the registered RTUS       */
   /* service.  This can be used to include the service registered by   */
   /* this call.  The final parameter is a pointer, that on input can be*/
   /* used to control the location of the service in the GATT database, */
   /* and on ouput to store the service handle range.  This function    */
   /* returns the positive, non-zero, Instance ID or a negative error   */
   /* code.                                                             */
   /* * NOTE * Only 1 RTUS Server may be open at a time, per Bluetooth  */
   /*          Stack ID.                                                */
   /* * NOTE * All Client Requests will be dispatch to the EventCallback*/
   /*          function that is specified by the second parameter to    */
   /*          this function.                                           */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Initialize_Service_Handle_Range(unsigned int BluetoothStackID, RTUS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Initialize_Service_Handle_Range_t)(unsigned int BluetoothStackID, RTUS_Event_Callback_t EventCallback, unsigned long CallbackParameter, unsigned int *ServiceID, GATT_Attribute_Handle_Group_t *ServiceHandleRange);
#endif

   /* The following function is responsible for closing a previously    */
   /* opened RTUS Server.  The first parameter is the Bluetooth Stack ID*/
   /* on which to close the server.  The second parameter is the        */
   /* InstanceID that was returned from a successful call to            */
   /* RTUS_Initialize_Service().  This function returns a zero if       */
   /* successful or a negative return error code if an error occurs.    */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Cleanup_Service(unsigned int BluetoothStackID, unsigned int InstanceID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Cleanup_Service_t)(unsigned int BluetoothStackID, unsigned int InstanceID);
#endif

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
BTPSAPI_DECLARATION unsigned long BTPSAPI RTUS_Suspend(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef unsigned long (BTPSAPI *PFN_RTUS_Suspend_t)(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer);
#endif

   /* The following function is used to perform a resume of the         */
   /* Bluetooth stack after a successful suspend has been performed (see*/
   /* RTUS_Suspend()).  This function accepts as input the Bluetooth    */
   /* Stack ID of the Bluetooth Stack that the Device is associated     */
   /* with.  The final two parameters are the buffer size and buffer    */
   /* that contains the memory that was used to collapse Bluetopia      */
   /* context into with a successfull call to RTUS_Suspend().  This     */
   /* function returns ZERO on success or a negative error code.        */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Resume(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Resume_t)(unsigned int BluetoothStackID, unsigned long BufferSize, void *Buffer);
#endif

   /* The following function is responsible for querying the number of  */
   /* attributes that are contained in the RTUS Service that is         */
   /* registered with a call to RTUS_Initialize_Service().  This        */
   /* function returns the non-zero number of attributes that are       */
   /* contained in a RTUS Server or zero on failure.                    */
BTPSAPI_DECLARATION unsigned int BTPSAPI RTUS_Query_Number_Attributes(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef unsigned int (BTPSAPI *PFN_RTUS_Query_Number_Attributes_t)(void);
#endif

   /* The following function is responsible for setting the Time Update */
   /* State on the specified RTUS Instance.  The first parameter        */
   /* is the Bluetooth Stack ID of the Bluetooth Device.  The second    */
   /* parameter is the InstanceID returned from a successful call to    */
   /* RTUS_Initialize_Server().  The final parameter is the Time Update */
   /* State to set for the specified RTUS Instance.  This function      */
   /* returns a zero if successful or a negative return error code if   */
   /* an error occurs.                                                  */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Set_Time_Update_State(unsigned int BluetoothStackID, unsigned int InstanceID, RTUS_Time_Update_State_Data_t *TimeUpdateState);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Set_Time_Update_State_t)(unsigned int BluetoothStackID, unsigned int InstanceID, RTUS_Time_Update_State_Data_t *TimeUpdateState);
#endif

   /* The following function is responsible for querying the Time Update*/
   /* State on the specified RTUS Instance.The first parameter is       */
   /* the Bluetooth Stack ID of the Bluetooth Device.  The second       */
   /* parameter is the InstanceID returned from a successful call to    */
   /* RTUS_Initialize_Server().The final parameter is a pointer to      */
   /* return the Time Update State for the specified RTUS Instance.     */
   /* This function returns a zero if successful or a negative return   */
   /* error code if an error occurs.                                    */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Query_Time_Update_State(unsigned int BluetoothStackID, unsigned int InstanceID, RTUS_Time_Update_State_Data_t *TimeUpdateState);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Query_Time_Update_State_t)(unsigned int BluetoothStackID, unsigned int InstanceID, RTUS_Time_Update_State_Data_t *TimeUpdateState);
#endif

   /* RTUS Client API.                                                  */

   /* The following function is responsible for parsing a value received*/
   /* from a remote RTUS Server interpreting it as a Time Update State  */
   /* characteristic.  The first parameter is the length of the value   */
   /* returned by the remote RTUS Server.  The second parameter is a    */
   /* pointer to the data returned by the remote RTUS Server.The final  */
   /* parameter is a pointer to store the parsed Time Update State      */
   /* value. This function returns a zero if successful or a negative   */
   /* return error code if an error occurs.                             */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Decode_Time_Update_State(unsigned int ValueLength, Byte_t *Value, RTUS_Time_Update_State_Data_t *TimeUpdateState);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Decode_Time_Update_State_t)(unsigned int ValueLength, Byte_t *Value, RTUS_Time_Update_State_Data_t *TimeUpdateState);
#endif

   /* The following function is responsible for formatting a Reference  */
   /* Time Update Control Command into a user specified buffer.  The    */
   /* first parameter is the command to format.  The final two          */
   /* parameters contain the length of the buffer, and the buffer, to   */
   /* format the command into.  This function returns a zero if         */
   /* successful or a negative return error code if an error occurs.    */
   /* * NOTE * The BufferLength and Buffer parameter must point to a    */
   /*          buffer of at least                                       */
   /*          RTUS_TIME_UPDATE_CONTROL_POINT_VALUE_LENGTH in size.     */
BTPSAPI_DECLARATION int BTPSAPI RTUS_Format_Control_Point_Command(RTUS_Time_Update_Control_Command_t Command, unsigned int BufferLength, Byte_t *Buffer);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_RTUS_Format_Control_Point_Command_t)(RTUS_Time_Update_Control_Command_t Command, unsigned int BufferLength, Byte_t *Buffer);
#endif

#endif
