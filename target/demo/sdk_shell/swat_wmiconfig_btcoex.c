/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

//#include <osapi.h>
//#include "threadx/tx_api.h"
//#include <wlan_dev.h>
#include "base.h"
//#include "athdefs.h"
#include "wmi_cdr.h"
#include "qcom_common.h"
#include "swat_wmiconfig_btcoex.h"

extern int atoi(const char *buf);

A_STATUS
ath_set_btcoex_fe_ant(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_FE_ANT_CMD BtcoexFeAntCmd;
    } btcoex_cmd;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_FE_ANT_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexFeAntCmd), 0, sizeof(btcoex_cmd.BtcoexFeAntCmd));

    if (3 > argc) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }
    btcoex_cmd.BtcoexFeAntCmd.btcoexFeAntType = atoi(argv[2]);
    if (btcoex_cmd.BtcoexFeAntCmd.btcoexFeAntType >= WMI_BTCOEX_FE_ANT_TYPE_MAX) {
        A_PRINTF("Invalid configuration [1 - Single Antenna]\n");
        A_PRINTF("                      [2 - 1x1 Dual Antenna Low Isolation, 3 - 1x1 Dual Antenna High Isolation]\n");
        A_PRINTF("                      [4 - 2x2 w/ Shared Antenna w/ Low Isolation, 5 - 2x2 w/ Shared Antenna w/ High Isolation\n"); 
        return A_ERROR;
    }
    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetColocatedBtdevConfigCmd(dev, &BtcoexFeAntCmd);
}

A_STATUS
ath_set_btcoex_colocated_bt_dev(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMD BtcoexCoLocatedBtCmd;
    } btcoex_cmd;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_COLOCATED_BT_DEV_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexCoLocatedBtCmd), 0, sizeof(btcoex_cmd.BtcoexCoLocatedBtCmd));

    if (3 > argc) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }

    btcoex_cmd.BtcoexCoLocatedBtCmd.btcoexCoLocatedBTdev = atoi(argv[2]);
    if (btcoex_cmd.BtcoexCoLocatedBtCmd.btcoexCoLocatedBTdev > 5) {
        A_PRINTF("Invalid configuration %d\n",
               btcoex_cmd.BtcoexCoLocatedBtCmd.btcoexCoLocatedBTdev);
        return A_ERROR;
    }
    A_PRINTF("btcoex colocated antType = %d\n",
           btcoex_cmd.BtcoexCoLocatedBtCmd.btcoexCoLocatedBTdev);
   
    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetColocatedBtdevConfigCmd(dev, &BtcoexCoLocatedBtCmd);
}

A_STATUS
ath_set_btcoex_sco_config(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_SCO_CONFIG_CMD BtcoexScoConfigCmd;
    } btcoex_cmd;
    A_UINT32 index;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_SCO_CONFIG_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexScoConfigCmd), 0, sizeof(btcoex_cmd.BtcoexScoConfigCmd));

    index = 2;
    if ((index + 17) > argc) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }
    btcoex_cmd.BtcoexScoConfigCmd.scoConfig.scoSlots =  atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoConfig.scoIdleSlots = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoConfig.scoFlags = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoConfig.linkId = atoi(argv[index++]);

    btcoex_cmd.BtcoexScoConfigCmd.scoPspollConfig.scoCyclesForceTrigger = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoPspollConfig.scoDataResponseTimeout = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoPspollConfig.scoStompDutyCyleVal = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoPspollConfig.scoStompDutyCyleMaxVal = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoPspollConfig.scoPsPollLatencyFraction = atoi(argv[index++]);

    btcoex_cmd.BtcoexScoConfigCmd.scoOptModeConfig.scoStompCntIn100ms = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoOptModeConfig.scoContStompMax = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoOptModeConfig.scoMinlowRateMbps = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoOptModeConfig.scoLowRateCnt = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoOptModeConfig.scoHighPktRatio = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoOptModeConfig.scoMaxAggrSize = atoi(argv[index++]);

    btcoex_cmd.BtcoexScoConfigCmd.scoWlanScanConfig.scanInterval = atoi(argv[index++]);
    btcoex_cmd.BtcoexScoConfigCmd.scoWlanScanConfig.maxScanStompCnt = atoi(argv[index++]);

    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetScoConfigCmddev, &BtcoexScoConfigCmd);
}

