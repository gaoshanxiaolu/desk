/**
 * @file
 */
/******************************************************************************
* Copyright 2013-2014, Qualcomm Connected Experiences, Inc.
*
******************************************************************************/

#define AJ_MODULE TARGET_NVRAM

#include "aj_nvram.h"
#include "aj_target_nvram.h"
#include "qcom/qcom_dset.h"
#include "dsetid.h"

#ifndef NDEBUG
uint8_t dbgTARGET_NVRAM = 0;
#endif

/*
 * Globals shared with core aj_nvram.c
 */
uint32_t AJ_EMULATED_NVRAM[AJ_NVRAM_SIZE/4];
uint8_t* AJ_NVRAM_BASE_ADDRESS;


static uint8_t* AJ_NVRAM_RW_ADDRESS;
static size_t AJ_NVRAM_RW_SIZE;

#define AJ_NVRAM_END ((void *) &AJ_EMULATED_NVRAM[AJ_NVRAM_SIZE/4])

#define MIN(a,b)    ((a)<(b)?(a):(b))

static int WriteToFlash()
{
    int32_t status = A_ERROR;
    uint32_t handle = 0;
    uint32_t sentinel_image = AJ_NV_SENTINEL;

    // overwrite the dset if it already exists
    qcom_dset_delete(AJ_RW_DATA_ID, DSET_MEDIA_NVRAM, NULL, NULL);

    // first create the dset (or overwrite if it already exists)
    status = qcom_dset_create(&handle, AJ_RW_DATA_ID, AJ_NVRAM_RW_SIZE + SENTINEL_OFFSET, DSET_MEDIA_NVRAM, NULL, NULL);
    if (status != A_OK) {
        AJ_ErrPrintf(("WriteToFlash: Failed to create RW dset\n"));
        return -1;
    }

    // write the AJNVRAM sentinel
    status = qcom_dset_write(
        handle, (uint8_t*)&sentinel_image, SENTINEL_OFFSET,
        0, DSET_MEDIA_NVRAM, NULL, NULL);

    if (status == A_OK) {
        // now write the entire emulated NVRAM chunk to the dset
        status = qcom_dset_write(
            handle, AJ_NVRAM_RW_ADDRESS, AJ_NVRAM_RW_SIZE,
            SENTINEL_OFFSET, DSET_MEDIA_NVRAM, NULL, NULL);

        if (status == A_OK) {
            // now commit
            qcom_dset_commit(handle, NULL, NULL);
        }
    }

    // finally, close the handle
    qcom_dset_close(handle, NULL, NULL);
    return AJ_OK;
}

static int ReadFromFlash()
{
    int32_t status;
    uint32_t handle;
    uint32_t sentinel = 0;
    uint16_t* data = (uint16_t*)(AJ_NVRAM_BASE_ADDRESS + SENTINEL_OFFSET);
    uint32_t size;
    int i;

    // attempt to open the dset
    status = qcom_dset_open(&handle, AJ_RO_DATA_ID, DSET_MEDIA_NVRAM, NULL, NULL);
    if (status == A_OK) {
        size = qcom_dset_size(handle);
        size = MIN(size, sizeof(AJ_EMULATED_NVRAM));

        // now read from the whole dset into RAM
        qcom_dset_read(handle, (A_UINT8*)AJ_EMULATED_NVRAM, size, 0, NULL, NULL);

        /* Don't use this data if no Sentinel */
        if (AJ_EMULATED_NVRAM[0] == AJ_NV_SENTINEL) {
            /*
             * Find end of RO data space
             */
            while (data < (uint16_t*)AJ_NVRAM_END && *data != INVALID_DATA) {
                uint16_t entrySize;
                NV_EntryHeader* header = (NV_EntryHeader*) data;
                entrySize = ENTRY_HEADER_SIZE + header->capacity;
                data += entrySize >> 1;
            }
        } else {
            AJ_WarnPrintf(("No sentinel found (AJ_RO_DATA_ID): %x != %x (%x ?)\n", sentinel, AJ_NV_SENTINEL));
        }

        // finally, close the handle
        qcom_dset_close(handle, NULL, NULL);
    }

    AJ_EMULATED_NVRAM[0] = AJ_NV_SENTINEL;
    AJ_NVRAM_RW_ADDRESS = (uint8_t *) data;
    AJ_NVRAM_RW_SIZE = sizeof(AJ_EMULATED_NVRAM) - (size_t) (AJ_NVRAM_RW_ADDRESS - ((uint8_t *) AJ_EMULATED_NVRAM));

    /* attempt to open and verify the RW dset */
    status = qcom_dset_open(&handle, AJ_RW_DATA_ID, DSET_MEDIA_NVRAM, NULL, NULL);
    if (status == A_OK) {
        /* Verify sentinel is correct */
        qcom_dset_read(handle, (A_UINT8*)&sentinel, SENTINEL_OFFSET, 0, NULL, NULL);
        if (sentinel != AJ_NV_SENTINEL) {
            AJ_WarnPrintf(("No sentinel found (AJ_RW_DATA_ID): %x != %x (%x ?)\n", sentinel, AJ_NV_SENTINEL));
            qcom_dset_close(handle, NULL, NULL);
        }
        /* If valid Image: leave handle open */
    }

    /* if RW dset missing/invalid, try the Factory Reset dset */
    if (sentinel != AJ_NV_SENTINEL) {
        status = qcom_dset_open(&handle, AJ_FACTORY_DATA_ID, DSET_MEDIA_NVRAM, NULL, NULL);
        if (status == A_OK) {
            /* Verify sentinel is correct */
            qcom_dset_read(handle, (A_UINT8*)&sentinel, SENTINEL_OFFSET, 0, NULL, NULL);
            if (sentinel != AJ_NV_SENTINEL) {
                AJ_WarnPrintf(("No sentinel found (AJ_FACTORY_DATA_ID): %x != %x (%x ?)\n", sentinel, AJ_NV_SENTINEL));
                qcom_dset_close(handle, NULL, NULL);
            }
            /* If valid Image: leave handle open */
        }
    }

    if (sentinel == AJ_NV_SENTINEL) {
        /* We have a valid RW image (it might be the RW dset or the Factory Reset dset) */
        /* read from the dset into RAM */
        size = qcom_dset_size(handle);
        size = MIN(size - SENTINEL_OFFSET, AJ_NVRAM_RW_SIZE);
        qcom_dset_read(handle, (A_UINT8*)AJ_NVRAM_RW_ADDRESS, size, SENTINEL_OFFSET, NULL, NULL);

        /* close the handle */
        qcom_dset_close(handle, NULL, NULL);
    }

    return AJ_OK;
}

