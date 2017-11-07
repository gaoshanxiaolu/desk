/*****< rubyspp_spple.c >******************************************************/
/*      Copyright 2012 - 2015 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  RUBYSPP_SPPLE - Ruby Bluetooth SPP and SPP Emulation using GATT (LE)      */
/*                   application.                                             */
/*                                                                            */
/*  Author:  Tim Thomas                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   04/16/15  Tim Thomas     Initial creation.                               */
/******************************************************************************/
#include "RubySPP_SPPLE.h"     /* Main Application Prototypes and Constants.  */
#include "SS1BTPS.h"           /* Main SS1 BT Stack Header.                   */
#include "SS1BTGAT.h"          /* Main SS1 GATT Header.                       */
#include "SS1BTGAP.h"          /* Main SS1 GAP Service Header.                */
#include "SS1BTDIS.h"          /* Main SS1 DIS Service Header.                */
#include "BTPSVEND.h"
#include "HAL.h"
#include "qcom_uart.h"
#include "threadxdmn_api.h"

#define PRINTF                                qcom_printf

#define CONSOLE                               "UART0"

#define BYTE_POOL_SIZE                      (5*1024)
#define PSEUDO_HOST_STACK_SIZE            (4 * 1024)

   /* MACRO used to eliminate unreferenced variable warnings.           */
#define UNREFERENCED_PARAM(_p)                      ((void)(_p))


#define MAX_SUPPORTED_COMMANDS                     (64)  /* Denotes the       */
                                                         /* maximum number of */
                                                         /* User Commands that*/
                                                         /* are supported by  */
                                                         /* this application. */

#define MAX_COMMAND_LENGTH                         (64)  /* Denotes the max   */
                                                         /* buffer size used  */
                                                         /* for user commands */
                                                         /* input via the     */
                                                         /* User Interface.   */

#define MAX_NUM_OF_PARAMETERS                       (5)  /* Denotes the max   */
                                                         /* number of         */
                                                         /* parameters a      */
                                                         /* command can have. */

#define MAX_INQUIRY_RESULTS                         (10)  /* Denotes the max  */
                                                         /* number of inquiry */
                                                         /* results.          */

#define MAX_SUPPORTED_LINK_KEYS                     (6)  /* Max supported Link*/
                                                         /* keys.             */

#define MAX_LE_CONNECTIONS                          (6)  /* Denotes the max   */
                                                         /* number of LE      */
                                                         /* connections that  */
                                                         /* are allowed at    */
                                                         /* the same time.    */

#define MAX_SIMULTANEOUS_SPP_PORTS                  (1) /* Maximum SPP Ports  */
                                                        /* that we support.   */

#define SPP_PERFORM_MASTER_ROLE_SWITCH              (1) /* Defines if TRUE    */
                                                        /* that a role switch */
                                                        /* should be performed*/
                                                        /* for all SPP        */
                                                        /* connections.       */

#define MAXIMUM_SPP_BUFFER_SIZE                    (64) /* Maximum size of the*/
                                                        /* buffer used in     */
                                                        /* raw mode.          */

#define DEFAULT_LE_IO_CAPABILITY   (licNoInputNoOutput)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with LE      */
                                                         /* Pairing.          */

#define DEFAULT_LE_MITM_PROTECTION              (TRUE)   /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with LE Pairing.  */

#define DEFAULT_IO_CAPABILITY          (icDisplayYesNo)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define DEFAULT_MITM_PROTECTION                  (TRUE)  /* Denotes the       */
                                                         /* default value used*/
                                                         /* for Man in the    */
                                                         /* Middle (MITM)     */
                                                         /* protection used   */
                                                         /* with Secure Simple*/
                                                         /* Pairing.          */

#define SPPLE_DATA_BUFFER_LENGTH  (ATT_PROTOCOL_MTU_MAXIMUM-3)
                                                         /* Defines the length*/
                                                         /* of a SPPLE Data   */
                                                         /* Buffer.           */

#define SPPLE_DATA_CREDITS        (SPPLE_DATA_BUFFER_LENGTH*1) /* Defines the */
                                                         /* number of credits */
                                                         /* in an SPPLE Buffer*/

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

#define EXIT_MODE                                  (-11) /* Flags exit from   */
                                                         /* any Mode.         */

#define INDENT_LENGTH                                 3  /* Denotes the number*/
                                                         /* of character      */
                                                         /* spaces to be used */
                                                         /* for indenting when*/
                                                         /* displaying SDP    */
                                                         /* Data Elements.    */

   /* The following MACRO is used to convert an ASCII character into the*/
   /* equivalent decimal value.  The MACRO converts lower case          */
   /* characters to upper case before the conversion.                   */
#define ToInt(_x)                                  (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))

   /* Determine the Name we will use for this compilation.              */
#define LE_DEMO_DEVICE_NAME                        "RubySPP_SPPLE"

   /* Following converts a Sniff Parameter in Milliseconds to frames.   */
#define MILLISECONDS_TO_BASEBAND_SLOTS(_x)         ((_x) / (0.625))

   /* The following macro calculates the best fit MTU size from the MTU */
   /* that is available.  This takes into account the 4 byte L2CAP      */
   /* Header and the 3 Byte ATT header and have them fit into a integral*/
   /* number of full packets.                                           */
#define BEST_FIT_NOTIFICATION(_x)                  ((((_x)/27)*27)-7)

   /* The following represent the possible values of UI_Mode variable.  */
#define UI_MODE_SELECT         (0)
#define UI_MODE_IS_SPP         (1)
#define UI_MODE_IS_SPPLE       (2)
#define UI_MODE_IS_INVALID     (-1)

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

   /* The following structure holds the connection parameters in Raw    */
   /* Data format.                                                      */
typedef struct Connect_Params_t
{
   Word_t ConnectIntMin;
   Word_t ConnectIntMax;
   Word_t SlaveLatency;
   Word_t MinConnectLength;
   Word_t MaxConnectLength;
   Word_t SupervisionTO;
} Connect_Params_t;

   /* The following structure is used to hold the valid range values of */
   /* a Connect Parameter value.                                        */
typedef struct _tagRange_t
{
   Word_t Low;
   Word_t High;
} Range_t;

   /* The following structure holds status information about a send     */
   /* process.                                                          */
typedef struct _tagSend_Info_t
{
   Boolean_t    BufferFull;
   DWord_t      BytesToSend;
   DWord_t      BytesSent;
} Send_Info_t;

   /* The following structure is used to track the sending and receiving*/
   /* of data for the throughput test.                                  */
typedef struct _tagXferInfo_t
{
   DWord_t        FirstPacketSize;
   DWord_t        TxCount;
   DWord_t        RxCount;
   Boolean_t      SeenFirst;
   TimeStamp_t    FirstTime;
   TimeStamp_t    LastTime;
} XferInfo_t;

   /* The following defines the structure that is used to hold          */
   /* information about all open SPP Ports.                             */
typedef struct SPP_Context_Info_t
{
   unsigned int  LocalSerialPortID;
   unsigned int  ServerPortNumber;
   Word_t        Connection_Handle;
   BD_ADDR_t     BD_ADDR;
   DWord_t       SPPServerSDPHandle;
   Boolean_t     Connected;
   Send_Info_t   SendInfo;
   unsigned int  BufferLength;
   unsigned char Buffer[MAXIMUM_SPP_BUFFER_SIZE];
} SPP_Context_Info_t;

   /* The following defines the format of a SPPLE Data Buffer.          */
typedef struct _tagSPPLE_Data_Buffer_t
{
   unsigned int  InIndex;
   unsigned int  OutIndex;
   unsigned int  BytesFree;
   unsigned int  BufferSize;
   Byte_t        Buffer[SPPLE_DATA_CREDITS];
} SPPLE_Data_Buffer_t;

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

#define DIS_MAN_NAME_HANDLE_OFFSET                 0
#define DIS_MODEL_NUM_HANDLE_OFFSET                1
#define DIS_SERIAL_NUM_HANDLE_OFFSET               2
#define DIS_HARDWARE_REV_HANDLE_OFFSET             3
#define DIS_FIRMWARE_REV_HANDLE_OFFSET             4
#define DIS_SOFTWARE_REV_HANDLE_OFFSET             5
#define DIS_SYSTEM_ID_HANDLE_OFFSET                6
#define DIS_IEEE_CERT_HANDLE_OFFSET                7
#define DIS_PNP_ID_HANDLE_OFFSET                   8

   /* The following structure holds information on known Device         */
   /* Information Service handles.                                      */
typedef struct _tagDIS_Client_Info_t
{
   Word_t ManufacturerNameHandle;
   Word_t ModelNumberHandle;
   Word_t SerialNumberHandle;
   Word_t HardwareRevisionHandle;
   Word_t FirmwareRevisionHandle;
   Word_t SoftwareRevisionHandle;
   Word_t SystemIDHandle;
   Word_t IEEE11073CertHandle;
   Word_t PnPIDHandle;
} DIS_Client_Info_t;

   /* Defines the bit mask flags that may be set in the DeviceInfo_t    */
   /* structure.                                                        */
#define DEVICE_INFO_FLAG_LTK_VALID                         0x0001
#define DEVICE_INFO_FLAG_LETP_CLIENT                       0x0002
#define DEVICE_INFO_FLAG_LETP_SERVER                       0x0004
#define DEVICE_INFO_FLAG_SPPLE_SERVER                      0x0008
#define DEVICE_INFO_FLAG_SPPLE_CLIENT                      0x0010
#define DEVICE_INFO_FLAG_LINK_ENCRYPTED                    0x0020
#define DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING     0x0040
#define DEVICE_INFO_FLAG_SERVICE_DISCOVERY_DIS             0x0080
#define DEVICE_INFO_FLAG_SERVICE_DISCOVERY_GAPS            0x0100
#define DEVICE_INFO_FLAG_SERVICE_DISCOVERY_SPPLE           0x0200
#define DEVICE_INFO_FLAG_SERVICE_DISCOVERY_LETP            0x0400
#define DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE            0x07C0
#define DEVICE_INFO_FLAG_LETP_TIMING_ACTIVE                0x0800
#define DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE               0x1000
#define DEVICE_INFO_FLAG_LETP_INTERVAL_EXPIRED             0x2000

   /* The following structure is used to hold a list of information     */
   /* on all paired devices.                                            */
typedef struct _tagDeviceInfo_t
{
   Word_t                   Flags;
   Byte_t                   EncryptionKeySize;
   GAP_LE_Address_Type_t    ConnectionAddressType;
   BD_ADDR_t                ConnectionBD_ADDR;
   Word_t                   ServerMTU;
   Long_Term_Key_t          LTK;
   Random_Number_t          Rand;
   Word_t                   EDIV;
   XferInfo_t               XferInfo;
   unsigned int             TransmitCredits;
   unsigned int             LETP_TimerID;
   SPPLE_Data_Buffer_t      ReceiveBuffer;
   int                      DISHandleIndex;
   DIS_Client_Info_t        DISClientInfo;
   GAPS_Client_Info_t       GAPSClientInfo;
   SPPLE_Client_Info_t      ClientInfo;
   SPPLE_Server_Info_t      ServerInfo;
   LETP_Client_Info_t       TPClientInfo;
   LETP_Server_Info_t       TPServerInfo;
   struct _tagDeviceInfo_t *NextDeviceInfoInfoPtr;
} DeviceInfo_t;

#define DEVICE_INFO_DATA_SIZE                               (sizeof(DeviceInfo_t))

#define MAX_SCAN_ENTRIES                                    10
#define MAX_NAME_SIZE                                       24

#define SCAN_INFO_FLAG_ADVERTISING_PACKET_PROCESSED         0x01
#define SCAN_INFO_FLAG_SCAN_RESPONSE_PACKET_PROCESSED       0x02

typedef struct _tagScanInfo_t
{
   Byte_t                Flags;
   BD_ADDR_t             BD_ADDR;
   char                  NameBuffer[MAX_NAME_SIZE+1];
   GAP_LE_Address_Type_t AddressType;
} ScanInfo_t;

#define SCAN_INFO_DATA_SIZE                              (sizeof(ScanInfo_t))

   /* The following structure is used to hold all of the SPPLE related  */
   /* information pertaining to buffers and credits.                    */
typedef struct _tagSPPLE_Buffer_Info_t
{
   unsigned int        TransmitCredits;
   Word_t              QueuedCredits;
   SPPLE_Data_Buffer_t ReceiveBuffer;
} SPPLE_Buffer_Info_t;

   /* The following structure is used to hold information on a connected*/
   /* LE Device.                                                        */
typedef struct _tagLE_Context_Info_t
{
   BD_ADDR_t           ConnectionBD_ADDR;
   unsigned int        ConnectionID;
   SPPLE_Buffer_Info_t SPPLEBufferInfo;
   Boolean_t           BufferFull;
   unsigned int        BytesToSend;
   unsigned int        BytesSent;
   unsigned int        DataStrIndex;
}  LE_Context_Info_t;

   /* The following type definition represents the container type which */
   /* holds the mapping between Profile UUIDs and Profile Names (UUID   */
   /* <-> Name).                                                        */
typedef struct _tagUUIDInfo_t
{
   char       *Name;
   UUID_128_t  UUID;
} UUIDInfo_t;

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

static int                 UI_Mode;                 /* Holds the UI Mode.              */

static Byte_t              SPPLEBuffer[SPPLE_DATA_BUFFER_LENGTH+1];  /* Buffer that is */
                                                    /* used for Sending/Receiving      */
                                                    /* SPPLE Service Data.             */

static unsigned int        LETPServiceID;           /* The following holds the LE TP   */
                                                    /* Service ID that is returned from*/
                                                    /* GATT_Register_Service().        */

static unsigned int        SPPLEServiceID;          /* The following holds the SPP LE  */
                                                    /* Service ID that is returned from*/
                                                    /* GATT_Register_Service().        */

static unsigned int        DISInstanceID;           /* Holds the Instance ID for the   */
                                                    /* DIS Service.                    */

static unsigned int        GAPSInstanceID;          /* Holds the Instance ID for the   */
                                                    /* GAP Service.                    */

static GAPLE_Parameters_t  LE_Parameters;           /* Holds GAP Parameters like       */
                                                    /* Discoverability, Connectability */
                                                    /* Modes.                          */

static DeviceInfo_t       *DeviceInfoList;          /* Holds the list head for the     */
                                                    /* device info list.               */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static Boolean_t           LocalDeviceIsMaster;     /* Boolean that tells if the local */
                                                    /* device is the master of the     */
                                                    /* current connection.             */

static BD_ADDR_t           CurrentLERemoteBD_ADDR;  /* Variable which holds the        */
                                                    /* current LE BD_ADDR of the device*/
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static BD_ADDR_t           CurrentCBRemoteBD_ADDR;  /* Variable which holds the        */
                                                    /* current CB BD_ADDR of the device*/
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static BD_ADDR_t           InquiryResultList[MAX_INQUIRY_RESULTS]; /* Variable which   */
                                                    /* contains the inquiry result     */
                                                    /* received from the most recently */
                                                    /* preformed inquiry.              */

static unsigned int        NumberofValidResponses;  /* Variable which holds the number */
                                                    /* of valid inquiry results within */
                                                    /* the inquiry results array.      */

static LinkKeyInfo_t       LinkKeyInfo[MAX_SUPPORTED_LINK_KEYS]; /* Variable holds     */
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */

static GAP_IO_Capability_t IOCapability;            /* Variable which holds the        */
                                                    /* current I/O Capabilities that   */
                                                    /* are to be used for Secure Simple*/
                                                    /* Pairing.                        */

static Boolean_t           OOBSupport;              /* Variable which flags whether    */
                                                    /* or not Out of Band Secure Simple*/
                                                    /* Pairing exchange is supported.  */

static Boolean_t           MITMProtection;          /* Variable which flags whether or */
                                                    /* not Man in the Middle (MITM)    */
                                                    /* protection is to be requested   */
                                                    /* during a Secure Simple Pairing  */
                                                    /* procedure.                      */

static Boolean_t           DisplayRawData;          /* Variable which flags whether or */
                                                    /* not the application is to       */
                                                    /* simply display the Raw Data     */
                                                    /* when it is received.            */

static Boolean_t           AutomaticReadActive;     /* Variable which flags whether or */
                                                    /* not the application is to       */
                                                    /* automatically read all data     */
                                                    /* as it is received.              */

static unsigned int        NumberCommands;          /* Variable which is used to hold  */
                                                    /* the number of Commands that are */
                                                    /* supported by this application.  */
                                                    /* Commands are added individually.*/

static CommandTable_t      CommandTable[MAX_SUPPORTED_COMMANDS]; /* Variable which is  */
                                                    /* used to hold the actual Commands*/
                                                    /* that are supported by this      */
                                                    /* application.                    */

static SPP_Context_Info_t  SPPContextInfo[MAX_SIMULTANEOUS_SPP_PORTS];
                                                    /* Variable that contains          */
                                                    /* information about the current   */
                                                    /* open SPP Ports                  */

static LE_Context_Info_t   LEContextInfo[MAX_LE_CONNECTIONS]; /* Array that contains   */
                                                    /* the connection ID and BD_ADDR   */
                                                    /* of each connected device.       */

static Mutex_t             DeviceListMutex;
static int                 NumScanEntries;
static ScanInfo_t          ScanInfo[MAX_SCAN_ENTRIES];
static char                DisBuf[DIS_MAXIMUM_SUPPORTED_STRING+1];
static Connect_Params_t    ConnectParams;
static Range_t             ConnectParamRange[] = {{8,4000}, {8,4000}, {0,499}, {0,40959}, {0,65535},{100,23000}};
static char               *DataStrPtr;
static int                 DataStrLength;
static DWord_t             console_fd;
static TX_THREAD           host_thread;
static TX_BYTE_POOL        pool;

   /* The following defines a data sequence that will be used to        */
   /* generate message data.                                            */
static BTPSCONST char DataStr[]  = "~!@#$%^&*()_+`1234567890-=:;\"'<>?,./@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]`abcdefghijklmnopqrstuvwxyz{|}<>\r\n~!@#$%^&*()_+`1234567890-=:;\"'<>?,./@ABCDEFGHIJKLMNOPQRSTUVWXYZ[\\]`abcdefghijklmnopqrstuvwxyz{|}<>\r\n";

#define DATA_STR_LENGTH                         (sizeof(DataStr)-1)

#define DATA_STR_HALF_LENGTH                    (DATA_STR_LENGTH >> 1)

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
};

#define NUMBER_GATT_ERROR_CODES  (sizeof(ErrorCodeStr)/sizeof(char *))

   /* The following array is used to map Device Appearance Values to    */
   /* strings.                                                          */
static GAPS_Device_Appearance_Mapping_t AppearanceMappings[] =
{
   {GAP_DEVICE_APPEARENCE_VALUE_UNKNOWN,                        "Unknown"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_PHONE,                  "Generic Phone"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_COMPUTER,               "Generic Computer"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_WATCH,                  "Generic Watch"},
   {GAP_DEVICE_APPEARENCE_VALUE_SPORTS_WATCH,                   "Sports Watch"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_CLOCK,                  "Generic Clock"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_DISPLAY,                "Generic Display"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_GENERIC_REMOTE_CONTROL, "Generic Remote Control"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_EYE_GLASSES,            "Eye Glasses"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_TAG,                    "Generic Tag"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_KEYRING,                "Generic Keyring"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_MEDIA_PLAYER,           "Generic Media Player"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_BARCODE_SCANNER,        "Generic Barcode Scanner"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_THERMOMETER,            "Generic Thermometer"},
   {GAP_DEVICE_APPEARENCE_VALUE_THERMOMETER_EAR,                "Ear Thermometer"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_HEART_RATE_SENSOR,      "Generic Heart Rate Sensor"},
   {GAP_DEVICE_APPEARENCE_VALUE_BELT_HEART_RATE_SENSOR,         "Belt Heart Rate Sensor"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_BLOOD_PRESSURE,         "Generic Blood Pressure"},
   {GAP_DEVICE_APPEARENCE_VALUE_BLOOD_PRESSURE_ARM,             "Blood Pressure: ARM"},
   {GAP_DEVICE_APPEARENCE_VALUE_BLOOD_PRESSURE_WRIST,           "Blood Pressure: Wrist"},
   {GAP_DEVICE_APPEARENCE_VALUE_HUMAN_INTERFACE_DEVICE,         "Human Interface Device"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_KEYBOARD,                   "HID Keyboard"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_MOUSE,                      "HID Mouse"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_JOYSTICK,                   "HID Joystick"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_GAMEPAD,                    "HID Gamepad"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_DIGITIZER_TABLET,           "HID Digitizer Tablet"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_CARD_READER,                "HID Card Reader"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_DIGITAL_PEN,                "HID Digitizer Pen"},
   {GAP_DEVICE_APPEARENCE_VALUE_HID_BARCODE_SCANNER,            "HID Bardcode Scanner"},
   {GAP_DEVICE_APPEARENCE_VALUE_GENERIC_GLUCOSE_METER,          "Generic Glucose Meter"}
};

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

static UUIDInfo_t UUIDTable[] =
{
   { "L2CAP",                 { 0x00, 0x00, 0x01, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Advanced Audio",        { 0x00, 0x00, 0x11, 0x0D, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "A/V Remote Control",    { 0x00, 0x00, 0x11, 0x0E, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Basic Imaging",         { 0x00, 0x00, 0x11, 0x1A, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Basic Printing",        { 0x00, 0x00, 0x11, 0x22, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Dial-up Networking",    { 0x00, 0x00, 0x11, 0x03, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "FAX",                   { 0x00, 0x00, 0x11, 0x11, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "File Transfer",         { 0x00, 0x00, 0x11, 0x06, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Hard Copy Cable Repl.", { 0x00, 0x00, 0x11, 0x25, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Health Device",         { 0x00, 0x00, 0x14, 0x00, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Headset",               { 0x00, 0x00, 0x11, 0x08, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Handsfree",             { 0x00, 0x00, 0x11, 0x1E, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "HID",                   { 0x00, 0x00, 0x11, 0x24, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "LAN Access",            { 0x00, 0x00, 0x11, 0x02, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Message Access",        { 0x00, 0x00, 0x11, 0x34, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Object Push",           { 0x00, 0x00, 0x11, 0x05, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Personal Area Network", { 0x00, 0x00, 0x00, 0x0F, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Phonebook Access",      { 0x00, 0x00, 0x11, 0x30, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "SIM Access",            { 0x00, 0x00, 0x11, 0x2D, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "Serial Port",           { 0x00, 0x00, 0x11, 0x01, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } },
   { "IrSYNC",                { 0x00, 0x00, 0x11, 0x04, 0x00, 0x00, 0x10, 0x00, 0x80, 0x00, 0x00, 0x80, 0x5F, 0x9B, 0x34, 0xFB } }
} ;

#define NUM_UUIDS                               (sizeof(UUIDTable)/sizeof(UUIDInfo_t))

   /*********************************************************************/
   /**                     SPPLE Service Table                         **/
   /*********************************************************************/

   /* The SPPLE Service Declaration UUID.                               */
static BTPSCONST GATT_Primary_Service_128_Entry_t SPPLE_Service_UUID =
{
   SPPLE_SERVICE_UUID_CONSTANT
};

   /* The Tx Characteristic Declaration.                                */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Tx_Declaration =
{
   GATT_CHARACTERISTIC_PROPERTIES_NOTIFY,
   SPPLE_TX_CHARACTERISTIC_UUID_CONSTANT
};

   /* The Tx Characteristic Value.                                      */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t  SPPLE_Tx_Value =
{
   SPPLE_TX_CHARACTERISTIC_UUID_CONSTANT,
   0,
   NULL
};

   /* The Tx Credits Characteristic Declaration.                        */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Tx_Credits_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_WRITE_WITHOUT_RESPONSE|GATT_CHARACTERISTIC_PROPERTIES_WRITE),
   SPPLE_TX_CREDITS_CHARACTERISTIC_UUID_CONSTANT
};

   /* The Tx Credits Characteristic Value.                              */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t SPPLE_Tx_Credits_Value =
{
   SPPLE_TX_CREDITS_CHARACTERISTIC_UUID_CONSTANT,
   0,
   NULL
};

   /* The SPPLE RX Characteristic Declaration.                          */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Rx_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_WRITE_WITHOUT_RESPONSE),
   SPPLE_RX_CHARACTERISTIC_UUID_CONSTANT
};

   /* The SPPLE RX Characteristic Value.                                */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t  SPPLE_Rx_Value =
{
   SPPLE_RX_CHARACTERISTIC_UUID_CONSTANT,
   0,
   NULL
};


   /* The SPPLE Rx Credits Characteristic Declaration.                  */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t SPPLE_Rx_Credits_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY),
   SPPLE_RX_CREDITS_CHARACTERISTIC_UUID_CONSTANT
};

   /* The SPPLE Rx Credits Characteristic Value.                        */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t SPPLE_Rx_Credits_Value =
{
   SPPLE_RX_CREDITS_CHARACTERISTIC_UUID_CONSTANT,
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

   /* The following defines the SPPLE service that is registered with   */
   /* the GATT_Register_Service function call.                          */
   /* * NOTE * This array will be registered with GATT in the call to   */
   /*          GATT_Register_Service.                                   */
BTPSCONST GATT_Service_Attribute_Entry_t SPPLE_Service[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,            (Byte_t *)&SPPLE_Service_UUID},                  //0
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Tx_Declaration},                //1
   {0,                                      aetCharacteristicValue128,       (Byte_t *)&SPPLE_Tx_Value},                      //2
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&Client_Characteristic_Configuration}, //3
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Tx_Credits_Declaration},        //4
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicValue128,       (Byte_t *)&SPPLE_Tx_Credits_Value},              //5
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Rx_Declaration},                //6
   {GATT_ATTRIBUTE_FLAGS_WRITABLE,          aetCharacteristicValue128,       (Byte_t *)&SPPLE_Rx_Value},                      //7
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&SPPLE_Rx_Credits_Declaration},        //8
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicValue128,       (Byte_t *)&SPPLE_Rx_Credits_Value},              //9
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&Client_Characteristic_Configuration}, //10
};

#define SPPLE_SERVICE_ATTRIBUTE_COUNT               (sizeof(SPPLE_Service)/sizeof(GATT_Service_Attribute_Entry_t))

#define SPPLE_SERVICE_ATTRIBUTE_COUNT               (sizeof(SPPLE_Service)/sizeof(GATT_Service_Attribute_Entry_t))

#define SPPLE_TX_CHARACTERISTIC_ATTRIBUTE_OFFSET               2
#define SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET           3
#define SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET       5
#define SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET               7
#define SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET       9
#define SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET   10

   /*********************************************************************/
   /**                    END OF SPPLE SERVICE TABLE                   **/
   /*********************************************************************/

   /*********************************************************************/
   /**                     LETP Service Table                          **/
   /*********************************************************************/

   /* The LETP Service Declaration UUID.                                */
static BTPSCONST GATT_Primary_Service_128_Entry_t LETP_Service_UUID =
{
   LETP_SERVICE_UUID_CONSTANT
};

   /* The LETP Tx Interval Characteristic Declaration.                  */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t LETP_Tx_Interval_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_READ|GATT_CHARACTERISTIC_PROPERTIES_WRITE),
   LETP_TX_INTERVAL_CHARACTERISTIC_UUID_CONSTANT
};

   /* The LETP Tx Interval Characteristic Value.                        */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t LETP_Tx_Interval_Value =
{
   LETP_TX_INTERVAL_CHARACTERISTIC_UUID_CONSTANT,
   0,
   NULL
};

   /* The LETP Tx Bulk Characteristic Declaration.                      */
static BTPSCONST GATT_Characteristic_Declaration_128_Entry_t LETP_Tx_Bulk_Declaration =
{
   (GATT_CHARACTERISTIC_PROPERTIES_WRITE|GATT_CHARACTERISTIC_PROPERTIES_NOTIFY),
   LETP_TX_BULK_CHARACTERISTIC_UUID_CONSTANT
};

   /* The LETP Tx Bulk Characteristic Value.                            */
static BTPSCONST GATT_Characteristic_Value_128_Entry_t LETP_Tx_Bulk_Value =
{
   LETP_TX_BULK_CHARACTERISTIC_UUID_CONSTANT,
   0,
   NULL
};

   /* The following defines the LETP service that is registered with the*/
   /* GATT_Register_Service function call.                              */
   /* * NOTE * This array will be registered with GATT in the call to   */
   /*          GATT_Register_Service.                                   */
BTPSCONST GATT_Service_Attribute_Entry_t LETP_Service[] =
{
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetPrimaryService128,            (Byte_t *)&LETP_Service_UUID},                   //0
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&LETP_Tx_Interval_Declaration},        //1
   {GATT_ATTRIBUTE_FLAGS_WRITABLE,          aetCharacteristicValue128,       (Byte_t *)&LETP_Tx_Interval_Value},              //2
   {GATT_ATTRIBUTE_FLAGS_READABLE,          aetCharacteristicDeclaration128, (Byte_t *)&LETP_Tx_Bulk_Declaration},            //3
   {GATT_ATTRIBUTE_FLAGS_WRITABLE,          aetCharacteristicValue128,       (Byte_t *)&LETP_Tx_Bulk_Value},                  //4
   {GATT_ATTRIBUTE_FLAGS_READABLE_WRITABLE, aetCharacteristicDescriptor16,   (Byte_t *)&Client_Characteristic_Configuration}, //5
};

#define LETP_SERVICE_ATTRIBUTE_COUNT               (sizeof(LETP_Service)/sizeof(GATT_Service_Attribute_Entry_t))

#define LETP_TX_INTERVAL_CHARACTERISTIC_ATTRIBUTE_OFFSET           2
#define LETP_TX_BULK_CHARACTERISTIC_ATTRIBUTE_OFFSET               4
#define LETP_TX_BULK_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET           5

   /*********************************************************************/
   /**                    END OF LETP SERVICE TABLE                    **/
   /*********************************************************************/

   /* Internal function prototypes.                                     */
static Boolean_t CreateNewDeviceInfoEntry(DeviceInfo_t **ListHead, GAP_LE_Address_Type_t ConnectionAddressType, BD_ADDR_t ConnectionBD_ADDR);
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);
static DeviceInfo_t *DeleteDeviceInfoEntry(DeviceInfo_t **ListHead, BD_ADDR_t BD_ADDR);
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree);
static void FreeDeviceInfoList(DeviceInfo_t **ListHead);
static int AddDeviceToScanResults(GAP_LE_Advertising_Report_Data_t *DeviceEntry);

static void UserInterface_Selection(void);
static unsigned long StringToUnsignedInteger(char *StringInteger);
static char *StringParser(char *String);
static int CommandParser(UserCommand_t *TempCommand, char *Input);
static int CommandInterpreter(UserCommand_t *TempCommand);
static int AddCommand(char *CommandName, CommandFunction_t CommandFunction);
static CommandFunction_t FindCommand(char *Command);
static void ClearCommands(void);

static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);

static int DisplayScanEntries(ParameterList_t *TempParam);
static void DisplayIOCapabilities(void);
static void DisplayClassOfDevice(Class_of_Device_t Class_of_Device);
static void DisplayKey(char *Prompt, int KeyLength, Byte_t *Key);
static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data);
static void DisplayPairingInformation(GAP_LE_Pairing_Capabilities_t Pairing_Capabilities);
static void DisplayUUID(GATT_UUID_t *UUID);
static void DisplayPrompt(void);
static void DisplayUsage(char *UsageString);
static void DisplayFunctionError(char *Function,int Status);
static void DisplayFunctionSuccess(char *Function);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation);
static int CloseStack(void);

static int SetDisc(void);
static int SetConnect(void);
static int SetPairable(void);

static void DumpAppearanceMappings(void);
static Boolean_t AppearanceToString(Word_t Appearance, char **String);
static Boolean_t AppearanceIndexToAppearance(unsigned int Index, Word_t *Appearance);

static void DISPopulateHandles(DIS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);
static void GAPSPopulateHandles(GAPS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);
static void SPPLEPopulateHandles(SPPLE_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);
static void LETPPopulateHandles(LETP_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo);

static void SetDataStrBuffer(Word_t Size);
static unsigned int AddDataToBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int DataLength, Byte_t *Data);
static unsigned int RemoveDataFromBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int BufferLength, Byte_t *Buffer);
static void InitializeBuffer(SPPLE_Data_Buffer_t *DataBuffer);

static int EnableDisableNotificationsIndications(unsigned int ConnectionID, Word_t ClientConfigurationHandle, Word_t ClientConfigurationValue, GATT_Client_Event_Callback_t ClientEventCallback);
static void SendSPPLEData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo);
static void SPPLESendCredits(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength);
static void SPPLEReceiveCreditEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int Credits);
static unsigned int SPPLESendData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data);
static void SPPLEDataIndicationEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data);
static int SPPLEReadData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int BufferLength, Byte_t *Buffer);

static int StartScan(unsigned int BluetoothStackID);
static int StopScan(unsigned int BluetoothStackID);

static int ConnectLEDevice(unsigned int BluetoothStackID, GAP_LE_Address_Type_t Address_Type, BD_ADDR_t BD_ADDR, Boolean_t UseWhiteList);
static int DisconnectLEDevice(unsigned int BluetoothStackID, BD_ADDR_t BD_ADDR);

