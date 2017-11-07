#ifndef _AJ_TARGET_NVRAM_H_
#define _AJ_TARGET_NVRAM_H_

/**
 * @file
 */
/******************************************************************************
* Copyright 2013, Qualcomm Connected Experiences, Inc.
*
******************************************************************************/

#include "alljoyn.h"
#include "qcom/qcom_dset.h"
#include "dsetid.h"

/*
 * Identifies an AJ NVRAM block
 */
#define AJ_NV_SENTINEL ('A' | ('J' << 8) | ('N' << 16) | ('V' << 24))
#define INVALID_ID (0)
#define INVALID_DATA (0xFFFF)
#define INVALID_DATA_BYTE (0xFF)
#define SENTINEL_OFFSET (4)
#define WORD_ALIGN(x) ((x & 0x3) ? ((x >> 2) + 1) << 2 : x)
//#define AJ_NVRAM_SIZE (2024)
//LGD_AJ_NVM
//#define AJ_NVRAM_SIZE (1600)
#define FLASH_PARAM_BASE_OFFSET 0x7e000
#define PARAM_PARTITION_SIZE  4096
#ifndef AJ_NVRAM_SIZE
#define AJ_NVRAM_SIZE (512)  
#endif

/* Ruby DSET IDs */
#define AJ_RW_DATA_ID       DSETID_ALLJOYN_START+0
#define AJ_FACTORY_DATA_ID  DSETID_ALLJOYN_START+1
#define AJ_RO_DATA_ID       DSETID_ALLJOYN_START+2
#define AJ_JS_SCRIPT        DSETID_ALLJOYN_START+3

typedef struct _NV_EntryHeader {
    uint16_t id;           /**< The unique id */
    uint16_t capacity;     /**< The data set size */
} NV_EntryHeader;

#define ENTRY_HEADER_SIZE (sizeof(NV_EntryHeader))
#define AJ_NVRAM_END_ADDRESS (AJ_NVRAM_BASE_ADDRESS + AJ_NVRAM_SIZE)

/**
 * Write a block of data to NVRAM
 *
 * @param dest  Pointer a location of NVRAM
 * @param buf   Pointer to data to be written
 * @param size  The number of bytes to be written
 */
//void _AJ_NV_Write(void* dest, void* buf, uint16_t size, uint8_t flashFlag);
void _AJ_NV_Write(void* dest, void* buf, uint16_t size);

/**
 * Read a block of data from NVRAM
 *
 * @param src   Pointer a location of NVRAM
 * @param buf   Pointer to data to be written
 * @param size  The number of bytes to be written
 */
void _AJ_NV_Read(void* src, void* buf, uint16_t size);

/**
 * Erase the whole NVRAM sector and write the sentinel data
 */
void _AJ_NVRAM_Clear();

#endif

