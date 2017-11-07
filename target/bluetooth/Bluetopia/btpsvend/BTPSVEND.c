/*****< btvend.c >*************************************************************/
/*      Copyright 2009 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BTVEND - Bluetooth Stack Bluetooth Vendor Specific Implementation for     */
/*           Stonestreet One Bluetooth Protocol Stack.                        */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   07/31/09  D. Lange       Initial creation.                               */
/******************************************************************************/
#include "BTPSVEND.h"         /* Bluetooth Vend. Spec. Prototypes/Constants.  */
#include "HCIDRV.h"
#include "SS1BTPS.h"          /* Bluetooth Stack API Prototypes/Constants.    */
#include "BTPSKRNL.h"         /* BTPS Kernel Prototypes/Constants.            */

   /* Miscellaneous Type Declarations.                                  */
#define COEX_MODE_VARIABLE_NAME                          "BTHOST_8311_COEX_MODE"
#define BD_ADDR_VARIABLE_NAME                            "BTHOST_BD_ADDR"

#define DEFAULT_BAUD_RATE                                115200
#define CSR_CONTROLLER                                   0x000A
#define CSR_8X11_A08_ID                                  0x2031
#define CSR_8X11_A12_ID                                  0x2918

#define PSKEY_DATA_SEQ_NUM_OFFSET                             5
#define PSKEY_DATA_START_OFFSET                              17

#define PATCH_DATA_WARM_RESET_ID                              0
#define PATCH_DATA_PATCH_ID                                   1
#define PATCH_DATA_PROTOCOL_ID                                2
#define PATCH_DATA_BAUD_RATE_ID                               3
#define PATCH_DATA_BD_ADDR_ID                                 4
#define PATCH_DATA_COEX_DATA                                  5
#define PATCH_DATA_COEX_MODE                                  6
#define PATCH_DATA_A08_PATCH_ID                               7
#define PATCH_DATA_A12_PATCH_ID                               8
#define PATCH_DATA_UART_CONFIG                                9
#define PATCH_DATA_FTRIM_ID                                  10
#define PATCH_DATA_DEEP_SLEEP_ID                             11

   /* The following defines the structure of a PS Key Header.           */
typedef struct _tagBCCMD_PS_Header_t
{
   NonAlignedByte_t ChannelID;
   NonAlignedWord_t CmdType;
   NonAlignedWord_t TotalLength;
   NonAlignedWord_t SequenceNumber;
   NonAlignedWord_t VARID;
   NonAlignedWord_t Status;
   NonAlignedWord_t PSKeyID;
   NonAlignedWord_t PSKeyLength;
   NonAlignedWord_t PSStore;
} BCCMD_PS_Header_t;

#define BCCMD_PS_HEADER_DATA_SIZE                (sizeof(BCCMD_PS_Header_t))
#define BCCMD_PS_HEADER_TOTAL_LENGTH             ((BCCMD_PS_HEADER_DATA_SIZE-1) >> 1)

   /* The following define the constants that can be used with the PS   */
   /* Key Header structure.                                             */
#define CHANNEL_ID_FLAG_FIRST_PACKET                   0x40
#define CHANNEL_ID_FLAG_LAST_PACKET                    0x80
#define BCCMD_CHANNEL_ID                               0x02
#define BCCMD_TYPE_GETREQ                            0x0000
#define BCCMD_TYPE_GETRESP                           0x0001
#define BCCMD_TYPE_SETREQ                            0x0002
#define BCCMD_VARID_PS                               0x7003
#define BCCMD_VARID_WARM_RESET                       0x4002
#define BCCMD_PS_STORE_DEFAULT                       0x0000
#define BCCMD_PS_STORE_RAM                           0x0008

   /* The following MACRO is used to convert an ASCII-Hex Character to  */
   /* its integer value.                                                */
#define ToInt(_x)                  (((_x) > 0x39)?((_x)-0x37):((_x)-0x30))

typedef struct _tagPSKEY_BD_ADDR_t
{
   Word_t UAP;
   Word_t NAP;
   Word_t LAPL;
   Word_t LAPU;
} PSKEY_BD_ADDR_t;

   /* The following structure is a container structure used to hold CSR */
   /* BCCMD commands that are to be sent at chipset initialization.     */
typedef struct _tagBCCMD_Info_t
{
   Byte_t  ExpectResponse;
   Byte_t *BCCMDData;
} BCCMD_Info_t;

   /* The following structure is used to pass data to the HCI Event     */
   /* Callback.                                                         */
typedef struct _tagCallbackInfo_t
{
   int     Status;
   Event_t Event;
} CallbackInfo_t;

   /* Internal Variables to this Module (Remember that all variables    */
   /* declared static are initialized to 0 automatically by the         */
   /* compiler as part of standard C/C++).                              */

   /* The following variable is used to track whether or not the Vendor */
   /* Specific Commands (and Patch RAM commands) have already been      */
   /* issued to the device.  This is done so that we do not issue them  */
   /* more than once for a single device (there is no need).  They could*/
   /* be potentially issued because the HCI Reset hooks (defined below) */
   /* are called for every HCI_Reset() that is issued.                  */
static Boolean_t                    VendorCommandsIssued;
static HCI_DriverType_t             hDriverType;
static Boolean_t                    DeviceSync;
static Boolean_t                    A12Device;
static Word_t                       SeqNum;
static Boolean_t                    VendParamsValid = FALSE;
static VendParams_t                 VendParams;
static PSKEY_BD_ADDR_t              PSKey_BD_ADDR;
static HCI_COMMDriverInformation_t  COMMDriverInformation;
static char                        *Variable;
static unsigned int                 DriverID;
static HCI_Patch_Callback_t         PatchCallback = NULL;
static unsigned long                PatchCallbackParameter;

   /* The following are the Commands that are used to patch the ROM of  */
   /* the CSR chip.                                                     */
