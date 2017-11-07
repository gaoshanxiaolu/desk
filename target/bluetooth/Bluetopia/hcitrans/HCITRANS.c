/*****< hcitrans.c >***********************************************************/
/*      Copyright 2013 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  HCITRANS - HCI Transport Layer for use with Bluetopia.                    */
/*                                                                            */
/*  Author:  Marcus Funk                                                      */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   11/08/12  M. Funk        Initial creation.                               */
/******************************************************************************/
#include "tx_api.h"
#include "qcom/basetypes.h"
#include "qcom/base.h"
#include "qcom/qcom_uart.h"
#include "qcom/select_api.h"

#include "HCITRANS.h"       /* HCI Transport Prototypes/Constants.            */
#include "BTPSKRNL.h"

#define TRANSPORT_ID                         1
#define BT_UART_NAME                         "UART2"
#define BT_RX_THREAD_STACK_SIZE              2048
#define BT_RX_THREAD_PRIORITY                (16)
#define BT_RX_THREAD_TIME_SLICE              (4)
#define RX_BUFFER_SIZE                       (512)

typedef struct _tagUartContext_t
{
   A_UINT32                 BaudRate;
   HCI_COMM_Protocol_t      Protocol;
   A_INT32                  BT_FD;
   TX_THREAD                RxThread;
   A_UINT8                  RxThreadStack[BT_RX_THREAD_STACK_SIZE];
   HCITR_COMDataCallback_t  COMDataCallbackFunction;
   unsigned long            COMDataCallbackParameter;
} UartContext_t;

#define UART_CONTEXT_DATA_SIZE               (sizeof(UartContext_t))

static UartContext_t       *UartContext = NULL;

/**
 * Refer to header file for API description
 *
 */
A_INT16 SetBaudRate(A_UINT32 rate)
{
   return(1);
}

static A_UINT8  buffer[RX_BUFFER_SIZE];
static A_UINT32 buffer_length;
static void BT_RxThread(ULONG arg)
{
   int      ret_val;
   q_fd_set fd;

   while(1)
   {
      FD_ZERO(&fd);
      FD_SET(UartContext->BT_FD, &fd);

      ret_val = qcom_select(1, &fd, NULL, NULL, NULL);
      if((ret_val) && (FD_ISSET(UartContext->BT_FD, &fd)))
      {
         while(1)
         {
            buffer_length = RX_BUFFER_SIZE;
            ret_val = qcom_uart_read(UartContext->BT_FD, (A_CHAR *)buffer, &buffer_length);
            if((ret_val >= 0) && (buffer_length) && (UartContext->COMDataCallbackFunction))
            {
               (*UartContext->COMDataCallbackFunction)(TRANSPORT_ID, buffer_length, buffer, UartContext->COMDataCallbackParameter);
            }
            else
               break;
         }
      }
   }
}


  /* The following function is responsible for opening the HCI         */
  /* Transport layer that will be used by Bluetopia to send and receive*/
  /* COM (Serial) data.  This function must be successfully issued in  */
  /* order for Bluetopia to function.  This function accepts as its    */
  /* parameter the HCI COM Transport COM Information that is to be used*/
  /* to open the port.  The final two parameters specify the HCI       */
  /* Transport Data Callback and Callback Parameter (respectively) that*/
  /* is to be called when data is received from the UART.  A successful*/
  /* call to this function will return a non-zero, positive value which*/
  /* specifies the HCITransportID that is used with the remaining      */
  /* transport functions in this module.  This function returns a      */
  /* negative return value to signify an error.                        */
