/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */
#if defined(P2P_ENABLED)
#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "qcom_internal.h"
#include "swat_parse.h"
#include "qcom_p2p_api.h"
#include "qcom_wps.h"
#include "qcom_mem.h"
#include "p2p.h"

extern int qcom_isdigit(char *buf);
extern int g_wifi_state[];

#define P2P_SSID_AUTO_GO "DIRECT-IO"
#define P2P_PASS_AUTO_GO "1234567890"
#define P2P_DEFAULT_CHAN 1

#define A_OFFSETOF(type,field) (int)(&(((type *)NULL)->field))
#define A_GET_UINT32_FIELD(p,type,field) \
    (((A_UINT32)(((A_UINT8 *)(p))[(A_OFFSETOF(type,field) + 3)])) << 24 | \
     ((A_UINT32)(((A_UINT8 *)(p))[(A_OFFSETOF(type,field) + 2)])) << 16 | \
     ((A_UINT32)(((A_UINT8 *)(p))[(A_OFFSETOF(type,field) + 1)])) << 8   | \
     ((A_UINT32)(((A_UINT8 *)(p))[(A_OFFSETOF(type,field))])))

#define ASSEMBLE_UNALIGNED_UINT16(p,highbyte,lowbyte) \
        (((A_UINT16)(((A_UINT8 *)(p))[(highbyte)])) << 8 | (A_UINT16)(((A_UINT8 *)(p))[(lowbyte)]))
#define A_GET_UINT16_FIELD(p,type,field) \
    ASSEMBLE_UNALIGNED_UINT16(p,\
                              A_OFFSETOF(type,field) + 1, \
                              A_OFFSETOF(type,field))

A_STATUS app_p2p_start_persistent_dev(int device_id, A_CHAR *pssid);

/*local var*/
typedef struct {
    A_CHAR wps_pin[WPS_PIN_LEN+1] ;
    A_UINT8 peer_addr[ATH_MAC_LEN] ;
    A_UINT8 wps_role ;
    A_UINT8 dialog_token;
    A_UINT16 dev_password_id;
	A_UINT8 wps_method;
    A_BOOL persistent;
} CMD_P2P_INFO;
CMD_P2P_INFO p2p_info;

A_UINT16 g_p2p_op_ch;

void cmd_p2p_set_persistent(A_BOOL pers)
{
    p2p_info.persistent = pers;
}

A_BOOL cmd_p2p_get_persistent(void)
{
    return p2p_info.persistent;
}

void cmd_p2p_set_wps_pin(char *ppin)
{
    if (NULL == ppin) {
        return;
    }

    if (A_STRLEN(ppin) != WPS_PIN_LEN) {
        return;
    }

    A_STRCPY(p2p_info.wps_pin,ppin);
}

void cmd_p2p_get_wps_pin(char *ppin)
{
    if (NULL == ppin) {
        return;
    }

    A_STRNCPY(ppin,p2p_info.wps_pin,WPS_PIN_LEN);
    ppin[WPS_PIN_LEN] = '\0';

    return;
}

A_STATUS cmd_p2p_set_role(A_UINT8 role)
{
	p2p_info.wps_role= role;

    return A_OK;
}

A_STATUS cmd_p2p_get_role(A_UINT8 *role)
{

    if (NULL == role) {
        return A_ERROR;
    }

	*role=p2p_info.wps_role;

    return A_OK;
}

A_STATUS cmd_p2p_set_wps_method(A_UINT8 wps_method)
{
	p2p_info.wps_method= wps_method;

    return A_OK;
}

A_STATUS cmd_p2p_get_wps_method(A_UINT8 *wps_method)
{
    if (NULL == wps_method) {
        return A_ERROR;
    }

	*wps_method=p2p_info.wps_method;

    return A_OK;
}

A_STATUS cmd_p2p_set_peer_mac(A_UINT8 *pmac)
{

    if (NULL == pmac) {
        return A_ERROR;
    }

    A_MEMCPY(p2p_info.peer_addr, pmac, IEEE80211_ADDR_LEN);

    return A_OK;
}

A_STATUS cmd_p2p_get_peer_mac(A_UINT8 *pmac)
{

    if (NULL == pmac) {
        return A_ERROR;
    }

    A_MEMCPY(pmac, p2p_info.peer_addr, IEEE80211_ADDR_LEN);

    return A_OK;
}

A_STATUS cmd_p2p_set_peer_token(A_UINT8 dialog_token)
{
    p2p_info.dialog_token = dialog_token;
    return A_OK;
}

A_STATUS cmd_p2p_get_peer_token(A_UINT8 *pdialog_token)
{

    if (NULL == pdialog_token) {
        return A_ERROR;
    }

    *pdialog_token = p2p_info.dialog_token;

    return A_OK;
}

A_UINT32 cmd_p2p_get_persistent_index(A_CHAR *pssid, P2P_PERSISTENT_MAC_LIST *p2p_pers_list)
{
    A_UINT32 i;
    A_UINT32 uilen;

    if (NULL == pssid) {
        return MAX_LIST_COUNT;
    }

    uilen = A_STRLEN(pssid);
    if (uilen >= MAX_SSID_LEN) {
        return MAX_LIST_COUNT;
    }

    for (i = 0; i < MAX_LIST_COUNT; i++) {
        if (A_STRNCMP((A_CHAR *)p2p_pers_list[i].ssid, pssid, uilen) == 0) {
            break;
        }
    }

    return i;
}


int cmd_wlan_p2p_find(int argc, char *argv[])
{
    int timeout;
    P2P_DISC_TYPE type;

    if(3 == argc) {
        if (!strcmp(argv[2], "full")) {
            type = P2P_DISC_START_WITH_FULL;
            /* default 2mins */
            timeout = 300;
        }
        else if (!strcmp(argv[2], "social")) {
            type = P2P_DISC_ONLY_SOCIAL;
            /* default 2mins */
            timeout = 300;
        }
        else if (!strcmp(argv[2], "prog")) {
            type = P2P_DISC_PROGRESSIVE;
            /* default 2mins */
            timeout = 300;
        }
        else {
            if (1 == qcom_isdigit(argv[2])) {
                /* default social type */
                type = P2P_DISC_ONLY_SOCIAL;
                timeout = atoi(argv[2]);
            }
            else {
                printf("error type\n");
                return -1;
            }
        }
    }
    else if(4 == argc) {
        if (!strcmp(argv[2], "full")) {
            type = P2P_DISC_START_WITH_FULL;
        }
        else if (!strcmp(argv[2], "social")) {
            type = P2P_DISC_ONLY_SOCIAL;
        }
        else if (!strcmp(argv[2], "prog")) {
            type = P2P_DISC_PROGRESSIVE;
        }
        else {
            printf("error type\n");
            return -1;
        }

        timeout = atoi(argv[3]);
    }
    else {
        type = P2P_DISC_ONLY_SOCIAL;
        timeout = 300;
    }
    
    qcom_p2p_func_find(currentDeviceId, type, timeout);

    return 0;
}

