/*****< bvendapi.h >***********************************************************/
/*      Copyright 2008 - 2012 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  BVENDAPI - Vendor specific functions/definitions/constants used to define */
/*             a set of vendor specific functions supported by the Bluetopia  */
/*             Protocol Stack.  These functions may be unique to a given      */
/*             hardware platform.                                             */
/*                                                                            */
/*  Author:  Damon Lange                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   06/09/09  D. Lange       Initial creation.                               */
/******************************************************************************/
#ifndef __BVENDAPIH__
#define __BVENDAPIH__

#include "SS1BTPS.h"            /* Bluetopia API Prototypes/Constants.        */

   /* The following define the status values that can be returned in the*/
   /* BCCMD_Result_t structure.                                         */
#define BCCMD_CMD_STATUS_OK                              0x0000
#define BCCMD_CMD_STATUS_NO_SUCH_VARID                   0x0001
#define BCCMD_CMD_STATUS_TOO_BIG                         0x0002
#define BCCMD_CMD_STATUS_NO_VALUE                        0x0003
#define BCCMD_CMD_STATUS_BAD_REQ                         0x0004
#define BCCMD_CMD_STATUS_NO_ACCESS                       0x0005
#define BCCMD_CMD_STATUS_READ_ONLY                       0x0006
#define BCCMD_CMD_STATUS_WRITE_ONLY                      0x0007
#define BCCMD_CMD_STATUS_ERROR                           0x0008
#define BCCMD_CMD_STATUS_PERMISSION_DENIED               0x0009

   /* The following define the Controller ID values.                    */
#define CONTROLLER_ID_CSR8X11_A08                        0x2031
#define CONTROLLER_ID_CSR8X11_A12                        0x2918

   /* The following defines the structure used to receive the results of*/
   /* a BCCMD Command.  The result data contained in the ResultBuffer   */
   /* consists of an array of WORD values that make up the result.  The */
   /* ResultBufferLength value defines the number of WORD values that   */
   /* provided in the ResultBuffer.  The ResultStatus parameter is used */
   /* to indicate the success or failure of the command.                */
   /* * NOTE * The ResultBuffer must be large enough to hold the entire */
   /*          response value.                                          */
   /* * NOTE * On return, the ResultBufferLength will indicate the      */
   /*          number of WORD values that has been placed in            */
   /*          ResultBuffer.                                            */
typedef struct _tagBCCMD_Cmd_Result_t
{
   Byte_t  ResultBufferLength;
   Word_t *ResultBuffer;
   Word_t  ResultStatus;
} BCCMD_Cmd_Result_t;

#define BCCMD_CMD_RESULT_DATA_SIZE            (sizeof(BCCMD_Cmd_Result_t))

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIOpen(HCI_DriverInformation_t *HCI_DriverInformation);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeBeforeHCIOpen_t)(HCI_DriverInformation_t *HCI_DriverInformation);
#endif

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIOpen(unsigned int HCIDriverID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeAfterHCIOpen_t)(unsigned int HCIDriverID);
#endif

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIReset(unsigned int HCIDriverID, unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeBeforeHCIReset_t)(unsigned int HCIDriverID, unsigned int BluetoothStackID);
#endif

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIReset(unsigned int HCIDriverID, unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeAfterHCIReset_t)(unsigned int HCIDriverID, unsigned int BluetoothStackID);
#endif

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeBeforeHCIClose(unsigned int HCIDriverID, unsigned int BluetoothStackID);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeBeforeHCIClose_t)(unsigned int HCIDriverID, unsigned int BluetoothStackID);
#endif

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_InitializeAfterHCIClose(void);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_InitializeAfterHCIClose_t)(void);
#endif

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_EnableFeature(unsigned int BluetoothStackID, unsigned long Feature);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_EnableFeature_t)(unsigned int BluetoothStackID, unsigned long Feature);
#endif

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
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_DisableFeature(unsigned int BluetoothStackID, unsigned long Feature);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_DisableFeature_t)(unsigned int BluetoothStackID, unsigned long Feature);
#endif

   /* The following function prototype represents method for passing    */
   /* Vendor Specific information to this module.  This is used to pass */
   /* controller information needed at the time of patching to this     */
   /* module.  This function needs to be called before the stack is     */
   /* opened via a call to BSC_Initialize().  This function returns TRUE*/
   /* if the parameters were successfully cached.                       */
   /* * NOTE * The information passed in the VendParams_t structure is  */
   /*          specific to the controller and should be defined in      */
   /*          BTPSVEND.h.                                              */
BTPSAPI_DECLARATION Boolean_t BTPSAPI HCI_VS_SetParams(VendParams_t Params);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef Boolean_t (BTPSAPI *PFN_HCI_VS_SetParams_t)(VendParams_t Params);
#endif

   /* The following declared type represents the Prototype Function for */
   /* an HCI_Patch Callback.  This function will be called whenever the */
   /* controller needs to be patched during controller initialization   */
   /* process of the Bluetopia Stack.  This function passes to the      */
   /* caller the Bluetooth Stack ID, the Controller ID, and the Callback*/
   /* Parameter that was specified when this Callback was installed.    */
   /* * NOTE * This function is dispatched to the caller before         */
   /*          performing a Warm Reset of the controller.               */
   /* * NOTE * The callback provides the Controller ID to assist in the */
   /*          programming of the appropriate keys.                     */
   /* * NOTE * The HCI_VS_SetPSKey() function may be used to assist in  */
   /*          the programming of the keys.  However, the PerformReset  */
   /*          parameter must be set to FALSE.                          */
typedef void (BTPSAPI *HCI_Patch_Callback_t)(unsigned int BluetoothStackID, unsigned int ControllerID, unsigned long CallbackParameter);

   /* The following function is used to hook the patching process of the*/
   /* controller and MUST be called prior to the call to                */
   /* BSC_Initialize().  When registered, the callback is dispatched    */
   /* after all the default patching is complete and prior to the       */
   /* sending of a Warm Reset.  This provides a mechanism for           */
   /* programming additional PSKeys or replacing any of the default     */
   /* values.                                                           */
BTPSAPI_DECLARATION void BTPSAPI HCI_Register_Patch_Callback(HCI_Patch_Callback_t HCI_PatchCallback, unsigned long CallbackParameter);
#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef void (BTPSAPI *PFN_HCI_Register_Patch_Callback_t)(HCI_Patch_Callback_t HCI_PatchCallback, unsigned long CallbackParameter);
#endif

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
BTPSAPI_DECLARATION int BTPSAPI HCI_VS_SetPSKey(unsigned int BluetoothStackID, Word_t PSKeyID, int KeyLength, Word_t *KeyData, Word_t *CmdStatus, Boolean_t PerformReset);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_HCI_VS_SetPSKey_t)(unsigned int BluetoothStackID, Word_t PSKeyID, int KeyLength, Word_t *KeyData, Word_t *CmdStatus, Boolean_t PerformReset);
#endif

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
BTPSAPI_DECLARATION int BTPSAPI HCI_VS_GetPSKey(unsigned int BluetoothStackID, Word_t PSKeyID, BCCMD_Cmd_Result_t *BCCMD_Cmd_Result);

#ifdef INCLUDE_BLUETOOTH_API_PROTOTYPES
   typedef int (BTPSAPI *PFN_HCI_VS_GetPSKey_t)(unsigned int BluetoothStackID, Word_t PSKeyID, BCCMD_Cmd_Result_t *BCCMD_Cmd_Result);
#endif

#endif
