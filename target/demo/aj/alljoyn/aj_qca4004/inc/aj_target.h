#ifndef _AJ_TARGET_H
#define _AJ_TARGET_H
/**
 * @file
 */
/******************************************************************************
* Copyright 2012,2015 Qualcomm Connected Experiences, Inc.
*
******************************************************************************/

#include <stdlib.h>
#include <qcom/base.h>
#include <qcom/stdint.h>
#include <qcom/qcom_utils.h>

extern void* memmove(void* dest, const void* src, unsigned int n);
extern size_t strlen(const char* s);
extern int strcmp(const char* s1, const char* s2);
extern int strncmp(const char* s1, const char* s2, size_t n);
extern char* strcpy(char* dest, const char* src);
extern char* strncpy(char* dest, const char* src, size_t n);

#ifndef strcat
char* AJ_strcat(char* dest, const char* source);
#define strcat AJ_strcat
#endif

typedef unsigned long long uint64_t;
typedef long long int64_t;

#ifndef NULL
#define NULL ((void*) 0)
#endif

#ifndef TRUE
#define TRUE (1)
#endif

#ifndef FALSE
#define FALSE (0)
#endif

#ifndef max
#define max(x, y) ((x) > (y) ? (x) : (y))
#endif

#ifndef min
#define min(x, y) ((x) < (y) ? (x) : (y))
#endif

#ifndef USE_STD_ABS
#ifndef abs
#define abs(x) (((x) < 0) ? -(x) : (x))
#endif
#endif

#define WORD_ALIGN(x) ((x & 0x3) ? ((x >> 2) + 1) << 2 : x)
#define HOST_IS_LITTLE_ENDIAN  TRUE
#define HOST_IS_BIG_ENDIAN     FALSE

#define AJ_Printf(fmat, ...) \
    do { printf(fmat, ## __VA_ARGS__); } while (0)

#ifndef NDEBUG
    #define AJ_ASSERT(x) \
    do { \
        if (!(x)) { \
            AJ_Printf("ASSFAIL at %s:%u\n", __FILE__, __LINE__); \
            while(1); \
        } \
    } while (0);
#else
    #define AJ_ASSERT(x)
#endif

/**
 * Reboot the MCU
 */
void AJ_Reboot(void);

/**
 * For testing purposes, if USE_MAC_FOR_GUID is defined then the AJ_GUID will be
 * created from the device's MAC address. This allows the AJ_GUID for a device
 * to remain fixed even if it is erased from NVRAM.
 * This depends on the function AJ_GetLocalGUID using the function defined by
 * AJ_CreateNewGUID to initialize the AJ_GUID for the first time.
 */
#ifdef USE_MAC_FOR_GUID
#define AJ_CreateNewGUID AJ_CreateNewGUIDFromMAC
extern void AJ_CreateNewGUIDFromMAC(uint8_t* rand, uint32_t len);
#else
#define AJ_CreateNewGUID AJ_RandBytes
#endif

#define AJ_EXPORT

#define AJ_GetDebugTime(x) AJ_ERR_RESOURCES

#endif
