/*****< rubytip.c >************************************************************/
/*      Copyright 2014 Stonestreet One.                                       */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  RUBYTIP - Ruby Bluetooth Time Profile using GATT (LE) application.        */
/*                                                                            */
/*  Author:  Cliff Xu                                                         */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   05/14/14  C. Xu          Initial creation.                               */
/******************************************************************************/

#include "RubyTIP.h"             /* Application Header.                       */

#include "SS1BTPS.h"             /* Main SS1 BT Stack Header.                 */
#include "SS1BTGAT.h"            /* Main SS1 GATT Header.                     */
#include "SS1BTGAP.h"            /* Main SS1 GAP Service Header.              */

#include "SS1BTCTS.h"            /* Main SS1 CTS Service Header.              */
#include "SS1BTNDC.h"            /* Main SS1 NDCS Service Header.             */
#include "SS1BTRTU.h"            /* Main SS1 RTUS Service Header.             */
#include "SS1BTDIS.h"            /* Main SS1 DIS Service Header.              */

#include "HAL.h"
#include "BTPSVEND.h"
#include "qcom_uart.h"
#include "threadxdmn_api.h"

#define PRINTF                                qcom_printf

#define CONSOLE                               "UART0"

#define BYTE_POOL_SIZE                      (5*1024)
#define PSEUDO_HOST_STACK_SIZE            (4 * 1024)
#define MAX_SUPPORTED_COMMANDS                      (64) /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_COMMAND_LENGTH                         (64)  /* Denotes the max   */
                                                         /* buffer size used  */
                                                         /* for user commands */
                                                         /* input via the     */
                                                         /* User Interface.   */

#define MAX_NUM_OF_PARAMETERS                      (16)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define DEFAULT_IO_CAPABILITY       (licNoInputNoOutput) /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Pairing.*/

#define DEFAULT_MITM_PROTECTION                 (TRUE)   /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

#define NO_COMMAND_ERROR                           (-1)  /* Denotes that no   */
                                                         /* command was       */
                                                         /* specified to the  */
                                                         /* parser.           */

#define INVALID_COMMAND_ERROR                      (-2)  /* Denotes that the  */
                                                         /* Command does not  */
                                                         /* exist for         */
                                                         /* processing.       */

#define EXIT_CODE                                  (-3)  /* Denotes that the  */
                                                         /* Command specified */
                                                         /* was the Exit      */
                                                         /* Command.          */

#define FUNCTION_ERROR                             (-4)  /* Denotes that an   */
                                                         /* error occurred in */
                                                         /* execution of the  */
                                                         /* Command Function. */

#define TO_MANY_PARAMS                             (-5)  /* Denotes that there*/
                                                         /* are more          */
                                                         /* parameters then   */
                                                         /* will fit in the   */
                                                         /* UserCommand.      */

#define INVALID_PARAMETERS_ERROR                   (-6)  /* Denotes that an   */
                                                         /* error occurred due*/
                                                         /* to the fact that  */
                                                         /* one or more of the*/
                                                         /* required          */
                                                         /* parameters were   */
                                                         /* invalid.          */

#define UNABLE_TO_INITIALIZE_STACK                 (-7)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* while Initializing*/
                                                         /* the Bluetooth     */
                                                         /* Protocol Stack.   */

#define INVALID_STACK_ID_ERROR                     (-8)  /* Denotes that an   */
                                                         /* occurred due to   */
                                                         /* attempted         */
                                                         /* execution of a    */
                                                         /* Command when a    */
                                                         /* Bluetooth Protocol*/
                                                         /* Stack has not been*/
                                                         /* opened.           */

#define UNABLE_TO_REGISTER_SERVER                  (-9)  /* Denotes that an   */
                                                         /* error occurred    */
                                                         /* when trying to    */
                                                         /* create a Serial   */
                                                         /* Port Server.      */

#define EXIT_MODE                                  (-10) /* Flags exit from   */
                                                         /* any Mode.         */

#define DEFAULT_CURRENT_TIME           {{{{2014,myJuly,9,0,0,0},wdMonday},0},0x0}
                                                         /* Default           */
                                                         /* initialization    */
                                                         /* value for the     */
                                                         /* current time.     */

#define DEFAULT_LOCAL_TIME_INFO        {22,0}
                                                         /* Default           */
                                                         /* initialization    */
                                                         /* value for the     */
                                                         /* local time.       */

#define DEFAULT_REFERENCE_TIME_INFO    {0,0,0,0}
                                                         /* Default           */
                                                         /* initialization    */
                                                         /* value for the     */
                                                         /* reference time.   */

#define DEFAULT_TIME_WITH_DST           {{2012,myJuly,9,0,0,0},0x00}

#define DEFAULT_TIME_UPDATE_STATE_DATA  {RTUS_CURRENT_STATE_IDLE,RTUS_RESULT_SUCCESSFUL}

   /* Determine the Name we will use for this compilation.              */
#define LE_DEMO_DEVICE_NAME                        "RubyTIP"
   /* The following type definition represents the container type which */
   /* holds the mapping between Bluetooth devices (based on the BD_ADDR)*/
   /* and the Link Key (BD_ADDR <-> Link Key Mapping).                  */
typedef struct _tagLinkKeyInfo_t
{
   BD_ADDR_t  BD_ADDR;
   Link_Key_t LinkKey;
} LinkKeyInfo_t;

   /* The following type definition represents the structure which holds*/
   /* all information about the parameter, in particular the parameter  */
   /* as a string and the parameter as an unsigned int.                 */
typedef struct _tagParameter_t
{
   char *strParam;
   int   intParam;
} Parameter_t;

   /* The following type definition represents the structure which holds*/
   /* a list of parameters that are to be associated with a command The */
   /* NumberofParameters variable holds the value of the number of      */
   /* parameters in the list.                                           */
typedef struct _tagParameterList_t
{
   int         NumberofParameters;
   Parameter_t Params[MAX_NUM_OF_PARAMETERS];
} ParameterList_t;

   /* The following type definition represents the structure which holds*/
   /* the command and parameters to be executed.                        */
typedef struct _tagUserCommand_t
{
   char            *Command;
   ParameterList_t  Parameters;
} UserCommand_t;

   /* The following type definition represents the generic function     */
   /* pointer to be used by all commands that can be executed by the    */
   /* test program.                                                     */
typedef int (*CommandFunction_t)(ParameterList_t *TempParam);

   /* The following type definition represents the structure which holds*/
   /* information used in the interpretation and execution of Commands. */
typedef struct _tagCommandTable_t
{
   char              *CommandName;
   CommandFunction_t  CommandFunction;
} CommandTable_t;

   /* The following enumerated type definition defines the different    */
   /* types of service discovery that can be performed.                 */
typedef enum
{
   sdGAPS,
   sdCTS,
   sdNDCS,
   sdRTUS
} Service_Discovery_Type_t;

   /* Structure used to hold all of the GAP LE Parameters.              */
typedef struct _tagGAPLE_Parameters_t
{
   GAP_LE_Connectability_Mode_t ConnectableMode;
   GAP_Discoverability_Mode_t   DiscoverabilityMode;
   GAP_LE_IO_Capability_t       IOCapability;
   Boolean_t                    MITMProtection;
   Boolean_t                    OOBDataPresent;
} GAPLE_Parameters_t;

#define GAPLE_PARAMETERS_DATA_SIZE                       (sizeof(GAPLE_Parameters_t))

   /* The following structure represents the information we will store  */
   /* on a Discovered GAP Service.                                      */
typedef struct _tagGAPS_Client_Info_t
{
   Word_t DeviceNameHandle;
   Word_t DeviceAppearanceHandle;
} GAPS_Client_Info_t;

   /* The following structure holds information on known Device         */
   /* Appearance Values.                                                */
typedef struct _tagGAPS_Device_Appearance_Mapping_t
{
   Word_t  Appearance;
   char   *String;
} GAPS_Device_Appearance_Mapping_t;

   /* The following structure for a Master is used to hold a list of    */
   /* information on all paired devices. For slave we will not use this */
   /* structure.                                                        */
typedef struct _tagDeviceInfo_t
{
   Byte_t                    Flags;
   Byte_t                    EncryptionKeySize;
   GAP_LE_Address_Type_t     ConnectionAddressType;
   BD_ADDR_t                 ConnectionBD_ADDR;
   Long_Term_Key_t           LTK;
   Random_Number_t           Rand;
   Word_t                    EDIV;
   GAPS_Client_Info_t        GAPSClientInfo;
   CTS_Client_Information_t   ClientInfo;
   CTS_Server_Information_t   ServerInfo;
   NDCS_Client_Information_t  NDCS_ClientInfo;
   NDCS_Server_Information_t  NDCS_ServerInfo;
   RTUS_Client_Information_t  RTUS_ClientInfo;
   RTUS_Server_Information_t  RTUS_ServerInfo;
   struct _tagDeviceInfo_t  *NextDeviceInfoPtr;
} DeviceInfo_t;

#define DEVICE_INFO_DATA_SIZE                            (sizeof(DeviceInfo_t))

   /* Defines the bitmask flags that may be set in the DeviceInfo_t     */
   /* structure.                                                        */
#define DEVICE_INFO_FLAGS_LTK_VALID                         0x01
#define DEVICE_INFO_FLAGS_LINK_ENCRYPTED                    0x02
#define DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING     0x04
#define DEVICE_INFO_FLAGS_CTS_SERVICE_DISCOVERY_COMPLETE    0x08
#define DEVICE_INFO_FLAGS_NDCS_SERVICE_DISCOVERY_COMPLETE   0x10
#define DEVICE_INFO_FLAGS_RTUS_SERVICE_DISCOVERY_COMPLETE   0x20

   /* User to represent a structure to hold a BD_ADDR return from       */
   /* BD_ADDRToStr.                                                     */
typedef char BoardStr_t[16];

                        /* The Encryption Root Key should be generated  */
                        /* in such a way as to guarantee 128 bits of    */
                        /* entropy.                                     */
static BTPSCONST Encryption_Key_t ER = {0x28, 0xBA, 0xE1, 0x37, 0x13, 0xB2, 0x20, 0x45, 0x16, 0xB2, 0x19, 0xD0, 0x80, 0xEE, 0x4A, 0x51};

                        /* The Identity Root Key should be generated    */
                        /* in such a way as to guarantee 128 bits of    */
                        /* entropy.                                     */
static BTPSCONST Encryption_Key_t IR = {0x41, 0x09, 0xA0, 0x88, 0x09, 0x6B, 0x70, 0xC0, 0x95, 0x23, 0x3C, 0x8C, 0x48, 0xFC, 0xC9, 0xFE};

                        /* The following keys can be regenerated on the */
                        /* fly using the constant IR and ER keys and    */
                        /* are used globally, for all devices.          */
static Encryption_Key_t DHK;
static Encryption_Key_t IRK;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */
static unsigned int        CTSInstanceID;           /* The following holds the CTS     */
                                                    /* Instance ID that is returned    */
                                                    /* from CTS_Initialize_Service().  */

static unsigned int        NDCSInstanceID;          /* The following holds the NDCS    */
                                                    /* Instance ID that is returned    */
                                                    /* from NDCS_Initialize_Service    */

static unsigned int        RTUSInstanceID;          /* The following holds the RTUS    */
                                                    /* Instance ID that is returned    */
                                                    /* from RTUS_Initialize_Service    */

static unsigned int        GAPSInstanceID;          /* Holds the Instance ID for the   */
                                                    /* GAP Service.                    */

static unsigned int        DISInstanceID;           /* Holds the Instance ID for the   */
                                                    /* DIS Service.                    */

static GAPLE_Parameters_t  LE_Parameters;           /* Holds GAP Parameters like       */
                                                    /* Discoverability, Connectability */
                                                    /* Modes.                          */

static DeviceInfo_t       *DeviceInfoList;          /* Holds the list head for the     */
                                                    /* device info list.               */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static BD_ADDR_t           ConnectionBD_ADDR;       /* Holds the BD_ADDR of the        */
                                                    /* currently connected device.     */

static BD_ADDR_t           CurrentRemoteBD_ADDR;    /* Holds the BD_ADDR of the        */
                                                    /* currently connected device.     */

static unsigned int        ConnectionID;            /* Holds the Connection ID of the  */
                                                    /* currently connected device.     */

static Boolean_t           LocalDeviceIsMaster;     /* Boolean that tells if the local */
                                                    /* device is the master of the     */
                                                    /* current connection.             */

static unsigned int        NumberCommands;          /* Variable which is used to hold  */
                                                    /* the number of Commands that are */
                                                    /* supported by this application.  */
                                                    /* Commands are added individually.*/

static CommandTable_t      CommandTable[MAX_SUPPORTED_COMMANDS]; /* Variable which is  */
                                                    /* used to hold the actual Commands*/
                                                    /* that are supported by this      */
                                                    /* application.                    */

                                                    /* Variable which is used to hold  */
                                                    /* the Local Time Information      */
                                                    /* of Server.                      */
static CTS_Local_Time_Information_Data_t LocalTimeInformation = DEFAULT_LOCAL_TIME_INFO;

                                                    /* Variable which is used to hold  */
                                                    /* the Reference Time Information  */
                                                    /* of Server.                      */

static CTS_Reference_Time_Information_Data_t ReferenceTimeInformation = DEFAULT_REFERENCE_TIME_INFO;

CTS_Current_Time_Data_t   CurrentTime = DEFAULT_CURRENT_TIME;
NDCS_Time_With_Dst_Data_t TimeWithDst = DEFAULT_TIME_WITH_DST;

static Boolean_t           ScanInProgress;          /* A boolean flag to show if a scan*/
                                                    /* is in process                   */

static DWord_t             console_fd;
static TX_THREAD           host_thread;
static TX_BYTE_POOL        pool;

   /* The following is used to map from ATT Error Codes to a printable  */
   /* string.                                                           */
static char *ErrorCodeStr[] =
{
   "ATT_PROTOCOL_ERROR_CODE_NO_ERROR",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_HANDLE",
   "ATT_PROTOCOL_ERROR_CODE_READ_NOT_PERMITTED",
   "ATT_PROTOCOL_ERROR_CODE_WRITE_NOT_PERMITTED",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_PDU",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_AUTHENTICATION",
   "ATT_PROTOCOL_ERROR_CODE_REQUEST_NOT_SUPPORTED",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_OFFSET",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_AUTHORIZATION",
   "ATT_PROTOCOL_ERROR_CODE_PREPARE_QUEUE_FULL",
   "ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_FOUND",
   "ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION_KEY_SIZE",
   "ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH",
   "ATT_PROTOCOL_ERROR_CODE_UNLIKELY_ERROR",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_ENCRYPTION",
   "ATT_PROTOCOL_ERROR_CODE_UNSUPPORTED_GROUP_TYPE",
   "ATT_PROTOCOL_ERROR_CODE_INSUFFICIENT_RESOURCES"
} ;

#define NUMBER_OF_ERROR_CODES     (sizeof(ErrorCodeStr)/sizeof(char *))

   /* The following array is used to map Device Appearance Values to    */
   /* strings.                                                          */
static GAPS_Device_Appearance_Mapping_t AppearanceMappings[] =
{
   { GAP_DEVICE_APPEARENCE_VALUE_UNKNOWN,                        "Unknown"                   },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_PHONE,                  "Generic Phone"             },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_COMPUTER,               "Generic Computer"          },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_WATCH,                  "Generic Watch"             },
   { GAP_DEVICE_APPEARENCE_VALUE_SPORTS_WATCH,                   "Sports Watch"              },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_CLOCK,                  "Generic Clock"             },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_DISPLAY,                "Generic Display"           },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_GENERIC_REMOTE_CONTROL, "Generic Remote Control"    },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_EYE_GLASSES,            "Eye Glasses"               },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_TAG,                    "Generic Tag"               },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_KEYRING,                "Generic Keyring"           },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_MEDIA_PLAYER,           "Generic Media Player"      },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_BARCODE_SCANNER,        "Generic Barcode Scanner"   },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_THERMOMETER,            "Generic Thermometer"       },
   { GAP_DEVICE_APPEARENCE_VALUE_THERMOMETER_EAR,                "Ear Thermometer"           },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_HEART_RATE_SENSOR,      "Generic Heart Rate Sensor" },
   { GAP_DEVICE_APPEARENCE_VALUE_BELT_HEART_RATE_SENSOR,         "Belt Heart Rate Sensor"    },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_BLOOD_PRESSURE,         "Generic Blood Pressure"    },
   { GAP_DEVICE_APPEARENCE_VALUE_BLOOD_PRESSURE_ARM,             "Blood Pressure: ARM"       },
   { GAP_DEVICE_APPEARENCE_VALUE_BLOOD_PRESSURE_WRIST,           "Blood Pressure: Wrist"     },
   { GAP_DEVICE_APPEARENCE_VALUE_HUMAN_INTERFACE_DEVICE,         "Human Interface Device"    },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD,                   "HID Keyboard"              },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_MOUSE,                      "HID Mouse"                 },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_JOYSTICK,                   "HID Joystick"              },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_GAMEPAD,                    "HID Gamepad"               },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_DIGITIZER_TABLET,           "HID Digitizer Tablet"      },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_CARD_READER,                "HID Card Reader"           },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_DIGITAL_PEN,                "HID Digitizer Pen"         },
   { GAP_DEVICE_APPEARENCE_VALUE_HID_BARCODE_SCANNER,            "HID Bardcode Scanner"      },
   { GAP_DEVICE_APPEARENCE_VALUE_GENERIC_GLUCOSE_METER,          "Generic Glucose Meter"     }
} ;

#define NUMBER_OF_APPEARANCE_MAPPINGS     (sizeof(AppearanceMappings)/sizeof(GAPS_Device_Appearance_Mapping_t))

   /* The following string table is used to map HCI Version information */
   /* to an easily displayable version string.                          */
static BTPSCONST char *HCIVersionStrings[] =
{
   "1.0b",
   "1.1",
   "1.2",
   "2.0",
   "2.1",
   "3.0",
   "4.0",
   "4.1",
   "4.2",
   "5.0",
   "Unknown (greater 5.0)"
} ;

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

   /* The following string table is used to map the API I/O Capabilities*/
   /* values to an easily displayable string.                           */
static BTPSCONST char *IOCapabilitiesStrings[] =
{
   "Display Only",
   "Display Yes/No",
   "Keyboard Only",
   "No Input/Output",
   "Keyboard/Display"
} ;

   /* Internal function prototypes.                                     */
static Boolean_t CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR);
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree);
static void FreeDeviceInfoList(DeviceInfo_t **ListHead);

static void UserInterface(void);
static unsigned int StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *Input);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);
static void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address);

static void DisplayKey(char *Prompt, int KeyLength, Byte_t *Key);
static void DisplayIOCapabilities(void);
static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data);
static void DisplayPairingInformation(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities);
static void DisplayUUID(GATT_UUID_t *UUID);
static void DisplayPrompt(void);
static void DisplayUsage(char *UsageString);
static void DisplayFunctionError(char *Function,int Status);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation);
static int CloseStack(void);

static int SetDisc(void);
static int SetConnect(void);
static int SetPairable(void);

static void DumpAppearanceMappings(void);
static Boolean_t AppearanceToString(Word_t Appearance, char **String);
static Boolean_t AppearanceIndexToAppearance(unsigned int Index, Word_t *Appearance);

static void GAPSPopulateHandles(GAPS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);
static int EnableDisableNotificationsIndications(Word_t ClientConfigurationHandle, Word_t ClientConfigurationValue, GATT_Client_Event_Callback_t ClientEventCallback);

static int StartScan(unsigned int BluetoothStackID);
static int StopScan(unsigned int BluetoothStackID);

static int ConnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_LE_Address_Type_t AddressType, Boolean_t UseWhiteList);
static int DisconnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

static void ConfigureCapabilities(GAP_LE_Pairing_Capabilities_t *Capabilities);
static int SendPairingRequest(BD_ADDR_t BD_ADDR, Boolean_t ConnectionMaster);
static int SlavePairingRequestResponse(BD_ADDR_t BD_ADDR);
static int EncryptionInformationRequestResponse(BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information);

static int DisplayHelp(ParameterList_t *TempParam);
static int SetDiscoverabilityMode(ParameterList_t *TempParam);
static int SetConnectabilityMode(ParameterList_t *TempParam);
static int SetPairabilityMode(ParameterList_t *TempParam);
static int ChangePairingParameters(ParameterList_t *TempParam);
static int LEPassKeyResponse(ParameterList_t *TempParam);
static int LEQueryEncryption(ParameterList_t *TempParam);
static int LESetPasskey(ParameterList_t *TempParam);
static int GetLocalAddress(ParameterList_t *TempParam);

static int AdvertiseLE(ParameterList_t *TempParam);

static int StartScanning(ParameterList_t *TempParam);
static int StopScanning(ParameterList_t *TempParam);

static int ConnectLE(ParameterList_t *TempParam);
static int DisconnectLE(ParameterList_t *TempParam);
static int CancelConnect(ParameterList_t *TempParam);

static int PairLE(ParameterList_t *TempParam);

static int DiscoverGAPS(ParameterList_t *TempParam);
static int ReadLocalName(ParameterList_t *TempParam);
static int SetLocalName(ParameterList_t *TempParam);
static int ReadRemoteName(ParameterList_t *TempParam);
static int ReadLocalAppearance(ParameterList_t *TempParam);
static int SetLocalAppearance(ParameterList_t *TempParam);
static int ReadRemoteAppearance(ParameterList_t *TempParam);

static int GetGATTMTU(ParameterList_t *TempParam);
static int SetGATTMTU(ParameterList_t *TempParam);

   /* Current Time Profile Function Commands                            */
static int RegisterCTS(ParameterList_t *TempParam);
static int UnregisterCTS(ParameterList_t *TempParam);
static int DiscoverCTS(ParameterList_t *TempParam);
static int ConfigureRemoteCTS(ParameterList_t *TempParam);
static int SetCurrentTime(ParameterList_t *TempParam);
static int GetCurrentTime(ParameterList_t *TempParam);
static int NotifyCurrentTime(ParameterList_t *TempParam);
static int SetLocalTimeInformation(ParameterList_t *TempParam);
static int GetLocalTimeInformation(ParameterList_t *TempParam);
static int SetReferenceTimeInformation(ParameterList_t *TempParam);
static int GetReferenceTimeInformation(ParameterList_t *TempParam);

   /* Current Time Profile Helper Functions                             */
static void CTSPopulateHandles(DeviceInfo_t *DeviceInfo, GATT_Service_Discovery_Indication_Data_t *ServiceDiscoveryData);
static int FormatCurrentTime(Parameter_t *Params, CTS_Current_Time_Data_t *CurrentTime);
static int FormatLocalTimeInformation(Parameter_t *Params,CTS_Local_Time_Information_Data_t *LocalTime);
static int FormatReferenceTimeInformation(Parameter_t *Params,CTS_Reference_Time_Information_Data_t *ReferenceTime);

static int DecodeDisplayCurrentTime(Word_t BufferLength, Byte_t *Buffer);
static int DecodeDisplayLocalTime(Word_t BufferLength, Byte_t *Buffer);
static int DecodeDisplayReferenceTime(Word_t BufferLength, Byte_t *Buffer);

static void DisplayCurrentTime(const CTS_Current_Time_Data_t *CurrentTime);
static void DisplayLocalTime(const CTS_Local_Time_Information_Data_t *LocalTime);
static void DisplayReferenceTime(const CTS_Reference_Time_Information_Data_t *ReferenceTime);

static void DisplayCurrentTimeUsage(const char* FunName);
static void DisplayLocalTimeUsage(const char* FunName);
static void DisplayReferenceTimeUsage(const char* FunName);

   /* NDCS Profile Function Commands                                    */
static int RegisterNDCS(ParameterList_t *TempParam);
static int UnregisterNDCS(ParameterList_t *TempParam);
static int DiscoverNDCS(ParameterList_t *TempParam);
static int GetTimeWithDST(ParameterList_t *TempParam);
static int SetTimeWithDST(ParameterList_t *TempParam);

   /* NDCS Helper Function Commands                                     */
static void NDCSPopulateHandles(DeviceInfo_t *DeviceInfo, GATT_Service_Discovery_Indication_Data_t *ServiceDiscoveryData);
static void DisplayTimeWithOffset(NDCS_Time_With_Dst_Data_t *TimeWithDST);
static int  DecodeTimeWithOffset(Word_t BufferLength, Byte_t *Buffer);

   /* RTUS Profile Function Commands                                    */
static int RegisterRTUS(ParameterList_t *TempParam);
static int UnRegisterRTUS(ParameterList_t *TempParam);
static int DiscoverRTUS(ParameterList_t *TempParam);
static int WriteTimeUpdateControlPoint(ParameterList_t *TempParam);
static int GetTimeUpdateState(ParameterList_t *TempParam);
static int SetTimeUpdateState(ParameterList_t *TempParam);

   /* RTUS Helper Function Commands                                     */
static void DisplayTimeUpdateStateUsage(const char* FuncName);
static void DisplayTimeUpdateState(RTUS_Time_Update_State_Data_t *Time_Update_State);
static void RTUSPopulateHandles(DeviceInfo_t *DeviceInfo, GATT_Service_Discovery_Indication_Data_t *ServiceDiscoveryData);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID, GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI CTS_EventCallback(unsigned int BluetoothStackID, CTS_Event_Data_t *CTS_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI NDCS_EventCallback(unsigned int BluetoothStackID, NDCS_Event_Data_t *NDCS_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI RTUS_EventCallback(unsigned int BluetoothStackID, RTUS_Event_Data_t *RTUS_Event_Data, unsigned long callbackParameter);
static void BTPSAPI GATT_ClientEventCallback_GAPS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback_CTS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback_NDCS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback_RTUS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Service_Discovery_Event_Callback(unsigned int BluetoothStackID, GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, unsigned long CallbackParameter);

   /* The following function adds the specified Entry to the specified  */
   /* List.  This function allocates and adds an entry to the list that */
   /* has the same attributes as parameters to this function.  This     */
   /* function will return FALSE if NO Entry was added.  This can occur */
   /* if the element passed in was deemed invalid or the actual List    */
   /* Head was invalid.                                                 */
   /* ** NOTE ** This function does not insert duplicate entries into   */
   /*            the list.  An element is considered a duplicate if the */
   /*            Connection BD_ADDR.  When this occurs, this function   */
   /*            returns NULL.                                          */
static Boolean_t CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR)
{
   Boolean_t     ret_val = FALSE;
   DeviceInfo_t *DeviceInfoPtr;

   /* Verify that the passed in parameters seem semi-valid.             */
   if((ListHead) && (!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR)))
   {
      /* Allocate the memory for the entry.                             */
      if((DeviceInfoPtr = BTPS_AllocateMemory(sizeof(DeviceInfo_t))) != NULL)
      {
         /* Initialize the entry.                                       */
         BTPS_MemInitialize(DeviceInfoPtr, 0, sizeof(DeviceInfo_t));
         DeviceInfoPtr->ConnectionAddressType = ConnectionAddressType;
         BTPS_MemCopy(&(DeviceInfoPtr->ConnectionBD_ADDR), &ConnectionBD_ADDR, sizeof(ConnectionBD_ADDR));

         ret_val = BSC_AddGenericListEntry_Actual(ekBD_ADDR_t, BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead), (void *)(DeviceInfoPtr));
         if(!ret_val)
         {
            /* Failed to add to list so we should free the memory that  */
            /* we allocated for the entry.                              */
            BTPS_FreeMemory(DeviceInfoPtr);
         }
      }
   }

   return(ret_val);
}

   /* The following function searches the specified List for the        */
   /* specified Connection BD_ADDR.  This function returns NULL if      */
   /* either the List Head is invalid, the BD_ADDR is invalid, or the   */
   /* Connection BD_ADDR was NOT found.                                 */
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR)
{
   return(BSC_SearchGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead)));
}

   /* The following function searches the specified Key Info List for   */
   /* the specified BD_ADDR and removes it from the List.  This function*/
   /* returns NULL if either the List Head is invalid, the BD_ADDR is   */
   /* invalid, or the specified Entry was NOT present in the list.  The */
   /* entry returned will have the Next Entry field set to NULL, and    */
   /* the caller is responsible for deleting the memory associated with */
   /* this entry by calling the FreeKeyEntryMemory() function.          */
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR)
{
   return(BSC_DeleteGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr), (void **)(ListHead)));
}

   /* This function frees the specified Key Info Information member     */
   /* memory.                                                           */
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree)
{
   BSC_FreeGenericListEntryMemory((void *)(EntryToFree));
}

   /* The following function deletes (and frees all memory) every       */
   /* element of the specified Key Info List. Upon return of this       */
   /* function, the Head Pointer is set to NULL.                        */
static void FreeDeviceInfoList(DeviceInfo_t **ListHead)
{
   BSC_FreeGenericListEntryList((void **)(ListHead), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoPtr));
}

   /* This function is responsible for taking the input from the user   */
   /* and dispatching the appropriate Command Function.  First, this    */
   /* function retrieves a String of user input, parses the user input  */
   /* into Command and Parameters, and finally executes the Command or  */
   /* Displays an Error Message if the input is not a valid Command.    */