int BTPSAPI HCITR_COMOpen(HCI_COMMDriverInformation_t *COMMDriverInformation, HCITR_COMDataCallback_t COMDataCallback, unsigned long CallbackParameter)
{
   int            ret_val = 0;
   DWord_t        status;
   int            bt_fd;
   qcom_uart_para com_uart_cfg;

   /* First, make sure that the port is not already open and make sure  */
   /* that valid COMM Driver Information was specified.                 */
   if((!UartContext) && (COMMDriverInformation) && (COMDataCallback))
   {
      /* Initialize the Uarts if they have not already been initialized.*/
      qcom_uart_init();
      com_uart_cfg.BaudRate    = COMMDriverInformation->BaudRate;
      com_uart_cfg.number      = 8;
      com_uart_cfg.StopBits    = 1;
      com_uart_cfg.parity      = 0;

      if((COMMDriverInformation->Protocol == cpUART_RTS_CTS) || (COMMDriverInformation->Protocol == cpH4DS_RTS_CTS) || (COMMDriverInformation->Protocol == cp3Wire_RTS_CTS))
         com_uart_cfg.FlowControl = 1;
      else
         com_uart_cfg.FlowControl = 0;

      qcom_set_uart_config(BT_UART_NAME, &com_uart_cfg);

      /* Attempt to Open the port.                                      */
      bt_fd = qcom_uart_open(BT_UART_NAME);
      if(bt_fd >= 0)
      {
         /* Allocate memory for the Uart Context.                       */
         UartContext = (UartContext_t *)BTPS_AllocateMemory(UART_CONTEXT_DATA_SIZE);
         if(UartContext)
         {
            /* Initialize the context structure.                        */
            memset(UartContext, 0, sizeof(UartContext_t));
            UartContext->BT_FD                    = bt_fd;
            UartContext->BaudRate                 = COMMDriverInformation->BaudRate;
            UartContext->Protocol                 = COMMDriverInformation->Protocol;
            UartContext->COMDataCallbackFunction  = COMDataCallback;
            UartContext->COMDataCallbackParameter = CallbackParameter;

            /* Start the read thread.                                     */
            status = tx_thread_create(&UartContext->RxThread, "BT_RxThread", BT_RxThread, 0, UartContext->RxThreadStack, BT_RX_THREAD_STACK_SIZE, BT_RX_THREAD_PRIORITY, BT_RX_THREAD_PRIORITY, BT_RX_THREAD_TIME_SLICE, TX_AUTO_START);
            if(status == TX_SUCCESS)
               ret_val = TRANSPORT_ID;
         }
      }
   }
   return(ret_val);
}

  /* The following function is responsible for closing the specific HCI*/
  /* Transport layer that was opened via a successful call to the      */
  /* HCITR_COMOpen() function (specified by the first parameter).      */
  /* Bluetopia makes a call to this function whenever an either        */
  /* Bluetopia is closed, or an error occurs during initialization and */
  /* the driver has been opened (and ONLY in this case).  Once this    */
  /* function completes, the transport layer that was closed will no   */
  /* longer process received data until the transport layer is         */
  /* Re-Opened by calling the HCITR_COMOpen() function.                */
  /* * NOTE * This function *MUST* close the specified COM Port.  This */
  /*          module will then call the registered COM Data Callback   */
  /*          function with zero as the data length and NULL as the    */
  /*          data pointer.  This will signify to the HCI Driver that  */
  /*          this module is completely finished with the port and     */
  /*          information and (more importantly) that NO further data  */
  /*          callbacks will be issued.  In other words the very last  */
  /*          data callback that is issued from this module *MUST* be a*/
  /*          data callback specifying zero and NULL for the data      */
  /*          length and data buffer (respectively).                   */