int cmd_wlan_p2p_pass(int argc, char *argv[])
{
    if(argc < 4){
        printf("error cmd\n");
        return -1;
    }

    qcom_p2p_func_set_pass_ssid(currentDeviceId, argv[2], argv[3]);
    
    return 0;
}

int cmd_wlan_p2p_auth(int argc, char *argv[])
{
    int rt;
    int dev_auth = 0;
    enum p2p_wps_method wps_method;
    int aidata[6];
    unsigned char ucpersistent = 0;
    unsigned char peer_mac[6];

    if(argc < 4){
        printf("error cmd\n");
        return -1;
    }

    else if(!strcmp(argv[3], "push")) {
        wps_method = WPS_PBC;
    }
    else if(!strcmp(argv[3], "display")) {
        char wps_pin[12];
        wps_method = WPS_PIN_DISPLAY;

        if((argc > 4) && (strlen(argv[4]) == 8)) {
            extern void cmd_p2p_set_wps_pin(char *ppin);
            cmd_p2p_set_wps_pin(argv[4]);
        }
        cmd_p2p_get_wps_pin(wps_pin);
        printf("WPS PIN %s \n",wps_pin);
    }
    else if(!strcmp(argv[3], "keypad")) {
        char wps_pin[12];
        wps_method = WPS_PIN_KEYPAD;

        if((argc > 4) && (strlen(argv[4]) == 8)) {
            extern void cmd_p2p_set_wps_pin(char *ppin);
            cmd_p2p_set_wps_pin(argv[4]);
        }
        cmd_p2p_get_wps_pin(wps_pin);
        printf("WPS PIN %s \n",wps_pin);
    }
    else if (!strcmp(argv[3], "deauth")) {
        dev_auth = 1;
        wps_method = WPS_NOT_READY;
    }
    else{
        printf("wps mode error.\n");
        return -1;
    }

	cmd_p2p_set_wps_method(wps_method);
    /*peer mac*/
    rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }
    
    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }

    /* TODO */
#if 0
    {
        extern A_STATUS cmd_p2p_get_go_intend(int dev_id, A_UINT8 *pgo_intend_cfg);
        A_UINT8 go_intend_cfg;
        cmd_p2p_get_go_intend(currentDeviceId, &go_intend_cfg);
        printf("dev: %d, go intend: %d\n", currentDeviceId, go_intend_cfg);
    }
#endif

    if (argc >= 5){
        if (strlen(argv[4]) == 8) { //pin
            if ((argc > 5) && (!strcmp(argv[5], "persistent"))) {
                ucpersistent = 1;
            }
        }
        else if (!strcmp(argv[4], "persistent")) {
            ucpersistent = 1;
        }
    }
    cmd_p2p_set_persistent(ucpersistent);
    qcom_p2p_func_auth(currentDeviceId, dev_auth, wps_method, peer_mac, ucpersistent);

    return 0;
}

int cmd_wlan_p2p_connect(int argc, char *argv[])
{
    int rt;
    enum p2p_wps_method wps_method;
    int aidata[6];
    unsigned char ucpersistent = 0;
    unsigned char peer_mac[6];

    if(argc < 4){
        printf("error cmd\n");
        return -1;
    }

    if(!strcmp(argv[3], "push")) {
        wps_method = WPS_PBC;
    }
    else if(!strcmp(argv[3], "display")) {
        extern void cmd_p2p_get_wps_pin(char *ppin);
        char wps_pin[12];
        wps_method = WPS_PIN_DISPLAY;

        if((argc > 4) && (strlen(argv[4]) == 8)) {
            extern void cmd_p2p_set_wps_pin(char *ppin);
            cmd_p2p_set_wps_pin(argv[4]);
        }

        cmd_p2p_get_wps_pin(wps_pin);
        printf("WPS PIN %s \n",wps_pin);
    }
    else if(!strcmp(argv[3], "keypad")) {
        char wps_pin[12];
        wps_method = WPS_PIN_KEYPAD;

		if(argc < 5){
			printf("incomplete parameter\n");
			return -1;
		}

        if((argc > 4) && (strlen(argv[4]) == 8)) {
            extern void cmd_p2p_set_wps_pin(char *ppin);
            cmd_p2p_set_wps_pin(argv[4]);
        }
        cmd_p2p_get_wps_pin(wps_pin);
        printf("WPS PIN %s \n",wps_pin);
    }
    else{
        printf("wps mode error.\n");
        return -1;
    }

	cmd_p2p_set_wps_method(wps_method);
    rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }

    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }

    /* TODO */
#if 0
    {
        extern A_STATUS cmd_p2p_get_go_intend(int dev_id, A_UINT8 *pgo_intend_cfg);
        A_UINT8 go_intend_cfg;
        cmd_p2p_get_go_intend(currentDeviceId, &go_intend_cfg);
        printf("dev=%d, go intend: %d\n", currentDeviceId, go_intend_cfg);
    }
#endif

    if (argc >= 5){
        if (strlen(argv[4]) == 8) { //pin
            if ((argc > 5) && (!strcmp(argv[5], "persistent"))) {
                ucpersistent = 1;
            }
        }
        else if (!strcmp(argv[4], "persistent")) {
            ucpersistent = 1;
        }
    }
    cmd_p2p_set_persistent(ucpersistent);
    qcom_p2p_func_connect(currentDeviceId, wps_method, peer_mac, ucpersistent);

    return 0;
}

