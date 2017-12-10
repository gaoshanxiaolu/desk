/******************************************************************************
    Copyright (C) Cambridge Silicon Radio Ltd 2011

FILE
    csr1k_host_boot.c

DESCRIPTION
    This file contains example code to boot CSR1000 or CSR1001 over the
    debug SPI port.

    This code has been developed for an NXP LPC210x processor and run on
    an Olimex LPC-P2106-B low cost development board.

    The tool chain is WINARM and used the "Programmers Notepad 2" IDE 
    although a makefile build should be possible.

DISCLAIMER
    This program is distributed in the hope that it will be useful,
    but WITHOUT ANY WARRANTY; without even the implied warranty of
    MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  
    
    It is only for use with Cambridge Silicon Radio's CSR1000/1001 parts.
    
    It must not be disclosed to any 3rd parties without the express written
    permission of CSR.

******************************************************************************/


#include "csr1k_host_boot.h"

static void CSRspiTXword(uint16 addr, uint16 tx_data16);
static void CSRspiTXblock(uint16 addr, uint16 *ptx_data16, uint16 length);
static void spiStartTransaction(uint8 r_or_w, uint16 addr);
static void spiClock2Cyles(void);
static void spiStopTransaction(void);
static void spiTXword(uint16 tx_data16);


/*--------------------------------------------------------------------------*/
/* CSR1000/1001 boot code is here  lives here                               */
/*--------------------------------------------------------------------------*/

/* Include auto generated header file of constants, approx 50 kb as 25 kw   */
#define UWORD16 uint16
#include "xap_code.h"
#include "qcom/qcom_gpio.h"

#define CS_PIN		0
#define MOSI_PIN	1
#define MISO_PIN	4
#define CLK_PIN		5

#define SPI_CLK_DELAY_TIME 1

#define SPI_CLK_WAIT	spi_delay(SPI_CLK_DELAY_TIME)

void spi_delay(int time)
{
	while(time--)
		;
}

void gpio_spi_init(void)
{
	qcom_gpio_apply_peripheral_configuration(QCOM_PERIPHERAL_ID_GPIOn(CS_PIN), 0);
	qcom_gpio_apply_peripheral_configuration(QCOM_PERIPHERAL_ID_GPIOn(MISO_PIN), 0);
	qcom_gpio_apply_peripheral_configuration(QCOM_PERIPHERAL_ID_GPIOn(MOSI_PIN), 0);
	qcom_gpio_apply_peripheral_configuration(QCOM_PERIPHERAL_ID_GPIOn(CLK_PIN), 0);

	qcom_gpio_pin_dir(MOSI_PIN, 0);
	qcom_gpio_pin_dir(MISO_PIN, 1);
	qcom_gpio_pin_dir(CLK_PIN, 0);
	qcom_gpio_pin_dir(CS_PIN, 0);

	qcom_gpio_pin_set(CS_PIN, 1);
	qcom_gpio_pin_set(CLK_PIN, 0);
	qcom_gpio_pin_set(MOSI_PIN, 1);

}

void gpio_spi_cs(int level)
{
	qcom_gpio_pin_set(CS_PIN, level);
}

void gpio_spi_tx_byte(uint8 tx_data8)
{
	int i = 0;

	for(i=7;i>=0;i--){
		qcom_gpio_pin_set(CLK_PIN, 0);
		SPI_CLK_WAIT;
		qcom_gpio_pin_set(MOSI_PIN, 0x01&(tx_data8>>i));
		SPI_CLK_WAIT;
		qcom_gpio_pin_set(CLK_PIN, 1);
		SPI_CLK_WAIT;
		SPI_CLK_WAIT;
	}

	qcom_gpio_pin_set(CLK_PIN, 0);
	SPI_CLK_WAIT;
	
}


void CSR1Kreboot(void)
{
    gpio_spi_init();

    CSRspiTXword(0xF82F, 1);
    timeDelayInMS(15);
    CSRspiTXword(0xF81D, 2);

    /* Set PC to zero */
    CSRspiTXword(0xFFEA, 0);
    CSRspiTXword(0xFFE9, 0);

    /* And GO (which is really an "un-stop" :) */
    CSRspiTXword(0xF81D, 0);
}