A_STATUS
ath_set_btcoex_a2dp_config(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_A2DP_CONFIG_CMD BtcoexA2dpConfigCmd;
    } btcoex_cmd;
    A_UINT32 index;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_A2DP_CONFIG_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexA2dpConfigCmd), 0, sizeof(btcoex_cmd.BtcoexA2dpConfigCmd));

    index = 2;
    if ((index + 10) > argc ) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dpConfig.a2dpFlags =  atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dpConfig.linkId = atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dppspollConfig.a2dpWlanMaxDur = atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dppspollConfig.a2dpMinBurstCnt = atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dppspollConfig.a2dpDataRespTimeout = atoi(argv[index++]);

    btcoex_cmd.BtcoexA2dpConfigCmd.a2dpOptConfig.a2dpMinlowRateMbps = atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dpOptConfig.a2dpLowRateCnt = atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dpOptConfig.a2dpHighPktRatio = atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dpOptConfig.a2dpMaxAggrSize = atoi(argv[index++]);
    btcoex_cmd.BtcoexA2dpConfigCmd.a2dpOptConfig.a2dpPktStompCnt = atoi(argv[index++]);

    A_PRINTF("a2dp Config, flags=%x\n", btcoex_cmd.BtcoexA2dpConfigCmd.a2dpConfig.a2dpFlags);
   
    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetA2dpConfigCmd(dev, &BtcoexA2dpConfigCmd);
}

A_STATUS
ath_set_btcoex_hid_config(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_HID_CONFIG_CMD BtcoexHidConfigCmd;
    } btcoex_cmd;
    A_UINT32 index;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_HID_CONFIG_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexHidConfigCmd), 0, sizeof(btcoex_cmd.BtcoexHidConfigCmd));

    index = 2;
    if ((index + 3) > argc ) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }

    btcoex_cmd.BtcoexHidConfigCmd.hidConfig.hidFlags =  atoi(argv[index++]);
    btcoex_cmd.BtcoexHidConfigCmd.hidConfig.hiddevices = atoi(argv[index++]);
    btcoex_cmd.BtcoexHidConfigCmd.hidConfig.aclPktCntLowerLimit = atoi(argv[index++]);
    A_PRINTF("hid Config, flags=%x  %d\n", btcoex_cmd.BtcoexHidConfigCmd.hidConfig.hidFlags,
           btcoex_cmd.BtcoexHidConfigCmd.hidConfig.hiddevices);

    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetHidConfigCmd(dev, &BtcoexHidConfigCmd);
}

A_STATUS
ath_set_btcoex_aclcoex_config(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_ACLCOEX_CONFIG_CMD BtcoexAclCoexConfigCmd;
    } btcoex_cmd;
    A_UINT32 index;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_ACLCOEX_CONFIG_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexAclCoexConfigCmd), 0, sizeof(btcoex_cmd.BtcoexAclCoexConfigCmd));

    index = 2;
    if ((index + 14) > argc ) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.aclWlanMediumDur     =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.aclBtMediumDur       =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.aclDetectTimeout     =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.aclPktCntLowerLimit  =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.aclIterForEnDis      =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.aclPktCntUpperLimit  =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.aclCoexFlags         =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexConfig.linkId               =  atoi(argv[index++]);

    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexPspollConfig.aclDataRespTimeout =  atoi(argv[index++]);

    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexOptConfig.aclCoexMinlowRateMbps  =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexOptConfig.aclCoexLowRateCnt  =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexOptConfig.aclCoexHighPktRatio  =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexOptConfig.aclCoexMaxAggrSize  =  atoi(argv[index++]);
    btcoex_cmd.BtcoexAclCoexConfigCmd.aclCoexOptConfig.aclPktStompCnt  =  atoi(argv[index++]);

    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetAclCoexConfigCmd(dev, &BtcoexAclCoexConfigCmd);
}