int cmd_wlan_p2p_listen(int argc, char *argv[])
{
    A_UINT32 timeout = 30; /* seconds */
    if(argc < 2){
        printf("err cmd\n");
        return -1;
    }

    if(3 == argc) {
        timeout = atoi(argv[2]);
    }

    qcom_p2p_func_listen(currentDeviceId, timeout);
    return 0;
}

/* This command is typically called after successful invitation request/response
 * (either initiated by self or peer) for re-invoking a persistent group
 */
int cmd_wlan_p2p_start_pers(int argc, char *argv[])
{
    if(argc < 3){
        printf("err cmd\n");
        return -1;
    }

    app_p2p_start_persistent_dev(currentDeviceId, argv[2]);
    return 0;
}

int cmd_wlan_p2p_auto_go(int argc, char *argv[])
{
    A_UINT8 ucpersistent = 0;
    A_UINT16 sta_vap_ch = 0;
    
    if(argc < 2){
        printf("err cmd\n");
        return -1;
    }

    if(3 == argc) {
        if(!strcmp(argv[2], "persistent")) {
            ucpersistent = 1;
        }
    }

    cmd_p2p_set_persistent(ucpersistent);

    if (2 == g_wifi_state[(currentDeviceId+1)%(WLAN_NUM_OF_DEVICES)]) {
        qcom_get_channel((currentDeviceId+1)%(WLAN_NUM_OF_DEVICES), &sta_vap_ch);
    } else {
        sta_vap_ch = (!g_p2p_op_ch) ? P2P_DEFAULT_CHAN : g_p2p_op_ch;
    }
    qcom_p2p_func_start_go(currentDeviceId,(A_UINT8 *) P2P_SSID_AUTO_GO,(A_UINT8 *) P2P_PASS_AUTO_GO, sta_vap_ch, ucpersistent);
    return 0;
}

A_STATUS cmd_wlan_p2p_list_network(A_UINT8 dev_id)
{
    P2P_PERSISTENT_MAC_LIST *p_evt;

    if ((p_evt = (P2P_PERSISTENT_MAC_LIST *)mem_alloc(PERSISTENT_LIST_SIZE)) == NULL) {
        return A_ERROR;
    }

    if (qcom_p2p_func_get_network_list(dev_id, p_evt, PERSISTENT_LIST_SIZE)
            == A_OK) {
        int i;
        for(i = 0; i < MAX_LIST_COUNT; i++)
        {
            printf("mac_addr[%d] : %x:%x:%x:%x:%x:%x  \n", i, \
                    p_evt[i].macaddr[0], p_evt[i].macaddr[1],\
                    p_evt[i].macaddr[2], p_evt[i].macaddr[3],\
                    p_evt[i].macaddr[4], p_evt[i].macaddr[5]);
            printf("ssid[%d] : %s \n", i, p_evt[i].ssid);
        }
    }

    mem_free(p_evt);
}

A_STATUS cmd_wlan_p2p_get_nodelist(A_UINT8 dev_id)
{
    P2P_NODE_LIST_EVENT *p_evt;

#define NODELIST_SIZE (sizeof(P2P_NODE_LIST_EVENT) + \
                            (P2P_MAX_NODE_INDIC * sizeof(P2P_DEVICE_LITE)))
    if ((p_evt = (P2P_NODE_LIST_EVENT *)mem_alloc(NODELIST_SIZE)) == NULL) {
        return A_ERROR;
    }

    if (qcom_p2p_func_get_node_list(dev_id, p_evt, NODELIST_SIZE)
            == A_OK) {

        int i;
        P2P_DEVICE_LITE *pdev_node;
        printf("\nnum dev: %d\n", p_evt->num_p2p_dev);
        for (i = 0; i < p_evt->num_p2p_dev; i++) {
            pdev_node = (P2P_DEVICE_LITE *)(p_evt->data + i * sizeof(P2P_DEVICE_LITE));
            printf("\np2p_dev_name    : %s\n", pdev_node->device_name);
            printf("wps_method      : 0x%08x\n", 
                   A_GET_UINT32_FIELD(pdev_node, P2P_DEVICE_LITE, wps_method));
            printf("config_methods  : 0x%04x\n", 
                    A_GET_UINT16_FIELD(pdev_node, P2P_DEVICE_LITE, config_methods));
            printf("persistent_grp  : 0x%02x\n", pdev_node->persistent_grp);
            printf("p2p_int_addr    : %x:%x:%x:%x:%x:%x\n", \
                    pdev_node->interface_addr[0],\
                    pdev_node->interface_addr[1],\
                    pdev_node->interface_addr[2],\
                    pdev_node->interface_addr[3],\
                    pdev_node->interface_addr[4],\
                    pdev_node->interface_addr[5]);
            printf("p2p_dev_addr    : %x:%x:%x:%x:%x:%x\n", \
                    pdev_node->p2p_device_addr[0],\
                    pdev_node->p2p_device_addr[1],\
                    pdev_node->p2p_device_addr[2],\
                    pdev_node->p2p_device_addr[3],\
                    pdev_node->p2p_device_addr[4],\
                    pdev_node->p2p_device_addr[5]);
            printf("p2p_dev_capab   : 0x%02x\n", \
                    pdev_node->dev_capab);
            printf("p2p_grp_capab   : 0x%02x\n", \
                    pdev_node->group_capab);
        }
    }

    mem_free(p_evt);
}

int cmd_wlan_p2p_get(int argc, char *argv[])
{
    if(argc < 2){
        printf("err cmd\n");
        return -1;
    }
    
    if (!strcmp(argv[2], "passphrase")){
        A_CHAR szpass[65];
        A_CHAR szssid[MAX_SSID_LEN+1];
        memset(szpass, 0, sizeof(szpass));
        memset(szssid, 0, sizeof(szssid));
        qcom_p2p_func_get_pass_ssid(currentDeviceId, szpass, szssid);
        printf("SSID : %s, PASS : %s.\n", szssid, szpass);
    }
    else {
        printf("err cmd\n");
        return -1;
    }

    return 0;
}