static void UserInterface(void)
{
   UserCommand_t TempCommand;
   int  Result = !EXIT_CODE;
   char UserInput[MAX_COMMAND_LENGTH];

   /* First let's make sure that we start on new line.                  */
   PRINTF("\n");

   /* Display the available commands.                                   */
   DisplayHelp(NULL);

   /* Clear the installed command.                                      */
   ClearCommands();

   /* Install the commands relevant for this UI.                        */
   AddCommand("SETDISCOVERABILITYMODE", SetDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetPairabilityMode);
   AddCommand("CHANGEPAIRINGPARAMETERS", ChangePairingParameters);
   AddCommand("GETLOCALADDRESS", GetLocalAddress);
   AddCommand("ADVERTISELE", AdvertiseLE);
   AddCommand("STARTSCANNING", StartScanning);
   AddCommand("STOPSCANNING", StopScanning);
   AddCommand("CONNECTLE", ConnectLE);
   AddCommand("DISCONNECTLE", DisconnectLE);
   AddCommand("CANCELCONNECT", CancelConnect);
   AddCommand("PAIRLE", PairLE);
   AddCommand("LEPASSKEYRESPONSE", LEPassKeyResponse);
   AddCommand("QUERYENCRYPTIONMODE", LEQueryEncryption);
   AddCommand("SETPASSKEY", LESetPasskey);
   AddCommand("DISCOVERGAPS", DiscoverGAPS);
   AddCommand("GETLOCALNAME", ReadLocalName);
   AddCommand("SETLOCALNAME", SetLocalName);
   AddCommand("GETREMOTENAME", ReadRemoteName);
   AddCommand("GETLOCALAPPEARANCE", ReadLocalAppearance);
   AddCommand("SETLOCALAPPEARANCE", SetLocalAppearance);
   AddCommand("GETREMOTEAPPEARANCE", ReadRemoteAppearance);
   AddCommand("REGISTERCTS", RegisterCTS);
   AddCommand("UNREGISTERCTS", UnregisterCTS);
   AddCommand("DISCOVERCTS", DiscoverCTS);
   AddCommand("CONFIGUREREMOTECTS", ConfigureRemoteCTS);
   AddCommand("SETCURRENTTIME", SetCurrentTime);
   AddCommand("GETCURRENTTIME", GetCurrentTime);
   AddCommand("NOTIFYCURRENTTIME", NotifyCurrentTime);
   AddCommand("SETLOCALTIMEINFORMATION", SetLocalTimeInformation);
   AddCommand("GETLOCALTIMEINFORMATION", GetLocalTimeInformation);
   AddCommand("SETREFERENCETIMEINFORMATION", SetReferenceTimeInformation);
   AddCommand("GETREFERENCETIMEINFORMATION", GetReferenceTimeInformation);
   AddCommand("REGISTERNDCS", RegisterNDCS);
   AddCommand("UNREGISTERNDCS",UnregisterNDCS);
   AddCommand("DISCOVERNDCS", DiscoverNDCS);
   AddCommand("GETTIMEWITHDST", GetTimeWithDST);
   AddCommand("SETTIMEWITHDST",SetTimeWithDST);
   AddCommand("REGISTERRTUS", RegisterRTUS);
   AddCommand("UNREGISTERRTUS",UnRegisterRTUS);
   AddCommand("DISCOVERRTUS",DiscoverRTUS);
   AddCommand("GETTIMEUPDATESTATE",GetTimeUpdateState);
   AddCommand("SETTIMEUPDATESTATE",SetTimeUpdateState);
   AddCommand("WRITETIMEUPDATECONTROLPOINT",WriteTimeUpdateControlPoint);
   AddCommand("GETMTU", GetGATTMTU);
   AddCommand("SETMTU", SetGATTMTU);
   AddCommand("HELP", DisplayHelp);

   /* This is the main loop of the program.  It gets user input from the*/
   /* command window, make a call to the command parser, and command    */
   /* interpreter.  After the function has been ran it then check the   */
   /* return value and displays an error message when appropriate.  If  */
   /* the result returned is ever the EXIT_CODE the loop will exit      */
   /* leading the exit of the program.                                  */
   while(Result != EXIT_CODE)
   {
      /* Initialize the value of the variable used to store the users   */
      /* input and output "Input: " to the command window to inform the */
      /* user that another command may be entered.                      */
      UserInput[0] = '\0';

      /* Output an Input Shell-type prompt.                             */
      DisplayPrompt();

      /* Retrieve the command entered by the user and store it in the   */
      /* User Input Buffer.  Note that this command will fail if the    */
      /* application receives a signal which cause the standard file    */
      /* streams to be closed.  If this happens the loop will be broken */
      /* out of so the application can exit.                            */
      if(Fgets(UserInput, sizeof(UserInput), console_fd) != NULL)
      {
         /* Start a newline for the results.                            */
         PRINTF("\n");

         /* Next, check to see if a command was input by the user.      */
         if(strlen(UserInput))
         {
            /* The string input by the user contains a value, now run   */
            /* the string through the Command Parser.                   */
            if(CommandParser(&TempCommand, UserInput) >= 0)
            {
               /* The Command was successfully parsed, run the Command. */
               Result = CommandInterpreter(&TempCommand);

               switch(Result)
               {
                  case INVALID_COMMAND_ERROR:
                     PRINTF("Invalid Command.\n");
                     break;
                  case FUNCTION_ERROR:
                     PRINTF("Function Error.\n");
                     break;
                  case EXIT_CODE:
                     break;
               }
            }
            else
               PRINTF("Invalid Input.\n");
         }
      }
      else
      {
         Result = EXIT_CODE;
      }
   }
}

   /* The following function is responsible for converting number       */
   /* strings to their unsigned integer equivalent.  This function can  */
   /* handle leading and tailing white space, however it does not handle*/
   /* signed or comma delimited values.  This function takes as its     */
   /* input the string which is to be converted.  The function returns  */
   /* zero if an error occurs otherwise it returns the value parsed from*/
   /* the string passed as the input parameter.                         */
