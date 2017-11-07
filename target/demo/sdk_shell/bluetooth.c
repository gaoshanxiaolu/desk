/*****< bluetooth_handler.c >**************************************************/
/*      Copyright 2012 - 2015 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BLUETOOTH_HANDLER - Ruby Bluetooth SPP and SPP Emulation using GATT (LE)  */
/*                      application.                                          */
/*                                                                            */
/*  Author:  Tim Thomas                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   12/01/15  Tim Thomas     Initial creation.                               */
/******************************************************************************/

#ifdef ENABLE_BLUETOOTH

#include "qcom_common.h"
#include "qcom_uart.h"
#include "swat_parse.h"
#include "qcom_pwr.h"

#include "BTPSVEND.h"
#include "SS1BTPS.h"           /* Main SS1 BT Stack Header.                   */
#include "SS1BTGAT.h"          /* Main SS1 GATT Header.                       */
#include "SS1BTGAP.h"          /* Main SS1 GAP Service Header.                */
#include "SS1BTDIS.h"          /* Main SS1 DIS Service Header.                */
#include "SPPLETyp.h"          /* GATT based SPP-like Types File.             */

   /* MACRO used to eliminate unreferenced variable warnings.           */
#define UNREFERENCED_PARAM(_p)                      ((void)(_p))

#define PRINTF                                      qcom_printf
#define DBG_PRINT                                   if(dbg_printf) qcom_printf

#define BT_UART_NAME                                "UART2"

#define DEFAULT_SPP_SERVER_PORT                     (1)  /* Default SPP Server*/
                                                         /* Port nnumber      */

#define MAX_SUPPORTED_LINK_KEYS                     (3)  /* Max supported Link*/
                                                         /* keys.             */

#define MAX_SUPPORTED_LE_DEVICES                    (3)  /* Max supported LE  */
                                                         /* devices to track  */

#define MAX_LE_CONNECTIONS                          (1)  /* Denotes the max   */
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

#define MAXIMUM_SPP_BUFFER_SIZE                  (1024) /* Maximum size of the*/
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

#define DEFAULT_IO_CAPABILITY       (icNoInputNoOutput)  /* Denotes the       */
                                                         /* default I/O       */
                                                         /* Capability that is*/
                                                         /* used with Secure  */
                                                         /* Simple Pairing.   */

#define DEFAULT_MITM_PROTECTION                 (FALSE)  /* Denotes the       */
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

#define EXIT_TEST_MODE                             (-10) /* Flags exit from   */
                                                         /* Test Mode.        */

#define EXIT_MODE                                  (-11) /* Flags exit from   */
                                                         /* any Mode.         */

#define INDENT_LENGTH                                 3  /* Denotes the number*/
                                                         /* of character      */
                                                         /* spaces to be used */
                                                         /* for indenting when*/
                                                         /* displaying SDP    */
                                                         /* Data Elements.    */

   /* The following define Flags that can be passed to InitBluetooth    */
   /* function.                                                         */
#define APPLICATION_FLAG_ENABLE_SPP                    0x01
#define APPLICATION_FLAG_ENABLE_SPPLE                  0x02
#define APPLICATION_FLAG_SIMULTANEOUS_LE_CLASSIC       0x04
#define APPLICATION_FLAG_ENABLE_DEBUG_PRINTS           0x08
#define APPLICATION_FLAG_ENABLE_LE_CENTRAL             0x10
#define APPLICATION_FLAG_TEST_MODE_VALUE               0x20
#define APPLICATION_FLAG_SHUTDOWN_IN_PROGRESS          0x40
#define APPLICATION_FLAG_ADVERTISING_ENABLED           0x80

   /* The following MACRO is used to convert an ASCII character into the*/
   /* equivalent decimal value.  The MACRO converts lower case          */
   /* characters to upper case before the conversion.                   */
#define ToInt(_x)                                  (((_x) > 0x39)?(((_x) & ~0x20)-0x37):((_x)-0x30))

   /* Determine the Name we will use for this compilation.              */
#define LE_DEMO_DEVICE_NAME                        "bluetooth"

   /* Following converts a Sniff Parameter in Milliseconds to frames.   */
#define MILLISECONDS_TO_BASEBAND_SLOTS(_x)         ((_x) / (0.625))

   /* The following macro calculates the best fit MTU size from the MTU */
   /* that is available.  This takes into account the 4 byte L2CAP      */
   /* Header and the 3 Byte ATT header and have them fit into a integral*/
   /* number of full packets.                                           */
#define BEST_FIT_NOTIFICATION(_x)                  ((((_x)/27)*27)-7)

   /* The following type definition represents the container type which */
   /* holds the mapping between Bluetooth devices (based on the BD_ADDR)*/
   /* and the Link Key (BD_ADDR <-> Link Key Mapping).                  */
typedef struct _tagLinkKeyInfo_t
{
   BD_ADDR_t  BD_ADDR;
   Link_Key_t LinkKey;
} LinkKeyInfo_t;

   /* Structure used to hold all of the GAP LE Parameters.              */
typedef struct _tagGAPLE_Parameters_t
{
   GAP_LE_Connectability_Mode_t ConnectableMode;
   GAP_Discoverability_Mode_t   DiscoverabilityMode;
   GAP_LE_IO_Capability_t       IOCapability;
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

   /* The following defines the structure that is used to hold          */
   /* information about all open SPP Ports.                             */
typedef struct SPP_Context_Info_t
{
   unsigned int  SerialPortID;
   Word_t        Connection_Handle;
   BD_ADDR_t     BD_ADDR;
   DWord_t       SPPServerSDPHandle;
   Boolean_t     Connected;
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
   Byte_t        Buffer[SPPLE_DATA_BUFFER_LENGTH];
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
#define DEVICE_INFO_FLAG_SPPLE_SERVER                      0x0002
#define DEVICE_INFO_FLAG_LINK_ENCRYPTED                    0x0004

   /* The following structure is used to hold a list of information     */
   /* on all paired devices.                                            */
typedef struct _tagDeviceInfo_t
{
   Word_t                Flags;
   GAP_LE_Address_Type_t AddressType;
   BD_ADDR_t             BD_ADDR;
   Long_Term_Key_t       LTK;
   Random_Number_t       Rand;
   Word_t                EDIV;
   Byte_t                EncryptionKeySize;
   SPPLE_Server_Info_t   ServerInfo;
} DeviceInfo_t;

#define DEVICE_INFO_DATA_SIZE                               (sizeof(DeviceInfo_t))

   /* The following structure is used to hold information on a connected*/
   /* LE Device.                                                        */
typedef struct _tagLE_Context_Info_t
{
   Word_t        MTU;
   BD_ADDR_t     ConnectedBD_ADDR;
   unsigned int  ConnectionID;
   Boolean_t     BufferFull;
   Word_t        BytesToSend;
   Word_t        InIndex;
   Word_t        OutIndex;
   unsigned char RxBuffer[SPPLE_DATA_BUFFER_LENGTH];
   Word_t        CreditsToSend;
   Word_t        TransmitCredits;
}  LE_Context_Info_t;

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

static DeviceInfo_t        RemoteDeviceInfo;        /* Holds the list head for the     */
                                                    /* device info list.               */

static unsigned int        BluetoothStackID;        /* Variable which holds the Handle */
                                                    /* of the opened Bluetooth Protocol*/
                                                    /* Stack.                          */

static BD_ADDR_t           CurrentLERemoteBD_ADDR;  /* Variable which holds the        */
                                                    /* current LE BD_ADDR of the device*/
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static BD_ADDR_t           CurrentCBRemoteBD_ADDR;  /* Variable which holds the        */
                                                    /* current CB BD_ADDR of the device*/
                                                    /* which is currently pairing or   */
                                                    /* authenticating.                 */

static LinkKeyInfo_t       LinkKeyInfo[MAX_SUPPORTED_LINK_KEYS]; /* Variable holds     */
                                                    /* BD_ADDR <-> Link Keys for       */
                                                    /* pairing.                        */

static GAP_IO_Capability_t IOCapability;            /* Variable which holds the        */
                                                    /* current I/O Capabilities that   */
                                                    /* are to be used for Secure Simple*/
                                                    /* Pairing.                        */

static SPP_Context_Info_t  SPPContextInfo;          /* Variable that contains          */
                                                    /* information about the current   */
                                                    /* open SPP Ports                  */

static LE_Context_Info_t   LEContextInfo;           /* Structure that contains the     */
                                                    /* connection ID and BD_ADDR of    */
                                                    /* each connected device.          */

static Byte_t              ApplicationFlags;
static Mutex_t             DeviceListMutex;
static DeviceInfo_t        DeviceInfoList[MAX_SUPPORTED_LE_DEVICES];
static Connect_Params_t    ConnectParams;
static int                 SendBufferSize;
static int                 SendIndex;
static Byte_t             *SendBuffer;
static Word_t              CoExMode;
static Word_t              LAPU;
static Word_t              LAPL;
static Boolean_t           dbg_printf;
static BD_ADDR_t           TargetBD_ADDR;
static GAP_LE_Scan_Type_t  ScanType;
static Event_t             ShutdownEvent;
static int                 CoexPrio;
   /* The following constants represent the default log file names that */
   /* are used if no Log file name is specified when enabling debug.    */
#define DEFAULT_DEBUG_LOG_FILE_NAME  "bluetooth_ASC.log"
#define DEFAULT_DEBUG_FTS_FILE_NAME  "bluetooth_FTS.log"

   /* The following string table is used to map HCI Version information */
   /* to an easily displayable version string.                          */
static char *HCIVersionStrings[] =
{
   "1.0b",
   "1.1",
   "1.2",
   "2.0",
   "2.1",
   "3.0",
   "4.0",
   "Unknown (greater 4.0)"
} ;

#define NUM_SUPPORTED_HCI_VERSIONS              (sizeof(HCIVersionStrings)/sizeof(char *) - 1)

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

   /* Internal function prototypes.                                     */
static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(BD_ADDR_t BD_ADDR);
static DeviceInfo_t *AddDeviceInfoEntryByBD_ADDR(BD_ADDR_t BD_ADDR);

static long long str2int(char *str);
static void BD_ADDRToStr(BD_ADDR_t Board_Address, BoardStr_t BoardStr);

static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation);
static int CloseStack(void);

static void WriteEIRInformation(char *LocalName);

static int RegisterSPP(void);

static int DeleteLinkKey(BD_ADDR_t BD_ADDR);

static void SPPLESendCredits(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int GrantedCredits);
static void SPPLEReceiveCreditEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int Credits);
static unsigned int SPPLESendData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo);
static void SPPLEDataIndicationEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data);

static void ConfigureCapabilities(GAP_LE_Pairing_Capabilities_t *Capabilities);
static int SendPairingRequest(BD_ADDR_t BD_ADDR, Boolean_t ConnectionMaster);
static int SlavePairingRequestResponse(BD_ADDR_t BD_ADDR);
static int EncryptionInformationRequestResponse(BD_ADDR_t BD_ADDR, Byte_t KeySize, GAP_LE_Authentication_Response_Information_t *GAP_LE_Authentication_Response_Information);

static int AdvertiseLE(int Mode);
static int RegisterSPPLE(void);
static int UnregisterSPPLE(void);
static int AdvertiseLE(int Mode);
static int ScanLE(unsigned int State);
static int ConnectLEDevice(unsigned int BluetoothStackID, GAP_LE_Address_Type_t Address_Type, BD_ADDR_t BD_ADDR, Boolean_t UseWhiteList);

   /* BTPS Callback function prototypes.                                */