A_STATUS
ath_set_btcoex_btinquiry_page_config(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_BTINQUIRY_PAGE_CONFIG_CMD BtcoexbtinquiryPageConfigCmd;
    } btcoex_cmd;
    A_UINT32 index;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_BTINQUIRY_PAGE_CONFIG_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexbtinquiryPageConfigCmd), 0, sizeof(btcoex_cmd.BtcoexbtinquiryPageConfigCmd));

    index = 2;
    if ((index + 3) > argc) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }
    btcoex_cmd.BtcoexbtinquiryPageConfigCmd.btInquiryDataFetchFrequency = atoi(argv[index++]);
    btcoex_cmd.BtcoexbtinquiryPageConfigCmd.protectBmissDurPostBtInquiry = atoi(argv[index++]);
    btcoex_cmd.BtcoexbtinquiryPageConfigCmd.btInquiryPageFlag = atoi(argv[index++]);

    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetBtInquiryPageConfigCmd(dev, &BtcoexbtinquiryPageConfigCmd);
}

A_STATUS
ath_set_btcoex_debug(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_DEBUG_CMD BtcoexDebugCmd;
    } btcoex_cmd;
    A_UINT32 index;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_DEBUG_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexDebugCmd), 0, sizeof(btcoex_cmd.BtcoexDebugCmd));

    index = 2;
    if ((index + 5) > argc) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }
    btcoex_cmd.BtcoexDebugCmd.btcoexDbgParam1=atoi(argv[index++]);
    btcoex_cmd.BtcoexDebugCmd.btcoexDbgParam2=atoi(argv[index++]);
    btcoex_cmd.BtcoexDebugCmd.btcoexDbgParam3=atoi(argv[index++]);
    btcoex_cmd.BtcoexDebugCmd.btcoexDbgParam4=atoi(argv[index++]);
    btcoex_cmd.BtcoexDebugCmd.btcoexDbgParam5=atoi(argv[index++]);

    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetDebugCmd(dev, &BtcoexDebugCmd);
}

A_STATUS
ath_set_btcoex_bt_operating_status(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMD BtcoexBtOperatingStatusCmd;
    } btcoex_cmd;
    A_UINT32 index;

    btcoex_cmd.hdr.commandId = WMI_SET_BTCOEX_BT_OPERATING_STATUS_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexBtOperatingStatusCmd), 0, sizeof(btcoex_cmd.BtcoexBtOperatingStatusCmd));

    index = 2;
    if ((index + 3) > argc) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }

    btcoex_cmd.BtcoexBtOperatingStatusCmd.btProfileType =atoi(argv[index++]);
    btcoex_cmd.BtcoexBtOperatingStatusCmd.btOperatingStatus =atoi(argv[index++]);
    btcoex_cmd.BtcoexBtOperatingStatusCmd.btLinkId =atoi(argv[index++]);

    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));
    return 1;
//    return BTCOEX_SetBtOperatingStatusCmd(dev, &BtcoexBtOperatingStatusCmd);
}