int cmd_wlan_p2p_set(int argc, char *argv[])
{
    int len;
    void *pval = NULL;
    P2P_CONF_ID cfg_id;

    if(argc < 4){
        printf("err cmd\n");
        return -1;
    }

    if(!strcmp(argv[2], "p2pmode")) {
        pval = mem_alloc(sizeof(int));
        if (NULL == pval) {
            return -1;
        }
        
        if(!strcmp(argv[3], "p2pdev"))
        {
            *(int *)pval = P2P_DEV;
        }
        else if(!strcmp(argv[3], "p2pclient"))
        {
            *(int *)pval = P2P_CLIENT;
        }
        else if(!strcmp(argv[3], "p2pgo"))
        {
            *(int *)pval = P2P_GO;
        }
        else
        {
            printf("p2p mode error.\n");
            mem_free(pval);
            return 0;
        }

        len = 4;
        cfg_id = P2P_CONFIG_P2P_OPMODE;
    }
    else if(!strcmp(argv[2], "gointent")) {
        pval = mem_alloc(sizeof(int));
        if (NULL == pval) {
            return -1;
        }
        
        *(int *)pval = atoi(argv[3]);
        len = 4;
        cfg_id = P2P_CONFIG_GO_INTENT; 
    }
    else if(!strcmp(argv[2], "postfix")) {
        pval = mem_alloc(sizeof(P2P_SET_SSID_POSTFIX));
        if (NULL == pval) {
            return -1;
        }

        memset(pval, 0, sizeof(P2P_SET_SSID_POSTFIX));
        
        ((P2P_SET_SSID_POSTFIX *)pval)->ssid_postfix_len = strlen(argv[3]);
        if (((P2P_SET_SSID_POSTFIX *)pval)->ssid_postfix_len >= \
            sizeof(((P2P_SET_SSID_POSTFIX *)pval)->ssid_postfix)) {
            ((P2P_SET_SSID_POSTFIX *)pval)->ssid_postfix_len = sizeof(((P2P_SET_SSID_POSTFIX *)pval)->ssid_postfix) - 1;
        }
        memcpy(&(((P2P_SET_SSID_POSTFIX *)pval)->ssid_postfix), \
                      argv[3], \
                      ((P2P_SET_SSID_POSTFIX *)pval)->ssid_postfix_len);
        
        len = sizeof(P2P_SET_SSID_POSTFIX);
        cfg_id = P2P_CONFIG_SSID_POSTFIX;
    }
    else if(!strcmp(argv[2], "intrabss")) {
        pval = mem_alloc(sizeof(P2P_SET_INTRA_BSS));
        if (NULL == pval) {
            return -1;
        }

        memset(pval, 0, sizeof(P2P_SET_INTRA_BSS));

        ((P2P_SET_INTRA_BSS *)pval)->flag = atoi(argv[3]);
        len = sizeof(P2P_SET_INTRA_BSS);
        cfg_id = P2P_CONFIG_INTRA_BSS;
    }
    else if(!strcmp(argv[2], "name")) {
        pval = mem_alloc(sizeof(P2P_SET_DEV_NAME));
        if (NULL == pval) {
            return -1;
        }

        memset(pval, 0, sizeof(P2P_SET_DEV_NAME));

        if (strlen(argv[3]) >= WPS_MAX_DEVNAME_LEN) {
            mem_free(pval);
            return -1;
        }

        strcpy((char *)(((P2P_SET_DEV_NAME *)pval)->dev_name), argv[3]);
        ((P2P_SET_DEV_NAME *)pval)->dev_name_len = strlen(argv[3]);
        cfg_id = P2P_CONFIG_DEV_NAME;
	    len = sizeof(P2P_SET_DEV_NAME);
    }
    else {
        printf("Do not support.\n");
        return 0;
    }

    qcom_p2p_func_set(currentDeviceId, cfg_id, pval, len);
    mem_free(pval);

    return 0;
}

int cmd_wlan_p2p_set_config(int argc, char *argv[])
{
    unsigned int uigo_intend;
    unsigned int uilistend_ch;
    unsigned int uiop_ch;
    unsigned int uiage_timeout;

    if(argc < 7){
        printf("err cmd\n");
        return -1;
    }

    uigo_intend = atoi(argv[2]);
    uilistend_ch = atoi(argv[3]);
    uiop_ch = atoi(argv[4]);
    /* Removed setting country code from qcom_p2p_func_set_config. However
     * retain current shell usage (argv[5] is country) so as to not break CST tests/scripts
     */
    uiage_timeout = atoi(argv[6]);

    /*Store Operating Channel*/
    g_p2p_op_ch = uiop_ch;

    qcom_p2p_func_set_config(currentDeviceId, uigo_intend, uilistend_ch, uiop_ch, uiage_timeout,
            81, 81, 5);
    
    return 0;
}

int cmd_wlan_p2p_key(int argc, char *argv[])
{
    int rt;
    int aidata[6];
    unsigned char peer_mac[6];

    if(argc < 4){
        printf("Incomplete parameter\n");
		printf("Usage: wmiconfig --p2p key <pin> <peer_mac>\n");
        return -1;
    }

	if((strlen(argv[2]) != 8)) {
        printf("Incomplete parameter\n");
		printf("Usage: wmiconfig --p2p key <pin> <peer_mac>\n");
        return -1;
	 }

    cmd_p2p_set_wps_pin(argv[2]);	

    rt = sscanf(argv[3], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }

    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }

	
	A_STATUS cmd_p2p_set_peer_mac(A_UINT8 *pmac);
	cmd_p2p_set_peer_mac(peer_mac);

    return 0;
}


int cmd_wlan_p2p_invite(int argc, char *argv[])
{
    int rt;
    int wps_method;
    int aidata[6];
    unsigned char ucpersistent = 0;
    unsigned char peer_mac[6];
    /* role doesn't matter in case of non-persistent invite. So initialize to
     * some value */
    P2P_INV_ROLE role = P2P_INV_ROLE_CLIENT; 

    if(argc < 4){
        printf("err cmd\n");
        return -1;
    }

    rt = sscanf(argv[3], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }

    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }

    if(!strcmp(argv[4], "push")) {
        wps_method = WPS_PBC;
    }
    else if(!strcmp(argv[4], "display")) {
        wps_method = WPS_PIN_DISPLAY;
    }
    else if(!strcmp(argv[4], "keypad")) {
        wps_method = WPS_PIN_KEYPAD;
    }
    else{
        printf("wps mode error.\n");
        return -1;
    }

    if (argc > 5) {
         if (!strcmp(argv[5], "persistent")) {
            ucpersistent = 1;
        }
    }
    if (ucpersistent) {
        P2P_PERSISTENT_MAC_LIST p2p_pers_list[MAX_LIST_COUNT];
        if (qcom_p2p_func_get_network_list(currentDeviceId, p2p_pers_list, PERSISTENT_LIST_SIZE) == A_OK) {
            A_UINT32 index = cmd_p2p_get_persistent_index((A_CHAR *)argv[2], p2p_pers_list);
            if (index == MAX_LIST_COUNT) {
                /* Persistent network not found. Return error */
                return A_ERROR;
            }
            role = qcom_p2p_func_get_role(p2p_pers_list[index].role);
        }
    }

    /*GO only:P2P_INVITE_ROLE_GO*/
    qcom_p2p_func_invite(currentDeviceId, argv[2], wps_method, peer_mac, ucpersistent, role);

    return 0;
}