static void BTPSAPI SPPLE_AsynchronousCallback(unsigned int BluetoothStackID, unsigned long CallbackParameter);
static void BTPSAPI GAP_LE_Event_Callback(unsigned int BluetoothStackID,GAP_LE_Event_Data_t *GAP_LE_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_SPPLE_ServerEventCallback(unsigned int BluetoothStackID, GATT_Server_Event_Data_t *GATT_ServerEventData, unsigned long CallbackParameter);
static void BTPSAPI GATT_ClientEventCallback(unsigned int BluetoothStackID, GATT_Client_Event_Data_t *GATT_Client_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GATT_Connection_Event_Callback(unsigned int BluetoothStackID, GATT_Connection_Event_Data_t *GATT_Connection_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI GAP_Event_Callback(unsigned int BluetoothStackID, GAP_Event_Data_t *GAP_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter);
static void BTPSAPI SPP_Event_Callback(unsigned int BluetoothStackID, SPP_Event_Data_t *SPP_Event_Data, unsigned long CallbackParameter);

/* Throughput related.                                */
static void SPPLEDataMetaRecord(int dataLength);
static void SPPLEDataMetaRecordPrint();

/* Coex related.                                      */
void SetCSR8x11Pskey(unsigned int BluetoothStackID, Word_t pskey, Word_t* buffer, Byte_t len);
void GetCSR8x11Pskey(unsigned int BluetoothStackID, Word_t pskey);
void CSR8x11PatchCallback(unsigned int BluetoothStackID, unsigned int ControllerID, unsigned long CallbackParameter);

static DeviceInfo_t *SearchDeviceInfoEntryByBD_ADDR(BD_ADDR_t BD_ADDR)
{
   int           i;
   BD_ADDR_t     NullBD_ADDR;
   DeviceInfo_t *ret_val = NULL;

   ASSIGN_BD_ADDR(NullBD_ADDR, 0, 0, 0, 0, 0, 0);
   for(i=0; i < MAX_SUPPORTED_LE_DEVICES; i++)
   {
      if(COMPARE_BD_ADDR(NullBD_ADDR, DeviceInfoList[i].BD_ADDR))
         break;

      if(COMPARE_BD_ADDR(BD_ADDR, DeviceInfoList[i].BD_ADDR))
      {
         ret_val = &DeviceInfoList[i];
         break;
      }
   }

   return(ret_val);
}

static DeviceInfo_t *AddDeviceInfoEntryByBD_ADDR(BD_ADDR_t BD_ADDR)
{
   int           i;
   BD_ADDR_t     NullBD_ADDR;
   DeviceInfo_t *ret_val;

   ASSIGN_BD_ADDR(NullBD_ADDR, 0, 0, 0, 0, 0, 0);
   for(i=0; i < MAX_SUPPORTED_LE_DEVICES; i++)
   {
      if(COMPARE_BD_ADDR(NullBD_ADDR, DeviceInfoList[i].BD_ADDR))
         break;
   }
   if(i == MAX_SUPPORTED_LE_DEVICES)
   {
      for(i=0; i < (MAX_SUPPORTED_LE_DEVICES-1); i++)
         DeviceInfoList[i] = DeviceInfoList[i+1];
   }

   BTPS_MemInitialize(&DeviceInfoList[i], 0, DEVICE_INFO_DATA_SIZE);
   DeviceInfoList[i].BD_ADDR = BD_ADDR;

   ret_val = &DeviceInfoList[i];

   return(ret_val);
}

static long long str2int(char *str)
{
   int       radix   = 10;
   long long ret_val = -1;

   if(str)
   {
      if(strlen(str) >= 2)
      {
         if((str[0] | 0x20) == 'x')
         {
            radix = 16;
            str++;
         }
         if((str[0] == '0') || ((str[1] | 0x20) == 'x'))
         {
            radix  = 16;
            str   += 2;
         }
      }

      ret_val = 0;
      while(*str)
      {
         ret_val = (ret_val * radix) + ToInt(*str);
         str++;
      }
   }

   return(ret_val);
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

   /* The following function is responsible for opening the SS1         */
   /* Bluetooth Protocol Stack.  This function accepts a pre-populated  */
   /* HCI Driver Information structure that contains the HCI Driver     */
   /* Transport Information.  This function returns zero on successful  */
   /* execution and a negative value on all errors.                     */
static int OpenStack(HCI_DriverInformation_t *HCI_DriverInformation)
{
   int                        ret_val = 0;
   Byte_t                     Status;
   Byte_t                     NumberLEPackets;
   Word_t                     LEPacketLength;
   char                       BluetoothAddress[16];
   BD_ADDR_t                  BD_ADDR;
   unsigned int               ServiceID;
   HCI_Version_t              HCIVersion;
   L2CA_Link_Connect_Params_t L2CA_Link_Connect_Params;

   /* First check to see if the Stack has already been opened.          */
   if(!BluetoothStackID)
   {
      /* Next, makes sure that the Driver Information passed appears to */
      /* be semi-valid.                                                 */
      if(HCI_DriverInformation)
      {
         PRINTF("\nOpenStack().\n");

         HCI_Register_Patch_Callback(CSR8x11PatchCallback, 0);  

         /* Initialize the Stack                                        */
         ret_val = BSC_Initialize(HCI_DriverInformation, 0);

         /* Next, check the return value of the initialization to see   */
         /* if it was successful.                                       */
         if(ret_val > 0)
         {
            /* The Stack was initialized successfully, inform the user  */
            /* and set the return value of the initialization function  */
            /* to the Bluetooth Stack ID.                               */
            BluetoothStackID = ret_val;
            PRINTF("Bluetooth Stack ID: %d.\n", BluetoothStackID);

            /* Initialize the Default Pairing Parameters.               */
            LE_Parameters.IOCapability   = DEFAULT_LE_IO_CAPABILITY;

            /* Initialize the default Secure Simple Pairing parameters. */
            IOCapability                 = DEFAULT_IO_CAPABILITY;

            if(!HCI_Version_Supported(BluetoothStackID, &HCIVersion))
               PRINTF("Device Chipset: %s.\n", ((int)HCIVersion <= NUM_SUPPORTED_HCI_VERSIONS)?HCIVersionStrings[HCIVersion]:HCIVersionStrings[NUM_SUPPORTED_HCI_VERSIONS]);

            /* Let's output the Bluetooth Device Address so that the    */
            /* user knows what the Device Address is.                   */
            if(!GAP_Query_Local_BD_ADDR(BluetoothStackID, &BD_ADDR))
            {
               BD_ADDRToStr(BD_ADDR, BluetoothAddress);

               PRINTF("BD_ADDR: %s\n", BluetoothAddress);
            }

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

            LEContextInfo.ConnectionID = 0;
            ASSIGN_BD_ADDR(LEContextInfo.ConnectedBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);

            /* Set the default connection parameters.                   */
            ConnectParams.ConnectIntMin    = 50;
            ConnectParams.ConnectIntMax    = 100;
            ConnectParams.SlaveLatency     = 0;
            ConnectParams.MaxConnectLength = 10000;
            ConnectParams.MinConnectLength = 0;
            ConnectParams.SupervisionTO    = 20000;

            /* Regenerate IRK and DHK from the constant Identity Root   */
            /* Key.                                                     */
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 1,0, &IRK);
            GAP_LE_Diversify_Function(BluetoothStackID, (Encryption_Key_t *)(&IR), 3, 0, &DHK);

            /* Flag that we have no Key Information in the Key List.    */
            DeviceListMutex = BTPS_CreateMutex(FALSE);

            GAP_Set_Local_Device_Name(BluetoothStackID, LE_DEMO_DEVICE_NAME);
            WriteEIRInformation(LE_DEMO_DEVICE_NAME);

            /* Initialize the GATT Service.                             */
            if((ret_val = GATT_Initialize(BluetoothStackID, GATT_INITIALIZATION_FLAGS_SUPPORT_LE, GATT_Connection_Event_Callback, 0)) == 0)
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
               ret_val = GAPS_Initialize_Service(BluetoothStackID, &ServiceID);
               if(ret_val > 0)
               {
                  /* Save the Instance ID of the GAP Service.           */
                  GAPSInstanceID = (unsigned int)ret_val;

                  /* Set the GAP Device Name and Device Appearance.     */
                  GAPS_Set_Device_Name(BluetoothStackID, GAPSInstanceID, LE_DEMO_DEVICE_NAME);
                  GAPS_Set_Device_Appearance(BluetoothStackID, GAPSInstanceID, GAP_DEVICE_APPEARENCE_VALUE_GENERIC_COMPUTER);

                  /* Initialize the DIS Service.                        */
                  ret_val = DIS_Initialize_Service(BluetoothStackID, &ServiceID);
                  if(ret_val >= 0)
                  {
                     /* Save the Instance ID of the DIS Service.        */
                     DISInstanceID = (unsigned int)ret_val;

                     /* Set the discoverable attributes                 */
                     DIS_Set_Manufacturer_Name(BluetoothStackID, DISInstanceID, "Qualcomm Inc");
                     DIS_Set_Model_Number(BluetoothStackID, DISInstanceID, "Model Bluetopia");
                     DIS_Set_Serial_Number(BluetoothStackID, DISInstanceID, "Serial Number 1234");
                     DIS_Set_Software_Revision(BluetoothStackID, DISInstanceID, "Software 4.1");
                  }
                  else
                     PRINTF("DIS_Initialize_Service %d\n", ret_val);

                  /* Return success to the caller.                      */
                  ret_val        = 0;
               }
               else
               {
                  /* The Stack was NOT initialized successfully, inform */
                  /* the user and set the return value of the           */
                  /* initialization function to an error.               */
                  PRINTF("GAPS_Initialize_Service %d\n", ret_val);

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
               PRINTF("GATT_Initialize %d\n", ret_val);

               BluetoothStackID = 0;

               ret_val          = UNABLE_TO_INITIALIZE_STACK;
            }

            /* Initialize SPP context.                                  */
            BTPS_MemInitialize(&SPPContextInfo, 0, sizeof(SPPContextInfo));
         }
         else
         {
            /* The Stack was NOT initialized successfully, inform the   */
            /* user and set the return value of the initialization      */
            /* function to an error.                                    */
            PRINTF("BSC_Initialize %d\n", ret_val);

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

      /* Release the control Mutex.                                     */
      BTPS_CloseMutex(DeviceListMutex);
      DeviceListMutex = NULL;

      /* Close the shutdown event.                                      */
      BTPS_CloseEvent(ShutdownEvent);
      ShutdownEvent = NULL;

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

   /* WriteEIRInformation formats and writes extended inquiry response  */
   /* data.                                                             */
static void WriteEIRInformation(char *LocalName)
{
   int                               Index;
   int                               Length;
   Byte_t                            Status;
   SByte_t                           TxPowerEIR;
   Extended_Inquiry_Response_Data_t  EIRData;

   if((BluetoothStackID) && (HCI_Command_Supported(BluetoothStackID, HCI_SUPPORTED_COMMAND_WRITE_EXTENDED_INQUIRY_RESPONSE_BIT_NUMBER) > 0))
   {
      Index  = 0;

      /* Add the local name.                                            */
      if(LocalName)
      {
         Length = strlen(LocalName) + 1;

         EIRData.Extended_Inquiry_Response_Data[Index++] = (Byte_t)Length;
         EIRData.Extended_Inquiry_Response_Data[Index++] = (Byte_t)HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_LOCAL_NAME_COMPLETE;
         BTPS_MemCopy(&(EIRData.Extended_Inquiry_Response_Data[Index]), LocalName, Length);

         Index += Length;
      }

      /* Read the transmit power level.                                 */
      if((HCI_Read_Inquiry_Response_Transmit_Power_Level(BluetoothStackID, &Status, &TxPowerEIR)) && (Status == HCI_ERROR_CODE_NO_ERROR))
      {
         EIRData.Extended_Inquiry_Response_Data[Index++] = 2;
         EIRData.Extended_Inquiry_Response_Data[Index++] = HCI_EXTENDED_INQUIRY_RESPONSE_DATA_TYPE_TX_POWER_LEVEL;
         EIRData.Extended_Inquiry_Response_Data[Index++] = TxPowerEIR;
      }

      /* Write the EIR Data.                                            */
      if(Index)
        GAP_Write_Extended_Inquiry_Information(BluetoothStackID, HCI_EXTENDED_INQUIRY_RESPONSE_FEC_REQUIRED, &EIRData);
   }
}

   /* The following is responsable for opening an SPP Server on the     */
   /* default SPP Port.                                                 */
static int RegisterSPP(void)
{
   int   ret_val;
   char *ServiceName;

   /* Simply attempt to open an Serial Server, on RFCOMM Server Port 1. */
   ret_val = SPP_Open_Server_Port(BluetoothStackID, DEFAULT_SPP_SERVER_PORT, SPP_Event_Callback, (unsigned long)0);
   if(ret_val > 0)
   {
      /* Note the Serial Port Server ID of the opened Serial Port       */
      /* Server.                                                        */
      SPPContextInfo.SerialPortID = (unsigned int)ret_val;

      /* Create a Buffer to hold the Service Name.                      */
      if((ServiceName = BTPS_AllocateMemory(32)) != NULL)
      {
         /* The Server was opened successfully, now register a SDP      */
         /* Record indicating that an Serial Port Server exists.  Do    */
         /* this by first creating a Service Name.                      */
         BTPS_SprintF(ServiceName, "Serial Port Server Port %d", DEFAULT_SPP_SERVER_PORT);

         /* Now that a Service Name has been created try to Register the*/
         /* SDP Record.                                                 */
         ret_val = SPP_Register_SDP_Record(BluetoothStackID, SPPContextInfo.SerialPortID, NULL, ServiceName, &SPPContextInfo.SPPServerSDPHandle);
         if(!ret_val)
            PRINTF("Successfully registered SPP Service.\n");
         else
            SPP_Close_Server_Port(BluetoothStackID, SPPContextInfo.SerialPortID);

         BTPS_FreeMemory(ServiceName);
      }
   }

   return(ret_val);
}

   /* The following function is responsible for registering a SPPLE     */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int RegisterSPPLE(void)
{
   int                           ret_val;
   GATT_Attribute_Handle_Group_t ServiceHandleGroup;

   /* Initialize the handle group to 0 .                                */
   ServiceHandleGroup.Starting_Handle = 0;
   ServiceHandleGroup.Ending_Handle   = 0;

   /* Register the SPPLE Service.                                       */
   ret_val = GATT_Register_Service(BluetoothStackID, SPPLE_SERVICE_FLAGS, SPPLE_SERVICE_ATTRIBUTE_COUNT, (GATT_Service_Attribute_Entry_t *)SPPLE_Service, &ServiceHandleGroup, GATT_SPPLE_ServerEventCallback, 0);
   if(ret_val > 0)
   {
      /* Display success message.                                       */
      PRINTF("Successfully registered SPPLE Service.\n");

      /* Save the ServiceID of the registered service.                  */
      SPPLEServiceID = (unsigned int)ret_val;

      /* Return success to the caller.                                  */
      ret_val        = 0;
   }

   return(ret_val);
}

   /* The following function is responsible for unregistering a SPPLE   */
   /* Service.  This function will return zero on successful execution  */
   /* and a negative value on errors.                                   */
static int UnregisterSPPLE(void)
{
   int ret_val;

   if(!LEContextInfo.ConnectionID)
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

   /* The following function is responsible for transmitting the        */
   /* specified number of credits to the remote device.                 */
static void SPPLESendCredits(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int GrantedCredits)
{
   int              Result;
   NonAlignedWord_t Credits;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      /* Add to the total number of credits to send.                    */
      LEContextInfo->CreditsToSend += (Word_t)GrantedCredits;

      /* Only attempt to send the credits if the LE buffer is not full. */
      if(LEContextInfo->BufferFull == FALSE)
      {
         /* Format the credit packet.                                   */
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Credits, LEContextInfo->CreditsToSend);

         /* We are acting as a server so notify the Rx Credits          */
         /* characteristic.                                             */
         if(DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
            Result = GATT_Handle_Value_Notification(BluetoothStackID, SPPLEServiceID, LEContextInfo->ConnectionID, SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET, WORD_SIZE, (Byte_t *)&Credits);
         else
            Result = 0;

         /* If an error occurred we need to queue the credits to try    */
         /* again.                                                      */
         if(Result >= 0)
         {
            /* Clear the queued credit count as if there were any queued*/
            /* credits they have now been sent.                         */
            LEContextInfo->CreditsToSend = 0;
         }
         else
         {
            if(Result == BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE)
            {
               /* Flag that the buffer is full.                         */
               LEContextInfo->BufferFull = TRUE;
            }
         }
      }
   }
}

   /* The following function is responsible for handling a received     */
   /* credit, event.                                                    */
static void SPPLEReceiveCreditEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int Credits)
{
   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      DBG_PRINT("Received %d Credits\n", Credits);

      /* If this is a real credit event store the number of credits.    */
      LEContextInfo->TransmitCredits += (Word_t)Credits;
   }
}

   /* The following function sends the specified data to the specified  */
   /* data.  This function will queue any of the data that does not go  */
   /* out.  This function returns the number of bytes sent if all the   */
   /* data was sent, or 0.                                              */
   /* * NOTE * If DataLength is 0 and Data is NULL then all queued data */
   /*          will be sent.                                            */
static unsigned int SPPLESendData(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo)
{
   int    ret_val = 0;
   int    Result;
   Word_t DataCount;
   Word_t Length;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (LEContextInfo->BytesToSend) && (SendBuffer))
   {
      if((DeviceInfo) && (DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE))
      {
         /* Check to see if we have credits to use to transmit the data */
         /* (and that the buffer is not FULL).                          */
         while((LEContextInfo->TransmitCredits) && (LEContextInfo->BytesToSend) && (LEContextInfo->BufferFull == FALSE))
         {
            if(LEContextInfo->TransmitCredits > LEContextInfo->BytesToSend)
               DataCount = (Word_t)(LEContextInfo->BytesToSend);
            else
               DataCount = (Word_t)LEContextInfo->TransmitCredits;

            /* Get the maximum length of what we can send in this       */
            /* transaction.                                             */
            if(DataCount > (Word_t)(LEContextInfo->MTU - SendIndex))
               DataCount = (Word_t)(LEContextInfo->MTU - SendIndex);

            if(DataCount)
            {
               if(LEContextInfo->OutIndex >= LEContextInfo->InIndex)
                  Length = (Word_t)(SPPLE_DATA_BUFFER_LENGTH-LEContextInfo->OutIndex);
               else
                  Length = (Word_t)(LEContextInfo->InIndex-LEContextInfo->OutIndex);

               if(Length > DataCount)
                  Length = DataCount;

               BTPS_MemCopy(&SendBuffer[SendIndex], &(LEContextInfo->RxBuffer[LEContextInfo->OutIndex]), Length);

               SendIndex                      += Length;
               LEContextInfo->OutIndex         = (Word_t)((LEContextInfo->OutIndex + Length) % SPPLE_DATA_BUFFER_LENGTH);
               LEContextInfo->BytesToSend     -= Length;
               LEContextInfo->TransmitCredits -= Length;
            }

            if((!LEContextInfo->BytesToSend) || (!LEContextInfo->TransmitCredits) || (!DataCount))
            {
               /* We are acting as SPPLE Server, so notify the Tx       */
               /* Characteristic.                                       */
               Result = GATT_Handle_Value_Notification(BluetoothStackID, SPPLEServiceID, LEContextInfo->ConnectionID, SPPLE_TX_CHARACTERISTIC_ATTRIBUTE_OFFSET, (Word_t)SendIndex, SendBuffer);
               if(Result >= 0)
               {
                  /* Adjust the data.                                   */
                  SendIndex -= (Word_t)Result;
                  ret_val   += Result;
                  SPPLESendCredits(LEContextInfo, DeviceInfo, Result);
               }
               else
               {
                  /* Check to see what error has occurred.              */
                  if(Result == BTPS_ERROR_INSUFFICIENT_BUFFER_SPACE)
                  {
                     /* Flag that the LE buffer is full.                */
                     LEContextInfo->BufferFull = TRUE;
                  }
                  else
                  {
                     DBG_PRINT("SEND failed with error %d\n", Result);
                  }
               }
            }
         }
      }
   }
   return(ret_val);
}

   /* The following function is responsible for handling a data         */
   /* indication event.                                                 */
static void SPPLEDataIndicationEvent(LE_Context_Info_t *LEContextInfo, DeviceInfo_t *DeviceInfo, unsigned int DataLength, Byte_t *Data)
{
   Word_t Length;

   /* Verify that the input parameters are semi-valid.                  */
   if((LEContextInfo) && (DeviceInfo))
   {
      /* Display a Data indication event.                               */
      DBG_PRINT("\nData Indication Event, Connection ID %u, Received %u bytes %p.\n", LEContextInfo->ConnectionID, DataLength, Data);

      if((DataLength) && (Data))
      {
         /* Record recv packet lenght and time                          */
         SPPLEDataMetaRecord(DataLength);

         /* Copy the data to the RX Buffer.                             */
         while((DataLength) && (LEContextInfo->BytesToSend < SPPLE_DATA_BUFFER_LENGTH))
         {
            /* Determine how much data we can copy in this loop.        */
            if(LEContextInfo->InIndex < LEContextInfo->OutIndex)
               Length = (Word_t)(LEContextInfo->OutIndex - LEContextInfo->InIndex);
            else
               Length = (Word_t)(SPPLE_DATA_BUFFER_LENGTH - LEContextInfo->InIndex);

            if((Word_t)DataLength < Length)
               Length = (Word_t)DataLength;

            if(Length > (Word_t)(SPPLE_DATA_BUFFER_LENGTH-LEContextInfo->BytesToSend))
            {
               DBG_PRINT("Buffer Overflow %d\n", (DataLength - Length));
               Length = (Word_t)(SPPLE_DATA_BUFFER_LENGTH-LEContextInfo->BytesToSend);
            }

            BTPS_MemCopy(&LEContextInfo->RxBuffer[LEContextInfo->InIndex], Data, Length);
            LEContextInfo->InIndex      = (Word_t)((LEContextInfo->InIndex + Length) % SPPLE_DATA_BUFFER_LENGTH);
            DataLength                 -= (unsigned int)Length;
            Data                       += Length;
            LEContextInfo->BytesToSend += (Word_t)Length;
         }
      }
   }
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
      Capabilities->Bonding_Type  = lbtBonding;
      Capabilities->IO_Capability = LE_Parameters.IOCapability;
      Capabilities->MITM          = FALSE;
      Capabilities->OOB_Present   = FALSE;

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
         DBG_PRINT("Attempting to Pair to %s.\n", BoardStr);

         /* Attempt to pair to the remote device.                       */
         if(ConnectionMaster)
         {
            /* Start the pairing process.                               */
            ret_val = GAP_LE_Pair_Remote_Device(BluetoothStackID, BD_ADDR, &Capabilities, GAP_LE_Event_Callback, 0);

            DBG_PRINT("     GAP_LE_Pair_Remote_Device returned %d.\n", ret_val);
         }
         else
         {
            /* As a slave we can only request that the Master start     */
            /* the pairing process.                                     */
            ret_val = GAP_LE_Request_Security(BluetoothStackID, BD_ADDR, Capabilities.Bonding_Type, Capabilities.MITM, GAP_LE_Event_Callback, 0);

            DBG_PRINT("     GAP_LE_Request_Security returned %d.\n", ret_val);
         }
      }
      else
      {
         DBG_PRINT("Invalid Parameters.\n");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      DBG_PRINT("Stack ID Invalid.\n");

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
      DBG_PRINT("Sending Pairing Response to %s.\n", BoardStr);

      /* We must be the slave if we have received a Pairing Request     */
      /* thus we will respond with our capabilities.                    */
      AuthenticationResponseData.GAP_LE_Authentication_Type = larPairingCapabilities;
      AuthenticationResponseData.Authentication_Data_Length = GAP_LE_PAIRING_CAPABILITIES_SIZE;

      /* Configure the Application Pairing Parameters.                  */
      ConfigureCapabilities(&(AuthenticationResponseData.Authentication_Data.Pairing_Capabilities));

      /* Attempt to pair to the remote device.                          */
      ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, &AuthenticationResponseData);

      DBG_PRINT("GAP_LE_Authentication_Response returned %d.\n", ret_val);
   }
   else
   {
      DBG_PRINT("Stack ID Invalid.\n");

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
         DBG_PRINT("   Calling GAP_LE_Generate_Long_Term_Key.\n");

         /* Generate a new LTK, EDIV and Rand tuple.                    */
         ret_val = GAP_LE_Generate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.LTK), &LocalDiv, &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.EDIV), &(GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Rand));
         if(!ret_val)
         {
            DBG_PRINT("   Encryption Information Request Response.\n");

            /* Response to the request with the LTK, EDIV and Rand      */
            /* values.                                                  */
            GAP_LE_Authentication_Response_Information->GAP_LE_Authentication_Type                                     = larEncryptionInformation;
            GAP_LE_Authentication_Response_Information->Authentication_Data_Length                                     = GAP_LE_ENCRYPTION_INFORMATION_DATA_SIZE;
            GAP_LE_Authentication_Response_Information->Authentication_Data.Encryption_Information.Encryption_Key_Size = KeySize;

            ret_val = GAP_LE_Authentication_Response(BluetoothStackID, BD_ADDR, GAP_LE_Authentication_Response_Information);
            if(!ret_val)
            {
               DBG_PRINT("   GAP_LE_Authentication_Response (larEncryptionInformation) success.\n");
            }
            else
            {
               DBG_PRINT("   Error - SM_Generate_Long_Term_Key returned %d.\n", ret_val);
            }
         }
         else
         {
            DBG_PRINT("   Error - SM_Generate_Long_Term_Key returned %d.\n", ret_val);
         }
      }
      else
      {
         DBG_PRINT("Invalid Parameters.\n");

         ret_val = INVALID_PARAMETERS_ERROR;
      }
   }
   else
   {
      DBG_PRINT("Stack ID Invalid.\n");

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
   int       ret_val = 0;
   BD_ADDR_t Null_BD_ADDR;

   ASSIGN_BD_ADDR(Null_BD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
   if(COMPARE_BD_ADDR(BD_ADDR, Null_BD_ADDR))
      BTPS_MemInitialize(LinkKeyInfo, 0, sizeof(LinkKeyInfo));
   else
   {
      /* Individual Link Key.  Go ahead and see if know about the entry */
      /* in the list.                                                   */
      for(ret_val = 0; ret_val < MAX_SUPPORTED_LINK_KEYS; ret_val++)
      {
         if(COMPARE_BD_ADDR(BD_ADDR, LinkKeyInfo[ret_val].BD_ADDR))
         {
            LinkKeyInfo[ret_val].BD_ADDR = Null_BD_ADDR;
            ret_val                      = 0;
            break;
         }
      }
   }

   return(ret_val);
}

   /* The following function is responsible for enabling LE             */
   /* Advertisements.  This function returns zero on successful         */
   /* execution and a negative value on all errors.                     */
static int AdvertiseLE(int Mode)
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
      if(!Mode)
      {
         /* Disable Advertising.                                        */
         ret_val = GAP_LE_Advertising_Disable(BluetoothStackID);
         if(!ret_val)
         {
            ApplicationFlags &= (Byte_t)~APPLICATION_FLAG_ADVERTISING_ENABLED;
            DBG_PRINT("   GAP_LE_Advertising_Disable success.\n");
         }
         else
         {
            DBG_PRINT("   GAP_LE_Advertising_Disable returned %d.\n", ret_val);
         }
      }
      else
      {
         /* Enable Advertising.  Set the Advertising Data.              */
         BTPS_MemInitialize(&(Advertisement_Data_Buffer.AdvertisingData), 0, sizeof(Advertising_Data_t));

         /* Set the Flags A/D Field (1 byte type and 1 byte Flags.      */
         Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] = 2;
         Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[1] = HCI_LE_ADVERTISING_REPORT_DATA_TYPE_FLAGS;
         Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = 0;

         /* Configure the flags field based on the Discoverability Mode.*/
         if(LE_Parameters.DiscoverabilityMode == dmGeneralDiscoverableMode)
            Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_GENERAL_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
         else
         {
            if(LE_Parameters.DiscoverabilityMode == dmLimitedDiscoverableMode)
               Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[2] = HCI_LE_ADVERTISING_FLAGS_LIMITED_DISCOVERABLE_MODE_FLAGS_BIT_MASK;
         }

         /* Write thee advertising data to the chip.                    */
         ret_val = GAP_LE_Set_Advertising_Data(BluetoothStackID, (Advertisement_Data_Buffer.AdvertisingData.Advertising_Data[0] + 1), &(Advertisement_Data_Buffer.AdvertisingData));
         if(!ret_val)
         {
            BTPS_MemInitialize(&(Advertisement_Data_Buffer.ScanResponseData), 0, sizeof(Scan_Response_Data_t));

            /* Set the Scan Response Data.                              */
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
               /* Set up the advertising parameters.                    */
               AdvertisingParameters.Advertising_Channel_Map   = HCI_LE_ADVERTISING_CHANNEL_MAP_DEFAULT;
               AdvertisingParameters.Scan_Request_Filter       = fpNoFilter;
               AdvertisingParameters.Connect_Request_Filter    = fpNoFilter;
               AdvertisingParameters.Advertising_Interval_Min  = 100;
               AdvertisingParameters.Advertising_Interval_Max  = 200;

               /* Configure the Connectability Parameters.              */
               /* * NOTE * Since we do not ever put ourselves to be     */
               /*          direct connectable then we will set the      */
               /*          DirectAddress to all 0s.                     */
               ConnectabilityParameters.Connectability_Mode   = LE_Parameters.ConnectableMode;
               ConnectabilityParameters.Own_Address_Type      = latPublic;
               ConnectabilityParameters.Direct_Address_Type   = latPublic;
               ASSIGN_BD_ADDR(ConnectabilityParameters.Direct_Address, 0, 0, 0, 0, 0, 0);

               /* Now enable advertising.                               */
               ret_val = GAP_LE_Advertising_Enable(BluetoothStackID, TRUE, &AdvertisingParameters, &ConnectabilityParameters, GAP_LE_Event_Callback, 0);
               if(!ret_val)
               {
                  DBG_PRINT("   GAP_LE_Advertising_Enable success.\n");
                  ApplicationFlags |= (Byte_t)APPLICATION_FLAG_ADVERTISING_ENABLED;
               }
               else
               {
                  DBG_PRINT("   GAP_LE_Advertising_Enable returned %d.\n", ret_val);

                  ret_val = FUNCTION_ERROR;
               }
            }
            else
            {
               DBG_PRINT("   GAP_LE_Set_Advertising_Data(dtScanResponse) returned %d.\n", ret_val);

               ret_val = FUNCTION_ERROR;
            }

         }
         else
         {
            DBG_PRINT("   GAP_LE_Set_Advertising_Data(dtAdvertising) returned %d.\n", ret_val);

            ret_val = FUNCTION_ERROR;
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

   /* The following function is responsible for starting a scan.        */
static int ScanLE(unsigned int State)
{
   int Result;

   /* First, determine if the input parameters appear to be semi-valid. */
   if(BluetoothStackID)
   {
      if(State)
      {
         /* Not currently scanning, go ahead and attempt to perform the    */
         /* scan.                                                          */
         Result = GAP_LE_Perform_Scan(BluetoothStackID, ScanType, 10, 10, latPublic, fpNoFilter, TRUE, GAP_LE_Event_Callback, 0);

         if(!Result)
         {
            DBG_PRINT("%s Scan started successfully.\n", ((ScanType)?"Active":"Passive"));
         }
         else
         {
            /* Unable to start the scan.                                   */
            DBG_PRINT("Unable to perform scan: %d\n", Result);
         }
      }
      else
      {
         Result = GAP_LE_Cancel_Scan(BluetoothStackID);
         if(!Result)
         {
            DBG_PRINT("Scan stopped successfully.\n");
         }
         else
         {
            /* Error stopping scan.                                        */
            DBG_PRINT("Unable to stop scan: %d\n", Result);
         }
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
   unsigned int                   WhiteListChanged;
   GAP_LE_White_List_Entry_t      WhiteListEntry;
   GAP_LE_Connection_Parameters_t ConnectionParameters;

   /* First, determine if the input parameters appear to be semi-valid. */
   if((BluetoothStackID) && (!COMPARE_NULL_BD_ADDR(BD_ADDR)))
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
            DBG_PRINT("Connection Request successful.\n");
         }
         else
         {
            /* Unable to create connection.                       */
            DBG_PRINT("Unable to create connection: %d.\n", Result);
         }
      }
      else
      {
         /* Unable to add device to White List.                   */
         DBG_PRINT("Unable to add device to White List.\n");
      }
   }
   else
      Result = -1;

   return(Result);
}

   /* ***************************************************************** */
   /*                         Event Callbacks                           */
   /* ***************************************************************** */

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
static void BTPSAPI SPPLE_AsynchronousCallback(unsigned int BluetoothStackID, unsigned long CallbackParameter)
{
   DeviceInfo_t *DeviceInfo;

   if((BluetoothStackID) && (CallbackParameter))
   {
      /* Acquire the List Mutex.                                        */
      if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
      {
         /* Check to make sure that there is data to send.              */
         DeviceInfo = (DeviceInfo_t *)CallbackParameter;
         SPPLESendData(&LEContextInfo, DeviceInfo);

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
   BoardStr_t                                    BoardStr;
   unsigned int                                  Index;
   DeviceInfo_t                                 *DeviceInfo;
   Boolean_t                                     UpdatePrompt = TRUE;
   Long_Term_Key_t                               GeneratedLTK;
   GAP_LE_Connection_Parameters_t                ConnectionParameters;
   GAP_LE_Advertising_Report_Data_t             *DeviceEntryPtr;
   GAP_LE_Authentication_Event_Data_t           *Authentication_Event_Data;
   GAP_LE_Authentication_Response_Information_t  GAP_LE_Authentication_Response_Information;
   GAP_LE_Security_Information_t                 GAP_LE_Security_Information;

   /* Verify that all parameters to this callback are Semi-Valid.       */
   if((BluetoothStackID) && (GAP_LE_Event_Data))
   {
      switch(GAP_LE_Event_Data->Event_Data_Type)
      {
         case etLE_Advertising_Report:
            for(Index = 0; Index < GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Number_Device_Entries; Index++)
            {
               DeviceEntryPtr = &(GAP_LE_Event_Data->Event_Data.GAP_LE_Advertising_Report_Event_Data->Advertising_Data[Index]);
               /* Display the packet type for the device             */
               switch(DeviceEntryPtr->Advertising_Report_Type)
               {
                  case rtConnectableUndirected:
                     DBG_PRINT("  Advertising Type: %s.\n", "rtConnectableUndirected");
                     break;
                  case rtConnectableDirected:
                     DBG_PRINT("  Advertising Type: %s.\n", "rtConnectableDirected");
                     break;
                  case rtScannableUndirected:
                     DBG_PRINT("  Advertising Type: %s.\n", "rtScannableUndirected");
                     break;
                  case rtNonConnectableUndirected:
                     DBG_PRINT("  Advertising Type: %s.\n", "rtNonConnectableUndirected");
                     break;
                  case rtScanResponse:
                     DBG_PRINT("  Advertising Type: %s.\n", "rtScanResponse");
                     break;
               }

               /* Display the Address Type.                          */
               if(DeviceEntryPtr->Address_Type == latPublic)
               {
                  DBG_PRINT("  Address Type: %s.\n","atPublic");
               }
               else
               {
                  DBG_PRINT("  Address Type: %s.\n","atRandom");
               }

               /* Display the Device Address.                        */
               DBG_PRINT("  Address: 0x%02X%02X%02X%02X%02X%02X.\n", DeviceEntryPtr->BD_ADDR.BD_ADDR5, DeviceEntryPtr->BD_ADDR.BD_ADDR4, DeviceEntryPtr->BD_ADDR.BD_ADDR3, DeviceEntryPtr->BD_ADDR.BD_ADDR2, DeviceEntryPtr->BD_ADDR.BD_ADDR1, DeviceEntryPtr->BD_ADDR.BD_ADDR0);
               DBG_PRINT("  RSSI: %d.\n", DeviceEntryPtr->RSSI);
               DBG_PRINT("  Data Length: %d.\n", DeviceEntryPtr->Raw_Report_Length);
               if((DeviceEntryPtr->Advertising_Report_Type == rtConnectableUndirected) && (COMPARE_BD_ADDR(DeviceEntryPtr->BD_ADDR, TargetBD_ADDR)))
               {
                  ScanLE(0);
                  Result = ConnectLEDevice(BluetoothStackID, rtConnectableUndirected, TargetBD_ADDR, FALSE);
                  if(Result)
                  {
                     ScanLE(1);
                  }
               }
            }
            break;
         case etLE_Connection_Complete:
            DBG_PRINT("etLE_Connection_Complete with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address, BoardStr);

               DBG_PRINT("   Status:       0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status);
               DBG_PRINT("   Role:         %s.\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)?"Master":"Slave");
               DBG_PRINT("   Address Type: %s.\n", (GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type == latPublic)?"Public":"Random");
               DBG_PRINT("   BD_ADDR:      %s.\n", BoardStr);

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  /* Set a global flag to indicate if we are the        */
                  /* connection master.                                 */
                  LEContextInfo.ConnectedBD_ADDR = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address;
                  /* Make sure that no entry already exists.            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(LEContextInfo.ConnectedBD_ADDR)) == NULL)
                  {
                     /* No entry exists so create one.                  */
                     DeviceInfo = AddDeviceInfoEntryByBD_ADDR(LEContextInfo.ConnectedBD_ADDR);
                     if(DeviceInfo)
                     {
                        DeviceInfo->AddressType = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Peer_Address_Type;
                     }
                     else
                     {
                        DBG_PRINT("Failed to add device to Device Info List.");
                     }
                  }

                  /* Check to see if we need to re-establish security.  */
                  if((DeviceInfo) && (DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID))
                  {
                     /* If we are the Master of the connection we will  */
                     /* attempt to Re-Establish Security if a LTK for   */
                     /* this device exists (i.e.  we previously paired).*/
                     if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Complete_Event_Data->Master)
                     {
                        /* Re-Establish Security if there is a LTK that */
                        /* is stored for this device.                   */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID)
                        {
                           /* Re-Establish Security with this LTK.      */
                           DBG_PRINT("Attempting to Re-Establish Security.\n");

                           /* Attempt to re-establish security to this  */
                           /* device.                                   */
                           GAP_LE_Security_Information.Local_Device_Is_Master                                      = TRUE;
                           GAP_LE_Security_Information.Security_Information.Master_Information.LTK                 = DeviceInfo->LTK;
                           GAP_LE_Security_Information.Security_Information.Master_Information.EDIV                = DeviceInfo->EDIV;
                           GAP_LE_Security_Information.Security_Information.Master_Information.Rand                = DeviceInfo->Rand;
                           GAP_LE_Security_Information.Security_Information.Master_Information.Encryption_Key_Size = DeviceInfo->EncryptionKeySize;

                           Result = GAP_LE_Reestablish_Security(BluetoothStackID, DeviceInfo->BD_ADDR, &GAP_LE_Security_Information, GAP_LE_Event_Callback, 0);
                           if(Result)
                           {
                              DBG_PRINT("GAP_LE_Reestablish_Security returned %d.\n",Result);
                           }
                        }
                     }
                  }
               }
            }
            break;
         case etLE_Disconnection_Complete:
            DBG_PRINT("etLE_Disconnection_Complete with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data)
            {
               DBG_PRINT("   Status: 0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Status);
               DBG_PRINT("   Reason: 0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Reason);

               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Disconnection_Complete_Event_Data->Peer_Address, BoardStr);
               DBG_PRINT("   BD_ADDR: %s.\n", BoardStr);

               SPPLEDataMetaRecordPrint();
            }
            if(ApplicationFlags & APPLICATION_FLAG_SHUTDOWN_IN_PROGRESS)
            {
               BTPS_SetEvent(ShutdownEvent);
            }
            break;
         case etLE_Connection_Parameter_Update_Request:
            DBG_PRINT("\netLE_Connection_Parameter_Update_Request with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, BoardStr);
               DBG_PRINT("   BD_ADDR:             %s.\n", BoardStr);
               DBG_PRINT("   Minimum Interval:    %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min);
               DBG_PRINT("   Maximum Interval:    %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max);
               DBG_PRINT("   Slave Latency:       %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency);
               DBG_PRINT("   Supervision Timeout: %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout);

               /* Initialize the connection parameters.                 */
               ConnectionParameters.Connection_Interval_Min    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Min;
               ConnectionParameters.Connection_Interval_Max    = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Interval_Max;
               ConnectionParameters.Slave_Latency              = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Slave_Latency;
               ConnectionParameters.Supervision_Timeout        = GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->Conn_Supervision_Timeout;
               ConnectionParameters.Minimum_Connection_Length  = 0;
               ConnectionParameters.Maximum_Connection_Length  = 10000;

               DBG_PRINT("\nAttempting to accept connection parameter update request.\n");

               /* Go ahead and accept whatever the slave has requested. */
               Result = GAP_LE_Connection_Parameter_Update_Response(BluetoothStackID, GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Update_Request_Event_Data->BD_ADDR, TRUE, &ConnectionParameters);
               if(!Result)
               {
                  DBG_PRINT("      GAP_LE_Connection_Parameter_Update_Response() success.\n");
               }
               else
               {
                  DBG_PRINT("      GAP_LE_Connection_Parameter_Update_Response() error %d.\n", Result);
               }
            }
            break;
         case etLE_Connection_Parameter_Updated:
            DBG_PRINT("\netLE_Connection_Parameter_Updated with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data)
            {
               BD_ADDRToStr(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->BD_ADDR, BoardStr);
               DBG_PRINT("   Status:              0x%02X.\n", GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status);
               DBG_PRINT("   BD_ADDR:             %s.\n", BoardStr);

               if(GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  DBG_PRINT("   Connection Interval: %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Connection_Interval);
                  DBG_PRINT("   Slave Latency:       %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Slave_Latency);
                  DBG_PRINT("   Supervision Timeout: %u.\n", (unsigned int)GAP_LE_Event_Data->Event_Data.GAP_LE_Connection_Parameter_Updated_Event_Data->Current_Connection_Parameters.Supervision_Timeout);
               }
            }
            break;
         case etLE_Authentication:
            DBG_PRINT("etLE_Authentication with size %d.\n", (int)GAP_LE_Event_Data->Event_Data_Size);

            /* Make sure the authentication event data is valid before  */
            /* continuing.                                              */
            if((Authentication_Event_Data = GAP_LE_Event_Data->Event_Data.GAP_LE_Authentication_Event_Data) != NULL)
            {
               BD_ADDRToStr(Authentication_Event_Data->BD_ADDR, BoardStr);

               switch(Authentication_Event_Data->GAP_LE_Authentication_Event_Type)
               {
                  case latLongTermKeyRequest:
                     DBG_PRINT("    latKeyRequest: \n");
                     DBG_PRINT("      BD_ADDR: %s.\n", BoardStr);

                     /* The other side of a connection is requesting    */
                     /* that we start encryption. Thus we should        */
                     /* regenerate LTK for this connection and send it  */
                     /* to the chip.                                    */
                     Result = GAP_LE_Regenerate_Long_Term_Key(BluetoothStackID, (Encryption_Key_t *)(&DHK), (Encryption_Key_t *)(&ER), Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.EDIV, &(Authentication_Event_Data->Authentication_Event_Data.Long_Term_Key_Request.Rand), &GeneratedLTK);
                     if(!Result)
                     {
                        DBG_PRINT("      GAP_LE_Regenerate_Long_Term_Key Success.\n");

                        /* Respond with the Re-Generated Long Term Key. */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type                                        = larLongTermKey;
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length                                        = GAP_LE_LONG_TERM_KEY_INFORMATION_DATA_SIZE;
                        GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Encryption_Key_Size = GAP_LE_MAXIMUM_ENCRYPTION_KEY_SIZE;
                        GAP_LE_Authentication_Response_Information.Authentication_Data.Long_Term_Key_Information.Long_Term_Key       = GeneratedLTK;
                     }
                     else
                     {
                        DBG_PRINT("      GAP_LE_Regenerate_Long_Term_Key returned %d.\n",Result);

                        /* Since we failed to generate the requested key*/
                        /* we should respond with a negative response.  */
                        GAP_LE_Authentication_Response_Information.GAP_LE_Authentication_Type = larLongTermKey;
                        GAP_LE_Authentication_Response_Information.Authentication_Data_Length = 0;
                     }

                     /* Send the Authentication Response.               */
                     Result = GAP_LE_Authentication_Response(BluetoothStackID, Authentication_Event_Data->BD_ADDR, &GAP_LE_Authentication_Response_Information);
                     if(Result)
                     {
                        DBG_PRINT("      GAP_LE_Authentication_Response returned %d.\n",Result);
                     }
                     break;
                  case latSecurityRequest:
                     /* Display the data for this event.                */
                     /* * NOTE * This is only sent from Slave to Master.*/
                     /*          Thus we must be the Master in this     */
                     /*          connection.                            */
                     DBG_PRINT("    latSecurityRequest:.\n");
                     DBG_PRINT("      BD_ADDR: %s.\n", BoardStr);
                     DBG_PRINT("      Bonding Type: %s.\n", ((Authentication_Event_Data->Authentication_Event_Data.Security_Request.Bonding_Type == lbtBonding)?"Bonding":"No Bonding"));
                     DBG_PRINT("      MITM: %s.\n", ((Authentication_Event_Data->Authentication_Event_Data.Security_Request.MITM)?"YES":"NO"));

                     /* Determine if we have previously paired with the */
                     /* device. If we have paired we will attempt to    */
                     /* re-establish security using a previously        */
                     /* exchanged LTK.                                  */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(Authentication_Event_Data->BD_ADDR)) != NULL)
                     {
                        /* Determine if a Valid Long Term Key is stored */
                        /* for this device.                             */
                        if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID)
                        {
                           DBG_PRINT("Attempting to Re-Establish Security.\n");

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
                              DBG_PRINT("GAP_LE_Reestablish_Security returned %d.\n",Result);
                           }
                        }
                        else
                        {
                           CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                           /* We do not have a stored Link Key for this */
                           /* device so go ahead and pair to this       */
                           /* device.                                   */
                           SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                        }
                     }
                     else
                     {
                        CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                        /* There is no Key Info Entry for this device   */
                        /* so we will just treat this as a slave        */
                        /* request and initiate pairing.                */
                        SendPairingRequest(Authentication_Event_Data->BD_ADDR, TRUE);
                     }

                     break;
                  case latPairingRequest:
                     CurrentLERemoteBD_ADDR = Authentication_Event_Data->BD_ADDR;

                     DBG_PRINT("Pairing Request: %s.\n",BoardStr);

                     /* This is a pairing request. Respond with a       */
                     /* Pairing Response.                               */
                     /* * NOTE * This is only sent from Master to Slave.*/
                     /*          Thus we must be the Slave in this      */
                     /*          connection.                            */

                     /* Send the Pairing Response.                      */
                     SlavePairingRequestResponse(Authentication_Event_Data->BD_ADDR);
                     break;
                  case latConfirmationRequest:
                     DBG_PRINT("latConfirmationRequest.\n");

                     if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtNone)
                     {
                        DBG_PRINT("Invoking Just Works.\n");

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
                           DBG_PRINT("GAP_LE_Authentication_Response returned %d.\n",Result);
                        }
                     }
                     else
                     {
                        if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtPasskey)
                        {
                           DBG_PRINT("Call LEPasskeyResponse [PASSCODE].\n");
                        }
                        else
                        {
                           if(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Request_Type == crtDisplay)
                           {
                              DBG_PRINT("Passkey: %06u.\n", (unsigned int)(Authentication_Event_Data->Authentication_Event_Data.Confirmation_Request.Display_Passkey));
                           }
                        }
                     }
                     break;
                  case latSecurityEstablishmentComplete:
                     DBG_PRINT("Security Re-Establishment Complete: %s.\n", BoardStr);
                     DBG_PRINT("                            Status: 0x%02X.\n", Authentication_Event_Data->Authentication_Event_Data.Security_Establishment_Complete.Status);
                     break;
                  case latPairingStatus:
                     ASSIGN_BD_ADDR(CurrentLERemoteBD_ADDR, 0, 0, 0, 0, 0, 0);

                     DBG_PRINT("Pairing Status: %s.\n", BoardStr);
                     DBG_PRINT("        Status: 0x%02X.\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status);

                     if(Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Status == GAP_LE_PAIRING_STATUS_NO_ERROR)
                     {
                        DBG_PRINT("        Key Size: %d.\n", Authentication_Event_Data->Authentication_Event_Data.Pairing_Status.Negotiated_Encryption_Key_Size);
                     }
                     break;
                  case latEncryptionInformationRequest:
                     DBG_PRINT("Encryption Information Request %s.\n", BoardStr);

                     /* Search for the entry for this slave to store */
                     /* the information into.                        */
                     if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(Authentication_Event_Data->BD_ADDR)) != NULL)
                     {
                        DeviceInfo->LTK               = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.LTK;
                        DeviceInfo->EDIV              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.EDIV;
                        DeviceInfo->Rand              = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Rand;
                        DeviceInfo->EncryptionKeySize = Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size;
                        DeviceInfo->Flags            |= DEVICE_INFO_FLAG_LTK_VALID;
                     }

                     /* Generate new LTK, EDIV and Rand and respond with*/
                     /* them.                                           */
                     EncryptionInformationRequestResponse(Authentication_Event_Data->BD_ADDR, Authentication_Event_Data->Authentication_Event_Data.Encryption_Request_Information.Encryption_Key_Size, &GAP_LE_Authentication_Response_Information);
                     break;
                  case latEncryptionInformation:
                     /* Display the information from the event.         */
                     DBG_PRINT(" Encryption Information from RemoteDevice: %s.\n", BoardStr);
                     DBG_PRINT("                             Key Size: %d.\n", Authentication_Event_Data->Authentication_Event_Data.Encryption_Information.Encryption_Key_Size);
                     break;
                  default:
                     DBG_PRINT(" Unhandled Type %d\n", Authentication_Event_Data->GAP_LE_Authentication_Event_Type);
                     break;
               }
            }
            break;
         default:
            break;
      }

      /* Display the command prompt.                                    */
      if(UpdatePrompt)
      {
         DBG_PRINT("\r\nSPPLE>");
      }
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
                  /* Acquire the List Mutex.                            */
                  if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
                  {
                     /* Grab the device info for the currently connected*/
                     /* device.                                         */
                     DeviceInfo = &RemoteDeviceInfo;
                     if(DeviceInfo)
                     {
                        /* Determine which request this read is coming  */
                        /* for.                                         */
                        switch(GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset)
                        {
                           case SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor);
                              break;
                           case SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, LEContextInfo.TransmitCredits);
                              break;
                           case SPPLE_RX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                              Value = (Word_t)(SPPLE_DATA_BUFFER_LENGTH-LEContextInfo.BytesToSend);
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, Value);
                              break;
                           case SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                              ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(Temp, DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor);
                              break;
                        }

                        GATT_Read_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, WORD_SIZE, Temp);
                     }
                     else
                     {
                        DBG_PRINT("Error - No device info entry for this device.\n");
                     }

                     /* Release the List Mutex.                         */
                     BTPS_ReleaseMutex(DeviceListMutex);
                  }
               }
               else
                  GATT_Error_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->TransactionID, GATT_ServerEventData->Event_Data.GATT_Read_Request_Data->AttributeOffset, ATT_PROTOCOL_ERROR_CODE_ATTRIBUTE_NOT_LONG);
            }
            else
            {
               DBG_PRINT("Invalid Read Request Event Data.\n");
            }
            break;
         case etGATT_Server_Write_Request:
            /* Verify that the Event Data is valid.                     */
            if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data)
            {
               /* Acquire the List Mutex.                               */
               if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
               {
                  /* Grab the device info for the currently connected   */
                  /* device.                                            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(LEContextInfo.ConnectedBD_ADDR)) != NULL)
                  {
                     if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueOffset == 0)
                     {
                        /* Cache the Attribute Offset.                  */
                        AttributeOffset = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeOffset;
                        AttributeLength = GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength;

                        /* Verify that the value is of the correct      */
                        /* length.                                      */
                        if((AttributeOffset == SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET) || ((AttributeLength) && (AttributeLength <= WORD_SIZE)))
                        {
                           /* Since the value appears valid go ahead and*/
                           /* accept the write request.                 */
                           GATT_Write_Response(BluetoothStackID, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->TransactionID);

                           /* If this is not a write to the Rx          */
                           /* Characteristic we will read the data here.*/
                           if(AttributeOffset != SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET)
                           {
                              if(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength == WORD_SIZE)
                                 Value = READ_UNALIGNED_WORD_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                              else
                                 Value = READ_UNALIGNED_BYTE_LITTLE_ENDIAN(GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                           }
                           else
                              Value = 0;

                           /* Determine which attribute this write      */
                           /* request is for.                           */
                           switch(AttributeOffset)
                           {
                              case SPPLE_TX_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                 /* Client has updated the Tx CCCD.  Now*/
                                 /* we need to check if we have any data*/
                                 /* to send.                            */
                                 DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor = Value;

                                 /* If may be possible for transmit     */
                                 /* queued data now.  So check to see if*/
                                 /* we have data to send.               */
                                 if((LEContextInfo.BytesToSend) && (Value == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE))
                                 {
                                    BSC_ScheduleAsynchronousCallback(BluetoothStackID, SPPLE_AsynchronousCallback, (unsigned long)DeviceInfo);
                                 }
                                 break;
                              case SPPLE_TX_CREDITS_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                 /* Client has sent updated credits.    */
                                 SPPLEReceiveCreditEvent(&LEContextInfo, DeviceInfo, Value);
                                 /* Check to see if we have data to     */
                                 /* send.                               */
                                 if((LEContextInfo.BytesToSend) && (DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE))
                                 {
                                    BSC_ScheduleAsynchronousCallback(BluetoothStackID, SPPLE_AsynchronousCallback, (unsigned long)DeviceInfo);
                                 }
                                 break;
                              case SPPLE_RX_CHARACTERISTIC_ATTRIBUTE_OFFSET:
                                 /* Client has sent data, so we should  */
                                 /* handle this as a data indication    */
                                 /* event.                              */
                                 SPPLEDataIndicationEvent(&LEContextInfo, DeviceInfo, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValueLength, GATT_ServerEventData->Event_Data.GATT_Write_Request_Data->AttributeValue);
                                 if((LEContextInfo.BytesToSend) && (DeviceInfo->ServerInfo.Tx_Client_Configuration_Descriptor == GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE))
                                 {
                                    BSC_ScheduleAsynchronousCallback(BluetoothStackID, SPPLE_AsynchronousCallback, (unsigned long)DeviceInfo);
                                 }
                                 break;
                              case SPPLE_RX_CREDITS_CHARACTERISTIC_CCD_ATTRIBUTE_OFFSET:
                                 /* Cache the previous CCD Value.       */
                                 PreviousValue = DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor;

                                 /* Note the updated Rx CCCD Value.     */
                                 DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor = Value;

                                 /* If we were not previously configured*/
                                 /* for notifications send the initial  */
                                 /* credits to the device.              */
                                 if(PreviousValue != GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                                 {
                                    /* Send the initial credits to the  */
                                    /* device.                          */
                                    SPPLESendCredits(&LEContextInfo, DeviceInfo, SPPLE_DATA_BUFFER_LENGTH);
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
                     DBG_PRINT("Error - No device info entry for this device.\n");
                  }

                  /* Release the List Mutex.                            */
                  BTPS_ReleaseMutex(DeviceListMutex);
               }
            }
            else
            {
               DBG_PRINT("Invalid Write Request Event Data.\n");
            }
         default:
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      DBG_PRINT("\n");

      DBG_PRINT("GATT Callback Data: Event_Data = NULL.\n");

      DBG_PRINT("\r\nSPPLE>");
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
   BoardStr_t BoardStr;

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
               DBG_PRINT("\nError Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RemoteDevice, BoardStr);
               DBG_PRINT("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionID);
               DBG_PRINT("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->TransactionID);
               DBG_PRINT("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               DBG_PRINT("   BD_ADDR:         %s.\n", BoardStr);
               DBG_PRINT("   Error Type:      %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)?"Response Error":"Response Timeout");
                /* Only print out the rest if it is valid.              */
               if(GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorType == retErrorResponse)
               {
                  DBG_PRINT("   Request Opcode:  0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestOpCode);
                  DBG_PRINT("   Request Handle:  0x%04X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->RequestHandle);
                  DBG_PRINT("   Error Code:      0x%02X.\n", GATT_Client_Event_Data->Event_Data.GATT_Request_Error_Data->ErrorCode);
               }
            }
            else
               DBG_PRINT("Error - Null Error Response Data.\n");
            break;
         case etGATT_Client_Exchange_MTU_Response:
            if(GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data)
            {
               DBG_PRINT("\nExchange MTU Response.\n");
               BD_ADDRToStr(GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->RemoteDevice, BoardStr);
               DBG_PRINT("   Connection ID:   %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ConnectionID);
               DBG_PRINT("   Transaction ID:  %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->TransactionID);
               DBG_PRINT("   Connection Type: %s.\n", (GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ConnectionType == gctLE)?"LE":"BR/EDR");
               DBG_PRINT("   BD_ADDR:         %s.\n", BoardStr);
               DBG_PRINT("   MTU:             %u.\n", GATT_Client_Event_Data->Event_Data.GATT_Exchange_MTU_Response_Data->ServerMTU);
            }
            else
               DBG_PRINT("\nError - Null Write Response Data.\n");
            break;
         default:
            break;
      }
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
   int           ret_val;
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
               DBG_PRINT("\netGATT_Connection_Device_Connection with size %u: \n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->RemoteDevice, BoardStr);
               DBG_PRINT("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID);
               DBG_PRINT("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               DBG_PRINT("   Remote Device:   %s.\n", BoardStr);
               DBG_PRINT("   GATT MTU:        %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU-3);

               /* Acquire the List Mutex.                               */
               if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
               {
                  /* Search for the device info for the connection.     */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(LEContextInfo.ConnectedBD_ADDR)) != NULL)
                  {
                     LEContextInfo.ConnectionID    = GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID;
                     LEContextInfo.MTU             = (Word_t)(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU-3);

                     /* Re-initialize the Transmit and Receive Buffers, */
                     /* as well as the transmit credits.                */
                     LEContextInfo.BufferFull      = FALSE;
                     LEContextInfo.TransmitCredits = 0;
                     LEContextInfo.CreditsToSend   = 0;
                     LEContextInfo.BytesToSend     = 0;
                     LEContextInfo.InIndex         = 0;
                     LEContextInfo.OutIndex        = 0;
                     SendIndex                     = 0;
                     if(SendBuffer)
                     {
                        if(SendBufferSize < LEContextInfo.MTU)
                        {
                           BTPS_FreeMemory(SendBuffer);
                           SendBuffer     = NULL;
                           SendBufferSize = 0;
                        }
                     }
                     if(!SendBuffer)
                     {
                        SendBuffer     = BTPS_AllocateMemory(LEContextInfo.MTU);
                        SendBufferSize = LEContextInfo.MTU;
                     }

                     if(ApplicationFlags & APPLICATION_FLAG_ENABLE_LE_CENTRAL)
                     {
                        /* Attempt to update the MTU to the maximum  */
                        /* supported.                                */
                        ret_val = GATT_Exchange_MTU_Request(BluetoothStackID, GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->ConnectionID, ATT_PROTOCOL_MTU_MAXIMUM, GATT_ClientEventCallback, 0);
                        if(ret_val < 0)
                           DBG_PRINT("Failed to set MTU %d\n", ret_val);
                     }

                     /* Check to see if we are bonded and the Tx Credit */
                     /* notifications have been enabled.                */
                     if(DeviceInfo->Flags & DEVICE_INFO_FLAG_LTK_VALID)
                     {
                        if(DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor & GATT_CLIENT_CONFIGURATION_CHARACTERISTIC_NOTIFY_ENABLE)
                        {
                           /* Send the Initial Credits if the Rx Credit */
                           /* CCD is already configured (for a bonded   */
                           /* device this could be the case).           */
                           SPPLESendCredits(&LEContextInfo, DeviceInfo, SPPLE_DATA_BUFFER_LENGTH);
                        }
                     }
                     else
                     {
                        DeviceInfo->ServerInfo.Rx_Credit_Client_Configuration_Descriptor = 0;
                     }
                  }

                  /* Release the List Mutex.                            */
                  BTPS_ReleaseMutex(DeviceListMutex);
               }
            }
            else
            {
               DBG_PRINT("Error - Null Connection Data.\n");
            }
            break;
         case etGATT_Connection_Device_Disconnection:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data)
            {
               DBG_PRINT("\netGATT_Connection_Device_Disconnection with size %u: \n", GATT_Connection_Event_Data->Event_Data_Size);
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->RemoteDevice, BoardStr);
               DBG_PRINT("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionID);
               DBG_PRINT("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Disconnection_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               DBG_PRINT("   Remote Device:   %s.\n", BoardStr);
            }
            else
            {
               DBG_PRINT("Error - Null Disconnection Data.\n");
            }
            break;
         case etGATT_Connection_Device_Connection_MTU_Update:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data)
            {
               DBG_PRINT("\r\etGATT_Connection_Device_Connection_MTU_Update\n");
               BD_ADDRToStr(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->RemoteDevice, BoardStr);
               DBG_PRINT("   Connection ID:   %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->ConnectionID);
               DBG_PRINT("   Connection Type: %s.\n", ((GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_MTU_Update_Data->ConnectionType == gctLE)?"LE":"BR/EDR"));
               DBG_PRINT("   Remote Device:   %s.\n", BoardStr);
               DBG_PRINT("   GATT MTU:        %u.\n", GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU-3);

               LEContextInfo.MTU = (Word_t)(GATT_Connection_Event_Data->Event_Data.GATT_Device_Connection_Data->MTU-3);

               /* If the MTU is larger, than we need to make sure we    */
               /* allocate a buffer for sending loopback data that is   */
               /* large enough.                                         */
               if(SendBuffer)
               {
                  if(SendBufferSize < LEContextInfo.MTU)
                  {
                     BTPS_FreeMemory(SendBuffer);

                     SendBuffer     = NULL;
                     SendBufferSize = 0;
                  }
               }

               if(!SendBuffer)
               {
                  SendBuffer     = BTPS_AllocateMemory(LEContextInfo.MTU);
                  SendBufferSize = LEContextInfo.MTU;
               }
            }
            break;
         case etGATT_Connection_Device_Buffer_Empty:
            if(GATT_Connection_Event_Data->Event_Data.GATT_Device_Buffer_Empty_Data)
            {
               /* Acquire the List Mutex.                               */
               if(BTPS_WaitMutex(DeviceListMutex, BTPS_INFINITE_WAIT))
               {
                  /* Grab the device info for the currently connected   */
                  /* device.                                            */
                  if((DeviceInfo = SearchDeviceInfoEntryByBD_ADDR(LEContextInfo.ConnectedBD_ADDR)) != NULL)
                  {
                     if(LEContextInfo.BufferFull)
                     {
                        /* Flag that the buffer is no longer empty.     */
                        LEContextInfo.BufferFull = FALSE;

                        /* Attempt to send any queued credits that we   */
                        /* may have.                                    */
                        SPPLESendCredits(&LEContextInfo, DeviceInfo, 0);

                        /* If may be possible for transmit queued data  */
                        /* now.  So fake a Receive Credit event with 0  */
                        /* as the received credits.                     */
                        SPPLEReceiveCreditEvent(&LEContextInfo, DeviceInfo, 0);

                        /* Suppress the command prompt.                 */
                        SuppressResponse   = TRUE;
                     }
                  }

                  /* Release the List Mutex.                            */
                  BTPS_ReleaseMutex(DeviceListMutex);
               }
            }
            break;
        default:
            break;
      }

      /* Print the command line prompt.                                 */
      if(!SuppressResponse)
      {
         DBG_PRINT("\r\nSPPLE>");
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      DBG_PRINT("\n");

      DBG_PRINT("GATT Connection Callback Data: Event_Data = NULL.\n");

      DBG_PRINT("\r\nSPPLE>");
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
   BoardStr_t                        Callback_BoardStr;
   PIN_Code_t                        PINCode;
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
         case etAuthentication:
            /* An authentication event occurred, determine which type of*/
            /* authentication event occurred.                           */
            switch(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->GAP_Authentication_Event_Type)
            {
               case atLinkKeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\n");
                  DBG_PRINT("atLinkKeyRequest: %s\n", Callback_BoardStr);

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
                  {
                     DBG_PRINT("GAP_Authentication_Response Success");
                  }
                  else
                  {
                     DBG_PRINT("GAP_Authentication_Response %d\n", Result);
                  }
                  break;
               case atPINCodeRequest:
                  /* A pin code request event occurred, first display   */
                  /* the BD_ADD of the remote device requesting the pin.*/
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\n");
                  DBG_PRINT("atPINCodeRequest: %s\n", Callback_BoardStr);

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the PIN Code.                                      */
                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Initialize the PIN code.                                 */
                  ASSIGN_PIN_CODE(PINCode, '0', '0', '0', '0', 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0);

                  /* Populate the response structure.                         */
                  GAP_Authentication_Information.GAP_Authentication_Type      = atPINCode;
                  GAP_Authentication_Information.Authentication_Data_Length   = 4;
                  GAP_Authentication_Information.Authentication_Data.PIN_Code = PINCode;

                  /* Submit the Authentication Response.                      */
                  GAP_Authentication_Response(BluetoothStackID, CurrentCBRemoteBD_ADDR, &GAP_Authentication_Information);
                  DBG_PRINT("PINCodeResponse 0000\n");
                  break;
               case atLinkKeyCreation:
                  /* A link key creation event occurred, first display  */
                  /* the remote device that caused this event.          */
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\n");
                  DBG_PRINT("atLinkKeyCreation: %s\n", Callback_BoardStr);

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

                     DBG_PRINT("Link Key Stored %d.\n", Index);
                  }
                  else
                  {
                     DBG_PRINT("Link Key array full.\n");
                  }
                  break;
               case atIOCapabilityRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\n");
                  DBG_PRINT("atIOCapabilityRequest: %s\n", Callback_BoardStr);

                  /* Setup the Authentication Information Response      */
                  /* structure.                                         */
                  GAP_Authentication_Information.GAP_Authentication_Type                                      = atIOCapabilities;
                  GAP_Authentication_Information.Authentication_Data_Length                                   = sizeof(GAP_IO_Capabilities_t);
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.IO_Capability            = (GAP_IO_Capability_t)IOCapability;
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.MITM_Protection_Required = FALSE;
                  GAP_Authentication_Information.Authentication_Data.IO_Capabilities.OOB_Data_Present         = FALSE;

                  /* Submit the Authentication Response.                */
                  Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                  /* Check the result of the submitted command.         */
                  /* Check the result of the submitted command.         */
                  if(!Result)
                  {
                     DBG_PRINT("Auth Success");
                  }
                  else
                  {
                     DBG_PRINT("Auth %d\n", Result);
                  }
                  break;
               case atIOCapabilityResponse:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\natIOCapabilityResponse: %s\n", Callback_BoardStr);
                  break;
               case atUserConfirmationRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\n");
                  DBG_PRINT("atUserConfirmationRequest: %s\n", Callback_BoardStr);

                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  if(IOCapability != icDisplayYesNo)
                  {
                     /* Invoke JUST Works Process...                    */
                     GAP_Authentication_Information.GAP_Authentication_Type          = atUserConfirmation;
                     GAP_Authentication_Information.Authentication_Data_Length       = (Byte_t)sizeof(Byte_t);
                     GAP_Authentication_Information.Authentication_Data.Confirmation = TRUE;

                     /* Submit the Authentication Response.             */
                     DBG_PRINT("\nAuto Accepting: %lu\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value);

                     Result = GAP_Authentication_Response(BluetoothStackID, GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, &GAP_Authentication_Information);

                     if(!Result)
                     {
                        DBG_PRINT("GAP_Authentication_Response Success");
                     }
                     else
                     {
                        DBG_PRINT("GAP_Authentication_Response %d\n", Result);
                     }

                     /* Flag that there is no longer a current          */
                     /* Authentication procedure in progress.           */
                     ASSIGN_BD_ADDR(CurrentCBRemoteBD_ADDR, 0x00, 0x00, 0x00, 0x00, 0x00, 0x00);
                  }
                  else
                  {
                     DBG_PRINT("User Confirmation: %lu\n", (unsigned long)GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value);

                     /* Inform the user that they will need to respond  */
                     /* with a PIN Code Response.                       */
                     DBG_PRINT("Respond with: UserConfirmationResponse\n");
                  }
                  break;
               case atPasskeyRequest:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\n");
                  DBG_PRINT("atPasskeyRequest: %s\n", Callback_BoardStr);

                  /* Note the current Remote BD_ADDR that is requesting */
                  /* the Passkey.                                       */
                  CurrentCBRemoteBD_ADDR = GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device;

                  /* Inform the user that they will need to respond with*/
                  /* a Passkey Response.                                */
                  DBG_PRINT("Respond with: PassKeyResponse\n");
                  break;
               case atPasskeyNotification:
                  BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Remote_Device, Callback_BoardStr);
                  DBG_PRINT("\n");
                  DBG_PRINT("atPasskeyNotification: %s\n", Callback_BoardStr);

                  DBG_PRINT("Passkey Value: %lu\n", GAP_Event_Data->Event_Data.GAP_Authentication_Event_Data->Authentication_Event_Data.Numeric_Value);
                  break;
               default:
                  DBG_PRINT("Un-handled Auth. Event.\n");
                  break;
            }
            break;
         case etEncryption_Change_Result:
            BD_ADDRToStr(GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Remote_Device, Callback_BoardStr);
            DBG_PRINT("\netEncryption_Change_Result for %s, Status: 0x%02X, Mode: %s.\n", Callback_BoardStr,
                                                                                           GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Encryption_Change_Status,
                                                                                           ((GAP_Event_Data->Event_Data.GAP_Encryption_Mode_Event_Data->Encryption_Mode == emDisabled)?"Disabled": "Enabled"));
            break;
         default:
            /* An unknown/unexpected GAP event was received.            */
            DBG_PRINT("\nUnknown Event: %d.\n", GAP_Event_Data->Event_Data_Type);
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      DBG_PRINT("\n");
      DBG_PRINT("Null Event\n");
   }

   DBG_PRINT("\r\nSPPLE>");
}

   /* The following function is responsible for processing HCI Mode     */
   /* change events.                                                    */
static void BTPSAPI HCI_Event_Callback(unsigned int BluetoothStackID, HCI_Event_Data_t *HCI_Event_Data, unsigned long CallbackParameter)
{
   unsigned int              Int;
   HCI_LE_Meta_Event_Data_t *MetaData;

   /* Make sure that the input parameters that were passed to us are    */
   /* semi-valid.                                                       */
   if((BluetoothStackID) && (HCI_Event_Data))
   {
      /* Process the Event Data.                                        */
      switch(HCI_Event_Data->Event_Data_Type)
      {
         case etConnection_Complete_Event:
            /* Check to see if the connection was a success.            */
            if(HCI_Event_Data->Event_Data.HCI_Connection_Complete_Event_Data->Status == HCI_ERROR_CODE_NO_ERROR)
            {
               /* Check to see if LE is enaabled.                       */
               if(ApplicationFlags & APPLICATION_FLAG_ENABLE_SPPLE)
               {
                  /* Check to see if we should disable advertising.     */
                  if(!(ApplicationFlags & APPLICATION_FLAG_SIMULTANEOUS_LE_CLASSIC))
                  {
                     /* Disable advertising.                            */
                     AdvertiseLE(0);
                     DBG_PRINT("Disable Advertising\n");
                  }
               }

               /* Make ourselves Non-Discoverable and Non-Connectable.  */
               GAP_Set_Discoverability_Mode(BluetoothStackID, dmNonDiscoverableMode, 0);
               GAP_Set_Connectability_Mode(BluetoothStackID, cmNonConnectableMode);
            }
            break;
         case etDisconnection_Complete_Event:
            if(ApplicationFlags & APPLICATION_FLAG_SHUTDOWN_IN_PROGRESS)
            {
               BTPS_SetEvent(ShutdownEvent);
            }
            else
            {
               /* Check to see if LE is enaabled.                       */
               if(ApplicationFlags & APPLICATION_FLAG_ENABLE_SPPLE)
               {
                  if(ApplicationFlags & APPLICATION_FLAG_ENABLE_LE_CENTRAL)
                  {
                     ScanLE(1);
                     PRINTF("Start Scanning\n");
                  }
                  else
                  {
                     AdvertiseLE(1);
                     PRINTF("Enable Advertising\n");
                  }
               }
               /* Check to see if SPP is enabled.                          */
               if(ApplicationFlags & APPLICATION_FLAG_ENABLE_SPP)
               {
                  GAP_Set_Discoverability_Mode(BluetoothStackID, dmGeneralDiscoverableMode, 0);
                  GAP_Set_Connectability_Mode(BluetoothStackID, cmConnectableMode);
                  DBG_PRINT("Enable Classic\n");
               }
            }
            break;
         case etLE_Meta_Event:
            MetaData = HCI_Event_Data->Event_Data.HCI_LE_Meta_Event_Data;
            if(MetaData->LE_Event_Data_Type == meConnection_Complete_Event)
            {
               Int = (MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Conn_Interval*125);
               DBG_PRINT("LE Connection Complete\n");
               DBG_PRINT("   ConnInterval: %d.%d ms\n", (Int/100), ((Int % 100)/10));
               DBG_PRINT("   ConnLatency:  %d\n", MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Conn_Latency);
               DBG_PRINT("   SuperTimeout: %d\n", MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Supervision_Timeout);
               DBG_PRINT("   Role:         %s\n", (char *)((MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Role)?"Slave":"Master"));
               DBG_PRINT("   Status:       %d\n", MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Status);

               ApplicationFlags &= ~APPLICATION_FLAG_ADVERTISING_ENABLED;

               if(MetaData->Event_Data.HCI_LE_Connection_Complete_Event_Data.Status == HCI_ERROR_CODE_NO_ERROR)
               {
                  /* Check to see if LE is enaabled.                       */
                  if(ApplicationFlags & APPLICATION_FLAG_ENABLE_SPP)
                  {
                     /* Check to see if we should disable advertising.     */
                     if(!(ApplicationFlags & APPLICATION_FLAG_SIMULTANEOUS_LE_CLASSIC))
                     {
                        GAP_Set_Discoverability_Mode(BluetoothStackID, dmNonDiscoverableMode, 0);
                        GAP_Set_Connectability_Mode(BluetoothStackID, cmNonConnectableMode);
                        DBG_PRINT("Disable Classic\n");
                     }
                  }
               }
            }
            if(MetaData->LE_Event_Data_Type == meConnection_Update_Complete_Event)
            {
               Int = (MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Conn_Interval*125);
               DBG_PRINT("LE Connection Update\n");
               DBG_PRINT("   ConnInterval: %d.%d ms\n", (Int/100), ((Int % 100)/10));
               DBG_PRINT("   ConnLatency:  %d\n", MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Conn_Latency);
               DBG_PRINT("   SuperTimeout: %d\n", MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Supervision_Timeout);
               DBG_PRINT("   Status:       %d\n", MetaData->Event_Data.HCI_LE_Connection_Update_Complete_Event_Data.Status);
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
   int        ret_val;
   int        Length;
   int        DataLength;
   Word_t     ConnectionHandle;
   Boolean_t  _DisplayPrompt = TRUE;
   BoardStr_t BoardStr;

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
            BD_ADDRToStr(SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->BD_ADDR, BoardStr);

            DBG_PRINT("\nSPP Open Indication, ID: 0x%04X, Board: %s.\n", SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->SerialPortID, BoardStr);

            /* Flag that we are connected to the device.             */
            SPPContextInfo.Connected = TRUE;
            SPPContextInfo.BD_ADDR   = SPP_Event_Data->Event_Data.SPP_Open_Port_Indication_Data->BD_ADDR;

            /* Query the connection handle.                          */
            ret_val = GAP_Query_Connection_Handle(BluetoothStackID, SPPContextInfo.BD_ADDR, &ConnectionHandle);
            if(!ret_val)
            {
               /* Save the connection handle of this connection.     */
               SPPContextInfo.Connection_Handle = ConnectionHandle;
            }
            break;
         case etPort_Close_Port_Indication:
            DBG_PRINT("\nSPP Close Port\n");

            ASSIGN_BD_ADDR(SPPContextInfo.BD_ADDR, 0, 0, 0, 0, 0, 0);
            SPPContextInfo.Connected = FALSE;
            break;
         case etPort_Data_Indication:
            DBG_PRINT("\nSPP Data Indication\n");
            /* Loop through and read all data that is present in the    */
            /* buffer.                                                  */
            DataLength = SPP_Event_Data->Event_Data.SPP_Data_Indication_Data->DataLength;
            while((DataLength) || (SPPContextInfo.BufferLength))
            {
               if(SPPContextInfo.BufferLength)
               {
                  ret_val = SPP_Data_Write(BluetoothStackID, SPPContextInfo.SerialPortID, (Word_t)SPPContextInfo.BufferLength, SPPContextInfo.Buffer);
                  if(ret_val > 0)
                  {
                     if(ret_val < (int)SPPContextInfo.BufferLength)
                     {
                        Length = (SPPContextInfo.BufferLength-ret_val);
                        if(Length)
                        {
                           BTPS_MemCopy(SPPContextInfo.Buffer, &SPPContextInfo.Buffer[ret_val], Length);
                        }
                     }
                     SPPContextInfo.BufferLength -= ret_val;
                  }
               }

               if(DataLength)
               {
                  Length = (int)(MAXIMUM_SPP_BUFFER_SIZE-SPPContextInfo.BufferLength);
                  if(Length > 0)
                  {
                     /* Read as much data as possible.                  */
                     ret_val = SPP_Data_Read(BluetoothStackID, SPPContextInfo.SerialPortID, (Word_t)Length, (Byte_t *)&SPPContextInfo.Buffer[SPPContextInfo.BufferLength]);
                     if(ret_val > 0)
                     {
                        DataLength                  -= ret_val;
                        SPPContextInfo.BufferLength += ret_val;
                     }
                  }
               }
            }

            _DisplayPrompt = FALSE;

            break;
         case etPort_Transmit_Buffer_Empty_Indication:
            DBG_PRINT("\etPort_Transmit_Buffer_Empty_Indication.\n");
            break;
         default:
            /* An unknown/unexpected SPP event was received.            */
            DBG_PRINT("\nUnknown Event.\n");
            break;
      }
   }
   else
   {
      /* There was an error with one or more of the input parameters.   */
      DBG_PRINT("\nNull Event\n");
   }

   if(_DisplayPrompt)
   {
      DBG_PRINT("\r\nSPPLE>");
   }
}

   /* ***************************************************************** */
   /*                    End of Event Callbacks.                        */
   /* ***************************************************************** */
int InitBluetooth(Byte_t InitFlags)
{
   int                     ret_val;
   VendParams_t            VendParams;
   HCI_DriverInformation_t HCI_DriverInformation;
#ifdef SDK_50

   A_UINT16        trim_value;
   A_UINT8         bd_addr[6];

#endif

   /* Setup BT_RESET GPIO                                               */
   *((unsigned int *)0x14010) = (unsigned int)(1 << 14);

   /* Perform a Hardware Reset.                                         */
   *((unsigned int *)0x14008) = (unsigned int)(1 << 14);
   BTPS_Delay(10);
   *((unsigned int *)0x14004) = (unsigned int)(1 << 14);
   BTPS_Delay(10);

   BTPS_MemInitialize(&VendParams, 0, sizeof(VendParams_t));
#ifdef SDK_50

      BTPS_MemInitialize(bd_addr, 0, 6);
      trim_value = 0;

      if(qcom_bluetooth_get_otp_mac(6, bd_addr) == A_OK)
         ASSIGN_BD_ADDR(VendParams.BD_ADDR, bd_addr[0], bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5]);

      if(qcom_bluetooth_get_otp_trim(&trim_value) == A_OK)
         VendParams.TrimValue = trim_value;

      //A_PRINTF("bt mac is %02x:%02x:%02x:%02x:%02x:%02x, trim is %d\n", bd_addr[0],
       //     bd_addr[1], bd_addr[2], bd_addr[3], bd_addr[4], bd_addr[5], trim_value);

#else
   /* Initialize the Vendor Parameters.                                 */

   VendParams.BD_ADDR.BD_ADDR0 = (Byte_t)(LAPL & 0xff);
   VendParams.BD_ADDR.BD_ADDR1 = (Byte_t)((LAPL >> 8) & 0xff);
   VendParams.BD_ADDR.BD_ADDR2 = (Byte_t)(LAPU & 0xff);
   VendParams.BD_ADDR.BD_ADDR3 = 0x5B; // UAP
   VendParams.BD_ADDR.BD_ADDR4 = 0x02; // NAP
   VendParams.BD_ADDR.BD_ADDR5 = 0x00; // NAP

#endif

   VendParams.CoExMode      = CoExMode;

   HCI_VS_SetParams(VendParams);

   /* The Transport selected was USB, setup the Driver Information      */
   /* Structure to use USB has the HCI Transport.                       */
   HCI_DRIVER_SET_COMM_INFORMATION(&HCI_DriverInformation, 1, 115200, cpH4DS);

   /* Try to Open the stack and check if it was successful.             */
   if(!OpenStack(&HCI_DriverInformation))
   {
      /* Attempt to register a HCI Event Callback.                      */
      ret_val = HCI_Register_Event_Callback(BluetoothStackID, HCI_Event_Callback, (unsigned long)NULL);
      if(ret_val > 0)
      {
         /* Initialize the return value.                                */
         ret_val = 0;

         /* Initialize the Context Information.                            */
         ApplicationFlags = InitFlags;
         BTPS_MemInitialize(&SPPContextInfo, 0, sizeof(SPP_Context_Info_t));
         BTPS_MemInitialize(&LEContextInfo, 0, sizeof(LE_Context_Info_t));

         /* Initialize the selected services.                              */
         if(ApplicationFlags & APPLICATION_FLAG_ENABLE_SPP)
         {
            ret_val = RegisterSPP();
            if(!ret_val)
            {
               GAP_Set_Discoverability_Mode(BluetoothStackID, dmGeneralDiscoverableMode, 0);
               GAP_Set_Connectability_Mode(BluetoothStackID, cmConnectableMode);
               GAP_Register_Remote_Authentication(BluetoothStackID, GAP_Event_Callback, (unsigned long)0);
//               GAP_Set_Pairability_Mode(BluetoothStackID, pmPairableMode);
               GAP_Set_Pairability_Mode(BluetoothStackID, pmPairableMode_EnableSecureSimplePairing);
            }
         }
         else
         {
            GAP_Set_Discoverability_Mode(BluetoothStackID, dmNonDiscoverableMode, 0);
            GAP_Set_Connectability_Mode(BluetoothStackID, cmNonConnectableMode);
         }

         /* Initialize the selected services.                              */
         if((!ret_val) && (ApplicationFlags & APPLICATION_FLAG_ENABLE_SPPLE))
         {
            ret_val = RegisterSPPLE();
            if(!ret_val)
            {
               /* The device has been set to pairable mode, now register*/
               /* an Authentication Callback to handle the              */
               /* Authentication events if required.                    */
               GAP_LE_Register_Remote_Authentication(BluetoothStackID, GAP_LE_Event_Callback, (unsigned long)0);
               GAP_LE_Set_Pairability_Mode(BluetoothStackID, lpmPairableMode);
               LE_Parameters.ConnectableMode     = lcmConnectable;
               LE_Parameters.DiscoverabilityMode = dmGeneralDiscoverableMode;

               ShutdownEvent = BTPS_CreateEvent(FALSE);
               if(ApplicationFlags & APPLICATION_FLAG_ENABLE_LE_CENTRAL)
                  ScanLE(1);
               else
                  AdvertiseLE(1);
            }
         }
      }
   }
   else
   {
      /* There was an error while attempting to open the Stack.         */
      DBG_PRINT("Unable to open the stack.\n");
      ret_val = -1;
   }

   return(ret_val);
}

extern A_UINT8 currentDeviceId;

void CloseBluetooth(void)
{
   if(SPPContextInfo.Connected)
   {
      ApplicationFlags |= APPLICATION_FLAG_SHUTDOWN_IN_PROGRESS;
      GAP_Disconnect_Link(BluetoothStackID, SPPContextInfo.BD_ADDR);
      SPPContextInfo.Connected = FALSE;
      BTPS_WaitEvent(ShutdownEvent, 2000);
   }

   if(LEContextInfo.ConnectionID)
   {
      ApplicationFlags |= APPLICATION_FLAG_SHUTDOWN_IN_PROGRESS;
      GAP_LE_Disconnect(BluetoothStackID, LEContextInfo.ConnectedBD_ADDR);
      LEContextInfo.ConnectionID = 0;
      BTPS_WaitEvent(ShutdownEvent, 2000);
   }

   if(ApplicationFlags & APPLICATION_FLAG_ADVERTISING_ENABLED)
      AdvertiseLE(0);
   if(SPPLEServiceID)
     UnregisterSPPLE();

   /* Close the Bluetooth Stack.                                     */
   CloseStack();

   /* Setup BT_RESET GPIO                                               */
   *((unsigned int *)0x14010) = (unsigned int)(1 << 14);

   /* Perform a Hardware Reset.                                         */
   *((unsigned int *)0x14008) = (unsigned int)(1 << 14);
}

   /* Main Program Entry Point.                                         */
int bluetooth_server_handler(int argc, char *argv[])
{
   int            ret_val = -1;
   DWord_t        LAP     = 0;
   unsigned char  Flags   = 0;
   long long      Target;
   

   if (argc >= 3 && !strcmp(argv[1], "prio"))
   {
      int tmp = str2int(argv[2]);
      if(tmp >= 0 && tmp <= 2)
         CoexPrio = tmp;
      else
      {
         DBG_PRINT("Coex Priority is 0~2, 0 disable the CSR8x11 BT_STATUS, 1 enable BT_STATUS but low priority, 2 enable BT_STATUS but some high priority\n");
      }
      return;
   }

   /* Read any Flags that have been specified.                          */
   if(argc >= 2)
   {
      Flags = (unsigned char)str2int(argv[1]);
   }
   /* Verify that there is enough input for the Flags specified.        */
   if((argc == 1) || ((!(Flags & APPLICATION_FLAG_TEST_MODE_VALUE)) && (Flags & APPLICATION_FLAG_ENABLE_LE_CENTRAL) && (argc < 6)))
   {
      PRINTF("bluetooth [Mask] [CoExMode] [Bluetooth UAP_NAP] <[Scan Mode] [Target Address]>\n");
      PRINTF("    where Mask : 0x00 - Disable Bluetooth\n");
      PRINTF("                 0x01 - Enable Classic\n");
      PRINTF("                 0x02 - Enable LE\n");
      PRINTF("                 0x04 - Simultaneous Classic/LE\n");
      PRINTF("                 0x08 - Enable Debug Prints\n");
      PRINTF("                 0x10 - Enable LE Central Mode\n");
      PRINTF("      CoExMode : 0-11\n");
      PRINTF("      UAP/NAP  : 0x000000-0xFFFFFF\n\n");
      PRINTF("   if 'Enable LE Central Mode' is defined then\n");
      PRINTF("   Scan Mode and Target Address are required.\n\n");
      PRINTF("     Scan  Mode: 0/1 - (Passive Scan - 0, Active Scan - 1)\n");
      PRINTF(" Target Address: 0x000000000000-0xFFFFFFFFFFFF\n");
      PRINTF("bluetooth [prio] [CoexPrio]\n");
      PRINTF("    where prio : 0x00 - Disable BT PRIORITY\n");
      PRINTF("                 0x01 - Enable  BT Low  Priority\n");
      PRINTF("                 0x02 - Enable  BT High Priority\n");
   }
   else
   {
      CoExMode = 0;
      LAPU     = 0;
      LAPL     = 0;

      /* Read the parameters passed in on the command line.             */
      if(argc >= 3)
         CoExMode = (Word_t)str2int(argv[2]);
      if(argc >= 4)
      {
         LAP  = (DWord_t)str2int(argv[3]);
         LAPU = (Word_t)((LAP >> 16) & 0x00FF);
         LAPL = (Word_t)(LAP & 0xFFFF);
      }
      if(argc >= 5)
      {
         ret_val = (int)str2int(argv[4]);
         if((ret_val == 0) || (ret_val == 1))
            ScanType = (GAP_LE_Scan_Type_t)ret_val;
         else
            ScanType = stPassive;
      }
      if(argc >= 6)
      {
         Target                 = str2int(argv[5]);
         TargetBD_ADDR.BD_ADDR0 = (unsigned char)(Target & 0xFF);
         TargetBD_ADDR.BD_ADDR1 = (unsigned char)((Target >> 8) & 0xFF);
         TargetBD_ADDR.BD_ADDR2 = (unsigned char)((Target >> 16) & 0xFF);
         TargetBD_ADDR.BD_ADDR3 = (unsigned char)((Target >> 24) & 0xFF);
         TargetBD_ADDR.BD_ADDR4 = (unsigned char)((Target >> 32) & 0xFF);
         TargetBD_ADDR.BD_ADDR5 = (unsigned char)((Target >> 40) & 0xFF);
      }
      if(!Flags)
         CloseBluetooth();
      else
      {
         if(Flags & (APPLICATION_FLAG_ENABLE_SPP | APPLICATION_FLAG_ENABLE_SPPLE))
         {
            /* Check if Debug messages are enabled.                     */
            if(Flags & APPLICATION_FLAG_ENABLE_DEBUG_PRINTS)
               dbg_printf = 1;
            else
               dbg_printf = 0;

            if(CoExMode <= 11)
            {
               if(LAP <= 0xFFFFFF)
               {
                  /* Check to see if we are to enter into a power test  */
                  /* mode.                                              */
                  if(Flags & APPLICATION_FLAG_TEST_MODE_VALUE)
                  {
                     if(Flags == 255)
                     {
                        PRINTF("Enter Test Mode: No Bluetooth Activity\n");
                        LAPU     = 0xFA;
                        LAPL     = 0xCADE;
                        CoExMode = 0;
                        Flags    = 0;
                     }
                     if(Flags == 254)
                     {
                        PRINTF("Enter Test Mode: Release Bluetooth Reset\n");
                        PRINTF("Enter Test Mode: Disable Ruby Power Management\n");

                        /* Release Reset to 8311.                       */
                        *((unsigned int *)0x14010) = (unsigned int)(1 << 14);
                        *((unsigned int *)0x14004) = (unsigned int)(1 << 14);

                        qcom_power_set_mode(currentDeviceId, MAX_PERF_POWER);
                     }
                     if(Flags == 253)
                     {
                        PRINTF("Enter Test Mode: Release Bluetooth Reset\n");
                        PRINTF("Enter Test Mode: Retain Ruby Power Management\n");

                        /* Release Reset to 8311.                       */
                        *((unsigned int *)0x14010) = (unsigned int)(1 << 14);
                        *((unsigned int *)0x14004) = (unsigned int)(1 << 14);
                     }
                  }

                  if (!(Flags & APPLICATION_FLAG_TEST_MODE_VALUE))
                  {
                     A_UINT32 power_mode;

                     qcom_power_get_mode(currentDeviceId, &power_mode);

                     if (power_mode != MAX_PERF_POWER)
                     {
                        /* XXX WAR for UART thread not waking up: disable     */
                        /* Power Management                                   */
                        qcom_power_set_mode(currentDeviceId, MAX_PERF_POWER);
                     }

                     ret_val = InitBluetooth(Flags);
                     PRINTF("ret_val %d\n", ret_val);
                     if (ret_val)
                     {
                        CloseBluetooth();
                        ret_val = 0;
                     }

                     if (power_mode != MAX_PERF_POWER)
                     {
                        qcom_power_set_mode(currentDeviceId, REC_POWER);
                        /* Enable BT UART Rx wakeup, so following rx packet */
                        /* which carries break signal could wake cpu from   */
                        /* deep sleep                                       */
                        qcom_uart_wakeup_config(BT_UART_NAME, 0, 1);
                        /* By default soc will sleep in 91.5us in power save mode.    */
                        /* HCI command response (about 500us, no break signal)        */
                        /* will be blocked                                            */
                        qcom_set_keep_awake_time(8);
                     }
                  }
               }
               else
                  PRINTF("Invalid Bluetooth UAP/NAP\n");
            }
            else
               PRINTF("Invalid CoEx\n");
         }
         else
            PRINTF("Invalid Flags\n");
      }
   }

   return 0;
}

typedef struct
{
    A_UINT32 firstTime;    /* ms first packet received */
    A_UINT32 lastTime;     /* ms last packet received */
    A_UINT32 bytes;        /* total data length % 1024 */
    A_UINT32 kbytes;       /* total data length / 1024 */
    A_UINT8  notFirst;     /* first : 0, following : 1 */
} STREAM_CALC_t;

static STREAM_CALC_t calc;

/**
 * [SPPLEDataMetaRecord] : record the incoming data length
 * @param dataLength payload data length
 */
static void SPPLEDataMetaRecord(int dataLength)
{
   //if this is the first packet
   if(!calc.notFirst)
   {
      calc = (STREAM_CALC_t){0};
      calc.notFirst = 1;
      calc.firstTime = time_ms();
   }
   else
   {
      calc.bytes += dataLength;
      if (0 != calc.bytes / 1024) {
        calc.kbytes += calc.bytes / 1024;
        calc.bytes = calc.bytes % 1024;
      }

      calc.lastTime = time_ms();
   }
}

/**
 * [SPPLEDataMetaRecord]: Print summary the rx data rate and print the report 
 */
static void SPPLEDataMetaRecordPrint(void)
{
   A_UINT32 throughput = 0;
   A_UINT32 msInterval = 0;
   A_UINT32 sInterval  = 0;   //seconds interval
   msInterval = (calc.lastTime - calc.firstTime);
   sInterval  = msInterval / 1000;
   if (msInterval > 0)
   {
      if (calc.kbytes < 0x7FFFF)  /*prevent overflow*/
      {
         throughput =
            ((calc.kbytes * 1024 * 8) / (msInterval)) +
            ((calc.bytes * 8) / (msInterval));
      }
      else
      {
         throughput = ((calc.kbytes * 8) / (sInterval))
                      + ((calc.bytes * 8 / 1024) / (sInterval));
      }
   }
   else
   {
      throughput = 0;
   }
   calc.notFirst = 0;
   DBG_PRINT("\t%d Bytes in %d seconds %d ms  \n", calc.kbytes * 1024 + calc.bytes,
             msInterval / 1000, msInterval % 1000);
   DBG_PRINT("\t throughput %d Kb/sec\n", throughput);
}

/**
 * [SetCSR8x11Pskey set the PSKey for CSR8x11 controller ]
 * @param BluetoothStackID [Bluetooth StackID]
 * @param pskey            [PSKey ID]
 * @param buffer           [PSKey value]
 * @param len              [PSKey length in word]
 */
void SetCSR8x11Pskey(unsigned int BluetoothStackID, Word_t pskey, Word_t* buffer, Byte_t len)
{
    if (BluetoothStackID && buffer && len)
    {
        Word_t status = -1;
        int ret = -1;
        ret = HCI_VS_SetPSKey(BluetoothStackID, pskey, len, buffer, &status, FALSE);
        DBG_PRINT("PSKey(0x%x) set %s, status is %d\n", pskey, ret ? "failed" : "successful", status);
    }
}

/**
 * [GetCSR8x11Pskey get the value of specified PSKey ID for CSR8x11 controller]
 * @param BluetoothStackID [Bluetooth StackID]
 * @param pskey            [PSKey ID]
 */
void GetCSR8x11Pskey(unsigned int BluetoothStackID, Word_t pskey)
{
    if (BluetoothStackID)
    {
        Word_t buffer[20] = {0}; //modify me if the pskey length is large than 20
        int ret = 0, i = 0;
        BCCMD_Cmd_Result_t result =
        {
            20, buffer,  1
        };
        ret = HCI_VS_GetPSKey(BluetoothStackID, (Word_t)pskey, &result);
        DBG_PRINT("PSKey(%x) get %s, result is %u, value len is %u\n", pskey, ret ? "failed" : "successful", result.ResultStatus, result.ResultBufferLength);
        if (!ret)
        {
            for (; i < result.ResultBufferLength; i++)
            {
                DBG_PRINT("%x,", result.ResultBuffer[i]);
            }
            DBG_PRINT("\n");
        }
    }
}

/**
 * [CSR8x11PatchCallback callback used to customize the pskeys for user]
 * @param BluetoothStackID  [Bluetooth StackID]
 * @param ControllerID      [Controller ID]
 * @param CallbackParameter [Callback parameter]
 */
void CSR8x11PatchCallback(unsigned int BluetoothStackID, unsigned int ControllerID, unsigned long CallbackParameter)
{
    Word_t cmdBuffer[16] = {0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 0, 1};
    Word_t cmdBuffer2[2] = {3, 1};
#define PSKEY_COEX_PIO_UNITY_3_BT_STATUS          0x2484
#define PSKEY_COEX_BLE_TRANSACTION_PRIORITY_TABLE 0x2493
    switch (CoexPrio)
    {
    case 0:
        SetCSR8x11Pskey(BluetoothStackID, PSKEY_COEX_PIO_UNITY_3_BT_STATUS, cmdBuffer2, 2);
        GetCSR8x11Pskey(BluetoothStackID, PSKEY_COEX_PIO_UNITY_3_BT_STATUS);
        break;
    case 1:
        SetCSR8x11Pskey(BluetoothStackID, PSKEY_COEX_BLE_TRANSACTION_PRIORITY_TABLE, cmdBuffer, 16);
        GetCSR8x11Pskey(BluetoothStackID, PSKEY_COEX_BLE_TRANSACTION_PRIORITY_TABLE);
        break;
    case 2:
        cmdBuffer[3]  = 1;  //Connectable Directed Advertising
        cmdBuffer[8]  = 1;  //Initiator
        cmdBuffer[9]  = 1;  //Connection Establishment (Master)
        cmdBuffer[10] = 1;  //Connection Establishment (Slave)
        cmdBuffer[11] = 1;  //Anchor Point (Master)
        cmdBuffer[12] = 1;  //Anchor Point (Slave)
        SetCSR8x11Pskey(BluetoothStackID, PSKEY_COEX_BLE_TRANSACTION_PRIORITY_TABLE, cmdBuffer, 16);
        GetCSR8x11Pskey(BluetoothStackID, PSKEY_COEX_BLE_TRANSACTION_PRIORITY_TABLE);
        break;
    default:
        break;
    }
}

#endif /* ENABLE_BLUETOOTH */