static void
printBtcoexStats(int devId, WMI_BTCOEX_STATS_EVENT *pStats, A_UINT32 btProfileType, int length)
{
    A_PRINTF("---\n");
    A_PRINTF("GENERAL BTCOEX STATS\n");
    A_PRINTF("highRatePktCnt = \t\t%d\n"
             "firstBmissCnt = \t\t%d\n"
             "psPollFailureCnt = \t\t%d\n"
             "nullFrameFailureCnt = \t\t%d\n"
             "optModeTransitionCnt = \t\t%d\n",
             pStats->coexStats.highRatePktCnt,
             pStats->coexStats.firstBmissCnt,
             pStats->coexStats.psPollFailureCnt,
             pStats->coexStats.nullFrameFailureCnt,
             pStats->coexStats.optModeTransitionCnt);
    A_PRINTF("---\n");

    switch(btProfileType) {
    case WMI_BTCOEX_BT_PROFILE_SCO:
        A_PRINTF("BTCOEX SCO STATS\n");
        A_PRINTF("scoStompCntAvg = \t\t%d\n"
                 "scoStompIn100ms = \t\t%d\n"
                 "scoMaxContStomp = \t\t%d\n"
                 "scoAvgNoRetries = \t\t%d\n"
                 "scoMaxNoRetriesIn100ms = \t%d\n",
                 pStats->scoStats.scoStompCntAvg,
                 pStats->scoStats.scoStompIn100ms,
                 pStats->scoStats.scoMaxContStomp,
                 pStats->scoStats.scoAvgNoRetries,
                 pStats->scoStats.scoMaxNoRetriesIn100ms);
        break;
    case WMI_BTCOEX_BT_PROFILE_A2DP:
        A_PRINTF("BTCOEX A2DP STATS\n");
        A_PRINTF("a2dpBurstCnt = \t\t\t%d\n"
                 "a2dpMaxBurstCnt = \t\t%d\n"
                 "a2dpAvgIdletimeIn100ms = \t%d\n"
                 "a2dpAvgStompCnt = \t\t%d\n",
                 pStats->a2dpStats.a2dpBurstCnt,
                 pStats->a2dpStats.a2dpMaxBurstCnt,
                 pStats->a2dpStats.a2dpAvgIdletimeIn100ms,
                 pStats->a2dpStats.a2dpAvgStompCnt);
        break;
    case WMI_BTCOEX_BT_PROFILE_ACLCOEX:
        A_PRINTF("BTCOEX ACLCOEX STATS\n");
        A_PRINTF("aclPktCntInBtTime = \t\t%d\n"
                 "aclStompCntInWlanTime = \t%d\n"
                 "aclPktCntIn100ms = \t\t%d\n",
                 pStats->aclCoexStats.aclPktCntInBtTime,
                 pStats->aclCoexStats.aclStompCntInWlanTime,
                 pStats->aclCoexStats.aclPktCntIn100ms);
        break;
    default:
        break;
    }
}