int cmd_wlan_p2p_invite_auth(int argc, char *argv[])
{
    int rt;
    int aidata[6];
    unsigned char peer_mac[6];
    P2P_FW_INVITE_REQ_RSP_CMD invite_rsp_cmd;

    if(argc < 2){
        printf("err cmd\n");
        return -1;
    }

    rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }
    
    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }

    memset(&invite_rsp_cmd, 0, sizeof(invite_rsp_cmd));
    memcpy(invite_rsp_cmd.group_bssid, peer_mac, 6);

    if((argc > 3) && (!strcmp(argv[3], "reject"))) {
        invite_rsp_cmd.status = 1;
    }

    qcom_p2p_func_invite_auth(currentDeviceId, &invite_rsp_cmd);

    return 0;
}

int cmd_wlan_p2p_join(int argc, char *argv[])
{
    int rt;
    enum p2p_wps_method wps_method;
    int aidata[6];
    unsigned char peer_mac[6];
    char wps_pin[8];

    if(argc < 4){
        printf("err cmd\n");
        return -1;
    }

    rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }

    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }

    wps_pin[0] = 0;
    
    if(!strcmp(argv[3], "push")) {
        wps_method = WPS_PBC;
    }
    else if(!strcmp(argv[3], "display")) {
        wps_method = WPS_PIN_DISPLAY;
        cmd_p2p_get_wps_pin(wps_pin);
        printf("WPS PIN %s \n",wps_pin);
    }
    else if(!strcmp(argv[3], "keypad")) {
        wps_method = WPS_PIN_KEYPAD;
        if (argc == 4) {
            cmd_p2p_get_wps_pin(wps_pin);
        }
        else {
            if (strlen(argv[4]) > 8) {
                return -1;
            }
            strcpy(wps_pin, argv[4]);
        }
    }
    else{
        return -1;
    }

	cmd_p2p_set_wps_method(wps_method);
    qcom_p2p_func_join(currentDeviceId, wps_method, peer_mac, wps_pin);

    return 0;
}

int cmd_wlan_p2p_join_profile(int argc, char *argv[])
{
    int rt;
    int aidata[6];
    unsigned char peer_mac[6];
    
    if(argc < 3){
        printf("err cmd\n");
        return -1;
    }

    rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }

    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }
    
    qcom_p2p_func_join_profile(currentDeviceId, peer_mac);

    return 0;
}

int cmd_wlan_p2p_prov(int argc, char *argv[])
{
    int rt;
    enum p2p_wps_method wps_method;
    int aidata[6];
    unsigned char peer_mac[6];

    if(argc < 4){
        printf("err cmd\n");
        return -1;
    }

    rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
    if (rt < 0)
    {
        printf("wrong mac format.\n");
        return -1;
    }

    for (rt = 0; rt < 6; rt++)
    {
        peer_mac[rt] = (unsigned char)aidata[rt];
    }

    if(!strcmp(argv[3], "push")) {
        wps_method = WPS_PBC;
    }
    else if(!strcmp(argv[3], "display")) {
        wps_method = WPS_PIN_DISPLAY;
    }
    else if(!strcmp(argv[3], "keypad")) {
        wps_method = WPS_PIN_KEYPAD;
    }
    else{
        printf("wps mode error.\n");
        return -1;
    }
    qcom_p2p_func_prov(currentDeviceId, wps_method, peer_mac);

    return 0;
}

int cmd_wlan_p2p_opps(int argc, char *argv[])
{
    A_UINT8 enable;
    A_UINT8 ctwin;
        
    if(argc < 4){
        printf("err cmd\n");
        return -1;
    }
    
    enable = atoi(argv[2]);
    ctwin = atoi(argv[3]);
    qcom_p2p_func_set_opps(currentDeviceId, enable, ctwin);

    return 0;
}

int cmd_wlan_p2p_noa(int argc, char *argv[])
{
    A_UINT8 uccount;
    A_UINT32 uistart;
    A_UINT32 uiduration;
    A_UINT32 uiinterval;
    
    if(argc < 6){
        printf("err cmd\n");
        return -1;
    }

    uccount = atoi(argv[2]);
    uistart = atoi(argv[3]);
    uiduration = atoi(argv[4]);
    uiinterval = atoi(argv[5]);
    qcom_p2p_func_set_noa(currentDeviceId, uccount, uistart, uiduration, uiinterval);
    
    return 0;
}

A_STATUS app_p2p_start_wps(A_UINT8 devid)
{
    char szpin[WPS_PIN_LEN+1] = {0};
    A_UINT8 wps_method;
    extern A_STATUS (* qcom_sta_connect_state_event_cb)(A_UINT8 device_id);

    cmd_p2p_get_wps_method(&wps_method);
    cmd_p2p_get_wps_pin(szpin);

    qcom_wps_start(devid, 1, (wps_method == WPS_PBC) ? 1 : 0, szpin);

    qcom_sta_connect_state_event_cb = NULL;

    return A_OK;
}