static BCCMD_Info_t PSKeyStr[] =
{
   /* PSKEY_LM_TEST_SEND_ACCEPTED_TWICE                                 */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xF6\x00\x01\x00\x00\x00\x01\x00"},
   /* PSKEY_ANA_FREQ                                                    */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xFE\x01\x01\x00\x00\x00\x90\x65"},
   /* PSKEY_ANA_FTRIM                                                   */
   { PATCH_DATA_FTRIM_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xF6\x01\x01\x00\x00\x00\x11\x00"},
   /* PSKEY_BDADDR                                                      */
   { PATCH_DATA_BD_ADDR_ID,    (Byte_t *)"\xC2\x02\x00\x0C\x00\x00\x00\x03\x70\x00\x00\x01\x00\x04\x00\x00\x00\xCC\x00\xAD\xDE\x5B\x00\x02\x00"},
   /* PSKEY_HOST_INTERFACE                                              */
   { PATCH_DATA_PROTOCOL_ID,   (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xF9\x01\x01\x00\x00\x00\x03\x00"},
   /* PSKEY_HOST_BAUD_RATE                                              */
   { PATCH_DATA_BAUD_RATE_ID,  (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\xEA\x01\x02\x00\x00\x00\x01\x00\x00\xC2"},
   /* PSKEY_UART_CONFIG_H4                                              */
   { PATCH_DATA_UART_CONFIG,   (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xC0\x01\x01\x00\x00\x00\xA8\x08"},
   /* PSKEY_UART_CONFIG_H4DS                                            */
   { PATCH_DATA_UART_CONFIG,   (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xCB\x01\x01\x00\x00\x00\xA8\x08"},
   /* PSKEY_DEEP_SLEEP_STATE                                            */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\x29\x02\x01\x00\x00\x00\x03\x00"},
   /* PSKEY_DEEP_SLEEP_USE_EXTERNAL_CLOCK                               */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\xC3\x03\x01\x00\x00\x00\x00\x00"},
   /* PSKEY_PCM_CONFIG32                                                */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\xB3\x01\x02\x00\x00\x00\x80\x08\x00\x20"},
   /* PSKEY_PCM2_CONFIG32                                               */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\xD8\x01\x02\x00\x00\x00\x80\x08\x00\x20"},
   /* PSKEY_PCM_USE_LOW_JITTER_MODE                                     */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xC9\x23\x01\x00\x00\x00\x01\x00"},
   /* PSKEY_PCM2_USE_LOW_JITTER_MODE                                    */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xCA\x23\x01\x00\x00\x00\x01\x00"},
   /* PSKEY_PCM_SLOTS_PER_FRAME                                         */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xBB\x23\x01\x00\x00\x00\x04\x00"},
   /* PSKEY_PCM2_SLOTS_PER_FRAME                                        */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xBD\x23\x01\x00\x00\x00\x04\x00"},
   /* PSKEY_CLOCK_REQUEST_ENABLE                                        */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\x46\x02\x01\x00\x00\x00\x00\x00"},
   /* PSKEY_DEEP_SLEEP_USE_EXTERNAL_CLOCK                               */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xC3\x03\x01\x00\x00\x00\x00\x00"},
   /* PSKEY_LC_MAX_TX_POWER                                             */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\x17\x00\x01\x00\x00\x00\x04\x00"},
   /* PSKEY_LC_DEFAULT_TX_POWER                                         */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\x21\x00\x01\x00\x00\x00\x00\x00"},
   /* PSKEY_BT_TX_MIXER_CTRIM_OFFSET                                    */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x0D\x00\x00\x00\x03\x70\x00\x00\x75\x21\x05\x00\x00\x00\x00\x00\x00\x00\x00\x00\xFF\xFF\xFF\xEF"},
   /* PSKEY_BT_POWER_TABLE_V0                                           */
   { PATCH_DATA_PATCH_ID,      (Byte_t *)"\xC2\x02\x00\x30\x00\x00\x00\x03\x70\x00\x00\x1A\x24\x28\x00\x00\x00\x17\x27\x50\x00\x28\x29\x40\x00\x00\xEC\x17\x29\x40\x00\x28\x28\x30\x00\x00\xF0\x17\x28\x30\x00\x28\x28\x20\x00\x00\xF4\x17\x28\x20\x00\x28\x27\x10\x00\x00\xF8\x17\x28\x10\x00\x28\x28\x00\x00\x00\xFC\x18\x2B\x00\x00\x29\x37\x00\x00\x00\x00\x3A\x38\x00\x00\x8F\x3D\x00\x00\x00\x04\xFF\x3F\x00\x00\xFF\x3F\x00\x00\x00\x08"},

   /* PSKEY_UART_HOST_WAKE_SIGNAL                                       */
   { PATCH_DATA_DEEP_SLEEP_ID, (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xCA\x01\x01\x00\x00\x00\x03\x00"},
   /* PSKEY_UART_HOST_WAKE                                              */
   { PATCH_DATA_DEEP_SLEEP_ID, (Byte_t *)"\xC2\x02\x00\x0C\x00\x00\x00\x03\x70\x00\x00\xC7\x01\x04\x00\x00\x00\x01\x00\x40\x00\x03\x00\x02\x00"},
   /* PSKEY_H4DS_WAKE_DURATION                                          */
   { PATCH_DATA_DEEP_SLEEP_ID, (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\xCC\x01\x01\x00\x00\x00\x03\x00"},

   /* CoEx Params                                                       */

   /* PSKEY_COEX_SCHEME                                                 */
   { PATCH_DATA_COEX_MODE,     (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\x80\x24\x01\x00\x00\x00\x00\x00"},
   /* PSKEY_COEX_PIO_UNITY_3_BT_ACTIVE (BT_active)                      */
   { PATCH_DATA_COEX_DATA,     (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\x83\x24\x02\x00\x00\x00\x04\x00\x01\x00"},
   /* PSKEY_COEX_PIO_UNITY_3_BT_STATUS (BT_WAKE)                        */
   { PATCH_DATA_COEX_DATA,     (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\x84\x24\x02\x00\x00\x00\x01\x00\x01\x00"},
   /* PSKEY_COEX_PIO_UNITY_3_BT_DENY (WLAN active?)                     */
   { PATCH_DATA_COEX_DATA,     (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\x85\x24\x02\x00\x00\x00\x00\x00\x01\x00"},
   /* PSKEY_COEX_PIO_UNITY_3_BT_PERIODIC (BT_priority)                  */
   { PATCH_DATA_COEX_DATA,     (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\x87\x24\x02\x00\x00\x00\x03\x00\x01\x00"},
   /* PSKEY_COEX_PIO_UNITY_3_TIMINGS                                    */
   { PATCH_DATA_COEX_DATA,     (Byte_t *)"\xC2\x02\x00\x0A\x00\x00\x00\x03\x70\x00\x00\x8A\x24\x02\x00\x00\x00\x14\x00\x0A\x00"},

   /* CSR8311 A08 Params                                                */

   /* PSKEY_RX_MR_SAMP_CONFIG                                           */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\x3C\x00\x01\x00\x00\x00\x26\x04"},
   /* PSKEY_PATCH50                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x18\x00\x00\x00\x03\x70\x00\x00\x2C\x21\x10\x00\x00\x00\x00\x00\x7D\xC4\x14\x57\x18\x00\x2B\xFF\x0E\xFF\x00\xD8\x18\x79\x9E\x00\x18\x00\x2B\xFF\x0E\xFF\x00\xC5\x18\x80\xE2\x00\x80\x70"},
   /* PSKEY_PATCH51                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x21\x00\x00\x00\x03\x70\x00\x00\x2D\x21\x19\x00\x00\x00\x02\x00\x79\x5B\x14\x00\x00\xE7\x25\x95\x1B\x01\x26\x06\x9A\x08\xF4\x0A\x18\x02\x2B\xFF\x0E\xFF\x00\x5B\x18\x43\x9E\x00\x14\x00\x1B\x01\x26\x08\x18\x02\x2B\xFF\x0E\xFF\x00\x5C\x18\x82\xE2\x00\x3A\xA6"},
   /* PSKEY_PATCH52                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x2E\x00\x00\x00\x03\x70\x00\x00\x2E\x21\x26\x00\x00\x00\x02\x00\xA7\xD0\x16\x03\x18\x03\x2B\xFF\x0E\xFF\x00\x23\x18\x06\x9E\x00\x99\xE1\xF4\x14\x19\xE1\x12\x08\x00\x80\xC0\x00\xF0\x10\x1B\x06\x12\x02\x00\x01\xC0\xFF\x80\x07\xF0\x0A\x16\x03\x10\x00\x18\x03\x2B\xFF\x0E\xFF\x00\x38\x18\x13\x9E\x00\x0F\xF7\x18\x02\x2B\xFF\x0E\xFF\x00\xD1\x18\xAC\xE2\x00\x53\xCF"},
   /* PSKEY_PATCH53                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x22\x00\x00\x00\x03\x70\x00\x00\x2F\x21\x1A\x00\x00\x00\x03\x00\x16\xC9\x14\x01\x27\x00\x17\x05\x00\x2B\x84\xA4\xF0\x06\x00\x2B\x14\xA9\x27\x05\x14\x01\xE0\x02\x14\x00\x27\x01\x14\x00\x27\x02\x23\x03\x15\xE3\x18\x03\x2B\xFF\x0E\xFF\x00\xC9\x18\x1D\xE2\x00\x5F\xCB"},
   /* PSKEY_PATCH54                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x20\x00\x00\x00\x03\x70\x00\x00\x30\x21\x18\x00\x00\x00\x02\x00\x7A\xCA\x18\x00\x2B\xFF\x0E\xFF\x00\x84\x18\xCB\x9E\x00\x00\xEA\x11\x5F\x00\x01\x80\x00\x24\x05\x00\x01\xB0\x00\x00\xEA\x21\x5F\x18\x02\x2B\xFF\x0E\xFF\x00\xCA\x18\x7D\xE2\x00\x13\x25"},
   /* PSKEY_PATCH55                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x21\x00\x00\x00\x03\x70\x00\x00\x31\x21\x19\x00\x00\x00\x03\x00\x75\xFE\x00\xEB\x14\xC1\x18\x02\x2B\xFF\x0E\xFF\x00\x67\x18\xE7\x9E\x00\x00\xEA\x14\x67\x18\x02\x2B\xFF\x0E\xFF\x00\x67\x18\xE7\x9E\x00\x18\x03\x2B\xFF\x0E\xFF\x00\xFE\x18\x7A\xE2\x00\x6F\x0C"},
   /* PSKEY_PATCH58                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1B\x00\x00\x00\x03\x70\x00\x00\x34\x21\x13\x00\x00\x00\x01\x00\x6F\x1A\x1B\x05\x16\x01\x84\x02\xF0\x03\x10\x03\x22\x01\x10\x01\x1B\x07\x00\x01\x22\x89\x18\x01\x2B\xFF\x0E\xFF\x00\x1A\x18\x73\xE2\x00\x8C\xD5"},
   /* PSKEY_PATCH59                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1E\x00\x00\x00\x03\x70\x00\x00\x35\x21\x16\x00\x00\x00\x01\x00\xD1\x14\x40\xFF\x27\x06\x23\x05\x1B\x09\x16\x01\x84\x02\xF0\x07\x10\x03\x22\x01\x16\x5F\x00\x48\xB4\x00\x26\x5F\x18\x01\x2B\xFF\x0E\xFF\x00\x15\x18\xD4\xE2\x00\xFE\xF7"},
   /* PSKEY_PATCH60                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1C\x00\x00\x00\x03\x70\x00\x00\x36\x21\x14\x00\x00\x00\x03\x00\xA5\xF9\x18\x03\x2B\xFF\x0E\xFF\x00\xF9\x18\xF0\x9E\x00\x99\xE1\xF4\x04\x00\x40\x14\xFF\x0F\xF9\x18\x03\x2B\xFF\x0E\xFF\x00\xF9\x18\x73\xE2\x00\xC3\xAE"},
   /* PSKEY_PATCH61                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x11\x00\x00\x00\x03\x70\x00\x00\x37\x21\x09\x00\x00\x00\x01\x00\xF2\xE8\x18\x01\x2B\xFF\x0E\xFF\x00\xE9\x18\xF6\xE2\x00\x30\x58"},
   /* PSKEY_PATCH63                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x23\x00\x00\x00\x03\x70\x00\x00\x39\x21\x1B\x00\x00\x00\x01\x00\x02\x8D\x18\x00\x2B\xFF\x0E\xFF\x00\xD6\x18\xCC\x9E\x00\x18\x01\x2B\xFF\x0E\xFF\x00\x79\x18\x31\x9E\x00\x99\xE1\xF4\x09\x00\xE1\x19\xB2\x16\x03\x27\xFE\x16\x02\x27\xFF\x0E\xFF\x9F\xFE\x14\x01\x0F\xF8\x6F\xD0"},
   /* PSKEY_PATCH64                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x2D\x00\x00\x00\x03\x70\x00\x00\x3A\x21\x25\x00\x00\x00\x00\x00\xAE\xD5\x18\x01\x2B\xFF\x0E\xFF\x00\x6A\x18\xE4\x9E\x00\x99\xE1\xF4\x15\x13\x05\x00\x77\x80\xB5\xF4\x02\x34\x05\x19\xE1\x16\x00\x34\x04\x25\xF8\x16\x01\xA0\x08\x35\xF8\x1B\x02\x26\x01\x18\x00\x2B\xFF\x0E\xFF\x00\xD6\x18\xBF\xE2\x00\x18\x00\x2B\xFF\x0E\xFF\x00\xD6\x18\xBC\xE2\x00\x2A\xC3"},
   /* PSKEY_PATCH65                                                     */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x27\x00\x00\x00\x03\x70\x00\x00\x3B\x21\x1F\x00\x00\x00\x01\x00\xAE\x8C\xF0\x0A\x00\xE1\x15\xA3\x18\x01\x2B\xFF\x0E\xFF\x00\x69\x18\xA5\x9E\x00\x0F\xF8\x9B\x04\xF0\x0B\x00\xE1\x14\xA1\x00\xE5\x10\xED\x18\x00\x2B\xFF\x0E\xFF\x00\xD6\x18\x8C\x9E\x00\x18\x01\x2B\xFF\x0E\xFF\x00\x8D\x18\xCC\xE2\x00\xA7\xDF"},
   /* PSKEY_PATCH107                                                    */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1C\x00\x00\x00\x03\x70\x00\x00\xFB\x21\x14\x00\x00\x00\x00\xF0\x15\x32\x00\x08\xB4\x00\x00\xF0\x25\x32\x00\xF0\x15\x32\x00\xF8\xC4\xFF\x00\xF0\x25\x32\x00\xF0\x15\x32\x00\x08\xB4\x00\x00\xF0\x25\x32\xE2\x00\xE0\x49"},
   /* PSKEY_PATCH112                                                    */
   { PATCH_DATA_A08_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x0F\x00\x00\x00\x03\x70\x00\x00\x00\x22\x07\x00\x00\x00\x00\xF1\x15\xCF\xB4\x01\x00\xF1\x25\xCF\xE2\x00\xD1\x9D"},

   /* CSR8311 A12 Params                                                */

   /* PSKEY_HCI_LMP_LOCAL_VERSION                                       */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\x0D\x01\x01\x00\x00\x00\x08\x08"},
   /* PSKEY_LMP_REMOTE_VERSION                                          */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x03\x70\x00\x00\x0E\x01\x01\x00\x00\x00\x08\x00"},
   /* PSKEY_PATCH50                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x40\x00\x00\x00\x03\x70\x00\x00\x2C\x21\x38\x00\x00\x00\x00\x00\x01\xF0\x17\x06\x13\x05\x18\x01\x2B\xFF\x0E\xFF\x00\x1A\x18\x28\x9E\x00\x1B\x08\x00\xF1\x88\x88\xF0\x24\x25\xF9\x21\xF8\x17\x0A\x84\x01\xF0\x0C\x17\x01\x13\x00\x09\x00\xA4\x02\x25\xFB\x21\xFA\x15\xF9\x11\xF8\x55\xFB\x61\xFA\xE0\x09\x84\xFF\xF0\x10\x17\x01\x13\x00\x09\x00\xA4\x02\x35\xF9\x41\xF8\x25\xF9\x21\xF8\x1B\x0F\x12\x07\xA4\x10\x94\x04\x12\x07\x51\xE1\x22\x07\x15\xF9\x11\xF8\x18\x00\x2B\xFF\x0E\xFF\x00\xF0\x18\x05\xE2\x00\x79\x5A"},
   /* PSKEY_PATCH51                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1D\x00\x00\x00\x03\x70\x00\x00\x2D\x21\x15\x00\x00\x00\x02\x00\x8A\x96\x63\x08\x25\xF9\x21\xF8\x57\x07\x63\x06\x99\xE0\xEC\x02\xE0\x05\x15\xF9\x27\x07\x15\xF8\x27\x06\x18\x02\x2B\xFF\x0E\xFF\x00\x97\x18\x8D\xE2\x00\x34\x7E"},
   /* PSKEY_PATCH52                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x18\x00\x00\x00\x03\x70\x00\x00\x2E\x21\x10\x00\x00\x00\x02\x00\x5A\x0B\x00\x01\x34\x78\x40\x00\x27\x03\x23\x02\x15\xF8\x11\xE3\x18\x02\x2B\xFF\x0E\xFF\x00\x0B\x18\x5E\xE2\x00\xC1\x59"},
   /* PSKEY_PATCH53                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x19\x00\x00\x00\x03\x70\x00\x00\x2F\x21\x11\x00\x00\x00\x00\x00\x15\x73\x84\x00\xF0\x04\x00\x08\x14\x00\xE0\x03\x00\xF8\x15\x12\x27\x0B\x18\x00\x2B\xFF\x0E\xFF\x00\x73\x18\x18\xE2\x00\x49\x05"},
   /* PSKEY_PATCH54                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1D\x00\x00\x00\x03\x70\x00\x00\x30\x21\x15\x00\x00\x00\x01\x00\xF8\x53\x17\x08\x27\x0E\x00\x0C\x84\x63\xF0\x07\x18\x01\x2B\xFF\x0E\xFF\x00\x54\x18\x09\xE2\x00\x17\x09\x18\x01\x2B\xFF\x0E\xFF\x00\x54\x18\xFB\xE2\x00\xF6\x91"},
   /* PSKEY_PATCH55                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x39\x00\x00\x00\x03\x70\x00\x00\x31\x21\x31\x00\x00\x00\x02\x00\xD5\xC0\x23\x04\xF4\x05\x1B\x03\x12\x00\x80\x02\xF0\x1D\x1B\x02\x16\x09\x00\x10\xC4\x00\xF0\x1E\x16\x09\xC4\x03\xF4\x1B\x1B\x03\x12\x00\x80\x02\xF4\x17\x80\x03\xF4\x15\x80\x04\xF4\x13\x80\x05\xF4\x11\x80\x06\xF4\x0F\x80\x0B\xF4\x0D\x80\x0D\xF4\x0B\x9C\x01\x00\xFB\x19\x67\x9E\x0D\x18\x02\x2B\xFF\x0E\xFF\x00\xC2\x18\x8C\xE2\x00\x18\x02\x2B\xFF\x0E\xFF\x00\xC1\x18\xDE\xE2\x00\xCF\xC5"},
   /* PSKEY_PATCH56                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x32\x00\x00\x00\x03\x70\x00\x00\x32\x21\x2A\x00\x00\x00\x03\x00\x3F\x24\x0B\xF4\x27\x08\x23\x09\x19\xE0\x16\x09\xB4\x01\x26\x09\x18\x03\x2B\xFF\x0E\xFF\x00\x24\x18\x42\xE2\x00\x0B\xFA\x14\x3D\x27\x03\x14\x01\x27\x02\x14\x00\x27\x01\x27\x00\x1B\x08\x16\x08\x18\x02\x2B\xFF\x0E\xFF\x00\xF7\x18\xFD\x9E\x00\x11\xE1\x1B\x08\x16\x08\x18\x02\x2B\xFF\x0E\xFF\x00\x33\x18\xFF\x9E\x00\x0F\xFA\xF2\x40"},
   /* PSKEY_PATCH57                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1F\x00\x00\x00\x03\x70\x00\x00\x33\x21\x17\x00\x00\x00\x03\x00\x4E\x25\xC0\x02\x16\x09\x00\x10\xC4\x00\xF4\x02\xB0\x02\x16\x09\xC4\xFE\xB1\xE1\x22\x09\x16\x08\x18\x03\x2B\xFF\x0E\xFF\x00\x25\x18\x52\x00\x80\xC0\x00\xF2\x08\xE2\x00\x02\xE4"},
   /* PSKEY_PATCH58                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x18\x00\x00\x00\x03\x70\x00\x00\x34\x21\x10\x00\x00\x00\x03\x00\xCA\x21\x0B\xFA\x27\x02\x23\x03\x19\xE0\x16\x09\xB4\x01\x26\x09\x18\x03\x2B\xFF\x0E\xFF\x00\x22\x18\xCD\xE2\x00\xDC\x22"},
   /* PSKEY_PATCH59                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1A\x00\x00\x00\x03\x70\x00\x00\x35\x21\x12\x00\x00\x00\x00\x00\xE2\x55\xF4\x09\x18\x02\x2B\xFF\x0E\xFF\x00\x2C\x18\xA7\x9E\x00\x14\x03\x0F\xFC\x18\x00\x2B\xFF\x0E\xFF\x00\x56\x18\x01\xE2\x00\x08\xA0"},
   /* PSKEY_PATCH60                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x26\x00\x00\x00\x03\x70\x00\x00\x36\x21\x1E\x00\x00\x00\x04\x00\x80\x0D\xA4\x1A\x18\x04\x2B\xFF\x0E\xFF\x00\x1D\x18\xBA\x9E\x00\x84\x01\x2C\x0D\x13\x00\x00\x8F\xD0\x89\x17\x01\x00\xBF\xD4\xD6\x18\x03\x2B\xFF\x0E\xFF\x00\x27\x18\x42\x9E\x00\x18\x04\x2B\xFF\x0E\xFF\x00\x0E\x18\x84\xE2\x00\x55\x68"},

   /* PSKEY_PATCH61                                                     */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x1E\x00\x00\x00\x03\x70\x00\x00\x37\x21\x16\x00\x00\x00\x02\x00\xA5\x4D\x18\x01\x2B\xFF\x0E\xFF\x00\x62\x18\xC5\x9E\x00\x1B\x03\x22\x2B\xC4\xFC\x26\x2C\x14\x01\x00\xE7\x25\xF0\x18\x02\x2B\xFF\x0E\xFF\x00\x4E\x18\xA8\xE2\x00\xFC\x8D"},
   /* PSKEY_PATCH123                                                    */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x39\x00\x00\x00\x03\x70\x00\x00\x0B\x22\x31\x00\x00\x00\x0B\xFA\x17\x07\x84\x04\xF0\x2C\x17\x06\x90\x06\x19\xE1\x00\x0C\x38\xD4\x1A\x04\x1A\x00\x2B\x02\xF4\x23\x16\x01\x84\x01\xF0\x20\x16\x02\x27\x03\xF4\x1D\x19\xE1\x16\x00\x84\x07\xF0\x19\x00\xE5\x15\xAB\x00\x80\x54\x00\x00\x01\x18\xC0\x2B\xFF\x27\xFE\x17\x03\x0E\xFF\x9F\xFE\x99\xE1\xF4\x0C\x00\x7D\x14\xF2\x27\x01\x14\x00\x27\x00\x17\x03\x0E\x00\x9F\x01\x14\x00\x1B\x02\x26\x02\x0F\xFA\x92\x83"},
   /* PSKEY_PATCH155                                                    */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x44\x00\x00\x00\x03\x70\x00\x00\x2B\x22\x3C\x00\x00\x00\x0B\xE7\x27\x16\x00\xA1\x14\x85\x27\x02\x14\x02\x27\x01\x00\x6B\x14\xD8\x27\x04\x14\x02\x27\x03\x17\x04\x27\xFE\x17\x03\x27\xFF\x13\x16\x30\x02\x14\x08\x27\x00\x15\xE4\x34\x05\x0E\xFF\x9F\xFE\x13\x05\x09\x00\xA4\x01\x15\xE0\xC4\x7F\x84\x7F\xF0\x1B\x17\x06\x84\x15\xF0\x18\x1B\x16\x16\x01\x0E\x01\x9F\x02\x19\xE1\x9A\x04\xF4\x11\x16\x01\x84\x41\xF0\x0E\x00\xE5\x11\xAC\x00\x80\x50\x00\x00\x01\x14\xC0\x27\xFF\x23\xFE\x16\x08\x0E\xFF\x9F\xFE\x14\x01\xE0\x02\x14\x00\x0F\xE7\x65\x4D"},
   /* PSKEY_PATCH156                                                    */
   { PATCH_DATA_A12_PATCH_ID,  (Byte_t *)"\xC2\x02\x00\x30\x00\x00\x00\x03\x70\x00\x00\x2C\x22\x28\x00\x00\x00\x0B\xF4\x27\x09\x00\x29\x14\xD4\x27\x04\x14\x03\x27\x03\x00\x35\x14\xEA\x27\x06\x14\x03\x27\x05\x00\x29\x14\x96\x27\x08\x14\x04\x27\x07\x17\x09\x10\x01\x0E\x07\x9F\x08\x17\x09\x0E\x03\x9F\x04\x17\x06\x27\xFE\x17\x05\x27\xFF\x14\x7F\x27\x00\x14\x15\x27\x01\x14\x2A\x27\x02\x14\x01\x13\x09\x0E\xFF\x9F\xFE\x0F\xF4\x78\x34"},


   /* WARM RESET                                                        */
   { PATCH_DATA_WARM_RESET_ID, (Byte_t *)"\xC2\x02\x00\x09\x00\x00\x00\x02\x40\x00\x00\x00\x00\x00\x00\x00\x00\x00\x00"}
 };

#define NUM_PSKEY_ENTRIES        (sizeof(PSKeyStr)/sizeof(BCCMD_Info_t))

   /* Internal Function Prototypes.                                     */
static void BTPSAPI HCI_DriverCallback(unsigned int HCIDriverID, HCI_Packet_t *HCIPacket, unsigned long CallbackParameter);
static int  SyncCSR8311(HCI_DriverInformation_t *HCI_DriverInformation);
static void ToPSKEY_BD_ADDR(BD_ADDR_t *BD_ADDR, PSKEY_BD_ADDR_t *PS_BD_ADDR);

#ifdef ENABLE_ENVIRONMENT_CONFIG

static void StrToBD_ADDR(char *AddrStr, BD_ADDR_t *BD_ADDR);

   /* The following converts a Hex String into a BD_ADDR.               */
static void StrToBD_ADDR(char *AddrStr, BD_ADDR_t *BD_ADDR)
{
   int  i;
   char val;
   char ADDR[6];

   /* Verify that the pointers passed in appear valid.                  */
   if((AddrStr) && (BD_ADDR))
   {
      /* Check to see if the string leads with '0x'.                    */
      if((AddrStr[0]) && (AddrStr[1]) && (AddrStr[0] == '0') && ((AddrStr[1] == 'x') || (AddrStr[1] == 'X')))
      {
         /* Skip over the header.                                       */
         AddrStr += 2;
      }

      /* Convert the next 6 bytes.                                      */
      for(i=0; i<6; i++)
      {
         /* Verify that there are 2 characters available.               */
         if((AddrStr[0]) && (AddrStr[1]))
         {
            val  = (char)(ToInt(*AddrStr) * 0x10);
            AddrStr++;
            val += (char)ToInt(*AddrStr);
            AddrStr++;
         }

         /* Save the value in the BD_ADDR structure.                    */
         ADDR[i] = (Byte_t)val;
      }

      /* Set the BD_ADDR.                                               */
      BD_ADDR->BD_ADDR5 = ADDR[0];
      BD_ADDR->BD_ADDR4 = ADDR[1];
      BD_ADDR->BD_ADDR3 = ADDR[2];
      BD_ADDR->BD_ADDR2 = ADDR[3];
      BD_ADDR->BD_ADDR1 = ADDR[4];
      BD_ADDR->BD_ADDR0 = ADDR[5];
   }
}

#endif

   /* The following function is used to convert a BD_ADDR to a          */
   /* PSKEY_BD_ADDT_t.  The PSKEY_BD_ADDR_t format is required for      */
   /* programming a BD_ADDR.                                            */
static void ToPSKEY_BD_ADDR(BD_ADDR_t *BD_ADDR, PSKEY_BD_ADDR_t *PS_BD_ADDR)
{
   /* Verify that the porters to the data appears valid.                */
   if((BD_ADDR) && (PS_BD_ADDR))
   {
      /* Set the PSKey Address Format.                                  */
      PS_BD_ADDR->LAPU = (Word_t)BD_ADDR->BD_ADDR2;
      PS_BD_ADDR->LAPL = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&BD_ADDR->BD_ADDR0);
      PS_BD_ADDR->UAP  = (Word_t)BD_ADDR->BD_ADDR3;
      PS_BD_ADDR->NAP  = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&BD_ADDR->BD_ADDR4);
   }
}

   /* The following function is the HCI Driver Callback that is         */
   /* installed for packets that expect responses.                      */
static void BTPSAPI HCI_DriverCallback(unsigned int HCIDriverID, HCI_Packet_t *HCIPacket, unsigned long CallbackParameter)
{
   /* Verify that the input parameters are semi-valid.                  */
   if((HCIDriverID) && (HCIPacket) && (CallbackParameter))
   {
      /* Verify that the packet is valid.                               */
      if(HCIPacket->HCIPacketType == ptHCIEventPacket)
      {
         if(DeviceSync)
         {
            BTPS_SetEvent(*(Event_t *)CallbackParameter);
         }
         else
         {
            /* Verify that we have the correct amount of data for the   */
            /* correct event.                                           */
            if((HCIPacket->HCIPacketLength == (HCI_EVENT_HEADER_SIZE + 4) && ((((HCI_Event_Header_t *)(HCIPacket->HCIPacketData)))->Event_Code == HCI_EVENT_CODE_COMMAND_COMPLETE)))
            {
               /* Get the command for the event.                        */
               if(READ_UNALIGNED_WORD_LITTLE_ENDIAN(&((((HCI_Command_Complete_Event_Header_t *)(HCIPacket->HCIPacketData)))->Command_OpCode)) == HCI_COMMAND_OPCODE_RESET)
               {
                  BTPS_SetEvent(*(Event_t *)CallbackParameter);
               }
            }
         }
      }
   }
}

   /* The following function is used to query the Bluetooth controller  */
   /* in order to identify that version of the controller.              */
static int SyncCSR8311(HCI_DriverInformation_t *HCI_DriverInformation)
{
   int             ret_val = -1;
   int             Count;
   int             HCIDriverID;
   Byte_t          CommandBuffer[HCI_CALCULATE_PACKET_SIZE(11)];
   HCI_Packet_t   *HCI_Packet;
   Byte_t         *HCI_PacketData;
   int             CallbackID;
   CallbackInfo_t  CallbackInfo;

   /* Since downloading the Patch Ram causes a USB re-enumeration, we   */
   /* are downloading the Patch BEFORE the stack opens the device.      */
   /* After that, we will let the rest of the Vendor Specific process   */
   /* function normally (i.e.  do nothing) as there is nothing left to  */
   /* do.                                                               */
   if((HCIDriverID = HCI_OpenDriver(HCI_DriverInformation)) > 0)
   {
      /* Allocate an event to use for signalling from the callback.     */
      BTPS_MemInitialize(&CallbackInfo, 0, sizeof(CallbackInfo_t));
      CallbackInfo.Event = BTPS_CreateEvent(FALSE);
      if(CallbackInfo.Event)
      {
         CallbackID = HCI_RegisterEventCallback(HCIDriverID, HCI_DriverCallback, (unsigned long)&CallbackInfo.Event);

         /* Reset the callback event.                                   */
         BTPS_ResetEvent(CallbackInfo.Event);

         /* Format the HCI Command Packet.                              */
         HCI_Packet                  = (HCI_Packet_t *)CommandBuffer;
         HCI_Packet->HCIPacketType   = 0x81;
         HCI_Packet->HCIPacketLength = 11;
         HCI_PacketData              = HCI_Packet->HCIPacketData;
         BTPS_MemCopy(HCI_PacketData, (Byte_t *)"\x70\x51\x1F\x68\x34\x91\x80\xD9\x8F\x48\x34", 11);

         Count      = 10;
         DeviceSync = TRUE;
         while(Count)
         {
            /* Send the packet to the HCI Driver.                       */
            ret_val = HCI_SendPacket(HCIDriverID, HCI_Packet, NULL, 0);
            if(ret_val > 0)
            {
               /* Wait for up to the timeout for a response.            */
               if(BTPS_WaitEvent(CallbackInfo.Event, 1000))
               {
                  ret_val = 0;
                  break;
               }
               Count--;
            }
            else
               break;
         }

         DeviceSync = FALSE;

         if(!ret_val)
         {
            /* Format the HCI Command Packet.                           */
            HCI_Packet                  = (HCI_Packet_t *)CommandBuffer;
            HCI_Packet->HCIPacketType   = ptHCICommandPacket;
            HCI_Packet->HCIPacketLength = HCI_COMMAND_HEADER_SIZE;
            HCI_PacketData              = HCI_Packet->HCIPacketData;
            HCI_PacketData[0]           = (Byte_t)HCI_COMMAND_OPCODE_RESET;
            HCI_PacketData[1]           = (Byte_t)(HCI_COMMAND_OPCODE_RESET >> 8);
            HCI_PacketData[2]           = 0;

            /* Reset the callback event.                                */
            BTPS_ResetEvent(CallbackInfo.Event);

            Count = 5;
            while(Count)
            {
               /* Send the packet to the HCI Driver.                    */
               ret_val = HCI_SendPacket(HCIDriverID, HCI_Packet, NULL, 0);
               if(ret_val > 0)
               {
                  /* Wait for up to the timeout for a response.         */
                  if(BTPS_WaitEvent(CallbackInfo.Event, 1000))
                  {
                     ret_val = 0;
                     break;
                  }
                  Count--;
               }
               else
                  break;
            }
         }

         /* Close the event.                                            */
         BTPS_CloseEvent(CallbackInfo.Event);
         HCI_UnRegisterCallback(HCIDriverID, CallbackID);
      }

      /* Close the Driver because we are finished with it.              */
      HCI_CloseDriver((unsigned int)HCIDriverID);
   }

   return(ret_val);
}

   /* The following function is responsible for making sure that the    */
   /* Bluetooth Stack BTVEND module is Initialized correctly.  This     */
   /* function *MUST* be called before ANY other Bluetooth Stack BTVEND */
   /* function can be called.  This function returns zero if the Module */
   /* was initialized correctly, or a non-zero value if there was an    */
   /* error.                                                            */
   /* * NOTE * Internally, this module will make sure that this         */
   /*          function has been called at least once so that the       */
   /*          module will function.  Calling this function from an     */
   /*          external location is not necessary.                      */
int InitializeBTVENDModule(void)
{
   /* Nothing to do here.                                               */
   return(TRUE);
}

   /* The following function is responsible for instructing the         */
   /* Bluetooth Stack BTVEND Module to clean up any resources that it   */
   /* has allocated.  Once this function has completed, NO other        */
   /* Bluetooth Stack BTVEND Functions can be called until a successful */
   /* call to the InitializeBTVENDModule() function is made.  The       */
   /* parameter to this function specifies the context in which this    */
   /* function is being called.  If the specified parameter is TRUE,    */
   /* then the module will make sure that NO functions that would       */
   /* require waiting/blocking on Mutexes/Events are called.  This      */
   /* parameter would be set to TRUE if this function was called in a   */
   /* context where threads would not be allowed to run.  If this       */
   /* function is called in the context where threads are allowed to run*/
   /* then this parameter should be set to FALSE.                       */
void CleanupBTVENDModule(Boolean_t ForceCleanup)
{
   /* Nothing to do here.                                               */
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality that needs to be performed before   */
   /* the HCI Communications layer is opened.  This function is called  */
   /* immediately prior to calling the initialization of the HCI        */
   /* Communications layer.  This function should return a BOOLEAN TRUE */
   /* indicating successful completion or should return FALSE to        */
   /* indicate unsuccessful completion.  If an error is returned the    */
   /* stack will fail the initialization process.                       */
   /* * NOTE * The parameter passed to this function is the exact       */
   /*          same parameter that was passed to BSC_Initialize() for   */
   /*          stack initialization.  If this function changes any      */
   /*          members that this pointer points to, it will change the  */
   /*          structure that was originally passed.                    */
   /* * NOTE * No HCI communication calls are possible to be used in    */
   /*          this function because the driver has not been initialized*/
   /*          at the time this function is called.                     */
Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIOpen(HCI_DriverInformation_t *HCI_DriverInformation)
{
   int ret_val = 0;

   /* Flag that we have not issued the first Vendor Specific Commands   */
   /* before the first reset.                                           */
   VendorCommandsIssued = FALSE;

   /* Save the Driver Type.                                             */
   hDriverType = HCI_DriverInformation->DriverType;

   /* Check to see if this is a Com Device.                             */
   if(hDriverType == hdtCOMM)
   {
      /* Check for Custom Parameters.                                   */
      COMMDriverInformation = HCI_DriverInformation->DriverInformation.COMMDriverInformation;

      /* Check to see if the Vendor Parameters have already been        */
      /* assigned.                                                      */
      if(!VendParamsValid)
      {
         /* Check to see if the configuration information is obtained   */
         /* from the environment variables.                             */
#ifdef ENABLE_ENVIRONMENT_CONFIG

         /* Retrieve the CoEx Mode from the Environment.                */
         Variable = (char *)getenv(COEX_MODE_VARIABLE_NAME);
         if(Variable)
            VendParams.CoExMode = (Word_t)atoi(Variable);
         else
            VendParams.CoExMode = 0;

         Variable = (char *)getenv(BD_ADDR_VARIABLE_NAME);
         if(Variable)
            StrToBD_ADDR(Variable, &VendParams.BD_ADDR);
         else
         {
            ASSIGN_BD_ADDR(VendParams.BD_ADDR, 0x00, 0x02, 0x5B, 0x00, 0x00, 0x00);
         }

#else

         /* Check for Custom Parameters.                                */
         if(COMMDriverInformation.Flags)
         {
            VendParams = *((VendParams_t *)(COMMDriverInformation.Flags));
         }
         else
         {
            /* Clear all Vendor Parameters.                             */
            BTPS_MemInitialize(&VendParams, 0, sizeof(VendParams_t));

            /* Use a crude method for generating a random number.       */
            Variable = (char *)BTPS_AllocateMemory(sizeof(char *));
            ASSIGN_BD_ADDR(VendParams.BD_ADDR, 0x00, 0x02, 0x5B, ((unsigned char *)&Variable)[0], ((unsigned char *)&Variable)[1], ((unsigned char *)&Variable)[2]);
            BTPS_FreeMemory(Variable);
         }

#endif

      }

      /* Set the PSKey Address Format.                                  */
      ToPSKEY_BD_ADDR(&VendParams.BD_ADDR, &PSKey_BD_ADDR);

      /* If we are using UART mode, then we have to Sync with the       */
      /* controller.                                                    */
      if((HCI_DriverInformation->DriverInformation.COMMDriverInformation.Protocol == cpUART) || (HCI_DriverInformation->DriverInformation.COMMDriverInformation.Protocol == cpUART_RTS_CTS))
         ret_val = SyncCSR8311(HCI_DriverInformation);
      else
         ret_val = 0;
   }

   return((Boolean_t)(ret_val == 0));
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality after the HCI Communications layer  */
   /* is initialized (the driver only).  This function is called        */
   /* immediately after returning from the initialization of the HCI    */
   /* Communications layer (HCI Driver).  This function should return a */
   /* BOOLEAN TRUE indicating successful completion or should return    */
   /* FALSE to indicate unsuccessful completion.  If an error is        */
   /* returned the stack will fail the initialization process.          */
   /* * NOTE * No HCI layer function calls are possible to be used in   */
   /*          this function because the actual stack has not been      */
   /*          initialized at this point.  The only initialization that */
   /*          has occurred is with the HCI Driver (hence the HCI       */
   /*          Driver ID that is passed to this function).              */
Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIOpen(unsigned int HCIDriverID)
{
   DriverID = HCIDriverID;

   return(TRUE);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functions after the HCI Communications layer AND  */
   /* the HCI Stack layer has been initialized.  This function is called*/
   /* after all HCI functionality is established, but before the initial*/
   /* HCI Reset is sent to the stack.  The function should return a     */
   /* BOOLEAN TRUE to indicate successful completion or should return   */
   /* FALSE to indicate unsuccessful completion.  If an error is        */
   /* returned the stack will fail the initialization process.          */
   /* * NOTE * At the time this function is called HCI Driver and HCI   */
   /*          layer functions can be called, however no other stack    */
   /*          layer functions are able to be called at this time       */
   /*          (hence the HCI Driver ID and the Bluetooth Stack ID      */
   /*          passed to this function).                                */
Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIReset(unsigned int HCIDriverID, unsigned int BluetoothStackID)
{
   int                              ret_val = 0;
   Byte_t                           Status;
   Byte_t                           Length;
   Byte_t                           BufferLength;
   Byte_t                           Buffer[255];
   Word_t                           ControllerID;
   unsigned int                     Index;
   HCI_Driver_Reconfigure_Data_t    DriverReconfigureData;
   HCI_COMMReconfigureInformation_t COMReconfigureData;

   /* If we haven't issued the Vendor Specific Commands yet, then go    */
   /* ahead and issue them.  If we have, then there isn't anything to   */
   /* do.                                                               */
   if(!VendorCommandsIssued)
   {
      /* Flag that we have issued the Vendor Specific Commands (so we   */
      /* don't do it again if someone issues an HCI_Reset().            */
      VendorCommandsIssued = TRUE;

      /* Check to see if this is a Com Device.                          */
      if(hDriverType == hdtCOMM)
      {
         /* Initialize the Patch Index and Sequence Number starting     */
         /* point.                                                      */
         Index  = 0;
         SeqNum = 0x0020;

         /* Determine which controller is installed.                    */
         A12Device = FALSE;
         ret_val   = HCI_Read_Local_Version_Information(BluetoothStackID, &Status, &Buffer[0], (Word_t *)&Buffer[2], &Buffer[4], (Word_t *)&Buffer[6], (Word_t *)&Buffer[8]);
         if(!ret_val)
         {
            /* Verify that this is a CSR Controller.                    */
            if(READ_UNALIGNED_WORD_LITTLE_ENDIAN(&Buffer[6]) == CSR_CONTROLLER)
            {
               /* Check to see if this is the A12 device.               */
               ControllerID = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&Buffer[2]);
               if(ControllerID == CSR_8X11_A12_ID)
                  A12Device = TRUE;
            }
            else
            {
               /* This is not an expected controller, so skip the       */
               /* patching.                                             */
               Index = NUM_PSKEY_ENTRIES;
            }
         }

         while(Index < NUM_PSKEY_ENTRIES)
         {
            Length       = (Byte_t)((PSKeyStr[Index].BCCMDData[3] << 1)+1);
            BufferLength = sizeof(Buffer);
            BTPS_MemCopy(Buffer, PSKeyStr[Index].BCCMDData, Length);

            /* Check to see if the Expected Response value indicates the*/
            /* Baud Rate setting.                                       */
            if(PSKeyStr[Index].ExpectResponse == PATCH_DATA_BAUD_RATE_ID)
            {
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET],   ((COMMDriverInformation.BaudRate >> 16) & 0xFFFF));
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET+2], (COMMDriverInformation.BaudRate & 0xFFFF));
            }

            /* Check to see if the Expected Response value indicates the*/
            /* Protocol setting.                                        */
            if(PSKeyStr[Index].ExpectResponse == PATCH_DATA_PROTOCOL_ID)
            {
               /* Configure the COMM Protocol that we are going to use. */
               switch(COMMDriverInformation.Protocol)
               {
                  case cpBCSP:
                     Buffer[PSKEY_DATA_START_OFFSET] = 0x01;
                     break;
                  case cpH4DS:
                  case cpH4DS_RTS_CTS:
                     Buffer[PSKEY_DATA_START_OFFSET] = 0x07;
                     break;
                  case cp3Wire:
                  case cp3Wire_RTS_CTS:
                     Buffer[PSKEY_DATA_START_OFFSET] = 0x06;
                     break;
                  default:
                     /* Default to UART.                                */
                     Buffer[PSKEY_DATA_START_OFFSET] = 0x03;
                     break;
               }
            }

            /* Check to see if the Expected Response value indicates    */
            /* hardware flow control or not.                            */
            if(PSKeyStr[Index].ExpectResponse == PATCH_DATA_UART_CONFIG)
            {
               if((COMMDriverInformation.Protocol == cpUART_RTS_CTS) || (COMMDriverInformation.Protocol == cpH4DS_RTS_CTS) || (COMMDriverInformation.Protocol == cp3Wire_RTS_CTS))
                  Buffer[PSKEY_DATA_START_OFFSET] = 0xA8;
               else
                  Buffer[PSKEY_DATA_START_OFFSET] = 0xA0;
            }

            /* Check to see if the Expected Response value indicates the*/
            /* BD_ADDR setting.                                         */
            if(PSKeyStr[Index].ExpectResponse == PATCH_DATA_BD_ADDR_ID)
            {
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET],   PSKey_BD_ADDR.LAPU);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET+2], PSKey_BD_ADDR.LAPL);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET+4], PSKey_BD_ADDR.UAP);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET+6], PSKey_BD_ADDR.NAP);
            }

            /* Check to see if the Expected Response value indicates the*/
            /* CoExistance Mode.                                        */
            if((PSKeyStr[Index].ExpectResponse == PATCH_DATA_COEX_MODE) || (PSKeyStr[Index].ExpectResponse == PATCH_DATA_COEX_DATA))
            {
               /* Check to see if we are enabling CoEx Mode.            */
               if(VendParams.CoExMode)
               {
                  /* Check to see if we need to insert the CoEx mode.   */
                  if(PSKeyStr[Index].ExpectResponse == PATCH_DATA_COEX_MODE)
                     ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET], VendParams.CoExMode);
               }
               else
               {
                  /* Advance to the next entry and continue.            */
                  Index++;
                  continue;
               }
            }

            /* Check to see if the Expected Response value indicates the*/
            /* FTRIM setting.                                           */
            if((VendParams.TrimValue) && (PSKeyStr[Index].ExpectResponse == PATCH_DATA_FTRIM_ID))
            {
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_START_OFFSET], VendParams.TrimValue);
            }

            /* Check to see if this is patch data for the wrong type of */
            /* device.                                                  */
            if(((PSKeyStr[Index].ExpectResponse == PATCH_DATA_A08_PATCH_ID) && (A12Device)) || ((PSKeyStr[Index].ExpectResponse == PATCH_DATA_A12_PATCH_ID) && (!A12Device)))
            {
               /* This patch code is not for this device, so skip       */
               /* sending it.                                           */
               Index++;
               continue;
            }


            /* Check to see if this is the Warm Reset and a Patch       */
            /* Callback has been specified.                             */
            if((PSKeyStr[Index].ExpectResponse == PATCH_DATA_WARM_RESET_ID) && (PatchCallback))
            {
               /* Call the user to provide mode PSKey data.             */
               (*PatchCallback)(BluetoothStackID, ControllerID, PatchCallbackParameter);
            }

            /* Assign the next sequence number and send the command to  */
            /* the controller.                                          */
            ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&Buffer[PSKEY_DATA_SEQ_NUM_OFFSET], SeqNum);
            ret_val = HCI_Send_Raw_Command(BluetoothStackID, HCI_COMMAND_CODE_VENDOR_SPECIFIC_DEBUG_OGF, 0, Length, Buffer, &Status, &BufferLength, Buffer, (Boolean_t)(PSKeyStr[Index].ExpectResponse?TRUE:FALSE));
            if(ret_val)
               break;

            Index++;
            SeqNum++;
         }

         /* Delay after the warm reset to allow the chip time to start  */
         /* up.                                                         */
         BTPS_Delay(40);

         /* Check to see if we are using a non UART mode.  If so, then  */
         /* we need to resync.                                          */
         if((!ret_val) && (COMMDriverInformation.Protocol != cpUART) && (COMMDriverInformation.Protocol != cpUART_RTS_CTS))
         {
            COMReconfigureData.ReconfigureFlags = HCI_COMM_RECONFIGURE_INFORMATION_RECONFIGURE_FLAGS_CHANGE_PROTOCOL;
            COMReconfigureData.Protocol         = COMMDriverInformation.Protocol;

            DriverReconfigureData.ReconfigureCommand = HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_COMM_PARAMETERS;
            DriverReconfigureData.ReconfigureData    = (void *)&COMReconfigureData;

            /* Finished with all patches and commands, go ahead and     */
            /* force a resychronize (note the last thing we did was to  */
            /* issue a Warm Reset so the new state machine needs to be  */
            /* resynchronized).                                         */
            HCI_ReconfigureDriver(HCIDriverID, TRUE, &DriverReconfigureData);
         }
      }
   }

   return((Boolean_t)(ret_val == 0));
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality after the HCI layer has issued any  */
   /* HCI Reset as part of the initialization.  This function is called */
   /* after all HCI functionality is established, just after the initial*/
   /* HCI Reset is sent to the stack.  The function should return a     */
   /* BOOLEAN TRUE to indicate successful completion or should return   */
   /* FALSE to indicate unsuccessful completion.  If an error is        */
   /* returned the stack will fail the initialization process.          */
   /* * NOTE * At the time this function is called HCI Driver and HCI   */
   /*          layer functions can be called, however no other stack    */
   /*          layer functions are able to be called at this time (hence*/
   /*          the HCI Driver ID and the Bluetooth Stack ID passed to   */
   /*          this function).                                          */
Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIReset(unsigned int HCIDriverID, unsigned int BluetoothStackID)
{
   return(TRUE);
}

   /* The following function prototype represents the vendor specific   */
   /* function which would is used to implement any needed Bluetooth    */
   /* device vendor specific functionality before the HCI layer is      */
   /* closed.  This function is called at the start of the HCI_Cleanup()*/
   /* function (before the HCI layer is closed), at which time all HCI  */
   /* functions are still operational.  The caller is NOT able to call  */
   /* any other stack functions other than the HCI layer and HCI Driver */
   /* layer functions because the stack is being shutdown (i.e.         */
   /* something has called BSC_Shutdown()).  The caller is free to      */
   /* return either success (TRUE) or failure (FALSE), however, it will */
   /* not circumvent the closing down of the stack or of the HCI layer  */
   /* or HCI Driver (i.e. the stack ignores the return value from this  */
   /* function).                                                        */
   /* * NOTE * At the time this function is called HCI Driver and HCI   */
   /*          layer functions can be called, however no other stack    */
   /*          layer functions are able to be called at this time (hence*/
   /*          the HCI Driver ID and the Bluetooth Stack ID passed to   */
   /*          this function).                                          */
Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIClose(unsigned int HCIDriverID, unsigned int BluetoothStackID)
{
   return(TRUE);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to implement any needed Bluetooth device   */
   /* vendor specific functionality after the entire Bluetooth Stack is */
   /* closed.  This function is called during the HCI_Cleanup()         */
   /* function, after the HCI Driver has been closed.  The caller is    */
   /* free return either success (TRUE) or failure (FALSE), however, it */
   /* will not circumvent the closing down of the stack as all layers   */
   /* have already been closed.                                         */
   /* * NOTE * No Stack calls are possible in this function because the */
   /*          entire stack has been closed down at the time this       */
   /*          function is called.                                      */
Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIClose(void)
{
   return(TRUE);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to enable a specific vendor specific       */
   /* feature.  This can be used to reconfigure the chip for a specific */
   /* feature (i.e. if a special configuration/patch needs to be        */
   /* dynamically loaded it can be done in this function).  This        */
   /* function returns TRUE if the feature was able to be enabled       */
   /* successfully, or FALSE if the feature was unable to be enabled.   */
   /* * NOTE * This functionality is not normally supported by default  */
   /*          (i.e. a custom stack build is required to enable this    */
   /*          functionality).                                          */
Boolean_t BTPSAPI HCI_VS_EnableFeature(unsigned int BluetoothStackID, unsigned long Feature)
{
   return(FALSE);
}

   /* The following function prototype represents the vendor specific   */
   /* function which is used to enable a specific vendor specific       */
   /* feature.  This can be used to reconfigure the chip for a specific */
   /* feature (i.e. if a special configuration/patch needs to be        */
   /* dynamically loaded it can be done in this function).  This        */
   /* function returns TRUE if the feature was able to be disabled      */
   /* successfully, or FALSE if the feature was unable to be disabled.  */
   /* * NOTE * This functionality is not normally supported by default  */
   /*          (i.e. a custom stack build is required to enable this    */
   /*          functionality).                                          */
Boolean_t BTPSAPI HCI_VS_DisableFeature(unsigned int BluetoothStackID, unsigned long Feature)
{
   return(FALSE);
}

   /* The following function prototype represents method for passing    */
   /* Vendor Specific information to this module.  This is used to pass */
   /* controller information needed at the time of patching to this     */
   /* module.  This function needs to be called before the stack is     */
   /* opened via a call to BSC_Initialize().  This function returns TRUE*/
   /* if the parameters were successfully cached.                       */
   /* * NOTE * The information passed in the VendParams_t structure is  */
   /*          specific to the controller and should be defined in      */
   /*          BTPSVEND.h.                                              */
   /* * NOTE * Processing the parameters in the VendParams_t structure  */
   /*          is vendor/controller specific.                           */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_SetParams(VendParams_t Params)
{
   /* Save the parameters and flag that they are valid.                 */
   VendParams      = Params;
   VendParamsValid = TRUE;

   return(TRUE);
}

   /* The following function is used to hook the patching process of the*/
   /* controller and MUST be called prior to the call to                */
   /* BSC_Initialize().  When registered, the callback is dispatched    */
   /* after all the default patching is complete and prior to the       */
   /* sending of a Warm Reset.  This provides a mechanism for           */
   /* programming additional PSKeys or replacing any of the default     */
   /* values.                                                           */
BTPSAPI_DECLARATION void BTPSAPI HCI_Register_Patch_Callback(HCI_Patch_Callback_t HCI_PatchCallback, unsigned long CallbackParameter)
{
   /* Save the Callback information.                                    */
   PatchCallback          = HCI_PatchCallback;
   PatchCallbackParameter = CallbackParameter;
}

   /* The follow function is used to set a PSKey value in the CSR       */
   /* controller.  All PS Keys consist of an array of 16 bit values.    */
   /* The function takes the BluetoothStackID of the stack associated   */
   /* with the controller.  The PSKeyID identifies the PS Key that is to*/
   /* be set.  The KeyLength parameter indicates the number of Word     */
   /* values that are to be written to the controller.  The KeyData     */
   /* parameter is a pointer to the PS Key data to be sent to the       */
   /* controller.  The CmdStatus parameter receives the status value    */
   /* returned from by the controller.  Some PS Key parameters require a*/
   /* reset of the controller for the change to take effect.            */
BTPSAPI_DECLARATION int BTPSAPI HCI_VS_SetPSKey(unsigned int BluetoothStackID, Word_t PSKeyID, int KeyLength, Word_t *KeyData, Word_t *CmdStatus, Boolean_t PerformReset)
{
   int     ret_val;
   Byte_t  PSKeyCmdLen;
   Byte_t  Status;
   Byte_t *CmdBuffer;

   /* Verify that the parameters passed in appear to be valid.          */
   if((PSKeyID) && (KeyLength) && (KeyData) && (CmdStatus))
   {
      /* Calculate the size of the buffer that will be required to build*/
      /* the command.                                                   */
      PSKeyCmdLen   = (Byte_t)(BCCMD_PS_HEADER_DATA_SIZE + (KeyLength << 1));
      CmdBuffer     = (Byte_t *)BTPS_AllocateMemory(PSKeyCmdLen);
      if(CmdBuffer)
      {
         /* Build the PSKEY Set command and send the command to the     */
         /* controller.                                                 */
         ((BCCMD_PS_Header_t *)CmdBuffer)->ChannelID = (CHANNEL_ID_FLAG_FIRST_PACKET | CHANNEL_ID_FLAG_LAST_PACKET | BCCMD_CHANNEL_ID);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->CmdType), BCCMD_TYPE_SETREQ);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->TotalLength), (BCCMD_PS_HEADER_TOTAL_LENGTH+KeyLength));
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->SequenceNumber), SeqNum);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->VARID), BCCMD_VARID_PS);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->Status), 0);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSKeyID), PSKeyID);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSKeyLength), KeyLength);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSStore), BCCMD_PS_STORE_DEFAULT);
         BTPS_MemCopy(&CmdBuffer[BCCMD_PS_HEADER_DATA_SIZE], KeyData, (KeyLength << 1));
         SeqNum++;
         ret_val = HCI_Send_Raw_Command(BluetoothStackID, HCI_COMMAND_CODE_VENDOR_SPECIFIC_DEBUG_OGF, 0, PSKeyCmdLen, CmdBuffer, &Status, &PSKeyCmdLen, CmdBuffer, TRUE);
         if(!ret_val)
         {
            /* Read the Status value from the response.                 */
            *CmdStatus = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((BCCMD_PS_Header_t *)CmdBuffer)->Status));
            if((!(*CmdStatus)) && (PerformReset))
            {
               /* Build the Warm Reset command and send it to the       */
               /* controller.  Since the response does not adhere to the*/
               /* Bluetooth response format, we will perform a small    */
               /* delay after sending the command instead of waiting for*/
               /* a response.                                           */
               PSKeyCmdLen                                 = (Byte_t)(BCCMD_PS_HEADER_DATA_SIZE + (1 << 1));
               ((BCCMD_PS_Header_t *)CmdBuffer)->ChannelID = (CHANNEL_ID_FLAG_FIRST_PACKET | CHANNEL_ID_FLAG_LAST_PACKET | BCCMD_CHANNEL_ID);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->CmdType), BCCMD_TYPE_SETREQ);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->TotalLength), (BCCMD_PS_HEADER_TOTAL_LENGTH+1));
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->SequenceNumber), SeqNum);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->VARID), BCCMD_VARID_WARM_RESET);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->Status), 0);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSKeyID), 0);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSKeyLength), 0);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSStore), 0);
               ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(CmdBuffer[BCCMD_PS_HEADER_DATA_SIZE]), 0);
               SeqNum++;
               HCI_Send_Raw_Command(BluetoothStackID, HCI_COMMAND_CODE_VENDOR_SPECIFIC_DEBUG_OGF, 0, PSKeyCmdLen, CmdBuffer, &Status, &PSKeyCmdLen, CmdBuffer, FALSE);
               BTPS_Delay(50);
            }
         }
      }
      else
         ret_val = BTPS_ERROR_MEMORY_ALLOCATION_ERROR;
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}

   /* The following function is used to retrieve PS Key data from the   */
   /* CSR controller.  All PS Keys consist of an array of 16 bit values.*/
   /* The function takes the BluetoothStackID of the stack associated   */
   /* with the controller.  The PSKeyID identifies the PS Key that is to*/
   /* be read.  The BCCMD_Cmd_Result structure is used to receive the   */
   /* result of the read.  Prior to calling this function the user must */
   /* initialize the BCCMD_Cmd_Result structure to indicate the size of */
   /* the buffer that is being provided to receive the result data.  The*/
   /* Status parameter will indicate the status of the read function.   */
   /* On a successful read, the Length parameter will indicates the     */
   /* number of Word values that were copied to the result buffer on    */
   /* return.                                                           */
   /* * NOTE * The receive buffer must be large enough to receive the   */
   /*          entire PS Key data.                                      */
