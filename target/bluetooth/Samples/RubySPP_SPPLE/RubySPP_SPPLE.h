/*****< rubyspp.h >************************************************************/
/*      Copyright 2001 - 2014 Stonestreet One.                                */
/*      All Rights Reserved.                                                  */
/*                                                                            */
/*  RUBYSPP - Simple Ruby application using SPP and SPPLE Profile.            */
/*                                                                            */
/*  Author:  TIm Thomas                                                       */
/*                                                                            */
/*** MODIFICATION HISTORY *****************************************************/
/*                                                                            */
/*   mm/dd/yy  F. Lastname    Description of Modification                     */
/*   --------  -----------    ------------------------------------------------*/
/*   09/08/15  T. Thomas      Initial creation.                               */
/******************************************************************************/
#ifndef __RUBYSPPH__
#define __RUBYSPPH__

#include "threadx/tx_api.h"
#include "SPPLETyp.h"   /* GATT based SPP-like Types File.                    */
#include "LETPType.h"   /* GATT based Throughput Types File.                  */

#define TimeStamp_t                 unsigned long
#define CAPTURE_TIMESTAMP(_x)       (*(_x) = time_ms())
#define DIFF_TIMESTAMP(_x, _y)      ((_y) - (_x))

#endif