A_STATUS app_p2p_event_sd_pd(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    P2P_SDPD_RX_EVENT *p_tmp;

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }

    p_tmp = (P2P_SDPD_RX_EVENT *)pEvtBuffer;

    printf("type : %d   frag id : %x \n", p_tmp->type, p_tmp->frag_id);
    printf("transaction_status : %x \n", p_tmp->transaction_status);
    printf("freq : %d status_code : %d comeback_delay : %d tlv_length : %d update_indic : %d \n", \
                      A_GET_UINT16_FIELD(p_tmp, P2P_SDPD_RX_EVENT, freq), \
                      A_GET_UINT16_FIELD(p_tmp, P2P_SDPD_RX_EVENT, status_code), \
                      A_GET_UINT16_FIELD(p_tmp, P2P_SDPD_RX_EVENT, comeback_delay), \
                      A_GET_UINT16_FIELD(p_tmp, P2P_SDPD_RX_EVENT, tlv_length), \
                      A_GET_UINT16_FIELD(p_tmp, P2P_SDPD_RX_EVENT, update_indic));
    printf("source addr : %x:%x:%x:%x:%x:%x \n", \
                      p_tmp->peer_addr[0], p_tmp->peer_addr[1], p_tmp->peer_addr[2], \
                      p_tmp->peer_addr[3], p_tmp->peer_addr[4], p_tmp->peer_addr[5]);

    return A_OK;
}

A_STATUS app_p2p_event_prov_resp(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    A_UINT16 data_tmp;
    P2P_PROV_DISC_RESP_EVENT *p_tmp;

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }

    p_tmp = (P2P_PROV_DISC_RESP_EVENT *)pEvtBuffer;
    printf("\npeer mac : %x:%x:%x:%x:%x:%x\n", \
                                p_tmp->peer[0], p_tmp->peer[1], p_tmp->peer[2],\
                                p_tmp->peer[3], p_tmp->peer[4], p_tmp->peer[5]);
    data_tmp = A_GET_UINT16_FIELD(p_tmp, \
                                                        P2P_PROV_DISC_RESP_EVENT, \
                                                        config_methods);
    printf("wps method : 0x%04x\n", data_tmp);
    switch (data_tmp) {
        case WPS_CONFIG_DISPLAY:
        {
            printf("provisional disc resp - Display\n");
            break;
        }
        case WPS_CONFIG_KEYPAD:
        {
            char szpin[WPS_PIN_LEN+1];
            A_MEMZERO(szpin, sizeof(szpin));
            cmd_p2p_get_wps_pin(szpin);
            printf("provisional disc resp - WPS PIN [%s]\n", szpin);
            break;
        }
        case WPS_CONFIG_PUSHBUTTON:
        {
            printf("provisional disc resp - Push Button\n");
            break;
        }
        default:
            printf("invalid provisional request \n");
            break;
    }

    return A_OK;
}

A_STATUS app_p2p_event_auth_req(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    P2P_REQ_TO_AUTH_EVENT *p_tmp;

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }

    p_tmp = (P2P_REQ_TO_AUTH_EVENT *)pEvtBuffer;
    printf("\nSA : %x:%x:%x:%x:%x:%x\n", \
                                p_tmp->sa[0], p_tmp->sa[1], p_tmp->sa[2],\
                                p_tmp->sa[3], p_tmp->sa[4], p_tmp->sa[5]);
    printf("dev_password_id : 0x%04x\n", \
                       A_GET_UINT16_FIELD(p_tmp, P2P_REQ_TO_AUTH_EVENT, dev_password_id));

    return A_OK;
}

A_STATUS app_p2p_start_persistent_dev(int device_id, A_CHAR *pssid)
{
    P2P_INV_ROLE role;
    P2P_PERSISTENT_MAC_LIST  p2p_pers_list[MAX_LIST_COUNT];
    A_UINT16 sta_vap_ch = 0;

    if (NULL == pssid) {
        return A_ERROR;
    }
    if (qcom_p2p_func_get_network_list(device_id, p2p_pers_list, PERSISTENT_LIST_SIZE)
            == A_OK) {
        A_CHAR pass[MAX_SSID_LEN];
        A_UINT32 index = cmd_p2p_get_persistent_index(pssid, p2p_pers_list);
        if (index == MAX_LIST_COUNT) {
            /* Persistent network not found. Return error */
            return A_ERROR;
        }
        A_STRNCPY(pass, (A_CHAR *)p2p_pers_list[index].passphrase, MAX_SSID_LEN);
        role = qcom_p2p_func_get_role(p2p_pers_list[index].role);

        if ((role == P2P_INV_ROLE_GO) || (role == P2P_INV_ROLE_ACTIVE_GO)) {
            if (2 == g_wifi_state[(device_id+1)%(WLAN_NUM_OF_DEVICES)]) {
                qcom_get_channel((device_id+1)%(WLAN_NUM_OF_DEVICES), &sta_vap_ch);
            } else {
                sta_vap_ch = (!g_p2p_op_ch) ? P2P_DEFAULT_CHAN : g_p2p_op_ch;
            }
            qcom_p2p_func_start_go(device_id, (A_UINT8 *)pssid, (A_UINT8 *)pass, sta_vap_ch, 1);
            return A_OK;
        }
    }
    return A_ERROR;
}


A_STATUS app_p2p_event_invite_req(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    P2P_FW_INVITE_REQ_EVENT *p_tmp;

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }
    p_tmp = (P2P_FW_INVITE_REQ_EVENT *)pEvtBuffer;
    printf("\nP2P_FW_INVITE_REQ_EVENT: Invite from ");
    printf("%x:%x:%x:%x:%x:%x\n", \
                    p_tmp->go_dev_addr[0], p_tmp->go_dev_addr[1], \
                    p_tmp->go_dev_addr[2], p_tmp->go_dev_addr[3], \
                    p_tmp->go_dev_addr[4], p_tmp->go_dev_addr[5]);
    printf("SSID: %s\n", p_tmp->ssid.ssid);

    /* Application thread needs to call invite_auth after this. invite_auth
     * should not be called in the WMI event handler which is running in the context
     * of a proxy pseudo host thread
     */
    return A_OK;
}

A_STATUS app_p2p_event_invite_result_send(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    P2P_INVITE_SENT_RESULT_EVENT *p_tmp;

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }

    p_tmp = (P2P_INVITE_SENT_RESULT_EVENT *)pEvtBuffer;
    printf("\nP2P_INVITE_SENT_RESULT_EVENT result : %d\n", p_tmp->status);
    printf("bssid : %x:%x:%x:%x:%x:%x\n", \
                                        p_tmp->bssid[0], p_tmp->bssid[1], p_tmp->bssid[2],\
                                        p_tmp->bssid[3], p_tmp->bssid[4], p_tmp->bssid[5]);
    return A_OK;
}