void AJ_NVRAM_Init()
{
    AJ_NVRAM_BASE_ADDRESS = (uint8_t* )AJ_EMULATED_NVRAM;
    static uint8_t inited = FALSE;
    if (!inited) {
        inited = TRUE;

        AJ_InfoPrintf(("AJ_NVRAM_Init: One Time Initialization\n"));
        memset(AJ_NVRAM_BASE_ADDRESS, INVALID_DATA_BYTE, AJ_NVRAM_SIZE);

        if (ReadFromFlash() == AJ_OK) {
//          AJ_NVRAM_Layout_Print();
        }

        if (*((uint32_t*) AJ_NVRAM_BASE_ADDRESS) != AJ_NV_SENTINEL) {
            AJ_WarnPrintf(("AJ_NVRAM_Init: No sentinel found: %x != %x (%x ?)\n", *((uint32_t*) AJ_NVRAM_BASE_ADDRESS), AJ_NV_SENTINEL, AJ_NVRAM_BASE_ADDRESS[0]));
            _AJ_NVRAM_Clear();
        }
    }
}
static uint8_t g_flashFlag = TRUE;
void _AJ_NV_Write(void* dest, void* buf, uint16_t size)
{
    uint8_t *d = dest;
    uint8_t *b = buf;

    /*
     * Only accept writes within AJ_NVRAM
     */
    if (d < AJ_NVRAM_BASE_ADDRESS || d + size >= (uint8_t*)AJ_NVRAM_END)
        return;

    /*
     * Protect Read-Only area from writes
     */
    if (d < AJ_NVRAM_RW_ADDRESS) {
        size_t adj = AJ_NVRAM_RW_ADDRESS - d;

        if (adj >= size)
            return;

        dest = AJ_NVRAM_RW_ADDRESS;
        buf = b += adj;
        size -= adj;
    }

    memcpy(dest, buf, size);
    if (g_flashFlag == TRUE) {
        WriteToFlash();
    }
}

void _AJ_NV_Read(void* src, void* buf, uint16_t size)
{
   memcpy(buf, src, size);
}

void _AJ_NVRAM_Clear()
{
    /* Delete Runtime Modifications and reload emulation images */
    qcom_dset_delete(AJ_RW_DATA_ID, DSET_MEDIA_NVRAM, NULL, NULL);
    memset(AJ_EMULATED_NVRAM, INVALID_DATA_BYTE, sizeof(AJ_EMULATED_NVRAM));
    ReadFromFlash();
}

// Compact the storage by removing invalid entries
AJ_Status _AJ_CompactNVStorage()
{
    uint16_t* data = (uint16_t*)AJ_NVRAM_RW_ADDRESS;
    uint8_t* writePtr = (uint8_t*) data;
    uint16_t entrySize = 0;
    uint16_t garbage = 0;
    while ((uint8_t*) data < (uint8_t*) AJ_NVRAM_END_ADDRESS && *data != INVALID_DATA) {
        NV_EntryHeader* header = (NV_EntryHeader*) data;
        entrySize = ENTRY_HEADER_SIZE + header->capacity;
        if (header->id != INVALID_ID) {
            // do a single commit at the end
            memcpy(writePtr, data, entrySize);
            writePtr += entrySize;
        } else {
            garbage += entrySize;
        }
        data += entrySize >> 1;
    }

    memset(writePtr, INVALID_DATA_BYTE, garbage);
    WriteToFlash();
    return AJ_OK;
}