static unsigned int StringToUnsignedInteger(char *StringInteger)
{
   int          IsHex;
   unsigned int Index;
   unsigned int ret_val = 0;

   /* Before proceeding make sure that the parameter that was passed as */
   /* an input appears to be at least semi-valid.                       */
   if((StringInteger) && (BTPS_StringLength(StringInteger)))
   {
      /* Initialize the variable.                                       */
      Index = 0;

      /* Next check to see if this is a hexadecimal number.             */
      if(BTPS_StringLength(StringInteger) > 2)
      {
         if((StringInteger[0] == '0') && ((StringInteger[1] == 'x') || (StringInteger[1] == 'X')))
         {
            IsHex = 1;

            /* Increment the String passed the Hexadecimal prefix.      */
            StringInteger += 2;
         }
         else
            IsHex = 0;
      }
      else
         IsHex = 0;

      /* Process the value differently depending on whether or not a    */
      /* Hexadecimal Number has been specified.                         */
      if(!IsHex)
      {
         /* Decimal Number has been specified.                          */
         while(1)
         {
            /* First check to make sure that this is a valid decimal    */
            /* digit.                                                   */
            if((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9'))
            {
               /* This is a valid digit, add it to the value being      */
               /* built.                                                */
               ret_val += (StringInteger[Index] & 0xF);

               /* Determine if the next digit is valid.                 */
               if(((Index + 1) < BTPS_StringLength(StringInteger)) && (StringInteger[Index+1] >= '0') && (StringInteger[Index+1] <= '9'))
               {
                  /* The next digit is valid so multiply the current    */
                  /* return value by 10.                                */
                  ret_val *= 10;
               }
               else
               {
                  /* The next value is invalid so break out of the loop.*/
                  break;
               }
            }

            Index++;
         }
      }
      else
      {
         /* Hexadecimal Number has been specified.                      */
         while(1)
         {
            /* First check to make sure that this is a valid Hexadecimal*/
            /* digit.                                                   */
            if(((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9')) || ((StringInteger[Index] >= 'a') && (StringInteger[Index] <= 'f')) || ((StringInteger[Index] >= 'A') && (StringInteger[Index] <= 'F')))
            {
               /* This is a valid digit, add it to the value being      */
               /* built.                                                */
               if((StringInteger[Index] >= '0') && (StringInteger[Index] <= '9'))
                  ret_val += (StringInteger[Index] & 0xF);
               else
               {
                  if((StringInteger[Index] >= 'a') && (StringInteger[Index] <= 'f'))
                     ret_val += (StringInteger[Index] - 'a' + 10);
                  else
                     ret_val += (StringInteger[Index] - 'A' + 10);
               }

               /* Determine if the next digit is valid.                 */
               if(((Index + 1) < BTPS_StringLength(StringInteger)) && (((StringInteger[Index+1] >= '0') && (StringInteger[Index+1] <= '9')) || ((StringInteger[Index+1] >= 'a') && (StringInteger[Index+1] <= 'f')) || ((StringInteger[Index+1] >= 'A') && (StringInteger[Index+1] <= 'F'))))
               {
                  /* The next digit is valid so multiply the current    */
                  /* return value by 16.                                */
                  ret_val *= 16;
               }
               else
               {
                  /* The next value is invalid so break out of the loop.*/
                  break;
               }
            }

            Index++;
         }
      }
   }

   return(ret_val);
}

   /* The following function is responsible for parsing strings into    */
   /* components.  The first parameter of this function is a pointer to */
   /* the String to be parsed.  This function will return the start of  */
   /* the string upon success and a NULL pointer on all errors.         */
static char *StringParser(char *String)
{
   int   Index;
   char *ret_val = NULL;

   /* Before proceeding make sure that the string passed in appears to  */
   /* be at least semi-valid.                                           */
   if((String) && (BTPS_StringLength(String)))
   {
      /* The string appears to be at least semi-valid.  Search for the  */
      /* first space character and replace it with a NULL terminating   */
      /* character.                                                     */
      for(Index=0, ret_val=String;Index < BTPS_StringLength(String);Index++)
      {
         /* Is this the space character.                                */
         if((String[Index] == ' ') || (String[Index] == '\r') || (String[Index] == '\n'))
         {
            /* This is the space character, replace it with a NULL      */
            /* terminating character and set the return value to the    */
            /* beginning character of the string.                       */
            String[Index] = '\0';
            break;
         }
      }
   }

   return(ret_val);
}

   /* This function is responsible for taking command strings and       */
   /* parsing them into a command, param1, and param2.  After parsing   */
   /* this string the data is stored into a UserCommand_t structure to  */
   /* be used by the interpreter.  The first parameter of this function */
   /* is the structure used to pass the parsed command string out of the*/
   /* function.  The second parameter of this function is the string    */
   /* that is parsed into the UserCommand structure.  Successful        */
   /* execution of this function is denoted by a return value of zero.  */
   /* Negative return values denote an error in the parsing of the      */
   /* string parameter.                                                 */
static int CommandParser(UserCommand_t *TempCommand, char *Input)
{
   int            ret_val;
   int            StringLength;
   char          *LastParameter;
   unsigned int   Count         = 0;

   /* Before proceeding make sure that the passed parameters appear to  */
   /* be at least semi-valid.                                           */
   if((TempCommand) && (Input) && (BTPS_StringLength(Input)))
   {
      /* Retrieve the first token in the string, this should be the     */
      /* command.                                                       */
      TempCommand->Command = StringParser(Input);

      /* Flag that there are NO Parameters for this Command Parse.      */
      TempCommand->Parameters.NumberofParameters = 0;

       /* Check to see if there is a Command                            */
      if(TempCommand->Command)
      {
         /* Initialize the return value to zero to indicate success on  */
         /* commands with no parameters.                                */
         ret_val    = 0;

         /* Adjust the UserInput pointer and StringLength to remove the */
         /* Command from the data passed in before parsing the          */
         /* parameters.                                                 */
         Input        += BTPS_StringLength(TempCommand->Command) + 1;
         StringLength  = BTPS_StringLength(Input);

         /* There was an available command, now parse out the parameters*/
         while((StringLength > 0) && ((LastParameter = StringParser(Input)) != NULL))
         {
            /* There is an available parameter, now check to see if     */
            /* there is room in the UserCommand to store the parameter  */
            if(Count < (sizeof(TempCommand->Parameters.Params)/sizeof(Parameter_t)))
            {
               /* Save the parameter as a string.                       */
               TempCommand->Parameters.Params[Count].strParam = LastParameter;

               /* Save the parameter as an unsigned int intParam will   */
               /* have a value of zero if an error has occurred.        */
               TempCommand->Parameters.Params[Count].intParam = StringToUnsignedInteger(LastParameter);

               Count++;
               Input        += BTPS_StringLength(LastParameter) + 1;
               StringLength -= BTPS_StringLength(LastParameter) + 1;

               ret_val = 0;
            }
            else
            {
               /* Be sure we exit out of the Loop.                      */
               StringLength = 0;

               ret_val      = TO_MANY_PARAMS;
            }
         }

         /* Set the number of parameters in the User Command to the     */
         /* number of found parameters                                  */
         TempCommand->Parameters.NumberofParameters = Count;
      }
      else
      {
         /* No command was specified                                    */
         ret_val = NO_COMMAND_ERROR;
      }
   }
   else
   {
      /* One or more of the passed parameters appear to be invalid.     */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* This function is responsible for determining the command in which */
   /* the user entered and running the appropriate function associated  */
   /* with that command.  The first parameter of this function is a     */
   /* structure containing information about the command to be issued.  */
   /* This information includes the command name and multiple parameters*/
   /* which maybe be passed to the function to be executed.  Successful */
   /* execution of this function is denoted by a return value of zero.  */
   /* A negative return value implies that command was not found and is */
   /* invalid.                                                          */
static int CommandInterpreter(UserCommand_t *TempCommand)
{
   int               i;
   int               ret_val;
   CommandFunction_t CommandFunction;

   /* If the command is not found in the table return with an invalid   */
   /* command error                                                     */
   ret_val = INVALID_COMMAND_ERROR;

   /* Let's make sure that the data passed to us appears semi-valid.    */
   if((TempCommand) && (TempCommand->Command))
   {
      /* Now, let's make the Command string all upper case so that we   */
      /* compare against it.                                            */
      for(i=0;i<(int)BTPS_StringLength(TempCommand->Command);i++)
      {
         if((TempCommand->Command[i] >= 'a') && (TempCommand->Command[i] <= 'z'))
            TempCommand->Command[i] -= (char)('a' - 'A');
      }

      /* Check to see if the command which was entered was exit.        */
      if(BTPS_MemCompare(TempCommand->Command, "QUIT", BTPS_StringLength("QUIT")) != 0)
      {
         /* The command entered is not exit so search for command in    */
         /* table.                                                      */
         if((CommandFunction = FindCommand(TempCommand->Command)) != NULL)
         {
            /* The command was found in the table so call the command.  */
            if(!(ret_val = ((*CommandFunction)(&TempCommand->Parameters))))
            {
               /* Return success to the caller.                         */
               ret_val = 0;
            }
            else
            {
               if (ret_val != EXIT_CODE)
                  ret_val = FUNCTION_ERROR;
            }
         }
      }
      else
      {
         /* The command entered is exit, set return value to EXIT_CODE  */
         /* and return.                                                 */
         ret_val = EXIT_CODE;
      }
   }
   else
      ret_val = INVALID_PARAMETERS_ERROR;

   return(ret_val);
}

   /* The following function is provided to allow a means to            */
   /* programmaticly add Commands the Global (to this module) Command   */
   /* Table.  The Command Table is simply a mapping of Command Name     */
   /* (NULL terminated ASCII string) to a command function.  This       */
   /* function returns zero if successful, or a non-zero value if the   */
   /* command could not be added to the list.                           */
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction)
{
   int ret_val;

   /* First, make sure that the parameters passed to us appear to be    */
   /* semi-valid.                                                       */
   if((CommandName) && (CommandFunction))
   {
      /* Next, make sure that we still have room in the Command Table   */
      /* to add commands.                                               */
      if(NumberCommands < MAX_SUPPORTED_COMMANDS)
      {
         /* Simply add the command data to the command table and        */
         /* increment the number of supported commands.                 */
         CommandTable[NumberCommands].CommandName       = CommandName;
         CommandTable[NumberCommands++].CommandFunction = CommandFunction;

         /* Return success to the caller.                               */
         ret_val                                        = 0;
      }
      else
         ret_val = 1;
   }
   else
      ret_val = 1;

   return(ret_val);
}

   /* The following function searches the Command Table for the         */
   /* specified Command.  If the Command is found, this function returns*/
   /* a NON-NULL Command Function Pointer.  If the command is not found */
   /* this function returns NULL.                                       */
static CommandFunction_t FindCommand(char *Command)
{
   unsigned int      Index;
   CommandFunction_t ret_val;

   /* First, make sure that the command specified is semi-valid.        */
   if(Command)
   {
      /* Now loop through each element in the table to see if there is  */
      /* a match.                                                       */
      for(Index = 0, ret_val = NULL; ((Index < NumberCommands) && (!ret_val)); Index++)
      {
         if((BTPS_StringLength(CommandTable[Index].CommandName) == BTPS_StringLength(Command)) && (BTPS_MemCompare(Command, CommandTable[Index].CommandName, BTPS_StringLength(CommandTable[Index].CommandName)) == 0))
            ret_val = CommandTable[Index].CommandFunction;
      }
   }
   else
      ret_val = NULL;

   return(ret_val);
}

   /* The following function is provided to allow a means to clear out  */
   /* all available commands from the command table.                    */
static void ClearCommands(void)
{
   /* Simply flag that there are no commands present in the table.      */
   NumberCommands = 0;
}

   /* The following function is responsible for converting data of type */
   /* BD_ADDR to a string.  The first parameter of this function is the */
   /* BD_ADDR to be converted to a string.  The second parameter of this*/
   /* function is a pointer to the string in which the converted BD_ADDR*/
   /* is to be stored.                                                  */
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr)
{
   BTPS_SprintF((char *)BoardStr, "0x%02X%02X%02X%02X%02X%02X", Board_Address.BD_ADDR5, Board_Address.BD_ADDR4, Board_Address.BD_ADDR3, Board_Address.BD_ADDR2, Board_Address.BD_ADDR1, Board_Address.BD_ADDR0);
}

   /* The following function is responsible for the specified string    */
   /* into data of type BD_ADDR.  The first parameter of this function  */
   /* is the BD_ADDR string to be converted to a BD_ADDR.  The second   */
   /* parameter of this function is a pointer to the BD_ADDR in which   */
   /* the converted BD_ADDR String is to be stored.                     */
static void StrToBD_ADDR(char *BoardStr, BD_ADDR_t *Board_Address)
{
   char Buffer[5];

   if((BoardStr) && (BTPS_StringLength(BoardStr) == sizeof(BD_ADDR_t)*2) && (Board_Address))
   {
      Buffer[0] = '0';
      Buffer[1] = 'x';
      Buffer[4] = '\0';

      Buffer[2] = BoardStr[0];
      Buffer[3] = BoardStr[1];
      Board_Address->BD_ADDR5 = (Byte_t)StringToUnsignedInteger(Buffer);

      Buffer[2] = BoardStr[2];
      Buffer[3] = BoardStr[3];
      Board_Address->BD_ADDR4 = (Byte_t)StringToUnsignedInteger(Buffer);

      Buffer[2] = BoardStr[4];
      Buffer[3] = BoardStr[5];
      Board_Address->BD_ADDR3 = (Byte_t)StringToUnsignedInteger(Buffer);

      Buffer[2] = BoardStr[6];
      Buffer[3] = BoardStr[7];
      Board_Address->BD_ADDR2 = (Byte_t)StringToUnsignedInteger(Buffer);

      Buffer[2] = BoardStr[8];
      Buffer[3] = BoardStr[9];
      Board_Address->BD_ADDR1 = (Byte_t)StringToUnsignedInteger(Buffer);

      Buffer[2] = BoardStr[10];
      Buffer[3] = BoardStr[11];
      Board_Address->BD_ADDR0 = (Byte_t)StringToUnsignedInteger(Buffer);
   }
   else
   {
      if(Board_Address)
         BTPS_MemInitialize(Board_Address, 0, sizeof(BD_ADDR_t));
   }
}

static void DisplayKey(char *Prompt, int KeyLength, Byte_t *Key)
{
   int Index;

   PRINTF("%s", Prompt);

   for(Index = 0; Index < KeyLength; Index++)
      PRINTF("%02X", Key[Index]);

   PRINTF("\n");
}

   /* Displays the current I/O Capabilities.                            */
static void DisplayIOCapabilities(void)
{
   PRINTF("I/O Capabilities: %s, MITM: %s.\n", IOCapabilitiesStrings[(unsigned int)(LE_Parameters.IOCapability - licDisplayOnly)], LE_Parameters.MITMProtection?"TRUE":"FALSE");
}

   /* Utility function to display advertising data.                     */
static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data)
{
   unsigned int Index;
   unsigned int Index2;

   /* Verify that the input parameters seem semi-valid.                 */
   if(Advertising_Data)
   {
      for(Index = 0; Index < Advertising_Data->Number_Data_Entries; Index++)
      {
         PRINTF("  AD Type: 0x%02X.\n", (unsigned int)(Advertising_Data->Data_Entries[Index].AD_Type));
         PRINTF("  AD Length: 0x%02X.\n", (unsigned int)(Advertising_Data->Data_Entries[Index].AD_Data_Length));
         if(Advertising_Data->Data_Entries[Index].AD_Data_Buffer)
         {
            PRINTF("  AD Data: ");
            for(Index2 = 0; Index2 < Advertising_Data->Data_Entries[Index].AD_Data_Length; Index2++)
            {
               PRINTF("0x%02X ", Advertising_Data->Data_Entries[Index].AD_Data_Buffer[Index2]);
            }
            PRINTF("\n");
         }
      }
   }
}

   /* The following function displays the pairing capabilities that is  */
   /* passed into this function.                                        */
static void DisplayPairingInformation(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities)
{
   /* Display the IO Capability.                                        */
   switch(Pairing_Capabilities.IO_Capability)
   {
      case licDisplayOnly:
         PRINTF("   IO Capability:       lcDisplayOnly.\n");
         break;
      case licDisplayYesNo:
         PRINTF("   IO Capability:       lcDisplayYesNo.\n");
         break;
      case licKeyboardOnly:
         PRINTF("   IO Capability:       lcKeyboardOnly.\n");
         break;
      case licNoInputNoOutput:
         PRINTF("   IO Capability:       lcNoInputNoOutput.\n");
         break;
      case licKeyboardDisplay:
         PRINTF("   IO Capability:       lcKeyboardDisplay.\n");
         break;
   }

   PRINTF("   MITM:                %s.\n", (Pairing_Capabilities.MITM)?"TRUE":"FALSE");
   PRINTF("   Bonding Type:        %s.\n", (Pairing_Capabilities.Bonding_Type == lbtBonding)?"Bonding":"No Bonding");
   PRINTF("   OOB:                 %s.\n", (Pairing_Capabilities.OOB_Present)?"OOB":"OOB Not Present");
   PRINTF("   Encryption Key Size: %d.\n", Pairing_Capabilities.Maximum_Encryption_Key_Size);
   PRINTF("   Sending Keys: \n");
   PRINTF("      LTK:              %s.\n", ((Pairing_Capabilities.Sending_Keys.Encryption_Key)?"YES":"NO"));
   PRINTF("      IRK:              %s.\n", ((Pairing_Capabilities.Sending_Keys.Identification_Key)?"YES":"NO"));
   PRINTF("      CSRK:             %s.\n", ((Pairing_Capabilities.Sending_Keys.Signing_Key)?"YES":"NO"));
   PRINTF("   Receiving Keys: \n");
   PRINTF("      LTK:              %s.\n", ((Pairing_Capabilities.Receiving_Keys.Encryption_Key)?"YES":"NO"));
   PRINTF("      IRK:              %s.\n", ((Pairing_Capabilities.Receiving_Keys.Identification_Key)?"YES":"NO"));
   PRINTF("      CSRK:             %s.\n", ((Pairing_Capabilities.Receiving_Keys.Signing_Key)?"YES":"NO"));
}

   /* The following function is provided to properly print a UUID.      */
static void DisplayUUID(GATT_UUID_t *UUID)
{
   if(UUID)
   {
      if(UUID->UUID_Type == guUUID_16)
         PRINTF("%02X%02X", UUID->UUID.UUID_16.UUID_Byte1, UUID->UUID.UUID_16.UUID_Byte0);
      else
      {
         if(UUID->UUID_Type == guUUID_128)
         {
            PRINTF("%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X", UUID->UUID.UUID_128.UUID_Byte15, UUID->UUID.UUID_128.UUID_Byte14, UUID->UUID.UUID_128.UUID_Byte13,
                                                                                       UUID->UUID.UUID_128.UUID_Byte12, UUID->UUID.UUID_128.UUID_Byte11, UUID->UUID.UUID_128.UUID_Byte10,
                                                                                       UUID->UUID.UUID_128.UUID_Byte9,  UUID->UUID.UUID_128.UUID_Byte8,  UUID->UUID.UUID_128.UUID_Byte7,
                                                                                       UUID->UUID.UUID_128.UUID_Byte6,  UUID->UUID.UUID_128.UUID_Byte5,  UUID->UUID.UUID_128.UUID_Byte4,
                                                                                       UUID->UUID.UUID_128.UUID_Byte3,  UUID->UUID.UUID_128.UUID_Byte2,  UUID->UUID.UUID_128.UUID_Byte1,
                                                                                       UUID->UUID.UUID_128.UUID_Byte0);
         }
      }
   }

   PRINTF(".\n");
}

   /* Displays the correct prompt depending on the Server/Client Mode.  */
static void DisplayPrompt(void)
{
   PRINTF("\nTIP>");
}

   /* Displays a usage string..                                         */
static void DisplayUsage(char *UsageString)
{
   PRINTF("Usage: %s.\n",UsageString);
}

   /* Displays a function error message.                                */
static void DisplayFunctionError(char *Function,int Status)
{
   PRINTF("%s Failed: %d.\n", Function, Status);
}

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation)
{
   int           Result;
   int           ret_val = 0;
   char          BluetoothAddress[16];
   BD_ADDR_t     BD_ADDR;
   unsigned int  ServiceID;
   HCI_Version_t HCIVersion;

   /* First check to see if the Stack has already been opened.          */
   if(!BluetoothStackID)
   {
      /* Next, makes sure that the Driver Information passed appears to */
      /* be semi-valid.                                                 */
      if(HCI_DriverInformation)
      {
         PRINTF("\n");

         PRINTF("OpenStack().\n");

         /* Initialize the Stack                                        */
         Result = BSC_Initialize(HCI_DriverInformation, 0);

         /* Next, check the return value of the initialization to see if*/
         /* it was successful.                                          */
         if(Result > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            BluetoothStackID = Result;
            PRINTF("Bluetooth Stack ID: %d.\n", BluetoothStackID);

            /* Initialize the Default Pairing Parameters.               */
            LE_Parameters.IOCapability   = DEFAULT_IO_CAPABILITY;
            LE_Parameters.OOBDataPresent = FALSE;
            LE_Parameters.MITMProtection = DEFAULT_MITM_PROTECTION;

            if(!HCI_Version_Supported(BluetoothStackID, &HCIVersion))
               PRINTF("Device Chipset: %s.\n", (HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]);

            /* Let's output the Bluetooth Device Address so that the    */
            /* user knows what the Device Address is.                   */
            if(!GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR))
            {
               BD_ADDRToStr(BD_ADDR, BluetoothAddress);

               PRINTF("BD_ADDR: %s\n", BluetoothAddress);
            }

            /* Flag that no connection is currently active.             */
            ASSIGN_BD_ADDR(ConnectionBD_ADDR, 0, 0, 0, 0, 0, 0);
            ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0, 0, 0, 0, 0, 0);
            LocalDeviceIsMaster = FALSE;

            /* Regenerate IRK and DHK from the constant Identity Root   */
            /* Key.                                                     */
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 1,0, &IRK);
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 3, 0, &DHK);

            /* Flag that we have no Key Information in the Key List.    */
            DeviceInfoList = NULL;

            /* Initialize the GATT Service.                             */
            if(!(Result = GATT_Initialize(BluetoothStackID, GATT_INITIALIZATION_FLAGS_SUPPORT_LE, GATT_Connection_Event_Callback, 0)))
            {
               /* Initialize the GAPS Service.                          */
               Result = GAPS_Initialize_Service(BluetoothStackID, &ServiceID);
               if(Result > 0)
               {
                  /* Save the Instance ID of the GAP Service.           */
                  GAPSInstanceID = (unsigned int)Result;

                  /* Set the GAP Device Name and Device Appearance.     */
                  GAPS_Set_Device_Name(BluetoothStackID, GAPSInstanceID, LE_DEMO_DEVICE_NAME);
                  GAPS_Set_Device_Appearance(BluetoothStackID, GAPSInstanceID, GAP_DEVICE_APPEARENCE_VALUE_GENERIC_COMPUTER);

                  /* Initialize the DIS Service.                        */
                  Result = DIS_Initialize_Service(BluetoothStackID, &ServiceID);
                  if(Result > 0)
                  {
                     /* Save the Instance ID of the GAP Service.        */
                     DISInstanceID = (unsigned int)Result;

                     /* Set the discoverable attributes                 */
                     DIS_Set_Manufacturer_Name(BluetoothStackID, DISInstanceID, BTPS_VERSION_COMPANY_NAME_STRING);
                     DIS_Set_Model_Number(BluetoothStackID, DISInstanceID, BTPS_VERSION_VERSION_STRING);
                     DIS_Set_Serial_Number(BluetoothStackID, DISInstanceID, BTPS_VERSION_VERSION_STRING);

                     /* Return success to the caller.                   */
                     ret_val        = 0;
                  }
               }
               else
               {
                  /* The Stack was NOT initialized successfully, inform */
                  /* the user and set the return value of the           */
                  /* initialization function to an error.               */
                  DisplayFunctionError("GAPS_Initialize_Service", Result);

                  /* Cleanup GATT Module.                               */
                  GATT_Cleanup(BluetoothStackID);

                  BluetoothStackID = 0;

                  ret_val          = UNABLE_TO_INITIALIZE_STACK;
               }
            }
            else
            {
               /* The Stack was NOT initialized successfully, inform the*/
               /* user and set the return value of the initialization   */
               /* function to an error.                                 */
               DisplayFunctionError("GATT_Initialize", Result);

               BluetoothStackID = 0;

               ret_val          = UNABLE_TO_INITIALIZE_STACK;
            }
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            DisplayFunctionError("BSC_Initialize", Result);

            BluetoothStackID = 0;

            ret_val          = UNABLE_TO_INITIALIZE_STACK;
         }
      }
      else
      {
         /* One or more of the necessary parameters are invalid.        */
         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for closing the SS1         */
   /* Bluetooth Protocol Stack.  This function requires that the        */
   /* Bluetooth Protocol stack previously have been initialized via the */
   /* OpenStack() function.  This function returns zero on successful   */
   /* execution and a negative value on all errors.                     */
static int CloseStack(void)
{
   int ret_val = 0;

   /* First check to see if the Stack has been opened.                  */
   if(BluetoothStackID)
   {
      /* Cleanup GAP Service Module.                                    */
      if(GAPSInstanceID)
      {
         GAPS_Cleanup_Service(BluetoothStackID, GAPSInstanceID);

         GAPSInstanceID = 0;
      }

      /* Cleanup CTS Service.                                           */
      if(CTSInstanceID)
      {
         CTS_Cleanup_Service(BluetoothStackID, CTSInstanceID);

         CTSInstanceID = 0;
      }

      /* Cleanup NDCS Service.                                          */
      if(NDCSInstanceID)
      {
         NDCS_Cleanup_Service(BluetoothStackID, NDCSInstanceID);

         NDCSInstanceID = 0;
      }

      /* Cleanup RTUS Service.                                          */
      if(RTUSInstanceID)
      {
         RTUS_Cleanup_Service(BluetoothStackID, RTUSInstanceID);

         RTUSInstanceID = 0;
      }

      /* Cleanup DIS Service Module.                                    */
      if(DISInstanceID)
      {
         DIS_Cleanup_Service(BluetoothStackID, DISInstanceID);

         DISInstanceID = 0;
      }

      /* Cleanup GATT Module.                                           */
      GATT_Cleanup(BluetoothStackID);

      /* Simply close the Stack                                         */
      BSC_Shutdown(BluetoothStackID);

      /* Free BTPSKRNL allocated memory.                                */
      BTPS_DeInit();

      PRINTF("Stack Shutdown.\n");

      /* Free the Key List.                                             */
      FreeDeviceInfoList(&DeviceInfoList);

      /* Flag that the Stack is no longer initialized.                  */
      BluetoothStackID = 0;

      /* Flag success to the caller.                                    */
      ret_val          = 0;
   }
   else
   {
      /* A valid Stack ID does not exist, inform to user.               */
      ret_val = UNABLE_TO_INITIALIZE_STACK;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the Local       */
   /* Bluetooth Device into General Discoverablity Mode.  Once in this  */
   /* mode the Device will respond to Inquiry Scans from other Bluetooth*/
   /* Devices.  This function requires that a valid Bluetooth Stack ID  */
   /* exists before running.  This function returns zero on successful  */
   /* execution and a negative value if an error occurred.              */
static int SetDisc(void)
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* * NOTE * Discoverability is only applicable when we are        */
      /*          advertising so save the default Discoverability Mode  */
      /*          for later.                                            */
      LE_Parameters.DiscoverabilityMode = dmGeneralDiscoverableMode;
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the Local       */
   /* Bluetooth Device into Connectable Mode.  Once in this mode the    */
   /* Device will respond to Page Scans from other Bluetooth Devices.   */
   /* This function requires that a valid Bluetooth Stack ID exists     */
   /* before running.  This function returns zero on success and a      */
   /* negative value if an error occurred.                              */
static int SetConnect(void)
{
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* * NOTE * Connectability is only an applicable when advertising */
      /*          so we will just save the default connectability for   */
      /*          the next time we enable advertising.                  */
      LE_Parameters.ConnectableMode = lcmConnectable;
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for placing the local       */
   /* Bluetooth device into Pairable mode.  Once in this mode the device*/
   /* will response to pairing requests from other Bluetooth devices.   */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int SetPairable(void)
{
   int Result;
   int ret_val = 0;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* Attempt to set the attached device to be pairable.             */
      Result = GAP_LE_Set_Pairability_Mode(BluetoothStackID, lpmPairableMode);

      /* Next, check the return value of the GAP Set Pairability mode   */
      /* command for successful execution.                              */
      if(!Result)
      {
         /* The device has been set to pairable mode, now register an   */
         /* Authentication Callback to handle the Authentication events */
         /* if required.                                                */
         Result = GAP_LE_Register_Remote_Authentication(BluetoothStackID, GAP_LE_Event_Callback, (unsigned long)0);

         /* Next, check the return value of the GAP Register Remote     */
         /* Authentication command for successful execution.            */
         if(Result)
         {
            /* An error occurred while trying to execute this function. */
            DisplayFunctionError("GAP_LE_Register_Remote_Authentication", Result);

            ret_val = Result;
         }
      }
      else
      {
         /* An error occurred while trying to make the device pairable. */
         DisplayFunctionError("GAP_LE_Set_Pairability_Mode", Result);

         ret_val = Result;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is a utility function that is used to dump */
   /* the Appearance to String Mapping Table.                           */
static void DumpAppearanceMappings(void)
{
   unsigned int Index;

   for(Index=0;Index<NUMBER_OF_APPEARANCE_MAPPINGS;++Index)
      PRINTF("   %u = %s.\n", Index, AppearanceMappings[Index].String);
}

   /* The following function is used to map a Appearance Value to it's  */
   /* string representation.  This function returns TRUE on success or  */
   /* FALSE otherwise.                                                  */
static Boolean_t AppearanceToString(Word_t Appearance, char **String)
{
   Boolean_t    ret_val;
   unsigned int Index;

   /* Verify that the input parameters are semi-valid.                  */
   if(String)
   {
      for(Index=0,ret_val=FALSE;Index<NUMBER_OF_APPEARANCE_MAPPINGS;++Index)
      {
         if(AppearanceMappings[Index].Appearance == Appearance)
         {
            *String = AppearanceMappings[Index].String;
            ret_val = TRUE;
            break;
         }
      }
   }
   else
      ret_val = FALSE;

   return(ret_val);
}

   /* The following function is used to map an Index into the Appearance*/
   /* Mapping table to it's Appearance Value.  This function returns    */
   /* TRUE on success or FALSE otherwise.                               */
static Boolean_t AppearanceIndexToAppearance(unsigned int Index, Word_t *Appearance)
{
   Boolean_t ret_val;

   if((Index < NUMBER_OF_APPEARANCE_MAPPINGS) && (Appearance))
   {
      *Appearance = AppearanceMappings[Index].Appearance;
      ret_val     = TRUE;
   }
   else
      ret_val = FALSE;

   return(ret_val);
}

   /* The following function is a utility function that provides a      */
   /* mechanism of populating discovered GAP Service Handles.           */
static void GAPSPopulateHandles(GAPS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo)
{
   unsigned int                       Index1;
   GATT_Characteristic_Information_t *CurrentCharacteristic;

   /* Verify that the input parameters are semi-valid.                  */
   if((ClientInfo) && (ServiceInfo) && (ServiceInfo->ServiceInformation.UUID.UUID_Type == guUUID_16) && (GAP_COMPARE_GAP_SERVICE_UUID_TO_UUID_16(ServiceInfo->ServiceInformation.UUID.UUID.UUID_16)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceInfo->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1 = 0; Index1 < ServiceInfo->NumberOfCharacteristics; Index1++, CurrentCharacteristic++)
         {
            /* All GAP Service UUIDs are defined to be 16 bit UUIDs.    */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_16)
            {
               /* Determine which characteristic this is.               */
               if(!GAP_COMPARE_GAP_DEVICE_NAME_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  if(!GAP_COMPARE_GAP_DEVICE_APPEARANCE_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
                     continue;
                  else
                  {
                     ClientInfo->DeviceAppearanceHandle = CurrentCharacteristic->Characteristic_Handle;
                     continue;
                  }
               }
               else
               {
                  ClientInfo->DeviceNameHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
            }
         }
      }
   }
}

   /* The following function is a utility function that provides a      */
   /* mechanism of populating a CTS Client Information structure with   */
   /* the information discovered from a GDIS Discovery operation.       */
static void CTSPopulateHandles(DeviceInfo_t *DeviceInfo, GATT_Service_Discovery_Indication_Data_t *ServiceDiscoveryData)
{
   unsigned int                                  Index1;
   unsigned int                                  Index2;
   GATT_Characteristic_Information_t            *CurrentCharacteristic;
   GATT_Characteristic_Descriptor_Information_t *CurrentDescriptor;

   /* Verify that the input parameters are semi-valid.                  */
   if((DeviceInfo) && (ServiceDiscoveryData) && (ServiceDiscoveryData->ServiceInformation.UUID.UUID_Type == guUUID_16) && (CTS_COMPARE_CTS_SERVICE_UUID_TO_UUID_16(ServiceDiscoveryData->ServiceInformation.UUID.UUID.UUID_16)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceDiscoveryData->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1 = 0; Index1 < ServiceDiscoveryData->NumberOfCharacteristics; Index1++, CurrentCharacteristic++)
         {
            /* All HTS UUIDs are defined to be 16 bit UUIDs.            */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_16)
            {
               /* Determine which characteristic this is.               */
               if(!CTS_COMPARE_CTS_CURRENT_TIME_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  if(!CTS_COMPARE_CTS_LOCAL_TIME_INFORMATION_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
                  {
                     if(!CTS_COMPARE_CTS_REFERENCE_TIME_INFORMATION_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
                     {
                        continue;
                     }
                     else /* CTS_REFERENCE_TIME_INFORMATION_UUID        */
                     {
                        DeviceInfo->ClientInfo.Reference_Time_Information = CurrentCharacteristic->Characteristic_Handle;

                        /* Verify that read is supported.               */
                        if(!(CurrentCharacteristic->Characteristic_Properties & GATT_CHARACTERISTIC_PROPERTIES_READ))
                           PRINTF("Warning - Mandatory read property of Reference Time Information characteristic not supported!\n");
                        continue;
                     }
                  }
                  else /* CTS_LOCAL_TIME_INFORMATION                    */
                  {
                     DeviceInfo->ClientInfo.Local_Time_Information = CurrentCharacteristic->Characteristic_Handle;

                     /* Verify that read is supported.                  */
                     if(!(CurrentCharacteristic->Characteristic_Properties & GATT_CHARACTERISTIC_PROPERTIES_READ))
                        PRINTF("Warning - Mandatory read property of Local Time Information characteristic not supported!\n");
                  }
               }
               else /* CTS_CURRENT_TIME_UUID                            */
               {
                  DeviceInfo->ClientInfo.Current_Time = CurrentCharacteristic->Characteristic_Handle;

                  /* Verify that read is supported.                     */
                  if(!(CurrentCharacteristic->Characteristic_Properties & GATT_CHARACTERISTIC_PROPERTIES_READ))
                     PRINTF("Warning - Mandatory read property of Current Time characteristic not supported!\n");

                  /* Verify that notify is supported.                   */
                  if(!(CurrentCharacteristic->Characteristic_Properties & GATT_CHARACTERISTIC_PROPERTIES_NOTIFY))
                     PRINTF("Warning - Mandatory notify property of Current Time characteristic not supported!\n");

                  CurrentDescriptor = CurrentCharacteristic->DescriptorList;
                  /* Get CCC Descriptor                                 */
                  for(Index2 = 0; Index2 < CurrentCharacteristic->NumberOfDescriptors; Index2++)
                  {
                     if(CurrentDescriptor->Characteristic_Descriptor_UUID.UUID_Type == guUUID_16)
                     {
                        if(GATT_COMPARE_CLIENT_CHARACTERISTIC_CONFIGURATION_ATTRIBUTE_TYPE_TO_BLUETOOTH_UUID_16(CurrentCharacteristic->DescriptorList[Index2].Characteristic_Descriptor_UUID.UUID.UUID_16))
                        {
                           DeviceInfo->ClientInfo.Current_Time_Client_Configuration = CurrentCharacteristic->DescriptorList[Index2].Characteristic_Descriptor_Handle;
                        }
                     }
                  }

                  continue;
               }
            }
         }
      }
   }
}

   /* The following function is a utility function that provides a      */
   /* mechanism of populating a NDCS ClientInformation structure with   */
   /* the information discovered from a GDIS Discovery operation.       */
static void NDCSPopulateHandles(DeviceInfo_t *DeviceInfo, GATT_Service_Discovery_Indication_Data_t *ServiceDiscoveryData)
{
   Word_t                            *ClientConfigurationHandle;
   unsigned int                       Index1;
   GATT_Characteristic_Information_t *CurrentCharacteristic;

   /* Use ClientConfigurationHandle here to avoid compiler warning      */
   ClientConfigurationHandle = NULL;
   if(ClientConfigurationHandle != NULL)
      ;
   /* Verify that the input parameters are semi-valid.                  */
   if((DeviceInfo) && (ServiceDiscoveryData) && (ServiceDiscoveryData->ServiceInformation.UUID.UUID_Type == guUUID_16) && (NDCS_COMPARE_NDCS_SERVICE_UUID_TO_UUID_16(ServiceDiscoveryData->ServiceInformation.UUID.UUID.UUID_16)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceDiscoveryData->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1 = 0; Index1 < ServiceDiscoveryData->NumberOfCharacteristics; Index1++, CurrentCharacteristic++)
         {
            /* All NDCS UUIDs are defined to be 16 bit UUIDs.           */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_16)
            {
               ClientConfigurationHandle = NULL;

               /* Determine which characteristic this is.               */
               if(!NDCS_COMPARE_NDCS_TIME_WITH_DST_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  continue;
               }
               else /*NDCS_TIME_WITH_DST_UUID                           */
               {
                  DeviceInfo->NDCS_ClientInfo.Time_With_Dst = CurrentCharacteristic->Characteristic_Handle;
                  /* Verify that read is supported                      */
                  if(!(CurrentCharacteristic->Characteristic_Properties & GATT_CHARACTERISTIC_PROPERTIES_READ))
                     PRINTF("Warning - Mandatory read property of Time With DST offset characteristic not supported!\n");
                  continue;
               }
            }
         }
      }
   }
}

   /* The following function is a utility function that provides a      */
   /* mechanism of populating a RTUS ClientInformation structure with   */
   /* the information discovered from a GDIS Discovery operation.       */
static void RTUSPopulateHandles(DeviceInfo_t *DeviceInfo, GATT_Service_Discovery_Indication_Data_t *ServiceDiscoveryData)
{
   Word_t                            *ClientConfigurationHandle;
   unsigned int                       Index1;
   GATT_Characteristic_Information_t *CurrentCharacteristic;

   /* Use ClientConfigurationHandle here to avoid compiler warning      */
   ClientConfigurationHandle = NULL;
   if(ClientConfigurationHandle != NULL)
      ;
   /* Verify that the input parameters are semi-valid.                  */
   if((DeviceInfo) && (ServiceDiscoveryData) && (ServiceDiscoveryData->ServiceInformation.UUID.UUID_Type == guUUID_16) && (RTUS_COMPARE_RTUS_SERVICE_UUID_TO_UUID_16(ServiceDiscoveryData->ServiceInformation.UUID.UUID.UUID_16)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceDiscoveryData->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1 = 0; Index1 < ServiceDiscoveryData->NumberOfCharacteristics; Index1++, CurrentCharacteristic++)
         {
            if(RTUS_COMPARE_RTUS_TIME_UPDATE_CONTROL_POINT_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
            {
               /* All RTUS UUIDs are defined to be 16 bit UUIDs.        */
               if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_16)
               {
                  ClientConfigurationHandle = NULL;

                  DeviceInfo->RTUS_ClientInfo.Control_Point = CurrentCharacteristic->Characteristic_Handle;

                  /* Verify that write without response is supported.   */
                  if(!(CurrentCharacteristic->Characteristic_Properties & GATT_CHARACTERISTIC_PROPERTIES_WRITE_WITHOUT_RESPONSE))
                     PRINTF("Warning - Mandatory write without response property of Time update Control Point characteristic not supported!\n");
               }
            }
            else
            {
               if(RTUS_COMPARE_RTUS_TIME_UPDATE_STATE_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  /* All RTUS UUIDs are defined to be 16 bit UUIDs.     */
                  if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_16)
                  {
                     ClientConfigurationHandle = NULL;

                     DeviceInfo->RTUS_ClientInfo.Time_Update_State = CurrentCharacteristic->Characteristic_Handle;

                     /* Verify that Read Property is supported.         */
                     if(!(CurrentCharacteristic->Characteristic_Properties & GATT_CHARACTERISTIC_PROPERTIES_READ))
                        PRINTF("Warning - Mandatory Read property of Time update State characteristic not supported!\n");
                  }
               }
            }
         }
      }
   }
}

   /* The following function is used to enable/disable notifications on */
   /* a specified handle.  This function returns the positive non-zero  */
   /* Transaction ID of the Write Request or a negative error code.     */
static int EnableDisableNotificationsIndications(Word_t ClientConfigurationHandle, Word_t ClientConfigurationValue, GATT_Client_Event_Callback_t ClientEventCallback)
{
   int              ret_val;
   NonAlignedWord_t Buffer;

   /* Verify the input parameters.                                      */
   if((BluetoothStackID) && (ConnectionID) && (ClientConfigurationHandle))
   {
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer, ClientConfigurationValue);

      ret_val = GATT_Write_Request(BluetoothStackID, ConnectionID, ClientConfigurationHandle, sizeof(Buffer), &Buffer, ClientEventCallback, ClientConfigurationHandle);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}

   /* The following function is responsible for starting a scan.        */
static int StartScan(unsigned int BluetoothStackID)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if(BluetoothStackID)
   {
      /* Not currently scanning, go ahead and attempt to perform the    */
      /* scan.                                                          */
      Result = GAP_LE_Perform_Scan(BluetoothStackID, stActive, 10, 10, latPublic, fpNoFilter, TRUE, GAP_LE_Event_Callback, 0);

      if(!Result)
      {
         PRINTF("Scan started successfully.\n");
      }
      else
      {
         /* Unable to start the scan.                                   */
         PRINTF("Unable to perform scan: %d\n", Result);
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function is responsible for stopping on on-going    */
   /* scan.                                                             */
static int StopScan(unsigned int BluetoothStackID)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if(BluetoothStackID)
   {
      Result = GAP_LE_Cancel_Scan(BluetoothStackID);
      if(!Result)
      {
         PRINTF("Scan stopped successfully.\n");
      }
      else
      {
         /* Error stopping scan.                                        */
         PRINTF("Unable to stop scan: %d\n", Result);
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function is responsible for creating an LE          */
   /* connection to the specified Remote Device.                        */
static int ConnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR, GAP_LE_Address_Type_t AddressType, Boolean_t UseWhiteList)
{
   int                            Result;
   unsigned int                   WhiteListChanged;
   GAP_LE_White_List_Entry_t      WhiteListEntry;
   GAP_LE_Connection_Parameters_t ConnectionParameters;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
      if(COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
      {
         /* Remove any previous entries for this device from the White  */
         /* List.                                                       */
         WhiteListEntry.Address_Type = AddressType;
         WhiteListEntry.Address      = BD_ADDR;

         GAP_LE_Remove_Device_From_White_List(BluetoothStackID, 1, &WhiteListEntry, &WhiteListChanged);

         if(UseWhiteList)
            Result = GAP_LE_Add_Device_To_White_List(BluetoothStackID, 1, &WhiteListEntry, &WhiteListChanged);
         else
            Result = 1;

         /* If everything has been successful, up until this point, then*/
         /* go ahead and attempt the connection.                        */
         if(Result >= 0)
         {
            /* Initialize the connection parameters.                    */
            ConnectionParameters.Connection_Interval_Min    = 50;
            ConnectionParameters.Connection_Interval_Max    = 200;
            ConnectionParameters.Minimum_Connection_Length  = 0;
            ConnectionParameters.Maximum_Connection_Length  = 10000;
            ConnectionParameters.Slave_Latency              = 0;
            ConnectionParameters.Supervision_Timeout        = 20000;

            /* Everything appears correct, go ahead and attempt to make */
            /* the connection.                                          */
            Result = GAP_LE_Create_Connection(BluetoothStackID, 100, 100, Result ? fpNoFilter : fpWhiteList, AddressType, Result ? &BD_ADDR : NULL, latPublic, &ConnectionParameters, GAP_LE_Event_Callback, 0);

            if(!Result)
            {
               PRINTF("Connection Request successful.\n");

               /* Note the connection information.                      */
               ConnectionBD_ADDR = BD_ADDR;
            }
            else
            {
               /* Unable to create connection.                          */
               PRINTF("Unable to create connection: %d.\n", Result);
            }
         }
         else
         {
            /* Unable to add device to White List.                      */
            PRINTF("Unable to add device to White List.\n");
         }
      }
      else
      {
         /* Device already connected.                                   */
         PRINTF("Device is already connected.\n");

         Result = -2;
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function is provided to allow a mechanism to        */
   /* disconnect a currently connected device.                          */
static int DisconnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
      {
         Result = GAP_LE_Disconnect(BluetoothStackID, BD_ADDR);

         if(!Result)
         {
            PRINTF("Disconnect Request successful.\n");
         }
         else
         {
            /* Unable to disconnect device.                             */
            PRINTF("Unable to disconnect device: %d.\n", Result);
         }
      }
      else
      {
         /* Device not connected.                                       */
         PRINTF("Device is not connected.\n");

         Result = 0;
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* The following function provides a mechanism to configure a        */
   /* Pairing Capabilities structure with the application's pairing     */
   /* parameters.                                                       */
static void ConfigureCapabilities(GAP_LE_Pairing_Capabilities_t *Capabilities)
{
   /* Make sure the Capabilities pointer is semi-valid.                 */
   if(Capabilities)
   {
      /* Initialize the capabilities.                                   */
      BTPS_MemInitialize(Capabilities, 0, GAP_LE_PAIRING_CAPABILITIES_SIZE);

      /* Configure the Pairing Capabilities structure.                  */
      Capabilities->Bonding_Type                    = lbtBonding;
      Capabilities->IO_Capability                   = LE_Parameters.IOCapability;
      Capabilities->MITM                            = LE_Parameters.MITMProtection;
      Capabilities->OOB_Present                     = LE_Parameters.OOBDataPresent;

      /* ** NOTE ** This application always requests that we use the    */
      /*            maximum encryption because this feature is not a    */
      /*            very good one, if we set less than the maximum we   */
      /*            will internally in GAP generate a key of the        */
      /*            maximum size (we have to do it this way) and then   */
      /*            we will zero out how ever many of the MSBs          */
      /*            necessary to get the maximum size.  Also as a slave */
      /*            we will have to use Non-Volatile Memory (per device */
      /*            we are paired to) to store the negotiated Key Size. */
      /*            By requesting the maximum (and by not storing the   */
      /*            negotiated key size if less than the maximum) we    */
      /*            allow the slave to power cycle and regenerate the   */
      /*            LTK for each device it is paired to WITHOUT storing */
      /*            any information on the individual devices we are    */
      /*            paired to.                                          */
      Capabilities->Maximum_Encryption_Key_Size        = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;

      /* This application only demonstrates using Long Term Key's (LTK) */
      /* for encryption of a LE Link, however we could request and send */
      /* all possible keys here if we wanted to.                        */
      Capabilities->Receiving_Keys.Encryption_Key     = TRUE;
      Capabilities->Receiving_Keys.Identification_Key = FALSE;
      Capabilities->Receiving_Keys.Signing_Key        = FALSE;

      Capabilities->Sending_Keys.Encryption_Key       = TRUE;
      Capabilities->Sending_Keys.Identification_Key   = FALSE;
      Capabilities->Sending_Keys.Signing_Key          = FALSE;
   }
}

   /* The following function provides a mechanism for sending a pairing */
   /* request to a device that is connected on an LE Link.              */
static int SendPairingRequest(BD_ADDR_t BD_ADDR, Boolean_t ConnectionMaster)
{
   int                           ret_val;
   BoardStr_t                    BoardStr;
   GAP_LE_Pairing_Capabilities_t Capabilities;

   /* Make sure a Bluetooth Stack is open.                              */
   if(BluetoothStackID)
   {
      /* Make sure the BD_ADDR is valid.                                */
      if(!COMPARE_NULL_BD_ADDR(BD_ADDR))
      {
         /* Configure the application pairing parameters.               */
         ConfigureCapabilities(&Capabilities);

         /* Set the BD_ADDR of the device that we are attempting to pair*/
         /* with.                                                       */
         CurrentRemoteBD_ADDR = BD_ADDR;

         BD_ADDRToStr(BD_ADDR, BoardStr);
         PRINTF("Attempting to Pair to %s.\n", BoardStr);

         /* Attempt to pair to the remote device.                       */
         if(ConnectionMaster)
         {
            /* Start the pairing process.                               */
            ret_val = GAP_LE_Pair_Remote_Device(BluetoothStackID, BD_ADDR, &Capabilities, GAP_LE_Event_Callback, 0);

            PRINTF("     GAP_LE_Pair_Remote_Device returned %d.\n", ret_val);
         }
         else
         {
            /* As a slave we can only request that the Master start     */
            /* the pairing process.                                     */
            ret_val = GAP_LE_Request_Security(BluetoothStackID, BD_ADDR, Capabilities.Bonding_Type, Capabilities.MITM, GAP_LE_Event_Callback, 0);

            PRINTF("     GAP_LE_Request_Security returned %d.\n", ret_val);
         }
      }
      else
      {
         PRINTF("Invalid Parameters.\n");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      PRINTF("Stack ID Invalid.\n");

      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function provides a mechanism of sending a Slave    */
   /* Pairing Response to a Master's Pairing Request.                   */
static int SlavePairingRequestResponse(BD_ADDR_t BD_ADDR)
{
   int                                          ret_val;
   BoardStr_t                                   BoardStr;
   GAP_LE_Authentication_Response_Information_t AuthenticationResponseData;

   /* Make sure a Bluetooth Stack is open.                              */
   if(BluetoothStackID)
   {
      BD_ADDRToStr(BD_ADDR, BoardStr);
      PRINTF("Sending Pairing Response to %s.\n", BoardStr);

      /* We must be the slave if we have received a Pairing Request     */
      /* thus we will respond with our capabilities.                    */
      AuthenticationResponseData.GAP_LE_Authentication_Type = larPairingCapabilities;
      AuthenticationResponseData.Authentication_Data_Length = GAP_LE_PAIRING_CAPABILITIES_SIZE;

      /* Configure the Application Pairing Parameters.                  */
      ConfigureCapabilities(&(AuthenticationResponseData.Authentication_Data.Pairing_Capabilities));

      /* Attempt to pair to the remote device.                          */
      ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, &AuthenticationResponseData);

      PRINTF("GAP_LE_Authentication_Response returned %d.\n", ret_val);
   }
   else
   {
      PRINTF("Stack ID Invalid.\n");

      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is provided to allow a mechanism of        */
   /* responding to a request for Encryption Information to send to a   */
   /* remote device.                                                    */
static int EncryptionInformationRequestResponse(BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information)
{
   int    ret_val;
   Word_t LocalDiv;

   /* Make sure a Bluetooth Stack is open.                              */
   if(BluetoothStackID)
   {
      /* Make sure the input parameters are semi-valid.                 */
      if((!COMPARE_NULL_BD_ADDR(BD_ADDR)) && (GAP_LE_Authentication_Response_Information))
      {
         PRINTF("   Calling GAP_LE_Generate_Long_Term_Key.\n");

         /* Generate a new LTK, EDIV and Rand tuple.                    */
         ret_val = GAP_LE_Generate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.LTK), &LocalDiv, &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.EDIV), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Rand));
         if(!ret_val)
         {
            PRINTF("   Encryption Information Request Response.\n");

            /* Response to the request with the LTK, EDIV and Rand      */
            /* values.                                                  */
            GAP_LE_Authentication_Response_Information->GAP_LE_Authentication_Type                                     = larEncryptionInformation;
            GAP_LE_Authentication_Response_Information->Authentication_Data_Length                                     = GAP_LE_ENCRYPTION_INFORMATION_DATA_SIZE;
            GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Encryption_Key_Size = KeySize;

            ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, GAP_LE_Authentication_Response_Information);
            if(!ret_val)
            {
               PRINTF("   GAP_LE_Authentication_Response (larEncryptionInformation) success.\n");
            }
            else
            {
               DisplayFunctionError("GAP_LE_Authentication_Response", ret_val);
            }
         }
         else
         {
            DisplayFunctionError("GAP_LE_Generate_Long_Term_Key", ret_val);
         }
      }
      else
      {
         PRINTF("Invalid Parameters.\n");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      PRINTF("Stack ID Invalid.\n");

      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is used to format a Current Time from user */
   /* input.  The first parameter is a parameter list resulting from a  */
   /* parsed command line string in the format defined by the           */
   /* DisplayCurrentTimeUsage function.  The second parameter is a      */
   /* pointer to a CTS_Current_Time_Data_t data structure that will be  */
   /* filled on successful function completion.  Zero will be returned  */
   /* on success; otherwise, INVALID_PARAMETERS_ERROR will be returned. */
static int FormatCurrentTime(Parameter_t *Params, CTS_Current_Time_Data_t *CurrentTime)
{
   CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Year    = (Word_t)Params[0].intParam;
   CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Month   = (CTS_Month_Of_Year_Type_t)Params[1].intParam;
   CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Day     = (Byte_t)Params[2].intParam;
   CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Hours   = (Byte_t)Params[3].intParam;
   CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Minutes = (Byte_t)Params[4].intParam;
   CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Seconds = (Byte_t)Params[5].intParam;
   CurrentTime->Exact_Time.Day_Date_Time.Day_Of_Week       = (CTS_Week_Day_Type_t)Params[6].intParam;
   CurrentTime->Exact_Time.Fractions256                    = (Byte_t)Params[7].intParam;
   CurrentTime->Adjust_Reason_Mask                              = (Byte_t)Params[8].intParam;

   switch (CurrentTime->Adjust_Reason_Mask)
   {
      case 0:
         CurrentTime->Adjust_Reason_Mask = CTS_CURRENT_TIME_ADJUST_REASON_MANUAL_TIME_UPDATE ;
         break;

      case 1:
         CurrentTime->Adjust_Reason_Mask = CTS_CURRENT_TIME_ADJUST_REASON_CHANGE_OF_TIMEZONE ;
         break;

      case 2:
         CurrentTime->Adjust_Reason_Mask = CTS_CURRENT_TIME_ADJUST_REASON_CHANGE_OF_TIMEZONE | CTS_CURRENT_TIME_ADJUST_REASON_MANUAL_TIME_UPDATE;
         break;

      case 3:
         CurrentTime->Adjust_Reason_Mask = CTS_CURRENT_TIME_ADJUST_REASON_CHANGE_OF_DST;
         break;

      case 4:
         CurrentTime->Adjust_Reason_Mask = CTS_CURRENT_TIME_ADJUST_REASON_CHANGE_OF_DST | CTS_CURRENT_TIME_ADJUST_REASON_MANUAL_TIME_UPDATE;
         break;

      case 5:
         CurrentTime->Adjust_Reason_Mask = CTS_CURRENT_TIME_ADJUST_REASON_EXTERNAL_REFERENCE_TIME_UPDATE;
         break;

      default:
         CurrentTime->Adjust_Reason_Mask = 0x00;
         break;
   }

   return(0);
}

   /* The following function is used to format a Local Time Information */
   /* from user input.  The first parameter is a parameter list         */
   /* resulting from a parsed command line string in the format defined */
   /* by the DisplayLocalTimeUsage function. The second parameter is a  */
   /* pointer to a CTS_Local_Time_Information_Data_t data structure that*/
   /* will be filled on successful function completion.  Zero will be   */
   /* returned on success; otherwise, INVALID_PARAMETERS_ERROR will be  */
   /* returned.                                                         */
static int FormatLocalTimeInformation(Parameter_t *Params,CTS_Local_Time_Information_Data_t *LocalTime)
{
   LocalTime->Time_Zone            = (CTS_Time_Zone_Type_t)Params[0].intParam;
   LocalTime->Daylight_Saving_Time = (CTS_DST_Offset_Type_t)Params[1].intParam;

   return(0);
}

   /* The following function is used to format a Reference Time         */
   /* Information from user input.  The first parameter is a parameter  */
   /* list resulting from a parsed command line string in the format    */
   /* defined by the DisplayReferenceTimeUsage function. The second     */
   /* parameter is a pointer to a CTS_Reference_Time_Information_Data_t */
   /* data structure that will be filled on successful function         */
   /* completion.  Zero will be returned on success; otherwise,         */
   /* INVALID_PARAMETERS_ERROR will be returned.                        */
static int FormatReferenceTimeInformation(Parameter_t *Params,CTS_Reference_Time_Information_Data_t *ReferenceTime)
{
   ReferenceTime->Source             = (CTS_Time_Source_Type_t)Params[0].intParam;
   ReferenceTime->Accuracy           = (Byte_t)Params[1].intParam;
   ReferenceTime->Days_Since_Update  = (Byte_t)Params[2].intParam;
   ReferenceTime->Hours_Since_Update = (Byte_t)Params[3].intParam;

   return(0);
}

   /* The following function is used to decode and display a current    */
   /* time at client end while getting read response or notification    */
   /* from Time Service. The first parameter provides the buffer length */
   /* of the data to be decoded, the second parameter provides the data */
   /* packet to be decoded, This function returns a zero if successful  */
   /* or a negative return error code if an error occurs.               */
static int DecodeDisplayCurrentTime(Word_t BufferLength, Byte_t *Buffer)
{
   CTS_Current_Time_Data_t CurrentTime;
   int                     ret_val;

   /* Verify that the input parameters seem semi-valid.                 */
   if((BufferLength) && (Buffer))
   {
      /* Decode the Current Time.                                       */
      ret_val = CTS_Decode_Current_Time(BufferLength, Buffer, &CurrentTime);
      if(ret_val == 0)
      {
         DisplayCurrentTime(&CurrentTime);
      }
      else
         DisplayFunctionError("CTS_Decode_Current_Time", ret_val);
   }
   else
   {
      ret_val = CTS_ERROR_INVALID_PARAMETER;
   }

   return(ret_val);
}

   /* The following function is used to decode and display a local time */
   /* information at client end while getting read response from Time   */
   /* Service. The first parameter provides the buffer length of the    */
   /* data to be decoded, the second parameter provides the data packet */
   /* to be decoded, This function returns a zero if successful or a    */
   /* negative return error code if an error occurs.                    */
static int DecodeDisplayLocalTime(Word_t BufferLength, Byte_t *Buffer)
{
   CTS_Local_Time_Information_Data_t LocalTimeInfo;
   int                               ret_val;

   /* Verify that the input parameters seem semi-valid.                 */
   if((BufferLength) && (Buffer))
   {
      /* Decode the Current Time.                                       */
      ret_val = CTS_Decode_Local_Time_Information(BufferLength, Buffer, &LocalTimeInfo);
      if(ret_val == 0)
      {
         DisplayLocalTime(&LocalTimeInfo);
      }
      else
         DisplayFunctionError("CTS_Decode_Local_Time_Information", ret_val);
   }
   else
   {
      ret_val = CTS_ERROR_INVALID_PARAMETER;
   }

   return(ret_val);
}

   /* The following function is used to decode and display a reference  */
   /* time information at client end while getting read response from   */
   /* Time Service. The first parameter provides the buffer length of   */
   /* the data to be decoded, the second parameter provides the data    */
   /* packet to be decoded, This function returns a zero if successful  */
   /* or a negative return error code if an error occurs.               */
static int DecodeDisplayReferenceTime(Word_t BufferLength, Byte_t *Buffer)
{
   CTS_Reference_Time_Information_Data_t RefTimeInfo;
   int                                   ret_val;

   /* Verify that the input parameters seem semi-valid.                 */
   if((BufferLength) && (Buffer))
   {
      /* Decode the Current Time.                                       */
      ret_val = CTS_Decode_Reference_Time_Information(BufferLength, Buffer, &RefTimeInfo);
      if(ret_val == 0)
      {
         DisplayReferenceTime(&RefTimeInfo);
      }
      else
         DisplayFunctionError("CTS_Decode_Reference_Time_Information", ret_val);
   }
   else
   {
      ret_val = CTS_ERROR_INVALID_PARAMETER;
   }

   return(ret_val);
}

   /* Display Current Time in YYYY/MM/DD HH:MM:SS Day_Of_Week           */
   /* Fraction256 Adjust_Reason_Mask Format                             */
static void DisplayCurrentTime(const CTS_Current_Time_Data_t *CurrentTime)
{
   PRINTF("Current Time:\n   %u/%u/%u %u:%u:%u %u %u %u", CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Year,
                                          CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Month,
                                          CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Day,
                                          CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Hours,
                                          CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Minutes,
                                          CurrentTime->Exact_Time.Day_Date_Time.Date_Time.Seconds,
                                          CurrentTime->Exact_Time.Day_Date_Time.Day_Of_Week,
                                          CurrentTime->Exact_Time.Fractions256,
                                          CurrentTime->Adjust_Reason_Mask);
}

   /* Display Local Time Information                                    */
static void DisplayLocalTime(const CTS_Local_Time_Information_Data_t *LocalTime)
{
   PRINTF("TimeZone %u DST Offset %u", LocalTime->Time_Zone, LocalTime->Daylight_Saving_Time);
}

   /* Display Reference Time Information                                */
static void DisplayReferenceTime(const CTS_Reference_Time_Information_Data_t *ReferenceTime)
{
   PRINTF("\nSource :%u \nAccuracy :%u \nDays_Since_Update :%u \nHours_Since_Update :%u",
                                    ReferenceTime->Source,ReferenceTime->Accuracy,
                                    ReferenceTime->Days_Since_Update, ReferenceTime->Hours_Since_Update);
}

   /* The following function displays the usage information for         */
   /* SetCurrentTime command.  The only parameter is a string to the    */
   /* command name that will be displayed  with the information.        */
static void DisplayCurrentTimeUsage(const char *FunName)
{
   PRINTF("Usage : %s [Year (1582-9999)] (Month (0-12)] [Day (1-31)]\n", FunName);
   PRINTF("           [Hours (0-23)] [Minutes (0-59)] [Seconds (0-59)]\n");
   PRINTF("           [DayOfWeek (0-7)] [Fractions256 (0-255)] [AdjustReason (0-5)]\n");
   PRINTF("   Where AdjustReason = \n");
   PRINTF("      0 : ManualTimeUpdate\n");
   PRINTF("      1 : TimeZoneUpadte\n");
   PRINTF("      2 : TimeZoneUpdateManually\n");
   PRINTF("      3 : DSTOffsetUpdate\n");
   PRINTF("      4 : DSTOffsetUpdateManually\n");
   PRINTF("      5 : ExternalReferenceTimeUpdate\n");
}

   /* The following function displays the usage information for         */
   /* SetLocalTimeInformation command.  The only parameter is a string  */
   /* to the command name that will be displayed with the information.  */
static void DisplayLocalTimeUsage(const char* FunName)
{
   PRINTF("Usage : %s [TimeZone (-48 to 56)] [DST Offset (0 to 8)]\n", FunName);
}

   /* The following function displays the usage information for         */
   /* SetReferenceTimeInformation command.The only parameter is a string*/
   /* to the command name that will be displayed with the information.  */
static void DisplayReferenceTimeUsage(const char* FunName)
{
   PRINTF("Usage : %s [Time Source (0 - 7)] [Accuracy (0-253)] [Days Since Update (0 - 254)] [Hours Since Update (0 - 23)]\n", FunName);
}

static void DisplayTimeWithOffset(NDCS_Time_With_Dst_Data_t *TimeWithDST)
{
   PRINTF("%u/%u/%u %u:%u:%u [%u]\n",TimeWithDST->Date_Time.Day,
                                         TimeWithDST->Date_Time.Month,
                                         TimeWithDST->Date_Time.Year,
                                         TimeWithDST->Date_Time.Hours,
                                         TimeWithDST->Date_Time.Minutes,
                                         TimeWithDST->Date_Time.Seconds,
                                         TimeWithDST->Dst_Offset);
}

   /* The following function is used to decode and display a Time       */
   /* With DST Offset at client end while getting read response         */
   /* from NDCS Time Service. The first parameter provides the buffer   */
   /* len of the data to be decoded, the second parameter provides the  */
   /* packet to be decoded, This function returns a zero if successful  */
   /* or a negative return error code if an error occurs.               */
static int  DecodeTimeWithOffset(Word_t BufferLength, Byte_t *Buffer)
{
   NDCS_Time_With_Dst_Data_t Time_With_Dst;
   int                       Result;

   /* Verify that the input parameters seem semi-valid.                 */
   if((BufferLength) && (Buffer))
   {
      /* Decode the Current Time.                                       */
      Result = NDCS_Decode_Time_With_Dst(BufferLength, Buffer, &Time_With_Dst);
      if(Result == 0)
      {
         DisplayTimeWithOffset(&Time_With_Dst);
      }
      else
         DisplayFunctionError("NDCS_Decode_Time_With_Dst", Result);
   }
   else
   {
      Result = NDCS_ERROR_INVALID_PARAMETER;
   }
   return(Result);
}

   /* The following function is used to decode and display a Time       */
   /* Update State at client end while getting read response            */
   /* from RTUS Time Service. The first parameter provides the buffer   */
   /* len of the data to be decoded, the second parameter provides the  */
   /* packet to be decoded, This function returns a zero if successful  */
   /* or a negative return error code if an error occurs.               */
static int  DecodeTimeUpdateState(Word_t BufferLength, Byte_t *Buffer)
{
   RTUS_Time_Update_State_Data_t Time_Update_State;
   int                           Result;

   /* Verify that the input parameters seem semi-valid.                 */
   if((BufferLength) && (Buffer))
   {
      /* Decode the Time Update State.                                  */
      Result = RTUS_Decode_Time_Update_State(BufferLength, Buffer, &Time_Update_State);
      if(Result == 0)
      {
         DisplayTimeUpdateState(&Time_Update_State);
      }
      else
         DisplayFunctionError("RTUS_Decode_Time_Update_State", Result);
   }
   else
   {
      Result = RTUS_ERROR_INVALID_PARAMETER;
   }
   return Result;

}

   /* The following function displays the Time Update State information */
static void DisplayTimeUpdateState(RTUS_Time_Update_State_Data_t *Time_Update_State)
{
   if(Time_Update_State)
   {
      PRINTF(" Time Update State  \n");
      PRINTF(" -------------------------\n");
      /* Display CurrentState of Time Update State                      */
      switch(Time_Update_State->CurrentState)
      {
      case csIdle:
         PRINTF("CurrentState :Idle \n");
         break;
      case csUpdatePending:
         PRINTF("CurrentState :Update Pending \n");
         break;
      default:
         break;
      }
      /* Display Result  of Time Update State                           */
      switch(Time_Update_State->Result)
      {
      case reSuccessful:
         PRINTF(" Result   :Successful \n");
         break;
      case reCanceled:
         PRINTF(" Result   :Cancelled \n");
         break;
      case reNoConnectionToReference:
         PRINTF(" Result   :No Connection to Reference \n");
         break;
      case reReferenceRespondedWithError:
         PRINTF(" Result   :Reference Responded with Error \n");
         break;
      case reTimeout:
         PRINTF(" Result   :Timeout \n");
         break;
      case reUpdateNotAttemptedAfterReset:
         PRINTF(" Result   :Update Not Attempted After Reset \n");
         break;
      default:
         break;
      }
   }
}

   /* The following function displays the usage information for         */
   /* SetTimeUpdateState command.  The only parameter is a string to the*/
   /* command name that will be displayed  with the information.        */
static void DisplayTimeUpdateStateUsage(const char* FuncName)
{
   PRINTF("SetTimeUpdateState             [CurrentState] [Result] \n");
   PRINTF("    CurrentState [0: csIdle]          Result [0: reSuccessful]\n");
   PRINTF("    CurrentState [1: csUpdatePending] Result [1: reCanceled]\n");
   PRINTF("                                      Result [2: reNoConnectionToReference]\n");
   PRINTF("                                      Result [3: reReferenceRespondedWithError]\n");
   PRINTF("                                      Result [4: reTimeout]\n");
   PRINTF("                                      Result [5: reUpdateNotAttemptedAfterReset]\n");
}

   /* The following function is responsible for displaying the current  */
   /* Command Options for either Serial Port Client or Serial Port      */
   /* Server.  The input parameter to this function is completely       */
   /* ignored, and only needs to be passed in because all Commands that */
   /* can be entered at the Prompt pass in the parsed information.  This*/
   /* function displays the current Command Options that are available  */
   /* and always returns zero.                                          */
static int DisplayHelp(ParameterList_t *TempParam)
{
   PRINTF("\n");
   PRINTF("******************************************************************\n");
   PRINTF("* Command Options General: Help, Quit, GetLocalAddress,          *\n");
   PRINTF("*                          GetMTU, SetMTU                        *\n");
   PRINTF("* Command Options GAPLE:   SetDiscoverabilityMode,               *\n");
   PRINTF("*                          SetConnectabilityMode,                *\n");
   PRINTF("*                          SetPairabilityMode,                   *\n");
   PRINTF("*                          ChangePairingParameters,              *\n");
   PRINTF("*                          AdvertiseLE, StartScanning,           *\n");
   PRINTF("*                          StopScanning, ConnectLE,              *\n");
   PRINTF("*                          DisconnectLE, CancelConnect, PairLE,  *\n");
   PRINTF("*                          LEPasskeyResponse,                    *\n");
   PRINTF("*                          QueryEncryptionMode, SetPasskey,      *\n");
   PRINTF("*                          DiscoverGAPS, GetLocalName,           *\n");
   PRINTF("*                          SetLocalName, GetRemoteName,          *\n");
   PRINTF("*                          SetLocalAppearance,                   *\n");
   PRINTF("*                          GetLocalAppearance,                   *\n");
   PRINTF("*                          GetRemoteAppearance,                  *\n");
   PRINTF("* Command Options CTS:     RegisterCTS, UnregisterCTS,           *\n");
   PRINTF("*                          DiscoverCTS, ConfigureRemoteCTS,      *\n");
   PRINTF("*                          SetCurrentTime,                       *\n");
   PRINTF("*                          GetCurrentTime,                       *\n");
   PRINTF("*                          NotifyCurrentTime,                    *\n");
   PRINTF("*                          SetLocalTimeInformation,              *\n");
   PRINTF("*                          GetLocalTimeInformation,              *\n");
   PRINTF("*                          SetReferenceTimeInformation,          *\n");
   PRINTF("*                          GetReferenceTimeInformation,          *\n");
   PRINTF("* Command Options NDCS:    RegisterNDCS,                         *\n");
   PRINTF("*                          UnRegisterNDCS,                       *\n");
   PRINTF("*                          DiscoverNDCS,                         *\n");
   PRINTF("*                          GetTimeWithDST,                       *\n");
   PRINTF("*                          SetTimeWithDST,                       *\n");
   PRINTF("* Command Options RTUS:    RegisterRTUS,                         *\n");
   PRINTF("*                          UnRegisterRTUS,                       *\n");
   PRINTF("*                          DiscoverRTUS,                         *\n");
   PRINTF("*                          GetTimeUpdateState,                   *\n");
   PRINTF("*                          SetTimeUpdateState,                   *\n");
   PRINTF("*                          WriteTimeUpdateControlPoint           *\n");
   PRINTF("******************************************************************\n");

   return(0);
}

   /* The following function is responsible for setting the             */
   /* Discoverability Mode of the local device.  This function returns  */
   /* zero on successful execution and a negative value on all errors.  */
static int SetDiscoverabilityMode(ParameterList_t *TempParam)
{
   int                        ret_val;
   GAP_Discoverability_Mode_t DiscoverabilityMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 2))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 1)
            DiscoverabilityMode = dmLimitedDiscoverableMode;
         else
         {
            if(TempParam->Params[0].intParam == 2)
               DiscoverabilityMode = dmGeneralDiscoverableMode;
            else
               DiscoverabilityMode = dmNonDiscoverableMode;
         }

         /* Set the LE Discoveryability Mode.                           */
         LE_Parameters.DiscoverabilityMode = DiscoverabilityMode;

         /* The Mode was changed successfully.                          */
         PRINTF("Discoverability: %s.\n", (DiscoverabilityMode == dmNonDiscoverableMode)?"Non":((DiscoverabilityMode == dmGeneralDiscoverableMode)?"General":"Limited"));

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetDiscoverabilityMode [Mode(0 = Non Discoverable, 1 = Limited Discoverable, 2 = General Discoverable)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the             */
   /* Connectability Mode of the local device.  This function returns   */
   /* zero on successful execution and a negative value on all errors.  */
static int SetConnectabilityMode(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         /* * NOTE * The Connectability Mode in LE is only applicable   */
         /*          when advertising, if a device is not advertising   */
         /*          it is not connectable.                             */
         if(TempParam->Params[0].intParam == 0)
            LE_Parameters.ConnectableMode = lcmNonConnectable;
         else
            LE_Parameters.ConnectableMode = lcmConnectable;

         /* The Mode was changed successfully.                          */
         PRINTF("Connectability Mode: %s.\n", (LE_Parameters.ConnectableMode == lcmNonConnectable)?"Non Connectable":"Connectable");

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetConnectabilityMode [(0 = NonConectable, 1 = Connectable)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the Pairability */
   /* Mode of the local device.  This function returns zero on          */
   /* successful execution and a negative value on all errors.          */
static int SetPairabilityMode(ParameterList_t *TempParam)
{
   int                        Result;
   int                        ret_val;
   char                      *Mode;
   GAP_LE_Pairability_Mode_t  PairabilityMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         PairabilityMode = lpmNonPairableMode;
         Mode            = "lpmNonPairableMode";

         if(TempParam->Params[0].intParam == 1)
         {
            PairabilityMode = lpmPairableMode;
            Mode            = "lpmPairableMode";
         }

         /* Parameters mapped, now set the Pairability Mode.            */
         Result = GAP_LE_Set_Pairability_Mode(BluetoothStackID, PairabilityMode);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            PRINTF("Pairability Mode Changed to %s.\n", Mode);

            /* If Secure Simple Pairing has been enabled, inform the    */
            /* user of the current Secure Simple Pairing parameters.    */
            if(PairabilityMode == lpmPairableMode)
               DisplayIOCapabilities();

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            DisplayFunctionError("GAP_Set_Pairability_Mode", Result);

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetPairabilityMode [Mode (0 = Non Pairable, 1 = Pairable]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      PRINTF("Invalid Stack ID.\n");

      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for changing the Secure     */
   /* Simple Pairing Parameters that are exchanged during the Pairing   */
   /* procedure when Secure Simple Pairing (Security Level 4) is used.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int ChangePairingParameters(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 4))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
            LE_Parameters.IOCapability = licDisplayOnly;
         else
         {
            if(TempParam->Params[0].intParam == 1)
               LE_Parameters.IOCapability = licDisplayYesNo;
            else
            {
               if(TempParam->Params[0].intParam == 2)
                  LE_Parameters.IOCapability = licKeyboardOnly;
               else
               {
                  if(TempParam->Params[0].intParam == 3)
                     LE_Parameters.IOCapability = licNoInputNoOutput;
                  else
                     LE_Parameters.IOCapability = licKeyboardDisplay;
               }
            }
         }

         /* Finally map the Man in the Middle (MITM) Protection value.  */
         LE_Parameters.MITMProtection = (Boolean_t)(TempParam->Params[1].intParam?TRUE:FALSE);

         /* Inform the user of the New I/O Capabilities.                */
         DisplayIOCapabilities();

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         PRINTF("Usage: ChangePairingParameters [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, 3 = No Input/Output, 4 = Keyboard/Display)] [MITM Requirement (0 = No, 1 = Yes)].\n");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for issuing a GAP           */
   /* Authentication Response with a Pass Key value specified via the   */
   /* input parameter.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int LEPassKeyResponse(ParameterList_t *TempParam)
{
   int                              Result;
   int                              ret_val;
   GAP_LE_Authentication_Response_Information_t  GAP_LE_Authentication_Response_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentRemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= GAP_LE_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS))
         {
            /* Parameters appear to be valid, go ahead and populate the */
            /* response structure.                                      */
            GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type  = larPasskey;
            GAP_LE_Authentication_Response_Information.Authentication_Data_Length  = (Byte_t)(sizeof(DWord_t));
            GAP_LE_Authentication_Response_Information.Authentication_Data.Passkey = (DWord_t)(TempParam->Params[0].intParam);

            /* Submit the Authentication Response.                      */
            Result = GAP_LE_Authentication_Response(BluetoothStackID, CurrentRemoteBD_ADDR, &GAP_LE_Authentication_Response_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               PRINTF("Passkey Response Success.");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               DisplayFunctionError("GAP_LE_Authentication_Response", Result);

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            PRINTF("PassKeyResponse [Numeric Passkey(0 - 999999)].\n");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         PRINTF("Pass Key Authentication Response: Authentication not in progress.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the Encryption */
   /* Mode for an LE Connection.  This function returns zero on         */
   /* successful execution and a negative value on all errors.          */
static int LEQueryEncryption(ParameterList_t *TempParam)
{
   int                   ret_val;
   GAP_Encryption_Mode_t GAP_Encryption_Mode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
      {
         /* Query the current Encryption Mode for this Connection.      */
         ret_val = GAP_LE_Query_Encryption_Mode(BluetoothStackID, ConnectionBD_ADDR, &GAP_Encryption_Mode);
         if(!ret_val)
            PRINTF("Current Encryption Mode: %s.\n", (GAP_Encryption_Mode == emEnabled)?"Enabled":"Disabled");
         else
         {
            PRINTF("Error - GAP_LE_Query_Encryption_Mode returned %d.\n", ret_val);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         PRINTF("Not Connected.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the Encryption */
   /* Mode for an LE Connection.  This function returns zero on         */
   /* successful execution and a negative value on all errors.          */
static int LESetPasskey(ParameterList_t *TempParam)
{
   int     ret_val;
   DWord_t Passkey;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this         */
      /* function appear to be at least semi-valid.                     */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && ((TempParam->Params[0].intParam == 0) || (TempParam->Params[0].intParam == 1)))
      {
         if(TempParam->Params[0].intParam == 1)
         {
            /* We are setting the passkey so make sure it is valid.     */
            if(BTPS_StringLength(TempParam->Params[1].strParam) <= GAP_LE_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS)
            {
               Passkey = (DWord_t)(TempParam->Params[1].intParam);

               ret_val = GAP_LE_Set_Fixed_Passkey(BluetoothStackID, &Passkey);
               if(!ret_val)
                  PRINTF("Fixed Passkey set to %06u.\n", (unsigned int)Passkey);
            }
            else
            {
               PRINTF("Error - Invalid Passkey.\n");

               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            /* Un-set the fixed passkey that we previously configured.  */
            ret_val = GAP_LE_Set_Fixed_Passkey(BluetoothStackID, NULL);
            if(!ret_val)
               PRINTF("Fixed Passkey no longer configured.\n");
         }

         /* If GAP_LE_Set_Fixed_Passkey returned an error display this. */
         if((ret_val) && (ret_val != INVALID_PARAMETERS_ERROR))
         {
            PRINTF("Error - GAP_LE_Set_Fixed_Passkey returned %d.\n", ret_val);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         PRINTF("SetPasskey [(0 = UnSet Passkey, 1 = Set Fixed Passkey)] [6 Digit Passkey (optional)].\n");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Address of the local Bluetooth Device.  This function      */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int GetLocalAddress(ParameterList_t *TempParam)
{
   int        Result;
   int        ret_val;
   BD_ADDR_t  BD_ADDR;
   BoardStr_t BoardStr;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR);

      /* Check the return value of the submitted command for success.   */
      if(!Result)
      {
         BD_ADDRToStr(BD_ADDR, BoardStr);

         PRINTF("BD_ADDR of Local Device is: %s.\n", BoardStr);

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* Display a message indicating that an error occurred while   */
         /* attempting to query the Local Device Address.               */
         PRINTF("GAP_Query_Local_BD_ADDR() Failure: %d.\n", Result);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for enabling LE             */
   /* Advertisements.  This function returns zero on successful         */
   /* execution and a negative value on all errors.                     */
static int AdvertiseLE(ParameterList_t *TempParam)
{
   int                                 ret_val;
   int                                 Length;
   GAP_LE_Advertising_Parameters_t     AdvertisingParameters;
   GAP_LE_Connectability_Parameters_t  ConnectabilityParameters;
   union
   {
      Advertising_Data_t               AdvertisingData;
      Scan_Response_Data_t             ScanResponseData;
   } Advertisement_Data_Buffer;

   /* First, check that valid Bluetooth Stack ID exists. And that we are*/
   /* not already connected.                                            */
   if((BluetoothStackID) && (!ConnectionID))
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Determine whether to enable or disable Advertising.         */
         if(TempParam->Params[0].intParam == 0)
         {
            /* Disable Advertising.                                     */
            ret_val = GAP_LE_Advertising_Disable(BluetoothStackID);
            if(!ret_val)
               PRINTF("   GAP_LE_Advertising_Disable success.\n");
            else
            {
               PRINTF("   GAP_LE_Advertising_Disable returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* Enable Advertising.                                      */
            /* Set the Advertising Data.                                */
            BTPS_MemInitialize(&(Advertisement_Data_Buffer.AdvertisingData), 0, sizeof(Advertising_Data_t));

            /* Set the Flags A/D Field (1 byte type and 1 byte Flags.   */
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] = 2;
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_FLAGS;
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = 0;

            /* Configure the flags field based on the Discoverability   */
            /* Mode.                                                    */
            if(LE_Parameters.DiscoverabilityMode == dmGeneralDiscoverableMode)
               Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_GENERAL_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
            else
            {
               if(LE_Parameters.DiscoverabilityMode == dmLimitedDiscoverableMode)
                  Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_LIMITED_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
            }

            if(CTSInstanceID)
            {
               /* Advertise the CTS Server (1 byte type & 2 bytes UUID).*/
               Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[3] = 3;
               Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[4] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_16_BIT_SERVICE_UUID_COMPLETE;
               CTS_ASSIGN_CTS_SERVICE_UUID_16(&(Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[5]));
            }

            /* Write the advertising data to the chip.                  */
            ret_val = GAP_LE_Set_Advertising_Data(BluetoothStackID, (Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] + Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[3] + 2), &(Advertisement_Data_Buffer.AdvertisingData));
            if(!ret_val)
            {
               BTPS_MemInitialize(&(Advertisement_Data_Buffer.ScanResponseData), 0, sizeof(Scan_Response_Data_t));

               /* Set the Scan Response Data.                           */
               Length = BTPS_StringLength(LE_DEMO_DEVICE_NAME);
               if(Length < (ADVERTISING_DATA_MAXIMUM_SIZE - 2))
               {
                  Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE;
               }
               else
               {
                  Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_SHORTENED;
                  Length = (ADVERTISING_DATA_MAXIMUM_SIZE - 2);
               }

               Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[0] = (Byte_t)(1 + Length);
               BTPS_MemCopy(&(Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[2]),LE_DEMO_DEVICE_NAME,Length);

               ret_val = GAP_LE_Set_Scan_Response_Data(BluetoothStackID, (Advertisement_Data_Buffer.ScanResponseData.Scan_Response_Data[0] + 1), &(Advertisement_Data_Buffer.ScanResponseData));
               if(!ret_val)
               {
                  /* Set up the advertising parameters.                 */
                  AdvertisingParameters.Advertising_Channel_Map   = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
                  AdvertisingParameters.Scan_Request_Filter       = fpNoFilter;
                  AdvertisingParameters.Connect_Request_Filter    = fpNoFilter;
                  AdvertisingParameters.Advertising_Interval_Min  = 100;
                  AdvertisingParameters.Advertising_Interval_Max  = 200;

                  /* Configure the Connectability Parameters.           */
                  /* * NOTE * Since we do not ever put ourselves to be  */
                  /*          direct connectable then we will set the   */
                  /*          DirectAddress to all 0s.                  */
                  ConnectabilityParameters.Connectability_Mode   = LE_Parameters.ConnectableMode;
                  ConnectabilityParameters.Own_Address_Type      = latPublic;
                  ConnectabilityParameters.Direct_Address_Type   = latPublic;
                  ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

                  /* Now enable advertising.                            */
                  ret_val = GAP_LE_Advertising_Enable(BluetoothStackID, TRUE, &AdvertisingParameters, &ConnectabilityParameters, GAP_LE_Event_Callback, 0);
                  if(!ret_val)
                  {
                     PRINTF("   GAP_LE_Advertising_Enable success.\n");
                  }
                  else
                  {
                     PRINTF("   GAP_LE_Advertising_Enable returned %d.\n", ret_val);

                     ret_val = FUNCTION_ERROR;
                  }
               }
               else
               {
                  PRINTF("   GAP_LE_Set_Advertising_Data(dtScanResponse) returned %d.\n", ret_val);

                  ret_val = FUNCTION_ERROR;
               }

            }
            else
            {
               PRINTF("   GAP_LE_Set_Advertising_Data(dtAdvertising) returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
      }
      else
      {
         DisplayUsage("AdvertiseLE [(0 = Disable, 1 = Enable)]");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      if(BluetoothStackID)
         PRINTF("Connection already active.\n");

      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for starting an LE scan     */
   /* procedure.  This function returns zero if successful and a        */
   /* negative value if an error occurred.                              */
static int StartScanning(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      if(!ScanInProgress)
      {
         ScanInProgress = TRUE;

         /* Simply start scanning.                                      */
         if(!StartScan(BluetoothStackID))
            ret_val = 0;
         else
            ret_val = FUNCTION_ERROR;
      }
      else
      {
         PRINTF("\nScan already in progress!\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for stopping an LE scan     */
   /* procedure.  This function returns zero if successful and a        */
   /* negative value if an error occurred.                              */
static int StopScanning(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      if(ScanInProgress)
      {
         ScanInProgress = FALSE;

         /* Simply stop scanning.                                       */
         if(!StopScan(BluetoothStackID))
            ret_val = 0;
         else
            ret_val = FUNCTION_ERROR;
      }
      else
      {
         PRINTF("\nScan is not in progress.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for connecting to an LE     */
   /* device.  This function returns zero if successful and a negative  */
   /* value if an error occurred.                                       */
static int ConnectLE(ParameterList_t *TempParam)
{
   int       ret_val;
   BD_ADDR_t BD_ADDR;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      if(ScanInProgress == TRUE)
      {
         StopScan(BluetoothStackID);

         PRINTF("\nScan stopped before making LE Connection\n");
      }

      /* Next, make sure that a valid device address exists.            */
      if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) == (sizeof(BD_ADDR_t)*2)))
      {
         /* Convert the parameter to a Bluetooth Device Address.        */
         StrToBD_ADDR(TempParam->Params[0].strParam, &BD_ADDR);

         if ((TempParam->Params[1].intParam >= 0) && (TempParam->Params[1].intParam <= 1))
         {
            if(!ConnectLEDevice(BluetoothStackID, BD_ADDR, TempParam->Params[1].intParam ? latRandom : latPublic, FALSE))
               ret_val = 0;
            else
               ret_val = FUNCTION_ERROR;
         }
         else
         {
               PRINTF("Usage: ConnectLE [BD_ADDR] [ADDR Type (0 = Public, 1 = Random)].\n");
               ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         PRINTF("Usage: ConnectLE [BD_ADDR] [ADDR Type (0 = Public, 1 = Random)].\n");

         /* Flag that an error occurred while submitting the command.   */
         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for disconnecting to an LE  */
   /* device.  This function returns zero if successful and a negative  */
   /* value if an error occurred.                                       */
static int DisconnectLE(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, check to make sure we are currently connected.           */
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
      {
         if(!DisconnectLEDevice(BluetoothStackID, ConnectionBD_ADDR))
            ret_val = 0;
         else
            ret_val = FUNCTION_ERROR;
      }
      else
      {
         PRINTF("Device is not connected.\n");

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for canceling a LE          */
   /* connection establishment process.This function returns zero if    */
   /* successful and a negative value if an error occurred.             */
static int CancelConnect(ParameterList_t *TempParam)
{
   int ret_val;

      /* First, check that valid Bluetooth Stack ID exists.             */
   if(BluetoothStackID)
   {
      /* Next, check to make sure we are currently connected.           */
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
      {
         if(!GAP_LE_Cancel_Create_Connection(BluetoothStackID))
         {
            /* Notify that the current Connection BD_ADDR is not valid  */
            /* any more                                                 */
            ASSIGN_BD_ADDR(ConnectionBD_ADDR, 0, 0, 0, 0, 0, 0);

            ret_val = 0;
         }
         else
            ret_val = FUNCTION_ERROR;
      }
      else
      {
         PRINTF("Device is not connected.\n");

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is provided to allow a mechanism of        */
   /* Pairing (or requesting security if a slave) to the connected      */
   /* device.                                                           */
static int PairLE(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, check to make sure we are currently connected.           */
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
      {
         if(!SendPairingRequest(ConnectionBD_ADDR, LocalDeviceIsMaster))
            ret_val = 0;
         else
            ret_val = FUNCTION_ERROR;
      }
      else
      {
         PRINTF("Device is not connected.\n");

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for performing a GAP Service*/
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverGAPS(ParameterList_t *TempParam)
{
   int                            ret_val;
   GATT_UUID_t                    UUID[1];
   DeviceInfo_t                  *DeviceInfo;
   GATT_Attribute_Handle_Group_t  DiscoveryHandleRange;

   /* Verify that there is a connection that is established.            */
   if(ConnectionID)
   {
      /* Get the device info for the connection device.                 */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
      {
         /* Verify that no service discovery is outstanding for this    */
         /* device.                                                     */
         if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING))
         {
            /* Configure the filter so that only the GAP Service is     */
            /* discovered.                                              */
            UUID[0].UUID_Type = guUUID_16;
            GAP_ASSIGN_GAP_SERVICE_UUID_16(UUID[0].UUID.UUID_16);

            BTPS_MemInitialize(&DiscoveryHandleRange, 0, sizeof(DiscoveryHandleRange));

            /* Start the service discovery process.                     */
            if((TempParam->NumberofParameters >= 2) && (TempParam->Params[0].intParam) && (TempParam->Params[1].intParam) && (TempParam->Params[0].intParam <= TempParam->Params[1].intParam))
            {
               DiscoveryHandleRange.Starting_Handle = TempParam->Params[0].intParam;
               DiscoveryHandleRange.Ending_Handle   = TempParam->Params[1].intParam;

               ret_val = GATT_Start_Service_Discovery_Handle_Range(BluetoothStackID, ConnectionID, &DiscoveryHandleRange, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, sdGAPS);
            }
            else
               ret_val = GATT_Start_Service_Discovery(BluetoothStackID, ConnectionID, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, sdGAPS);

            if(!ret_val)
            {
               /* Display success message.                              */
               if(DiscoveryHandleRange.Starting_Handle == 0)
                  PRINTF("GATT_Service_Discovery_Start() success.\n");
               else
                  PRINTF("GATT_Start_Service_Discovery_Handle_Range() success.\n");

               /* Flag that a Service Discovery Operation is            */
               /* outstanding.                                          */
               DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
            }
            else
            {
               /* An error occur so just clean-up.                      */
               PRINTF("Error - GATT_Service_Discovery_Start returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            PRINTF("Service Discovery Operation Outstanding for Device.\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         PRINTF("No Device Info.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("No Connection Established\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the current     */
   /* Local Device Name.  This function will return zero on successful  */
   /* execution and a negative value on errors.                         */
static int ReadLocalName(ParameterList_t *TempParam)
{
   int  ret_val;
   char NameBuffer[BTPS_CONFIGURATION_GAPS_MAXIMUM_SUPPORTED_DEVICE_NAME+1];

   /* Verify that the GAP Service is registered.                        */
   if(GAPSInstanceID)
   {
      /* Initialize the Name Buffer to all zeros.                       */
      BTPS_MemInitialize(NameBuffer, 0, sizeof(NameBuffer));

      /* Query the Local Name.                                          */
      ret_val = GAPS_Query_Device_Name(BluetoothStackID, GAPSInstanceID, NameBuffer);
      if(!ret_val)
         PRINTF("Device Name: %s.\n", NameBuffer);
      else
      {
         PRINTF("Error - GAPS_Query_Device_Name returned %d.\n", ret_val);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("GAP Service not registered.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the current     */
   /* Local Device Name.  This function will return zero on successful  */
   /* execution and a negative value on errors.                         */
static int SetLocalName(ParameterList_t *TempParam)
{
   int  ret_val;

   /* Verify that the input parameters are semi-valid.                  */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= BTPS_CONFIGURATION_GAPS_MAXIMUM_SUPPORTED_DEVICE_NAME))
   {
      /* Verify that the GAP Service is registered.                     */
      if(GAPSInstanceID)
      {
         /* Query the Local Name.                                       */
         ret_val = GAPS_Set_Device_Name(BluetoothStackID, GAPSInstanceID, TempParam->Params[0].strParam);
         if(!ret_val)
            PRINTF("GAPS_Set_Device_Name success.\n");
         else
         {
            PRINTF("Error - GAPS_Query_Device_Name returned %d.\n", ret_val);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         PRINTF("GAP Service not registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Usage: SetLocalName [NameString].\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Device Name */
   /* for the currently connected remote device.  This function will    */
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int ReadRemoteName(ParameterList_t *TempParam)
{
   int           ret_val;
   DeviceInfo_t *DeviceInfo;

   /* Verify that there is a connection that is established.            */
   if(ConnectionID)
   {
      /* Get the device info for the connection device.                 */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
      {
         /* Verify that we discovered the Device Name Handle.           */
         if(DeviceInfo->GAPSClientInfo.DeviceNameHandle)
         {
            /* Attempt to read the remote device name.                  */
            ret_val = GATT_Read_Value_Request(BluetoothStackID, ConnectionID, DeviceInfo->GAPSClientInfo.DeviceNameHandle, GATT_ClientEventCallback_GAPS, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceNameHandle);
            if(ret_val > 0)
            {
               PRINTF("Attempting to read Remote Device Name.\n");

               ret_val = 0;
            }
            else
            {
               PRINTF("Error - GATT_Read_Value_Request returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            PRINTF("GAP Service Device Name Handle not discovered.\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         PRINTF("No Device Info.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("No Connection Established\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Local Device*/
   /* Appearance value.  This function will return zero on successful   */
   /* execution and a negative value on errors.                         */
static int ReadLocalAppearance(ParameterList_t *TempParam)
{
   int     ret_val;
   char   *AppearanceString;
   Word_t  Appearance;

   /* Verify that the GAP Service is registered.                        */
   if(GAPSInstanceID)
   {
      /* Query the Local Name.                                          */
      ret_val = GAPS_Query_Device_Appearance(BluetoothStackID, GAPSInstanceID, &Appearance);
      if(!ret_val)
      {
         /* Map the Appearance to a String.                             */
         if(AppearanceToString(Appearance, &AppearanceString))
            PRINTF("Device Appearance: %s(%u).\n", AppearanceString, Appearance);
         else
            PRINTF("Device Appearance: Unknown(%u).\n", Appearance);
      }
      else
      {
         PRINTF("Error - GAPS_Query_Device_Appearance returned %d.\n", ret_val);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("GAP Service not registered.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the Local Device*/
   /* Appearance value.  This function will return zero on successful   */
   /* execution and a negative value on errors.                         */
static int SetLocalAppearance(ParameterList_t *TempParam)
{
   int    ret_val;
   Word_t Appearance;

   /* Verify that the input parameters are semi-valid.                  */
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam < NUMBER_OF_APPEARANCE_MAPPINGS))
   {
      /* Verify that the GAP Service is registered.                     */
      if(GAPSInstanceID)
      {
         /* Map the Appearance Index to the GAP Appearance Value.       */
         if(AppearanceIndexToAppearance(TempParam->Params[0].intParam, &Appearance))
         {
            /* Set the Local Appearance.                                */
            ret_val = GAPS_Set_Device_Appearance(BluetoothStackID, GAPSInstanceID, Appearance);
            if(!ret_val)
               PRINTF("GAPS_Set_Device_Appearance success.\n");
            else
            {
               PRINTF("Error - GAPS_Set_Device_Appearance returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            PRINTF("Invalid Appearance Index.\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         PRINTF("GAP Service not registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Usage: SetLocalName [Index].\n");
      PRINTF("Where Index = \n");
      DumpAppearanceMappings();

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Device Name */
   /* for the currently connected remote device.  This function will    */
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int ReadRemoteAppearance(ParameterList_t *TempParam)
{
   int           ret_val;
   DeviceInfo_t *DeviceInfo;

   /* Verify that there is a connection that is established.            */
   if(ConnectionID)
   {
      /* Get the device info for the connection device.                 */
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
      {
         /* Verify that we discovered the Device Name Handle.           */
         if(DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle)
         {
            /* Attempt to read the remote device name.                  */
            ret_val = GATT_Read_Value_Request(BluetoothStackID, ConnectionID, DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle, GATT_ClientEventCallback_GAPS, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle);
            if(ret_val > 0)
            {
               PRINTF("Attempting to read Remote Device Appearance.\n");

               ret_val = 0;
            }
            else
            {
               PRINTF("Error - GATT_Read_Value_Request returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            PRINTF("GAP Service Device Appearance Handle not discovered.\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         PRINTF("No Device Info.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("No Connection Established\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for querying the GATT MTU.  */
   /* This function returns zero if successful and a negative value if  */
   /* an error occurred.                                                */
static int GetGATTMTU(ParameterList_t *TempParam)
{
   int    ret_val;
   Word_t MTU;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Simply query the Maximum Supported MTU from the GATT layer.    */
      if((ret_val = GATT_Query_Maximum_Supported_MTU(BluetoothStackID, &MTU)) == 0)
         PRINTF("Maximum GATT MTU: %u.\n", (unsigned int)MTU);
      else
      {
         PRINTF("Error - GATT_Query_Maximum_Supported_MTU() %d.\n", ret_val);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the GATT MTU.   */
   /* This function returns zero if successful and a negative value if  */
   /* an error occurred.                                                */
static int SetGATTMTU(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Bluetooth Stack is initialized, go ahead and check to see if   */
      /* the parameters are valid.                                      */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= ATT_PROTOCOL_MTU_MINIMUM_LE) && (TempParam->Params[0].intParam <= ATT_PROTOCOL_MTU_MAXIMUM))
      {
         /* Simply set the Maximum Supported MTU to the GATT layer.     */
         if((ret_val = GATT_Change_Maximum_Supported_MTU(BluetoothStackID, (Word_t)TempParam->Params[0].intParam)) == 0)
            PRINTF("GATT_Change_Maximum_Supported_MTU() success, new GATT MTU: %u.\n", (unsigned int)TempParam->Params[0].intParam);
         else
         {
            PRINTF("Error - GATT_Change_Maximum_Supported_MTU() %d.\n", ret_val);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         PRINTF("Usage: SetMTU [GATT MTU (>= %u, <= %u)].\n", (unsigned int)ATT_PROTOCOL_MTU_MINIMUM_LE, (unsigned int)ATT_PROTOCOL_MTU_MAXIMUM);

         /* Flag that an error occurred while submitting the command.   */
         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for registering a TPS       */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int RegisterCTS(ParameterList_t *TempParam)
{
   int                     ret_val;

   /* Verify that there is no active connection.                        */
   if(!ConnectionID)
   {
      /* Verify that the Service is not already registered.             */
      if(!CTSInstanceID)
      {
         /* Register the CTP Service with GATT.  Enable all features    */
         /* (CTS_FLAGS_...).                                            */
         ret_val = CTS_Initialize_Service_Flags(BluetoothStackID, CTS_DEFAULT_FEATURES_BIT_MASK, CTS_EventCallback, 0, &CTSInstanceID, NULL);
         if((ret_val > 0) && (CTSInstanceID > 0))
         {
            /* Display success message.                                 */
            PRINTF("Successfully registered CTS Service.\n");

            /* Save the ServiceID of the registered service.            */
            CTSInstanceID = (unsigned int)ret_val;

            /* Initialize internal CTS variables                        */
            CTS_Set_Local_Time_Information(BluetoothStackID,CTSInstanceID, &LocalTimeInformation);

            /* Return success to the caller.                            */
            ret_val        = 0;
         }
      }
      else
      {
         PRINTF("CTP Service already registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Connection current active.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for unregistering a CTP     */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int UnregisterCTS(ParameterList_t *TempParam)
{
   int ret_val = FUNCTION_ERROR;

   /* Verify that a service is registered.                              */
   if(CTSInstanceID)
   {
      /* If there is a connected device, then first disconnect it.      */
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
         DisconnectLEDevice(BluetoothStackID, ConnectionBD_ADDR);

      /* Unregister the CTP Service with GATT.                          */
      ret_val = CTS_Cleanup_Service(BluetoothStackID, CTSInstanceID);
      if(ret_val == 0)
      {
         /* Display success message.                                    */
         PRINTF("Successfully unregistered CTS Service.\n");

         /* Save the ServiceID of the registered service.               */
         CTSInstanceID = 0;
      }
      else
         DisplayFunctionError("CTS_Cleanup_Service", ret_val);
   }
   else
      PRINTF("CTS Service not registered.\n");

   return(ret_val);
}

   /* The following function is responsible for performing a CTP        */
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverCTS(ParameterList_t *TempParam)
{
   DeviceInfo_t *DeviceInfo;
   GATT_UUID_t   UUID[1];
   int           ret_val = FUNCTION_ERROR;

   /* Verify that we are not configured as a server                     */
   if(!CTSInstanceID)
   {
      /* Verify that there is a connection that is established.         */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that no service discovery is outstanding for this */
            /* device.                                                  */
            if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING))
            {
               /* Configure the filter so that only the CTP Service is  */
               /* discovered.                                           */
               UUID[0].UUID_Type = guUUID_16;
               CTS_ASSIGN_CTS_SERVICE_UUID_16(&(UUID[0].UUID.UUID_16));

               /* Start the service discovery process.                  */
               ret_val = GATT_Start_Service_Discovery(BluetoothStackID, ConnectionID, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, sdCTS);
               if(!ret_val)
               {
                  /* Display success message.                           */
                  PRINTF("GATT_Start_Service_Discovery success.\n");

                  /* Flag that a Service Discovery Operation is         */
                  /* outstanding.                                       */
                  DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
               }
               else
                  DisplayFunctionError("GATT_Start_Service_Discovery", ret_val);
            }
            else
               PRINTF("Service Discovery Operation Outstanding for Device.\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("No Connection Established\n");
   }
   else
      PRINTF("Cannot discover CTP Services when registered as a service.\n");

   return(ret_val);
}

   /* The following function is responsible for configure a CTP Service */
   /* on a remote device.  This function will return zero on successful */
   /* execution and a negative value on errors.                         */
static int ConfigureRemoteCTS(ParameterList_t *TempParam)
{
   DeviceInfo_t *DeviceInfo;
   int           ret_val = FUNCTION_ERROR;

   /* Verify that the input parameters are semi-valid.                  */
   if(TempParam && (TempParam->NumberofParameters > 0))
   {
      /* Verify that we are not configured as a server                  */
      if(!CTSInstanceID)
      {
         /* Verify that there is a connection that is established.      */
         if(ConnectionID)
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
            {
               /* Determine if service discovery has been performed on  */
               /* this device                                           */
               if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_CTS_SERVICE_DISCOVERY_COMPLETE)
               {
                  ret_val = 0;

                  PRINTF("Attempting to configure CCCDs...\n");

                  /* Determine if Current Time CC is                    */
                  /* supported (mandatory).                             */
                  if(DeviceInfo->ClientInfo.Current_Time_Client_Configuration)
                  {
                     ret_val = EnableDisableNotificationsIndications(DeviceInfo->ClientInfo.Current_Time_Client_Configuration, (TempParam->Params[0].intParam ? GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE : 0), GATT_ClientEventCallback_CTS);
                  }
                  else
                     PRINTF("   Error - Current Time CC not found on this device.\n");

                  /* Check for CC Configuration success                 */
                  if(ret_val > 0)
                  {
                     PRINTF("CCCD Configuration Success.\n");

                     ret_val = 0;
                  }
                  else
                  {
                     /* CC Configuration failed, check to see if it was */
                     /* from a call to                                  */
                     /* EnableDisableNotificationsIndications           */
                     if(ret_val < 0)
                     {
                        DisplayFunctionError("EnableDisableNotificationsIndications", ret_val);
                     }

                     ret_val = FUNCTION_ERROR;
                  }
               }
               else
                  PRINTF("Service discovery has not been performed on this device.\n");
            }
            else
               PRINTF("No Device Info.\n");
         }
         else
            PRINTF("No Connection Established.\n");
      }
      else
         PRINTF("Cannot configure remote CTP Services when registered as a service.\n");
   }
   else
   {
      PRINTF("Usage: ConfigureRemoteCTS [Notify Current Time (0 = disable, 1 = enable)]\n");
   }

   return(ret_val);
}

   /* The following function is responsible for writing the current     */
   /* time.  It can be executed only by a server.  This function will   */
   /* return zero on successful execution and a negative value on errors*/
static int SetCurrentTime(ParameterList_t *TempParam)
{
   int                      ret_val      = FUNCTION_ERROR;
   int                      BufferLength = CTS_CURRENT_TIME_SIZE;
   Byte_t                   Buffer[CTS_CURRENT_TIME_SIZE];
   DeviceInfo_t            *DeviceInfo;
   CTS_Current_Time_Data_t  LocalCurrentTime;

   if((TempParam) && (TempParam->NumberofParameters == 9))
   {
      if(CTSInstanceID)
      {
         FormatCurrentTime(TempParam->Params, &CurrentTime);
         PRINTF("Current Time successfully set\n");
         ret_val = 0;
      }
      else
      {
         if(ConnectionID)
         {
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
            {
               if(!FormatCurrentTime(TempParam->Params, &LocalCurrentTime))
               {
                  if(!(ret_val = CTS_Format_Current_Time(&LocalCurrentTime, BufferLength, Buffer)))
                  {
                     if(DeviceInfo->ClientInfo.Current_Time)
                     {
                        if(GATT_Write_Request(BluetoothStackID, ConnectionID, DeviceInfo->ClientInfo.Current_Time, BufferLength, Buffer, GATT_ClientEventCallback_CTS, 1) )
                        {
                           PRINTF("Gatt write request for current time sent.\n");

                           /* Simply return success to the caller.      */
                           ret_val = 0;
                        }
                     }
                     else
                        PRINTF("Client has not registered for Current Time.\n");
                  }
               }
               else
               {
                  PRINTF("Invalid Local Time Information.\n");
                  DisplayLocalTimeUsage(__FUNCTION__);
               }
            }
            else
               PRINTF("No Device Info.\n");
         }
         else
            PRINTF("CTP server not registered\n");
      }
   }
   else
   {
      DisplayCurrentTimeUsage(__FUNCTION__);
   }

   return(ret_val);
}


   /* The following function is responsible for reading the current     */
   /* time. It can be executed by a server or as a client with an open  */
   /* connection to a remote server if client write access to the       */
   /* current time is supported.  If executed as a client, a GATT read  */
   /* request will be generated, and the results will be returned as a  */
   /* response in the GATT client event callback.  This function will   */
   /* return zero on successful execution and a negative value on errors*/
static int GetCurrentTime(ParameterList_t *TempParam)
{
   int           ret_val = FUNCTION_ERROR;
   DeviceInfo_t *DeviceInfo;

   /* First check for a registered CTP Server                           */
   if(CTSInstanceID)
   {
      DisplayCurrentTime(&CurrentTime);
      ret_val = 0;
   }
   else
   {
      /* Check to see if we are configured as a client with an active   */
      /* connection                                                     */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that the client has received a valid Current      */
            /* Time Attribute Handle.                                   */
            if(DeviceInfo->ClientInfo.Current_Time != 0)
            {
               /* Finally, submit a read request to the server          */
               if((ret_val = GATT_Read_Value_Request(BluetoothStackID, ConnectionID, DeviceInfo->ClientInfo.Current_Time, GATT_ClientEventCallback_CTS, DeviceInfo->ClientInfo.Current_Time)) > 0)
               {
                  PRINTF("Get Current Time Request sent, Transaction ID = %u", ret_val);

                  ret_val = 0;
               }
               else
                  DisplayFunctionError("GATT_Read_Value_Request", ret_val);
            }
            else
               PRINTF("Error - Current Time attribute handle is invalid!\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("Either a CTP server must be registered or a CTP client must be connected.\n");
   }

   return(ret_val);
}

   /* The following function is responsible for performing a Current    */
   /* Time notification to a connected remote device. This function will*/
   /* return zero on successful execution and a negative value on errors*/
static int NotifyCurrentTime(ParameterList_t *TempParam)
{
   int                      ret_val = FUNCTION_ERROR;
   DeviceInfo_t            *DeviceInfo;
   CTS_Current_Time_Data_t  Current_time;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      if((TempParam) && (TempParam->NumberofParameters >= 9))
      {
         /* Verify that we have an open server and a connection.        */
         if(CTSInstanceID)
         {
            if((ConnectionID) && (!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR)))
            {
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
               {
                  if(DeviceInfo->ServerInfo.Current_Time_Client_Configuration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                  {
                     FormatCurrentTime(TempParam->Params,&Current_time);

                     ret_val = CTS_Notify_Current_Time(BluetoothStackID, CTSInstanceID, ConnectionID,&Current_time);
                     if(ret_val== 0)
                     {
                         PRINTF("CTS_Notify_Current_Time success.\n");
                     }
                     else
                     {
                        DisplayFunctionError("CTS_Notify_Current_Time", ret_val);
                     }
                  }
                  else
                  {
                     PRINTF("Client has not registered for current time notifications.\n");
                     ret_val = 0;
                  }
               }
               else
               {
                  PRINTF("Error - Unknown Client.\n");
               }
            }
            else
               PRINTF("Connection not established.\n");
         }
         else
         {
            if(ConnectionID)
               PRINTF("Error - Only a server can notify.\n");
            else
               PRINTF("Error - CTP server not registered\n");
         }
      }
      else
      {
         DisplayCurrentTimeUsage(__FUNCTION__);
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for writing the local time  */
   /* information. It can be executed only by a server. This function   */
   /* will return zero on successful execution and a negative value on  */
   /* errors                                                            */
static int SetLocalTimeInformation(ParameterList_t *TempParam)
{
   int                                ret_val = FUNCTION_ERROR;
   int                                BufferLength = CTS_LOCAL_TIME_INFORMATION_SIZE;
   Byte_t                             Buffer[CTS_LOCAL_TIME_INFORMATION_SIZE];
   DeviceInfo_t                      *DeviceInfo;
   CTS_Local_Time_Information_Data_t  LocalTimeInfo;

   if((TempParam) && (TempParam->NumberofParameters == 2))
   {
      if(CTSInstanceID)
      {
         ret_val = FormatLocalTimeInformation(TempParam->Params,&LocalTimeInfo);
         if(ret_val == 0)
         {
            ret_val = CTS_Set_Local_Time_Information(BluetoothStackID, CTSInstanceID, &LocalTimeInfo);
            if(ret_val == 0)
            {
               PRINTF("Local Time Information successfully set\n");
            }
            else
               DisplayFunctionError("CTS_Set_Local_Time_Information", ret_val);
         }
         else
         {
            PRINTF("Invalid Local Time Information.\n");
            DisplayLocalTimeUsage(__FUNCTION__);
         }
      }
      else
      {
         if(ConnectionID)
         {
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
            {
               if(!FormatLocalTimeInformation(TempParam->Params, &LocalTimeInfo))
               {
                  if(!(ret_val = CTS_Format_Local_Time_Information(&LocalTimeInfo, BufferLength, Buffer)))
                  {
                     if(DeviceInfo->ClientInfo.Local_Time_Information)
                     {
                        if((ret_val = GATT_Write_Request(BluetoothStackID, ConnectionID, DeviceInfo->ClientInfo.Local_Time_Information, BufferLength, Buffer, GATT_ClientEventCallback_CTS, 1)))
                        {
                           PRINTF("GATT write request for local time information sent. (Transaction ID: %d)\n", ret_val);

                           /* Simply return success to the caller.      */
                           ret_val = 0;
                        }
                        else
                           PRINTF("GATT write request for local time information not sent.\n");
                     }
                     else
                        PRINTF("Client has not registered for local time information.\n");
                  }
               }
               else
               {
                  PRINTF("Invalid Local Time Information.\n");
                  DisplayLocalTimeUsage(__FUNCTION__);
               }
            }
            else
               PRINTF("No Device info.\n");
         }
         else
            PRINTF("CTP server not registered\n");
      }
   }
   else
   {
      DisplayLocalTimeUsage(__FUNCTION__);
   }

   return(ret_val);
}

   /* The following function is responsible for reading the local time  */
   /* information. It can be executed by a server or as a client with   */
   /* an open connection to a remote server if client write access to   */
   /* the local time information is supported. If executed as a client, */
   /* a GATT read request will be generated, and the results will be    */
   /* returned as a response in the GATT client event callback.  This   */
   /* function will return zero on successful execution and a negative  */
   /* value on errors                                                   */
static int GetLocalTimeInformation(ParameterList_t *TempParam)
{
   int                                ret_val = FUNCTION_ERROR;
   DeviceInfo_t                      *DeviceInfo;
   CTS_Local_Time_Information_Data_t  LocalTimeInfo;

   /* First check for a registered CTP Server                           */
   if(CTSInstanceID)
   {
      ret_val = CTS_Query_Local_Time_Information(BluetoothStackID, CTSInstanceID, &LocalTimeInfo);
      if(ret_val == 0)
      {
         DisplayLocalTime(&LocalTimeInfo);
      }
      else
         DisplayFunctionError("CTS_Query_Local_Time_Information", ret_val);
   }
   else
   {
      /* Check to see if we are configured as a client with an active   */
      /* connection                                                     */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that the client has received a valid Local        */
            /* Time Information Attribute Handle.                       */
            if(DeviceInfo->ClientInfo.Local_Time_Information != 0)
            {
               /* Finally, submit a read request to the server          */
               if((ret_val = GATT_Read_Value_Request(BluetoothStackID, ConnectionID, DeviceInfo->ClientInfo.Local_Time_Information, GATT_ClientEventCallback_CTS, DeviceInfo->ClientInfo.Local_Time_Information)) > 0)
               {
                  PRINTF("Get Local Time Information Request sent, Transaction ID = %u", ret_val);

                  ret_val = 0;
               }
               else
                  DisplayFunctionError("GATT_Read_Value_Request", ret_val);
            }
            else
               PRINTF("Error - Local Time Information attribute handle is invalid!\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("Either a CTP server must be registered or a CTP client must be connected.\n");
   }

   return(ret_val);
}

   /* The following function is responsible for writing the reference   */
   /* time information. It can be executed only by a server. This       */
   /* function will return zero on successful execution and a negative  */
   /* value on errors.                                                  */
static int SetReferenceTimeInformation(ParameterList_t *TempParam)
{
   int ret_val = FUNCTION_ERROR;

   if((TempParam) && (TempParam->NumberofParameters == 4))
   {
      if(CTSInstanceID)
      {
         ret_val = FormatReferenceTimeInformation(TempParam->Params,&ReferenceTimeInformation);
         if(ret_val == 0)
         {
            PRINTF("Reference Time Information successfully set.\n");
         }
         else
         {
            PRINTF("Invalid Reference Time Information.\n");

            DisplayReferenceTimeUsage(__FUNCTION__);
         }
      }
      else
      {
         if(ConnectionID)
            PRINTF("Cannot write Reference Time Information as a client.\n");
         else
            PRINTF("CTP server not registered\n");
      }
   }
   else
   {
      DisplayReferenceTimeUsage(__FUNCTION__);
   }

   return(ret_val);
}

   /* The following function is responsible for reading the reference   */
   /* time information. It can be executed by a server or as a client   */
   /* with an open connection to a remote server if client write access */
   /* to the reference time information is supported. If executed as a  */
   /* client, a GATT read request will be generated, and the results    */
   /* will be returned as a response in the GATT client event callback. */
   /* This function will return zero on successful execution and a      */
   /* negative value on errors                                          */
static int GetReferenceTimeInformation(ParameterList_t *TempParam)
{
   int           ret_val = FUNCTION_ERROR;
   DeviceInfo_t *DeviceInfo;

   /* First check for a registered CTP Server                           */
   if(CTSInstanceID)
   {
      DisplayReferenceTime(&ReferenceTimeInformation);

      ret_val = 0;
   }
   else
   {
      /* Check to see if we are configured as a client with an active   */
      /* connection                                                     */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that the client has received a valid reference    */
            /* Time Information Attribute Handle.                       */
            if(DeviceInfo->ClientInfo.Reference_Time_Information != 0)
            {
               /* Finally, submit a read request to the server          */
               if((ret_val = GATT_Read_Value_Request(BluetoothStackID, ConnectionID, DeviceInfo->ClientInfo.Reference_Time_Information, GATT_ClientEventCallback_CTS, DeviceInfo->ClientInfo.Reference_Time_Information)) > 0)
               {
                  PRINTF("Get Reference Time Information Request sent, Transaction ID = %u", ret_val);

                  ret_val = 0;
               }
               else
                  DisplayFunctionError("GATT_Read_Value_Request", ret_val);
            }
            else
               PRINTF("Error - Reference Time Information attribute handle is invalid!\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("Either a CTP server must be registered or a CTP client must be connected.\n");
   }

   return(ret_val);
}

   /*********************************************************************/
   /*              NDCS Command Functions                               */
   /*********************************************************************/

   /* The following function is responsible for registering a TP        */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int RegisterNDCS(ParameterList_t *TempParam)
{
   int ret_val;

   /* Verify that there is no active connection.                        */
   if(!ConnectionID)
   {
      /* Verify that the Service is not already registered.             */
      if(!NDCSInstanceID)
      {
         /* Register the NDCS Service with GATT.                        */
         ret_val = NDCS_Initialize_Service(BluetoothStackID ,NDCS_EventCallback, 0, &NDCSInstanceID);
         if((ret_val > 0) && (NDCSInstanceID > 0))
         {
            /* Display success message.                                 */
            PRINTF("Successfully registered NDCS Service.\n");

            /* Save the ServiceID of the registered service.            */
            NDCSInstanceID = (unsigned int)ret_val;

            /* Return success to the caller.                            */
            ret_val = 0;
         }
      }
      else
      {
         PRINTF("NDCS Service already registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Connection current active.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for unregistering a NDCS    */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int UnregisterNDCS(ParameterList_t *TempParam)
{
   int ret_val = FUNCTION_ERROR;

   /* Verify that a service is registered.                              */
   if(NDCSInstanceID)
   {
      /* If there is a connected device, then first disconnect it.      */
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
         DisconnectLEDevice(BluetoothStackID, ConnectionBD_ADDR);

      /* Unregister the NDCS Service with GATT.                         */
      ret_val = NDCS_Cleanup_Service(BluetoothStackID, NDCSInstanceID);
      if(ret_val == 0)
      {
         /* Display success message.                                    */
         PRINTF("Successfully unregistered NDCS Service.\n");

         /* Save the ServiceID of the registered service.               */
         NDCSInstanceID = 0;
      }
      else
         DisplayFunctionError("NDCS_Cleanup_Service", ret_val);
   }
   else
      PRINTF("NDCS Service not registered.\n");

   return(ret_val);
}

   /* The following function is responsible for performing a NDCS       */
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverNDCS(ParameterList_t *TempParam)
{
   DeviceInfo_t *DeviceInfo;
   GATT_UUID_t   UUID[1];
   int           ret_val = FUNCTION_ERROR;

   /* Verify that we are not configured as a server                     */
   if(!NDCSInstanceID)
   {
      /* Verify that there is a connection that is established.         */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that no service discovery is outstanding for this */
            /* device.                                                  */
            if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING))
            {
               /* Configure the filter so that only the NDCS Service is */
               /* discovered.                                           */
               UUID[0].UUID_Type = guUUID_16;
               NDCS_ASSIGN_NDCS_SERVICE_UUID_16((&UUID[0].UUID.UUID_16));

               /* Start the service discovery process.                  */
               ret_val = GATT_Start_Service_Discovery(BluetoothStackID, ConnectionID, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, sdNDCS);
               if(!ret_val)
               {
                  /* Display success message.                           */
                  PRINTF("GATT_Start_Service_Discovery success.\n");

                  /* Flag that a Service Discovery Operation is         */
                  /* outstanding.                                       */
                  DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
               }
               else
                  DisplayFunctionError("GATT_Start_Service_Discovery", ret_val);
            }
            else
               PRINTF("Service Discovery Operation Outstanding for Device.\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("No Connection Established\n");
   }
   else
      PRINTF("Cannot discover NDCS Services when registered as a service.\n");

   return(ret_val);
}

   /* The following function is responsible for setting the Time with   */
   /* DST Offset.  This function will return zero on successful         */
   /* execution and a negative value on errors.                         */
static int SetTimeWithDST(ParameterList_t *TempParam)
{
   int ret_val = FUNCTION_ERROR;

   if((TempParam) && (TempParam->NumberofParameters == 7))
   {
      if(NDCSInstanceID)
      {
         TimeWithDst.Date_Time.Day     = (Byte_t)TempParam->Params[0].intParam;
         TimeWithDst.Date_Time.Month   = (Byte_t)TempParam->Params[1].intParam;
         TimeWithDst.Date_Time.Year    = (Word_t)TempParam->Params[2].intParam;
         TimeWithDst.Date_Time.Hours   = (Byte_t)TempParam->Params[3].intParam;
         TimeWithDst.Date_Time.Minutes = (Byte_t)TempParam->Params[4].intParam;
         TimeWithDst.Date_Time.Seconds = (Byte_t)TempParam->Params[5].intParam;
         TimeWithDst.Dst_Offset        = (Byte_t)TempParam->Params[6].intParam;

         PRINTF("Time With Dst successfully set\n");

         ret_val = 0;
      }
      else
      {
         if(ConnectionID)
            PRINTF("Cannot write Time with Offset as a client.\n");
         else
            PRINTF("NDCS server not registered\n");
      }
   }
   else
   {
      PRINTF("Usage SetTimeWithDST  [Day] [Month] [Year] [Hours] [Minutes] [Seconds] [DST_Offset :0-255]\n");
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Time        */
   /* withDST offset. It can be executed by a server or as a client     */
   /* with an open connection to a remote server if client write access */
   /* to timeWithDst is supported. f executed as a client, a GATT read  */
   /* request will be generated, and the results will be returned as a  */
   /* response in the GATT client event callback.  This function will   */
   /* return zero on successful execution and a negative value on errors*/
static int GetTimeWithDST(ParameterList_t *TempParam)
{
   DeviceInfo_t *DeviceInfo;
   int          ret_val = FUNCTION_ERROR;

   /* First check for a registered NDCS Server                          */
   if(NDCSInstanceID)
   {
         DisplayTimeWithOffset(&TimeWithDst);

         ret_val = 0;
   }
   else
   {
      /* Check to see if we are configured as a client with an active   */
      /* connection                                                     */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that the client has received a valid Time with    */
            /* Dst Attribute Handle.                                    */
            if(DeviceInfo->NDCS_ClientInfo.Time_With_Dst != 0)
            {
               /* Finally, submit a read request to the server          */
               if((ret_val = GATT_Read_Value_Request(BluetoothStackID, ConnectionID, DeviceInfo->NDCS_ClientInfo.Time_With_Dst, GATT_ClientEventCallback_NDCS, DeviceInfo->NDCS_ClientInfo.Time_With_Dst)) > 0)
               {
                  PRINTF("Get Time with Dst Request sent, Transaction ID = %u", ret_val);

                  ret_val = 0;
               }
               else
                  DisplayFunctionError("GATT_Read_Value_Request", ret_val);
            }
            else
               PRINTF("Error - TimeWithDST offset attribute handle is invalid!\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("Either a NDCS server must be registered or a NDCS client must be connected.\n");
   }

   return(ret_val);
}

   /* ***************************************************************** */
   /*                  RTUS Commands.                                   */
   /* ***************************************************************** */

   /* The following function is responsible for registering a RTUS      */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int RegisterRTUS(ParameterList_t *TempParam)
{
   int ret_val;
   RTUS_Time_Update_State_Data_t TimeUpdateStateData = DEFAULT_TIME_UPDATE_STATE_DATA;

   /* Verify that there is no active connection.                        */
   if(!ConnectionID)
   {
      /* Verify that the Service is not already registered.             */
      if(!RTUSInstanceID)
      {
         /* Register the RTUS Service with GATT.                        */
         ret_val = RTUS_Initialize_Service(BluetoothStackID, RTUS_EventCallback, 0, &RTUSInstanceID);
         if((ret_val > 0) && (RTUSInstanceID > 0))
         {
            /* Display success message.                                 */
            PRINTF("Successfully registered RTUS Service.\n");

            /* Save the ServiceID of the registered service.            */
            RTUSInstanceID = (unsigned int)ret_val;

            RTUS_Set_Time_Update_State(BluetoothStackID, RTUSInstanceID, &TimeUpdateStateData);

            /* Return success to the caller.                            */
            ret_val = 0;
         }
      }
      else
      {
         PRINTF("RTUS Service already registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Connection current active.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for unregistering a RTUS    */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int UnRegisterRTUS(ParameterList_t *TempParam)
{
   int ret_val = FUNCTION_ERROR;

   /* Verify that a service is registered.                              */
   if(RTUSInstanceID)
   {
      /* If there is a connected device, then first disconnect it.      */
      if(!COMPARE_NULL_BD_ADDR(ConnectionBD_ADDR))
         DisconnectLEDevice(BluetoothStackID, ConnectionBD_ADDR);

      /* Unregister the RTUS Service with GATT.                         */
      ret_val = RTUS_Cleanup_Service(BluetoothStackID, RTUSInstanceID);
      if(ret_val == 0)
      {
         /* Display success message.                                    */
         PRINTF("Successfully unregistered RTUS Service.\n");

         /* Save the ServiceID of the registered service.               */
         RTUSInstanceID = 0;
      }
      else
         DisplayFunctionError("RTUS_Cleanup_Service", ret_val);
   }
   else
      PRINTF("RTUS Service not registered.\n");

   return(ret_val);
}

   /* The following function is responsible for performing a RTUS       */
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverRTUS(ParameterList_t *TempParam)
{
   DeviceInfo_t *DeviceInfo;
   GATT_UUID_t   UUID[1];
   int           ret_val = FUNCTION_ERROR;

   /* Verify that we are not configured as a server                     */
   if(!RTUSInstanceID)
   {
      /* Verify that there is a connection that is established.         */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that no service discovery is outstanding for this */
            /* device.                                                  */
            if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING))
            {
               /* Configure the filter so that only the RTUS Service is */
               /* discovered.                                           */
               UUID[0].UUID_Type = guUUID_16;
               RTUS_ASSIGN_RTUS_SERVICE_UUID_16((&UUID[0].UUID.UUID_16));

               /* Start the service discovery process.                  */
               ret_val = GATT_Start_Service_Discovery(BluetoothStackID, ConnectionID, (sizeof(UUID)/sizeof(GATT_UUID_t)), UUID, GATT_Service_Discovery_Event_Callback, sdRTUS);
               if(!ret_val)
               {
                  /* Display success message.                           */
                  PRINTF("GATT_Start_Service_Discovery success.\n");

                  /* Flag that a Service Discovery Operation is         */
                  /* outstanding.                                       */
                  DeviceInfo->Flags |= DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;
               }
               else
                  DisplayFunctionError("GATT_Start_Service_Discovery", ret_val);
            }
            else
               PRINTF("Service Discovery Operation Outstanding for Device.\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("No Connection Established\n");
   }
   else
      PRINTF("Cannot discover RTUS Services when registered as a service.\n");

   return(ret_val);
}

   /* The following function is responsible for writing the Time update */
   /* State. It can be executed only by a Server. This                  */
   /* function will return zero on successful execution and a negative  */
   /* value on errors.                                                  */
static int SetTimeUpdateState(ParameterList_t *TempParam)
{
   int                           ret_val = FUNCTION_ERROR;
   RTUS_Time_Update_State_Data_t Time_Update_State;

   if((TempParam) && (TempParam->NumberofParameters == 2))
   {
      if(RTUSInstanceID)
      {
         Time_Update_State.CurrentState = (Byte_t)TempParam->Params[0].intParam;
         Time_Update_State.Result       = (Byte_t)TempParam->Params[1].intParam;

         ret_val = RTUS_Set_Time_Update_State(BluetoothStackID, RTUSInstanceID, &Time_Update_State);
         if(ret_val == 0)
         {
            PRINTF("Time Update State successfully set\n");
         }
         else
            DisplayFunctionError("RTUS_Set_Time_Update_State", ret_val);
      }
      else
      {
         if(ConnectionID)
            PRINTF("Cannot write Time Update State as a client.\n");
         else
            PRINTF("RTUS server not registered\n");
      }
   }
   else
   {
      DisplayTimeUpdateStateUsage(__FUNCTION__);
   }

   return(ret_val);
}

   /* The following function is responsible for reading the Time Update */
   /* State.  It can be executed by a server or a client with an open   */
   /* connection to a remote server.  If executed as a client, a GATT   */
   /* read request will be generated, and the results will be returned  */
   /* as a response in the GATT client event callback.  This function   */
   /* will return zero on successful execution and a negative value on  */
   /* errors.                                                           */
static int GetTimeUpdateState(ParameterList_t *TempParam)
{
   int                            ret_val = FUNCTION_ERROR;
   DeviceInfo_t                  *DeviceInfo;
   RTUS_Time_Update_State_Data_t  Time_Update_State;

   /* First check for a registered RTUS Server                          */
   if(RTUSInstanceID)
   {
      if((ret_val = RTUS_Query_Time_Update_State(BluetoothStackID, RTUSInstanceID, &Time_Update_State)) == 0)
      {
         DisplayTimeUpdateState(&Time_Update_State);
      }
      else
         DisplayFunctionError("RTUS_Query_Time_Update_State", ret_val);
   }
   else
   {
      /* Check to see if we are configured as a client with an active   */
      /* connection                                                     */
      if(ConnectionID)
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
         {
            /* Verify that the client has received a valid Time Update  */
            /* State.                                                   */
            if(DeviceInfo->RTUS_ClientInfo.Time_Update_State != 0)
            {
               /* Finally, submit a read request to the server          */
               if((ret_val = GATT_Read_Value_Request(BluetoothStackID, ConnectionID, DeviceInfo->RTUS_ClientInfo.Time_Update_State, GATT_ClientEventCallback_RTUS, DeviceInfo->RTUS_ClientInfo.Time_Update_State)) > 0)
               {
                  PRINTF("Get Time Update State Request sent, Transaction ID = %u", ret_val);

                  ret_val = 0;
               }
               else
                  DisplayFunctionError("GATT_Read_Value_Request", ret_val);
            }
            else
               PRINTF("Error - Time Update State not supported on remote service.\n");
         }
         else
            PRINTF("No Device Info.\n");
      }
      else
         PRINTF("Either a RTUS server must be registered or a RTUS client must be connected.\n");
   }

   return(ret_val);
}

   /* The following function is responsible for writing the Time Update */
   /* Control Point to connected remote device. It can be executed      */
   /* only by a client. This function will return zero on successful    */
   /* execution and a negative value on errors                          */
static int WriteTimeUpdateControlPoint(ParameterList_t *TempParam)
{
   int           ret_val = FUNCTION_ERROR;
   int           Input = -1;
   Byte_t        Buffer[RTUS_TIME_UPDATE_CONTROL_POINT_VALUE_LENGTH];
   DeviceInfo_t *DeviceInfo;

   if((TempParam) && (TempParam->NumberofParameters >= 1))
   {
      Input = TempParam->Params[0].intParam;
      if((Input == RTUS_TIME_UPDATE_CONTROL_POINT_GET_REFERENCE_UPDATE)||(Input == RTUS_TIME_UPDATE_CONTROL_POINT_CANCEL_REFERENCE_UPDATE))
      {
         /* Verify that we are not configured as a Server               */
         if(!RTUSInstanceID)
         {  /* Verify that there is a valid connection                  */
            if(ConnectionID)
            {
               /* Get the device info for the connection device         */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
               {
                  /* Verify that the client has received a valid Time   */
                  /* UpdateControlPoint.                                */
                  if(DeviceInfo->RTUS_ClientInfo.Control_Point != 0)
                  {
                     /* Format the Command                              */
                     if((ret_val = RTUS_Format_Control_Point_Command(Input, RTUS_TIME_UPDATE_CONTROL_POINT_VALUE_LENGTH, Buffer)) == 0)
                     {
                        /* Finally, submit a write without response     */
                        /* request to the server.                       */
                        if((ret_val = GATT_Write_Without_Response_Request(BluetoothStackID, ConnectionID, DeviceInfo->RTUS_ClientInfo.Control_Point, RTUS_TIME_UPDATE_CONTROL_POINT_VALUE_LENGTH, ((void *)Buffer))) > 0)
                        {
                           PRINTF("Number of Bytes written : %d",ret_val);
                           ret_val = 0;
                        }
                        else
                           DisplayFunctionError("GATT_Write_Without_Response_Request", ret_val);
                     }
                     else
                     {
                        DisplayFunctionError("RTUS_Format_Control_Point_Command", ret_val);
                        ret_val = FUNCTION_ERROR;
                     }
                  }
                  else
                     PRINTF("Error - Time Update Control Point not supported on remote service!\n");
               }
               else
                  PRINTF("Error getting device info.\n");
            }
            else
               PRINTF("Connection is not established.\n");
         }
         else
            PRINTF("Cannot Set Time Update Control Point when registered as a service.\n");
      }
      else
      {
         PRINTF("Input Invalid.\n");
         DisplayUsage("SetTimeUpdateControlPoint [Command (1 = Get_Reference_Update, 2 = Cancel_Reference_Update)]");
      }
   }
   else
   {
      DisplayUsage("SetTimeUpdateControlPoint [Command (1 = Get_Reference_Update, 2 = Cancel_Reference_Update)]");
   }

   return(ret_val);
}

   /* ***************************************************************** */
   /*                         Event Callbacks                           */
   /* ***************************************************************** */

   /* The following function is for the GAP LE Event Receive Data       */
   /* Callback.  This function will be called whenever a Callback has   */
   /* been registered for the specified GAP LE Action that is associated*/
   /* with the Bluetooth Stack.  This function passes to the caller the */
   /* GAP LE Event Data of the specified Event and the GAP LE Event     */
   /* Callback Parameter that was specified when this Callback was      */
   /* installed.  The caller is free to use the contents of the GAP LE  */
   /* Event Data ONLY in the context of this callback.  If the caller   */
   /* requires the Data for a longer period of time, then the callback  */
   /* function MUST copy the data into another Data Buffer.  This       */
   /* function is guaranteed NOT to be invoked more than once           */
   /* simultaneously for the specified installed callback (i.e.  this   */
   /* function DOES NOT have be reentrant).  It Needs to be noted       */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the Thread*/
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because other GAP Events will not be  */
   /* processed while this function call is outstanding).               */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID, GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter)
{
   int                                           Result;
   BoardStr_t                                    BoardStr;
   unsigned int                                  Index;
   DeviceInfo_t                                 *DeviceInfo;
   Long_Term_Key_t                               GeneratedLTK;
   GAP_LE_Connection_Parameters_t                ConnectionParameters;
   GAP_LE_Security_Information_t                 GAP_LE_Security_Information;
   GAP_LE_Advertising_Report_Data_t             *DeviceEntryPtr;
   GAP_LE_Authentication_Event_Data_t           *Authentication_Event_Data;
   GAP_LE_Authentication_Response_Information_t  GAP_LE_Authentication_Response_Information;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GAP_LE_Event_Data))
   {
      switch(GAP_LE_Event_Data->Event_Data_Type)
      {
         case etLE_Advertising_Report:
            PRINTF("etLE_Advertising_Report with size %d.\n",(int)GAP_LE_Event_Data->Event_Data_Size);
            PRINTF("  %d Responses.\n",GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries);

            for(Index = 0; Index < GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries; Index++)
            {
               DeviceEntryPtr = &(GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Advertising_Data[Index]);

               /* Display the packet type for the device                */
               switch(DeviceEntryPtr->Advertising_Report_Type)
               {
                  case rtConnectableUndirected:
                     PRINTF("  Advertising Type: %s.\n", "rtConnectableUndirected");
                     break;
                  case rtConnectableDirected:
                     PRINTF("  Advertising Type: %s.\n", "rtConnectableDirected");
                     break;
                  case rtScannableUndirected:
                     PRINTF("  Advertising Type: %s.\n", "rtScannableUndirected");
                     break;
                  case rtNonConnectableUndirected:
                     PRINTF("  Advertising Type: %s.\n", "rtNonConnectableUndirected");
                     break;
                  case rtScanResponse:
                     PRINTF("  Advertising Type: %s.\n", "rtScanResponse");
                     break;
               }

               /* Display the Address Type.                             */
               if(DeviceEntryPtr->Address_Type == latPublic)
               {
                  PRINTF("  Address Type: %s.\n","atPublic");
               }
               else
               {
                  PRINTF("  Address Type: %s.\n","atRandom");
               }

               /* Display the Device Address.                           */
               PRINTF("  Address: 0x%02X%02X%02X%02X%02X%02X.\n", DeviceEntryPtr->BD_ADDR.BD_ADDR5, DeviceEntryPtr->BD_ADDR.BD_ADDR4, DeviceEntryPtr->BD_ADDR.BD_ADDR3, DeviceEntryPtr->BD_ADDR.BD_ADDR2, DeviceEntryPtr->BD_ADDR.BD_ADDR1, DeviceEntryPtr->BD_ADDR.BD_ADDR0);
               PRINTF("  RSSI: 0x%02X.\n", DeviceEntryPtr->RSSI);
               PRINTF("  Data Length: %d.\n", DeviceEntryPtr->Raw_Report_Length);

               DisplayAdvertisingData(&(DeviceEntryPtr->Advertising_Data));
            }
            break;
         case etLE_Connection_Complete:
            PRINTF("etLE_Connection_Complete with size %d.\n",(int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address, BoardStr);

               PRINTF("   Status:       0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status);
               PRINTF("   Role:         %s.\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)?"Master":"Slave");
               PRINTF("   Address Type: %s.\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type == latPublic)?"Public":"Random");
               PRINTF("   BD_ADDR:      %s.\n", BoardStr);

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  ConnectionBD_ADDR   = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
                  LocalDeviceIsMaster = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master;

                  /* Make sure that no entry already exists.            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) == NULL)
                  {
                     /* No entry exists so create one.                  */
                     if(!CreateNewDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, ConnectionBD_ADDR))
                        PRINTF("Failed to add device to Device Info List.\n");
                  }
                  else
                  {
                     /* If we are the Master of the connection we will  */
                     /* attempt to Re-Establish Security if a LTK for   */
                     /* this device exists (i.e.  we previously paired).*/
                     if(LocalDeviceIsMaster)
                     {
                        /* Re-Establish Security if there is a LTK that */
                        /* is stored for this device.                   */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LTK_VALID)
                        {
                           /* Re-Establish Security with this LTK.      */
                           PRINTF("Attempting to Re-Establish Security.\n");

                           /* Attempt to re-establish security to this  */
                           /* device.                                   */
                           GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.LTK), &(DeviceInfo->LTK), sizeof(DeviceInfo->LTK));
                           GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.Rand), &(DeviceInfo->Rand), sizeof(DeviceInfo->Rand));
                           GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                           Result = GAP_LE_Reestablish_Security(BluetoothStackID, ConnectionBD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                           if(Result)
                           {
                              DisplayFunctionError("GAP_LE_Reestablish_Security", Result);
                           }
                        }
                     }
                  }
               }
            }
            break;
         case etLE_Connection_Parameter_Update_Request:
            PRINTF("\netLE_Connection_Parameter_Update_Request with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, BoardStr);
               PRINTF("   BD_ADDR:             %s.\n", BoardStr);
               PRINTF("   Minimum Interval:    %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min);
               PRINTF("   Maximum Interval:    %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max);
               PRINTF("   Slave Latency:       %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency);
               PRINTF("   Supervision Timeout: %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout);

               /* Initialize the connection parameters.                 */
               ConnectionParameters.Connection_Interval_Min    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min;
               ConnectionParameters.Connection_Interval_Max    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max;
               ConnectionParameters.Slave_Latency              = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency;
               ConnectionParameters.Supervision_Timeout        = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout;
               ConnectionParameters.Minimum_Connection_Length  = 0;
               ConnectionParameters.Maximum_Connection_Length  = 10000;

               PRINTF("\nAttempting to accept connection parameter update request.\n");

               /* Go ahead and accept whatever the slave has requested. */
               Result = GAP_LE_Connection_Parameter_Update_Response(BluetoothStackID, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, TRUE, &ConnectionParameters);
               if(!Result)
               {
                  PRINTF("      GAP_LE_Connection_Parameter_Update_Response() success.\n");
               }
               else
               {
                  PRINTF("      GAP_LE_Connection_Parameter_Update_Response() error %d.\n", Result);
               }
            }
            break;
         case etLE_Connection_Parameter_Updated:
            PRINTF("\netLE_Connection_Parameter_Updated with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->BD_ADDR, BoardStr);
               PRINTF("   Status:              0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status);
               PRINTF("   BD_ADDR:             %s.\n", BoardStr);

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  PRINTF("   Connection Interval: %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Connection_Interval);
                  PRINTF("   Slave Latency:       %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Slave_Latency);
                  PRINTF("   Supervision Timeout: %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Supervision_Timeout);
               }
            }
            break;
         case etLE_Disconnection_Complete:
            PRINTF("etLE_Disconnection_Complete with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data)
            {
               PRINTF("   Status: 0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Status);
               PRINTF("   Reason: 0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Reason);

               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address, BoardStr);
               PRINTF("   BD_ADDR: %s.\n", BoardStr);

               /* Check to see if the device info is present in the     */
               /* list.                                                 */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
               {
                  /* If this device is not paired, then delete it.  The */
                  /* LTK will be valid if the device is paired.         */
                  if(!(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LTK_VALID))
                  {
                     if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
                        FreeDeviceInfoEntryMemory(DeviceInfo);
                  }
                  else
                  {
                     /* Flag that the Link is no longer encrypted since */
                     /* we have disconnected.                           */
                     DeviceInfo->Flags = DEVICE_INFO_FLAGS_LTK_VALID;
                  }
               }
               else
                  PRINTF("Warning - Disconnect from unknown device.\n");

               /* Clear the saved Connection BD_ADDR.                   */
               ASSIGN_BD_ADDR(ConnectionBD_ADDR, 0, 0, 0, 0, 0, 0);
               LocalDeviceIsMaster = FALSE;
            }
            break;
         case etLE_Encryption_Change:
            PRINTF("etLE_Encryption_Change with size %d.\n",(int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data)
            {
               /* Search for the device entry to see flag if the link is*/
               /* encrypted.                                            */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->BD_ADDR)) != NULL)
               {
                  /* Check to see if the encryption change was          */
                  /* successful.                                        */
                  if((GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Change_Status == HCI_ERROR_CODE_NO_ERROR) && (GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Mode == emEnabled))
                     DeviceInfo->Flags |= DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
                  else
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
               }
            }
            break;
         case etLE_Encryption_Refresh_Complete:
            PRINTF("etLE_Encryption_Refresh_Complete with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data)
            {
               /* Search for the device entry to see flag if the link is*/
               /* encrypted.                                            */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->BD_ADDR)) != NULL)
               {
                  /* Check to see if the refresh was successful.        */
                  if(GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
                     DeviceInfo->Flags |= DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
                  else
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_LINK_ENCRYPTED;
               }
            }
            break;
         case etLE_Authentication:
            PRINTF("etLE_Authentication with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            /* Make sure the authentication event data is valid before  */
            /* continuing.                                              */
            if((Authentication_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Authentication_Event_Data) != NULL)
            {
               BD_ADDRToStr(Authentication_Event_Data->BD_ADDR, BoardStr);

               switch(Authentication_Event_Data->GAP_LE_Authentication_Event_Type)
               {
                  case latLongTermKeyRequest:
                     PRINTF("    latKeyRequest: \n");
                     PRINTF("     BD_ADDR : %s.\n", BoardStr);

                     /* The other side of a connection is requesting    */
                     /* that we start encryption. Thus we should        */
                     /* regenerate LTK for this connection and send it  */
                     /* to the chip.                                    */
                     Result = GAP_LE_Regenerate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV, &(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand), &GeneratedLTK);
                     if(!Result)
                     {
                        DisplayKey("     Link Key: 0x", sizeof(Link_Key_t), (Byte_t *)&GeneratedLTK);
                        DisplayKey("     Rand    : 0x", sizeof(Random_Number_t), (Byte_t *)&(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand));
                        PRINTF("     EDIV    : %04X\n", Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV);

                        /* Respond with the Re-Generated Long Term Key. */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type                                        = larLongTermKey;
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length                                        = GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
                        GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;
                        GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key       = GeneratedLTK;
                     }
                     else
                     {
                        PRINTF("      GAP_LE_Regenerate_Long_Term_Key returned %d.\n",Result);

                        /* Since we failed to generate the requested key*/
                        /* we should respond with a negative response.  */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larLongTermKey;
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length = 0;
                     }

                     /* Send the Authentication Response.               */
                     Result = GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
                     if(Result)
                     {
                        PRINTF("      GAP_LE_Authentication_Response returned %d.\n",Result);
                     }
                     break;
                  case latSecurityRequest:
                     /* Display the data for this event.                */
                     /* * NOTE * This is only sent from Slave to Master.*/
                     /*          Thus we must be the Master in this     */
                     /*          connection.                            */
                     PRINTF("    latSecurityRequest:.\n");
                     PRINTF("      BD_ADDR: %s.\n", BoardStr);
                     PRINTF("      Bonding Type: %s.\n", ((Authentication_Event_Data->Authentication_Event_Data.Security_Request.Bonding_Type == lbtBonding)?"Bonding":"No Bonding"));
                     PRINTF("      MITM: %s.\n", ((Authentication_Event_Data->Authentication_Event_Data.Security_Request.MITM)?"YES":"NO"));

                     /* Determine if we have previously paired with the */
                     /* device. If we have paired we will attempt to    */
                     /* re-establish security using a previously        */
                     /* exchanged LTK.                                  */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                     {
                        /* Determine if a Valid Long Term Key is stored */
                        /* for this device.                             */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAGS_LTK_VALID)
                        {
                           PRINTF("Attempting to Re-Establish Security.\n");

                           /* Attempt to re-establish security to this  */
                           /* device.                                   */
                           GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.LTK), &(DeviceInfo->LTK), sizeof(DeviceInfo->LTK));
                           GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                           BTPS_MemCopy(&(GAP_LE_Security_Information.Security_Information.Master_Information.Rand), &(DeviceInfo->Rand), sizeof(DeviceInfo->Rand));
                           GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                           Result = GAP_LE_Reestablish_Security(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                           if(Result)
                           {
                              PRINTF("GAP_LE_Reestablish_Security returned %d.\n",Result);
                           }
                        }
                        else
                        {
                           CurrentRemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                           /* We do not have a stored Link Key for this */
                           /* device so go ahead and pair to this       */
                           /* device.                                   */
                           SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                        }
                     }
                     else
                     {
                        CurrentRemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                        /* There is no Key Info Entry for this device   */
                        /* so we will just treat this as a slave        */
                        /* request and initiate pairing.                */
                        SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                     }

                     break;
                  case latPairingRequest:
                     CurrentRemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                     PRINTF("Pairing Request: %s.\n",BoardStr);
                     DisplayPairingInformation(Authentication_Event_Data->Authentication_Event_Data.Pairing_Request);

                     /* This is a pairing request. Respond with a       */
                     /* Pairing Response.                               */
                     /* * NOTE * This is only sent from Master to Slave.*/
                     /*          Thus we must be the Slave in this      */
                     /*          connection.                            */

                     /* Send the Pairing Response.                      */
                     SlavePairingRequestResponse(Authentication_Event_Data->BD_ADDR);
                     break;
                  case latConfirmationRequest:
                     PRINTF("latConfirmationRequest.\n");

                     if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtNone)
                     {
                        PRINTF("Invoking Just Works.\n");

                        /* Just Accept Just Works Pairing.              */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larConfirmation;

                        /* By setting the Authentication_Data_Length to */
                        /* any NON-ZERO value we are informing the GAP  */
                        /* LE Layer that we are accepting Just Works    */
                        /* Pairing.                                     */
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length = DWORD_SIZE;

                        Result = GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
                        if(Result)
                        {
                           PRINTF("GAP_LE_Authentication_Response returned %d.\n",Result);
                        }
                     }
                     else
                     {
                        if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtPasskey)
                        {
                           PRINTF("Call LEPasskeyResponse [PASSCODE].\n");
                        }
                        else
                        {
                           if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtDisplay)
                           {
                              PRINTF("Passkey: %06u.\n", (unsigned int)(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey));
                           }
                        }
                     }
                     break;
                  case latSecurityEstablishmentComplete:
                     PRINTF("Security Re-Establishment Complete: %s.\n", BoardStr);
                     PRINTF("                            Status: 0x%02X.\n", Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status);
                     break;
                  case latPairingStatus:
                     ASSIGN_BD_ADDR(CurrentRemoteBD_ADDR, 0, 0, 0, 0, 0, 0);

                     PRINTF("Pairing Status: %s.\n", BoardStr);
                     PRINTF("        Status: 0x%02X.\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status);

                     if(Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status == GAP_LE_PAIRING_STATUS_NO_ERROR)
                     {
                        PRINTF("        Key Size: %d.\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size);
                     }
                     else
                     {
                        /* Failed to pair so delete the key entry for   */
                        /* this device and disconnect the link.         */
                        if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                           FreeDeviceInfoEntryMemory(DeviceInfo);

                        /* Disconnect the Link.                         */
                        GAP_LE_Disconnect(BluetoothStackID, Authentication_Event_Data->BD_ADDR);
                     }
                     break;
                  case latEncryptionInformationRequest:
                     PRINTF("Encryption Information Request %s.\n", BoardStr);

                     /* Generate new LTK, EDIV and Rand and respond with*/
                     /* them.                                           */
                     EncryptionInformationRequestResponse(Authentication_Event_Data->BD_ADDR, Authentication_Event_Data->Authentication_Event_Data.Encryption_Request_Information.Encryption_Key_Size, &GAP_LE_Authentication_Response_Information);
                     break;
                  case latEncryptionInformation:
                     /* Display the information from the event.         */
                     PRINTF(" Encryption Information from RemoteDevice: %s.\n", BoardStr);
                     PRINTF("                                 Key Size: %d\n", Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size);
                     DisplayKey("                                 Link Key: 0x", sizeof(Link_Key_t), (Byte_t *)&Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK);
                     DisplayKey("                                 Rand    : 0x", sizeof(Random_Number_t), (Byte_t *)&(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand));
                     PRINTF("                                 EDIV    : %04X\n", Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV);

                     /* Search for the entry for this slave to store the*/
                     /* information into.                               */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                     {
                        BTPS_MemCopy(&(DeviceInfo->LTK), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK), sizeof(DeviceInfo->LTK));
                        DeviceInfo->EDIV              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.EDIV;
                        BTPS_MemCopy(&(DeviceInfo->Rand), &(Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Rand), sizeof(DeviceInfo->Rand));
                        DeviceInfo->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size;
                        DeviceInfo->Flags            |= DEVICE_INFO_FLAGS_LTK_VALID;
                     }
                     else
                     {
                        PRINTF("No Key Info Entry for this Slave.\n");
                     }
                     break;
                  default:
                     break;
               }
            }
            break;
         default:
            break;
      }

      /* Display the command prompt.                                    */
      DisplayPrompt();
   }
}

   /* The following is a CTP Server Event Callback.  This function will */
   /* be called whenever an CTP Server Profile Event occurs that is     */
   /* associated with the specified Bluetooth Stack ID.  This function  */
   /* passes to the caller the Bluetooth Stack ID, the CTS Event Data   */
   /* that occurred and the CTS Event Callback Parameter that was       */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the CTS Event Data ONLY in the context of this*/
   /* callback.  If the caller requires the Data for a longer period of */
   /* time, then the callback function MUST copy the data into another  */
   /* Data Buffer This function is guaranteed NOT to be invoked more    */
   /* than once simultaneously for the specified installed callback     */
   /* (i.e.  this function DOES NOT have be re-entrant).  It needs to be*/
   /* noted however, that if the same Callback is installed more than   */
   /* once, then the callbacks will be called serially.  Because of     */
   /* this, the processing in this function should be as efficient as   */
   /* possible.  It should also be noted that this function is called in*/
   /* the Thread Context of a Thread that the User does NOT own.        */
   /* Therefore, processing in this function should be as efficient as  */
   /* possible (this argument holds anyway because another CTS Event    */
   /* will not be processed while this function call is outstanding).   */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving CTS Event Packets.  */
   /*            A Deadlock WILL occur because NO CTS Event Callbacks   */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
static void BTPSAPI CTS_EventCallback(unsigned int BluetoothStackID, CTS_Event_Data_t *CTS_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;
   BoardStr_t    BoardStr;
   int           Result;
   unsigned int  Transaction_ID = 0;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (CTS_Event_Data))
   {
      switch(CTS_Event_Data->Event_Data_Type)
      {
         case etCTS_Server_Read_Client_Configuration_Request:
            PRINTF("etCTS_Server_Read_Client_Configuration_Request with size %u.\n", CTS_Event_Data->Event_Data_Size);

            if(CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data)
            {
               BD_ADDRToStr(CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->RemoteDevice, BoardStr);
               PRINTF("   Instance ID:      %u.\n", CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->InstanceID);
               PRINTF("   Connection ID:    %u.\n", CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->ConnectionID);
               PRINTF("   Transaction ID:   %u.\n", CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->TransactionID);
               PRINTF("   Connection Type:  %s.\n", ((CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:    %s.\n", BoardStr);

               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->RemoteDevice)) != NULL)
               {
                  /* Validate event parameters                          */
                  if(CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->InstanceID == CTSInstanceID)
                  {
                     Result = 0;

                     switch(CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->ClientConfigurationType)
                     {
                        case ctCurrentTime:
                           PRINTF("   Config Type:      ctCurrentTime.\n");
                           Result = CTS_Read_Client_Configuration_Response(BluetoothStackID, CTSInstanceID, CTS_Event_Data->Event_Data.CTS_Read_Client_Configuration_Data->TransactionID, DeviceInfo->ServerInfo.Current_Time_Client_Configuration);
                           break;

                        default:
                           PRINTF("   Config Type:      Unknown.\n");
                           break;
                     }

                     if(Result)
                        DisplayFunctionError("CTS_Read_Client_Configuration_Response", Result);
                  }
                  else
                  {
                     PRINTF("\nInvalid Event data.\n");
                  }
               }
               else
               {
                  PRINTF("\nUnknown Client.\n");
               }
            }
            break;
         case etCTS_Server_Update_Client_Configuration_Request:
            PRINTF("etCTS_Server_Update_Client_Configuration_Request with size %u.\n", CTS_Event_Data->Event_Data_Size);

            if(CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data)
            {
               BD_ADDRToStr(CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->RemoteDevice, BoardStr);
               PRINTF("   Instance ID:      %u.\n", CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->InstanceID);
               PRINTF("   Connection ID:    %u.\n", CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->ConnectionID);
               PRINTF("   Connection Type:  %s.\n", ((CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:    %s.\n", BoardStr);

               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->RemoteDevice)) != NULL)
               {
                  /* Validate event parameters                          */
                  if(CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->InstanceID == CTSInstanceID)
                  {
                     switch(CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->ClientConfigurationType)
                     {
                        case ctCurrentTime:
                           PRINTF("   Config Type:      ctCurrentTime.\n");
                           DeviceInfo->ServerInfo.Current_Time_Client_Configuration = CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->ClientConfiguration;
                           break;

                        default:
                           PRINTF("   Config Type:      Unknown.\n");
                           break;
                     }

                     PRINTF("   Value:            0x%04X.\n", (unsigned int)CTS_Event_Data->Event_Data.CTS_Client_Configuration_Update_Data->ClientConfiguration);
                  }
                  else
                  {
                     PRINTF("\nInvalid Event data.\n");
                  }
               }
               else
               {
                  PRINTF("\nUnknown Client.\n");
               }
            }
            break;

         case etCTS_Server_Read_Current_Time_Request:
            PRINTF(" etCTS_Server_Read_Current_Time_Request EVENT received \n");
            Transaction_ID = CTS_Event_Data->Event_Data.CTS_Read_Current_Time_Request_Data->TransactionID;
            Result  = CTS_Current_Time_Read_Request_Response(BluetoothStackID ,Transaction_ID,&CurrentTime);
            PRINTF("   Result:    %d.\n", Result);
            break ;

         case etCTS_Server_Write_Current_Time_Request:
            PRINTF(" etCTS_Server_Write_Current_Time_Request EVENT received \n");

            if(CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data)
            {
               BD_ADDRToStr(CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->RemoteDevice, BoardStr);
               PRINTF("   Instance ID:      %u.\n", CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->InstanceID);
               PRINTF("   Connection ID:    %u.\n", CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->ConnectionID);
               PRINTF("   Connection Type:  %s.\n", ((CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:    %s.\n", BoardStr);

               /* We will simply accept the changes from the client     */
               /* (ignoring adjust reason).                             */
               CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Year    = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Year;
               CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Month   = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Month;
               CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Day     = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Day;
               CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Hours   = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Hours;
               CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Minutes = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Minutes;
               CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Seconds = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Day_Date_Time.Date_Time.Seconds;

               CurrentTime.Exact_Time.Day_Date_Time.Day_Of_Week       = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Day_Date_Time.Day_Of_Week;
               CurrentTime.Exact_Time.Fractions256                    = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Exact_Time.Fractions256;

               CurrentTime.Adjust_Reason_Mask                         = CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->CurrentTime.Adjust_Reason_Mask;

               /* Send the response.                                    */
               if((Result = CTS_Current_Time_Write_Request_Response(BluetoothStackID, CTS_Event_Data->Event_Data.CTS_Write_Current_Time_Request_Data->TransactionID, CTS_ERROR_CODE_SUCCESS)) != 0)
                  DisplayFunctionError("CTS_Current_Time_Write_Request_Response", Result);
            }
            break ;
         case etCTS_Server_Write_Local_Time_Information_Request:
            PRINTF(" etCTS_Server_Write_Local_Time_Information_Request EVENT received \n");

            if(CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data)
            {
               BD_ADDRToStr(CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data->RemoteDevice, BoardStr);
               PRINTF("   Instance ID:      %u.\n", CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data->InstanceID);
               PRINTF("   Connection ID:    %u.\n", CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data->ConnectionID);
               PRINTF("   Connection Type:  %s.\n", ((CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:    %s.\n", BoardStr);

               /* Let's store the local time information.               */
               if(!(Result = CTS_Set_Local_Time_Information(BluetoothStackID, CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data->InstanceID, &(CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data->LocalTime))))
               {
                  /* Send the response.                                 */
                  if((Result = CTS_Local_Time_Information_Write_Request_Response(BluetoothStackID, CTS_Event_Data->Event_Data.CTS_Write_Local_Time_Information_Request_Data->TransactionID, CTS_ERROR_CODE_SUCCESS)) != 0)
                     DisplayFunctionError("CTS_Local_Time_Information_Write_Request_Response", Result);
               }
               else
                  DisplayFunctionError("CTS_Set_Local_Time_Information", Result);
            }
            break ;
         case etCTS_Server_Read_Reference_Time_Information_Request:
            PRINTF(" etCTS_Server_Read_Reference_Time_Information_Request EVENT received \n");
            Transaction_ID = CTS_Event_Data->Event_Data.CTS_Read_Reference_Time_Information_Request_Data->TransactionID;
            Result  = CTS_Reference_Time_Information_Read_Request_Response(BluetoothStackID,Transaction_ID,&ReferenceTimeInformation);
            PRINTF("   Result:    %d.\n", Result);
            break ;
         default:
            PRINTF("Unknown CTS Event\n");
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");

      PRINTF("CTS Callback Data: Event_Data = NULL.\n");
   }

   DisplayPrompt();
}

   /* The following is a RTUS Server Event Callback.  This function will*/
   /* be called whenever an RTUS Server Profile Event occurs that is    */
   /* associated with the specified Bluetooth Stack ID.  This function  */
   /* passes to the caller the Bluetooth Stack ID, the RTUS Event Data  */
   /* that occurred and the RTUS Event Callback Parameter that was      */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the RTUS Event Data ONLY in the context of    */
   /* this callback.  If the caller requires the Data for a longer      */
   /* period of time, then the callback function MUST copy the data into*/
   /* another Data Buffer This function is guaranteed NOT to be invoked */
   /* more than once simultaneously for the specified installed callback*/
   /* (i.e.  this function DOES NOT have be re-entrant).  It needs to be*/
   /* noted however, that if the same Callback is installed more than   */
   /* once, then the callbacks will be called serially.  Because of     */
   /* this, the processing in this function should be as efficient as   */
   /* possible.  It should also be noted that this function is called in*/
   /* the Thread Context of a Thread that the User does NOT own.        */
   /* Therefore, processing in this function should be as efficient as  */
   /* possible (this argument holds anyway because another RTUS Event   */
   /* will not be processed while this function call is outstanding).   */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving RTUS Event Packets. */
   /*            A Deadlock WILL occur because NO RTUS Event Callbacks  */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
static void BTPSAPI RTUS_EventCallback(unsigned int BluetoothStackID, RTUS_Event_Data_t *RTUS_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t                  *DeviceInfo;
   BoardStr_t                     BoardStr;
   int                            Result;
   RTUS_Time_Update_State_Data_t  Time_Update_State;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (RTUS_Event_Data))
   {
      switch(RTUS_Event_Data->Event_Data_Type)
      {
         case etRTUS_Server_Time_Update_Control_Point_Command:
            PRINTF("etRTUS_Server_Time_Update_Control_Point_Command with size %u.\n", RTUS_Event_Data->Event_Data_Size);

            if(RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data)
            {
               BD_ADDRToStr(RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data->RemoteDevice, BoardStr);
               PRINTF("   Instance ID:      %u.\n", RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data->InstanceID);
               PRINTF("   Connection ID:    %u.\n", RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data->ConnectionID);
               PRINTF("   Connection Type:  %s.\n", ((RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:    %s.\n", BoardStr);

               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data->RemoteDevice)) != NULL)
               {
                  /* Validate event parameters                          */
                  if(RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data->InstanceID == RTUSInstanceID)
                  {
                     Result = 0;

                     switch(RTUS_Event_Data->Event_Data.RTUS_Time_Update_Control_Command_Data->Command)
                     {
                        case cpGet_Reference_Update:
                           PRINTF("   Command Type:     cpGet_Reference_Update.\n");
                           Time_Update_State.CurrentState = RTUS_CURRENT_STATE_UPDATE_PENDING;

                           Result = RTUS_Set_Time_Update_State(BluetoothStackID, RTUSInstanceID, &Time_Update_State);
                           break;
                        case cpCancel_Reference_Update:
                           PRINTF("   Command Type:     cpCancel_Reference_Update.\n");
                           Time_Update_State.CurrentState = RTUS_CURRENT_STATE_IDLE;
                           Time_Update_State.Result       = RTUS_RESULT_CANCELED;

                           Result = RTUS_Set_Time_Update_State(BluetoothStackID, RTUSInstanceID, &Time_Update_State);
                           break;
                        default:
                           PRINTF("   Config Type:      Unknown.\n");
                           break;
                     }

                     if(Result)
                        DisplayFunctionError("RTUS_Set_Time_Update_State", Result);
                  }
                  else
                  {
                     PRINTF("\nInvalid Event data.\n");
                  }
               }
               else
               {
                  PRINTF("\nUnknown Client.\n");
               }
            }
            break;

         default:
            PRINTF("Unknown RTUS Event\n");
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\nRTUS Callback Data: Event_Data = NULL.\n");
   }

   DisplayPrompt();
}

static void BTPSAPI NDCS_EventCallback(unsigned int BluetoothStackID, NDCS_Event_Data_t *NDCS_Event_Data, unsigned long CallbackParameter)
{
   int          Result;
   unsigned int Transaction_ID = 0;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (NDCS_Event_Data))
   {
     switch(NDCS_Event_Data->Event_Data_Type)
     {
        case etNDCS_Server_Read_Current_Time_Request:
           PRINTF(" etNDCS_Server_Read_Current_Time_Request EVENT received \n");
           Transaction_ID = NDCS_Event_Data->Event_Data.NDCS_Read_Time_With_DST_Request_Data->TransactionID ;
           Result = NDCS_Time_With_DST_Read_Request_Response(BluetoothStackID,Transaction_ID , &TimeWithDst );
           PRINTF("   Result:    %d.\n", Result);
           break ;

        default:
           PRINTF("Unknown CTS Event\n");
           break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");

      PRINTF("CTS Callback Data: Event_Data = NULL.\n");
   }

   DisplayPrompt();
}

   /* The following function is for an GATT Client Event Callback.  This*/
   /* function will be called whenever a GATT Response is received for a*/
   /* request that was made when this function was registered.  This    */
   /* function passes to the caller the GATT Client Event Data that     */
   /* occurred and the GATT Client Event Callback Parameter that was    */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the GATT Client Event Data ONLY in the context*/
   /* of this callback.  If the caller requires the Data for a longer   */
   /* period of time, then the callback function MUST copy the data into*/
   /* another Data Buffer.  This function is guaranteed NOT to be       */
   /* invoked more than once simultaneously for the specified installed */
   /* callback (i.e.  this function DOES NOT have be reentrant).  It    */
   /* Needs to be noted however, that if the same Callback is installed */
   /* more than once, then the callbacks will be called serially.       */
   /* Because of this, the processing in this function should be as     */
   /* efficient as possible.  It should also be noted that this function*/
   /* is called in the Thread Context of a Thread that the User does NOT*/
   /* own.  Therefore, processing in this function should be as         */
   /* efficient as possible (this argument holds anyway because another */
   /* GATT Event (Server/Client or Connection) will not be processed    */
   /* while this function call is outstanding).                         */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_ClientEventCallback_CTS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;
   BoardStr_t    BoardStr;
   Word_t        Index;
   Byte_t       *Value;
   Word_t        ValueLength;
   int           Result;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Client_Event_Data))
   {
      /* Determine the event that occurred.                             */
      switch(GATT_Client_Event_Data->Event_Data_Type)
      {
         case etGATT_Client_Error_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data)
            {
               PRINTF("\nError Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Error Type:      %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)?"Response Error":"Response Timeout");

               /* Only print out the rest if it is valid.               */
               if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)
               {
                  PRINTF("   Request Opcode:  0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestOpCode);
                  PRINTF("   Request Handle:  0x%04X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestHandle);
                  PRINTF("   Error Code:      0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode);
               }
            }
            else
               PRINTF("Error - Null Error Response Data.\n");
            break;
         case etGATT_Client_Read_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data)
            {
               ValueLength = GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength;
               Value = GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue;

               PRINTF("\nRead Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Data Length:     %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);

               /* If we know about this device and a callback parameter */
               /* exists, then check if we know what read response this */
               /* is.                                                   */
               if(ValueLength != 0)
               {
                  if(((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) != NULL) && (CallbackParameter != 0))
                  {
                     if(CallbackParameter == DeviceInfo->ClientInfo.Current_Time)
                     {
                        if(ValueLength == CTS_CURRENT_TIME_SIZE)
                        {
                           if((Result = DecodeDisplayCurrentTime(ValueLength,Value)) != 0)
                           {
                              DisplayFunctionError("DecodeDisplayCurrentTime", Result);
                           }
                        }
                        else
                           PRINTF("\nError - Invalid length (%u) for Temperature Type response\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);
                     }
                     else
                     {
                        if(CallbackParameter == DeviceInfo->ClientInfo.Local_Time_Information)
                        {
                           if(ValueLength == CTS_LOCAL_TIME_INFORMATION_SIZE)
                           {
                              if((Result = DecodeDisplayLocalTime(ValueLength,Value)) != 0)
                              {
                                 DisplayFunctionError("DecodeDisplayLocalTime", Result);
                              }
                           }
                           else
                              PRINTF("\nError - Invalid length (%u) for Measurement Interval response.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);
                        }
                        else
                        {
                           if(CallbackParameter == DeviceInfo->ClientInfo.Reference_Time_Information)
                           {
                              if(ValueLength == CTS_REFERENCE_TIME_INFORMATION_SIZE)
                              {
                                 if((Result = DecodeDisplayReferenceTime(ValueLength,Value)) != 0)
                                 {
                                    DisplayFunctionError("DecodeDisplayReferenceTime", Result);
                                 }
                              }
                              else
                                 PRINTF("\nError - Invalid length (%u) for Reference Time Information response.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);
                           }
                           else
                           {
                              /* Could not find a descriptor to match   */
                              /* the read response, so display raw data */
                              CallbackParameter = 0;
                           }
                        }
                     }
                  }

                  /* If the data has not been decoded and displayed,    */
                  /* then just display the raw data                     */
                  if((DeviceInfo == NULL) || (CallbackParameter == 0))
                  {
                     PRINTF("   Data:            { ");
                     for(Index = 0; Index < (ValueLength - 1); Index++)
                        PRINTF("0x%02x, ", *Value + Index);

                     PRINTF("0x%02x }\n", *Value + Index);
                  }
               }
            }
            else
               PRINTF("\nError - Null Read Response Data.\n");
            break;
         case etGATT_Client_Exchange_MTU_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data)
            {
               PRINTF("\nExchange MTU Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   MTU:             %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ServerMTU);
            }
            else
               PRINTF("\nError - Null Write Response Data.\n");
            break;
         case etGATT_Client_Write_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data)
            {
               PRINTF("\nWrite Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Bytes Written:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Write_Response_Data->BytesWritten);

               /* If we know about this device and a callback parameter */
               /* exists, then check if we know what write response this*/
               /* is.                                                   */
               if(((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) != NULL) && (CallbackParameter != 0))
               {
                  if(CallbackParameter == DeviceInfo->ClientInfo.Current_Time)
                  {
                     PRINTF("\nWrite Current Time Complete.\n");
                  }
                  else
                  {
                     if(CallbackParameter == DeviceInfo->ClientInfo.Current_Time_Client_Configuration)
                     {
                        PRINTF("\nWrite Current Time CC Complete.\n");
                     }
                     else
                     {
                        if(CallbackParameter == DeviceInfo->ClientInfo.Local_Time_Information)
                        {
                           PRINTF("\nWrite Local Time Information Complete.\n");
                        }
                        else
                        {
                           if(CallbackParameter == DeviceInfo->ClientInfo.Reference_Time_Information)
                           {
                              PRINTF("\nWrite Reference Time Information Complete.\n");
                           }
                        }
                     }
                  }
               }
            }
            else
               PRINTF("\nError - Null Write Response Data.\n");
            break;
         default:
            break;
      }

      /* Print the command line prompt.                                 */
      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");

      PRINTF("GATT Callback Data: Event_Data = NULL.\n");

      DisplayPrompt();
   }
}

   /* The following function is for an GATT Client Event Callback.  This*/
   /* function will be called whenever a GATT Response is received for a*/
   /* request that was made when this function was registered.  This    */
   /* function passes to the caller the GATT Client Event Data that     */
   /* occurred and the GATT Client Event Callback Parameter that was    */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the GATT Client Event Data ONLY in the context*/
   /* of this callback.  If the caller requires the Data for a longer   */
   /* period of time, then the callback function MUST copy the data into*/
   /* another Data Buffer.  This function is guaranteed NOT to be       */
   /* invoked more than once simultaneously for the specified installed */
   /* callback (i.e.  this function DOES NOT have be reentrant).  It    */
   /* Needs to be noted however, that if the same Callback is installed */
   /* more than once, then the callbacks will be called serially.       */
   /* Because of this, the processing in this function should be as     */
   /* efficient as possible.  It should also be noted that this function*/
   /* is called in the Thread Context of a Thread that the User does NOT*/
   /* own.  Therefore, processing in this function should be as         */
   /* efficient as possible (this argument holds anyway because another */
   /* GATT Event (Server/Client or Connection) will not be processed    */
   /* while this function call is outstanding).                         */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_ClientEventCallback_NDCS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;
   BoardStr_t    BoardStr;
   Word_t        Index;
   Byte_t       *Value;
   Word_t        ValueLength;
   int           Result;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Client_Event_Data))
   {
      /* Determine the event that occurred.                             */
      switch(GATT_Client_Event_Data->Event_Data_Type)
      {
         case etGATT_Client_Error_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data)
            {
               PRINTF("\nError Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Error Type:      %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)?"Response Error":"Response Timeout");

               /* Only print out the rest if it is valid.               */
               if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)
               {
                  PRINTF("   Request Opcode:  0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestOpCode);
                  PRINTF("   Request Handle:  0x%04X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestHandle);
                  PRINTF("   Error Code:      0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode);
                  PRINTF("   Error Msg:       %s.\n", ErrorCodeStr[GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode]);
               }
            }
            else
               PRINTF("Error - Null Error Response Data.\n");
            break;
         case etGATT_Client_Read_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data)
            {
               ValueLength = GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength;
               Value       = GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue;

               PRINTF("\nRead Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Data Length:     %u.\n", ValueLength);

               /* If we know about this device and a callback parameter */
               /* exists, then check if we know what read response this */
               /* is.                                                   */
               if(ValueLength != 0)
               {
                  if(((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) != NULL) && (CallbackParameter != 0))
                  {
                     if(CallbackParameter == DeviceInfo->NDCS_ClientInfo.Time_With_Dst)
                     {
                        if(ValueLength == NDCS_TIME_WITH_DST_SIZE)
                        {
                           if(!(Result = DecodeTimeWithOffset(ValueLength,Value)))
                           {
                              DisplayFunctionError("DecodeTimeWithOffset", Result);
                           }
                        }
                        else
                           PRINTF("\nError - Invalid length (%u) for TimeWithDST Offset response\n", ValueLength);
                     }
                  }

                  /* If the data has not been decoded and displayed,    */
                  /* then just display the raw data                     */
                  if((DeviceInfo == NULL) || (CallbackParameter == 0))
                  {
                     PRINTF("Data:            { ");
                     for(Index = 0; Index < (ValueLength - 1); Index++)
                        PRINTF("0x%02x, ", *Value + Index);

                     PRINTF("0x%02x }\n", *Value + Index);
                  }
               }
            }
            else
               PRINTF("\nError - Null Read Response Data.\n");
            break;
         default:
            break;
      }

      /* Print the command line prompt.                                 */
      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\nGATT Callback Data: Event_Data = NULL.\n");

      DisplayPrompt();
   }
}

   /* The following function is for an GATT Client Event Callback.  This*/
   /* function will be called whenever a GATT Response is received for a*/
   /* request that was made when this function was registered.  This    */
   /* function passes to the caller the GATT Client Event Data that     */
   /* occurred and the GATT Client Event Callback Parameter that was    */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the GATT Client Event Data ONLY in the context*/
   /* of this callback.  If the caller requires the Data for a longer   */
   /* period of time, then the callback function MUST copy the data into*/
   /* another Data Buffer.  This function is guaranteed NOT to be       */
   /* invoked more than once simultaneously for the specified installed */
   /* callback (i.e.  this function DOES NOT have be reentrant).  It    */
   /* Needs to be noted however, that if the same Callback is installed */
   /* more than once, then the callbacks will be called serially.       */
   /* Because of this, the processing in this function should be as     */
   /* efficient as possible.  It should also be noted that this function*/
   /* is called in the Thread Context of a Thread that the User does NOT*/
   /* own.  Therefore, processing in this function should be as         */
   /* efficient as possible (this argument holds anyway because another */
   /* GATT Event (Server/Client or Connection) will not be processed    */
   /* while this function call is outstanding).                         */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_ClientEventCallback_RTUS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;
   BoardStr_t    BoardStr;
   Word_t        Index;
   Byte_t       *Value;
   Word_t        ValueLength;
   int           Result;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Client_Event_Data))
   {
      /* Determine the event that occurred.                             */
      switch(GATT_Client_Event_Data->Event_Data_Type)
      {
         case etGATT_Client_Error_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data)
            {
               PRINTF("\nError Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Error Type:      %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)?"Response Error":"Response Timeout");

               /* Only print out the rest if it is valid.               */
               if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)
               {
                  PRINTF("   Request Opcode:  0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestOpCode);
                  PRINTF("   Request Handle:  0x%04X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestHandle);
                  PRINTF("   Error Code:      0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode);
               }
            }
            else
               PRINTF("Error - Null Error Response Data.\n");
            break;
         case etGATT_Client_Read_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data)
            {
               ValueLength = GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength;
               Value       = GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue;

               PRINTF("\nRead Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Data Length:     %u.\n", ValueLength);

               /* If we know about this device and a callback parameter */
               /* exists, then check if we know what read response this */
               /* is.                                                   */
               if(ValueLength != 0)
               {
                  if(((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) != NULL) && (CallbackParameter != 0))
                  {
                     if(CallbackParameter == DeviceInfo->RTUS_ClientInfo.Time_Update_State)
                     {
                        if(ValueLength == RTUS_TIME_UPDATE_STATE_SIZE)
                        {
                           if(!(Result = DecodeTimeUpdateState(ValueLength,Value)))
                           {
                              DisplayFunctionError("DecodeTimeUpdateState", Result);
                           }
                        }
                        else
                           PRINTF("\nError - Invalid length (%u) for Time Update State response\n", ValueLength);
                     }
                  }

                  /* If the data has not been decoded and displayed,    */
                  /* then just display the raw data                     */
                  if((DeviceInfo == NULL) || (CallbackParameter == 0))
                  {
                     PRINTF("   Data:            { ");
                     for(Index = 0; Index < (ValueLength - 1); Index++)
                        PRINTF("0x%02x, ", *Value + Index);

                     PRINTF("0x%02x }\n", *Value + Index);
                  }
               }
            }
            else
               PRINTF("\nError - Null Read Response Data.\n");
            break;
         default:
            break;
      }

      /* Print the command line prompt.                                 */
      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");

      PRINTF("GATT Callback Data: Event_Data = NULL.\n");

      DisplayPrompt();
   }
}

   /* The following function is for an GATT Client Event Callback.  This*/
   /* function will be called whenever a GATT Response is received for a*/
   /* request that was made when this function was registered.  This    */
   /* function passes to the caller the GATT Client Event Data that     */
   /* occurred and the GATT Client Event Callback Parameter that was    */
   /* specified when this Callback was installed.  The caller is free to*/
   /* use the contents of the GATT Client Event Data ONLY in the context*/
   /* of this callback.  If the caller requires the Data for a longer   */
   /* period of time, then the callback function MUST copy the data into*/
   /* another Data Buffer.  This function is guaranteed NOT to be       */
   /* invoked more than once simultaneously for the specified installed */
   /* callback (i.e.  this function DOES NOT have be reentrant).  It    */
   /* Needs to be noted however, that if the same Callback is installed */
   /* more than once, then the callbacks will be called serially.       */
   /* Because of this, the processing in this function should be as     */
   /* efficient as possible.  It should also be noted that this function*/
   /* is called in the Thread Context of a Thread that the User does NOT*/
   /* own.  Therefore, processing in this function should be as         */
   /* efficient as possible (this argument holds anyway because another */
   /* GATT Event (Server/Client or Connection) will not be processed    */
   /* while this function call is outstanding).                         */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_ClientEventCallback_GAPS(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter)
{
   char         *NameBuffer;
   Word_t        Appearance;
   BoardStr_t    BoardStr;
   DeviceInfo_t *DeviceInfo;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Client_Event_Data))
   {
      /* Determine the event that occurred.                             */
      switch(GATT_Client_Event_Data->Event_Data_Type)
      {
         case etGATT_Client_Error_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data)
            {
               PRINTF("\nError Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionID);
               PRINTF("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->TransactionID);
               PRINTF("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               PRINTF("   BD_ADDR:         %s.\n", BoardStr);
               PRINTF("   Error Type:      %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)?"Response Error":"Response Timeout");

               /* Only print out the rest if it is valid.               */
               if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)
               {
                  PRINTF("   Request Opcode:  0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestOpCode);
                  PRINTF("   Request Handle:  0x%04X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestHandle);
                  PRINTF("   Error Code:      0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode);
                  PRINTF("   Error Msg:       %s.\n", ErrorCodeStr[GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode]);
               }
            }
            else
               PRINTF("Error - Null Error Response Data.\n");
            break;
         case etGATT_Client_Read_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data)
            {
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) != NULL)
               {
                  if((Word_t)CallbackParameter == DeviceInfo->GAPSClientInfo.DeviceNameHandle)
                  {
                     /* Display the remote device name.                 */
                     if((NameBuffer = (char *)BTPS_AllocateMemory(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength+1)) != NULL)
                     {
                        BTPS_MemInitialize(NameBuffer, 0, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength+1);
                        BTPS_MemCopy(NameBuffer, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);

                        PRINTF("\nRemote Device Name: %s.\n", NameBuffer);

                        BTPS_FreeMemory(NameBuffer);
                     }
                  }
                  else
                  {
                     if((Word_t)CallbackParameter == DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle)
                     {
                        if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength == GAP_DEVICE_APPEARENCE_VALUE_LENGTH)
                        {
                           Appearance = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue);
                           if(AppearanceToString(Appearance, &NameBuffer))
                              PRINTF("\nRemote Device Appearance: %s(%u).\n", NameBuffer, Appearance);
                           else
                              PRINTF("\nRemote Device Appearance: Unknown(%u).\n", Appearance);
                        }
                        else
                           PRINTF("Invalid Remote Appearance Value Length %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);
                     }
                  }
               }
            }
            else
               PRINTF("\nError - Null Read Response Data.\n");
            break;
         default:
            break;
      }

      /* Print the command line prompt.                                 */
      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");

      PRINTF("GATT Callback Data: Event_Data = NULL.\n");

      DisplayPrompt();
   }
}

   /* The following function is for an GATT Connection Event Callback.  */
   /* This function is called for GATT Connection Events that occur on  */
   /* the specified Bluetooth Stack.  This function passes to the caller*/
   /* the GATT Connection Event Data that occurred and the GATT         */
   /* Connection Event Callback Parameter that was specified when this  */
   /* Callback was installed.  The caller is free to use the contents of*/
   /* the GATT Client Event Data ONLY in the context of this callback.  */
   /* If the caller requires the Data for a longer period of time, then */
   /* the callback function MUST copy the data into another Data Buffer.*/
   /* This function is guaranteed NOT to be invoked more than once      */
   /* simultaneously for the specified installed callback (i.e.  this   */
   /* function DOES NOT have be reentrant).  It Needs to be noted       */
   /* however, that if the same Callback is installed more than once,   */
   /* then the callbacks will be called serially.  Because of this, the */
   /* processing in this function should be as efficient as possible.   */
   /* It should also be noted that this function is called in the Thread*/
   /* Context of a Thread that the User does NOT own.  Therefore,       */
   /* processing in this function should be as efficient as possible    */
   /* (this argument holds anyway because another GATT Event            */
   /* (Server/Client or Connection) will not be processed while this    */
   /* function call is outstanding).                                    */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;
   BoardStr_t    BoardStr;
   int           Result;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Connection_Event_Data))
   {
      /* Determine the Connection Event that occurred.                  */
      switch(GATT_Connection_Event_Data->Event_Data_Type)
      {
         case etGATT_Connection_Device_Connection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data)
            {
               /* Save the Connection ID for later use.                 */
               ConnectionID = GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID;

               PRINTF("\netGATT_Connection_Device_Connection with size %u: \n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID);
               PRINTF("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:   %s.\n", BoardStr);
               PRINTF("   Connection MTU:  %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU);

               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice)) != NULL)
               {
                  if(LocalDeviceIsMaster)
                  {
                     /* Attempt to update the MTU to the maximum        */
                     /* supported.                                      */
                     GATT_Exchange_MTU_Request(BluetoothStackID, ConnectionID, BTPS_CONFIGURATION_GATT_DEFAULT_MAXIMUM_SUPPORTED_MTU_SIZE, GATT_ClientEventCallback_CTS, 0);
                  }
               }
            }
            else
               PRINTF("Error - Null Connection Data.\n");
            break;
         case etGATT_Connection_Device_Disconnection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data)
            {
               /* Clear the Connection ID.                              */
               ConnectionID = 0;

               PRINTF("\netGATT_Connection_Device_Disconnection with size %u: \n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionID);
               PRINTF("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:   %s.\n", BoardStr);
            }
            else
               PRINTF("Error - Null Disconnection Data.\n");
            break;

         case etGATT_Connection_Server_Notification:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data)
            {
               PRINTF("\netGATT_Connection_Server_Notification with size %u: \n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:    %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->ConnectionID);
               PRINTF("   Connection Type:  %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:    %s.\n", BoardStr);
               PRINTF("   Attribute Handle: 0x%04X.\n", GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeHandle);
               PRINTF("   Attribute Length: %d.\n", GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength);

               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->RemoteDevice)) != NULL)
               {
                  if(DeviceInfo->ClientInfo.Current_Time == GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeHandle)
                  {
                     /* Decode and display the data.                    */
                     PRINTF("\nCurrent Time:\n");
                     if((Result = DecodeDisplayCurrentTime(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength, GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValue)) != 0)
                     {
                        DisplayFunctionError("DecodeDisplayCurrentTime", Result);
                     }
                  }
                  else
                     PRINTF("Error - Unknown Notification Attribute Handle.\n");
               }
               else
                  PRINTF("Error - Remote Server Unknown.\n");
            }
            else
               PRINTF("Error - Null Server Notification Data.\n");
            break;
         default:
            break;
      }

      /* Print the command line prompt.                                 */
      DisplayPrompt();
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");

      PRINTF("GATT Connection Callback Data: Event_Data = NULL.\n");

      DisplayPrompt();
   }
}

   /* The following function is for an GATT Discovery Event Callback.   */
   /* This function will be called whenever a GATT Service is discovered*/
   /* or a previously started service discovery process is completed.   */
   /* This function passes to the caller the GATT Discovery Event Data  */
   /* that occurred and the GATT Client Event Callback Parameter that   */
   /* was specified when this Callback was installed.  The caller is    */
   /* free to use the contents of the GATT Discovery Event Data ONLY in */
   /* the context of this callback.  If the caller requires the Data for*/
   /* a longer period of time, then the callback function MUST copy the */
   /* data into another Data Buffer.  This function is guaranteed NOT to*/
   /* be invoked more than once simultaneously for the specified        */
   /* installed callback (i.e.  this function DOES NOT have be          */
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another GATT Discovery Event will not be processed while this     */
   /* function call is outstanding).                                    */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_Service_Discovery_Event_Callback(unsigned int BluetoothStackID, GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;

   if((BluetoothStackID) && (GATT_Service_Discovery_Event_Data))
   {
      if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ConnectionBD_ADDR)) != NULL)
      {
         switch(GATT_Service_Discovery_Event_Data->Event_Data_Type)
         {
            case etGATT_Service_Discovery_Indication:
               /* Verify the event data.                                */
               if(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data)
               {
                  PRINTF("\n");
                  PRINTF("Service 0x%04X - 0x%04X, UUID: ", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.Service_Handle, GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.End_Group_Handle);
                  DisplayUUID(&(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.UUID));
                  PRINTF("\n");

                  if(((Service_Discovery_Type_t)CallbackParameter) == sdGAPS)
                  {
                     /* Attempt to populate the handles for the GAP     */
                     /* Service.                                        */
                        GAPSPopulateHandles(&(DeviceInfo->GAPSClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                  }

                  /* Check the service discovery type for CTS           */
                  if(((Service_Discovery_Type_t)CallbackParameter) == sdCTS)
                  {
                     /* Attempt to populate the handles for the CTP     */
                     /* Service.                                        */
                     CTSPopulateHandles(DeviceInfo,  GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                  }
                  if(((Service_Discovery_Type_t)CallbackParameter) == sdNDCS)
                  {
                     /* Attempt to populate the handles for the NDCS    */
                     /* Service.                                        */
                     NDCSPopulateHandles(DeviceInfo, GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                  }
                  if(((Service_Discovery_Type_t)CallbackParameter) == sdRTUS)
                  {
                     /* Attempt to populate the handles for the RTUS    */
                     /* Service.                                        */
                     RTUSPopulateHandles(DeviceInfo, GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                  }
               }
               break;
            case etGATT_Service_Discovery_Complete:
               /* Verify the event data.                                */
               if(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data)
               {
                  PRINTF("\n");
                  PRINTF("Service Discovery Operation Complete, Status 0x%02X.\n", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data->Status);

                  /* Flag that no service discovery operation is        */
                  /* outstanding for this device.                       */
                  DeviceInfo->Flags &= ~DEVICE_INFO_FLAGS_SERVICE_DISCOVERY_OUTSTANDING;

                  /* Check the service discovery type for CTS           */
                  if(((Service_Discovery_Type_t)CallbackParameter) == sdCTS)
                  {
                     /* Flag that service discovery has been performed  */
                     /* on for this connection.                         */
                     DeviceInfo->Flags |= DEVICE_INFO_FLAGS_CTS_SERVICE_DISCOVERY_COMPLETE;

                     /* Print a summary of what descriptor were found   */
                     PRINTF("\nCTP Service Discovery Summary\n");
                     PRINTF("   Current Time:               %s\n", (DeviceInfo->ClientInfo.Current_Time                      ? "Supported" : "Not Supported"));
                     PRINTF("   Current Time CC:            %s\n", (DeviceInfo->ClientInfo.Current_Time_Client_Configuration ? "Supported" : "Not Supported"));
                     PRINTF("   Local Time Information:     %s\n", (DeviceInfo->ClientInfo.Local_Time_Information            ? "Supported" : "Not Supported"));
                     PRINTF("   Reference Time Information: %s\n", (DeviceInfo->ClientInfo.Reference_Time_Information        ? "Supported" : "Not Supported"));
                  }

                  if(((Service_Discovery_Type_t)CallbackParameter) == sdNDCS)
                  {
                     /* Flag that service discovery has been performed  */
                     /* on for this connection.                         */
                     DeviceInfo->Flags |= DEVICE_INFO_FLAGS_NDCS_SERVICE_DISCOVERY_COMPLETE;

                     /* Print a summary of what descriptor were found   */
                     PRINTF("\nNDCS Service Discovery Summary\n");
                     PRINTF("   Time With Dst Offset: %s\n", (DeviceInfo->NDCS_ClientInfo.Time_With_Dst ? "Supported" : "Not Supported"));
                  }

                  if(((Service_Discovery_Type_t)CallbackParameter) == sdRTUS)
                  {
                     /* Flag that service discovery has been performed  */
                     /* on for this connection.                         */
                     DeviceInfo->Flags |= DEVICE_INFO_FLAGS_RTUS_SERVICE_DISCOVERY_COMPLETE;

                     /* Print a summary of what descriptor were found   */
                     PRINTF("\nRTUS Service Discovery Summary\n");
                     PRINTF("   Time Update State:         %s\n", (DeviceInfo->RTUS_ClientInfo.Time_Update_State ? "Supported" : "Not Supported"));
                     PRINTF("   Time Update Control Point: %s\n", (DeviceInfo->RTUS_ClientInfo.Control_Point     ? "Supported" : "Not Supported"));

                  }
               }
               break;
         }

         DisplayPrompt();
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");

      PRINTF("GATT Callback Data: Event_Data = NULL.\n");

      DisplayPrompt();
   }
}

extern void user_pre_init(void);

static void RubyTIP_entry(ULONG which_thread)
{
   int                      Result;
   qcom_uart_para           com_uart_cfg;
   HCI_DriverInformation_t  HCI_DriverInformation;
   VendParams_t             VendParams;

#ifdef SDK_50

   A_UINT16                 trim_value;
   A_UINT8                  bd_addr[6];

#else

   A_UINT32                 BluetoothAddress;

#endif

   user_pre_init();

   qcom_uart_init();
   com_uart_cfg.BaudRate    = 115200;
   com_uart_cfg.number      = 8;
   com_uart_cfg.StopBits    = 1;
   com_uart_cfg.parity      = 0;
   com_uart_cfg.FlowControl = 0;
   qcom_set_uart_config(CONSOLE, &com_uart_cfg);
   console_fd = qcom_uart_open(CONSOLE);

   /* Reset the Bluetooth Controller.                                   */
   EnableBluetooth();

   if(console_fd >= 0)
   {
      /* Initialize the Vender Params.                                  */
      BTPS_MemInitialize(&VendParams, 0, sizeof(VendParams_t));

#ifdef SDK_50

      BTPS_MemInitialize(bd_addr, 0, 6);
      trim_value = 0;

      if(qcom_bluetooth_get_otp_mac(6, bd_addr) == A_OK)
         ASSIGN_BD_ADDR(VendParams.BD_ADDR, bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

      if(qcom_bluetooth_get_otp_trim(&trim_value) == A_OK)
         VendParams.TrimValue = trim_value;

#else

      VendParams.BD_ADDR.BD_ADDR3 = 0x5B;
      VendParams.BD_ADDR.BD_ADDR4 = 0x02;
      VendParams.BD_ADDR.BD_ADDR5 = 0x00;

      /* Prompt the user for a Bluetooth Address.                       */
      Result = GetBD_ADDR(console_fd, &BluetoothAddress);
      if(Result >= 0)
      {
         VendParams.BD_ADDR.BD_ADDR0 = (Byte_t)(BluetoothAddress & 0xFF);
         VendParams.BD_ADDR.BD_ADDR1 = (Byte_t)((BluetoothAddress >> 8)  & 0xFF);
         VendParams.BD_ADDR.BD_ADDR2 = (Byte_t)((BluetoothAddress >> 16) & 0xFF);
      }

#endif

      HCI_VS_SetParams(VendParams);

      /* The Transport selected was UART, setup the Driver Information  */
      /* Structure to use UART has the HCI Transport.                   */
      HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, 115200, cpUART_RTS_CTS);

      /* Try to Open the stack and check if it was successful.          */
      if(!OpenStack(&HCI_DriverInformation))
      {
         /* The stack was opened successfully.  Now set some defaults.  */

         /* First, attempt to set the Device to be Connectable.         */
         Result = SetConnect();

         /* Next, check to see if the Device was successfully made      */
         /* Connectable.                                                */
         if(!Result)
         {
            /* Now that the device is Connectable attempt to make it    */
            /* Discoverable.                                            */
            Result = SetDisc();

            /* Next, check to see if the Device was successfully made   */
            /* Discoverable.                                            */
            if(!Result)
            {
               /* Now that the device is discoverable attempt to make it*/
               /* pairable.                                             */
               Result = SetPairable();

               if(!Result)
               {
                  /* Start the User Interface.                          */
                  UserInterface();
               }
            }
         }

         /* Close the Bluetooth Stack.                                  */
         CloseStack();
      }

      /* Place the Bluetooth Controller in Reset.                       */
      DisableBluetooth();
   }
}

void user_main(void)
{
   char *Ptr;

   tx_byte_pool_create(&pool, "ruby app demo pool", TX_POOL_CREATE_DYNAMIC, BYTE_POOL_SIZE);
   tx_byte_allocate(&pool, (void **) &Ptr, PSEUDO_HOST_STACK_SIZE, TX_NO_WAIT);
   tx_thread_create(&host_thread, "ruby tip demo thread", RubyTIP_entry, 0, Ptr, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);

   cdr_threadx_thread_init();
}