A_STATUS app_p2p_event_invite_result_recv(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    P2P_INVITE_RCVD_RESULT_EVENT *p_tmp;

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }

    p_tmp = (P2P_INVITE_RCVD_RESULT_EVENT *)pEvtBuffer;
    printf("\nP2P_INVITE_RCVD_RESULT_EVENT result : %d\n", p_tmp->status);
    printf("sa : %x:%x:%x:%x:%x:%x\n", \
                                        p_tmp->sa[0], p_tmp->sa[1], p_tmp->sa[2],\
                                        p_tmp->sa[3], p_tmp->sa[4], p_tmp->sa[5]);
    printf("SSID: %s\n", p_tmp->ssid.ssid);
    return A_OK;
}

A_STATUS app_p2p_event_go_neg(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    P2P_GO_NEG_RESULT_EVENT *p_tmp;
    char szpin[WPS_PIN_LEN+1] = {0};

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }

    p_tmp = (P2P_GO_NEG_RESULT_EVENT *)pEvtBuffer;

    printf("P2P GO neg result :\n");
    printf("Status       :%s \n",(p_tmp->status) ? "FAILURE":"SUCCESS");

    if (0 == p_tmp->status)
    {
        A_UINT16 chnnl;

        printf("P2P Role     :%s \n",(p_tmp->role_go) ? "GO": "Client");
        printf("SSID         :%s \n",p_tmp->ssid);

        chnnl = (*(A_UINT8 *)&(p_tmp->freq)) | \
                (*(((A_UINT8 *)&(p_tmp->freq)) + 1) << 8);
        printf("Channel      :%d \n",chnnl);
        printf("WPS Method   :%s \n",(p_tmp->wps_method == WPS_PBC) ? "PBC": "PIN");

        qcom_set_ssid(device_id, (A_CHAR *)(p_tmp->ssid));

        cmd_p2p_set_peer_mac(p_tmp->peer_interface_addr); //new
        cmd_p2p_set_role(p_tmp->role_go);
        cmd_p2p_set_wps_method(p_tmp->wps_method);
        cmd_p2p_get_wps_pin(szpin);

        if (p_tmp->role_go == 1)
        {
            A_UINT8 ucpersistent = cmd_p2p_get_persistent();
            extern A_STATUS (* qcom_sta_connect_state_event_cb)(A_UINT8 device_id);

            qcom_p2p_func_start_go(device_id, \
                    (A_UINT8 *)p_tmp->ssid, \
                    (A_UINT8 *)p_tmp->pass_phrase, \
                    (chnnl - 2412) / 5 + 1, \
                    ucpersistent);

            /* The previous _qcom_sta_connect_event_wait() uses
                        * tx_queue_receive()/tx_queue_send() to realize the Async
                        * mechanism, but actually we're already in the task "CDR proxy",
                        * this TX task is used for processing event, and the wakeup
                        * func _qcom_sta_connect_event_wakeup() is also in this task
                        * context, therefore it becomes such scenario: I'm waiting myself
                        * to wake up myself. So, finally it times out. To solve this, use
                        * CB to realize the Async mechanism.
                        * Start WPS after the AP connect event, in the cb of the connect event */
            //qcom_sta_connect_event_wait(device_id, &state);
            //qcom_wps_start(device_id, 1, p_tmp->wps_method, szpin);
            qcom_sta_connect_state_event_cb = app_p2p_start_wps;
        }
        else {
            _tx_thread_sleep(800);
        }

        if ((p_tmp->role_go == 0)) 
        {
            A_BOOL push_method = (p_tmp->wps_method == WPS_PBC) ? TRUE : FALSE;
            qcom_p2p_func_wps_start_no_scan(device_id, p_tmp->role_go, push_method, \
                    chnnl, p_tmp->ssid, \
                    (A_UINT8 *)szpin,\
                    p_tmp->peer_interface_addr);
        }
    }

    return A_OK;
}

A_STATUS app_p2p_event_prov_req(A_UINT8 device_id, A_UINT8 * pEvtBuffer, int len)
{
    A_UINT16 uswps_config_method;
    P2P_PROV_DISC_REQ_EVENT *p_tmp;

    if (NULL == pEvtBuffer) {
        return A_ERROR;
    }

    p_tmp = (P2P_PROV_DISC_REQ_EVENT *)pEvtBuffer;
    uswps_config_method = A_GET_UINT16_FIELD(p_tmp, \
                                             P2P_PROV_DISC_REQ_EVENT, \
                                             wps_config_method);
    printf("\nSA : %x:%x:%x:%x:%x:%x\n", \
                                p_tmp->sa[0], p_tmp->sa[1], p_tmp->sa[2],\
                                p_tmp->sa[3], p_tmp->sa[4], p_tmp->sa[5]);
    printf("wps method : 0x%04x\n", uswps_config_method);
    switch (uswps_config_method) {
        case WPS_CONFIG_DISPLAY:
        {
            char szpin[WPS_PIN_LEN+1];
            A_MEMZERO(szpin, sizeof(szpin));
            cmd_p2p_get_wps_pin(szpin);
            printf("provisional disc request - Display WPS PIN [%s]\n", szpin);
            break;
        }
        case WPS_CONFIG_KEYPAD:
        {
            printf("provisional disc request - Enter WPS PIN\n");
            break;
        }
        case WPS_CONFIG_PUSHBUTTON:
        {
            printf("provisional disc request - Push Button\n");
            break;
        }
        default:
            printf("invalid provisional request\n");
            break;
    }

    return A_OK;
}