static void ConfigureCapabilities(GAP_LE_Pairing_Capabilities_t *Capabilities);
static int SendPairingRequest(BD_ADDR_t BD_ADDR, Boolean_t ConnectionMaster);
static int SlavePairingRequestResponse(BD_ADDR_t BD_ADDR);
static int EncryptionInformationRequestResponse(BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information);
static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static int Select_SPP(ParameterList_t *TempParam);
static int Select_SPPLE(ParameterList_t *TempParam);
static int DisplayHelp(ParameterList_t *TempParam);
static int SetLEDiscoverabilityMode(ParameterList_t *TempParam);
static int SetLEConnectabilityMode(ParameterList_t *TempParam);
static int SetLEPairabilityMode(ParameterList_t *TempParam);
static int GetLocalAddress(ParameterList_t *TempParam);
static int ChangeLEPairingParameters(ParameterList_t *TempParam);
static int LEPassKeyResponse(ParameterList_t *TempParam);
static int AdvertiseLE(ParameterList_t *TempParam);
static int ScanLE(ParameterList_t *TempParam);
static int SetLEParams(ParameterList_t *TempParam);
static int ConnectLE(ParameterList_t *TempParam);
static int CancelConnectLE(ParameterList_t *TempParam);
static int DisconnectLE(ParameterList_t *TempParam);
static int PairLE(ParameterList_t *TempParam);
static int DiscoverDIS(ParameterList_t *TempParam);
static int DiscoverGAPS(ParameterList_t *TempParam);
static int ReadLocalName(ParameterList_t *TempParam);
static int SetLocalName(ParameterList_t *TempParam);
static int ReadRemoteName(ParameterList_t *TempParam);
static int ReadLocalAppearance(ParameterList_t *TempParam);
static int SetLocalAppearance(ParameterList_t *TempParam);
static int ReadRemoteAppearance(ParameterList_t *TempParam);
static int DiscoverLETP(ParameterList_t *TempParam);
static int RegisterLETP(ParameterList_t *TempParam);
static int UnregisterLETP(ParameterList_t *TempParam);
static int TimeLETP(ParameterList_t *TempParam);
static int SetLETPCount(ParameterList_t *TempParam);
static int SetLETPInterval(ParameterList_t *TempParam);
static int EnableLETP(ParameterList_t *TempParam);
static int DiscoverSPPLE(ParameterList_t *TempParam);
static int RegisterSPPLE(ParameterList_t *TempParam);
static int UnregisterSPPLE(ParameterList_t *TempParam);
static int ConfigureSPPLE(ParameterList_t *TempParam);
static int SendDataCommand(ParameterList_t *TempParam);
static int ReadDataCommand(ParameterList_t *TempParam);
static int DisplayRawModeData(ParameterList_t *TempParam);
static int AutomaticReadMode(ParameterList_t *TempParam);

static int Inquiry(ParameterList_t *TempParam);
static int DisplayDeviceList(ParameterList_t *TempParam);
static int SetCBDiscoverabilityMode(ParameterList_t *TempParam);
static int SetCBConnectabilityMode(ParameterList_t *TempParam);
static int SetCBPairabilityMode(ParameterList_t *TempParam);
static int ChangeSimplePairingParameters(ParameterList_t *TempParam);
static int Pair(ParameterList_t *TempParam);
static int EndPairing(ParameterList_t *TempParam);
static int PINCodeResponse(ParameterList_t *TempParam);
static int PassKeyResponse(ParameterList_t *TempParam);
static int UserConfirmationResponse(ParameterList_t *TempParam);
static int GetLocalName(ParameterList_t *TempParam);
static int SetClassOfDevice(ParameterList_t *TempParam);
static int GetClassOfDevice(ParameterList_t *TempParam);
static int GetRemoteName(ParameterList_t *TempParam);
static int ServiceDiscovery(ParameterList_t *TempParam);
static int OpenServer(ParameterList_t *TempParam);
static int CloseServer(ParameterList_t *TempParam);
static int OpenRemoteServer(ParameterList_t *TempParam);
static int CloseRemoteServer(ParameterList_t *TempParam);
static int Read(ParameterList_t *TempParam);
static int Write(ParameterList_t *TempParam);

static int FindSPPPortIndex(unsigned int SerialPortID);
static int FindSPPPortIndexByServerPortNumber(unsigned int ServerPortNumber);
static int FindSPPPortIndexByAddress(BD_ADDR_t BD_ADDR);
static int FindFreeSPPPortIndex(void);
static int ClosePortByNumber(unsigned int LocalServerPort);
static int FindFreeLEIndex(void);
static int FindLEIndexByAddress(BD_ADDR_t BD_ADDR);
static int FindLEIndexByConnectionID(unsigned int ConnectionID);
static int UpdateConnectionID(unsigned int ConnectionID, BD_ADDR_t BD_ADDR);
static void RemoveConnectionInfo(BD_ADDR_t BD_ADDR);

static void DisplaySDPAttributeResponse(SDP_Service_Attribute_Response_Data_t *SDPServiceAttributeResponse, unsigned int InitLevel);
static void DisplaySDPSearchAttributeResponse(SDP_Service_Search_Attribute_Response_Data_t *SDPServiceSearchAttributeResponse);
static void DisplayDataElement(SDP_Data_Element_t *SDPDataElement, unsigned int Level);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI LETP_AsynchronousCallback(unsigned int BluetoothStackID, unsigned long CallbackParameter);
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID,GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_LETP_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);
static void BTPSAPI GATT_SPPLE_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Service_Discovery_Event_Callback(unsigned int BluetoothStackID, GATT_Service_Discovery_Event_Data_t *GATT_Service_Discovery_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI SDP_Event_Callback(unsigned int BluetoothStackID, unsigned int SDPRequestID, SDP_Response_Data_t *SDP_Response_Data, unsigned long CallbackParameter);
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI SPP_Event_Callback(unsigned int BluetoothStackID, SPP_Event_Data_t *SPP_Event_Data, unsigned long CallbackParameter);

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
         DeviceInfoPtr->ConnectionBD_ADDR     = ConnectionBD_ADDR;
         DeviceInfoPtr->ServerMTU             = ATT_PROTOCOL_MTU_MINIMUM_LE;

         ret_val = BSC_AddGenericListEntry_Actual(ekBD_ADDR_t, BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead), (void *)(DeviceInfoPtr));
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
   return(BSC_SearchGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead)));
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
   return(BSC_DeleteGenericListEntry(ekBD_ADDR_t, (void *)(&BD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, ConnectionBD_ADDR), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr), (void **)(ListHead)));
}

   /* This function frees the specified Key Info Information member     */
   /* memory.                                                           */
static void FreeDeviceInfoEntryMemory(DeviceInfo_t *EntryToFree)
{
   BSC_FreeGenericListEntryMemory((void *)(EntryToFree));
}

   /* The following function deletes (and free's all memory) every      */
   /* element of the specified Key Info List. Upon return of this       */
   /* function, the Head Pointer is set to NULL.                        */
static void FreeDeviceInfoList(DeviceInfo_t **ListHead)
{
   BSC_FreeGenericListEntryList((void **)(ListHead), BTPS_STRUCTURE_OFFSET(DeviceInfo_t, NextDeviceInfoInfoPtr));
}

   /* The following function adds a device to the scan results array.   */
   /* Upon adding a new, non-duplicate entry to the array, the 0-based  */
   /* index will be returned. A negative return value indicates that no */
   /* new entry was added.                                              */
static int AddDeviceToScanResults(GAP_LE_Advertising_Report_Data_t *DeviceEntry)
{
   int i = -1;
   int Ndx;

   if((DeviceEntry) && (NumScanEntries < MAX_SCAN_ENTRIES))
   {
      i = 0;
      while(i < NumScanEntries)
      {
         if(COMPARE_BD_ADDR(DeviceEntry->BD_ADDR, ScanInfo[i].BD_ADDR))
            break;

         i++;
      }

      if(i == NumScanEntries)
      {
         memset(&ScanInfo[i], 0, SCAN_INFO_DATA_SIZE);
         ScanInfo[i].BD_ADDR     = DeviceEntry->BD_ADDR;
         ScanInfo[i].AddressType = DeviceEntry->Address_Type;
         NumScanEntries++;
      }
      else
      {
         if(DeviceEntry->Advertising_Report_Type == rtScanResponse)
         {
            if(ScanInfo[i].Flags & SCAN_INFO_FLAG_SCAN_RESPONSE_PACKET_PROCESSED)
               i = -1;
            else
               ScanInfo[i].Flags |= (Byte_t)SCAN_INFO_FLAG_SCAN_RESPONSE_PACKET_PROCESSED;
         }
         else
         {
            if(ScanInfo[i].Flags & SCAN_INFO_FLAG_ADVERTISING_PACKET_PROCESSED)
              i = -1;
            else
               ScanInfo[i].Flags |= (Byte_t)SCAN_INFO_FLAG_ADVERTISING_PACKET_PROCESSED;
         }
      }

      if(i >= 0)
      {
         Ndx = 0;
         while(Ndx < (int)DeviceEntry->Advertising_Data.Number_Data_Entries)
         {
            if((DeviceEntry->Advertising_Data.Data_Entries[Ndx].AD_Type == HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE) ||
               (DeviceEntry->Advertising_Data.Data_Entries[Ndx].AD_Type == HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_SHORTENED))
            {
               if(DeviceEntry->Advertising_Data.Data_Entries[Ndx].AD_Data_Length > MAX_NAME_SIZE)
                  DeviceEntry->Advertising_Data.Data_Entries[Ndx].AD_Data_Length = MAX_NAME_SIZE;

               memcpy(ScanInfo[i].NameBuffer, DeviceEntry->Advertising_Data.Data_Entries[Ndx].AD_Data_Buffer, DeviceEntry->Advertising_Data.Data_Entries[Ndx].AD_Data_Length);
               ScanInfo[i].NameBuffer[DeviceEntry->Advertising_Data.Data_Entries[Ndx].AD_Data_Length] = 0;
               break;
            }
            Ndx++;
         }
      }
   }

   return(i);
}

static void UserInterface_Selection(void)
{
   UI_Mode = UI_MODE_SELECT;

   /* Next display the available commands.                              */
   DisplayHelp(NULL);

   AddCommand("SPP", Select_SPP);
   AddCommand("SPPLE", Select_SPPLE);
   AddCommand("HELP", DisplayHelp);
}

static void UserInterface_SPP(void)
{
   UI_Mode = UI_MODE_IS_SPP;

   ClearCommands();

   AddCommand("INQUIRY", Inquiry);
   AddCommand("DISPLAYDEVICELIST", DisplayDeviceList);
   AddCommand("PAIR", Pair);
   AddCommand("ENDPAIRING", EndPairing);
   AddCommand("PINCODERESPONSE", PINCodeResponse);
   AddCommand("PASSKEYRESPONSE", PassKeyResponse);
   AddCommand("USERCONFIRMATIONRESPONSE", UserConfirmationResponse);
   AddCommand("SETDISCOVERABILITYMODE", SetCBDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetCBConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetCBPairabilityMode);
   AddCommand("CHANGESIMPLEPAIRINGPARAMETERS", ChangeSimplePairingParameters);
   AddCommand("GETLOCALADDRESS", GetLocalAddress);
   AddCommand("SETLOCALNAME", SetLocalName);
   AddCommand("GETLOCALNAME", GetLocalName);
   AddCommand("SETCLASSOFDEVICE", SetClassOfDevice);
   AddCommand("GETCLASSOFDEVICE", GetClassOfDevice);
   AddCommand("GETREMOTENAME", GetRemoteName);
   AddCommand("SERVICEDISCOVERY", ServiceDiscovery);
   AddCommand("OPEN", OpenRemoteServer);
   AddCommand("CLOSE", CloseRemoteServer);
   AddCommand("OPENSERVER", OpenServer);
   AddCommand("CLOSESERVER", CloseServer);
   AddCommand("READ", Read);
   AddCommand("WRITE", Write);
   AddCommand("SPPLE", Select_SPPLE);
   AddCommand("HELP", DisplayHelp);
}

static void UserInterface_SPPLE(void)
{
   UI_Mode = UI_MODE_IS_SPPLE;

   ClearCommands();

   /* Install the commands relevant for this UI.                        */
   AddCommand("DISPLAYDEVICELIST", DisplayDeviceList);
   AddCommand("DISPLAYSCANLIST", DisplayScanEntries);
   AddCommand("SETDISCOVERABILITYMODE", SetLEDiscoverabilityMode);
   AddCommand("SETCONNECTABILITYMODE", SetLEConnectabilityMode);
   AddCommand("SETPAIRABILITYMODE", SetLEPairabilityMode);
   AddCommand("CHANGEPAIRINGPARAMETERS", ChangeLEPairingParameters);
   AddCommand("GETLOCALADDRESS", GetLocalAddress);
   AddCommand("ADVERTISE", AdvertiseLE);
   AddCommand("SCAN", ScanLE);
   AddCommand("SETLEPARAMS", SetLEParams);
   AddCommand("CONNECT", ConnectLE);
   AddCommand("CANCELCONNECT", CancelConnectLE);
   AddCommand("DISCONNECT", DisconnectLE);
   AddCommand("PAIR", PairLE);
   AddCommand("LEPASSKEYRESPONSE", LEPassKeyResponse);
   AddCommand("SEND", SendDataCommand);
   AddCommand("READ", ReadDataCommand);
   AddCommand("DISCOVERLETP", DiscoverLETP);
   AddCommand("REGISTERLETP", RegisterLETP);
   AddCommand("UNREGISTERLETP", UnregisterLETP);
   AddCommand("SETLETPCOUNT", SetLETPCount);
   AddCommand("SETLETPINTERVAL", SetLETPInterval);
   AddCommand("ENABLELETP", EnableLETP);
   AddCommand("TIMELETP", TimeLETP);
   AddCommand("DISCOVERSPPLE", DiscoverSPPLE);
   AddCommand("REGISTERSPPLE", RegisterSPPLE);
   AddCommand("UNREGISTERSPPLE", UnregisterSPPLE);
   AddCommand("CONFIGURESPPLE", ConfigureSPPLE);
   AddCommand("DISCOVERDIS", DiscoverDIS);
   AddCommand("DISCOVERGAPS", DiscoverGAPS);
   AddCommand("GETLOCALNAME", ReadLocalName);
   AddCommand("SETLOCALNAME", SetLocalName);
   AddCommand("GETREMOTENAME", ReadRemoteName);
   AddCommand("GETLOCALAPPEARANCE", ReadLocalAppearance);
   AddCommand("SETLOCALAPPEARANCE", SetLocalAppearance);
   AddCommand("GETREMOTEAPPEARANCE", ReadRemoteAppearance);
   AddCommand("DISPLAYRAWMODEDATA", DisplayRawModeData);
   AddCommand("AUTOMATICREADMODE", AutomaticReadMode);
   AddCommand("SPP", Select_SPP);
   AddCommand("HELP", DisplayHelp);
}

   /* The following function is responsible for parsing user input      */
   /* and call appropriate command function.                            */