static void
printBtcoexConfig(int devId, WMI_BTCOEX_CONFIG_EVENT *pConfig, int len)
{
    switch(pConfig->btProfileType) {
    case WMI_BTCOEX_BT_PROFILE_SCO:
        {
            WMI_SET_BTCOEX_SCO_CONFIG_CMD *Cmd = &pConfig->info.scoConfigCmd;
            A_PRINTF("GENERIC SCO CONFIG\n");
            A_PRINTF("scoSlots = \t\t\t%d\n"
                    "scoIdleSlots = \t\t\t%d\n"
                    "scoFlags = \t\t\t0x%x\n"
                    "linkId = \t\t\t%d\n",
                    Cmd->scoConfig.scoSlots,
                    Cmd->scoConfig.scoIdleSlots,
                    Cmd->scoConfig.scoFlags,
                    Cmd->scoConfig.linkId
                   );
            A_PRINTF("PSPOLL SCO CONFIG \n");
            A_PRINTF("scoCyclesForceTrigger = \t%d\n"
                    "scoDataResponseTimeout = \t%d\n"
                    "scoStompDutyCyleVal = \t\t%d\n"
                    "scoStompDutyCyleMaxVal = \t%d\n"
                    "scoPsPollLatencyFraction = \t%d\n",
                    Cmd->scoPspollConfig.scoCyclesForceTrigger,
                    Cmd->scoPspollConfig.scoDataResponseTimeout,
                    Cmd->scoPspollConfig.scoStompDutyCyleVal,
                    Cmd->scoPspollConfig.scoStompDutyCyleMaxVal,
                    Cmd->scoPspollConfig.scoPsPollLatencyFraction
                    );
            A_PRINTF("SCO OPTMODE CONFIG\n");
            A_PRINTF("scoStompCntIn100ms = \t\t%d\n"
                    "scoContStompMax = \t\t%d\n"
                    "scoMinlowRateMbps = \t\t%d\n"
                    "scoLowRateCnt = \t\t%d\n"
                    "scoHighPktRatio = \t\t%d\n"
                    "scoMaxAggrSize = \t\t%d\n",
                    Cmd->scoOptModeConfig.scoStompCntIn100ms,
                    Cmd->scoOptModeConfig.scoContStompMax,
                    Cmd->scoOptModeConfig.scoMinlowRateMbps,
                    Cmd->scoOptModeConfig.scoLowRateCnt,
                    Cmd->scoOptModeConfig.scoHighPktRatio,
                    Cmd->scoOptModeConfig.scoMaxAggrSize
                 );
            A_PRINTF("SCO WLAN SCAN CONFIG\n");
            A_PRINTF("scanInterval = \t\t\t%d\n"
                   "maxScanStompCnt = \t\t%d\n",
                   Cmd->scoWlanScanConfig.scanInterval,
                   Cmd->scoWlanScanConfig.maxScanStompCnt
                  );
         }
         break;
    case WMI_BTCOEX_BT_PROFILE_A2DP:
        {
            WMI_SET_BTCOEX_A2DP_CONFIG_CMD *Cmd = &pConfig->info.a2dpConfigCmd;
            A_PRINTF("GENERIC A2DP CONFIG\n");
            A_PRINTF("a2dpFlags = \t\t\t0x%x\n"
                     "linkId = \t\t\t%d\n",
                     Cmd->a2dpConfig.a2dpFlags,
                     Cmd->a2dpConfig.linkId);
            A_PRINTF("PSPOLL A2DP CONFIG\n");
            A_PRINTF("a2dpWlanMaxDur = \t\t%d\n"
                     "a2dpMinBurstCnt = \t\t%d\n"
                     "a2dpDataRespTimeout = \t\t%d\n",
                     Cmd->a2dppspollConfig.a2dpWlanMaxDur,
                     Cmd->a2dppspollConfig.a2dpMinBurstCnt,
                     Cmd->a2dppspollConfig.a2dpDataRespTimeout);
            A_PRINTF("OPTMODE A2DP CONFIG\n");
            A_PRINTF("a2dpMinlowRateMbps = \t\t%d\n"
                     "a2dpLowRateCnt = \t\t%d\n"
                     "a2dpHighPktRatio = \t\t%d\n"
                     "a2dpMaxAggrSize = \t\t%d\n"
                     "a2dpPktStompCnt = \t\t%d\n",
                     Cmd->a2dpOptConfig.a2dpMinlowRateMbps,
                     Cmd->a2dpOptConfig.a2dpLowRateCnt,
                     Cmd->a2dpOptConfig.a2dpHighPktRatio,
                     Cmd->a2dpOptConfig.a2dpMaxAggrSize,
                     Cmd->a2dpOptConfig.a2dpPktStompCnt);
        }
        break;
    case WMI_BTCOEX_BT_PROFILE_INQUIRY_PAGE:
        {
            WMI_SET_BTCOEX_BTINQUIRY_PAGE_CONFIG_CMD *Cmd = &pConfig->info.btinquiryPageConfigCmd;
            A_PRINTF("BTCOEX INQUIRY PAGE CONFIG\n");
            A_PRINTF("btInquiryDataFetchFrequency = \t%d\n"
                     "protectBmissDurPostBtInquiry = \t%d\n"
                     "maxpageStomp = \t\t\t%d\n"
                     "btInquiryPageFlag = \t\t%d\n",
                     Cmd->btInquiryDataFetchFrequency,
                     Cmd->protectBmissDurPostBtInquiry,
                     Cmd->maxpageStomp,
                     Cmd->btInquiryPageFlag);
        }
        break;
    case WMI_BTCOEX_BT_PROFILE_ACLCOEX:
        {
            WMI_SET_BTCOEX_ACLCOEX_CONFIG_CMD *Cmd = &pConfig->info.aclcoexConfig;
            A_PRINTF("GENERIC ACLCOEX CONFIG\n");
            A_PRINTF("aclWlanMediumDur = \t\t%d\n"
                     "aclBtMediumDur = \t\t%d\n"
                     "aclDetectTimeout = \t\t%d\n"
                     "aclPktCntLowerLimit = \t\t%d\n"
                     "aclIterForEnDis = \t\t%d\n"
                     "aclPktCntUpperLimit = \t\t%d\n"
                     "aclCoexFlags = \t\t\t0x%x\n"
                     "linkId = \t\t\t%d\n",
                     Cmd->aclCoexConfig.aclWlanMediumDur,
                     Cmd->aclCoexConfig.aclBtMediumDur,
                     Cmd->aclCoexConfig.aclDetectTimeout,
                     Cmd->aclCoexConfig.aclPktCntLowerLimit,
                     Cmd->aclCoexConfig.aclIterForEnDis,
                     Cmd->aclCoexConfig.aclPktCntUpperLimit,
                     Cmd->aclCoexConfig.aclCoexFlags,
                     Cmd->aclCoexConfig.linkId);
            A_PRINTF("PSPOLL ACLCOEX CONFIG\n");
            A_PRINTF("aclDataRespTimeout = \t\t%d\n",
                     Cmd->aclCoexPspollConfig.aclDataRespTimeout);
            A_PRINTF("OPTMODE ACLCOEX CONFIG\n");
            A_PRINTF("aclCoexMinlowRateMbps = \t%d\n"
                     "aclCoexLowRateCnt = \t\t%d\n"
                     "aclCoexHighPktRatio = \t\t%d\n"
                     "aclCoexMaxAggrSize = \t\t%d\n"
                     "aclPktStompCnt = \t\t%d\n",
                     Cmd->aclCoexOptConfig.aclCoexMinlowRateMbps,
                     Cmd->aclCoexOptConfig.aclCoexLowRateCnt,
                     Cmd->aclCoexOptConfig.aclCoexHighPktRatio,
                     Cmd->aclCoexOptConfig.aclCoexMaxAggrSize,
                     Cmd->aclCoexOptConfig.aclPktStompCnt);
        }
        break;
    case WMI_BTCOEX_BT_PROFILE_HID:
        {
            WMI_SET_BTCOEX_HID_CONFIG_CMD *Cmd = &pConfig->info.hidConfigCmd;
            A_PRINTF("GENERIC HID CONFIG\n");
            A_PRINTF("hidFlags = \t\t\t0x%x\n"
                     "hiddevices = \t\t\t%d\n"
                     "maxStompSlot = \t\t\t%d\n"
                     "aclPktCntLowerLimit = \t\t%d\n",
                     Cmd->hidConfig.hidFlags,
                     Cmd->hidConfig.hiddevices,
                     Cmd->hidConfig.maxStompSlot,
                     Cmd->hidConfig.aclPktCntLowerLimit);
            A_PRINTF("PSPOLL HID CONFIG\n");
            A_PRINTF("hidWlanMaxDur = \t\t%d\n"
                     "hidMinBurstCnt = \t\t%d\n"
                     "hidDataRespTimeout = \t\t%d\n",
                     Cmd->hidpspollConfig.hidWlanMaxDur,
                     Cmd->hidpspollConfig.hidMinBurstCnt,
                     Cmd->hidpspollConfig.hidDataRespTimeout);
            A_PRINTF("OPTMODE HID CONFIG\n");
            A_PRINTF("hidMinlowRateMbps = \t\t%d\n"
                     "hidLowRateCnt = \t\t%d\n"
                     "hidHighPktRatio = \t\t%d\n"
                     "hidMaxAggrSize = \t\t%d\n"
                     "hidPktStompCnt = \t\t%d\n",
                     Cmd->hidOptConfig.hidMinlowRateMbps,
                     Cmd->hidOptConfig.hidLowRateCnt,
                     Cmd->hidOptConfig.hidHighPktRatio,
                     Cmd->hidOptConfig.hidMaxAggrSize,
                     Cmd->hidOptConfig.hidPktStompCnt);
        }
        break;
    default:
        {
            A_PRINTF("Unknown BTCOEX CONFIG profile type: %d\n", pConfig->btProfileType);
        }
        return;
    }

    /* print piggy-backed stats */
    printBtcoexStats(devId, (WMI_BTCOEX_STATS_EVENT *)&pConfig[1], pConfig->btProfileType,
                     len - sizeof(WMI_BTCOEX_CONFIG_EVENT));
}