BTPSAPI_DECLARATION int BTPSAPI HCI_VS_GetPSKey(unsigned int BluetoothStackID, Word_t PSKeyID, BCCMD_Cmd_Result_t *BCCMD_Cmd_Result)
{
   int     ret_val;
   Byte_t  PSKeyCmdLen;
   Byte_t  VS_Status;
   Byte_t *CmdBuffer;

   /* Verify that the parameters passed in appear valid.                */
   if((PSKeyID) && (BCCMD_Cmd_Result) && (BCCMD_Cmd_Result->ResultBuffer) && (BCCMD_Cmd_Result->ResultBufferLength))
   {
      /* Calculate the size of the buffer required to send and retrieve */
      /* the PS Key data.                                               */
      PSKeyCmdLen  = (Byte_t)(BCCMD_PS_HEADER_DATA_SIZE + (BCCMD_Cmd_Result->ResultBufferLength << 1));
      CmdBuffer    = (Byte_t *)BTPS_AllocateMemory(PSKeyCmdLen);
      if(CmdBuffer)
      {
         /* Build the PSKEY Get command and send the command to the     */
         /* controller.                                                 */
         ((BCCMD_PS_Header_t *)CmdBuffer)->ChannelID = (CHANNEL_ID_FLAG_FIRST_PACKET | CHANNEL_ID_FLAG_LAST_PACKET | BCCMD_CHANNEL_ID);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->CmdType), BCCMD_TYPE_GETREQ);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->TotalLength), (BCCMD_PS_HEADER_TOTAL_LENGTH+BCCMD_Cmd_Result->ResultBufferLength));
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->SequenceNumber), SeqNum++);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->VARID), BCCMD_VARID_PS);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->Status), 0);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSKeyID), PSKeyID);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSKeyLength), BCCMD_Cmd_Result->ResultBufferLength);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSStore), BCCMD_PS_STORE_DEFAULT);
         ASSIGN_HOST_WORD_TO_LITTLE_ENDIAN_UNALIGNED_WORD(&(CmdBuffer[BCCMD_PS_HEADER_DATA_SIZE]), 0);
         BTPS_MemInitialize(&CmdBuffer[BCCMD_PS_HEADER_DATA_SIZE], 0, (BCCMD_Cmd_Result->ResultBufferLength << 1));
         ret_val = HCI_Send_Raw_Command(BluetoothStackID, HCI_COMMAND_CODE_VENDOR_SPECIFIC_DEBUG_OGF, 0, PSKeyCmdLen, CmdBuffer, &VS_Status, &PSKeyCmdLen, (Byte_t *)CmdBuffer, TRUE);
         if(!ret_val)
         {
            /* Retrieve the result of the command.                      */
            BCCMD_Cmd_Result->ResultStatus = READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((BCCMD_PS_Header_t *)CmdBuffer)->Status));
            if(!(BCCMD_Cmd_Result->ResultStatus))
            {
               /* Copy the result data to the receive buffer.           */
               BCCMD_Cmd_Result->ResultBufferLength = (Byte_t)READ_UNALIGNED_WORD_LITTLE_ENDIAN(&(((BCCMD_PS_Header_t *)CmdBuffer)->PSKeyLength));
               BTPS_MemCopy(BCCMD_Cmd_Result->ResultBuffer, &CmdBuffer[BCCMD_PS_HEADER_DATA_SIZE], (BCCMD_Cmd_Result->ResultBufferLength << 1));
            }
            else
               BCCMD_Cmd_Result->ResultBufferLength = 0;
         }
         else
            BCCMD_Cmd_Result->ResultBufferLength = 0;
      }
      else
         ret_val = BTPS_ERROR_MEMORY_ALLOCATION_ERROR;
   }
   else
      ret_val = BTPS_ERROR_INVALID_PARAMETER;

   return(ret_val);
}