int app_p2p_event_handler(A_UINT8 device_id, P2P_EVENT_ID event_id, A_UINT8 *pBuffer, A_UINT32 Length, void *arg)
{
    switch (event_id) {
        case P2P_GO_NEG_RESULT_EVENTID:
        {
            app_p2p_event_go_neg(device_id, pBuffer, Length);
            break;
        }
        case P2P_PROV_DISC_REQ_EVENTID:
        {
            app_p2p_event_prov_req(device_id, pBuffer, Length);
            break;
        }
        case P2P_SDPD_RX_EVENTID:
        {
            app_p2p_event_sd_pd(device_id, pBuffer, Length);
            break;
        }
        case P2P_START_SDPD_EVENTID:
        {
            printf("start to SDPD.\n");
            break;
        }
        case P2P_PROV_DISC_RESP_EVENTID:
        {
            app_p2p_event_prov_resp(device_id, pBuffer, Length);
            break;
        }
        case P2P_REQ_TO_AUTH_EVENTID:
        {
            app_p2p_event_auth_req(device_id, pBuffer, Length);
            break;
        }
        case P2P_INVITE_REQ_EVENTID:
        {
            app_p2p_event_invite_req(device_id, pBuffer, Length);
            break;
        }
        case P2P_INVITE_SENT_RESULT_EVENTID:
        {
            app_p2p_event_invite_result_send(device_id, pBuffer, Length);
            break;
        }
        case P2P_INVITE_RCVD_RESULT_EVENTID:
        {
            app_p2p_event_invite_result_recv(device_id, pBuffer, Length);
            break;
        }
        default :
            break;
    }
}

int cmd_wlan_p2p(int argc, char *argv[])
{
    if(argc < 2){
        printf("wmiconfig --p2p <on|off>\n");
        printf("wmiconfig --p2p status\n");
        printf("wmiconfig --p2p cancel\n");
        printf("wmiconfig --p2p nodelist\n");
        printf("wmiconfig --p2p list_network\n");
        printf("wmiconfig --p2p autogo [persistent]\n");
        printf("wmiconfig --p2p passphrase <pass> <ssid>\n");
        printf("wmiconfig --p2p auth <peer_mac> <wps_method|deauth> [pin-num] [persistent]\n");
        printf("wmiconfig --p2p connect <peer_mac> <wps_method> [wpspin] [persistent]\n");
        printf("wmiconfig --p2p invite <ssid> <mac> <wps_method> [persistent]\n");
        printf("wmiconfig --p2p invite_auth <gomac> [reject]\n");
        printf("wmiconfig --p2p join <intf_mac> <wps_method> [wpspin] [persistent]\n");
        printf("wmiconfig --p2p prov <peer_mac> <wps_method>\n");
        printf("wmiconfig --p2p listen [timeout in seconds]\n");
        printf("wmiconfig --p2p start_pers <ssid>\n");
        printf("wmiconfig --p2p find [full|social|prog : default:social] [timeout:default-300s]\n");
        printf("wmiconfig --p2p get passphrase\n");
        printf("wmiconfig --p2p set <p2pmode p2pdev|p2pclient|p2pgo> | <name val>| <gointent val> | <persistent val>\n");
        printf("wmiconfig --p2p set <intrabss 0|1> | <postfix val(<=22)>\n");
        printf("wmiconfig --p2p setconfig <go_intent> <listen_channel> <op_channel> <country> <node_age_timeout>\n");
        printf("wmiconfig --p2p setoppps <enable> <ctwin>\n");
        printf("wmiconfig --p2p setnoa  <count [1 - 255]> <start in ms> <duration in ms> <interval in ms>\n");
        return -1;
    }

    if(!strcmp(argv[1], "on")) {
        qcom_p2p_func_init(currentDeviceId, 1);
        qcom_p2p_func_register_event_handler(app_p2p_event_handler, NULL);
        cmd_p2p_set_wps_pin("12345670");
    }
    else if(!strcmp(argv[1], "off")) {
        qcom_p2p_func_register_event_handler(NULL, NULL);
        qcom_p2p_func_init(currentDeviceId, 0);
    }
    else if(!strcmp(argv[1], "status")) {
        extern A_STATUS _qcom_p2p_show_status(int dev_id);
        return _qcom_p2p_show_status(currentDeviceId);
    }
    else if(!strcmp(argv[1], "find")) {
        return cmd_wlan_p2p_find(argc, argv);
    }
    else if(!strcmp(argv[1], "stop")) {
        return qcom_p2p_func_stop_find(currentDeviceId);
    }
    else if(!strcmp(argv[1], "cancel")) {
        return qcom_p2p_func_cancel(currentDeviceId);
    }
    else if(!strcmp(argv[1], "nodelist")) {
        cmd_wlan_p2p_get_nodelist(currentDeviceId);
    }
    else if(!strcmp(argv[1], "list_network")) {
        cmd_wlan_p2p_list_network(currentDeviceId);
    }
    else if (!strcmp(argv[1], "passphrase")) {
        return cmd_wlan_p2p_pass(argc, argv);
    }
    else if(!strcmp(argv[1], "auth")) {
        return cmd_wlan_p2p_auth(argc, argv);
    }
    else if(!strcmp(argv[1], "connect")) {
        return cmd_wlan_p2p_connect(argc, argv);
    }
    else if(!strcmp(argv[1], "listen")) {
        return cmd_wlan_p2p_listen(argc, argv);
    }
    else if(!strcmp(argv[1], "start_pers")) {
        return cmd_wlan_p2p_start_pers(argc, argv);
    }
    else if(!strcmp(argv[1], "autogo")) {
        cmd_wlan_p2p_auto_go(argc, argv);
    }
    else if(!strcmp(argv[1], "get")) {
        return cmd_wlan_p2p_get(argc, argv);
    }
    else if(!strcmp(argv[1], "set")) {
        return cmd_wlan_p2p_set(argc, argv);
    }
    else if(!strcmp(argv[1], "setconfig")) {
        return cmd_wlan_p2p_set_config(argc, argv);
    }
    else if(!strcmp(argv[1], "invite")) {
        return cmd_wlan_p2p_invite(argc, argv);
    }
    else if(!strcmp(argv[1], "invite_auth")) {
        return cmd_wlan_p2p_invite_auth(argc, argv);
    }
    else if(!strcmp(argv[1], "join")) {
        return cmd_wlan_p2p_join(argc, argv);
    }
    else if(!strcmp(argv[1], "prov")) {
        return cmd_wlan_p2p_prov(argc, argv);
    }
    else if(!strcmp(argv[1], "setoppps")) {
        return cmd_wlan_p2p_opps(argc, argv);
    }
    else if(!strcmp(argv[1], "setnoa")) {
        return cmd_wlan_p2p_noa(argc, argv);
    }
    else{
        printf("error cmd.\n");
        return -1;
    }

    return 0;
}

void swat_wmiconfig_p2p(int argc, char *argv[])
{
    cmd_wlan_p2p(argc,argv);

    return;
}
#endif