A_STATUS
ath_get_btcoex_config(int devId, int argc, char* argv[])
{
    struct {
        WMI_CDR_HDR hdr;
        WMI_GET_BTCOEX_CONFIG_CMD BtcoexGetConfigCmd;
    } btcoex_cmd;
    A_UINT32 index;
    A_UINT32 profile;

    btcoex_cmd.hdr.commandId = WMI_GET_BTCOEX_CONFIG_CMDID;
    btcoex_cmd.hdr.info1 = devId;
    A_MEMSET(&(btcoex_cmd.BtcoexGetConfigCmd), 0, sizeof (btcoex_cmd.BtcoexGetConfigCmd));

    index = 2;
    if ((index + 2) > argc) {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        return A_ERROR;
    }

    profile = atoi(argv[index++]);
    if (profile < WMI_BTCOEX_BT_PROFILE_SCO || profile > WMI_BTCOEX_BT_PROFILE_HID) {
        A_PRINTF("Profile type out of range: %d\n", profile);
        return A_ERROR;
    }
    btcoex_cmd.BtcoexGetConfigCmd.btProfileType = profile;
    btcoex_cmd.BtcoexGetConfigCmd.linkId = atoi(argv[index++]);

    /* set the callback for the results */
    qcom_set_btcoexconfig_callback(devId, (void *)printBtcoexConfig);

    WMI_CDR_initiate(&btcoex_cmd, sizeof(btcoex_cmd));

    return 1;
}