void CSR1Kboot(void)
{
    uint16 i;
    uint16 addr;
    uint16 len;
    uint16 *pbuf;

    gpio_spi_init(); //need "testgpio" to init it.

    /* Reset the chip.  No need to clear this later, it is self clearing */
    CSRspiTXword(0xF82F, 1);

    /* wait for the interal boot hardware in CSR1000 to give up trying to find
     * an image in external EEPROM or SPI Flash to download (that takes ~11ms).
     * While the chip is checking for memory we can't boot the chip over SPI.
     */
    timeDelayInMS(15);

    /* STOP the processor */
    CSRspiTXword(0xF81D, 2);

    /* download each section as a block transfer till we're done.
     * the xap code structure is produced by the "xuv_to_h" utility
     */
    for (i = 0; i < XAP_CODE_STRUCT_SIZE; i += len)
    {
        addr = xap_code_struct[i++];
        len  = xap_code_struct[i++];
        pbuf = (uint16 *) &xap_code_struct[i];

        CSRspiTXblock(addr, pbuf, len);
		printf("addr=%x,len=%d\n",addr,len);
    }

    /* Magic toggle */
    CSRspiTXword(0x0018, 1);
    CSRspiTXword(0x0018, 0);

    /* Set PC to zero */
    CSRspiTXword(0xFFEA, 0);
    CSRspiTXword(0xFFE9, 0);

    /* And GO (which is really an "un-stop" :) */
    CSRspiTXword(0xF81D, 0);

}


/*--------------------------------------------------------------------------*/
/* Basic CSR SPI primitives live here                                       */
/*--------------------------------------------------------------------------*/

/* CSR SPI write command.  Completely proprietary */
#define CSR_SPI_CMD_WRITE           0x02

static void CSRspiTXword(uint16 addr, uint16 tx_data16)
{
    spiStartTransaction(CSR_SPI_CMD_WRITE, addr);

    spiTXword(tx_data16);

    spiStopTransaction();
}

static void CSRspiTXblock(uint16 addr, uint16 *ptx_data16, uint16 length)
{	
    spiStartTransaction(CSR_SPI_CMD_WRITE, addr);

    while (length--)
    {
        spiTXword(*ptx_data16++);
    }

    spiStopTransaction();
    
}

    
/*--------------------------------------------------------------------------*/
/* CSR SPI helper functions live here                                       */
/*--------------------------------------------------------------------------*/

static void spiStartTransaction(uint8 r_or_w, uint16 addr)
{
    /* Note:  It is assummed that CSB is already high */
    qcom_gpio_pin_set(CS_PIN, 0);
    SPI_CLK_WAIT;
    SPI_CLK_WAIT;
    qcom_gpio_pin_set(CS_PIN, 1);
    timeDelayInMS(2);

    gpio_spi_cs(1);
    spiClock2Cyles();       /* so called "clocking in" */
    SPI_CLK_WAIT;
    gpio_spi_cs(0);
    SPI_CLK_WAIT;

    gpio_spi_tx_byte(r_or_w);

    spiTXword(addr);

}

static void spiClock2Cyles(void)
{

       qcom_gpio_pin_set(CLK_PIN, 0);
	SPI_CLK_WAIT;
	SPI_CLK_WAIT;
	qcom_gpio_pin_set(CLK_PIN, 1);
	SPI_CLK_WAIT;
	SPI_CLK_WAIT;
	qcom_gpio_pin_set(CLK_PIN, 0);
	SPI_CLK_WAIT;
	SPI_CLK_WAIT;
	qcom_gpio_pin_set(CLK_PIN, 1);
	SPI_CLK_WAIT;
	SPI_CLK_WAIT;
	
}

static void spiStopTransaction(void)
{
	SPI_CLK_WAIT;
	gpio_spi_cs(1);
	SPI_CLK_WAIT;
	qcom_gpio_pin_set(CLK_PIN, 1);

	spiClock2Cyles();   /* so called "clocking out */
}

static void spiTXword(uint16 tx_data16)
{
    gpio_spi_tx_byte((uint8)(0xff & (tx_data16 >> 8)));
    gpio_spi_tx_byte((uint8)(0xff &  tx_data16));
}

/* csr1k_host_boot.c */