void BTPSAPI HCITR_COMClose(unsigned int HCITransportID)
{
   HCITR_COMDataCallback_t  _COMDataCallbackFunction;
   unsigned long            _COMDataCallbackParameter;

  /* Check to make sure that the specified Transport ID is valid.      */
   if((HCITransportID == TRANSPORT_ID) && (UartContext))
   {
      tx_thread_terminate(&UartContext->RxThread);
      tx_thread_delete(&UartContext->RxThread);

      qcom_uart_close(UartContext->BT_FD);

      _COMDataCallbackFunction = UartContext->COMDataCallbackFunction;
      _COMDataCallbackParameter = UartContext->COMDataCallbackParameter;

      BTPS_FreeMemory(UartContext);
      UartContext = NULL;

      if(_COMDataCallbackFunction)
         (*_COMDataCallbackFunction)(1, 0, NULL, _COMDataCallbackParameter);
   }
}

  /* The following function is responsible for instructing the         */
  /* specified HCI Transport layer (first parameter) that was opened   */
  /* via a successful call to the HCITR_COMOpen() function to          */
  /* reconfigure itself with the specified information.  This          */
  /* information is completely opaque to the upper layers and is passed*/
  /* through the HCI Driver layer to the transport untouched.  It is   */
  /* the responsibility of the HCI Transport driver writer to define   */
  /* the contents of this member (or completely ignore it).            */
  /* * NOTE * This function does not close the HCI Transport specified */
  /*          by HCI Transport ID, it merely reconfigures the          */
  /*          transport.  This means that the HCI Transport specified  */
  /*          by HCI Transport ID is still valid until it is closed    */
  /*          via the HCI_COMClose() function.                         */
void BTPSAPI HCITR_COMReconfigure(unsigned int HCITransportID, HCI_Driver_Reconfigure_Data_t *DriverReconfigureData)
{
   HCI_COMMReconfigureInformation_t *COMMReconfigureInformation;

   /* Check to make sure that the parameters are valid and the transport*/
   /* is open.                                                          */
   if((HCITransportID == TRANSPORT_ID) && (UartContext) && (DriverReconfigureData))
   {
      /* Confirm that we are trying to reconfigure the comm parameters. */
      if(DriverReconfigureData->ReconfigureCommand == HCI_COMM_DRIVER_RECONFIGURE_DATA_COMMAND_CHANGE_COMM_PARAMETERS)
      {
         COMMReconfigureInformation = (HCI_COMMReconfigureInformation_t *)(DriverReconfigureData->ReconfigureData);

         /* Confirm the COMM driver information appears valid and the   */
         /* flags indicate that the baud rate needs to change.          */
         if((COMMReconfigureInformation) && (COMMReconfigureInformation->ReconfigureFlags & HCI_COMM_RECONFIGURE_INFORMATION_RECONFIGURE_FLAGS_CHANGE_BAUDRATE))
         {
            /* Set the new baudrate.                                    */
            SetBaudRate(COMMReconfigureInformation->BaudRate);
         }
      }
   }
}

  /* The following function is responsible for actually sending data   */
  /* through the opened HCI Transport layer (specified by the first    */
  /* parameter).  Bluetopia uses this function to send formatted HCI   */
  /* packets to the attached Bluetooth Device.  The second parameter to*/
  /* this function specifies the number of bytes pointed to by the     */
  /* third parameter that are to be sent to the Bluetooth Device.  This*/
  /* function returns a zero if the all data was transferred           */
  /* successfully or a negative value if an error occurred.  This      */
  /* function MUST NOT return until all of the data is sent (or an     */
  /* error condition occurs).  Bluetopia WILL NOT attempt to call this */
  /* function repeatedly if data fails to be delivered.  This function */
  /* will block until it has either buffered the specified data or sent*/
  /* all of the specified data to the Bluetooth Device.                */
  /* * NOTE * The type of data (Command, ACL, SCO, etc.) is NOT passed */
  /*          to this function because it is assumed that this         */
  /*          information is contained in the Data Stream being passed */
  /*          to this function.                                        */
int BTPSAPI HCITR_COMWrite(unsigned int HCITransportID, unsigned int Length, unsigned char *Buffer)
{
   int          ret_val = 0;
   unsigned int len;

   /* Check to make sure that the specified Transport ID is valid and   */
   /* the output buffer appears to be valid as well.                    */
   if((HCITransportID == TRANSPORT_ID) && (UartContext) && (Length) && (Buffer))
   {
      while(Length)
      {
         len = Length;
         qcom_uart_write(UartContext->BT_FD, (A_CHAR *)Buffer, &len);
         if(len)
         {
            Buffer += len;
            Length -= len;
         }

         if(Length)
            BTPS_Delay(5);
      }
   }
   else
      ret_val = HCITR_ERROR_WRITING_TO_PORT;

   return(ret_val);
}