static void UserInterface(void)
{
   UserCommand_t TempCommand;
   int  Result = !EXIT_CODE;
   char UserInput[MAX_COMMAND_LENGTH];

   /* First let's make sure that we start on new line.                  */
   PRINTF("\n");

   /* Next display the available commands.                              */
   UserInterface_Selection();

   /* This is the main loop of the program.  It gets user input from the*/
   /* command window, make a call to the command parser, and command    */
   /* interpreter.  After the function has been ran it then check the   */
   /* return value and displays an error message when appropriate.  If  */
   /* the result returned is ever the EXIT_CODE the loop will exit      */
   /* leading to the exit of the program.                               */
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
                     Result = 0;

                     /* Close all connected ports.                      */
                     CloseServer(NULL);
                     CloseRemoteServer(NULL);

                     /* Restart the User Interface Selection.           */
                     UI_Mode = UI_MODE_SELECT;

                     /* Set up the Selection Interface.                 */
                     UserInterface_Selection();
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
   /* strings to there unsigned integer equivalent.  This function can  */
   /* handle leading and tailing white space, however it does not handle*/
   /* signed or comma delimited values.  This function takes as its     */
   /* input the string which is to be converted.  The function returns  */
   /* zero if an error occurs otherwise it returns the value parsed from*/
   /* the string passed as the input parameter.                         */
static unsigned long StringToUnsignedInteger(char *StringInteger)
{
   int           IsHex;
   unsigned long Index;
   unsigned long ret_val = 0;

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
      for(Index=0, ret_val=String;Index < (int)BTPS_StringLength(String);Index++)
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
         Input        += BTPS_StringLength(TempCommand->Command)+1;
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
               Input        += BTPS_StringLength(LastParameter)+1;
               StringLength -= BTPS_StringLength(LastParameter)+1;

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
   /* A negative return value implies that the command was not found and*/
   /* is invalid.                                                       */
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
      for(Index=0,ret_val=NULL;((Index<NumberCommands) && (!ret_val));Index++)
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

static int DisplayScanEntries(ParameterList_t *TempParam)
{
   int        i;
   BoardStr_t Address;

   UNREFERENCED_PARAM(TempParam);

   i = 0;
   if(NumScanEntries)
   {
      while(i < NumScanEntries)
      {
         BD_ADDRToStr(ScanInfo[i].BD_ADDR, Address);
         PRINTF(" %d: %s %s\n", i+1, Address, ScanInfo[i].NameBuffer);
         i++;
      }
   }
   else
      PRINTF("Device list is empty.\n");

   return(0);
}

   /* Displays the current I/O Capabilities.                            */
static void DisplayIOCapabilities(void)
{
   PRINTF("LE:     I/O Capabilities: %s, MITM: %s.\n", IOCapabilitiesStrings[(unsigned int)(LE_Parameters.IOCapability - licDisplayOnly)], LE_Parameters.MITMProtection?"TRUE":"FALSE");
   PRINTF("BR/EDR: I/O Capabilities: %s, MITM: %s.\n", IOCapabilitiesStrings[(unsigned int)(IOCapability - icDisplayOnly)], MITMProtection?"TRUE":"FALSE");
}

   /* Utility function to display a Class of Device Structure.          */
static void DisplayClassOfDevice(Class_of_Device_t Class_of_Device)
{
   PRINTF("Class of Device: 0x%02X%02X%02X.\n", Class_of_Device.Class_of_Device0, Class_of_Device.Class_of_Device1, Class_of_Device.Class_of_Device2);
}

static void DisplayKey(char *Prompt, int KeyLength, Byte_t *Key)
{
   int Index;

   PRINTF("%s", Prompt);

   for(Index = 0; Index < KeyLength; Index++)
      PRINTF("%02X", Key[Index]);

   PRINTF("\n");
}

   /* Utility function to display advertising data.                     */
static void DisplayAdvertisingData(GAP_LE_Advertising_Data_t *Advertising_Data)
{
   Byte_t tmp;
   int    Index;
   int    Index2;

   /* Verify that the input parameters seem semi-valid.                 */
   if(Advertising_Data)
   {
      for(Index = 0; Index < (int)Advertising_Data->Number_Data_Entries; Index++)
      {
         PRINTF("  AD Type: 0x%02X.\n", (Byte_t)Advertising_Data->Data_Entries[Index].AD_Type);
         if(Advertising_Data->Data_Entries[Index].AD_Type == HCI_LE_ADVERTISING_REPORT_DATA_TYPE_LOCAL_NAME_COMPLETE)
         {
            Index2 = Advertising_Data->Data_Entries[Index].AD_Data_Length;
            tmp    = Advertising_Data->Data_Entries[Index].AD_Data_Buffer[Index2];
            Advertising_Data->Data_Entries[Index].AD_Data_Buffer[Index2] = 0;
            PRINTF("  AD Name: %s.\n", Advertising_Data->Data_Entries[Index].AD_Data_Buffer);
            Advertising_Data->Data_Entries[Index].AD_Data_Buffer[Index2] = tmp;
         }
         PRINTF("  AD Length: 0x%02X.\n", Advertising_Data->Data_Entries[Index].AD_Data_Length);
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

   PRINTF("   MITM:                %s.\n", (Pairing_Capabilities.MITM == TRUE)?"TRUE":"FALSE");
   PRINTF("   Bonding Type:        %s.\n", (Pairing_Capabilities.Bonding_Type == lbtBonding)?"Bonding":"No Bonding");
   PRINTF("   OOB:                 %s.\n", (Pairing_Capabilities.OOB_Present == TRUE)?"OOB":"OOB Not Present");
   PRINTF("   Encryption Key Size: %d.\n", Pairing_Capabilities.Maximum_Encryption_Key_Size);
   PRINTF("   Sending Keys: \n");
   PRINTF("      LTK:              %s.\n", ((Pairing_Capabilities.Sending_Keys.Encryption_Key == TRUE)?"YES":"NO"));
   PRINTF("      IRK:              %s.\n", ((Pairing_Capabilities.Sending_Keys.Identification_Key == TRUE)?"YES":"NO"));
   PRINTF("      CSRK:             %s.\n", ((Pairing_Capabilities.Sending_Keys.Signing_Key == TRUE)?"YES":"NO"));
   PRINTF("   Receiving Keys: \n");
   PRINTF("      LTK:              %s.\n", ((Pairing_Capabilities.Receiving_Keys.Encryption_Key == TRUE)?"YES":"NO"));
   PRINTF("      IRK:              %s.\n", ((Pairing_Capabilities.Receiving_Keys.Identification_Key == TRUE)?"YES":"NO"));
   PRINTF("      CSRK:             %s.\n", ((Pairing_Capabilities.Receiving_Keys.Signing_Key == TRUE)?"YES":"NO"));
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
   if(UI_Mode == UI_MODE_IS_SPP)
      PRINTF("\nSPP");
   if(UI_Mode == UI_MODE_IS_SPPLE)
      PRINTF("\nSPPLE");
   PRINTF(">");
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

   /* Displays a function success message.                              */
static void DisplayFunctionSuccess(char *Function)
{
   PRINTF("%s success.\n",Function);
}

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation)
{
   int                           i;
   int                           Result;
   int                           ret_val = 0;
   char                          BluetoothAddress[16];
   Byte_t                        Status;
   Byte_t                        NumberLEPackets;
   Word_t                        LEPacketLength;
   BD_ADDR_t                     BD_ADDR;
   unsigned int                  ServiceID;
   HCI_Version_t                 HCIVersion;
   L2CA_Link_Connect_Params_t    L2CA_Link_Connect_Params;

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

         /* Next, check the return value of the initialization to see   */
         /* if it was successful.                                       */
         if(Result > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            BluetoothStackID = Result;
            PRINTF("Bluetooth Stack ID: %d.\n", BluetoothStackID);

            /* Initialize the Default Pairing Parameters.               */
            LE_Parameters.IOCapability   = DEFAULT_LE_IO_CAPABILITY;
            LE_Parameters.MITMProtection = DEFAULT_LE_MITM_PROTECTION;
            LE_Parameters.OOBDataPresent = FALSE;

            /* Initialize the default Secure Simple Pairing parameters. */
            IOCapability                 = DEFAULT_IO_CAPABILITY;
            OOBSupport                   = FALSE;
            MITMProtection               = DEFAULT_MITM_PROTECTION;

            if(!HCI_Version_Supported(BluetoothStackID, &HCIVersion))
               PRINTF("Device Chipset: %s.\n", ((int)HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]);

            /* Let's output the Bluetooth Device Address so that the    */
            /* user knows what the Device Address is.                   */
            if(!GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR))
            {
               BD_ADDRToStr(BD_ADDR, BluetoothAddress);

               PRINTF("BD_ADDR: %s\n", BluetoothAddress);
            }

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Go ahead and allow Master/Slave Role Switch.             */
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Request_Config  = cqAllowRoleSwitch;
            L2CA_Link_Connect_Params.L2CA_Link_Connect_Response_Config = csMaintainCurrentRole;

            L2CA_Set_Link_Connection_Configuration(BluetoothStackID, &L2CA_Link_Connect_Params);

            if(HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_DEFAULT_LINK_POLICY_BIT_NUMBER) > 0)
               HCI_Write_Default_Link_Policy_Settings(BluetoothStackID, (HCI_LINK_POLICY_SETTINGS_ENABLE_MASTER_SLAVE_SWITCH|HCI_LINK_POLICY_SETTINGS_ENABLE_SNIFF_MODE), &Status);

            /* Delete all Stored Link Keys.                             */
            ASSIGN_BD_ADDR(BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            DeleteLinkKey(BD_ADDR);

            /* Flag that no connection is currently active.             */
            ASSIGN_BD_ADDR(CurrentLERemoteBD_ADDR, 0, 0, 0, 0, 0, 0);
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0, 0, 0, 0, 0, 0);
            LocalDeviceIsMaster = FALSE;

            for(i=0; i<MAX_LE_CONNECTIONS; i++)
            {
               LEContextInfo[i].ConnectionID = 0;
               ASSIGN_BD_ADDR(LEContextInfo[i].ConnectionBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
            }

            /* Set the default connection parameters.                   */
            ConnectParams.ConnectIntMin    = 50;
            ConnectParams.ConnectIntMax    = 100;
            ConnectParams.SlaveLatency     = 0;
            ConnectParams.MaxConnectLength = 10000;
            ConnectParams.MinConnectLength = 0;
            ConnectParams.SupervisionTO    = 20000;

            DataStrPtr    = (char *)DataStr;
            DataStrLength = DATA_STR_LENGTH;

            /* Regenerate IRK and DHK from the constant Identity Root   */
            /* Key.                                                     */
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 1,0, &IRK);
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 3, 0, &DHK);

            /* Flag that we have no Key Information in the Key List.    */
            DeviceInfoList = NULL;
            DeviceListMutex = BTPS_CreateMutex(FALSE);

            /* Initialize the GATT Service.                             */
            if((Result = GATT_Initialize(BluetoothStackID, GATT_INITIALIZATION_FLAGS_SUPPORT_LE, GATT_Connection_Event_Callback, 0)) == 0)
            {
               /* Determine the number of LE packets that the controller*/
               /* will accept at a time.                                */
               if((!HCI_LE_Read_Buffer_Size(BluetoothStackID, &Status, &LEPacketLength, &NumberLEPackets)) && (!Status) && (LEPacketLength))
               {
                  NumberLEPackets = (Byte_t)(NumberLEPackets/MAX_LE_CONNECTIONS);
                  NumberLEPackets = (Byte_t)((NumberLEPackets == 0)?1:NumberLEPackets);
               }
               else
                  NumberLEPackets = 1;

               /* Set a limit on the number of packets that we will     */
               /* queue internally.                                     */
               GATT_Set_Queuing_Parameters(BluetoothStackID, (unsigned int)NumberLEPackets, (unsigned int)(NumberLEPackets-1), FALSE);

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
                  if(Result >= 0)
                  {
                     /* Save the Instance ID of the DIS Service.        */
                     DISInstanceID = (unsigned int)Result;

                     /* Set the discoverable attributes                 */
                     DIS_Set_Manufacturer_Name(BluetoothStackID, DISInstanceID, "Qualcomm Inc");
                     DIS_Set_Model_Number(BluetoothStackID, DISInstanceID, "Model Bluetopia");
                     DIS_Set_Serial_Number(BluetoothStackID, DISInstanceID, "Serial Number 1234");
                     DIS_Set_Software_Revision(BluetoothStackID, DISInstanceID, "Software 4.1");
                  }
                  else
                     DisplayFunctionError("DIS_Initialize_Service", Result);

                  /* Return success to the caller.                      */
                  ret_val        = 0;
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

            /* Initialize SPP context.                                  */
            BTPS_MemInitialize(SPPContextInfo, 0, sizeof(SPPContextInfo));
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
   int ret_val;

   /* First check to see if the Stack has been opened.                  */
   if(BluetoothStackID)
   {
      if(DataStrPtr != DataStr)
         BTPS_FreeMemory(DataStrPtr);

      /* Cleanup DIS Service Module.                                    */
      if(DISInstanceID)
         DIS_Cleanup_Service(BluetoothStackID, DISInstanceID);

      /* Cleanup GAP Service Module.                                    */
      if(GAPSInstanceID)
         GAPS_Cleanup_Service(BluetoothStackID, GAPSInstanceID);

      /* Un-registered SPP LE Service.                                  */
      if(SPPLEServiceID)
         GATT_Un_Register_Service(BluetoothStackID, SPPLEServiceID);

      /* Cleanup GATT Module.                                           */
      GATT_Cleanup(BluetoothStackID);

      /* Simply close the Stack                                         */
      BSC_Shutdown(BluetoothStackID);

      /* Free BTPSKRNL allocated memory.                                */
      BTPS_DeInit();

      PRINTF("Stack Shutdown.\n");

      /* Free the Key List.                                             */
      FreeDeviceInfoList(&DeviceInfoList);

      /* Release the control Mutex.                                     */
      BTPS_CloseMutex(DeviceListMutex);

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
   int ret_val;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* A semi-valid Bluetooth Stack ID exists, now attempt to set the */
      /* attached Devices Discoverablity Mode to General.               */
      ret_val = GAP_Set_Discoverability_Mode(BluetoothStackID, dmGeneralDiscoverableMode, 0);

      /* Next, check the return value of the GAP Set Discoverability    */
      /* Mode command for successful execution.                         */
      if(!ret_val)
      {
         /* * NOTE * Discoverability is only applicable when we are     */
         /*          advertising so save the default Discoverability    */
         /*          Mode for later.                                    */
         LE_Parameters.DiscoverabilityMode = dmGeneralDiscoverableMode;
      }
      else
      {
         /* An error occurred while trying to set the Discoverability   */
         /* Mode of the Device.                                         */
         DisplayFunctionError("Set Discoverable Mode", ret_val);
      }
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
   int ret_val;

   /* First, check that a valid Bluetooth Stack ID exists.              */
   if(BluetoothStackID)
   {
      /* Attempt to set the attached Device to be Connectable.          */
      ret_val = GAP_Set_Connectability_Mode(BluetoothStackID, cmConnectableMode);

      /* Next, check the return value of the                            */
      /* GAP_Set_Connectability_Mode() function for successful          */
      /* execution.                                                     */
      if(!ret_val)
      {
         /* * NOTE * Connectability is only an applicable when          */
         /*          advertising so we will just save the default       */
         /*          connectability for the next time we enable         */
         /*          advertising.                                       */
         LE_Parameters.ConnectableMode = lcmConnectable;
      }
      else
      {
         /* An error occurred while trying to make the Device           */
         /* Connectable.                                                */
         DisplayFunctionError("Set Connectability Mode", ret_val);
      }
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
      Result = GAP_Set_Pairability_Mode(BluetoothStackID, pmPairableMode);

      /* Next, check the return value of the GAP Set Pairability mode   */
      /* command for successful execution.                              */
      if(!Result)
      {
         /* The device has been set to pairable mode, now register an   */
         /* Authentication Callback to handle the Authentication events */
         /* if required.                                                */
         Result = GAP_Register_Remote_Authentication(BluetoothStackID, GAP_Event_Callback, (unsigned long)0);

         /* Next, check the return value of the GAP Register Remote     */
         /* Authentication command for successful execution.            */
         if(!Result)
         {
            /* Now Set the LE Pairability.                              */

            /* Attempt to set the attached device to be pairable.       */
            Result = GAP_LE_Set_Pairability_Mode(BluetoothStackID, lpmPairableMode);

            /* Next, check the return value of the GAP Set Pairability  */
            /* mode command for successful execution.                   */
            if(!Result)
            {
               /* The device has been set to pairable mode, now register*/
               /* an Authentication Callback to handle the              */
               /* Authentication events if required.                    */
               Result = GAP_LE_Register_Remote_Authentication(BluetoothStackID, GAP_LE_Event_Callback, (unsigned long)0);

               /* Next, check the return value of the GAP Register      */
               /* Remote Authentication command for successful          */
               /* execution.                                            */
               if(Result)
               {
                  /* An error occurred while trying to execute this     */
                  /* function.                                          */
                  DisplayFunctionError("GAP_LE_Register_Remote_Authentication", Result);

                  ret_val = Result;
               }
            }
            else
            {
               /* An error occurred while trying to make the device     */
               /* pairable.                                             */
               DisplayFunctionError("GAP_LE_Set_Pairability_Mode", Result);

               ret_val = Result;
            }
         }
         else
         {
            /* An error occurred while trying to execute this function. */
            DisplayFunctionError("GAP_Register_Remote_Authentication", Result);

            ret_val = Result;
         }
      }
      else
      {
         /* An error occurred while trying to make the device pairable. */
         DisplayFunctionError("GAP_Set_Pairability_Mode", Result);

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
   /* mechanism of populating a Client Information structure with the   */
   /* information discovered from a GATT Discovery operation.           */
static void DISPopulateHandles(DIS_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo)
{
   unsigned int                                  Index;
   GATT_Characteristic_Information_t            *CurrentCharacteristic;

   /* Verify that the input parameters are semi-valid.                  */
   if((ClientInfo) && (ServiceInfo) && (ServiceInfo->ServiceInformation.UUID.UUID_Type == guUUID_16) && (DIS_COMPARE_DIS_SERVICE_UUID_TO_UUID_16(ServiceInfo->ServiceInformation.UUID.UUID.UUID_16)))
   {
      /* Initialize the Client Info Structure.                          */
      BTPS_MemInitialize(ClientInfo, 0, sizeof(DIS_Client_Info_t));

      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceInfo->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index=0;Index<ServiceInfo->NumberOfCharacteristics;Index++,CurrentCharacteristic++)
         {
            /* All DIS UUIDs are defined to be 16 bit UUIDs.            */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_16)
            {
               /* Determine which characteristic this is.               */
               if(DIS_COMPARE_DIS_MANUFACTURER_NAME_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->ManufacturerNameHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_MODEL_NUMBER_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->ModelNumberHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_SERIAL_NUMBER_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->SerialNumberHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_HARDWARE_REVISION_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->HardwareRevisionHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_FIRMWARE_REVISION_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->FirmwareRevisionHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_SOFTWARE_REVISION_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->SoftwareRevisionHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_SYSTEM_ID_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->SystemIDHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_IEEE_CERTIFICATION_DATA_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->IEEE11073CertHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
               if(DIS_COMPARE_DIS_PNP_ID_UUID_TO_UUID_16(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_16))
               {
                  ClientInfo->PnPIDHandle = CurrentCharacteristic->Characteristic_Handle;
                  continue;
               }
            }
         }
      }
   }
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
         for(Index1=0;Index1<ServiceInfo->NumberOfCharacteristics;Index1++,CurrentCharacteristic++)
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
   /* mechanism of populating a SPPLE Client Information structure with */
   /* the information discovered from a GATT Service Discovery          */
   /* operation.                                                        */
static void SPPLEPopulateHandles(SPPLE_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo)
{
   Word_t                                       *ClientConfigurationHandle;
   unsigned int                                  Index1;
   unsigned int                                  Index2;
   GATT_Characteristic_Information_t            *CurrentCharacteristic;
   GATT_Characteristic_Descriptor_Information_t *CurrentDescriptor;

   /* Verify that the input parameters are semi-valid.                  */
   if((ClientInfo) && (ServiceInfo) && (ServiceInfo->ServiceInformation.UUID.UUID_Type == guUUID_128) && (SPPLE_COMPARE_SPPLE_SERVICE_UUID_TO_UUID_128(ServiceInfo->ServiceInformation.UUID.UUID.UUID_128)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceInfo->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1=0;Index1<ServiceInfo->NumberOfCharacteristics;Index1++,CurrentCharacteristic++)
         {
            /* All SPPLE UUIDs are defined to be 128 bit UUIDs.         */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_128)
            {
               /* Determine which characteristic this is.               */
               if(!SPPLE_COMPARE_SPPLE_TX_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
               {
                  if(!SPPLE_COMPARE_SPPLE_TX_CREDITS_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
                  {
                     if(!SPPLE_COMPARE_SPPLE_RX_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
                     {
                        if(!SPPLE_COMPARE_SPPLE_RX_CREDITS_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
                           continue;
                        else
                        {
                           ClientInfo->Rx_Credit_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                           ClientConfigurationHandle            = &(ClientInfo->Rx_Credit_Client_Configuration_Descriptor);
                        }
                     }
                     else
                     {
                        ClientInfo->Rx_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                        continue;
                     }
                  }
                  else
                  {
                     ClientInfo->Tx_Credit_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                     continue;
                  }
               }
               else
               {
                  ClientInfo->Tx_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                  ClientConfigurationHandle     = &(ClientInfo->Tx_Client_Configuration_Descriptor);
               }

               /* Loop through the Descriptor List.                     */
               CurrentDescriptor = CurrentCharacteristic->DescriptorList;
               if((CurrentDescriptor) && (ClientConfigurationHandle))
               {
                  for(Index2=0;Index2<CurrentCharacteristic->NumberOfDescriptors;Index2++,CurrentDescriptor++)
                  {
                     if(CurrentDescriptor->Characteristic_Descriptor_UUID.UUID_Type == guUUID_16)
                     {
                        if(GATT_COMPARE_CLIENT_CHARACTERISTIC_CONFIGURATION_ATTRIBUTE_TYPE_TO_BLUETOOTH_UUID_16(CurrentDescriptor->Characteristic_Descriptor_UUID.UUID.UUID_16))
                        {
                           *ClientConfigurationHandle = CurrentDescriptor->Characteristic_Descriptor_Handle;
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

   /* The following function is a utility function that provides a      */
   /* mechanism of populating a LETP Client Information structure with  */
   /* the information discovered from a GATT Service Discovery          */
   /* operation.                                                        */
static void LETPPopulateHandles(LETP_Client_Info_t *ClientInfo, GATT_Service_Discovery_Indication_Data_t *ServiceInfo)
{
   Word_t                                       *ClientConfigurationHandle;
   unsigned int                                  Index1;
   unsigned int                                  Index2;
   GATT_Characteristic_Information_t            *CurrentCharacteristic;
   GATT_Characteristic_Descriptor_Information_t *CurrentDescriptor;

   /* Verify that the input parameters are semi-valid.                  */
   if((ClientInfo) && (ServiceInfo) && (ServiceInfo->ServiceInformation.UUID.UUID_Type == guUUID_128) && (LETP_COMPARE_LETP_SERVICE_UUID_TO_UUID_128(ServiceInfo->ServiceInformation.UUID.UUID.UUID_128)))
   {
      /* Loop through all characteristics discovered in the service     */
      /* and populate the correct entry.                                */
      CurrentCharacteristic = ServiceInfo->CharacteristicInformationList;
      if(CurrentCharacteristic)
      {
         for(Index1=0;Index1<ServiceInfo->NumberOfCharacteristics;Index1++,CurrentCharacteristic++)
         {
            /* All LETP UUIDs are defined to be 128 bit UUIDs.          */
            if(CurrentCharacteristic->Characteristic_UUID.UUID_Type == guUUID_128)
            {
               /* Determine if this is the Interval characteristic.     */
               if(LETP_COMPARE_LETP_TX_INTERVAL_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
                  ClientInfo->Tx_Interval_Characteristic = CurrentCharacteristic->Characteristic_Handle;

               /* Determine if this is the Bulk characteristic.         */
               if(LETP_COMPARE_LETP_TX_BULK_UUID_TO_UUID_128(CurrentCharacteristic->Characteristic_UUID.UUID.UUID_128))
               {
                  ClientInfo->Tx_Bulk_Characteristic = CurrentCharacteristic->Characteristic_Handle;
                  ClientConfigurationHandle          = &(ClientInfo->Tx_Bulk_Client_Configuration_Descriptor);
               }
               else
                  continue;

               /* Loop through the Descriptor List.                     */
               CurrentDescriptor = CurrentCharacteristic->DescriptorList;
               if((CurrentDescriptor) && (ClientConfigurationHandle))
               {
                  for(Index2=0;Index2<CurrentCharacteristic->NumberOfDescriptors;Index2++,CurrentDescriptor++)
                  {
                     if(CurrentDescriptor->Characteristic_Descriptor_UUID.UUID_Type == guUUID_16)
                     {
                        if(GATT_COMPARE_CLIENT_CHARACTERISTIC_CONFIGURATION_ATTRIBUTE_TYPE_TO_BLUETOOTH_UUID_16(CurrentDescriptor->Characteristic_Descriptor_UUID.UUID.UUID_16))
                        {
                           *ClientConfigurationHandle = CurrentDescriptor->Characteristic_Descriptor_Handle;
                           break;
                        }
                     }
                  }
               }
            }
         }
      }
   }
}

   /* The following function is used to ensure that the size of the     */
   /* Buffer that is used to indicate to the remote device is as large  */
   /* as the underlying MTU.                                            */
static void SetDataStrBuffer(Word_t Size)
{
   int ndx;
   int cnt;

   /* Check to see if the buffer need to be enlarged.                   */
   if(Size > DataStrLength)
   {
      /* Check to see the current buffer needs to be released.          */
      if(DataStrPtr != DataStr)
         BTPS_FreeMemory(DataStrPtr);

      /* Allocate a new buffer.                                         */
      DataStrPtr = (char *)BTPS_AllocateMemory(Size);
      if(DataStrPtr)
      {
         ndx = 0;
         while(ndx < Size)
         {
            /* Get the amount of data we still need to fill.            */
            cnt = (Size-ndx);

            /* Determine how many bytes we can copy on this round.      */
            if(cnt > DATA_STR_LENGTH)
               cnt = DATA_STR_LENGTH;

            /* Copy data to the buffer.                                 */
            BTPS_MemCopy(&DataStrPtr[ndx], DataStr, cnt);

            /* Adjust the Index.                                        */
            ndx += cnt;
         }

         /* Set the size of the string.                                 */
         DataStrLength = (int)Size;
      }
      else
         DataStrLength = 0;
   }
}

   /* The following function is a utility function that is used to add  */
   /* data (using InIndex as the buffer index) from the buffer specified*/
   /* by the DataBuffer parameter.  The second and third parameters     */
   /* specified the length of the data to add and the pointer to the    */
   /* data to add to the buffer.  This function returns the actual      */
   /* number of bytes that were added to the buffer (or 0 if none were  */
   /* added).                                                           */
static unsigned int AddDataToBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int DataLength, Byte_t *Data)
{
   unsigned int BytesAdded = 0;
   unsigned int Count;

   /* Verify that the input parameters are valid.                       */
   if((DataBuffer) && (DataLength) && (Data))
   {
      /* Loop while we have data AND space in the buffer.               */
      while(DataLength)
      {
         /* Get the number of bytes that can be placed in the buffer    */
         /* until it wraps.                                             */
         Count = DataBuffer->BufferSize - DataBuffer->InIndex;

         /* Determine if the number of bytes free is less than the      */
         /* number of bytes till we wrap and choose the smaller of the  */
         /* numbers.                                                    */
         Count = (DataBuffer->BytesFree < Count)?DataBuffer->BytesFree:Count;

         /* Cap the Count that we add to buffer to the length of the    */
         /* data provided by the caller.                                */
         Count = (Count > DataLength)?DataLength:Count;

         if(Count)
         {
            /* Copy the data into the buffer.                           */
            BTPS_MemCopy(&DataBuffer->Buffer[DataBuffer->InIndex], Data, Count);

            /* Update the counts.                                       */
            DataBuffer->InIndex   += Count;
            DataBuffer->BytesFree -= Count;
            DataLength            -= Count;
            BytesAdded            += Count;
            Data                  += Count;

            /* Wrap the InIndex if necessary.                           */
            if(DataBuffer->InIndex >= DataBuffer->BufferSize)
               DataBuffer->InIndex = 0;
         }
         else
            break;
      }
   }

   return(BytesAdded);
}

   /* The following function is a utility function that is used to      */
   /* removed data (using OutIndex as the buffer index) from the buffer */
   /* specified by the DataBuffer parameter The second parameter        */
   /* specifies the length of the Buffer that is pointed to by the third*/
   /* parameter.  This function returns the actual number of bytes that */
   /* were removed from the DataBuffer (or 0 if none were added).       */
   /* * NOTE * Buffer is optional and if not specified up to            */
   /*          BufferLength bytes will be deleted from the Buffer.      */
static unsigned int RemoveDataFromBuffer(SPPLE_Data_Buffer_t *DataBuffer, unsigned int BufferLength, Byte_t *Buffer)
{
   unsigned int Count;
   unsigned int BytesRemoved = 0;
   unsigned int MaxRemove;

   /* Verify that the input parameters are valid.                       */
   if((DataBuffer) && (BufferLength))
   {
      /* Loop while we have data to remove and space in the buffer to   */
      /* place it.                                                      */
      while(BufferLength)
      {
         /* Determine the number of bytes that are present in the       */
         /* buffer.                                                     */
         Count = DataBuffer->BufferSize - DataBuffer->BytesFree;
         if(Count)
         {
            /* Calculate the maximum number of bytes that I can remove  */
            /* from the buffer before it wraps.                         */
            MaxRemove = DataBuffer->BufferSize - DataBuffer->OutIndex;

            /* Cap max we can remove at the BufferLength of the caller's*/
            /* buffer.                                                  */
            MaxRemove = (MaxRemove > BufferLength)?BufferLength:MaxRemove;

            /* Cap the number of bytes I will remove in this iteration  */
            /* at the maximum I can remove or the number of bytes that  */
            /* are in the buffer.                                       */
            Count = (Count > MaxRemove)?MaxRemove:Count;

            /* Copy the data into the caller's buffer (If specified).   */
            if(Buffer)
            {
               BTPS_MemCopy(Buffer, &DataBuffer->Buffer[DataBuffer->OutIndex], Count);
               Buffer += Count;
            }

            /* Update the counts.                                       */
            DataBuffer->OutIndex  += Count;
            DataBuffer->BytesFree += Count;
            BytesRemoved          += Count;
            BufferLength          -= Count;

            /* Wrap the OutIndex if necessary.                          */
            if(DataBuffer->OutIndex >= DataBuffer->BufferSize)
               DataBuffer->OutIndex = 0;
         }
         else
            break;
      }
   }

   return(BytesRemoved);
}

   /* The following function is used to initialize the specified buffer */
   /* to the defaults.                                                  */
static void InitializeBuffer(SPPLE_Data_Buffer_t *DataBuffer)
{
   /* Verify that the input parameters are valid.                       */
   if(DataBuffer)
   {
      DataBuffer->BufferSize = SPPLE_DATA_CREDITS;
      DataBuffer->BytesFree  = SPPLE_DATA_CREDITS;
      DataBuffer->InIndex    = 0;
      DataBuffer->OutIndex   = 0;
   }
}

   /* The following function is used to enable/disable notifications on */
   /* a specified handle.  This function returns the positive non-zero  */
   /* Transaction ID of the Write Request or a negative error code.     */
static int EnableDisableNotificationsIndications(unsigned int ConnectionID, Word_t ClientConfigurationHandle, Word_t ClientConfigurationValue, GATT_Client_Event_Callback_t ClientEventCallback)
{
   int              ret_val;
   NonAlignedWord_t Buffer;

   /* Verify the input parameters.                                      */
   if((BluetoothStackID) && (ConnectionID) && (ClientConfigurationHandle))
   {
      ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer, ClientConfigurationValue);

      ret_val = GATT_Write_Request(BluetoothStackID, ConnectionID, ClientConfigurationHandle, sizeof(Buffer), &Buffer, ClientEventCallback, 0);
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}

   /* The following function is used to process data to be sent to a    */
   /* remote device.                                                    */
static void SendSPPLEData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo)
{
   int          Result;
   unsigned int DataLength;

   /* Loop through while sending data.                                  */
   while(LEContextInfo->BytesToSend != LEContextInfo->BytesSent)
   {
      /* Get the number of bytes left to send.                          */
      DataLength = (LEContextInfo->BytesToSend - LEContextInfo->BytesSent);

      /* Ensure DataToSend is no greater than the amount of buffer space*/
      /* available.                                                     */
      if(DataLength > (DATA_STR_LENGTH - LEContextInfo->DataStrIndex))
         DataLength = DATA_STR_LENGTH - LEContextInfo->DataStrIndex;

      /* Send the data.                                                 */
      Result = SPPLESendData(LEContextInfo, DeviceInfo, DataLength, (Byte_t *)&DataStr[LEContextInfo->DataStrIndex]);
      if(Result > 0)
      {
         /* Increment the counts.                                       */
         LEContextInfo->BytesSent    += Result;
         LEContextInfo->DataStrIndex += Result;

         if(LEContextInfo->DataStrIndex >= DATA_STR_HALF_LENGTH)
            LEContextInfo->DataStrIndex -= DATA_STR_HALF_LENGTH;
      }

      if(Result != (int)DataLength)
      {
         break;
      }
   }

   PRINTF("Sent %d bytes of %d bytes.\n", LEContextInfo->BytesSent, LEContextInfo->BytesToSend);

   if(LEContextInfo->BytesSent == LEContextInfo->BytesToSend)
   {
      PRINTF("Send Complete\n");
      LEContextInfo->BytesSent   = 0;
      LEContextInfo->BytesToSend = 0;
   }
}

   /* The following function is responsible for transmitting the        */
   /* specified number of credits to the remote device.                 */
static void SPPLESendCredits(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength)
{
   int              Result;
   unsigned int     ActualCredits;
   NonAlignedWord_t Credits;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo) && ((DataLength) || (LEContextInfo->SPPLEBufferInfo.QueuedCredits)))
   {
      /* Only attempt to send the credits if the LE buffer is not full. */
      if(LEContextInfo->BufferFull == FALSE)
      {
         /* Make sure that we don't credit more than can be filled in   */
         /* our receive buffer.                                         */
         ActualCredits = DataLength + LEContextInfo->SPPLEBufferInfo.QueuedCredits;
         ActualCredits = (ActualCredits > LEContextInfo->SPPLEBufferInfo.ReceiveBuffer.BytesFree)?LEContextInfo->SPPLEBufferInfo.ReceiveBuffer.BytesFree:ActualCredits;

         /* Format the credit packet.                                   */
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Credits, ActualCredits);

         /* Determine how to send credits based on the role.            */
         if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SPPLE_SERVER)
         {
            /* We are acting as a server so notify the Rx Credits       */
            /* characteristic.                                          */
            if(DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
               Result = GATT_Handle_Value_Notification(BluetoothStackID, SPPLEServiceID, LEContextInfo->ConnectionID, SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET, WORD_SIZE, (Byte_t *)&Credits);
            else
               Result = 0;
         }
         else
         {
            /* We are acting as a client so send a Write Without        */
            /* Response packet to the Tx Credit Characteristic.         */
            if(DeviceInfo->ClientInfo.Tx_Credit_Characteristic)
               Result = GATT_Write_Without_Response_Request(BluetoothStackID, LEContextInfo->ConnectionID, DeviceInfo->ClientInfo.Tx_Credit_Characteristic, WORD_SIZE, &Credits);
            else
               Result = 0;
         }

         /* If an error occurred we need to queue the credits to try    */
         /* again.                                                      */
         if(Result >= 0)
         {
            /* Clear the queued credit count as if there were any queued*/
            /* credits they have now been sent.                         */
            LEContextInfo->SPPLEBufferInfo.QueuedCredits = 0;
         }
         else
         {
            if(Result == BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE)
            {
               /* Flag that the buffer is full.                         */
               LEContextInfo->BufferFull = TRUE;
            }

            LEContextInfo->SPPLEBufferInfo.QueuedCredits += (Word_t)DataLength;
         }
      }
      else
         LEContextInfo->SPPLEBufferInfo.QueuedCredits += (Word_t)DataLength;
   }
}

   /* The following function is responsible for handling a received     */
   /* credit, event.                                                    */
static void SPPLEReceiveCreditEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int Credits)
{
   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      PRINTF("Received %d Credits\n", Credits);

      /* If this is a real credit event store the number of credits.    */
      LEContextInfo->SPPLEBufferInfo.TransmitCredits += Credits;

      /* Check to see if we have data to send.                          */
      if(LEContextInfo->BytesToSend)
         SendSPPLEData(LEContextInfo, DeviceInfo);
   }
}

   /* The following function sends the specified data to the specified  */
   /* data.  This function will queue any of the data that does not go  */
   /* out.  This function returns the number of bytes sent if all the   */
   /* data was sent, or 0.                                              */
   /* * NOTE * If DataLength is 0 and Data is NULL then all queued data */
   /*          will be sent.                                            */
static unsigned int SPPLESendData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data)
{
   int          Result = 0;
   unsigned int DataCount;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo) && (DataLength) && (Data))
   {
      /* Check to see if we have credits to use to transmit the data    */
      /* (and that the buffer is not FULL).                             */
      if((LEContextInfo->SPPLEBufferInfo.TransmitCredits) && (LEContextInfo->BufferFull == FALSE))
      {
         /* Get the maximum length of what we can send in this          */
         /* transaction.                                                */
         DataCount = (DataLength > LEContextInfo->SPPLEBufferInfo.TransmitCredits)?LEContextInfo->SPPLEBufferInfo.TransmitCredits:DataLength;

         /* Use the correct API based on device role for SPPLE.         */
         if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SPPLE_SERVER)
         {
            /* We are acting as SPPLE Server, so notify the Tx          */
            /* Characteristic.                                          */
            if(DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
               Result = GATT_Handle_Value_Notification(BluetoothStackID, SPPLEServiceID, LEContextInfo->ConnectionID, SPPLE_TX_CHARACTERISTIC_ATTRIBUTE_OFFSET, (Word_t)DataCount, Data);
         }
         else
         {
            /* We are acting as SPPLE Client, so write to the Rx        */
            /* Characteristic.                                          */
            if(DeviceInfo->ClientInfo.Tx_Characteristic)
               Result = GATT_Write_Without_Response_Request(BluetoothStackID, LEContextInfo->ConnectionID, DeviceInfo->ClientInfo.Rx_Characteristic, (Word_t)DataCount, Data);
         }

         /* Check to see if the data was written successfully.          */
         if(Result >= 0)
         {
            /* Adjust the counters.                                     */
            LEContextInfo->SPPLEBufferInfo.TransmitCredits -= (unsigned int)Result;
         }
         else
         {
            /* Check to see what error has occurred.                    */
            if(Result == BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE)
            {
               /* Flag that the LE buffer is full.                      */
               LEContextInfo->BufferFull = TRUE;
            }
            else
               PRINTF("SEND failed with error %d\n", Result);

            Result = 0;
         }
      }
   }

   return(Result);
}

   /* The following function is responsible for handling a data         */
   /* indication event.                                                 */
static void SPPLEDataIndicationEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data)
{
   unsigned int Length;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      /* Display a Data indication event.                               */
      PRINTF("\nData Indication Event, Connection ID %u, Received %u bytes.\n", LEContextInfo->ConnectionID, DataLength);

      if((DataLength) && (Data))
      {
         /* If we are automatically reading the data, go ahead and      */
         /* credit what we just received, as well as reading everting in*/
         /* the buffer.                                                 */
         if(!AutomaticReadActive)
         {
            /* If we are displaying the data then do that here.         */
            if(DisplayRawData)
            {
               for(Length = 0; Length < DataLength; Length++)
                  PRINTF("%c", Data[Length]);
            }

            /* We are not in Automatic Read Mode so just buffer all the */
            /* data.                                                    */
            Length = AddDataToBuffer(&(LEContextInfo->SPPLEBufferInfo.ReceiveBuffer), DataLength, Data);
            if(Length != DataLength)
               PRINTF("Receive Buffer Overflow of %u bytes.\n", DataLength - Length);
         }
         else
         {
            Length = DataLength;

            /* Credit the data we just sent.                            */
            SPPLESendCredits(LEContextInfo, DeviceInfo, Length);
         }
      }
   }
}

   /* The following function is used to read data from the specified    */
   /* device.  The final two parameters specify the BufferLength and the*/
   /* Buffer to read the data into.  On success this function returns   */
   /* the number of bytes read.  If an error occurs this will return a  */
   /* negative error code.                                              */
static int SPPLEReadData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int BufferLength, Byte_t *Buffer)
{
   int          ret_val;
   Boolean_t    Done;
   unsigned int Length;
   unsigned int TotalLength;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo) && (BufferLength) && (Buffer))
   {
      Done        = FALSE;
      TotalLength = 0;
      while(!Done)
      {
         Length = RemoveDataFromBuffer(&(LEContextInfo->SPPLEBufferInfo.ReceiveBuffer), BufferLength, Buffer);
         if(Length > 0)
         {
            PRINTF("Read %d bytes\n", Length);
            BufferLength -= Length;
            Buffer       += Length;
            TotalLength  += Length;
         }
         else
            Done = TRUE;
      }

      /* Credit what we read.                                           */
      if(TotalLength)
      {
         SPPLESendCredits(LEContextInfo, DeviceInfo, TotalLength);
         PRINTF("Send %d Credits\n", TotalLength);
      }

      /* Return the total number of bytes read.                         */
      ret_val = (int)TotalLength;
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
         NumScanEntries = 0;
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
         DisplayScanEntries(NULL);
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
static int ConnectLEDevice(unsigned int BluetoothStackID, GAP_LE_Address_Type_t Address_Type, BD_ADDR_t BD_ADDR, Boolean_t UseWhiteList)
{
   int                            Result;
   int                            LEConnectionIndex;
   unsigned int                   WhiteListChanged;
   GAP_LE_White_List_Entry_t      WhiteListEntry;
   GAP_LE_Connection_Parameters_t ConnectionParameters;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
   {
        /* Make sure that there are available connections               */
      if((LEConnectionIndex = FindFreeLEIndex()) >= 0)
      {
          /* Check to see if the device is already connected.           */
          if(FindLEIndexByAddress(BD_ADDR) == -1)
          {
            /* Remove any previous entries for this device from the     */
            /* White List.                                              */
            WhiteListEntry.Address_Type = Address_Type;
            WhiteListEntry.Address      = BD_ADDR;

            GAP_LE_Remove_Device_From_White_List(BluetoothStackID, 1, &WhiteListEntry, &WhiteListChanged);

            if(UseWhiteList)
               Result = GAP_LE_Add_Device_To_White_List(BluetoothStackID, 1, &WhiteListEntry, &WhiteListChanged);
            else
               Result = 1;

            /* If everything has been successful, up until this point,  */
            /* then go ahead and attempt the connection.                */
            if(Result >= 0)
            {
               ConnectionParameters.Connection_Interval_Min   = ConnectParams.ConnectIntMin;
               ConnectionParameters.Connection_Interval_Max   = ConnectParams.ConnectIntMax;
               ConnectionParameters.Slave_Latency             = ConnectParams.SlaveLatency;
               ConnectionParameters.Minimum_Connection_Length = ConnectParams.MinConnectLength;
               ConnectionParameters.Maximum_Connection_Length = ConnectParams.MaxConnectLength;
               ConnectionParameters.Supervision_Timeout       = ConnectParams.SupervisionTO;

               /* Everything appears correct, go ahead and attempt to   */
               /* make the connection.                                  */
               Result = GAP_LE_Create_Connection(BluetoothStackID, 100, 100, Result?fpNoFilter:fpWhiteList, Address_Type, Result?&BD_ADDR:NULL, latPublic, &ConnectionParameters, GAP_LE_Event_Callback, 0);

               if(!Result)
               {
                  PRINTF("Connection Request successful.\n");

                  /* Add to an unused position in the connection info   */
                  /* array.                                             */
                  LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR = BD_ADDR;
               }
               else
               {
                  /* Unable to create connection.                       */
                  PRINTF("Unable to create connection: %d.\n", Result);
               }
            }
            else
            {
               /* Unable to add device to White List.                   */
               PRINTF("Unable to add device to White List.\n");
            }
         }
         else
         {
            /* Device already connected.                                */
            PRINTF("Device is already connected.\n");

            Result = -2;
         }
      }
      else
      {
         /* MAX_LE_CONNECTIONS reached                                  */
         PRINTF("Connection limit reached. No connections available.\n");
         Result = -3;
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
      /* Make sure that a device with address BD_ADDR is connected      */
      if(FindLEIndexByAddress(BD_ADDR) >= 0)
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
         CurrentLERemoteBD_ADDR = BD_ADDR;

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
               PRINTF("   Error - SM_Generate_Long_Term_Key returned %d.\n", ret_val);
            }
         }
         else
         {
            PRINTF("   Error - SM_Generate_Long_Term_Key returned %d.\n", ret_val);
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

   /* The following function is a utility function that exists to delete*/
   /* the specified Link Key from the Local Bluetooth Device.  If a NULL*/
   /* Bluetooth Device Address is specified, then all Link Keys will be */
   /* deleted.                                                          */
static int DeleteLinkKey(BD_ADDR_t BD_ADDR)
{
   int       Result;
   Byte_t    Status_Result;
   Word_t    Num_Keys_Deleted = 0;
   BD_ADDR_t NULL_BD_ADDR;

   Result = HCI_Delete_Stored_Link_Key(BluetoothStackID, BD_ADDR, TRUE, &Status_Result, &Num_Keys_Deleted);

   /* Any stored link keys for the specified address (or all) have been */
   /* deleted from the chip.  Now, let's make sure that our stored Link */
   /* Key Array is in sync with these changes.                          */

   /* First check to see all Link Keys were deleted.                    */
   ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   if(COMPARE_BD_ADDR(BD_ADDR, NULL_BD_ADDR))
      BTPS_MemInitialize(LinkKeyInfo, 0, sizeof(LinkKeyInfo));
   else
   {
      /* Individual Link Key.  Go ahead and see if know about the entry */
      /* in the list.                                                   */
      for(Result=0;(Result<sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Result++)
      {
         if(COMPARE_BD_ADDR(BD_ADDR, LinkKeyInfo[Result].BD_ADDR))
         {
            LinkKeyInfo[Result].BD_ADDR = NULL_BD_ADDR;

            break;
         }
      }
   }

   return(Result);
}

   /* The following function is responsible for setting the application */
   /* state to support displaying Raw Data.  This function will return  */
   /* zero on successful execution and a negative value on errors.      */
static int DisplayRawModeData(ParameterList_t *TempParam)
{
   int ret_val;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Next check to see if the parameters required for the execution */
      /* of this function appear to be semi-valid.                      */
      if((TempParam) && (TempParam->NumberofParameters > 0))
      {
         if(TempParam->Params->intParam)
            DisplayRawData = TRUE;
         else
            DisplayRawData = FALSE;
      }
      else
         DisplayRawData = (Boolean_t)(DisplayRawData?FALSE:TRUE);

      /* Output the current Raw Data Display Mode state.                */
      PRINTF("Current Raw Data Display Mode set to: %s.\n", DisplayRawData?"ACTIVE":"INACTIVE");

      /* Flag that the function was successful.                         */
      ret_val = 0;
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for setting the application */
   /* state to support Automatically reading all data that is received  */
   /* through SPP.  This function will return zero on successful        */
   /* execution and a negative value on errors.                         */
static int AutomaticReadMode(ParameterList_t *TempParam)
{
   int ret_val;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Next check to see if the parameters required for the execution */
      /* of this function appear to be semi-valid.                      */
      if((TempParam) && (TempParam->NumberofParameters > 0))
      {
         if(TempParam->Params->intParam)
            AutomaticReadActive = TRUE;
         else
            AutomaticReadActive = FALSE;
      }
      else
         AutomaticReadActive = (Boolean_t)(AutomaticReadActive?FALSE:TRUE);

      /* Output the current Automatic Read Mode state.                  */
      PRINTF("Current Automatic Read Mode set to: %s.\n", AutomaticReadActive?"ACTIVE":"INACTIVE");

      /* Flag that the function was successful.                         */
      ret_val = 0;
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for performing a General    */
   /* Inquiry for discovering Bluetooth Devices.  This function requires*/
   /* that a valid Bluetooth Stack ID exists before running.  This      */
   /* function returns zero is successful or a negative value if there  */
   /* was an error.                                                     */
static int Inquiry(ParameterList_t *TempParam)
{
   int Result;
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Use the GAP_Perform_Inquiry() function to perform an Inquiry.  */
      /* The Inquiry will last 10 seconds or until 1 Bluetooth Device is*/
      /* found.  When the Inquiry Results become available the          */
      /* GAP_Event_Callback is called.                                  */
      Result = GAP_Perform_Inquiry(BluetoothStackID, itGeneralInquiry, 0, 0, 10, MAX_INQUIRY_RESULTS, GAP_Event_Callback, (unsigned long)NULL);

      /* Next, check to see if the GAP_Perform_Inquiry() function was   */
      /* successful.                                                    */
      if(!Result)
      {
         /* The Inquiry appears to have been sent successfully.         */
         /* Processing of the results returned from this command occurs */
         /* within the GAP_Event_Callback() function.                   */

         /* Flag that we have found NO Bluetooth Devices.               */
         NumberofValidResponses = 0;

         ret_val                = 0;
      }
      else
      {
         /* A error occurred while performing the Inquiry.              */
         DisplayFunctionError("Inquiry", Result);

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

   /* The following function is a utility function that exists to       */
   /* display the current Inquiry List (with Indexes) or .  This is     */
   /* useful in case the user has forgotten what Inquiry Index a        */
   /* particular Bluetooth Device was located in.  This function returns*/
   /* zero on successful execution and a negative value on all errors.  */
static int DisplayDeviceList(ParameterList_t *TempParam)
{
   int          ret_val;
   BoardStr_t   Function_BoardStr;
   unsigned int Index;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      if(UI_Mode == UI_MODE_IS_SPP)
      {
         /* Simply display all of the items in the Inquiry List.        */
         PRINTF("Device List: %d Devices%s\n\n", NumberofValidResponses, NumberofValidResponses?":":".");

         for(Index=0;Index<NumberofValidResponses;Index++)
         {
            BD_ADDRToStr(InquiryResultList[Index], Function_BoardStr);

            PRINTF(" Inquiry Result: %d, %s.\n", (Index+1), Function_BoardStr);
         }

         if(NumberofValidResponses)
            PRINTF("\n");

         /* All finished, flag success to the caller.                   */
         ret_val = 0;
      }
      if(UI_Mode == UI_MODE_IS_SPPLE)
      {
         /* Simply display all of the items in the Inquiry List.        */
         PRINTF("Connected Device List\n");

         ret_val = 0;
         for(Index=0;Index<MAX_LE_CONNECTIONS;Index++)
         {
            if(LEContextInfo[Index].ConnectionID)
            {
               ret_val++;
               BD_ADDRToStr(LEContextInfo[Index].ConnectionBD_ADDR, Function_BoardStr);
               PRINTF(" Device %d, %s.\n", (ret_val), Function_BoardStr);
            }
         }

         if(!ret_val)
            PRINTF("No connected Devices\n");

         PRINTF("\n");

         /* All finished, flag success to the caller.                   */
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

   /* The following function is responsible for setting the             */
   /* Discoverability Mode of the local device.  This function returns  */
   /* zero on successful execution and a negative value on all errors.  */
static int SetCBDiscoverabilityMode(ParameterList_t *TempParam)
{
   int                        Result;
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

         /* Parameters mapped, now set the Discoverability Mode.        */
         Result = GAP_Set_Discoverability_Mode(BluetoothStackID, DiscoverabilityMode, (DiscoverabilityMode == dmLimitedDiscoverableMode)?60:0);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            PRINTF("Discoverability: %s.\n", (DiscoverabilityMode == dmNonDiscoverableMode)?"Non":((DiscoverabilityMode == dmGeneralDiscoverableMode)?"General":"Limited"));

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            DisplayFunctionError("GAP_Set_Discoverability_Mode", Result);

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
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
static int SetCBConnectabilityMode(ParameterList_t *TempParam)
{
   int                       Result;
   int                       ret_val;
   GAP_Connectability_Mode_t ConnectableMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 1))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
            ConnectableMode = cmNonConnectableMode;
         else
            ConnectableMode = cmConnectableMode;

         /* Parameters mapped, now set the Connectabilty Mode.          */
         Result = GAP_Set_Connectability_Mode(BluetoothStackID, ConnectableMode);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            PRINTF("Connectability Mode: %s.\n", (ConnectableMode == cmNonConnectableMode)?"Non Connectable":"Connectable");

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error setting the Mode.                     */
            DisplayFunctionError("GAP_Set_Connectability_Mode", Result);

            /* Flag that an error occurred while submitting the command.*/
            ret_val = FUNCTION_ERROR;
         }
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
static int SetCBPairabilityMode(ParameterList_t *TempParam)
{
   int                    Result;
   int                    ret_val;
   char                   *Mode;
   GAP_Pairability_Mode_t PairabilityMode;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 2))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
         {
            PairabilityMode = pmNonPairableMode;
            Mode            = "pmNonPairableMode";
         }
         else
         {
            if(TempParam->Params[0].intParam == 1)
            {
               PairabilityMode = pmPairableMode;
               Mode            = "pmPairableMode";
            }
            else
            {
               PairabilityMode = pmPairableMode_EnableSecureSimplePairing;
               Mode            = "pmPairableMode_EnableSecureSimplePairing";
            }
         }

         /* Parameters mapped, now set the Pairability Mode.            */
         Result = GAP_Set_Pairability_Mode(BluetoothStackID, PairabilityMode);

         /* Next, check the return value to see if the command was      */
         /* issued successfully.                                        */
         if(Result >= 0)
         {
            /* The Mode was changed successfully.                       */
            PRINTF("Pairability Mode Changed to %s.\n", Mode);

            /* If Secure Simple Pairing has been enabled, inform the    */
            /* user of the current Secure Simple Pairing parameters.    */
            if(PairabilityMode == pmPairableMode_EnableSecureSimplePairing)
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
         DisplayUsage("SetPairabilityMode [Mode (0 = Non Pairable, 1 = Pairable, 2 = Pairable (Secure Simple Pairing)]");

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

   /* The following function is responsible for changing the Secure     */
   /* Simple Pairing Parameters that are exchanged during the Pairing   */
   /* procedure when Secure Simple Pairing (Security Level 4) is used.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int ChangeSimplePairingParameters(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters >= 2) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam <= 3))
      {
         /* Parameters appear to be valid, map the specified parameters */
         /* into the API specific parameters.                           */
         if(TempParam->Params[0].intParam == 0)
            IOCapability = icDisplayOnly;
         else
         {
            if(TempParam->Params[0].intParam == 1)
               IOCapability = icDisplayYesNo;
            else
            {
               if(TempParam->Params[0].intParam == 2)
                  IOCapability = icKeyboardOnly;
               else
                  IOCapability = icNoInputNoOutput;
            }
         }

         /* Finally map the Man in the Middle (MITM) Protection value.  */
         MITMProtection = (Boolean_t)(TempParam->Params[1].intParam?TRUE:FALSE);

         /* Inform the user of the New I/O Capabilities.                */
         DisplayIOCapabilities();

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("ChangeSimplePairingParameters [I/O Capability (0 = Display Only, 1 = Display Yes/No, 2 = Keyboard Only, 3 = No Input/Output)] [MITM Requirement (0 = No, 1 = Yes)]");

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

   /* The following function is responsible for initiating bonding with */
   /* a remote device.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int Pair(ParameterList_t *TempParam)
{
   int                Result;
   int                ret_val;
   GAP_Bonding_Type_t BondingType;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* There are currently no active connections, make sure that all  */
      /* of the parameters required for this function appear to be at   */
      /* least semi-valid.                                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= (int)NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
      {
         /* Check to see if General Bonding was specified.              */
         if(TempParam->NumberofParameters > 1)
            BondingType = TempParam->Params[1].intParam?btGeneral:btDedicated;
         else
            BondingType = btDedicated;

         /* Verify that we are not connected to device already.         */
         if(FindSPPPortIndexByAddress(InquiryResultList[(TempParam->Params[0].intParam - 1)]) < 0)
         {
            /* Before we submit the command to the stack, we need to    */
            /* make sure that we clear out any Link Key we have stored  */
            /* for the specified device.                                */
            DeleteLinkKey(InquiryResultList[(TempParam->Params[0].intParam - 1)]);

            /* Attempt to submit the command.                           */
            Result = GAP_Initiate_Bonding(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], BondingType, GAP_Event_Callback, (unsigned long)0);

            /* Check the return value of the submitted command for      */
            /* success.                                                 */
            if(!Result)
            {
               /* Display a message indicating that Bonding was         */
               /* initiated successfully.                               */
               PRINTF("GAP_Initiate_Bonding(%s): Success.\n", (BondingType == btDedicated)?"Dedicated":"General");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Display a message indicating that an error occurred   */
               /* while initiating bonding.                             */
               DisplayFunctionError("GAP_Initiate_Bonding", Result);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            /* Display an error to the user describing that Pairing can */
            /* only occur when we are not connected.                    */
            PRINTF("Only valid when not connected.\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("Pair [Inquiry Index] [0 = Dedicated, 1 = General (optional)]");

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

   /* The following function is responsible for ending a previously     */
   /* initiated bonding session with a remote device.  This function    */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int EndPairing(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= (int)NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
      {
         /* Attempt to submit the command.                              */
         Result = GAP_End_Bonding(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)]);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that the End bonding was    */
            /* successfully submitted.                                  */
            DisplayFunctionSuccess("GAP_End_Bonding");

            /* Flag success to the caller.                              */
            ret_val = 0;

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* ending bonding.                                          */
            DisplayFunctionError("GAP_End_Bonding", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("EndPairing [Inquiry Index]");

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
   /* Authentication Response with a PIN Code value specified via the   */
   /* input parameter.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int PINCodeResponse(ParameterList_t *TempParam)
{
   int                              Result;
   int                              ret_val;
   PIN_Code_t                       PINCode;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentCBRemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].strParam) && (BTPS_StringLength(TempParam->Params[0].strParam) > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= sizeof(PIN_Code_t)))
         {
            /* Parameters appear to be valid, go ahead and convert the  */
            /* input parameter into a PIN Code.                         */

            /* Initialize the PIN code.                                 */
            ASSIGN_PIN_CODE(PINCode, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

            BTPS_MemCopy(&PINCode, TempParam->Params[0].strParam, BTPS_StringLength(TempParam->Params[0].strParam));

            /* Populate the response structure.                         */
            GAP_Authentication_Information.GAP_Authentication_Type      = atPINCode;
            GAP_Authentication_Information.Authentication_Data_Length   = (Byte_t)(BTPS_StringLength(TempParam->Params[0].strParam));
            GAP_Authentication_Information.Authentication_Data.PIN_Code = PINCode;

            /* Submit the Authentication Response.                      */
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentCBRemoteBD_ADDR, &GAP_Authentication_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               PRINTF("GAP_Authentication_Response(), Pin Code Response Success.\n");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               PRINTF("GAP_Authentication_Response() Failure: %d.\n", Result);

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            DisplayUsage("PINCodeResponse [PIN Code]");

            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         PRINTF("PIN Code Authentication Response: Authentication not in progress.\n");

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

   /* The following function is responsible for issuing a GAP           */
   /* Authentication Response with a Pass Key value specified via the   */
   /* input parameter.  This function returns zero on successful        */
   /* execution and a negative value on all errors.                     */
static int PassKeyResponse(ParameterList_t *TempParam)
{
   int                              Result;
   int                              ret_val;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentCBRemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0) && (BTPS_StringLength(TempParam->Params[0].strParam) <= GAP_PASSKEY_MAXIMUM_NUMBER_OF_DIGITS))
         {
            /* Parameters appear to be valid, go ahead and populate the */
            /* response structure.                                      */
            GAP_Authentication_Information.GAP_Authentication_Type     = atPassKey;
            GAP_Authentication_Information.Authentication_Data_Length  = (Byte_t)(sizeof(DWord_t));
            GAP_Authentication_Information.Authentication_Data.Passkey = (DWord_t)(TempParam->Params[0].intParam);

            /* Submit the Authentication Response.                      */
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentCBRemoteBD_ADDR, &GAP_Authentication_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               DisplayFunctionSuccess("Passkey Response Success");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               DisplayFunctionError("GAP_Authentication_Response", Result);

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            PRINTF("PassKeyResponse[Numeric Passkey(0 - 999999)].\n");

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

   /* The following function is responsible for issuing a GAP           */
   /* Authentication Response with a User Confirmation value specified  */
   /* via the input parameter.  This function returns zero on successful*/
   /* execution and a negative value on all errors.                     */
static int UserConfirmationResponse(ParameterList_t *TempParam)
{
   int                              Result;
   int                              ret_val;
   GAP_Authentication_Information_t GAP_Authentication_Information;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* First, check to see if there is an on-going Pairing operation  */
      /* active.                                                        */
      if(!COMPARE_NULL_BD_ADDR(CurrentCBRemoteBD_ADDR))
      {
         /* Make sure that all of the parameters required for this      */
         /* function appear to be at least semi-valid.                  */
         if((TempParam) && (TempParam->NumberofParameters > 0))
         {
            /* Parameters appear to be valid, go ahead and populate the */
            /* response structure.                                      */
            GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
            GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)(sizeof(Byte_t));
            GAP_Authentication_Information.Authentication_Data.Confirmation = (Boolean_t)(TempParam->Params[0].intParam?TRUE:FALSE);

            /* Submit the Authentication Response.                      */
            Result = GAP_Authentication_Response(BluetoothStackID, CurrentCBRemoteBD_ADDR, &GAP_Authentication_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               DisplayFunctionSuccess("User Confirmation Response");

               /* Flag success to the caller.                           */
               ret_val = 0;
            }
            else
            {
               /* Inform the user that the Authentication Response was  */
               /* not successful.                                       */
               DisplayFunctionError("GAP_Authentication_Response", Result);

               ret_val = FUNCTION_ERROR;
            }

            /* Flag that there is no longer a current Authentication    */
            /* procedure in progress.                                   */
            ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
         }
         else
         {
            /* One or more of the necessary parameters is/are invalid.  */
            DisplayUsage("UserConfirmationResponse [Confirmation(0 = No, 1 = Yes)]");
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* There is not currently an on-going authentication operation,*/
         /* inform the user of this error condition.                    */
         PRINTF("User Confirmation Authentication Response: Authentication is not currently in progress.\n");

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

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Address of the local Bluetooth Device.  This function      */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int GetLocalAddress(ParameterList_t *TempParam)
{
   int        Result;
   int        ret_val;
   BD_ADDR_t  BD_ADDR;
   BoardStr_t Function_BoardStr;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR);

      /* Check the return value of the submitted command for success.   */
      if(!Result)
      {
         BD_ADDRToStr(BD_ADDR, Function_BoardStr);

         PRINTF("BD_ADDR of Local Device is: %s.\n", Function_BoardStr);

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

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Name of the local Bluetooth Device.  This function returns */
   /* zero on successful execution and a negative value on all errors.  */
static int GetLocalName(ParameterList_t *TempParam)
{
   int  Result;
   int  ret_val;
   char *LocalName;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Allocate a Buffer to hold the Local Name.                      */
      if((LocalName = BTPS_AllocateMemory(257)) != NULL)
      {
         /* Attempt to submit the command.                              */
         Result = GAP_Query_Local_Device_Name(BluetoothStackID, 257, (char *)LocalName);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            PRINTF("Name of Local Device is: %s.\n", LocalName);

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* attempting to query the Local Device Name.               */
            PRINTF("GAP_Query_Local_Device_Name() Failure: %d.\n", Result);

            ret_val = FUNCTION_ERROR;
         }

         BTPS_FreeMemory(LocalName);
      }
      else
      {
         PRINTF("Failed to allocate buffer to hold Local Name.\n");

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

   /* The following function is responsible for setting the Class of    */
   /* Device of the local Bluetooth Device to a Class Of Device value.  */
   /* This function returns zero on successful execution and a negative */
   /* value on all errors.                                              */
static int SetClassOfDevice(ParameterList_t *TempParam)
{
   int               Result;
   int               ret_val;
   Class_of_Device_t Class_of_Device;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0))
      {
         /* Attempt to submit the command.                              */
         ASSIGN_CLASS_OF_DEVICE(Class_of_Device, (Byte_t)((TempParam->Params[0].intParam) & 0xFF), (Byte_t)(((TempParam->Params[0].intParam) >> 8) & 0xFF), (Byte_t)(((TempParam->Params[0].intParam) >> 16) & 0xFF));

         Result = GAP_Set_Class_Of_Device(BluetoothStackID, Class_of_Device);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that the Class of Device was*/
            /* successfully submitted.                                  */
            DisplayClassOfDevice(Class_of_Device);

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* attempting to set the local Class of Device.             */
            DisplayFunctionError("GAP_Set_Class_Of_Device", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("SetClassOfDevice [Class of Device]");

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
   /* Class of Device of the local Bluetooth Device.  This function     */
   /* returns zero on successful execution and a negative value on all  */
   /* errors.                                                           */
static int GetClassOfDevice(ParameterList_t *TempParam)
{
   int               Result;
   int               ret_val;
   Class_of_Device_t Class_of_Device;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Attempt to submit the command.                                 */
      Result = GAP_Query_Class_Of_Device(BluetoothStackID, &Class_of_Device);

      /* Check the return value of the submitted command for success.   */
      if(!Result)
      {
         PRINTF("Local Class of Device is: 0x%02X%02X%02X.\n", Class_of_Device.Class_of_Device0, Class_of_Device.Class_of_Device1, Class_of_Device.Class_of_Device2);

         /* Flag success to the caller.                                 */
         ret_val = 0;
      }
      else
      {
         /* Display a message indicating that an error occurred while   */
         /* attempting to query the Local Class of Device.              */
         PRINTF("GAP_Query_Class_Of_Device() Failure: %d.\n", Result);

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

   /* The following function is responsible for querying the Bluetooth  */
   /* Device Name of the specified remote Bluetooth Device.  This       */
   /* function returns zero on successful execution and a negative value*/
   /* on all errors.                                                    */
static int GetRemoteName(ParameterList_t *TempParam)
{
   int       Result;
   int       ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Make sure that all of the parameters required for this function*/
      /* appear to be at least semi-valid.                              */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= (int)NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])))
      {
         /* Attempt to submit the command.                              */
         Result = GAP_Query_Remote_Device_Name(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], GAP_Event_Callback, (unsigned long)0);

         /* Check the return value of the submitted command for success.*/
         if(!Result)
         {
            /* Display a message indicating that Remote Name request was*/
            /* initiated successfully.                                  */
            DisplayFunctionSuccess("GAP_Query_Remote_Device_Name");

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* Display a message indicating that an error occurred while*/
            /* initiating the Remote Name request.                      */
            DisplayFunctionError("GAP_Query_Remote_Device_Name", Result);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         DisplayUsage("GetRemoteName [Inquiry Index]");

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

   /* The following function is responsible for issuing a Service Search*/
   /* Attribute Request to a Remote SDP Server.  This function returns  */
   /* zero if successful and a negative value if an error occurred.     */
static int ServiceDiscovery(ParameterList_t *TempParam)
{
   int                           Result;
   int                           ret_val;
   int                           Index;
   BD_ADDR_t                     NullADDR;
   SDP_UUID_Entry_t              SDPUUIDEntry;
   SDP_Attribute_ID_List_Entry_t AttributeID;

   ASSIGN_BD_ADDR(NullADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Now let's make sure that all of the parameters required for    */
      /* this function appear to be at least semi-valid.                */
      if((TempParam) && (TempParam->NumberofParameters > 1) && (((TempParam->Params[1].intParam) && (TempParam->Params[1].intParam <= (int)NUM_UUIDS)) || ((!TempParam->Params[1].intParam) && (TempParam->NumberofParameters > 2))) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= (int)NumberofValidResponses) && (!COMPARE_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)], NullADDR)))
      {
         /* OK, parameters appear to be semi-valid, now let's attempt to*/
         /* issue the SDP Service Attribute Request.                    */
         if(!TempParam->Params[1].intParam)
         {
            /* First let's build the UUID 32 value(s).                  */
            SDPUUIDEntry.SDP_Data_Element_Type = deUUID_32;

            ASSIGN_SDP_UUID_32(SDPUUIDEntry.UUID_Value.UUID_32, (Byte_t)((TempParam->Params[2].intParam & 0xFF000000) >> 24), (Byte_t)((TempParam->Params[2].intParam & 0x00FF0000) >> 16), (Byte_t)((TempParam->Params[2].intParam & 0x0000FF00) >> 8), (Byte_t)(TempParam->Params[2].intParam & 0x000000FF));
         }
         else
         {
            SDPUUIDEntry.SDP_Data_Element_Type = deUUID_128;

            SDPUUIDEntry.UUID_Value.UUID_128   = UUIDTable[TempParam->Params[1].intParam - 1].UUID;
         }

         AttributeID.Attribute_Range    = (Boolean_t)TRUE;
         AttributeID.Start_Attribute_ID = 0;
         AttributeID.End_Attribute_ID   = 65335;

         /* Finally submit the SDP Request.                             */
         Result = SDP_Service_Search_Attribute_Request(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], 1, &SDPUUIDEntry, 1, &AttributeID, SDP_Event_Callback, (unsigned long)0);

         if(Result > 0)
         {
            /* The SDP Request was submitted successfully.              */
            PRINTF("SDP_Service_Search_Attribute_Request(%s) Success.\n", TempParam->Params[1].intParam?UUIDTable[TempParam->Params[1].intParam - 1].Name:"Manual");

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
         else
         {
            /* There was an error submitting the SDP Request.           */
            PRINTF("SDP_Service_Search_Attribute_Request(%s) Failure: %d.\n", TempParam->Params[1].intParam?UUIDTable[TempParam->Params[1].intParam - 1].Name:"Manual", Result);

            /* Flag success to the caller.                              */
            ret_val = 0;
         }
      }
      else
      {
         /* One or more of the necessary parameters is/are invalid.     */
         PRINTF("Usage: ServiceDiscovery [Inquiry Index] [Profile Index] [16/32 bit UUID (Manual only)].\n");
         PRINTF("\n   Profile Index:\n");
         PRINTF("       0) Manual (MUST specify 16/32 bit UUID)\n");
         for(Index=0;Index<NUM_UUIDS;Index++)
            PRINTF("      %2d) %s\n", Index + 1, UUIDTable[Index].Name);
         PRINTF("\n");

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

   /* The following function is responsible for opening a Serial Port   */
   /* Server on the Local Device.  This function opens the Serial Port  */
   /* Server on the specified RFCOMM Channel.  This function returns    */
   /* zero if successful, or a negative return value if an error        */
   /* occurred.                                                         */
static int OpenServer(ParameterList_t *TempParam)
{
   int           SerialPortIndex;
   int           ret_val;
   char         *ServiceName;
   DWord_t       SPPServerSDPHandle;
   unsigned int  ServerPortID;
   unsigned int  ServerPortNumber;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Next, check to see if the parameters specified are valid.      */
      if((TempParam) && (TempParam->NumberofParameters >= 1) && (TempParam->Params[0].intParam))
      {
         /* Find an empty slot.                                         */
         if((SerialPortIndex = FindFreeSPPPortIndex()) >= 0)
         {
            /* Save the server port number.                             */
            ServerPortNumber = TempParam->Params[0].intParam;

            /* Simply attempt to open an Serial Server, on RFCOMM Server*/
            /* Port 1.                                                  */
            ret_val = SPP_Open_Server_Port(BluetoothStackID, ServerPortNumber, SPP_Event_Callback, (unsigned long)0);

            /* If the Open was successful, then note the Serial Port    */
            /* Server ID.                                               */
            if(ret_val > 0)
            {
               /* Note the Serial Port Server ID of the opened Serial   */
               /* Port Server.                                          */
               ServerPortID = (unsigned int)ret_val;

               /* Create a Buffer to hold the Service Name.             */
               if((ServiceName = BTPS_AllocateMemory(64)) != NULL)
               {
                  /* The Server was opened successfully, now register a */
                  /* SDP Record indicating that an Serial Port Server   */
                  /* exists. Do this by first creating a Service Name.  */
                  BTPS_SprintF(ServiceName, "Serial Port Server Port %d", ServerPortNumber);

                  /* Only register an SDP record if we have not already */
                  /* opened a server on this port number.               */
                  if(FindSPPPortIndexByServerPortNumber(ServerPortNumber) < 0)
                  {
                     /* Now that a Service Name has been created try to */
                     /* Register the SDP Record.                        */
                     ret_val = SPP_Register_Generic_SDP_Record(BluetoothStackID, ServerPortID, ServiceName, &SPPServerSDPHandle);
                  }
                  else
                  {
                     /* We already have opened another SPP Port on that */
                     /* RFCOMM Port Number so there is no need to       */
                     /* register a duplicate SDP Record.                */
                     SPPServerSDPHandle = 0;
                     ret_val            = 0;
                  }

                  /* If there was an error creating the Serial Port     */
                  /* Server's SDP Service Record then go ahead an close */
                  /* down the server an flag an error.                  */
                  if(ret_val < 0)
                  {
                     PRINTF("Unable to Register Server SDP Record, Error = %d.\n", ret_val);

                     SPP_Close_Server_Port(BluetoothStackID, ServerPortID);

                     ret_val      = UNABLE_TO_REGISTER_SERVER;
                  }
                  else
                  {
                     /* Simply flag to the user that everything         */
                     /* initialized correctly.                          */
                     PRINTF("Server Opened: Server Port %u, Serial Port ID %u.\n", (unsigned int)TempParam->Params[0].intParam, ServerPortID);

                     /* We found an empty slot to store the context     */
                     SPPContextInfo[SerialPortIndex].LocalSerialPortID  = ServerPortID;
                     SPPContextInfo[SerialPortIndex].ServerPortNumber   = ServerPortNumber;
                     SPPContextInfo[SerialPortIndex].SPPServerSDPHandle = SPPServerSDPHandle;
                     SPPContextInfo[SerialPortIndex].Connected          = FALSE;

                     /* If this message is not seen in the terminal log */
                     /* after opening a server, then something went     */
                     /* wrong                                           */
                     PRINTF("Server Port Context Stored.\n");

                     /* Flag success to the caller.                     */
                     ret_val = 0;
                  }

                  /* Free the Service Name buffer.                      */
                  BTPS_FreeMemory(ServiceName);
               }
               else
               {
                  PRINTF("Failed to allocate buffer to hold Service Name in SDP Record.\n");
               }
            }
            else
            {
               PRINTF("Unable to Open Server on: %d, Error = %d.\n", TempParam->Params[0].intParam, ret_val);

               ret_val = UNABLE_TO_REGISTER_SERVER;
            }
         }
         else
         {
            /* Maximum number of ports reached.                         */
            PRINTF("Maximum allowed server ports open.\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         DisplayUsage("Open [Port Number]");

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

   /* The following function is responsible for closing a Serial Port   */
   /* Server that was previously opened via a successful call to the    */
   /* OpenServer() function.  This function returns zero if successful  */
   /* or a negative return error code if there was an error.            */
static int CloseServer(ParameterList_t *TempParam)
{
   int i;
   int ret_val = 0;
   int LocalSerialPortID;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Figure out the server port that corresponds to the portID      */
      /* requested by the user.                                         */
      if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam))
      {
         LocalSerialPortID = TempParam->Params[0].intParam;
         ClosePortByNumber(LocalSerialPortID);
      }
      else
      {
         /* If no port is specified to close then we should close them  */
         /* all.                                                        */
         if((!TempParam) || ((TempParam) && (TempParam->NumberofParameters == 0)))
         {
            for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
            {
               /* If a server is register then close the port.          */
               if((SPPContextInfo[i].LocalSerialPortID) && (SPPContextInfo[i].ServerPortNumber))
                  ClosePortByNumber(SPPContextInfo[i].LocalSerialPortID);
            }
         }
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for initiating a connection */
   /* with a Remote Serial Port Server.  This function returns zero if  */
   /* successful and a negative value if an error occurred.             */
static int OpenRemoteServer(ParameterList_t *TempParam)
{
   int ret_val;
   int Result;
   int SerialPortIndex;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, let's make sure that the user has specified a Remote     */
      /* Bluetooth Device to open.                                      */
      if((TempParam) && (TempParam->NumberofParameters > 1) && (TempParam->Params[0].intParam) && (NumberofValidResponses) && (TempParam->Params[0].intParam <= (int)NumberofValidResponses) && (!COMPARE_NULL_BD_ADDR(InquiryResultList[(TempParam->Params[0].intParam - 1)])) && (TempParam->Params[1].intParam))
      {
         /* Find a free serial port entry.                              */
         if((SerialPortIndex = FindFreeSPPPortIndex()) >= 0)
         {
            /* Now let's attempt to open the Remote Serial Port Server. */
            Result = SPP_Open_Remote_Port(BluetoothStackID, InquiryResultList[(TempParam->Params[0].intParam - 1)], TempParam->Params[1].intParam, SPP_Event_Callback, (unsigned long)0);
            if(Result > 0)
            {
               /* Inform the user that the call to open the Remote      */
               /* Serial Port Server was successful.                    */
               PRINTF("SPP_Open_Remote_Port success, Serial Port ID = %u.\n", (unsigned int)Result);

               /* Note the Serial Port Client ID and Bluetooth Address. */
               SPPContextInfo[SerialPortIndex].LocalSerialPortID = (unsigned int)Result;
               SPPContextInfo[SerialPortIndex].BD_ADDR           = InquiryResultList[(TempParam->Params[0].intParam - 1)];

               /* Flag success to the caller.                           */
               ret_val                                          = 0;
            }
            else
            {
               /* Inform the user that the call to Open the Remote      */
               /* Serial Port Server failed.                            */
               DisplayFunctionError("SPP_Open_Remote_Port", Result);

               /* One or more of the necessary parameters is/are        */
               /* invalid.                                              */
               ret_val = INVALID_PARAMETERS_ERROR;
            }
         }
         else
         {
            /* Maximum client ports reached.                            */
            PRINTF("Maximum allowed client ports open.\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         DisplayUsage("Open [Inquiry Index] [RFCOMM Server Port].\n");

         /* One or more of the necessary parameters is/are invalid.     */
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

   /* The following function is responsible for terminating a connection*/
   /* with a Remote Serial Port Server.  This function returns zero if  */
   /* successful and a negative value if an error occurred.             */
static int CloseRemoteServer(ParameterList_t *TempParam)
{
   int          i;
   int          ret_val = 0;
   unsigned int LocalSerialPortID;

   /* First check to see if a valid Bluetooth Stack ID exists.          */
   if(BluetoothStackID)
   {
      /* Figure out the server port that corresponds to the portID      */
      /* requested by the user.                                         */
      if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam))
      {
         LocalSerialPortID = TempParam->Params[0].intParam;
         ClosePortByNumber(LocalSerialPortID);
      }
      else
      {
         /* If no port is specified to close then we should close them  */
         /* all.                                                        */
         if((!TempParam) || ((TempParam) && (TempParam->NumberofParameters == 0)))
         {
            for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
            {
               /* Attempt to close this port.                           */
               if(SPPContextInfo[i].LocalSerialPortID)
                  ClosePortByNumber(SPPContextInfo[i].LocalSerialPortID);
            }
         }
      }
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for reading data that was   */
   /* received via an Open SPP port.  The function reads a fixed number */
   /* of bytes at a time from the SPP Port and displays it. If the call */
   /* to the SPP_Data_Read() function is successful but no data is      */
   /* available to read the function displays "No data to read.".  This */
   /* function requires that a valid Bluetooth Stack ID and Serial Port */
   /* ID exist before running.  This function returns zero if successful*/
   /* and a negative value if an error occurred.                        */
static int Read(ParameterList_t *TempParam)
{
   int          ret_val;
   int          Result;
   int          SerialPortIndex;
   char         Buffer[32];
   unsigned int LocalSerialPortID;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Only allow the Read Command if we are not in Display Raw Data  */
      /* Mode.                                                          */
      if(!DisplayRawData)
      {
         /* Verify that the parameters to this function are valid.      */
         if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam))
         {
            /* Save the selected Serial Port ID.                        */
            LocalSerialPortID = (unsigned int)TempParam->Params[0].intParam;

            /* Find the serial port entry for this port.                */
            if(((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0) && (SPPContextInfo[SerialPortIndex].Connected == TRUE))
            {
               /* The required parameters appear to be semi-valid, send */
               /* the command to Read Data from SPP.                    */
               do
               {
                  /* Attempt to read data from the buffer.              */
                  Result = SPP_Data_Read(BluetoothStackID, SPPContextInfo[SerialPortIndex].LocalSerialPortID, (Word_t)(sizeof(Buffer)-1), (Byte_t*)&Buffer);

                  /* Next, check the return value to see if the command */
                  /* was successfully.                                  */
                  if(Result >= 0)
                  {
                     /* Null terminate the read data.                   */
                     Buffer[Result] = 0;

                     /* Data was read successfully, the result indicates*/
                     /* the of bytes that were successfully Read.       */
                     PRINTF("Read: %d.\n", Result);

                     if(Result > 0)
                        PRINTF("Message: %s\n", Buffer);

                     ret_val = 0;
                  }
                  else
                  {
                     /* An error occurred while reading from SPP.       */
                     DisplayFunctionError("SPP_Data_Read Failure", Result);

                     ret_val = Result;
                  }
               } while(Result > 0);
            }
            else
            {
               /* Maximum client ports reached.                         */
               PRINTF("Port is invalid or not connected.\n");

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            DisplayUsage("Read [SerialPortID].\n");

            /* One or more of the necessary parameters is/are invalid.  */
            ret_val = INVALID_PARAMETERS_ERROR;
         }
      }
      else
      {
         /* Simply inform the user that this command is not available in*/
         /* this mode.                                                  */
         PRINTF("This operation cannot be performed while in Displaying Raw Data.\n");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for Writing Data to an Open */
   /* SPP Port.  The string that is written is defined by the constant  */
   /* TEST_DATA (at the top of this file).  This function requires that */
   /* a valid Bluetooth Stack ID and Serial Port ID exist before        */
   /* running.  This function returns zero is successful or a negative  */
   /* return value if there was an error.                               */
static int Write(ParameterList_t *TempParam)
{
   int          ret_val;
   int          Result;
   int          SerialPortIndex;
   unsigned int LocalSerialPortID;

   /* First check to see if the parameters required for the execution of*/
   /* this function appear to be semi-valid.                            */
   if(BluetoothStackID)
   {
      /* Verify that the parameters to this function are valid.         */
      if((TempParam) && (TempParam->NumberofParameters) && (TempParam->Params[0].intParam) )
      {
         /* Save the selected Serial Port ID.                           */
        LocalSerialPortID = TempParam->Params[0].intParam;

        /* Find the serial port entry for this port.                    */
        if(((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0) && (SPPContextInfo[SerialPortIndex].Connected == TRUE))
        {
           /* Simply write out the default string value.                */
           Result = SPP_Data_Write(BluetoothStackID, SPPContextInfo[SerialPortIndex].LocalSerialPortID, (Word_t)DATA_STR_LENGTH, (Byte_t *)DataStr);

           /* Next, check the return value to see if the command was    */
           /* issued successfully.                                      */
           if(Result >= 0)
           {
              /* The Data was written successfully, Result indicates the*/
              /* number of bytes successfully written.                  */
              PRINTF("Wrote: %d.\n", Result);

              /* Flag success to the caller.                            */
              ret_val = 0;
           }
           else
           {
              /* There was an error writing the Data to the SPP Port.   */
              PRINTF("Failed: %d.\n", Result);

              /* Flag that an error occurred while submitting the       */
              /* command.                                               */
              ret_val = FUNCTION_ERROR;
           }
        }
        else
        {
           /* Maximum client ports reached.                             */
           PRINTF("Port is invalid or not connected.\n");

           ret_val = FUNCTION_ERROR;
        }
      }
      else
      {
         DisplayUsage("Write [SerialPortID].\n");

         /* One or more of the necessary parameters is/are invalid.     */
         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      /* One or more of the necessary parameters are invalid.           */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for changing the User       */
   /* Interface Mode to Server.                                         */
static int Select_SPP(ParameterList_t *TempParam)
{
   UserInterface_SPP();
   DisplayHelp(NULL);

   return(0);
}

   /* The following function is responsible for changing the User       */
   /* Interface Mode to Client.                                         */
static int Select_SPPLE(ParameterList_t *TempParam)
{
   UserInterface_SPPLE();
   DisplayHelp(NULL);

   return(0);
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
   if(UI_Mode == UI_MODE_IS_SPP)
   {
      PRINTF("\n");
      PRINTF("******************************************************************\n");
      PRINTF("* Command General: Help, Quit, SPPLE,                            *\n");
      PRINTF("*                  GetLocalAddress, GetLocalName, SetLocalName,  *\n");
      PRINTF("*                  Inquiry, DisplayDeviceList                    *\n");
      PRINTF("*                  Pair, PINCodeResponse, PassKeyResponse,       *\n");
      PRINTF("*                  UserConfirmationResponse,                     *\n");
      PRINTF("*                  SetDiscoverabilityMode, SetConnectabilityMode,*\n");
      PRINTF("*                  SetPairabilityMode,                           *\n");
      PRINTF("*                  ChangeSimplePairingParameters,                *\n");
      PRINTF("*                  GetRemoteName, ServiceDiscovery,              *\n");
      PRINTF("* Command SPP:     Open, Close, OpenServer, CloseServer,         *\n");
      PRINTF("*                  Read, Write                                   *\n");
      PRINTF("******************************************************************\n");
   }
   else
   {
      if(UI_Mode == UI_MODE_IS_SPPLE)
      {
         PRINTF("\n");
         PRINTF("******************************************************************\n");
         PRINTF("* Command General: Help, Quit, SPP,                              *\n");
         PRINTF("*                  GetLocalAddress, GetLocalName, SetLocalName,  *\n");
         PRINTF("*                  DisplayDeviceList, DisplayScanList            *\n");
         PRINTF("* Command GAPLE:   SetDiscoverabilityMode, SetConnectabilityMode,*\n");
         PRINTF("*                  SetPairabilityMode, ChangePairingParameters,  *\n");
         PRINTF("*                  Advertise, Scan, Connect, Disconnect, Pair,   *\n");
         PRINTF("*                  LEPasskeyResponse,                            *\n");
         PRINTF("*                  DiscoverDIS, DiscoverGAPS, GetRemoteName,     *\n");
         PRINTF("*                  SetLocalAppearance, GetLocalAppearance,       *\n");
         PRINTF("*                  GetRemoteAppearance, SetLEParams              *\n");
         PRINTF("* Command LETP:    DiscoverLETP, RegisterLETP, UnregisterLETP,   *\n");
         PRINTF("*                  EnableLETP, SetLETPInterval, SetLETPCount,    *\n");
         PRINTF("*                  TimeLETP,                                     *\n");
         PRINTF("* Command SPPLE:   DiscoverSPPLE, RegisterSPPLE, UnregisterSPPLE,*\n");
         PRINTF("*                  ConfigureSPPLE, Send, Read,                   *\n");
         PRINTF("*                  DisplayRawModeData, AutomaticReadMode         *\n");
         PRINTF("******************************************************************\n");
      }
      else
      {
         PRINTF("\n");
         PRINTF("******************************************************************\n");
         PRINTF("* Command Options: SPP, SPPLE, Help                              *\n");
         PRINTF("******************************************************************\n");
      }
   }
   return(0);
}

   /* The following function is responsible for setting the             */
   /* Discoverability Mode of the local device.  This function returns  */
   /* zero on successful execution and a negative value on all errors.  */
static int SetLEDiscoverabilityMode(ParameterList_t *TempParam)
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
static int SetLEConnectabilityMode(ParameterList_t *TempParam)
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
static int SetLEPairabilityMode(ParameterList_t *TempParam)
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
         if(TempParam->Params[0].intParam == 0)
         {
            PairabilityMode = lpmNonPairableMode;
            Mode            = "lpmNonPairableMode";
         }
         else
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
static int ChangeLEPairingParameters(ParameterList_t *TempParam)
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
      if(!COMPARE_NULL_BD_ADDR(CurrentLERemoteBD_ADDR))
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
            Result = GAP_LE_Authentication_Response(BluetoothStackID, CurrentLERemoteBD_ADDR, &GAP_LE_Authentication_Response_Information);

            /* Check the return value for the submitted command for     */
            /* success.                                                 */
            if(!Result)
            {
               /* Operation was successful, inform the user.            */
               DisplayFunctionSuccess("Passkey Response");

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
            ASSIGN_BD_ADDR(CurrentLERemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
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

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
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

            /* Write thee advertising data to the chip.                 */
            ret_val = GAP_LE_Set_Advertising_Data(BluetoothStackID, (Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] + 1), &(Advertisement_Data_Buffer.AdvertisingData));
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
         DisplayUsage("Advertise [(0 = Disable, 1 = Enable)].");

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


   /* The following function is responsible for starting an LE scan     */
   /* procedure.  This function returns zero if successful and a        */
   /* negative value if an error occurred.                              */
static int ScanLE(ParameterList_t *TempParam)
{
   int ret_val;

   UNREFERENCED_PARAM(TempParam);

   /* First, check that valid Bluetooth Stack ID exists.                */
   if((BluetoothStackID) && (TempParam->NumberofParameters))
   {
      if(TempParam->Params[0].intParam == 1)
         ret_val = StartScan(BluetoothStackID);
      else
         ret_val = StopScan(BluetoothStackID);
   }
   else
   {
      /* No valid Bluetooth Stack ID exists.                            */
      ret_val = INVALID_STACK_ID_ERROR;
   }

   return(ret_val);
}
   /* The following function is used to set the Connection Parameters.  */
static int SetLEParams(ParameterList_t *TempParam)
{
   int       ret_val = 0;
   int       ndx;
   Boolean_t DisplayCurrent = FALSE;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, make sure that a valid device address exists.            */
      if((TempParam) && (TempParam->NumberofParameters >= 2) && ((TempParam->Params[0].intParam >= 1) && (TempParam->Params[0].intParam <= 6)))
      {
         /* Verify and assign the new value.                            */
         ndx = TempParam->Params[0].intParam-1;
         if((TempParam->Params[1].intParam >= ConnectParamRange[ndx].Low) && ((TempParam->Params[1].intParam <= ConnectParamRange[ndx].High)))
         {
            ((Word_t *)&ConnectParams)[ndx] = (Word_t)TempParam->Params[1].intParam;
            DisplayCurrent = TRUE;
         }
         else
            ret_val = INVALID_PARAMETERS_ERROR;
      }
      else
      {
         /* Allow the current values to be displayed if there are no    */
         /* parameters provided.                                        */
         if((TempParam) && (!TempParam->NumberofParameters))
            DisplayCurrent = TRUE;

         ret_val = INVALID_PARAMETERS_ERROR;
      }

      if(DisplayCurrent)
      {
         PRINTF("Current Connect Params\n");
         PRINTF("   Connect Interval Min: %d\n", ConnectParams.ConnectIntMin);
         PRINTF("   Connect Interval Max: %d\n", ConnectParams.ConnectIntMax);
         PRINTF("          Slave Latency: %d\n", ConnectParams.SlaveLatency);
         PRINTF("  Connection Length Min: %d\n", ConnectParams.MinConnectLength);
         PRINTF("  Connection Length Max: %d\n", ConnectParams.MaxConnectLength);
         PRINTF("    Supervision Timeout: %d\n", ConnectParams.SupervisionTO);
         PRINTF("\n");
      }

      if(ret_val == INVALID_PARAMETERS_ERROR)
      {
         /* Invalid parameters specified so flag an error to the user.  */
         PRINTF("Usage: SetLEParams [Index] [Value].\n");
         PRINTF("         Index 1. Connect Int Min (8ms-4000ms)\n");
         PRINTF("         Index 2. Connect Int Max (8ms-4000ms)\n");
         PRINTF("         Index 3. Slave Latency   (0-499)\n");
         PRINTF("         Index 4. Connect Len Min (0ms-40959ms)\n");
         PRINTF("         Index 5. Connect Len Max (0ms-40959ms)\n");
         PRINTF("         Index 6. Supervision TO  (100ms-32000ms)\n\n");
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
   int                   ret_val;
   BD_ADDR_t             BD_ADDR;
   GAP_LE_Address_Type_t Address_Type;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /* Next, make sure that a valid device address exists.            */
      if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= NumScanEntries))
      {
         Address_Type = ScanInfo[TempParam->Params[0].intParam-1].AddressType;
         BD_ADDR      = ScanInfo[TempParam->Params[0].intParam-1].BD_ADDR;

         if(!ConnectLEDevice(BluetoothStackID, Address_Type, BD_ADDR, FALSE))
            ret_val = 0;
         else
            ret_val = FUNCTION_ERROR;
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         DisplayUsage("Connect [ScanList Index].\n");

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

   /* The following function is responsible for connecting to an LE     */
   /* device.  This function returns zero if successful and a negative  */
   /* value if an error occurred.                                       */
static int CancelConnectLE(ParameterList_t *TempParam)
{
   int ret_val;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      ret_val = GAP_LE_Cancel_Create_Connection(BluetoothStackID);
      if(!ret_val)
         PRINTF("Cancel Connect\n");
      else
         PRINTF("Cancel Connect Failed with %d\n", ret_val);
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
   int LEConnectionIndex;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /*Next, make sure that a valid device address exists.             */
      if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
      {
         /* Get the Connection Index Number.                            */
         LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

         /* Verify that we are connected                                */
         if(LEContextInfo[LEConnectionIndex].ConnectionID)
         {
            /* Attempt to disconnect the device.                        */
            if(!DisconnectLEDevice(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR))
               ret_val = 0;
            else
               ret_val = FUNCTION_ERROR;
         }
         else
         {
            /* No matching ConnectionBD_ADDR.                           */
            PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         PRINTF("Usage: Disconnect [Device Number].\n");

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

   /* The following function is provided to allow a mechanism of        */
   /* Pairing (or requesting security if a slave) to the connected      */
   /* device.                                                           */
static int PairLE(ParameterList_t *TempParam)
{
   int ret_val;
   int LEConnectionIndex;

   /* First, check that valid Bluetooth Stack ID exists.                */
   if(BluetoothStackID)
   {
      /*Next, make sure that a valid device address exists.             */
      if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
      {
         /* Get the Connection Index Number.                            */
         LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

         /* Verify that we are connected                                */
         if(LEContextInfo[LEConnectionIndex].ConnectionID)
         {
            /* Attempt to send a pairing request to the specified       */
            /* device.                                                  */
            if(!SendPairingRequest(LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR, LocalDeviceIsMaster))
               ret_val = 0;
            else
               ret_val = FUNCTION_ERROR;
         }
         else
         {
            /* No matching ConnectionBD_ADDR.                           */
            PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* Invalid parameters specified so flag an error to the user.  */
         PRINTF("Usage: Pair [Device Number].\n");

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

   /* The following function is responsible for performing a DIS Service*/
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverDIS(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   GATT_UUID_t   UUID;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that no service discovery is outstanding for   */
               /* this device.                                          */
               if(!(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING))
               {
                  /* Configure the filter so that only the DIS Service  */
                  /* is discovered.                                     */
                  UUID.UUID_Type = guUUID_16;
                  DIS_ASSIGN_DIS_SERVICE_UUID_16(UUID.UUID.UUID_16);

                  BTPS_MemInitialize(&DeviceInfo->DISClientInfo, 0, sizeof(DIS_Client_Info_t));

                  ret_val = GATT_Start_Service_Discovery(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, 1, &UUID, GATT_Service_Discovery_Event_Callback, 0);
                  if(!ret_val)
                  {
                     /* Display success message.                        */
                     PRINTF("GATT_Start_Service_Discovery success.\n");

                     /* Flag that a Service Discovery Operation is      */
                     /* outstanding.                                    */
                     DeviceInfo->Flags          |= (DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING | DEVICE_INFO_FLAG_SERVICE_DISCOVERY_DIS);
                     DeviceInfo->DISHandleIndex  = 0;
                  }
                  else
                  {
                     /* An error occur so just clean-up.                */
                     PRINTF("Error - GATT_Start_Service_Discovery returned %d.\n", ret_val);

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
            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: DiscoverDIS [Device Number].\n");

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for performing a GAP Service*/
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverGAPS(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   GATT_UUID_t   UUID;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that no service discovery is outstanding for   */
               /* this device.                                          */
               if(!(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING))
               {
                  /* Configure the filter so that only the GAPS Service */
                  /* is discovered.                                     */
                  UUID.UUID_Type = guUUID_16;
                  GAP_ASSIGN_GAP_SERVICE_UUID_16(UUID.UUID.UUID_16);

                  ret_val = GATT_Start_Service_Discovery(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, 1, &UUID, GATT_Service_Discovery_Event_Callback, 0);
                  if(!ret_val)
                  {
                     /* Display success message.                        */
                     PRINTF("GATT_Start_Service_Discovery success.\n");

                     /* Flag that a Service Discovery Operation is      */
                     /* outstanding.                                    */
                     DeviceInfo->Flags |= (DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING | DEVICE_INFO_FLAG_SERVICE_DISCOVERY_GAPS);
                  }
                  else
                  {
                     /* An error occur so just clean-up.                */
                     PRINTF("Error - GATT_Start_Service_Discovery returned %d.\n", ret_val);

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

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: DiscoverGAPS [Device Number].\n");

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
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
      /* Initialize the Name Buffer to all ZEROs.                       */
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
         /* Attempt to submit the command.                              */
         ret_val = GAP_Set_Local_Device_Name(BluetoothStackID, TempParam->Params[0].strParam);
         if(!ret_val)
         {
            /* Query the Local Name.                                    */
            ret_val = GAPS_Set_Device_Name(BluetoothStackID, GAPSInstanceID, TempParam->Params[0].strParam);
            if(!ret_val)
               PRINTF("Local Device Name set to %s.\n", TempParam->Params[0].strParam);
            else
            {
               PRINTF("Error - GAPS_Query_Device_Name returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }
         }
         else
         {
            PRINTF("Error - GAP_Set_Local_Device_Name returned %d.\n", ret_val);

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
   int           LEConnectionIndex;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that we discovered the Device Name Handle.     */
               if(DeviceInfo->GAPSClientInfo.DeviceNameHandle)
               {
                  /* Attempt to read the remote device name.            */
                  ret_val = GATT_Read_Value_Request(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->GAPSClientInfo.DeviceNameHandle, GATT_ClientEventCallback, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceNameHandle);
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

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: GetLERemoteName [Device Number].\n");

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
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
   if((TempParam) && (TempParam->NumberofParameters > 0) && (TempParam->Params[0].intParam >= 0) && (TempParam->Params[0].intParam < (int)NUMBER_OF_APPEARANCE_MAPPINGS))
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
      PRINTF("Usage: SetLocalAppearance [Index].\n");
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
   int           LEConnectionIndex;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that we discovered the Device Name Handle.     */
               if(DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle)
               {
                  /* Attempt to read the remote device name.            */
                  ret_val = GATT_Read_Value_Request(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle, GATT_ClientEventCallback, (unsigned long)DeviceInfo->GAPSClientInfo.DeviceAppearanceHandle);
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

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: GetRemoteAppearance [Device Number].\n");

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for performing a LETP       */
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverLETP(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   GATT_UUID_t   UUID;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that no service discovery is outstanding for   */
               /* this device.                                          */
               if(!(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING))
               {
                  /* Configure the filter so that only the SPP LE       */
                  /* Service is discovered.                             */
                  UUID.UUID_Type = guUUID_128;
                  LETP_ASSIGN_LETP_SERVICE_UUID_128(UUID.UUID.UUID_128);

                  /* Start the service discovery process.               */
                  ret_val = GATT_Start_Service_Discovery(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, 1, &UUID, GATT_Service_Discovery_Event_Callback, 0);
                  if(!ret_val)
                  {
                     /* Display success message.                        */
                     PRINTF("GATT_Start_Service_Discovery success.\n");

                     /* Flag that a Service Discovery Operation is      */
                     /* outstanding.                                    */
                     DeviceInfo->Flags |= (DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING | DEVICE_INFO_FLAG_SERVICE_DISCOVERY_LETP);
                  }
                  else
                  {
                     /* An error occur so just clean-up.                */
                     PRINTF("Error - GATT_Start_Service_Discovery returned %d.\n", ret_val);

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

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: DiscoverLETP [Device Number].\n");

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for registering a LETP      */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int RegisterLETP(ParameterList_t *TempParam)
{
   int                           ret_val;
   GATT_Attribute_Handle_Group_t ServiceHandleGroup;

   /* Verify that there is no active connection.                        */
   if(FindFreeLEIndex() != -1)
   {
      /* Verify that the Service is not already registered.             */
      if(!LETPServiceID)
      {
         /* Initialize the handle group to 0 .                          */
         ServiceHandleGroup.Starting_Handle = 0;
         ServiceHandleGroup.Ending_Handle   = 0;

         /* Register the LETP Service.                                  */
         ret_val = GATT_Register_Service(BluetoothStackID, LETP_SERVICE_FLAGS, LETP_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)LETP_Service, &ServiceHandleGroup, GATT_LETP_ServerEventCallback, 0);
         if(ret_val > 0)
         {
            /* Display success message.                                 */
            PRINTF("Successfully registered LETP Service. StartHandle %04X EndHandle %04X\n", ServiceHandleGroup.Starting_Handle, ServiceHandleGroup.Ending_Handle);

            /* Save the ServiceID of the registered service.            */
            LETPServiceID = (unsigned int)ret_val;

            /* Return success to the caller.                            */
            ret_val        = 0;
         }
      }
      else
      {
         PRINTF("LETP Service already registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Connection currently active.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for unregistering a LETP    */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int UnregisterLETP(ParameterList_t *TempParam)
{
   int ret_val;

   /* Verify that there is no active connection.                        */
   for(ret_val=0; ret_val<MAX_LE_CONNECTIONS; ret_val++)
   {
      if(LEContextInfo[ret_val].ConnectionID)
         break;
   }

   if(ret_val == MAX_LE_CONNECTIONS)
   {
      /* Verify that the Service is not already registered.             */
      if(LETPServiceID)
      {
         /* Un-registered LE TP Service.                                */
         GATT_Un_Register_Service(BluetoothStackID, LETPServiceID);

         /* Display success message.                                    */
         PRINTF("Successfully unregistered LETP Service.\n");

         /* Save the ServiceID of the registered service.               */
         LETPServiceID = 0;

         /* Return success to the caller.                               */
         ret_val        = 0;
      }
      else
      {
         PRINTF("LETP Service not registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Connection currently active.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is used to set the number of bytes that the*/
   /* server will notify when enabled.                                  */
static int SetLETPCount(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   DWord_t       Value;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 1) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Determine if a service discovery operation has been   */
               /* previously done.                                      */
               if(LETP_CLIENT_INFORMATION_VALID(DeviceInfo->TPClientInfo))
               {
                  /* Check to see if the value is within valid range    */
                  if(TempParam->Params[1].intParam)
                  {
                     ASSIGN_HOST_DWORD_TO_LITTLE_ENDIAN_UNALIGNED_DWORD(&Value, TempParam->Params[1].intParam);

                     ret_val = GATT_Write_Request(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->TPClientInfo.Tx_Bulk_Characteristic, DWORD_SIZE, &Value, GATT_ClientEventCallback, 0);
                     if(ret_val < 0)
                        PRINTF("SetLETPCount failed to set count. %d\n", ret_val);
                     else
                        ret_val = 0;
                  }
                  else
                     ret_val = INVALID_PARAMETERS_ERROR;
               }
               else
               {
                  PRINTF("No LETP Service discovered on device.\n");

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               PRINTF("No Device Info.\n");

               ret_val = FUNCTION_ERROR;
            }

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
      ret_val = INVALID_PARAMETERS_ERROR;

   if(ret_val == INVALID_PARAMETERS_ERROR)
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: SetLETPCount [Device Number] [# Bytes to Send (DWord Size)].\n");
   }

   return(ret_val);
}

   /* The following function is used to set the number of milliseconds  */
   /* between send operations.                                          */
static int SetLETPInterval(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   Word_t        Value;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 1) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Determine if a service discovery operation has been   */
               /* previously done.                                      */
               if(LETP_CLIENT_INFORMATION_VALID(DeviceInfo->TPClientInfo))
               {
                  /* Check to see if the value is within valid range    */
                  if(TempParam->Params[1].intParam >= 0)
                  {
                     ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Value, TempParam->Params[1].intParam);

                     ret_val = GATT_Write_Request(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->TPClientInfo.Tx_Interval_Characteristic, WORD_SIZE, &Value, GATT_ClientEventCallback, 0);
                     if(ret_val < 0)
                        PRINTF("SetLETPInterval failed to set count. %d\n", ret_val);
                  }
                  else
                     ret_val = INVALID_PARAMETERS_ERROR;
               }
               else
               {
                  PRINTF("No LETP Service discovered on device.\n");

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               PRINTF("No Device Info.\n");

               ret_val = FUNCTION_ERROR;
            }

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
      ret_val = INVALID_PARAMETERS_ERROR;

   if(ret_val == INVALID_PARAMETERS_ERROR)
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: SetLETPInterval [Device Number] [# ms between send events (Word Size)].\n");
      PRINTF("          An interval of 0 enables one-shot mode.\n");
   }

   return(ret_val);
}

   /* The following function is responsible for timing the throughput of*/
   /* an LETP Service.  This function will return zero on successful    */
   /* execution and a negative value on errors.                         */
static int TimeLETP(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   DWord_t       Diff;
   DWord_t       Throughput;
   XferInfo_t   *XferInfo;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 1) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Determine if a service discovery operation has been   */
               /* previously done.                                      */
               if(LETP_CLIENT_INFORMATION_VALID(DeviceInfo->TPClientInfo))
               {
                  /* Flag success as default.                           */
                  ret_val = 0;

                  /* Verify that we are not already timing.             */
                  if((TempParam->Params[1].intParam) && (DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_TIMING_ACTIVE))
                  {
                     PRINTF("Timing Already Enabled\n");

                     ret_val = FUNCTION_ERROR;
                  }
                  /* Verify that we are not already acting as a Server. */
                  if((!TempParam->Params[1].intParam) && (!(DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_TIMING_ACTIVE)))
                  {
                     PRINTF("LETP Timing is not Enabled\n");

                     ret_val = FUNCTION_ERROR;
                  }
                  /* Verify that the value is within valid range.       */
                  if((TempParam->Params[1].intParam < 0) || (TempParam->Params[1].intParam > 1))
                     ret_val = INVALID_PARAMETERS_ERROR;

                  /* Check to see if any errors have occurred.          */
                  if(!ret_val)
                  {
                     /* Check to see if we need to Enable or Disable the*/
                     /* service                                         */
                     if(TempParam->Params[1].intParam)
                     {
                        DeviceInfo->Flags |= DEVICE_INFO_FLAG_LETP_TIMING_ACTIVE;
                        BTPS_MemInitialize(&DeviceInfo->XferInfo, 0, sizeof(XferInfo_t));
                        PRINTF("Ready to receive\n");
                     }
                     else
                     {
                        DeviceInfo->Flags &= ~DEVICE_INFO_FLAG_LETP_TIMING_ACTIVE;

                        XferInfo = &DeviceInfo->XferInfo;
                        Diff     = DIFF_TIMESTAMP(XferInfo->LastTime, XferInfo->FirstTime);
                        if(Diff)
                        {
                           /* Subtract the size of the first packet     */
                           /* from the total bytes received since we    */
                           /* started the timer after the first packet. */
                           Throughput = (((XferInfo->RxCount - XferInfo->FirstPacketSize) << 3)/(Diff));
                           PRINTF("Throughput: %d Kbps\n", (int)Throughput);
                           PRINTF("  Rx Bytes: %lu\n", XferInfo->RxCount);
                           PRINTF("  Diff    : %lu ms\n", Diff);
                        }
                        else
                        {
                           PRINTF("Throughput: Unknown\n");
                           PRINTF("  Rx Bytes: %lu\n", XferInfo->RxCount);
                           PRINTF("No Time Differential\n");
                        }
                     }
                  }
               }
               else
               {
                  PRINTF("No LETP Service discovered on device.\n");

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               PRINTF("No Device Info.\n");

               ret_val = FUNCTION_ERROR;
            }

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
      ret_val = INVALID_PARAMETERS_ERROR;

   if(ret_val == INVALID_PARAMETERS_ERROR)
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: TimeLETP [Device Number] [0-Stop, 1-Start].\n");
   }

   return(ret_val);
}

   /* The following function is responsible for configure a LETP        */
   /* Service on a remote device.  This function will return zero on    */
   /* successful execution and a negative value on errors.              */
static int EnableLETP(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   DWord_t       Value;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 1) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Determine if a service discovery operation has been   */
               /* previously done.                                      */
               if(LETP_CLIENT_INFORMATION_VALID(DeviceInfo->TPClientInfo))
               {
                  /* Flag success as default.                           */
                  ret_val = 0;

                  /* Verify that we are not already acting as a Server. */
                  if((TempParam->Params[1].intParam) && (DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_CLIENT))
                  {
                     PRINTF("Already Enabled\n");

                     ret_val = FUNCTION_ERROR;
                  }
                  /* Verify that we are not already acting as a Server. */
                  if((!TempParam->Params[1].intParam) && (!(DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_CLIENT)))
                  {
                     PRINTF("LETP is not Enabled\n");

                     ret_val = FUNCTION_ERROR;
                  }
                  /* Verify that the value is within valid range.       */
                  if((TempParam->Params[1].intParam < 0) || (TempParam->Params[1].intParam > 1))
                     ret_val = INVALID_PARAMETERS_ERROR;

                  /* Check to see if any errors have occurred.          */
                  if(!ret_val)
                  {
                     PRINTF("LETP Service found on remote device, attempting to configured CCCD.\n");

                     /* Check to see if we need to Enable or Disable the*/
                     /* service                                         */
                     Value = (DWord_t)(TempParam->Params[1].intParam);
                     if(Value)
                     {
                        EnableDisableNotificationsIndications(LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->TPClientInfo.Tx_Bulk_Client_Configuration_Descriptor, GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE, GATT_ClientEventCallback);
                        DeviceInfo->Flags |= DEVICE_INFO_FLAG_LETP_CLIENT;
                     }
                     else
                     {
                        EnableDisableNotificationsIndications(LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->TPClientInfo.Tx_Bulk_Client_Configuration_Descriptor, 0, GATT_ClientEventCallback);
                        DeviceInfo->Flags &= ~DEVICE_INFO_FLAG_LETP_CLIENT;
                     }
                  }
               }
               else
               {
                  PRINTF("No LETP Service discovered on device.\n");

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               PRINTF("No Device Info.\n");

               ret_val = FUNCTION_ERROR;
            }

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
      ret_val = INVALID_PARAMETERS_ERROR;

   if(ret_val == INVALID_PARAMETERS_ERROR)
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: EnableLETP [Device Number] [0-Disable, 1-Enable].\n");
   }

   return(ret_val);
}

   /* The following function is responsible for performing a SPPLE      */
   /* Service Discovery Operation.  This function will return zero on   */
   /* successful execution and a negative value on errors.              */
static int DiscoverSPPLE(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   GATT_UUID_t   UUID;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that no service discovery is outstanding for   */
               /* this device.                                          */
               if(!(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING))
               {
                  /* Configure the filter so that only the SPP LE       */
                  /* Service is discovered.                             */
                  UUID.UUID_Type = guUUID_128;
                  SPPLE_ASSIGN_SPPLE_SERVICE_UUID_128(UUID.UUID.UUID_128);

                  /* Start the service discovery process.               */
                  ret_val = GATT_Start_Service_Discovery(BluetoothStackID, LEContextInfo[LEConnectionIndex].ConnectionID, 1, &UUID, GATT_Service_Discovery_Event_Callback, 0);
                  if(!ret_val)
                  {
                     /* Display success message.                        */
                     PRINTF("GATT_Start_Service_Discovery success.\n");

                     /* Flag that a Service Discovery Operation is      */
                     /* outstanding.                                    */
                     DeviceInfo->Flags |= (DEVICE_INFO_FLAG_SERVICE_DISCOVERY_OUTSTANDING | DEVICE_INFO_FLAG_SERVICE_DISCOVERY_SPPLE);
                  }
                  else
                  {
                     /* An error occur so just clean-up.                */
                     PRINTF("Error - GATT_Start_Service_Discovery returned %d.\n", ret_val);

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

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: DiscoverSPPLE [Device Number].\n");

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for registering a SPPLE     */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int RegisterSPPLE(ParameterList_t *TempParam)
{
   int                           ret_val;
   GATT_Attribute_Handle_Group_t ServiceHandleGroup;

   /* Verify that there is no active connection.                        */
   if(FindFreeLEIndex() != -1)
   {
      /* Verify that the Service is not already registered.             */
      if(!SPPLEServiceID)
      {
         /* Initialize the handle group to 0 .                          */
         ServiceHandleGroup.Starting_Handle = 0;
         ServiceHandleGroup.Ending_Handle   = 0;

         /* Register the SPPLE Service.                                 */
         ret_val = GATT_Register_Service(BluetoothStackID, SPPLE_SERVICE_FLAGS, SPPLE_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)SPPLE_Service, &ServiceHandleGroup, GATT_SPPLE_ServerEventCallback, 0);
         if(ret_val > 0)
         {
            /* Display success message.                                 */
            PRINTF("Successfully registered SPPLE Service.\n");

            /* Save the ServiceID of the registered service.            */
            SPPLEServiceID = (unsigned int)ret_val;

            /* Return success to the caller.                            */
            ret_val        = 0;
         }
      }
      else
      {
         PRINTF("SPPLE Service already registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Connection currently active.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for unregistering a SPPLE   */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int UnregisterSPPLE(ParameterList_t *TempParam)
{
   int ret_val;

   /* Verify that there is no active connection.                        */
   for(ret_val=0; ret_val<MAX_LE_CONNECTIONS; ret_val++)
   {
      if(LEContextInfo[ret_val].ConnectionID)
         break;
   }

   if(ret_val == MAX_LE_CONNECTIONS)
   {
      /* Verify that the Service is not already registered.             */
      if(SPPLEServiceID)
      {
         /* Un-registered SPP LE Service.                               */
         GATT_Un_Register_Service(BluetoothStackID, SPPLEServiceID);

         /* Display success message.                                    */
         PRINTF("Successfully unregistered SPPLE Service.\n");

         /* Save the ServiceID of the registered service.               */
         SPPLEServiceID = 0;

         /* Return success to the caller.                               */
         ret_val        = 0;
      }
      else
      {
         PRINTF("SPPLE Service not registered.\n");

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      PRINTF("Connection currently active.\n");

      ret_val = FUNCTION_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for configure a SPPLE       */
   /* Service on a remote device.  This function will return zero on    */
   /* successful execution and a negative value on errors.              */
static int ConfigureSPPLE(ParameterList_t *TempParam)
{
   int           ret_val;
   int           LEConnectionIndex;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that we are not already acting as a Server.    */
               if(!(DeviceInfo->Flags & DEVICE_INFO_FLAG_SPPLE_SERVER))
               {
                  /* Determine if a service discovery operation has been*/
                  /* previously done.                                   */
                  if(SPPLE_CLIENT_INFORMATION_VALID(DeviceInfo->ClientInfo))
                  {
                     PRINTF("SPPLE Service found on remote device, attempting to read Transmit Credits and configured CCCDs.\n");

                     /* Send the Initial Credits to the remote device.  */
                     SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);

                     /* Enable Notifications on the proper              */
                     /* characteristics.                                */
                     EnableDisableNotificationsIndications(LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->ClientInfo.Rx_Credit_Client_Configuration_Descriptor, GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE, GATT_ClientEventCallback);
                     EnableDisableNotificationsIndications(LEContextInfo[LEConnectionIndex].ConnectionID, DeviceInfo->ClientInfo.Tx_Client_Configuration_Descriptor, GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE, GATT_ClientEventCallback);

                     /* Flag that we are an SPPLE Client.               */
                     DeviceInfo->Flags |= DEVICE_INFO_FLAG_SPPLE_CLIENT;
                     ret_val            = 0;
                  }
                  else
                  {
                     PRINTF("No SPPLE Service discovered on device.\n");

                     ret_val = FUNCTION_ERROR;
                  }
               }
               else
               {
                  PRINTF("Already operating as a Server\n");

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               PRINTF("No Device Info.\n");

               ret_val = FUNCTION_ERROR;
            }

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
         {
            PRINTF("Unable to acquire the list mutex\n");

            ret_val = FUNCTION_ERROR;
         }
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);

         ret_val = FUNCTION_ERROR;
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: ConfigureSPPLE [Device Number].\n");

      /* Flag that an error occurred while submitting the command.      */
      ret_val = INVALID_PARAMETERS_ERROR;
   }

   return(ret_val);
}

   /* The following function is responsible for sending a number of     */
   /* characters to a remote device to which a connection exists.  The  */
   /* function receives a parameter that indicates the number of byte to*/
   /* be transferred.  This function will return zero on successful     */
   /* execution and a negative value on errors.                         */
static int SendDataCommand(ParameterList_t *TempParam)
{
   DeviceInfo_t      *DeviceInfo;
   LE_Context_Info_t *ContextInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      ContextInfo = &LEContextInfo[TempParam->Params[0].intParam-1];

      /* Verify that we are connected                                   */
      if(ContextInfo->ConnectionID)
      {
         /* Check to see if we are in the process of sending.           */
         if(!ContextInfo->BytesToSend)
         {
            /* Save the data length.                                    */
            ContextInfo->BytesToSend  = TempParam->Params[1].intParam;
            ContextInfo->BytesSent    = 0;
            ContextInfo->DataStrIndex = 0;

            /* Acquire the List Mutex.                                  */
            if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
            {
               /* Get the device info for the connection device.        */
               DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, ContextInfo->ConnectionBD_ADDR);
               if(DeviceInfo)
               {
                  /* Verify that we are a Client or Server.             */
                  if(DeviceInfo->Flags & (DEVICE_INFO_FLAG_SPPLE_CLIENT | DEVICE_INFO_FLAG_SPPLE_SERVER))
                  {
                     /* Process the data to be sent.                    */
                     SendSPPLEData(ContextInfo, DeviceInfo);
                  }
                  else
                     PRINTF("SPPLE has not been configured\n");
               }
               else
                  PRINTF("No Device Info.\n");

               /* Release the List Mutex.                               */
               BTPS_ReleaseMutex(DeviceListMutex);
            }
            else
               PRINTF("Unable to acquire the list mutex\n");
         }
         else
            PRINTF("Sending in progress %d of %d bytes sent.\n", ContextInfo->BytesSent, ContextInfo->BytesToSend);
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);
      }
   }
   else
      DisplayUsage("Send [Device Number] [Number of Bytes to send].\n");

   return(0);
}

   /* The following function is responsible for reading data sent by a  */
   /* remote device to which a connection exists.  This function will   */
   /* return zero on successful execution and a negative value on       */
   /* errors.                                                           */
static int ReadDataCommand(ParameterList_t *TempParam)
{
   int           LEConnectionIndex;
   Boolean_t     Done;
   unsigned int  Temp;
   DeviceInfo_t *DeviceInfo;

   /* Next, make sure that a valid device address exists.               */
   if((TempParam) && (TempParam->NumberofParameters > 0) && ((TempParam->Params[0].intParam > 0) && (TempParam->Params[0].intParam <= MAX_LE_CONNECTIONS)))
   {
      /* Get the Connection Index Number.                               */
      LEConnectionIndex = (int)(TempParam->Params[0].intParam-1);

      /* Verify that we are connected                                   */
      if(LEContextInfo[LEConnectionIndex].ConnectionID)
      {
         /* Acquire the List Mutex.                                     */
         if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
         {
            /* Get the device info for the connection device.           */
            if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
            {
               /* Verify that we are a Client or Server.                */
               if(DeviceInfo->Flags & (DEVICE_INFO_FLAG_SPPLE_CLIENT | DEVICE_INFO_FLAG_SPPLE_SERVER))
               {
                  /* Determine the number of bytes we are going to read.*/
                  Temp = LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BufferSize - LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree;

                  PRINTF("Read: %u.\n", Temp);

                  /* Loop and read all of the data.                     */
                  Done = FALSE;
                  while(!Done)
                  {
                     /* Read the data.                                  */
                     Temp = SPPLEReadData(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, SPPLE_DATA_BUFFER_LENGTH, SPPLEBuffer);
                     if(Temp > 0)
                     {
                        /* Display the data.                            */
                        SPPLEBuffer[Temp] = '\0';
                        PRINTF("%s", (char *)SPPLEBuffer);
                     }
                     else
                        Done = TRUE;
                  }
                  PRINTF("\n");
               }
               else
                  PRINTF("SPPLE has not been configured\n");
            }
            else
               PRINTF("No Device Info.\n");

            /* Release the List Mutex.                                  */
            BTPS_ReleaseMutex(DeviceListMutex);
         }
         else
            PRINTF("Unable to acquire the list mutex\n");
      }
      else
      {
         /* No matching ConnectionBD_ADDR.                              */
         PRINTF("No connection exists for Device %d.\n", TempParam->Params[0].intParam);
      }
   }
   else
   {
      /* Invalid parameters specified so flag an error to the user.     */
      PRINTF("Usage: Read [Device Number].\n");
   }

   return(0);
}

   /* The following function is a utility function that is used to find */
   /* the SPP Port Index by the specified Serial Port ID.  This function*/
   /* returns the index of the Serial Port or -1 on failure.            */
static int FindSPPPortIndex(unsigned int SerialPortID)
{
   int          ret_val = -1;
   unsigned int i;

   /* Search the list for the Serial Port Info.                         */
   for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
   {
      /* Check to see if this entry matches the entry we are to search  */
      /* for.                                                           */
      if(SPPContextInfo[i].LocalSerialPortID == SerialPortID)
      {
         ret_val = (int)i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is a utility function that is used to find */
   /* the SPP Port Index by the specified Server Port Number.  This     */
   /* function returns the index of the Serial Port or -1 on failure.   */
static int FindSPPPortIndexByServerPortNumber(unsigned int ServerPortNumber)
{
   int          ret_val = -1;
   unsigned int i;

   /* Search the list for the Serial Port Info.                         */
   for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
   {
      /* Check to see if this entry matches the entry we are to search  */
      /* for.                                                           */
      if(SPPContextInfo[i].ServerPortNumber == ServerPortNumber)
      {
         ret_val = (int)i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is a utility function that is used to find */
   /* the SPP Port Index by the specified BD_ADDR.  This function       */
   /* returns the index of the Serial Port or -1 on failure.            */
static int FindSPPPortIndexByAddress(BD_ADDR_t BD_ADDR)
{
   int          ret_val = -1;
   unsigned int i;

   /* Search the list for the Serial Port Info.                         */
   for(i=0; i < sizeof(SPPContextInfo)/sizeof(SPP_Context_Info_t); i++)
   {
      /* Check to see if this entry matches the entry we are to search  */
      /* for.                                                           */
      if(COMPARE_BD_ADDR(SPPContextInfo[i].BD_ADDR, BD_ADDR))
      {
         ret_val = (int)i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is a utility function that is used to find */
   /* a SPP Port Index that is not currently in use.  This function     */
   /* returns the index of the Serial Port or -1 on failure.            */
static int FindFreeSPPPortIndex(void)
{
   return(FindSPPPortIndex(0));
}

   /* The following function is a utility function which is used to     */
   /* close the specified port by the specified Serial Port ID.         */
static int ClosePortByNumber(unsigned int LocalSerialPortID)
{
   int       i;
   int       ret_val = -1;
   Boolean_t ServerPort;

   /* Find an empty slot.                                               */
   if((i = FindSPPPortIndex(LocalSerialPortID)) >= 0)
   {
      /* Un-register the SDP Record registered for this server port.    */
      if(SPPContextInfo[i].ServerPortNumber)
      {
         if(SPPContextInfo[i].SPPServerSDPHandle)
            SPP_Un_Register_SDP_Record(BluetoothStackID, SPPContextInfo[i].LocalSerialPortID, SPPContextInfo[i].SPPServerSDPHandle);

         ServerPort = TRUE;
      }
      else
         ServerPort = FALSE;

      /* Close the specified server port.                               */
      if(ServerPort)
         ret_val = SPP_Close_Server_Port(BluetoothStackID, SPPContextInfo[i].LocalSerialPortID);
      else
         ret_val = SPP_Close_Port(BluetoothStackID, SPPContextInfo[i].LocalSerialPortID);

      if(ret_val < 0)
      {
         if(ServerPort)
            DisplayFunctionError("SPP_Close_Server_Port", ret_val);
         else
            DisplayFunctionError("SPP_Close_Port", ret_val);

         ret_val = FUNCTION_ERROR;
      }
      else
         ret_val = 0;

      /* Clear SPP Server info for that port.                           */
      BTPS_MemInitialize(&SPPContextInfo[i], 0, sizeof(SPPContextInfo[i]));

      PRINTF("Port Context Cleared.\n");
   }
   else
      PRINTF("Invalid Port Number\n");

   return(ret_val);
}

   /* The following function is responsible for iterating through the   */
   /* array BDInfoArray[MAX_LE_CONNECTIONS], which contains the         */
   /* connection information for connected LE devices.  It returns -1 if*/
   /* the a free connection index is not found.  If a free index is     */
   /* found, it returns the free index which can be used for another    */
   /* connection.                                                       */
static int FindFreeLEIndex(void)
{
   BD_ADDR_t NullBD_ADDR;

   ASSIGN_BD_ADDR(NullBD_ADDR, 0, 0, 0, 0, 0, 0);

   return(FindLEIndexByAddress(NullBD_ADDR));
}

   /* The following function is responsible for iterating through the   */
   /* array BDInfoArray[MAX_LE_CONNECTIONS], which contains the         */
   /* connection information for connected LE devices.  It returns -1 if*/
   /* the BD_ADDR is not found.  If the BD_ADDR is found, it returns the*/
   /* index at which the BD_ADDR was found in the array.                */
static int FindLEIndexByAddress(BD_ADDR_t BD_ADDR)
{
   int i;
   int ret_val = -1;

   for(i=0; i<MAX_LE_CONNECTIONS; i++)
   {
      if(COMPARE_BD_ADDR(BD_ADDR, LEContextInfo[i].ConnectionBD_ADDR))
      {
         ret_val = i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for iterating through the   */
   /* array BDInfoArray[MAX_LE_CONNECTIONS], which contains the         */
   /* connection information for connected LE devices.  It returns -1 if*/
   /* the Connection ID is not found.  If the Connection ID is found, it*/
   /* returns the index at which the Connection ID was found in the     */
   /* array.                                                            */
static int FindLEIndexByConnectionID(unsigned int ConnectionID)
{
   int i;
   int ret_val = -1;

   for(i=0; i<MAX_LE_CONNECTIONS; i++)
   {
      if(LEContextInfo[i].ConnectionID == ConnectionID)
      {
         ret_val = i;
         break;
      }
   }

   return(ret_val);
}

   /* The following function is responsible for updating the connection */
   /* identifier for a given BD_ADDR.  If an entry in the array         */
   /* BDInfoArray[MAX_LE_CONNECTIONS] has a matching BD_ADDR, then its  */
   /* ConnectionID is set to the passed value of ConnectionID and the   */
   /* function returns its index in the array.  If no matching BD_ADDR  */
   /* is found, the function returns -1.                                */
static int UpdateConnectionID(unsigned int ConnectionID, BD_ADDR_t BD_ADDR)
{
   int LEConnectionIndex;

   /* Check for the index of the entry for this connection.             */
   LEConnectionIndex = FindLEIndexByAddress(BD_ADDR);
   if(LEConnectionIndex >= 0)
      LEContextInfo[LEConnectionIndex].ConnectionID = ConnectionID;
   else
   {
      PRINTF("Error in updating ConnectionID.\n");
   }

   return(LEConnectionIndex);
}

   /* The following function is responsible for clearing the values of  */
   /* an entry in BDInfoArray if its ConnectionBD_ADDR matches BD_ADDR. */
static void RemoveConnectionInfo(BD_ADDR_t BD_ADDR)
{
   int           LEConnectionIndex;
   DeviceInfo_t *DeviceInfo;

   /* If an index is returned (any but -1), then found                  */
   LEConnectionIndex = FindLEIndexByAddress(BD_ADDR);
   if(LEConnectionIndex >= 0)
   {
      /* Acquire the List Mutex.                                        */
      if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
      {
         /* Get the device info for the connection device.              */
         if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
         {
            /* If we are not bonded, the clear the states.              */
            if(!(DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID))
               DeviceInfo->Flags = 0;
            else
               DeviceInfo->Flags = DEVICE_INFO_FLAG_LTK_VALID;
         }

         /* Release the List Mutex.                                     */
         BTPS_ReleaseMutex(DeviceListMutex);
      }

      ASSIGN_BD_ADDR(LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
      LEContextInfo[LEConnectionIndex].ConnectionID = 0;

      /* Re-initialize the Transmit and Receive Buffers, as well as the */
      /* transmit credits.                                              */
      InitializeBuffer(&(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer));

      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitCredits = 0;
      LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.QueuedCredits   = 0;
      LEContextInfo[LEConnectionIndex].BytesToSend                     = 0;
      LEContextInfo[LEConnectionIndex].BytesSent                       = 0;
      LEContextInfo[LEConnectionIndex].BufferFull                      = FALSE;
   }
}

   /* The following function is responsible for Displaying the contents */
   /* of an SDP Service Attribute Response to the display.              */
static void DisplaySDPAttributeResponse(SDP_Service_Attribute_Response_Data_t *SDPServiceAttributeResponse, unsigned int InitLevel)
{
   int Index;

   /* First, check to make sure that there were Attributes returned.    */
   if(SDPServiceAttributeResponse->Number_Attribute_Values)
   {
      /* Loop through all returned SDP Attribute Values.                */
      for(Index = 0; Index < SDPServiceAttributeResponse->Number_Attribute_Values; Index++)
      {
         /* First Print the Attribute ID that was returned.             */
         PRINTF("%*s Attribute ID 0x%04X\n", (InitLevel*INDENT_LENGTH), "", SDPServiceAttributeResponse->SDP_Service_Attribute_Value_Data[Index].Attribute_ID);

         /* Now Print out all of the SDP Data Elements that were        */
         /* returned that are associated with the SDP Attribute.        */
         DisplayDataElement(SDPServiceAttributeResponse->SDP_Service_Attribute_Value_Data[Index].SDP_Data_Element, (InitLevel + 1));
      }
   }
   else
      PRINTF("No SDP Attributes Found.\n");
}

   /* The following function is responsible for displaying the contents */
   /* of an SDP Service Search Attribute Response to the display.       */
static void DisplaySDPSearchAttributeResponse(SDP_Service_Search_Attribute_Response_Data_t *SDPServiceSearchAttributeResponse)
{
   int Index;

   /* First, check to see if Service Records were returned.             */
   if(SDPServiceSearchAttributeResponse->Number_Service_Records)
   {
      /* Loop through all returned SDP Service Records.                 */
      for(Index = 0; Index < SDPServiceSearchAttributeResponse->Number_Service_Records; Index++)
      {
         /* First display the number of SDP Service Records we are      */
         /* currently processing.                                       */
         PRINTF("Service Record: %u:\n", (Index + 1));

         /* Call Display SDPAttributeResponse for all SDP Service       */
         /* Records received.                                           */
         DisplaySDPAttributeResponse(&(SDPServiceSearchAttributeResponse->SDP_Service_Attribute_Response_Data[Index]), 1);
      }
   }
   else
      PRINTF("No SDP Service Records Found.\n");
}

   /* The following function is responsible for actually displaying an  */
   /* individual SDP Data Element to the Display.  The Level Parameter  */
   /* is used in conjunction with the defined INDENT_LENGTH constant to */
   /* make readability easier when displaying Data Element Sequences    */
   /* and Data Element Alternatives.  This function will recursively    */
   /* call itself to display the contents of Data Element Sequences and */
   /* Data Element Alternatives when it finds these Data Types (and     */
   /* increments the Indent Level accordingly).                         */
static void DisplayDataElement(SDP_Data_Element_t *SDPDataElement, unsigned int Level)
{
   unsigned int Index;
   char         Buffer[256];

   switch(SDPDataElement->SDP_Data_Element_Type)
   {
      case deNIL:
         /* Display the NIL Type.                                       */
         PRINTF("%*s Type: NIL\n", (Level*INDENT_LENGTH), "");
         break;
      case deNULL:
         /* Display the NULL Type.                                      */
         PRINTF("%*s Type: NULL\n", (Level*INDENT_LENGTH), "");
         break;
      case deUnsignedInteger1Byte:
         /* Display the Unsigned Integer (1 Byte) Type.                 */
         PRINTF("%*s Type: Unsigned Int = 0x%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger1Byte);
         break;
      case deUnsignedInteger2Bytes:
         /* Display the Unsigned Integer (2 Bytes) Type.                */
         PRINTF("%*s Type: Unsigned Int = 0x%04X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger2Bytes);
         break;
      case deUnsignedInteger4Bytes:
         /* Display the Unsigned Integer (4 Bytes) Type.                */
         PRINTF("%*s Type: Unsigned Int = 0x%08X\n", (Level*INDENT_LENGTH), "", (unsigned int)SDPDataElement->SDP_Data_Element.UnsignedInteger4Bytes);
         break;
      case deUnsignedInteger8Bytes:
         /* Display the Unsigned Integer (8 Bytes) Type.                */
         PRINTF("%*s Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[7],
                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[6],
                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[5],
                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[4],
                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[3],
                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[2],
                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[1],
                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger8Bytes[0]);
         break;
      case deUnsignedInteger16Bytes:
         /* Display the Unsigned Integer (16 Bytes) Type.               */
         PRINTF("%*s Type: Unsigned Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[15],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[14],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[13],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[12],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[11],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[10],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[9],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[8],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[7],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[6],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[5],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[4],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[3],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[2],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[1],
                                                                                                                     SDPDataElement->SDP_Data_Element.UnsignedInteger16Bytes[0]);
         break;
      case deSignedInteger1Byte:
         /* Display the Signed Integer (1 Byte) Type.                   */
         PRINTF("%*s Type: Signed Int = 0x%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger1Byte);
         break;
      case deSignedInteger2Bytes:
         /* Display the Signed Integer (2 Bytes) Type.                  */
         PRINTF("%*s Type: Signed Int = 0x%04X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger2Bytes);
         break;
      case deSignedInteger4Bytes:
         /* Display the Signed Integer (4 Bytes) Type.                  */
         PRINTF("%*s Type: Signed Int = 0x%08X\n", (Level*INDENT_LENGTH), "", (unsigned int)SDPDataElement->SDP_Data_Element.SignedInteger4Bytes);
         break;
      case deSignedInteger8Bytes:
         /* Display the Signed Integer (8 Bytes) Type.                  */
         PRINTF("%*s Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[7],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[6],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[5],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[4],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[3],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[2],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[1],
                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger8Bytes[0]);
         break;
      case deSignedInteger16Bytes:
         /* Display the Signed Integer (16 Bytes) Type.                 */
         PRINTF("%*s Type: Signed Int = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[15],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[14],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[13],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[12],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[11],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[10],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[9],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[8],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[7],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[6],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[5],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[4],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[3],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[2],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[1],
                                                                                                                 SDPDataElement->SDP_Data_Element.SignedInteger16Bytes[0]);
         break;
      case deTextString:
         /* First retrieve the Length of the Text String so that we can */
         /* copy the Data into our Buffer.                              */
         Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(Buffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(Buffer)-1);

         /* Copy the Text String into the Buffer and then NULL terminate*/
         /* it.                                                         */
         memcpy(Buffer, SDPDataElement->SDP_Data_Element.TextString, Index);
         Buffer[Index] = '\0';

         PRINTF("%*s Type: Text String = %s\n", (Level*INDENT_LENGTH), "", Buffer);
         break;
      case deBoolean:
         PRINTF("%*s Type: Boolean = %s\n", (Level*INDENT_LENGTH), "", (SDPDataElement->SDP_Data_Element.Boolean)?"TRUE":"FALSE");
         break;
      case deURL:
         /* First retrieve the Length of the URL String so that we can  */
         /* copy the Data into our Buffer.                              */
         Index = (SDPDataElement->SDP_Data_Element_Length < sizeof(Buffer))?SDPDataElement->SDP_Data_Element_Length:(sizeof(Buffer)-1);

         /* Copy the URL String into the Buffer and then NULL terminate */
         /* it.                                                         */
         memcpy(Buffer, SDPDataElement->SDP_Data_Element.URL, Index);
         Buffer[Index] = '\0';

         PRINTF("%*s Type: URL = %s\n", (Level*INDENT_LENGTH), "", Buffer);
         break;
      case deUUID_16:
         PRINTF("%*s Type: UUID_16 = 0x%02X%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte0,
                                                                                 SDPDataElement->SDP_Data_Element.UUID_16.UUID_Byte1);
         break;
      case deUUID_32:
         PRINTF("%*s Type: UUID_32 = 0x%02X%02X%02X%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte0,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte1,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte2,
                                                                                         SDPDataElement->SDP_Data_Element.UUID_32.UUID_Byte3);
         break;
      case deUUID_128:
         PRINTF("%*s Type: UUID_128 = 0x%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X%02X\n", (Level*INDENT_LENGTH), "", SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte0,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte1,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte2,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte3,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte4,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte5,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte6,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte7,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte8,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte9,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte10,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte11,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte12,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte13,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte14,
                                                                                                               SDPDataElement->SDP_Data_Element.UUID_128.UUID_Byte15);
         break;
      case deSequence:
         /* Display that this is a SDP Data Element Sequence.           */
         PRINTF("%*s Type: Data Element Sequence\n", (Level*INDENT_LENGTH), "");

         /* Loop through each of the SDP Data Elements in the SDP Data  */
         /* Element Sequence.                                           */
         for(Index = 0; Index < SDPDataElement->SDP_Data_Element_Length; Index++)
         {
            /* Call this function again for each of the SDP Data        */
            /* Elements in this SDP Data Element Sequence.              */
            DisplayDataElement(&(SDPDataElement->SDP_Data_Element.SDP_Data_Element_Sequence[Index]), (Level + 1));
         }
         break;
      case deAlternative:
         /* Display that this is a SDP Data Element Alternative.        */
         PRINTF("%*s Type: Data Element Alternative\n", (Level*INDENT_LENGTH), "");

         /* Loop through each of the SDP Data Elements in the SDP Data  */
         /* Element Alternative.                                        */
         for(Index = 0; Index < SDPDataElement->SDP_Data_Element_Length; Index++)
         {
            /* Call this function again for each of the SDP Data        */
            /* Elements in this SDP Data Element Alternative.           */
            DisplayDataElement(&(SDPDataElement->SDP_Data_Element.SDP_Data_Element_Alternative[Index]), (Level + 1));
         }
         break;
      default:
         PRINTF("%*s Unknown SDP Data Element Type\n", (Level*INDENT_LENGTH), "");
         break;
   }
}

   /* ***************************************************************** */
   /*                         Event Callbacks                           */
   /* ***************************************************************** */

   /* The following function if for an Bluetooth Stack Timer Callback.  */
   /* This function will be called whenever an installed Timer has      */
   /* expired (installed via the BSC_StartTimer() function).  This      */
   /* function passes to the caller the Bluetooth Stack ID that the     */
   /* Timer is valid for, the Timer ID of the expired Timer (returned   */
   /* from BSC_StartTimer()) and the Timer Callback Parameter that was  */
   /* specified when this Callback was installed.  This function is     */
   /* guaranteed NOT to be invoked more than once simultaneously for the*/
   /* specified installed callback (i.e.  this function DOES NOT have be*/
   /* reentrant).  It should also be noted that this function is called */
   /* in the Thread Context of a Thread that the User does NOT own.     */
   /* Therefore, processing in this function should be as efficient as  */
   /* possible (this argument holds anyway because no other Timer and/or*/
   /* Stack Callbacks will be issued while function call is             */
   /* outstanding).                                                     */
   /* ** NOTE ** This function MUST NOT Block and wait for events that  */
   /*            can only be satisfied by Receiving other Stack Events. */
   /*            A Deadlock WILL occur because NO Stack Event Callbacks */
   /*            will be issued while this function is currently        */
   /*            outstanding.                                           */
static void BTPSAPI LETP_TimerCallback(unsigned int BluetoothStackID, unsigned int TimerID, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;

   /* Verify that the parameters passed in appear valid.                */
   if((BluetoothStackID) && (CallbackParameter))
   {
      /* Get a pointer to the Device Information.                       */
      DeviceInfo = (DeviceInfo_t *)CallbackParameter;

      /* Clear the Timer ID.                                            */
      DeviceInfo->LETP_TimerID = 0;

      /* Check to see if the sending process is still active.  If so,   */
      /* the we will have to wait till the current process has completed*/
      /* before we start the next interval.                             */
      if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE)
      {
         DeviceInfo->Flags |= DEVICE_INFO_FLAG_LETP_INTERVAL_EXPIRED;
      }
      else
      {
         /* Reload the send amount.  Flag that we are sending an        */
         /* schedule a callback to start the sending process.           */
         DeviceInfo->XferInfo.TxCount  = DeviceInfo->TPServerInfo.Tx_Bulk_Value;
         DeviceInfo->Flags            |= DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE;
         BSC_ScheduleAsynchronousCallback(BluetoothStackID, LETP_AsynchronousCallback, (unsigned long)DeviceInfo);

         /* Start a timer for the next interval.                        */
         DeviceInfo->LETP_TimerID = BSC_StartTimer(BluetoothStackID, DeviceInfo->TPServerInfo.Tx_Interval_Value, LETP_TimerCallback, (unsigned long)DeviceInfo);
      }
   }
}

   /* The following function is scheduled to be executed at the earliest*/
   /* convenience by the application.  This function will be called once*/
   /* for each call to BSC_ScheduleAsynchronousCallback.  This function */
   /* passes to the caller the Bluetooth Stack ID and the user specified*/
   /* Callback Parameter that was passed into the                       */
   /* BSC_ScheduleAsynchronousCallback function.  It should also be     */
   /* noted that this function is called in the Thread Context of a     */
   /* Thread that the User does NOT own.  Therefore, processing in this */
   /* function should be as efficient as possible.                      */
   /* ** NOTE ** The caller should keep the processing of these         */
   /*            Callbacks small because other Events will not be able  */
   /*            to be called while one is being serviced.              */
static void BTPSAPI LETP_AsynchronousCallback(unsigned int BluetoothStackID, unsigned long CallbackParameter)
{
   int           ret_val;
   Word_t        Length;
   DeviceInfo_t *DeviceInfo;
   int           LEConnectionIndex;

   if((BluetoothStackID) && (CallbackParameter))
   {
      /* Acquire the List Mutex.                                        */
      if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
      {
         /* Check to make sure that there is data to send.              */
         DeviceInfo = (DeviceInfo_t *)CallbackParameter;
         if(DeviceInfo->XferInfo.TxCount)
         {
            /* Locate the connection Information.                       */
            LEConnectionIndex = FindLEIndexByAddress(DeviceInfo->ConnectionBD_ADDR);
            if(LEConnectionIndex >= 0)
            {
               while(DeviceInfo->XferInfo.TxCount)
               {
                  /* Determine the max amount of data that we can send. */
                  Length = (Word_t)BEST_FIT_NOTIFICATION(DeviceInfo->ServerMTU);
                  if(!Length)
                     Length = DeviceInfo->ServerMTU;

                  if((DWord_t)Length > DeviceInfo->XferInfo.TxCount)
                     Length = (Word_t)DeviceInfo->XferInfo.TxCount;

                  /* Verify that there is enough data in the buffer.    */
                  if(Length <= DataStrLength)
                  {
                     ret_val = GATT_Handle_Value_Notification(BluetoothStackID, LETPServiceID, LEContextInfo[LEConnectionIndex].ConnectionID, LETP_TX_BULK_CHARACTERISTIC_ATTRIBUTE_OFFSET, Length, (void *)DataStrPtr);
                     if(ret_val > 0)
                     {
                        /* Adjust the amount that we have to send.      */
                        DeviceInfo->XferInfo.TxCount -= ret_val;

                        /* Check to see if we have sent all of the data.*/
                        if(!DeviceInfo->XferInfo.TxCount)
                        {
                           PRINTF("Send Complete\n");
                           DeviceInfo->Flags &= ~DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE;

                           /* Check to see if this transmission lasted  */
                           /* longer then the sending of the last       */
                           /* interval.                                 */
                           if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_INTERVAL_EXPIRED)
                           {
                              /* Clear the Expiration Flag and set the  */
                              /* sending count.                         */
                              DeviceInfo->Flags           &= ~DEVICE_INFO_FLAG_LETP_INTERVAL_EXPIRED;
                              DeviceInfo->XferInfo.TxCount  = DeviceInfo->TPServerInfo.Tx_Bulk_Value;
                              DeviceInfo->LETP_TimerID      = BSC_StartTimer(BluetoothStackID, DeviceInfo->TPServerInfo.Tx_Interval_Value, LETP_TimerCallback, (unsigned long)DeviceInfo);
                              BSC_ScheduleAsynchronousCallback(BluetoothStackID, LETP_AsynchronousCallback, (unsigned long)DeviceInfo);
                           }
                        }
                        else
                        {
                           PRINTF("Left %ld\n", DeviceInfo->XferInfo.TxCount);
                        }
                     }
                     else
                        break;
                  }
                  else
                     break;
               }
            }
         }

         /* Release the List Mutex.                                     */
         BTPS_ReleaseMutex(DeviceListMutex);
      }
   }
}

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
   int                                           LEConnectionInfo;
   BoardStr_t                                    BoardStr;
   Boolean_t                                     UpdatePrompt = TRUE;
   unsigned int                                  Index;
   DeviceInfo_t                                 *DeviceInfo;
   Long_Term_Key_t                               GeneratedLTK;
   GAP_LE_Security_Information_t                 GAP_LE_Security_Information;
   GAP_LE_Connection_Parameters_t                ConnectionParameters;
   GAP_LE_Advertising_Report_Data_t             *DeviceEntryPtr;
   GAP_LE_Authentication_Event_Data_t           *Authentication_Event_Data;
   GAP_LE_Authentication_Response_Information_t  GAP_LE_Authentication_Response_Information;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GAP_LE_Event_Data))
   {
      switch(GAP_LE_Event_Data->Event_Data_Type)
      {
         case etLE_Advertising_Report:
            for(Index = 0; Index < GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries; Index++)
            {
               DeviceEntryPtr = &(GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Advertising_Data[Index]);
               if((Result = AddDeviceToScanResults(DeviceEntryPtr)) >= 0)
               {
                  /* Display the packet type for the device             */
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

                  /* Display the Address Type.                          */
                  if(DeviceEntryPtr->Address_Type == latPublic)
                  {
                     PRINTF("  Address Type: %s.\n","atPublic");
                  }
                  else
                  {
                     PRINTF("  Address Type: %s.\n","atRandom");
                  }

                  /* Display the Device Address.                        */
                  PRINTF("  Address: 0x%02X%02X%02X%02X%02X%02X.\n", DeviceEntryPtr->BD_ADDR.BD_ADDR5, DeviceEntryPtr->BD_ADDR.BD_ADDR4, DeviceEntryPtr->BD_ADDR.BD_ADDR3, DeviceEntryPtr->BD_ADDR.BD_ADDR2, DeviceEntryPtr->BD_ADDR.BD_ADDR1, DeviceEntryPtr->BD_ADDR.BD_ADDR0);
                  PRINTF("  RSSI: %d.\n", DeviceEntryPtr->RSSI);
                  PRINTF("  Data Length: %d.\n", DeviceEntryPtr->Raw_Report_Length);

                  DisplayAdvertisingData(&(DeviceEntryPtr->Advertising_Data));
               }
               else
                  UpdatePrompt = FALSE;
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
                  /* If not already in the connection info array, add   */
                  /* it.                                                */
                  if(FindLEIndexByAddress(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address) < 0)
                  {
                     /* Find an unused position in the connection info  */
                     /* array.                                          */
                     LEConnectionInfo = FindFreeLEIndex();
                     if(LEConnectionInfo >= 0)
                        LEContextInfo[LEConnectionInfo].ConnectionBD_ADDR = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
                  }

                  /* Set a global flag to indicate if we are the        */
                  /* connection master.                                 */
                  LocalDeviceIsMaster = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master;

                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Make sure that no entry already exists.         */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address)) == NULL)
                     {
                        /* No entry exists so create one.               */
                        if(!CreateNewDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address))
                           PRINTF("Failed to add device to Device Info List.\n");
                     }
                     else
                     {
                        /* If we are the Master of the connection we    */
                        /* will attempt to Re-Establish Security if a   */
                        /* LTK for this device exists (i.e.  we         */
                        /* previously paired).                          */
                        if(LocalDeviceIsMaster)
                        {
                           /* Re-Establish Security if there is a LTK   */
                           /* that is stored for this device.           */
                           if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID)
                           {
                              /* Re-Establish Security with this LTK.   */
                              PRINTF("Attempting to Re-Establish Security.\n");

                              /* Attempt to re-establish security to    */
                              /* this device.                           */
                              GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
                              GAP_LE_Security_Information.Security_Information.Master_Information.LTK                 = DeviceInfo->LTK;
                              GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                              GAP_LE_Security_Information.Security_Information.Master_Information.Rand                = DeviceInfo->Rand;
                              GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                              Result = GAP_LE_Reestablish_Security(BluetoothStackID, DeviceInfo->ConnectionBD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                              if(Result)
                              {
                                 PRINTF("GAP_LE_Reestablish_Security returned %d.\n",Result);
                              }
                           }
                        }
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
               else
               {
                  /* Clear the Connection ID.                           */
                  RemoveConnectionInfo(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address);
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

               /* Acquire the List Mutex.                               */
               if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
               {
                  /* Check to see if the device info is present in the  */
                  /* list.                                              */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
                  {
                     /* Flag that no service discovery operation is     */
                     /* outstanding for this device.                    */
                     DeviceInfo->Flags &= (Word_t)(~DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE);

                     /* Re-initialize the Transmit and Receive Buffers, */
                     /* as well as the transmit credits.                */
                     InitializeBuffer(&(DeviceInfo->ReceiveBuffer));

                     /* Clear the CCCDs stored for this device.         */
                     DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor = 0;
                     DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor        = 0;

                     /* If this device is not paired, then delete it.   */
                     /* The LTK will be valid if the device is paired.  */
                     if(!(DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID))
                     {
                        if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address)) != NULL)
                        {
                           FreeDeviceInfoEntryMemory(DeviceInfo);
                           DeviceInfo = NULL;
                        }
                     }
                     else
                     {
                        /* Flag that the Link is no longer encrypted    */
                        /* since we have disconnected.                  */
                        DeviceInfo->Flags = DEVICE_INFO_FLAG_LTK_VALID;
                     }

                     if(DeviceInfo)
                     {
                        /* Clear the Transmit Credits count.            */
                        DeviceInfo->TransmitCredits = 0;

                        /* Clear the Client Info data.                  */
                        DeviceInfo->ClientInfo.Tx_Characteristic                         = 0;
                        DeviceInfo->ClientInfo.Tx_Client_Configuration_Descriptor        = 0;
                        DeviceInfo->ClientInfo.Rx_Characteristic                         = 0;
                        DeviceInfo->ClientInfo.Tx_Credit_Characteristic                  = 0;
                        DeviceInfo->ClientInfo.Rx_Credit_Characteristic                  = 0;
                        DeviceInfo->ClientInfo.Rx_Credit_Client_Configuration_Descriptor = 0;
                     }
                  }

                  /* Release the List Mutex.                            */
                  BTPS_ReleaseMutex(DeviceListMutex);
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
         case etLE_Encryption_Change:
            PRINTF("\netLE_Encryption_Change with size %d.\n",(int)GAP_LE_Event_Data->Event_Data_Size);

            /* Acquire the List Mutex.                                  */
            if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
            {
               /* Search for the device entry to see flag if the link is*/
               /* encrypted.                                            */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->BD_ADDR)) != NULL)
               {
                  /* Check to see if the encryption change was          */
                  /* successful.                                        */
                  if((GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Change_Status == HCI_ERROR_CODE_NO_ERROR) && (GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Change_Event_Data->Encryption_Mode == emEnabled))
                     DeviceInfo->Flags |= DEVICE_INFO_FLAG_LINK_ENCRYPTED;
                  else
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAG_LINK_ENCRYPTED;
               }

               /* Release the List Mutex.                               */
               BTPS_ReleaseMutex(DeviceListMutex);
            }
            break;
         case etLE_Encryption_Refresh_Complete:
            PRINTF("\netLE_Encryption_Refresh_Complete with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            /* Acquire the List Mutex.                                  */
            if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
            {
               /* Search for the device entry to see flag if the link is*/
               /* encrypted.                                            */
               if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->BD_ADDR)) != NULL)
               {
                  /* Check to see if the refresh was successful.        */
                  if(GAP_LE_Event_Data->Event_Data.GAP_LE_Encryption_Refresh_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
                     DeviceInfo->Flags |= DEVICE_INFO_FLAG_LINK_ENCRYPTED;
                  else
                     DeviceInfo->Flags &= ~DEVICE_INFO_FLAG_LINK_ENCRYPTED;
               }

               /* Release the List Mutex.                               */
               BTPS_ReleaseMutex(DeviceListMutex);
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

                     /* Acquire the List Mutex.                         */
                     if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                     {
                        /* Determine if we have previously paired with  */
                        /* the device.  If we have paired we will       */
                        /* attempt to re-establish security using a     */
                        /* previously exchanged LTK.                    */
                        if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                        {
                           /* Determine if a Valid Long Term Key is     */
                           /* stored for this device.                   */
                           if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID)
                           {
                              PRINTF("Attempting to Re-Establish Security.\n");

                              /* Attempt to re-establish security to    */
                              /* this device.                           */
                              GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
                              GAP_LE_Security_Information.Security_Information.Master_Information.LTK                 = DeviceInfo->LTK;
                              GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                              GAP_LE_Security_Information.Security_Information.Master_Information.Rand                = DeviceInfo->Rand;
                              GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                              Result = GAP_LE_Reestablish_Security(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                              if(Result)
                              {
                                 PRINTF("GAP_LE_Reestablish_Security returned %d.\n",Result);
                              }
                           }
                           else
                           {
                              CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                              /* We do not have a stored Link Key for   */
                              /* this device so go ahead and pair to    */
                              /* this device.                           */
                              SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                           }
                        }
                        else
                        {
                           CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                           /* There is no Key Info Entry for this device*/
                           /* so we will just treat this as a slave     */
                           /* request and initiate pairing.             */
                           SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                        }

                        /* Release the List Mutex.                      */
                        BTPS_ReleaseMutex(DeviceListMutex);
                     }
                     break;
                  case latPairingRequest:
                     CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

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
                     ASSIGN_BD_ADDR(CurrentLERemoteBD_ADDR, 0, 0, 0, 0, 0, 0);

                     PRINTF("Pairing Device: %s, Status 0x%02X.\n", BoardStr, Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status);

                     if(Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status == GAP_LE_PAIRING_STATUS_NO_ERROR)
                     {
                        PRINTF("      Key Size: %d.\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size);
                     }
                     else
                     {
                        /* Acquire the List Mutex.                      */
                        if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                        {
                           /* Failed to pair so delete the key entry for*/
                           /* this device and disconnect the link.      */
                           if((DeviceInfo = DeleteDeviceInfoEntry(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                              FreeDeviceInfoEntryMemory(DeviceInfo);

                           /* Disconnect the Link.                      */
                           GAP_LE_Disconnect(BluetoothStackID, Authentication_Event_Data->BD_ADDR);

                           /* Release the List Mutex.                   */
                           BTPS_ReleaseMutex(DeviceListMutex);
                        }
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
                     PRINTF(" Encryption Information from RemoteDevice: %s\n", BoardStr);
                     PRINTF("                                 Key Size: %d\n", Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size);
                     DisplayKey("                                 Link Key: 0x", sizeof(Link_Key_t), (Byte_t *)&Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK);
                     DisplayKey("                                 Rand    : 0x", sizeof(Random_Number_t), (Byte_t *)&(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand));
                     PRINTF("                                 EDIV    : %04X\n", Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV);

                     /* Acquire the List Mutex.                         */
                     if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                     {
                        /* Search for the entry for this slave to store */
                        /* the information into.                        */
                        if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, Authentication_Event_Data->BD_ADDR)) != NULL)
                        {
                           DeviceInfo->LTK               = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK;
                           DeviceInfo->EDIV              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.EDIV;
                           DeviceInfo->Rand              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Rand;
                           DeviceInfo->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size;
                           DeviceInfo->Flags            |= DEVICE_INFO_FLAG_LTK_VALID;
                        }
                        else
                        {
                           PRINTF("No Key Info Entry for this Slave.\n");
                        }

                        /* Release the List Mutex.                      */
                        BTPS_ReleaseMutex(DeviceListMutex);
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
      if(UpdatePrompt)
         DisplayPrompt();
   }
}

   /* The following function is for an GATT Server Event Callback.  This*/
   /* function will be called whenever a GATT Request is made to the    */
   /* server who registers this function that cannot be handled         */
   /* internally by GATT.  This function passes to the caller the GATT  */
   /* Server Event Data that occurred and the GATT Server Event Callback*/
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the GATT Server Event   */
   /* Data ONLY in the context of this callback.  If the caller requires*/
   /* the Data for a longer period of time, then the callback function  */
   /* MUST copy the data into another Data Buffer.  This function is    */
   /* guaranteed NOT to be invoked more than once simultaneously for the*/
   /* specified installed callback (i.e.  this function DOES NOT have be*/
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another GATT Event (Server/Client or Connection) will not be      */
   /* processed while this function call is outstanding).               */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_LETP_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
   int           LEConnectionIndex;
   DWord_t       Value;
   Word_t        AttributeLength;
   Word_t        AttributeOffset;
   DeviceInfo_t *DeviceInfo;

   UNREFERENCED_PARAM(CallbackParameter);

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_ServerEventData))
   {
      switch(GATT_ServerEventData->Event_Data_Type)
      {
         case etGATT_Server_Write_Request:
            /* Verify that the Event Data is valid.                     */
            if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data)
            {
               if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset == 0)
               {
                  /* Find the LE Connection Index for this connection.  */
                  if((LEConnectionIndex = FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice)) >= 0)
                  {
                     /* Acquire the List Mutex.                         */
                     if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                     {
                        /* Grab the device info for the currently       */
                        /* connected device.                            */
                        if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                        {
                           /* Cache the Attribute Offset.               */
                           AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;
                           AttributeLength = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;

                           if(AttributeOffset == LETP_TX_INTERVAL_CHARACTERISTIC_ATTRIBUTE_OFFSET)
                           {
                              /* Verify that the value is of the correct*/
                              /* length.                                */
                              if((AttributeLength) && (AttributeLength <= WORD_SIZE))
                              {
                                 /* Since the value appears valid go    */
                                 /* ahead and accept the write request. */
                                 GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);

                                 Value = 0;
                                 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == WORD_SIZE)
                                    Value = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                                 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == BYTE_SIZE)
                                    Value = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);

                                 /* Write the new value to the          */
                                 /* characteristic.                     */
                                 DeviceInfo->TPServerInfo.Tx_Interval_Value = (Word_t)Value;
                              }
                              else
                                 GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
                           }

                           if(AttributeOffset == LETP_TX_BULK_CHARACTERISTIC_ATTRIBUTE_OFFSET)
                           {
                              /* Verify that the value is of the correct*/
                              /* length.                                */
                              if((AttributeLength) && (AttributeLength <= DWORD_SIZE))
                              {
                                 /* Since the value appears valid go    */
                                 /* ahead and accept the write request. */
                                 GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);

                                 Value = 0;
                                 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == DWORD_SIZE)
                                    Value = READ_UNALIGNED_DWORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                                 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == WORD_SIZE)
                                    Value = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                                 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == BYTE_SIZE)
                                    Value = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);

                                 /* Write the new value to the          */
                                 /* characteristic.                     */
                                 DeviceInfo->TPServerInfo.Tx_Bulk_Value = Value;
                                 if(DeviceInfo->TPServerInfo.Tx_Bulk_Client_Configuration == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                                 {
                                    if(DeviceInfo->TPServerInfo.Tx_Bulk_Value)
                                    {
                                       PRINTF("Notifying %lu Bytes\n", DeviceInfo->TPServerInfo.Tx_Bulk_Value);
                                       DeviceInfo->XferInfo.TxCount = DeviceInfo->TPServerInfo.Tx_Bulk_Value;
                                       DeviceInfo->Flags           |= (DEVICE_INFO_FLAG_LETP_SERVER + DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE);
                                       BSC_ScheduleAsynchronousCallback(BluetoothStackID, LETP_AsynchronousCallback, (unsigned long)DeviceInfo);
                                    }
                                 }
                              }
                              else
                                 GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
                           }

                           if(AttributeOffset == LETP_TX_BULK_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET)
                           {
                              if((AttributeLength) && (AttributeLength <= WORD_SIZE))
                              {
                                 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == WORD_SIZE)
                                    Value = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                                 else
                                    Value = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);

                                 /* Note the updated Tx Bulk CCCD Value.*/
                                 DeviceInfo->TPServerInfo.Tx_Bulk_Client_Configuration = (Word_t)Value;

                                 /* Since the value appears valid go    */
                                 /* ahead and accept the write request. */
                                 GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);

                                 /* Check to see if we should start     */
                                 /* sending data.                       */
                                 if(DeviceInfo->TPServerInfo.Tx_Bulk_Client_Configuration == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                                 {
                                    /* Check to see if a limit was      */
                                    /* specified.                       */
                                    if(DeviceInfo->TPServerInfo.Tx_Bulk_Value)
                                    {
                                       PRINTF("Notifying %lu Bytes\n", DeviceInfo->TPServerInfo.Tx_Bulk_Value);
                                       DeviceInfo->XferInfo.TxCount = DeviceInfo->TPServerInfo.Tx_Bulk_Value;
                                       DeviceInfo->Flags           |= DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE;
                                       BSC_ScheduleAsynchronousCallback(BluetoothStackID, LETP_AsynchronousCallback, (unsigned long)DeviceInfo);
                                       /* Check to see if the           */
                                       /* transmission is to be         */
                                       /* scheduled on some interval.   */
                                       if(DeviceInfo->TPServerInfo.Tx_Interval_Value)
                                       {
                                          DeviceInfo->LETP_TimerID = BSC_StartTimer(BluetoothStackID, DeviceInfo->TPServerInfo.Tx_Interval_Value, LETP_TimerCallback, (unsigned long)DeviceInfo);
                                       }
                                    }
                                 }
                                 else
                                 {
                                    /* Since Notifications has been     */
                                    /* disabled, Clear the Sending and  */
                                    /* Interval Expired flag and kill   */
                                    /* any timer that may be running.   */
                                    DeviceInfo->Flags &= ~(DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE | DEVICE_INFO_FLAG_LETP_INTERVAL_EXPIRED);
                                    if(DeviceInfo->LETP_TimerID)
                                    {
                                       BSC_StopTimer(BluetoothStackID, DeviceInfo->LETP_TimerID);
                                       DeviceInfo->LETP_TimerID = 0;
                                    }
                                 }
                              }
                              else
                                 GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
                           }
                        }
                        else
                           GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);

                        /* Release the List Mutex.                      */
                        BTPS_ReleaseMutex(DeviceListMutex);
                     }
                  }
                  else
                     GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
               }
               else
                  GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
            }
            else
               PRINTF("Invalid Write Request Event Data.\n");
         default:
            break;
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

   /* The following function is for an GATT Server Event Callback.  This*/
   /* function will be called whenever a GATT Request is made to the    */
   /* server who registers this function that cannot be handled         */
   /* internally by GATT.  This function passes to the caller the GATT  */
   /* Server Event Data that occurred and the GATT Server Event Callback*/
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the GATT Server Event   */
   /* Data ONLY in the context of this callback.  If the caller requires*/
   /* the Data for a longer period of time, then the callback function  */
   /* MUST copy the data into another Data Buffer.  This function is    */
   /* guaranteed NOT to be invoked more than once simultaneously for the*/
   /* specified installed callback (i.e.  this function DOES NOT have be*/
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another GATT Event (Server/Client or Connection) will not be      */
   /* processed while this function call is outstanding).               */
   /* * NOTE * This function MUST NOT Block and wait for Events that can*/
   /*          only be satisfied by Receiving a Bluetooth Event         */
   /*          Callback.  A Deadlock WILL occur because NO Bluetooth    */
   /*          Callbacks will be issued while this function is currently*/
   /*          outstanding.                                             */
static void BTPSAPI GATT_SPPLE_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter)
{
   int           LEConnectionIndex;
   Byte_t        Temp[2];
   Word_t        Value;
   Word_t        PreviousValue;
   Word_t        AttributeLength;
   Word_t        AttributeOffset;
   DeviceInfo_t *DeviceInfo;

   UNREFERENCED_PARAM(CallbackParameter);

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_ServerEventData))
   {
      switch(GATT_ServerEventData->Event_Data_Type)
      {
         case etGATT_Server_Read_Request:
            /* Verify that the Event Data is valid.                     */
            if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data)
            {
               /* Verify that the read isn't a read blob (no SPPLE      */
               /* readable characteristics are long).                   */
               if(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeValueOffset == 0)
               {
                  /* Find the LE Connection Index for this connection.  */
                  if((LEConnectionIndex = FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->RemoteDevice)) >= 0)
                  {
                     /* Acquire the List Mutex.                         */
                     if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                     {
                        /* Grab the device info for the currently       */
                        /* connected device.                            */
                        if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                        {
                           /* Determine which request this read is      */
                           /* coming for.                               */
                           switch(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset)
                           {
                              case SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                 ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor);
                                 break;
                              case SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                 ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitCredits);
                                 break;
                              case SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                 ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);
                                 break;
                              case SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                 ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor);
                                 break;
                           }

                           GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, Temp);
                        }
                        else
                        {
                           PRINTF("Error - No device info entry for this device.\n");
                        }

                        /* Release the List Mutex.                      */
                        BTPS_ReleaseMutex(DeviceListMutex);
                     }
                  }
                  else
                  {
                     PRINTF("Error - No such device connected.\n");
                  }
               }
               else
                  GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
            }
            else
               PRINTF("Invalid Read Request Event Data.\n");
            break;
         case etGATT_Server_Write_Request:
            /* Verify that the Event Data is valid.                     */
            if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data)
            {
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->RemoteDevice)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Grab the device info for the currently connected*/
                     /* device.                                         */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset == 0)
                        {
                           /* Cache the Attribute Offset.               */
                           AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;
                           AttributeLength = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;

                           /* Check to see if this write is OK for our  */
                           /* role.                                     */
                           if((AttributeOffset == SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET) || (AttributeOffset == SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET))
                           {
                              /* Check to see if we know if we are the  */
                              /* Client or Server.                      */
                              if(!(DeviceInfo->Flags & (DEVICE_INFO_FLAG_SPPLE_CLIENT | DEVICE_INFO_FLAG_SPPLE_SERVER)))
                              {
                                 /* We will be the Server for this      */
                                 /* device.                             */
                                 DeviceInfo->Flags |= DEVICE_INFO_FLAG_SPPLE_SERVER;
                              }
                              else
                              {
                                 /* This indicates that we are acting as*/
                                 /* a Server.  Error the request if we  */
                                 /* are a Client                        */
                                 if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SPPLE_CLIENT)
                                 {
                                    /* Cause the Request to be invalid. */
                                    AttributeLength = 0;
                                 }
                              }
                           }

                           /* Verify that the value is of the correct   */
                           /* length.                                   */
                           if((AttributeOffset == SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET) || ((AttributeLength) && (AttributeLength <= WORD_SIZE)))
                           {
                              /* Since the value appears valid go ahead */
                              /* and accept the write request.          */
                              GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);

                              /* If this is not a write to the Rx       */
                              /* Characteristic we will read the data   */
                              /* here.                                  */
                              if(AttributeOffset != SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET)
                              {
                                 if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == WORD_SIZE)
                                    Value = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                                 else
                                    Value = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                              }
                              else
                                 Value = 0;

                              /* Determine which attribute this write   */
                              /* request is for.                        */
                              switch(AttributeOffset)
                              {
                                 case SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                    /* Client has updated the Tx CCCD.  */
                                    /* Now we need to check if we have  */
                                    /* any data to send.                */
                                    DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor = Value;

                                    /* If may be possible for transmit  */
                                    /* queued data now.  So fake a      */
                                    /* Receive Credit event with 0 as   */
                                    /* the received credits.            */
                                    SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, 0);
                                    break;
                                 case SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                    /* Client has sent updated credits. */
                                    SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, Value);
                                    break;
                                 case SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                    /* Client has sent data, so we      */
                                    /* should handle this as a data     */
                                    /* indication event.                */
                                    SPPLEDataIndicationEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);

                                    if((DisplayRawData) || (!AutomaticReadActive))
                                       DisplayPrompt();
                                    break;
                                 case SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                    /* Cache the previous CCD Value.    */
                                    PreviousValue = DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor;

                                    /* Note the updated Rx CCCD Value.  */
                                    DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor = Value;

                                    /* If we were not previously        */
                                    /* configured for notifications send*/
                                    /* the initial credits to the       */
                                    /* device.                          */
                                    if(PreviousValue != GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                                    {
                                       /* Send the initial credits to   */
                                       /* the device.                   */
                                       SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);
                                    }
                                    break;
                              }
                           }
                           else
                              GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_INVALID_ATTRIBUTE_VALUE_LENGTH);
                        }
                        else
                           GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
                     }
                     else
                     {
                        PRINTF("Error - No device info entry for this device.\n");
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
               else
               {
                  PRINTF("Error - No such device connected.");
               }
            }
            else
               PRINTF("Invalid Write Request Event Data.\n");
         default:
            break;
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
static void BTPSAPI GATT_ClientEventCallback(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter)
{
   int           ret_val;
   int           LEConnectionIndex;
   Word_t        Credits;
   Word_t        Handle;
   BoardStr_t    BoardStr;
   Word_t        Appearance;
   char         *NameBuffer;
   DeviceInfo_t *DeviceInfo;
   Boolean_t     ShowPrompt = TRUE;

   UNREFERENCED_PARAM(CallbackParameter);

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
                /* Only print out the rest if it is valid.              */
               if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)
               {
                  PRINTF("   Request Opcode:  0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestOpCode);
                  PRINTF("   Request Handle:  0x%04X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestHandle);
                  PRINTF("   Error Code:      0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode);

                  if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode < NUMBER_GATT_ERROR_CODES)
                  {
                     PRINTF("   Error Msg:       %s.\n", ErrorCodeStr[GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode]);
                  }
                  else
                  {
                     PRINTF("   Error Msg:       Unknown.\n");
                  }
               }
            }
            else
               PRINTF("Error - Null Error Response Data.\n");
            break;
         case etGATT_Client_Read_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data)
            {
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->RemoteDevice)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Grab the device info for the currently connected*/
                     /* device.                                         */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        if((Word_t)CallbackParameter == DeviceInfo->ClientInfo.Rx_Credit_Characteristic)
                        {
                           /* Make sure this is the correct size for a  */
                           /* Rx Credit Characteristic.                 */
                           if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength == WORD_SIZE)
                           {
                              /* Display the credits we just received.  */
                              Credits = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue);
                              PRINTF("\nReceived %u Initial Credits.\n", Credits);

                              /* We have received the initial credits   */
                              /* from the device so go ahead and handle */
                              /* a Receive Credit Event.                */
                              SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, Credits);
                           }
                        }
                        if((Word_t)CallbackParameter == DeviceInfo->GAPSClientInfo.DeviceNameHandle)
                        {
                           /* Display the remote device name.           */
                           if((NameBuffer = (char *)BTPS_AllocateMemory(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength+1)) != NULL)
                           {
                              BTPS_MemInitialize(NameBuffer, 0, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength+1);
                              BTPS_MemCopy(NameBuffer, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);

                              PRINTF("\nRemote Device Name: %s.\n", NameBuffer);

                              BTPS_FreeMemory(NameBuffer);
                           }
                        }
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
                        if((Word_t)CallbackParameter == DeviceInfo->DISClientInfo.ManufacturerNameHandle)
                        {
                           /* Don't display the prompt as we are        */
                           /* displaying the Device Information.        */
                           ShowPrompt = FALSE;

                           /* Copy the result data to a local buffer and*/
                           /* terminate the data with a NULL.           */
                           if(GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength)
                           {
                              BTPS_MemCopy(DisBuf, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValue, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength);
                              DisBuf[GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->AttributeValueLength] = 0;
                           }
                           else
                              DisBuf[0] = 0;

                           switch(DeviceInfo->DISHandleIndex)
                           {
                              case DIS_MAN_NAME_HANDLE_OFFSET:
                                 PRINTF("Manufacturer Name: %s\n", (DisBuf[0])?DisBuf:"NULL");
                                 break;
                              case DIS_MODEL_NUM_HANDLE_OFFSET:
                                 PRINTF("     Model Number: %s\n", (DisBuf[0])?DisBuf:"NULL");
                                 break;
                              case DIS_SERIAL_NUM_HANDLE_OFFSET:
                                 PRINTF("    Serial Number: %s\n", (DisBuf[0])?DisBuf:"NULL");
                                 break;
                              case DIS_HARDWARE_REV_HANDLE_OFFSET:
                                 PRINTF("Hardware Revision: %s\n", (DisBuf[0])?DisBuf:"NULL");
                                 break;
                              case DIS_FIRMWARE_REV_HANDLE_OFFSET:
                                 PRINTF("Firmware Revision: %s\n", (DisBuf[0])?DisBuf:"NULL");
                                 break;
                              case DIS_SOFTWARE_REV_HANDLE_OFFSET:
                                 PRINTF("Software Revision: %s\n", (DisBuf[0])?DisBuf:"NULL");
                                 break;
                              case DIS_SYSTEM_ID_HANDLE_OFFSET:
                                 PRINTF("System ID - ManID: 0x%02X%02X%02X%02X%02X\n", DisBuf[0], DisBuf[1], DisBuf[2], DisBuf[3], DisBuf[4]);
                                 PRINTF("              OUI: 0x%02X%02X%02X\n", DisBuf[5], DisBuf[6], DisBuf[7]);
                                 break;
                              case DIS_IEEE_CERT_HANDLE_OFFSET:
                                 PRINTF("   IEEE Cert Data: %s\n", (DisBuf[0])?DisBuf:"NULL");
                                 break;
                              case DIS_PNP_ID_HANDLE_OFFSET:
                                 PRINTF("  PnP ID - Source: %d\n", DisBuf[0]);
                                 PRINTF("              VID: 0x%04X\n", READ_UNALIGNED_WORD_LITTLE_ENDIAN(&DisBuf[1]));
                                 PRINTF("              PID: 0x%04X\n", READ_UNALIGNED_WORD_LITTLE_ENDIAN(&DisBuf[3]));
                                 PRINTF("          Version: 0x%04X\n", READ_UNALIGNED_WORD_LITTLE_ENDIAN(&DisBuf[5]));
                                 break;
                           }

                           Handle = 0;
                           while(!Handle)
                           {
                              DeviceInfo->DISHandleIndex++;
                              if(DeviceInfo->DISHandleIndex <= DIS_PNP_ID_HANDLE_OFFSET)
                                 Handle  = ((Word_t *)&DeviceInfo->DISClientInfo)[DeviceInfo->DISHandleIndex];
                              else
                                 break;
                           }
                           if(!Handle)
                           {
                              ShowPrompt                  = TRUE;
                              DeviceInfo->DISHandleIndex  = 0;
                              DeviceInfo->Flags          &= (Word_t)(~DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE);
                              PRINTF("\nDIS Search Complete\n");
                           }
                           else
                           {
                              ret_val = GATT_Read_Value_Request(BluetoothStackID, GATT_Client_Event_Data->Event_Data.GATT_Read_Response_Data->ConnectionID, Handle, GATT_ClientEventCallback, (unsigned long)DeviceInfo->DISClientInfo.ManufacturerNameHandle);
                              if(ret_val < 0)
                              {
                                 PRINTF("\nError reading remote DIS information\n");
                                 ShowPrompt                  = TRUE;
                                 DeviceInfo->DISHandleIndex  = 0;
                                 DeviceInfo->Flags          &= (Word_t)(~DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE);
                              }
                           }
                        }
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
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
            }
            else
               PRINTF("\nError - Null Write Response Data.\n");
         default:
            break;
      }

      /* Print the command line prompt.                                 */
      if(ShowPrompt)
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
   int ret_val;
   int           LEConnectionIndex;
   Word_t        Credits;
   Boolean_t     SuppressResponse = FALSE;
   BoardStr_t    BoardStr;
   DeviceInfo_t *DeviceInfo;

   UNREFERENCED_PARAM(CallbackParameter);

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GATT_Connection_Event_Data))
   {
      /* Determine the Connection Event that occurred.                  */
      switch(GATT_Connection_Event_Data->Event_Data_Type)
      {
         case etGATT_Connection_Device_Connection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data)
            {
               /* Update the ConnectionID associated with the BD_ADDR   */
               /* If UpdateConnectionID returns -1, then it failed.     */
               if(UpdateConnectionID(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID, GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice) < 0)
                   PRINTF("Error - No matching ConnectionBD_ADDR found.");

               PRINTF("\netGATT_Connection_Device_Connection with size %u: \n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID);
               PRINTF("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:   %s.\n", BoardStr);
               PRINTF("   GATT MTU:        %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU-3);

               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Search for the device info for the connection.  */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        /* Initialize the Transmit and Receive Buffers. */
                        InitializeBuffer(&(LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer));

                        /* Flag that we do not have any transmit credits*/
                        /* yet.                                         */
                        LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.TransmitCredits = 0;

                        /* Flag that no credits are queued.             */
                        LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.QueuedCredits   = 0;

                        if(LocalDeviceIsMaster)
                        {
                           /* Attempt to update the MTU to the maximum  */
                           /* supported.                                */
                           ret_val = GATT_Exchange_MTU_Request(BluetoothStackID, GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID, SPPLE_DATA_BUFFER_LENGTH, GATT_ClientEventCallback, 0);
                           if(ret_val < 0)
                              PRINTF("Failed to set MTU %d\n", ret_val);
                        }

                        /* Check to see if we are bonded and the Tx     */
                        /* Credit notifications have been enabled.      */
                        if((DeviceInfo->Flags & (DEVICE_INFO_FLAG_LTK_VALID | DEVICE_INFO_FLAG_SPPLE_SERVER)) == (DEVICE_INFO_FLAG_LTK_VALID | DEVICE_INFO_FLAG_SPPLE_SERVER))
                        {
                           if(DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                           {
                              /* Send the Initial Credits if the Rx     */
                              /* Credit CCD is already configured (for a*/
                              /* bonded device this could be the case). */
                              SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, LEContextInfo[LEConnectionIndex].SPPLEBufferInfo.ReceiveBuffer.BytesFree);
                           }
                        }
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
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
               RemoveConnectionInfo(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice);

               PRINTF("\netGATT_Connection_Device_Disconnection with size %u: \n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionID);
               PRINTF("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:   %s.\n", BoardStr);
            }
            else
               PRINTF("Error - Null Disconnection Data.\n");
            break;
         case etGATT_Connection_Device_Connection_MTU_Update:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data)
            {
               PRINTF("\r\etGATT_Connection_Device_Connection_MTU_Update\n");
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->RemoteDevice, BoardStr);
               PRINTF("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->ConnectionID);
               PRINTF("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               PRINTF("   Remote Device:   %s.\n", BoardStr);
               PRINTF("   GATT MTU:        %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU-3);

               SetDataStrBuffer(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU);

               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->RemoteDevice)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Grab the device info for the currently connected*/
                     /* device.                                         */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        DeviceInfo->ServerMTU = GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->MTU;
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
            }
            break;
         case etGATT_Connection_Device_Buffer_Empty:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Buffer_Empty_Data)
            {
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Device_Buffer_Empty_Data->RemoteDevice)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Grab the device info for the currently connected*/
                     /* device.                                         */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        if(LEContextInfo[LEConnectionIndex].BufferFull)
                        {
                           /* Flag that the buffer is no longer empty.  */
                           LEContextInfo[LEConnectionIndex].BufferFull = FALSE;

                           /* Attempt to send any queued credits that we*/
                           /* may have.                                 */
                           SPPLESendCredits(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, 0);

                           /* If may be possible for transmit queued    */
                           /* data now.  So fake a Receive Credit event */
                           /* with 0 as the received credits.           */
                           SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, 0);

                           /* Suppress the command prompt.              */
                           SuppressResponse   = TRUE;
                        }
                        /* Check to see if the LETP Server is           */
                        /* configured.                                  */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_SENDING_ACTIVE)
                        {
                           /* Check to see if Notifications are still   */
                           /* enabled.                                  */
                           if(DeviceInfo->TPServerInfo.Tx_Bulk_Client_Configuration & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                           {
                              /* Verify that there is data to send.     */
                              if(DeviceInfo->XferInfo.TxCount)
                              {
                                 BSC_ScheduleAsynchronousCallback(BluetoothStackID, LETP_AsynchronousCallback, (unsigned long)DeviceInfo);
                              }
                           }
                           /* Suppress the command prompt.              */
                           SuppressResponse   = TRUE;
                        }
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
            }
            break;
         case etGATT_Connection_Server_Notification:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data)
            {
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByAddress(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->RemoteDevice)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Find the Device Info for the device that has    */
                     /* sent us the notification.                       */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        /* Determine the characteristic that is being   */
                        /* notified.                                    */
                        if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeHandle == DeviceInfo->ClientInfo.Rx_Credit_Characteristic)
                        {
                           /* Verify that the length of the Rx Credit   */
                           /* Notification is correct.                  */
                           if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength == WORD_SIZE)
                           {
                              Credits = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValue);

                              /* Handle the received credits event.     */
                              SPPLEReceiveCreditEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, Credits);

                              /* Suppress the command prompt.           */
                              SuppressResponse   = TRUE;
                           }
                        }
                        else
                        {
                           if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeHandle == DeviceInfo->ClientInfo.Tx_Characteristic)
                           {
                              /* This is a Tx Characteristic Event.  So */
                              /* call the function to handle the data   */
                              /* indication event.                      */
                              SPPLEDataIndicationEvent(&(LEContextInfo[LEConnectionIndex]), DeviceInfo, GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength, GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValue);

                              /* If we are not looping back or doing    */
                              /* automatic reads we will display the    */
                              /* prompt.                                */
                              if(!AutomaticReadActive)
                                 SuppressResponse = FALSE;
                              else
                                 SuppressResponse = TRUE;
                           }
                           else
                           {
                              if(GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeHandle == DeviceInfo->TPClientInfo.Tx_Bulk_Characteristic)
                              {
                                 if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LETP_TIMING_ACTIVE)
                                 {
                                    if(DeviceInfo->XferInfo.SeenFirst == FALSE)
                                    {
                                       DeviceInfo->XferInfo.SeenFirst       = TRUE;
                                       DeviceInfo->XferInfo.FirstPacketSize = GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength;
                                       CAPTURE_TIMESTAMP(&DeviceInfo->XferInfo.FirstTime);
                                    }
                                    CAPTURE_TIMESTAMP(&DeviceInfo->XferInfo.LastTime);
                                    DeviceInfo->XferInfo.RxCount += GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength;
                                 }
                                 SuppressResponse = TRUE;
                                 PRINTF("Rx %d Bytes\n", GATT_Connection_Event_Data->Event_Data.GATT_Server_Notification_Data->AttributeValueLength);
                              }
                           }
                        }
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
            }
            else
               PRINTF("Error - Null Server Notification Data.\n");
         default:
            break;
      }

      /* Print the command line prompt.                                 */
      if(!SuppressResponse)
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
   int           ret_val;
   int           LEConnectionIndex;
   Boolean_t     ShowPrompt = TRUE;
   DeviceInfo_t *DeviceInfo;

   UNREFERENCED_PARAM(CallbackParameter);

   /* Verify that the input parameters are semi-valid.                  */
   if((BluetoothStackID) && (GATT_Service_Discovery_Event_Data))
   {
      switch(GATT_Service_Discovery_Event_Data->Event_Data_Type)
      {
         case etGATT_Service_Discovery_Indication:
            /* Verify the event data.                                   */
            if(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data)
            {
               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByConnectionID(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ConnectionID)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Find the device info for this connection.       */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        PRINTF("\n");
                        PRINTF("Service 0x%04X - 0x%04X, UUID: ", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.Service_Handle, GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.End_Group_Handle);
                        DisplayUUID(&(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data->ServiceInformation.UUID));
                        PRINTF("\n");

                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_DIS)
                        {
                           /* Attempt to populate the handles for the   */
                           /* DIS Service.                              */
                           DISPopulateHandles(&(DeviceInfo->DISClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                        }
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_GAPS)
                        {
                           /* Attempt to populate the handles for the   */
                           /* GAP Service.                              */
                           GAPSPopulateHandles(&(DeviceInfo->GAPSClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                        }
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_SPPLE)
                        {
                           /* Attempt to populate the handles for the   */
                           /* SPPLE Service.                            */
                           SPPLEPopulateHandles(&(DeviceInfo->ClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                        }
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_LETP)
                        {
                           /* Attempt to populate the handles for the   */
                           /* SPPLE Service.                            */
                           LETPPopulateHandles(&(DeviceInfo->TPClientInfo), GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Indication_Data);
                        }
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
            }
            break;
         case etGATT_Service_Discovery_Complete:
            /* Verify the event data.                                   */
            if(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data)
            {
               PRINTF("\n");
               PRINTF("Service Discovery Operation Complete, Status 0x%02X.\n", GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data->Status);

               /* Find the LE Connection Index for this connection.     */
               if((LEConnectionIndex = FindLEIndexByConnectionID(GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data->ConnectionID)) >= 0)
               {
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Find the device info for this connection.       */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(&DeviceInfoList, LEContextInfo[LEConnectionIndex].ConnectionBD_ADDR)) != NULL)
                     {
                        /* Flag that no service discovery operation is  */
                        /* outstanding for this device.                 */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_DIS)
                        {
                           /* Start to Query the Handles discovered.    */
                           PRINTF("\nQuery Remote PnP Information\n\n");
                           ShowPrompt = FALSE;

                           /* Dispatch a Callback to query the handles. */
                           ret_val = GATT_Read_Value_Request(BluetoothStackID, GATT_Service_Discovery_Event_Data->Event_Data.GATT_Service_Discovery_Complete_Data->ConnectionID, DeviceInfo->DISClientInfo.ManufacturerNameHandle, GATT_ClientEventCallback, (unsigned long)DeviceInfo->DISClientInfo.ManufacturerNameHandle);
                           if(ret_val < 0)
                           {
                              PRINTF("Error Reading DIS Attribute\n");
                              DeviceInfo->DISHandleIndex  = 0;
                              DeviceInfo->Flags          &= (Word_t)(~DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE);
                           }
                        }
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_LETP)
                        {
                           DeviceInfo->Flags &= (Word_t)(~DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE);
                           if(LETP_CLIENT_INFORMATION_VALID(DeviceInfo->TPClientInfo))
                              PRINTF("\nValid LETP Service Found.\n");
                           else
                              PRINTF("\nNo LETP Service Found.\n");
                        }
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_GAPS)
                        {
                           DeviceInfo->Flags &= (Word_t)(~DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE);
                        }
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_SERVICE_DISCOVERY_SPPLE)
                        {
                           DeviceInfo->Flags &= (Word_t)(~DEVICE_INFO_FLAG_SERVICE_DISCOVERY_IDLE);
                           if(SPPLE_CLIENT_INFORMATION_VALID(DeviceInfo->ClientInfo))
                              PRINTF("\nValid SPPLE Service Found.\n");
                           else
                              PRINTF("\nNo SPPLE Service Found.\n");
                        }
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
            }
         default:
            break;
      }
      if(ShowPrompt)
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

   /* The following function is for the GAP Event Receive Data Callback.*/
   /* This function will be called whenever a Callback has been         */
   /* registered for the specified GAP Action that is associated with   */
   /* the Bluetooth Stack.  This function passes to the caller the GAP  */
   /* Event Data of the specified Event and the GAP Event Callback      */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the GAP Event Data ONLY */
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer.  This function is guaranteed   */
   /* NOT to be invoked more than once simultaneously for the specified */
   /* installed callback (i.e.  this function DOES NOT have be          */
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* other GAP Events will not be processed while this function call is*/
   /* outstanding).                                                     */
   /* * NOTE * This function MUST NOT Block and wait for events that    */
   /*          can only be satisfied by Receiving other GAP Events.  A  */
   /*          Deadlock WILL occur because NO GAP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter)
{
   int                               Result;
   int                               Index;
   BD_ADDR_t                         NULL_BD_ADDR;
   Boolean_t                         OOB_Data;
   Boolean_t                         MITM;
   BoardStr_t                        Callback_BoardStr;
   GAP_IO_Capability_t               RemoteIOCapability;
   GAP_Inquiry_Event_Data_t         *GAP_Inquiry_Event_Data;
   GAP_Remote_Name_Event_Data_t     *GAP_Remote_Name_Event_Data;
   GAP_Authentication_Information_t  GAP_Authentication_Information;

   UNREFERENCED_PARAM(CallbackParameter);

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (GAP_Event_Data))
   {
      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming event is.                                    */
      switch(GAP_Event_Data->Event_Data_Type)
      {
         case etInquiry_Result:
            /* The GAP event received was of type Inquiry_Result.       */
            GAP_Inquiry_Event_Data = GAP_Event_Data->Event_Data.GAP_Inquiry_Event_Data;

            /* Next, Check to see if the inquiry event data received    */
            /* appears to be semi-valid.                                */
            if(GAP_Inquiry_Event_Data)
            {
               /* Now, check to see if the gap inquiry event data's     */
               /* inquiry data appears to be semi-valid.                */
               if(GAP_Inquiry_Event_Data->GAP_Inquiry_Data)
               {
                  PRINTF("\n");

                  /* Display a list of all the devices found from       */
                  /* performing the inquiry.                            */
                  for(Index=0;(Index<GAP_Inquiry_Event_Data->Number_Devices) && (Index<MAX_INQUIRY_RESULTS);Index++)
                  {
                     InquiryResultList[Index] = GAP_Inquiry_Event_Data->GAP_Inquiry_Data[Index].BD_ADDR;
                     BD_ADDRToStr(GAP_Inquiry_Event_Data->GAP_Inquiry_Data[Index].BD_ADDR, Callback_BoardStr);

                     PRINTF("Result: %d,%s.\n", (Index+1), Callback_BoardStr);
                  }

                  NumberofValidResponses = GAP_Inquiry_Event_Data->Number_Devices;
               }
            }
            break;
         case etInquiry_Entry_Result:
            /* Next convert the BD_ADDR to a string.                    */
            BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Inquiry_Entry_Event_Data->BD_ADDR, Callback_BoardStr);

            /* Display this GAP Inquiry Entry Result.                   */
            PRINTF("\n");
            PRINTF("Inquiry Entry: %s.\n", Callback_BoardStr);
            break;
         case etAuthentication:
            /* An authentication event occurred, determine which type of*/
            /* authentication event occurred.                           */
            switch(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->GAP_Authentication_Event_Type)
            {
               case atLinkKeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atLinkKeyRequest: %s\n", Callback_BoardStr);

                  /* Setup the authentication information response      */
                  /* structure.                                         */
                  GAP_Authentication_Information.GAP_Authentication_Type    = atLinkKey;
                  GAP_Authentication_Information.Authentication_Data_Length = 0;

                  /* See if we have stored a Link Key for the specified */
                  /* device.                                            */
                  for(Index=0;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                  {
                     if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                     {
                        /* Link Key information stored, go ahead and    */
                        /* respond with the stored Link Key.            */
                        GAP_Authentication_Information.Authentication_Data_Length   = sizeof(Link_Key_t);
                        GAP_Authentication_Information.Authentication_Data.Link_Key = LinkKeyInfo[Index].LinkKey;

                        break;
                     }
                  }

                  /* Submit the authentication response.                */
                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  /* Check the result of the submitted command.         */
                  if(!Result)
                     DisplayFunctionSuccess("GAP_Authentication_Response");
                  else
                     DisplayFunctionError("GAP_Authentication_Response", Result);
                  break;
               case atPINCodeRequest:
                  /* A pin code request event occurred, first display   */
                  /* the BD_ADD of the remote device requesting the pin.*/
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atPINCodeRequest: %s\n", Callback_BoardStr);

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the PIN Code.                                      */
                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Inform the user that they will need to respond with*/
                  /* a PIN Code Response.                               */
                  PRINTF("Respond with: PINCodeResponse\n");
                  break;
               case atAuthenticationStatus:
                  /* An authentication status event occurred, display   */
                  /* all relevant information.                          */
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atAuthenticationStatus: %d for %s\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Authentication_Status, Callback_BoardStr);

                  /* Flag that there is no longer a current             */
                  /* Authentication procedure in progress.              */
                  ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  break;
               case atLinkKeyCreation:
                  /* A link key creation event occurred, first display  */
                  /* the remote device that caused this event.          */
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atLinkKeyCreation: %s\n", Callback_BoardStr);

                  /* The BD_ADDR of the remote device has been displayed*/
                  /* now display the link key being created.            */
                  DisplayKey("         Link Key: 0x", sizeof(Link_Key_t), (Byte_t *)&(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key));

                  /* Now store the link Key in either a free location OR*/
                  /* over the old key location.                         */
                  ASSIGN_BD_ADDR(NULL_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

                  for(Index=0,Result=-1;Index<(sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t));Index++)
                  {
                     if(COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device))
                        break;
                     else
                     {
                        if((Result == (-1)) && (COMPARE_BD_ADDR(LinkKeyInfo[Index].BD_ADDR, NULL_BD_ADDR)))
                           Result = Index;
                     }
                  }

                  /* If we didn't find a match, see if we found an empty*/
                  /* location.                                          */
                  if(Index == (sizeof(LinkKeyInfo)/sizeof(LinkKeyInfo_t)))
                     Index = Result;

                  /* Check to see if we found a location to store the   */
                  /* Link Key information into.                         */
                  if(Index != (-1))
                  {
                     LinkKeyInfo[Index].BD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;
                     LinkKeyInfo[Index].LinkKey = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Link_Key_Info.Link_Key;

                     PRINTF("Link Key Stored.\n");
                  }
                  else
                     PRINTF("Link Key array full.\n");
                  break;
               case atIOCapabilityRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atIOCapabilityRequest: %s\n", Callback_BoardStr);

                  /* Setup the Authentication Information Response      */
                  /* structure.                                         */
                  GAP_Authentication_Information.GAP_Authentication_Type                                      = atIOCapabilities;
                  GAP_Authentication_Information.Authentication_Data_Length                                   = sizeof(GAP_IO_Capabilities_t);
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.IO_Capability            = (GAP_IO_Capability_t)IOCapability;
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.MITM_Protection_Required = MITMProtection;
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.OOB_Data_Present         = OOBSupport;

                  /* Submit the Authentication Response.                */
                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  /* Check the result of the submitted command.         */
                  /* Check the result of the submitted command.         */
                  if(!Result)
                     DisplayFunctionSuccess("Auth");
                  else
                     DisplayFunctionError("Auth", Result);
                  break;
               case atIOCapabilityResponse:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atIOCapabilityResponse: %s\n", Callback_BoardStr);

                  RemoteIOCapability = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.IO_Capability;
                  MITM               = (Boolean_t)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.MITM_Protection_Required;
                  OOB_Data           = (Boolean_t)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.IO_Capabilities.OOB_Data_Present;

                  PRINTF("Capabilities: %s%s%s\n", IOCapabilitiesStrings[RemoteIOCapability], ((MITM)?", MITM":""), ((OOB_Data)?", OOB Data":""));
                  break;
               case atUserConfirmationRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atUserConfirmationRequest: %s\n", Callback_BoardStr);

                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  if(IOCapability != icDisplayYesNo)
                  {
                     /* Invoke JUST Works Process...                    */
                     GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
                     GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)sizeof(Byte_t);
                     GAP_Authentication_Information.Authentication_Data.Confirmation = TRUE;

                     /* Submit the Authentication Response.             */
                     PRINTF("\nAuto Accepting: %lu\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value);

                     Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                     if(!Result)
                        DisplayFunctionSuccess("GAP_Authentication_Response");
                     else
                        DisplayFunctionError("GAP_Authentication_Response", Result);

                     /* Flag that there is no longer a current          */
                     /* Authentication procedure in progress.           */
                     ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  }
                  else
                  {
                     PRINTF("User Confirmation: %lu\n", (unsigned long)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value);

                     /* Inform the user that they will need to respond  */
                     /* with a PIN Code Response.                       */
                     PRINTF("Respond with: UserConfirmationResponse\n");
                  }
                  break;
               case atPasskeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atPasskeyRequest: %s\n", Callback_BoardStr);

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the Passkey.                                       */
                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Inform the user that they will need to respond with*/
                  /* a Passkey Response.                                */
                  PRINTF("Respond with: PassKeyResponse\n");
                  break;
               case atRemoteOutOfBandDataRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atRemoteOutOfBandDataRequest: %s\n", Callback_BoardStr);

                  /* This application does not support OOB data so      */
                  /* respond with a data length of Zero to force a      */
                  /* negative reply.                                    */
                  GAP_Authentication_Information.GAP_Authentication_Type    = atOutOfBandData;
                  GAP_Authentication_Information.Authentication_Data_Length = 0;

                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  if(!Result)
                     DisplayFunctionSuccess("GAP_Authentication_Response");
                  else
                     DisplayFunctionError("GAP_Authentication_Response", Result);
                  break;
               case atPasskeyNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atPasskeyNotification: %s\n", Callback_BoardStr);

                  PRINTF("Passkey Value: %lu\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value);
                  break;
               case atKeypressNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  PRINTF("\n");
                  PRINTF("atKeypressNotification: %s\n", Callback_BoardStr);

                  PRINTF("Keypress: %d\n", (int)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Keypress_Type);
                  break;
               case atSecureSimplePairingComplete:
                  PRINTF("atSecureSimplePairingComplete %d.\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Secure_Simple_Pairing_Status);
                  break;
               default:
                  PRINTF("Un-handled Auth. Event %d.\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->GAP_Authentication_Event_Type);
                  break;
            }
            break;
         case etRemote_Name_Result:
            /* Bluetooth Stack has responded to a previously issued     */
            /* Remote Name Request that was issued.                     */
            GAP_Remote_Name_Event_Data = GAP_Event_Data->Event_Data.GAP_Remote_Name_Event_Data;
            if(GAP_Remote_Name_Event_Data)
            {
               /* Inform the user of the Result.                        */
               BD_ADDRToStr(GAP_Remote_Name_Event_Data->Remote_Device, Callback_BoardStr);

               PRINTF("\n");
               PRINTF("BD_ADDR: %s.\n", Callback_BoardStr);

               if(GAP_Remote_Name_Event_Data->Remote_Name)
                  PRINTF("Name: %s.\n", GAP_Remote_Name_Event_Data->Remote_Name);
               else
                  PRINTF("Name: NULL.\n");
            }
            break;
         case etEncryption_Change_Result:
            BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Remote_Device, Callback_BoardStr);
            PRINTF("\netEncryption_Change_Result for %s, Status: 0x%02X, Mode: %s.\n", Callback_BoardStr,
                                                                                           GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Encryption_Change_Status,
                                                                                           ((GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Encryption_Mode == emDisabled)?"Disabled": "Enabled"));
            break;
         default:
            /* An unknown/unexpected GAP event was received.            */
            PRINTF("\nUnknown Event: %d.\n", GAP_Event_Data->Event_Data_Type);
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");
      PRINTF("Null Event\n");
   }

   DisplayPrompt();
}

   /* The following function is for the SDP Event Receive Data Callback.*/
   /* This function will be called whenever a Callback has been         */
   /* registered for the specified SDP Action that is associated with   */
   /* the Bluetooth Stack.  This function passes to the caller the SDP  */
   /* Request ID of the SDP Request, the SDP Response Event Data of the */
   /* specified Response Event and the SDP Response Event Callback      */
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the SDP Event Data ONLY */
   /* in the context of this callback.  If the caller requires the Data */
   /* for a longer period of time, then the callback function MUST copy */
   /* the data into another Data Buffer.  This function is guaranteed   */
   /* NOT to be invoked more than once simultaneously for the specified */
   /* installed callback (i.e. this function DOES NOT have be           */
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* other SDP Events will not be processed while this function call is*/
   /* outstanding).                                                     */
   /* * NOTE * This function MUST NOT Block and wait for events that    */
   /*          can only be satisfied by Receiving other SDP Events.  A  */
   /*          Deadlock WILL occur because NO SDP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI SDP_Event_Callback(unsigned int BluetoothStackID, unsigned int SDPRequestID, SDP_Response_Data_t *SDP_Response_Data, unsigned long CallbackParameter)
{
   int Index;

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((SDP_Response_Data != NULL) && (BluetoothStackID))
   {
      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming Event is.                                    */
      switch(SDP_Response_Data->SDP_Response_Data_Type)
      {
         case rdTimeout:
            /* A SDP Timeout was received, display a message indicating */
            /* this.                                                    */
            PRINTF("\n");
            PRINTF("SDP Timeout Received.\n");
            break;
         case rdConnectionError:
            /* A SDP Connection Error was received, display a message   */
            /* indicating this.                                         */
            PRINTF("\n");
            PRINTF("SDP Connection Error Received.\n");
            break;
         case rdErrorResponse:
            /* A SDP error response was received, display all relevant  */
            /* information regarding this event.                        */
            PRINTF("\n");
            PRINTF("SDP Error Response Received - Error Code: %d.\n", SDP_Response_Data->SDP_Response_Data.SDP_Error_Response_Data.Error_Code);
            break;
         case rdServiceSearchResponse:
            /* A SDP Service Search Response was received, display all  */
            /* relevant information regarding this event                */
            PRINTF("\n");
            PRINTF("SDP Service Search Response Received - Record Count: %d\n", SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Total_Service_Record_Count);

            /* First, check to see if any SDP Service Records were      */
            /* found.                                                   */
            if(SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Total_Service_Record_Count)
            {
               PRINTF("Record Handles:\n");

               for(Index = 0; (Word_t)Index < SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Total_Service_Record_Count; Index++)
               {
                  PRINTF("Record %u: 0x%08X\n", (Index + 1), (unsigned int)SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Response_Data.Service_Record_List[Index]);
               }
            }
            else
               PRINTF("No SDP Service Records Found.\n");
            break;
         case rdServiceAttributeResponse:
            /* A SDP Service Attribute Response was received, display   */
            /* all relevant information regarding this event            */
            PRINTF("\n");
            PRINTF("SDP Service Attribute Response Received\n");

            DisplaySDPAttributeResponse(&SDP_Response_Data->SDP_Response_Data.SDP_Service_Attribute_Response_Data, 0);
            break;
         case rdServiceSearchAttributeResponse:
            /* A SDP Service Search Attribute Response was received,    */
            /* display all relevant information regarding this event    */
            PRINTF("\n");
            PRINTF("SDP Service Search Attribute Response Received\n");

            DisplaySDPSearchAttributeResponse(&SDP_Response_Data->SDP_Response_Data.SDP_Service_Search_Attribute_Response_Data);
            break;
         default:
            /* An unknown/unexpected SDP event was received.            */
            PRINTF("\n");
            PRINTF("Unknown SDP Event.\n");
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("\n");
      PRINTF("SDP callback data: Response_Data = NULL.\n");
   }
}

   /* The following function is responsible for processing HCI Mode     */
   /* change events.                                                    */
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter)
{
   char                     *Mode;
   unsigned int              Int;
   HCI_LE_Meta_Event_Data_t *MetaData;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))
   int   SerialPortIndex;
#endif

   /* Make sure that the input parameters that were passed to us are    */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (HCI_Event_Data))
   {
      /* Process the Event Data.                                        */
      switch(HCI_Event_Data->Event_Data_Type)
      {

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))

         case etRole_Change_Event:
            if(HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data)
            {
               /* Find the Serial Port entry for this event.            */
               if((SerialPortIndex = FindSPPPortIndexByAddress(HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->BD_ADDR)) >= 0)
               {
                  if((HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR) && (HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->New_Role == HCI_CURRENT_ROLE_MASTER))
                  {
                     PRINTF("\nSPP Port %u: Role Change Success.\n", SPPContextInfo[SerialPortIndex].LocalSerialPortID);
                  }
                  else
                  {
                     PRINTF("\nSPP Port %u: Role Change Failure (Status 0x%02X, Role 0x%02X).\n", SPPContextInfo[SerialPortIndex].LocalSerialPortID, HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->Status, HCI_Event_Data->Event_Data.HCI_Role_Change_Event_Data->New_Role);
                  }

                  DisplayPrompt();
               }
            }
            break;

#endif

         case etMode_Change_Event:
            if(HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data)
            {
               switch(HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Current_Mode)
               {
                  case HCI_CURRENT_MODE_HOLD_MODE:
                     Mode = "Hold";
                     break;
                  case HCI_CURRENT_MODE_SNIFF_MODE:
                     Mode = "Sniff";
                     break;
                  case HCI_CURRENT_MODE_PARK_MODE:
                     Mode = "Park";
                     break;
                  case HCI_CURRENT_MODE_ACTIVE_MODE:
                  default:
                     Mode = "Active";
                     break;
               }

               PRINTF("\n");
               PRINTF("HCI Mode Change Event, Status: 0x%02X, Connection Handle: %d, Mode: %s, Interval: %d\n", HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Status,
                                                                                                                HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Connection_Handle,
                                                                                                                Mode,
                                                                                                                HCI_Event_Data->Event_Data.HCI_Mode_Change_Event_Data->Interval);
               DisplayPrompt();
            }
            break;
         case etLE_Meta_Event:
            MetaData = HCI_Event_Data->Event_Data.HCI_LE_Meta_Event_Data;
            if(MetaData->LE_Event_Data_Type == meConnection_Complete_Event)
            {
               Int = (MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Conn_Interval*125);
               PRINTF("LE Connection Complete\n");
               PRINTF("   ConnInterval: %d.%d ms\n", (Int/100), ((Int % 100)/10));
               PRINTF("   ConnLatency:  %d\n", MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Conn_Latency);
               PRINTF("   SuperTimeout: %d\n", MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Supervision_Timeout);
               PRINTF("   Role:         %s\n", (char *)((MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Role)?"Slave":"Master"));
               PRINTF("   Status:       %d\n", MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Status);
            }
            if(MetaData->LE_Event_Data_Type == meConnection_Update_Complete_Event)
            {
               Int = (MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Conn_Interval*125);
               PRINTF("LE Connection Update\n");
               PRINTF("   ConnInterval: %d.%d ms\n", (Int/100), ((Int % 100)/10));
               PRINTF("   ConnLatency:  %d\n", MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Conn_Latency);
               PRINTF("   SuperTimeout: %d\n", MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Supervision_Timeout);
               PRINTF("   Status:       %d\n", MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Status);
            }
            break;
         default:
            break;
      }
   }
}

   /* The following function is for an SPP Event Callback.  This        */
   /* function will be called whenever a SPP Event occurs that is       */
   /* associated with the Bluetooth Stack.  This function passes to the */
   /* caller the SPP Event Data that occurred and the SPP Event Callback*/
   /* Parameter that was specified when this Callback was installed.    */
   /* The caller is free to use the contents of the SPP SPP Event Data  */
   /* ONLY in the context of this callback.  If the caller requires the */
   /* Data for a longer period of time, then the callback function MUST */
   /* copy the data into another Data Buffer.  This function is         */
   /* guaranteed NOT to be invoked more than once simultaneously for the*/
   /* specified installed callback (i.e.  this function DOES NOT have be*/
   /* reentrant).  It Needs to be noted however, that if the same       */
   /* Callback is installed more than once, then the callbacks will be  */
   /* called serially.  Because of this, the processing in this function*/
   /* should be as efficient as possible.  It should also be noted that */
   /* this function is called in the Thread Context of a Thread that the*/
   /* User does NOT own.  Therefore, processing in this function should */
   /* be as efficient as possible (this argument holds anyway because   */
   /* another SPP Event will not be processed while this function call  */
   /* is outstanding).                                                  */
   /* * NOTE * This function MUST NOT Block and wait for Events that    */
   /*          can only be satisfied by Receiving SPP Event Packets.  A */
   /*          Deadlock WILL occur because NO SPP Event Callbacks will  */
   /*          be issued while this function is currently outstanding.  */
static void BTPSAPI SPP_Event_Callback(unsigned int BluetoothStackID, SPP_Event_Data_t *SPP_Event_Data, unsigned long CallbackParameter)
{
   int          SerialPortIndex;
   int          ret_val = 0;
   int          TempLength;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))
   Byte_t       StatusResult;
   Word_t       Connection_HandleResult;
   Byte_t       Current_Role;
#endif

   Word_t       ConnectionHandle;
   Boolean_t    _DisplayPrompt = TRUE;
   Boolean_t    Done;
   BoardStr_t   Callback_BoardStr;
   unsigned int LocalSerialPortID;

   /* **** SEE SPPAPI.H for a list of all possible event types.  This   */
   /* program only services its required events.                   **** */

   /* First, check to see if the required parameters appear to be       */
   /* semi-valid.                                                       */
   if((SPP_Event_Data) && (BluetoothStackID))
   {
      /* The parameters appear to be semi-valid, now check to see what  */
      /* type the incoming event is.                                    */
      switch(SPP_Event_Data->Event_Data_Type)
      {
         case etPort_Open_Indication:
            /* A remote port is requesting a connection.                */
            BD_ADDRToStr(SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->BD_ADDR, Callback_BoardStr);

            PRINTF("\n");
            PRINTF("SPP Open Indication, ID: 0x%04X, Board: %s.\n", SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->SerialPortID, Callback_BoardStr);

            /* Find the index of the SPP Port Information.              */
            if((SerialPortIndex = FindSPPPortIndex(SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->SerialPortID)) >= 0)
            {
               /* Flag that we are connected to the device.             */
               SPPContextInfo[SerialPortIndex].Connected = TRUE;
               SPPContextInfo[SerialPortIndex].BD_ADDR   = SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->BD_ADDR;

               /* Query the connection handle.                          */
               ret_val = GAP_Query_Connection_Handle(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, &ConnectionHandle);
               if(ret_val)
               {
                  /* Failed to Query the Connection Handle.             */
                  DisplayFunctionError("GAP_Query_Connection_Handle()",ret_val);
               }
               else
               {
                  /* Save the connection handle of this connection.     */
                  SPPContextInfo[SerialPortIndex].Connection_Handle = ConnectionHandle;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))

                  /* First determine the current role to determine if we*/
                  /* are already the master.                            */
                  StatusResult = 0;
                  ret_val = HCI_Role_Discovery(BluetoothStackID, SPPContextInfo[SerialPortIndex].Connection_Handle, &StatusResult, &Connection_HandleResult, &Current_Role);
                  if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                  {
                     /* Check to see if we aren't currently the master. */
                     if(Current_Role != HCI_CURRENT_ROLE_MASTER)
                     {
                        /* Attempt to switch to the master role.        */
                        StatusResult = 0;
                        ret_val = HCI_Switch_Role(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, HCI_ROLE_SWITCH_BECOME_MASTER, &StatusResult);
                        if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                        {
                            PRINTF("\nInitiating Role Switch.\n");
                        }
                        else
                        {
                            PRINTF("HCI Switch Role failed. %d: 0x%02X", ret_val, StatusResult);
                        }
                     }
                  }
                  else
                  {
                      PRINTF("HCI Role Discovery failed. %d: 0x%02X", ret_val, StatusResult);
                  }

#endif

               }
            }
            break;
         case etPort_Open_Confirmation:
            /* A Client Port was opened.  The Status indicates the      */
            /* Status of the Open.                                      */
            PRINTF("\n");
            PRINTF("SPP Open Confirmation, ID: 0x%04X, Status 0x%04X.\n", SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->SerialPortID,
                                                                            SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->PortOpenStatus);


            /* Find the index of the SPP Port Information.              */
            if((SerialPortIndex = FindSPPPortIndex(SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->SerialPortID)) >= 0)
            {
               /* Check the Status to make sure that an error did not   */
               /* occur.                                                */
               if(SPP_Event_Data->Event_Data.SPP_Open_Port_Confirmation_Data->PortOpenStatus)
               {
                  /* An error occurred while opening the Serial Port so */
                  /* invalidate the Serial Port ID.                     */
                  BTPS_MemInitialize(&SPPContextInfo[SerialPortIndex], 0, sizeof(SPPContextInfo[SerialPortIndex]));
               }
               else
               {
                  /* Flag that we are connected to the device.          */
                  SPPContextInfo[SerialPortIndex].Connected = TRUE;

                  /* Query the connection Handle.                       */
                  ret_val = GAP_Query_Connection_Handle(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, &ConnectionHandle);
                  if(ret_val)
                  {
                     /* Failed to Query the Connection Handle.          */
                     DisplayFunctionError("GAP_Query_Connection_Handle()",ret_val);
                  }
                  else
                  {
                     /* Save the connection handle of this connection.  */
                     SPPContextInfo[SerialPortIndex].Connection_Handle = ConnectionHandle;

#if ((SPP_PERFORM_MASTER_ROLE_SWITCH) && (MAX_SIMULTANEOUS_SPP_PORTS > 1))

                     /* First determine the current role to determine if*/
                     /* we are already the master.                      */
                     StatusResult = 0;
                     ret_val = HCI_Role_Discovery(BluetoothStackID, SPPContextInfo[SerialPortIndex].Connection_Handle, &StatusResult, &Connection_HandleResult, &Current_Role);
                     if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                     {
                        /* Check to see if we aren't currently the      */
                        /* master.                                      */
                        if(Current_Role != HCI_CURRENT_ROLE_MASTER)
                        {
                           /* Attempt to switch to the master role.     */
                           StatusResult = 0;
                           ret_val = HCI_Switch_Role(BluetoothStackID, SPPContextInfo[SerialPortIndex].BD_ADDR, HCI_ROLE_SWITCH_BECOME_MASTER, &StatusResult);
                           if((ret_val == 0) && (StatusResult == HCI_ERROR_CODE_NO_ERROR))
                           {
                               PRINTF("\nInitiating Role Switch.\n");
                           }
                           else
                           {
                               PRINTF("HCI Switch Role failed. %d: 0x%02X", ret_val, StatusResult);
                           }
                        }
                     }
                     else
                     {
                         PRINTF("HCI Role Discovery failed. %d: 0x%02X", ret_val, StatusResult);
                     }

#endif

                  }
               }
            }
            break;
         case etPort_Close_Port_Indication:
            /* The Remote Port was Disconnected.                        */
            LocalSerialPortID = SPP_Event_Data->Event_Data.SPP_Close_Port_Indication_Data->SerialPortID;

            PRINTF("\n");
            PRINTF("SPP Close Port, ID: 0x%04X\n", LocalSerialPortID);

            /* Find the port index of the SPP Port that just closed.    */
            if((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0)
            {
               ASSIGN_BD_ADDR(SPPContextInfo[SerialPortIndex].BD_ADDR, 0, 0, 0, 0, 0, 0);
               SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend = 0;
               SPPContextInfo[SerialPortIndex].Connected            = FALSE;

               /* If this is a client port we also need to clear the    */
               /* Serial Port ID since it is no longer in use.          */
               if(SPPContextInfo[SerialPortIndex].ServerPortNumber == 0)
                  SPPContextInfo[SerialPortIndex].LocalSerialPortID = 0;
            }
            break;
         case etPort_Status_Indication:
            /* Display Information about the new Port Status.           */
            PRINTF("\n");
            PRINTF("SPP Port Status Indication: 0x%04X, Status: 0x%04X, Break Status: 0x%04X, Length: 0x%04X.\n", SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->SerialPortID,
                                                                                                                    SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->PortStatus,
                                                                                                                    SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->BreakStatus,
                                                                                                                    SPP_Event_Data->Event_Data.SPP_Port_Status_Indication_Data->BreakTimeout);

            break;
         case etPort_Data_Indication:
            /* Data was received.                                       */
            LocalSerialPortID = SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->SerialPortID;

            /* Find the port index of the correct SPP Port.             */
            if((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0)
            {
               /* If we are operating in Raw Data Display Mode then     */
               /* simply display the data that was give to use.         */
               if((DisplayRawData) || (AutomaticReadActive))
               {
                  /* Initialize Done to false.                          */
                  Done = FALSE;

                  /* Loop through and read all data that is present in  */
                  /* the buffer.                                        */
                  while(!Done)
                  {

                     /* Read as much data as possible.                  */
                     if((TempLength = SPP_Data_Read(BluetoothStackID, LocalSerialPortID, (Word_t)sizeof(SPPContextInfo[SerialPortIndex].Buffer)-1, (Byte_t *)SPPContextInfo[SerialPortIndex].Buffer)) > 0)
                     {
                        /* Now simply display each character that we    */
                        /* have just read.                              */
                        if(DisplayRawData)
                        {
                           SPPContextInfo[SerialPortIndex].Buffer[TempLength] = '\0';

                           PRINTF("%s", (char *)SPPContextInfo[SerialPortIndex].Buffer);
                        }
                     }
                     else
                     {
                        /* Either an error occurred or there is no more */
                        /* data to be read.                             */
                        if(TempLength < 0)
                        {
                           /* Error occurred.                           */
                           PRINTF("SPP_Data_Read(): Error %d.\n", TempLength);
                        }

                        /* Regardless if an error occurred, we are      */
                        /* finished with the current loop.              */
                        Done = TRUE;
                     }
                  }

                  _DisplayPrompt = FALSE;
               }
               else
               {
                  /* Simply inform the user that data has arrived.      */
                  PRINTF("\n");
                  PRINTF("SPP Data Indication, ID: 0x%04X, Length: 0x%04X.\n", SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->SerialPortID,
                                                                                 SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->DataLength);
               }
            }
            break;
         case etPort_Send_Port_Information_Indication:
            /* Simply Respond with the information that was sent to us. */
            ret_val = SPP_Respond_Port_Information(BluetoothStackID, SPP_Event_Data->Event_Data.SPP_Send_Port_Information_Indication_Data->SerialPortID, &SPP_Event_Data->Event_Data.SPP_Send_Port_Information_Indication_Data->SPPPortInformation);
            break;
         case etPort_Transmit_Buffer_Empty_Indication:
            /* Locate the serial port for the SPP Port that now has a   */
            /* transmit buffer empty.                                   */
            LocalSerialPortID = SPP_Event_Data->Event_Data.SPP_Transmit_Buffer_Empty_Indication_Data->SerialPortID;

            /* Attempt to find the index of the SPP Port entry.         */
            if((SerialPortIndex = FindSPPPortIndex(LocalSerialPortID)) >= 0)
            {
               /* Flag that this buffer is no longer full.              */
               SPPContextInfo[SerialPortIndex].SendInfo.BufferFull = FALSE;

               /* The transmit buffer is now empty after being full.    */
               /* Next check the current application state.             */
               if(SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend)
               {
                  /* Send the remainder of the last attempt.            */
                  TempLength                    = (DATA_STR_LENGTH-SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend);

                  SPPContextInfo[SerialPortIndex].SendInfo.BytesSent    = (DWord_t)SPP_Data_Write(BluetoothStackID, LocalSerialPortID, (Word_t)TempLength, (Byte_t *)&(DataStr[SPPContextInfo[SerialPortIndex].SendInfo.BytesSent]));
                  if((int)(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent) >= 0)
                  {
                     if(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent <= SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend)
                        SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend -= SPPContextInfo[SerialPortIndex].SendInfo.BytesSent;
                     else
                        SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend  = 0;

                     while(SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend)
                     {
                        /* Set the Number of bytes to send in the next  */
                        /* packet.                                      */
                        if(SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend > DATA_STR_LENGTH)
                           TempLength = DATA_STR_LENGTH;
                        else
                           TempLength = SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend;

                        SPPContextInfo[SerialPortIndex].SendInfo.BytesSent = SPP_Data_Write(BluetoothStackID, LocalSerialPortID, (Word_t)TempLength, (Byte_t *)DataStr);
                        if((int)(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent) >= 0)
                        {
                           SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend -= SPPContextInfo[SerialPortIndex].SendInfo.BytesSent;
                           if(SPPContextInfo[SerialPortIndex].SendInfo.BytesSent < (DWord_t)TempLength)
                              break;
                        }
                        else
                        {
                           PRINTF("SPP_Data_Write returned %d.\n", (int)SPPContextInfo[SerialPortIndex].SendInfo.BytesSent);

                           SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend = 0;
                        }
                     }
                  }
                  else
                  {
                     PRINTF("SPP_Data_Write returned %d.\n", (int)SPPContextInfo[SerialPortIndex].SendInfo.BytesSent);

                     SPPContextInfo[SerialPortIndex].SendInfo.BytesToSend = 0;
                  }
               }
               else
               {
                  /* Only print the event indication to the user if we  */
                  /* are NOT operating in Raw Data Display Mode.        */
                  if(!DisplayRawData)
                  {
                     PRINTF("\nTransmit Buffer Empty Indication, ID: 0x%04X\n", SPP_Event_Data->Event_Data.SPP_Transmit_Buffer_Empty_Indication_Data->SerialPortID);
                  }
               }
            }
            else
            {
               PRINTF("Could not find SPP server Context after Buffer Empty Indication.\n");
            }

            _DisplayPrompt = FALSE;
            break;
         default:
            /* An unknown/unexpected SPP event was received.            */
            PRINTF("\n");
            PRINTF("Unknown Event.\n");
            break;
      }

      /* Check the return value of any function that might have been    */
      /* executed in the callback.                                      */
      if(ret_val)
      {
         /* An error occurred, so output an error message.              */
         PRINTF("\n");
         PRINTF("Error %d.\n", ret_val);
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      PRINTF("Null Event\n");
   }

   if(_DisplayPrompt)
      DisplayPrompt();
}

   /* ***************************************************************** */
   /*                    End of Event Callbacks.                        */
   /* ***************************************************************** */

extern void user_pre_init(void);

static void RubySPP_entry(ULONG which_thread)
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
                  /* Attempt to register a HCI Event Callback.          */
                  Result = HCI_Register_Event_Callback(BluetoothStackID, HCI_Event_Callback, (unsigned long)NULL);
                  if(Result > 0)
                  {
                     /* Restart the User Interface Selection.           */
                     UI_Mode = UI_MODE_SELECT;

                     /* Set up the Selection Interface.                 */
                     UserInterface();

                     /* Display the first command prompt.               */
                     DisplayPrompt();
                  }
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
   tx_thread_create(&host_thread, "ruby spp apple demo thread", RubySPP_entry, 0, Ptr, PSEUDO_HOST_STACK_SIZE, 16, 16, 4, TX_AUTO_START);

   cdr_threadx_thread_init();
}
