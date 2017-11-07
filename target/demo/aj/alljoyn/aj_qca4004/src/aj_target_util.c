/**
 * @file
 */
/******************************************************************************
* Copyright 2013, Qualcomm Connected Experiences, Inc.
*
******************************************************************************/

/**
 * Per-module definition of the current module for debug logging.  Must be defined
 * prior to first inclusion of aj_debug.h
 */
#define AJ_MODULE TARGET_UTIL


#include "aj_target.h"
#include "aj_status.h"
#include "aj_util.h"
#include "aj_debug.h"

#include <qcom/qcom_time.h>
#include <qcom/qcom_timer.h>
#include <qcom/qcom_system.h>
#include <qcom/qcom_utils.h>

/**
 * Turn on per-module debug printing by setting this variable to non-zero value
 * (usually in debugger).
 */
#ifndef NDEBUG
uint8_t dbgTARGET_UTIL = 0;
#endif


AJ_Status AJ_SuspendWifi(uint32_t msec)
{
    return AJ_OK;
}

void AJ_Sleep(uint32_t timeout)
{
    qcom_thread_msleep(timeout);
}

uint32_t AJ_GetElapsedTime(AJ_Time* timer, uint8_t cumulative)
{
    unsigned long now = time_ms();
    unsigned long elapsed = now;
    if (timer) {
        elapsed -= (timer->seconds * 1000 + timer->milliseconds);
    }

    if (timer && !cumulative) {
        timer->seconds = now / 1000;
        timer->milliseconds = now % 1000;
    }
    return (uint32_t) elapsed;
}

void AJ_InitTimer(AJ_Time* timer)
{
    uint32_t now_msec = time_ms();
    timer->seconds = now_msec / 1000;
    timer->milliseconds = now_msec % 1000;
}

int32_t AJ_GetTimeDifference(AJ_Time* timerA, AJ_Time* timerB)
{
    int32_t diff;
    diff = (1000 * (timerA->seconds - timerB->seconds)) + (timerA->milliseconds - timerB->milliseconds);
    return diff;
}

void AJ_TimeAddOffset(AJ_Time* timerA, uint32_t msec)
{
    uint32_t msecNew;
    if (msec == -1) {
        timerA->seconds = -1;
        timerA->milliseconds = -1;
    } else {
        msecNew = (timerA->milliseconds + msec);
        timerA->seconds = timerA->seconds + (msecNew / 1000);
        timerA->milliseconds = msecNew % 1000;
    }
}

int8_t AJ_CompareTime(AJ_Time timerA, AJ_Time timerB)
{
    if (timerA.seconds == timerB.seconds) {
        if (timerA.milliseconds == timerB.milliseconds) {
            return 0;
        } else if (timerA.milliseconds > timerB.milliseconds) {
            return 1;
        } else {
            return -1;
        }
    } else if (timerA.seconds > timerB.seconds) {
        return 1;
    } else {
        return -1;
    }
}

uint64_t AJ_DecodeTime(char* der, char* fmt)
{
    /* TODO */
    return 0;
}

void AJ_MemZeroSecure(void* s, size_t n)
{
    volatile unsigned char* p = s;
    while (n--) *p++ = '\0';
    return;
}

void AJ_Reboot(void)
{
    AJ_Printf("AJ_Reboot\n");
    /* Reset to chip. It is only a warm reset by pointing PC to reset vector */
    qcom_sys_reset();
}


/**
 *  These swapping functions should be replaced with any optimized version that might exist on the hardware.
 */
uint16_t AJ_ByteSwap16(uint16_t x)
{
    return (((x) >> 8) | ((x) << 8));
}

uint32_t AJ_ByteSwap32(uint32_t x)
{
    return  (((x) >> 24) | (((x) & 0xFF0000) >> 8) | (((x) & 0x00FF00) << 8) | ((x) << 24));
}

uint64_t AJ_ByteSwap64(uint64_t x)
{
    return (((x)                     ) >> 56) |
           (((x) & 0x00FF000000000000LL) >> 40) |
           (((x) & 0x0000FF0000000000LL) >> 24) |
           (((x) & 0x000000FF00000000LL) >>  8) |
           (((x) & 0x00000000FF000000LL) <<  8) |
           (((x) & 0x0000000000FF0000LL) << 24) |
           (((x) & 0x000000000000FF00LL) << 40) |
           (((x)                     ) << 56);
}


char* AJ_strcat(char* dest, const char* source)
{
    char* tail = dest;
    while(*tail) {
        tail++;
    }

    strcpy(tail, source);
    return dest;
}

void __assert_func(const char* file, int line, const char* func, const char* expr)
{
    AJ_AlwaysPrintf(("\nassert %s:%d in %s, failed %s\n", file, line, func, expr));
    while(1) {
        /* infinite loop */
    }
}


int _AJ_DbgEnabled(const char* module)
{
    return FALSE;
}

/* This platform needs memchr */
void *memchr(void *s, int c, size_t n)
{
    unsigned char *l = s;

    while (n--) {
        if (*l == c) return l;
        l++;
    }

    return NULL;
}

/* This platform needs strstr */
char *strstr(const char *haystack, const char *needle)
{
    size_t hlen, nlen;

    if (needle == NULL || haystack == NULL)
        return NULL;

    hlen = strlen(haystack);
    nlen = strlen(needle);

    if (nlen > hlen)
        return NULL;

    hlen = hlen - nlen + 1;
    while (hlen--) {
        if (*haystack == *needle) {
            if (strncmp(haystack, needle, nlen) == 0)
                return haystack;
        }
        haystack++;
    }

    return NULL;
}