A_STATUS
ath_set_btcoex_scheme(int devId, int argc, char* argv[])
{
    A_UINT8 index = 2;     //console commands begin index
    A_UINT8 schemeID = 0;  //bt coex scheme ID
    void* pData = NULL;    //cmd payload
    if ((index + 2) > argc)
    {
        A_PRINTF("Incorrect number of args: %d\n", argc);
        goto ERROR;
    }

    schemeID = atoi(argv[index ++]);
    if(schemeID == 1)
    {
        /**
         * if this is ALL BT MODE, the weight timer period 
         * and duty cycle is configurable
         */
        A_UINT32 timer_period = atoi(argv[index ++]);
        A_UINT8  duty_cycle   = atoi(argv[index]);
        if(timer_period <= 20 || duty_cycle > 100) goto ERROR;
        pData = qcom_mem_alloc(sizeof(BTCOEX_SCHEME_ALLBT_CONFIG));
        if(pData == NULL) goto ERROR;
        BTCOEX_SCHEME_ALLBT_CONFIG *config = (BTCOEX_SCHEME_ALLBT_CONFIG *)pData;
        config->allBtTimer                 = timer_period;
        config->stompNoneDutyCycle         = duty_cycle;
    }
    else
    {
        A_PRINTF("Unsupport BTCOEX Scheme ID %u\n", schemeID);
        goto ERROR;
    }

    qcom_btcoex_scheme_set(schemeID, pData);
    if(pData)
    {
        qcom_mem_free(pData);
    }

    return 1;

ERROR:
    A_PRINTF("Incorrect/missing param\r\n"
             "wmiconfig --setbtcoexscheme <scheme ID> <param1> <param2> ...\r\n"
             "Ex: wmiconfig --setbtcoexscheme 1 45 55 \r\n"
             "scheme ID: ALLBTMODE(1), other is not used currently\r\n"
             "When ALLBTMODE, param1 is AllBtTimer period(>20ms), recommend value is 45ms\r\n"
             "                param2 is STOMP NONE duty cycle(0~100), recommend value is 55\r\n");

    return A_ERROR;
}
