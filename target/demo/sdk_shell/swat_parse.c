/*
  * Copyright (c) 2015 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "swat_parse.h"
#include "qcom/socket_api.h"
#include "swat_bench_core.h"
#include "swat_bench_iniche_1.1.2.h"
#include "swat_wmiconfig_p2p.h"
#include "qcom/qcom_ssl.h"
#include "qcom/qcom_scan.h"
#include "qcom/qcom_set_Nf.h"
#include "hwcrypto_api.h"
#include "malloc_api.h"
#include "qcom/qcom_gpio.h"
#include "ezxml.h"
#include "string.h"
#include "qcom/qcom_mqtt_interface.h"
#include "qcom_dset.h"

#ifdef ENABLE_JSON
#include "json.h"
#endif //#ifdef ENABLE_JSON

#ifdef SSDP_ENABLED
#include "qcom/qcom_network.h"
#endif //#ifdef SSDP_ENABLED

#if defined(WLAN_BTCOEX_ENABLED)
#include "swat_wmiconfig_btcoex.h"
#endif

#ifdef ENABLE_MODULE 
#include "module_api.h"
#endif
extern int atoul(char *buf);
extern SSL_ROLE_T ssl_role;
A_UINT8 currentDeviceId = 0;
A_UINT8 bridge_mode = 0;
A_UINT8 ssl_flag_for_https = 0;
char *test_html[] = {"<html><head><title>IOE Ruby</title></head><body><br/><h1>Dynamic Page Test!</h1><br/></body>\n</html>", NULL};
A_UINT32 max_httpc_body_len = MAX_HTTCP_BODY_LEN;


#ifdef ENABLE_JSON
/*JSON global var*/
json_t* jsonPtr = NULL;
#endif //#ifdef ENABLE_JSON

extern unsigned long boot_time;


A_INT32
swat_boot_time(A_INT32 argc, A_CHAR *argv[]) {

    SWAT_PTF("Boot time is %ld \n", boot_time);

}

A_INT32
swat_ezxml_handle(A_INT32 argc, A_CHAR *argv[]) {

   char *s;    
   size_t len;
   A_UINT32 i=0;
   
   ezxml_t test = ezxml_parse_fd(0x403);
   s = ezxml_toxml(test);
   len = A_STRLEN(s);

   for(i=0;i<len;i++) {

       SWAT_PTF("%c", s[i]);

   }

   SWAT_PTF("\n");

   free(s);
   ezxml_free(test);

}

A_INT32
swat_iwconfig_scan_handle(A_INT32 argc, A_CHAR * argv[])
{
    A_UCHAR wifi_state = 0;
    A_CHAR* pssid;
    QCOM_BSS_SCAN_INFO* pOut;
    A_UINT16 count;
    A_CHAR saved_ssid[WMI_MAX_SSID_LEN+1];

    qcom_get_state(currentDeviceId, &wifi_state);
    A_MEMSET(saved_ssid, 0, WMI_MAX_SSID_LEN+1);
    qcom_get_ssid(currentDeviceId, saved_ssid);
    printf("\nshell> ");
    
    //if(wifi_state!=2)
    {
        //if not connected, scan
        if (argc == 2 || argc == 1) {
            pssid = "";
        } else if (argc == 3) {
            pssid = argv[2];
        } else {
        	A_CHAR ssid[32];
			A_UINT8 i, j;

			j = 0;
			A_MEMSET(ssid, 0, 32);
			for (i=2; i<argc; i++)
			{
				if ((j + strlen(argv[i])) > 32)
				{
					SWAT_PTF("ERROR: ssid > 32\n");
					return SWAT_ERROR;
				}
				else
				{
					do
					{
						ssid[j] = *argv[i];
						j++;
					} while (*argv[i]++);
				}

				ssid[j-1] = 0x20; //space
			}
			ssid[j-1] = 0;
            pssid = ssid;
        }
    }

    /*Set the SSID*/
    qcom_set_ssid(currentDeviceId, pssid);

    /*Start scan*/
//  TODO: USE_QCOM_REL_3_3_API is for KF, what's for Ruby?
//#ifndef USE_QCOM_REL_3_3_API
#if 1
        qcom_start_scan_params_t scanParams;
        scanParams.forceFgScan  = 1;
        scanParams.scanType     = WMI_LONG_SCAN;
        scanParams.numChannels  = 0;
        scanParams.forceScanIntervalInMs = 1;
        scanParams.homeDwellTimeInMs = 0;       
        qcom_set_scan(currentDeviceId, &scanParams);
#else
    qcom_set_scan(currentDeviceId);
#endif

    /*Get scan results*/
    if(qcom_get_scan(currentDeviceId, &pOut, &count) == A_OK){
        QCOM_BSS_SCAN_INFO* info = pOut;
        int i,j;

        tx_thread_sleep(1000);
        for (i = 0; i < count; i++)
        {
            printf("ssid = ");
            {
                for(j = 0;j < info[i].ssid_len;j++)
                {
                    printf("%c",info[i].ssid[j]);
                }
                printf("\n");
            }
#if 1 
            //tx_thread_sleep(10);
            printf("bssid = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",info[i].bssid[0],info[i].bssid[1],info[i].bssid[2],info[i].bssid[3],info[i].bssid[4],info[i].bssid[5]);

        //   tx_thread_sleep(10);
            printf("channel = %d\n",info[i].channel);

            printf("indicator = %d\n",info[i].rssi);

            printf("security = ");
            if(info[i].security_enabled){
                printf("\n");
                if(info[i].rsn_auth || info[i].rsn_cipher){
                    printf("RSN/WPA2= ");

                if(info[i].rsn_auth){
                    printf("{");
                    if(info[i].rsn_auth & SECURITY_AUTH_1X){
                        printf("802.1X ");
                    }
                    if(info[i].rsn_auth & SECURITY_AUTH_PSK){
                        printf("PSK ");
                    }
                    printf("}");
                }
                if(info[i].rsn_cipher){
                    printf("{");
                    if(info[i].rsn_cipher & ATH_CIPHER_TYPE_WEP){
                        printf("WEP ");
                    }
                    if(info[i].rsn_cipher & ATH_CIPHER_TYPE_TKIP){
                        printf("TKIP ");
                    }
                    if(info[i].rsn_cipher & ATH_CIPHER_TYPE_CCMP){
                        printf("AES ");
                    }
                    printf("}");
                }
                printf("\n");
            }
            if(info[i].wpa_auth || info[i].wpa_cipher){
                printf("WPA= ");

                if(info[i].wpa_auth){
                    printf("{");
                    if(info[i].wpa_auth & SECURITY_AUTH_1X){
                        printf("802.1X ");
                    }
                    if(info[i].wpa_auth & SECURITY_AUTH_PSK){
                        printf("PSK ");
                    } 
                    printf("}");
                }
   
             if(info[i].wpa_cipher){
                    printf("{");
                    if(info[i].wpa_cipher & ATH_CIPHER_TYPE_WEP){
                        printf("WEP ");
                    }
                    if(info[i].wpa_cipher & ATH_CIPHER_TYPE_TKIP){
                        printf("TKIP ");
                    }
                    if(info[i].wpa_cipher & ATH_CIPHER_TYPE_CCMP){
                        printf("AES ");
                    }
                    printf("}");
                }
                printf("\n");
            }
            if(info[i].rsn_cipher == 0 && info[i].wpa_cipher == 0){
                printf("WEP \n");
            }
        }else{
            printf("NONE! \n");
        }
		if (i != (count-1))
		printf("\n");
#endif        
    } 
	printf("shell> ");
    }

    qcom_set_ssid(currentDeviceId, saved_ssid);
    return 0;
}

#ifdef ENABLE_JSON
A_INT32 swat_json_decode(A_INT32 argc, A_CHAR *argv[])
{
    if(argc != 3)
    {
		SWAT_PTF("Incomplete params\n");
		return A_ERROR;
    }

    jsonPtr = json_decode(argv[2]);
    if (!jsonPtr)
    {
		SWAT_PTF("Error before: [%s]\n",json_get_error_ptr());
		return A_ERROR;	
    }

    return A_OK;
}

A_INT32 json_strcasecmp(const A_CHAR *s1,const A_CHAR *s2)
{
	 if (!s1) 
	 {
		 return (s1==s2)?0:1;
	 }
	 if (!s2) 
	 {
		 return 1;
	 }
	 for(; tolower((A_UCHAR)*s1) == tolower((A_UCHAR)*s2); ++s1, ++s2)	 
	 {
		 if(*s1 == 0)	 
		 {
			 return 0;
		 }
	 }
	 return tolower(*(const A_UCHAR *)s1) - tolower(*(const A_UCHAR *)s2);
}

json_t* parsejobject(json_t* t,char * name)
{
	 json_t *temp = NULL,*temp2 = NULL;
	 if(t->type == json_Object	|| t->type ==json_Array)
	 {
		 for(temp = t->child;temp;)
		 {
			 if(!json_strcasecmp(temp->string,name))
			 {
				 return temp;
			 }
			 else if(t->type == json_Object || t->type == json_Array)
			 {
				 temp2 = parsejobject(temp,name);
				 if(temp2)
					 return temp2;
				 temp = temp->next; 
			 }
			 else
				 temp = temp->next; 				 
		 }
	 }
	 return temp;
}

A_INT32 swat_json_query(A_INT32 argc,char *argv[])
{
	json_t *qjson = NULL;
	char * out = NULL;
	if(argc != 3)
	{
		SWAT_PTF("Incomplete params\n");
	    return A_ERROR;
	}
	
	if(!jsonPtr)
	{
		SWAT_PTF("Error! JSON object is freed create new json opbject using --jsondecode\n");
		return A_ERROR;
	}
	else
	{
		if(!json_strcasecmp(jsonPtr->string,argv[2]))
		{
			out = json_encode(jsonPtr);
			SWAT_PTF("Json query string: %s",out);
			return A_OK;
		}
		else
		{
			if(jsonPtr->type == json_Object || jsonPtr->type == json_Array)
			{
				qjson = parsejobject(jsonPtr,argv[2]);
				if(!qjson)
				{
					SWAT_PTF("Query string not found");
					SWAT_PTF("Error before: [%s]\n",json_get_error_ptr());
				}
				out = json_encode(qjson);
				SWAT_PTF("Json query string: %s",out);
			}
		}
	}

	return A_OK;
}

A_INT32 swat_json_free(A_INT32 argc,char *argv[])
{
	if(argc < 2)
	{
	    SWAT_PTF("Incomplete params\n");
	    return A_ERROR;
	}

	if(jsonPtr)
	{
	  json_delete(jsonPtr);
	}

	//Reset the jsonPtr
	jsonPtr = NULL;

	return A_OK;
}
#endif //#ifdef ENABLE_JSON

#if defined (HW_CRYPTO5_ENABLED)

void dset_create_test(int dset_id, A_UINT32 length, A_BOOL random)
{
	DSET_HANDLE handle = 0;
	A_UINT8 *buffer = NULL;
	A_STATUS status;
	int i;

	A_PRINTF("Dset ID %d (Length %d): Create ", dset_id, length);
	
	buffer = qcom_mem_alloc(length + 1);
	if (!buffer)
	{
		A_PRINTF("[FAIL] with no memory\n");
		return;
	}

	if (random)
	{
		// Init random data
		qcom_crypto_rng_get(buffer, length);
	}
	else
	{
		// Init data
		for (i = 0; i < length; i++)
		{
			buffer[i] = (i % 10) + '0';
		}
		buffer[length] = '\0';
	}

	status = qcom_dset_create(&handle, dset_id, length, DSET_MEDIA_NVRAM | DSET_FLAG_SECURE, NULL, NULL);
	if (status != A_OK)
	{
		A_PRINTF("[FAIL]\n");
		goto failed;
	}
	status = qcom_dset_write(handle, (uint8_t*)buffer, length, 0, 0, NULL, NULL);
	if (status != A_OK)
	{
		A_PRINTF("[FAIL] with write\n");
		goto failed;
	}
	
	status = qcom_dset_commit(handle, NULL, NULL);
	if (status != A_OK)
	{ 
		A_PRINTF("[FAIL] with Commit\n");
		goto failed;
	}
	else
	{
		A_PRINTF("[SUCCESS]\n");
	}

	A_PRINTF("Dset Message (Length %d)\n", length);

	if (random)
	{
		//////////////////////////////
		// Print data
		for (i = 0; i < length; i++)
		{
			A_PRINTF("%02x", buffer[i]);
			if ((i & 1023) == 1023)
			{
	        	tx_thread_sleep(120);
			}
		}
	}
	else
	{
		A_UINT8 *temp_buf = buffer;
		if (length > 1024)
		{
			int size;
			
			for (size = length; size > 1024; size -= 1024)
			{
				A_UINT8 temp = temp_buf[1024];
				temp_buf[1024] = '\0';
				A_PRINTF("%s", temp_buf);
				temp_buf += 1024;
				temp_buf[0] = temp;
	        	tx_thread_sleep(120);
			}
		}
		
		A_PRINTF("%s\n", temp_buf);
	}

failed:

	if (buffer)
	{
		qcom_mem_free(buffer);
	}

	if (handle)
	{
		qcom_dset_close(handle, NULL, NULL);
	}
	
	return;
}

void dset_open_test(int dset_id, A_BOOL random)
{
	DSET_HANDLE handle;
	A_UINT8 *buffer = NULL;
	int length = 0;
	int i;
	
	A_PRINTF("Dset ID %d: Open ", dset_id);
	
	if (qcom_dset_open(&handle, dset_id, DSET_MEDIA_NVRAM | DSET_FLAG_SECURE, NULL, NULL) != A_OK)
	{
		A_PRINTF("[FAIL]\n");
		return;
	}
	
	length = qcom_dset_size(handle);
	A_PRINTF("(Length %d) ", length);
	
	buffer = qcom_mem_alloc(length + 1);
	if (!buffer)
	{
		A_PRINTF("[FAIL] with no memory\n");
		goto failed;
	}
	
	if (qcom_dset_read(handle, buffer, length, 0, NULL, NULL) != A_OK)
	{
		A_PRINTF("[FAIL] with read\n");
		goto failed;
	}

	if (!random)
	{
		buffer[length] = '\0';

		for (i = 0; i < length; i++)
		{
			if (buffer[i] != (i % 10) + '0')
			{
				break;
			}
		}

		if (i != length)
		{
			A_PRINTF("[FAIL] with unexpected data\n");
		}
		else
		{
			A_PRINTF("[SUCCESS]\n");
		}
	}
	else
	{
		//////////////////////////////
		// Print data
		A_PRINTF("[UNKNOWN]\nDset Message (Length %d)\n", length);
		for (i = 0; i < length; i++)
		{
			A_PRINTF("%02x", buffer[i]);
			if ((i & 1023) == 1023)
			{
	        	tx_thread_sleep(120);
			}
		}
	}

failed:

	if (buffer)
	{
		qcom_mem_free(buffer);
	}

	if (handle)
	{
		qcom_dset_close(handle, NULL, NULL);
	}

	return;
}

void dset_delete_test(int dset_id)
{
	A_STATUS status;
	
	status = qcom_dset_delete(dset_id, DSET_MEDIA_NVRAM, NULL, NULL);
	
	A_PRINTF("Dset ID %d: Delete [%s]\n", dset_id, status == A_OK ? "SUCCESS" : "FAIL");

	return;
}

A_INT32
swat_wmiconfig_crypto_handle(A_INT32 argc, A_CHAR * argv[])
{
    int sessionNum,op1, op2;
	
    if (!A_STRCMP(argv[1],"--securedsettest"))
    {
    	int dset_id = 0;
		A_BOOL random = FALSE;
		
    	if (argc < 4)
		{
			goto failed;
		}

		dset_id = atoi(argv[3]);

		if (dset_id < DSETID_VENDOR_START)
		{
			SWAT_PTF("dset id is invalid");
			
			return SWAT_OK;
		}

		if (!A_STRCMP(argv[2],"create"))
		{
			A_UINT32 length = 0;
			
			if (argc < 5)
			{
				goto failed;
			}
			
        	length = atoi(argv[4]);
			if ((argc > 5) && atoi(argv[5]))
			{
				if (length > 1024)
				{
					SWAT_PTF("length SHOULD be <= 1024 for random mode.\n");
					return SWAT_OK;
				}
				random = TRUE;
			}

			dset_create_test(dset_id, length, random);
		}
		else if (!A_STRCMP(argv[2],"open"))
		{
			if ((argc > 4) && atoi(argv[4]))
			{
				random = TRUE;
			}
			dset_open_test(dset_id, random);
		}
		else if (!A_STRCMP(argv[2],"delete"))
		{
			dset_delete_test(dset_id);
		}
		else
		{
			goto failed;
		}
		
		return SWAT_OK;
		
	failed:

		SWAT_PTF("wmiconfig --securedsettest <create> <dset_id> <length> [<random>]\n");
		SWAT_PTF("wmiconfig --securedsettest <open/delete> <dset_id> [<random>]\n");
		
		return SWAT_OK;	
    }
    
#ifdef SWAT_CRYPTO
    if(!A_STRCMP(argv[1],"--cryptotest"))
    {
        return swat_crypto_test(argc,argv);
    }
#endif

#if defined (HW_CRYPTO5_ENABLED)
    if (!A_STRCMP(argv[1], "--crypto")) {
        sessionNum = atoi(argv[2]);
        if (sessionNum == 0) {
           hw_crypto_handler(sessionNum, 0, 0, 0, 0, 0, 0);   
        }else if (sessionNum == 1 || sessionNum == 3){
           op1 = atoi(argv[3]);
           hw_crypto_handler(sessionNum, op1, argv[4], argv[5], 0, 0, 0); 
        } else if (sessionNum == 2) {
           op1 = atoi(argv[3]);
           op2 = atoi(argv[6]);           
           hw_crypto_handler(sessionNum, op1, argv[4], argv[5], op2, argv[7], argv[8]);             
        }else if (sessionNum >3 && sessionNum<17) {  
           hw_crypto_handler(sessionNum, 0, 0, 0,  0, 0, 0);             
        }
        return SWAT_OK;
    }
#endif
    return SWAT_NOFOUND;
}    
#endif

A_INT32
swat_wmiconfig_handle(A_INT32 argc, A_CHAR * argv[])
{
    A_INT32 ret = 0;

    if (argc == 1) {
        swat_wmiconfig_information(currentDeviceId);
        return 0;
    }

    ret += swat_wmiconfig_connect_handle(argc, argv);

#if defined(SWAT_WMICONFIG_SOFTAP)
	ret += swat_wmiconfig_softAp_handle(argc,argv);
#endif
#if defined(SWAT_WMICONFIG_WEP)
	ret += swat_wmiconfig_wep_handle(argc,argv);
#endif
#if defined(SWAT_WMICONFIG_WPA)
    ret += swat_wmiconfig_wpa_handle(argc, argv);
#endif
#if defined(SWAT_WMICONFIG_WPS)
     ret += swat_wmiconfig_wps_handle(argc, argv);
#endif
#if defined(SWAT_WMICONFIG_P2P)
     ret += swat_wmiconfig_p2p_handle(argc, argv);
#endif
#if defined(SWAT_WMICONFIG_IP)
    ret += swat_wmiconfig_ip_handle(argc, argv);
#endif
#if defined(SWAT_WMICONFIG_MISC)
    ret += swat_wmiconfig_misc_handle(argc, argv);
#endif
#if defined(SWAT_WMICONFIG_MISC_EXT)
    ret += swat_wmiconfig_misc_ext_handle(argc, argv);
#endif
#if defined(HW_CRYPTO5_ENABLED) || defined(SWAT_CRYPTO)
    ret += swat_wmiconfig_crypto_handle(argc, argv);
#endif
#if defined(WLAN_BTCOEX_ENABLED)
    ret += swat_wmiconfig_btcoex_handle(argc, argv);
#endif
#if defined(SWAT_WMICONFIG_NOISE_FLOOR_CAL)
    ret += swat_wmiconfig_nl_cal_handle(argc,argv);

#endif

#if defined(ENABLE_AWS)
    extern int swat_wmiconfig_aws_handle(A_INT32 argc, A_CHAR * argv[]);
    ret += swat_wmiconfig_aws_handle(argc,argv);
#endif

#if defined(SWAT_COAP)
    extern int swat_wmiconfig_coap_handle(A_INT32 argc, A_CHAR * argv[]);
    ret += swat_wmiconfig_coap_handle(argc , argv);
#endif

    
    if (ret == 0) {
        SWAT_PTF("Unknown wmiconfig command!\n");
    }else if (ret == SWAT_ERROR){
        SWAT_PTF("Unknown/Invalid command\n");
    }

}


A_INT32
swat_wmiconfig_connect_handle(A_INT32 argc, A_CHAR * argv[])
{
    if (!A_STRCMP(argv[1], "--connect")) {
        A_UINT32 wifiMode;
        if (argc == 3) {
            swat_wmiconfig_connect_ssid(currentDeviceId, (char *)argv[2]);
        } else {
            A_INT8 ssid[32];
            A_UINT8 i, j;

            j = 0;
            A_MEMSET(ssid, 0, 32);
            for (i=2; i<argc; i++)
            {
                if ((j + strlen(argv[i])) > 32)
                {
                    SWAT_PTF("ERROR: ssid > 32\n");
                    return SWAT_ERROR;
                }
                else
                {
                    do
                    {
                        ssid[j] = *argv[i];
                        j++;
                    } while (*argv[i]++);
                }

				ssid[j-1] = 0x20; //space
            }
			ssid[j-1] = 0;

            swat_wmiconfig_connect_ssid(currentDeviceId, (char *) ssid);
        }

		qcom_op_get_mode(currentDeviceId, &wifiMode);

		if (currentDeviceId == 0 && wifiMode == 1 && bridge_mode == 0) // AP mode
		{
			// Set default IP address
        	swat_wmiconfig_ipstatic(currentDeviceId, "192.168.1.10", "255.255.255.0", "192.168.1.10");
			// Set default dhcp pool
        	swat_wmiconfig_dhcp_pool(currentDeviceId, "192.168.1.100", "192.168.1.200", 0xFFFFFFFF);
		}

        return SWAT_OK;
    }

	if (!A_STRCMP(argv[1], "--connect2"))
		{
		swat_wmiconfig_connect_ssid_2(currentDeviceId, (char *)argv[2]);
		return SWAT_OK;
		}

	if (!A_STRCMP(argv[1], "--adhoc"))
		{

        SWAT_PTF("Disabling Power Save\n");

		int other_devid=0;
		if(currentDeviceId==0) other_devid=1;
		if(currentDeviceId==1) other_devid=0;
		if(gDeviceContextPtr[other_devid]->chipMode == QCOM_WLAN_DEV_MODE_ADHOC)
		{
			SWAT_PTF("Don't support adhoc mode on both device 0 and 1, wmiconfig --mode to change\n");
			return SWAT_OK;
		}
		qcom_power_set_mode(currentDeviceId, MAX_PERF_POWER);

		swat_wmiconfig_connect_adhoc(currentDeviceId, (char *)argv[2]);

		return SWAT_OK;
		}

    if (!A_STRCMP(argv[1], "--disc")) {
        swat_wmiconfig_connect_disc(currentDeviceId);
        return SWAT_OK;

    }

	 if (!A_STRCMP(argv[1], "--stacountry")) {
	 	A_CHAR country_code[3];
		if (3 == argc){
			country_code[0] = argv[2][0];
            country_code[1] = argv[2][1];
            country_code[2] = 0x20;

            /* Removed country code QAPI. However, retain this application
             * API so as to not break CST scripts/tests
             */
            swat_wmiconfig_sta_country_code_set(currentDeviceId, country_code);
		}else{
            SWAT_PTF("Invalid paramter format\n");
            return SWAT_ERROR;
        }
        return SWAT_OK;

    }
	 
    return SWAT_NOFOUND;
}

A_INT32
swat_wmiconfig_misc_ext_handle(A_INT32 argc, A_CHAR * argv[])
{
    A_INT32 data = 0;
    A_INT32 data0 = 0;
    A_INT32 data1 = 0;

#if 0
    if (!A_STRCMP(argv[1], "--pwm")) {
        if (argc < 3) {
			SWAT_PTF("pwm parameters should be start or stop\n");
            return SWAT_ERROR;
        }
		if(!A_STRCMP(argv[2], "start")){
			data = 1;
		}
		else if(!A_STRCMP(argv[2], "stop")){
			data = 0;
		}
		else {
			SWAT_PTF("pwm parameters should be start or stop\n");
			return SWAT_ERROR;
		}
		swat_wmiconfig_pwm_start(data);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--selectpin")) {
        if (argc < 3) {
            SWAT_PTF("wmiconfig --selectpin pin_value\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		SWAT_PTF("pwm: select GPIO PIN %d as output\n", data);
		swat_wmiconfig_pwm_select(data);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--deselectpin")) {
        if (argc < 3) {
			SWAT_PTF("wmiconfig --deselectpin pin_value\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		SWAT_PTF("pwm: deselect GPIO PIN %d as output\n", data);
		swat_wmiconfig_pwm_deselect(data);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--pwmclock")) {
        if (argc < 3) {
			SWAT_PTF("wmiconfig --pwmclock divider\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		SWAT_PTF("pwm: configure pwm frequency division as %d\n", data);
		swat_wmiconfig_pwm_clock(data);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--dutycycle")) {
        if (argc < 3) {
			SWAT_PTF("wmiconfig --dutycycle duty_cycle\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		SWAT_PTF("pwm: configure pwm duty cycle as %d\n", data);
		swat_wmiconfig_pwm_duty(data);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--pwmset")) {
        if (argc < 4) {
			SWAT_PTF("wmiconfig --pwmset freq duty_cycle\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		data1 = atoi(argv[3]);
		SWAT_PTF("pwm: configure pwm freq as %d, and duty cycle as %d\n", data, data1);
		swat_wmiconfig_pwm_set(data, data1);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--pwmdump")) {
		swat_wmiconfig_pwm_dump();
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--pwmswinit")) {
		swat_wmiconfig_pwm_sw_init();
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--pwmswcreate")) {
        if (argc < 5) {
			SWAT_PTF("wmiconfig --pwmswcreate pin freq duty_cycle\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		data1 = atoi(argv[3]);
		data0 = atoi(argv[4]);
		A_UINT32 retVal = 0;
		retVal = swat_wmiconfig_pwm_sw_create(data, data1, data0);
		SWAT_PTF("pwm: sw pwm create with pin %d, freq %d, duty cycle %d, and id %d is returned\n", data, data1, data0, retVal);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--pwmswdelete")) {
        if (argc < 3) {
			SWAT_PTF("wmiconfig --pwmswdelete pwm_sw_id\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		SWAT_PTF("pwm: sw pwm delete with id as %d\n", data);
		swat_wmiconfig_pwm_sw_delete(data);
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--pwmswconfig")) {
        if (argc < 6) {
			SWAT_PTF("wmiconfig --pwmswconfig id pin freq duty_cycle\n");
            return SWAT_ERROR;
        }
		data = atoi(argv[2]);
		data1 = atoi(argv[3]);
		data0 = atoi(argv[4]);
		A_UINT32 data2=atoi(argv[5]);
		swat_wmiconfig_pwm_sw_config(data, data1, data0, data2);
		SWAT_PTF("pwm: sw pwm config with id %d, pin %d, freq %d, duty cycle %d\n", data, data1, data0, data2);
        return SWAT_OK;
    }
#endif
    if (!A_STRCMP(argv[1], "--raw")) {
        A_UINT8 i;
        int aidata[6];
        int rt;
        A_UINT8 addr[4][6];

	// demo data configuration for probe request
	A_UINT8 probe_req_str[46]={ 
	                             /*SSID */
		                     0x00,0x00,
				     /*supported rates*/
				     0x01, 0x08, 0x82, 0x84 ,0x8B, 0x0C, 0x2, 0x96, 0x18, 0x24,
				     /*extended supported Rates*/
				     0x32 ,0xC8, 0x5C ,0x9B ,0x31, 0xB6, 0x16, 0x0D ,0xFC, 0xB2, 
				     0xC0, 0x8B, 0x06, 0x00, 0x00, 0x00, 0x00, 0x00 ,0x00 ,0x98 ,
				     0x0D ,0x00 ,0x00 ,0x00 ,0x00 ,0x00 ,0xC0 ,0x06 ,0x00 ,0x88 ,0x01,0x66,0x15,0x0B 
				  };


        QCOM_RAW_MODE_PARAM_t para;

        if (argc < 7) {
            goto raw_usage;
        }
        else if(argc < 12){
            ;
        }
        else
        {
            goto raw_usage;
        }

        data = atoi(argv[2]);
        data0 = atoi(argv[3]);
        data1 = atoi(argv[4]);
        A_UINT32 data2=atoi(argv[5]);
        A_UINT32 data3=atoi(argv[6]);
        for (i=0; i<4; i++)
            memset(&addr[i][0], 0, sizeof(addr[i]));

        if (argc > 7){
            for (i=0; i<(argc-7); i++){
                memset(&aidata[0], 0, sizeof(aidata));
                rt = sscanf(argv[7+i], "%2X:%2X:%2X:%2X:%2X:%2X", \
                            &aidata[0], &aidata[1], &aidata[2], \
                            &aidata[3], &aidata[4], &aidata[5]);
                if (rt < 0)
                {
                    SWAT_PTF("wrong mac format.\n");
                    return SWAT_ERROR;
                }
                for (rt = 0; rt < 6; rt++)
                {
                    addr[i][rt] = (A_UINT8)aidata[rt];
                }
            }
        }

        if (argc == 7)
        {
            addr[0][0] = 0xff;
            addr[0][1] = 0xff;
            addr[0][2] = 0xff;
            addr[0][3] = 0xff;
            addr[0][4] = 0xff;
            addr[0][5] = 0xff;
            addr[1][0] = 0x00;
            addr[1][1] = 0x03;
            addr[1][2] = 0x7f;
            addr[1][3] = 0xdd;
            addr[1][4] = 0xdd;
            addr[1][5] = 0xdd;
            addr[2][0] = 0x00;
            addr[2][1] = 0x03;
            addr[2][2] = 0x7f;
            addr[2][3] = 0xdd;
            addr[2][4] = 0xdd;
            addr[2][5] = 0xdd;
            addr[3][0] = 0x00;
            addr[3][1] = 0x03;
            addr[3][2] = 0x7f;
            addr[3][3] = 0xee;
            addr[3][4] = 0xee;
            addr[3][5] = 0xee;

            if (data3 == 1)  //these codes are not right, add here only for remain the old design
            {
            	memcpy(addr[0], addr[1], ATH_MAC_LEN);
            	addr[2][3] = 0xaa;
            	addr[2][4] = 0xaa;
            	addr[2][5] = 0xaa;
            }
        }

        para.rate_index = data;
        para.tries = data0;
        para.size = data1;
        para.chan = data2;
        para.header_type = data3;
        para.seq = 0;
        memcpy(&para.addr1.addr[0], addr[0], ATH_MAC_LEN);
        memcpy(&para.addr2.addr[0], addr[1], ATH_MAC_LEN);
        memcpy(&para.addr3.addr[0], addr[2], ATH_MAC_LEN);
        memcpy(&para.addr4.addr[0], addr[3], ATH_MAC_LEN);

	if(para.header_type == 3){
              para.buflen = sizeof(probe_req_str);
              para.pdatabuf = probe_req_str;
	}
	else 
	{
              para.pdatabuf = NULL;
	      para.buflen = 0; 
	}
		
        if (!qcom_raw_mode_send_pkt(&para))
            return SWAT_OK;

        raw_usage:
        {
            SWAT_PTF("raw input error\n");
            SWAT_PTF("usage : wmiconfig --raw rate num_tries num_bytes channel header_type [addr1 [addr2 [addr3 [addr4]]]]\n");
            SWAT_PTF("rate = rate index where 0==1mbps; 1==2mbps; 2==5.5mbps etc\n");
            SWAT_PTF("num_tries = number of transmits 1 - 14\n");
            SWAT_PTF("num_bytes = payload size 0 to 1400\n");
            SWAT_PTF("channel = 0 -- 11, 0: send on current channel\n");
            SWAT_PTF("header_type = 0==beacon frame; 1==QOS data frame; 2==4 address data frame; 3==probe request\n");
            SWAT_PTF("addr1/2/3/4 = address 1/2/3/4 of 802.11 header xx:xx:xx:xx:xx:xx\n");
            return SWAT_ERROR;
        }
    }
    if (!A_STRCMP(argv[1], "--wdt")) {
        if(argc < 4){
            SWAT_PTF("Invalid params\n");
            return SWAT_ERROR;
        }
        data = atoi(argv[2]);
        data0 = atoi(argv[3]);

        qcom_watchdog(data,data0);
        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--wdttest")) {
         if(argc < 3){
            SWAT_PTF("Invalid params\n");
            return SWAT_ERROR;
        }       
        data = atoi(argv[2]);
        int curTime, startTime;
        startTime = curTime = swat_time_get_ms();

        /*block for specified time*/
        while(curTime < startTime + data*1000){
            curTime = swat_time_get_ms();
        }
        return SWAT_OK;
    }
    
#ifdef GPIO_TEST
        if (!A_STRCMP(argv[1], "--gpiotest")) {
            A_UINT32    peripheral_id = 0;
            A_UINT32    peripheral_disable = 0;
            A_INT32    retVal = 0;
            A_UINT32    cmd_category;
            A_UINT32 pmap = 0;
    
            cmd_category = swat_atoi(argv[2]);
    
            if (cmd_category == 0)
            {
                peripheral_id = swat_atoi(argv[3]);
                peripheral_disable = swat_atoi(argv[4]);
    
                retVal = qcom_gpio_apply_peripheral_configuration(peripheral_id, 
                                                            peripheral_disable);
               
                if (A_OK != retVal)
                {
                    qcom_gpio_peripheral_pin_conflict_check(peripheral_id, &pmap); 
                }
                SWAT_PTF("GPIO Test: PER ID %d, Disable %d, RETVAL %d, pmap %x\n", 
                                    peripheral_id, peripheral_disable, retVal, pmap);
            }
            else if (cmd_category == 1)
            {
                A_UINT32    gpio_pin;
    
                gpio_pin = swat_atoi(argv[3]);
                peripheral_disable = swat_atoi(argv[4]);
    
                peripheral_id = QCOM_PERIPHERAL_ID_GPIOn(gpio_pin);
    
                retVal = qcom_gpio_apply_peripheral_configuration(peripheral_id, 
                                                            peripheral_disable);
                if (A_OK != retVal)
                {
                    qcom_gpio_peripheral_pin_conflict_check(peripheral_id, &pmap); 
                }
            
                SWAT_PTF("GPIO Test: PER ID %d, Pin %d, Disable %d, RETVAL %d, pmap %x\n", peripheral_id, 
                                                        gpio_pin, peripheral_disable, retVal, pmap);
            }
            else if (cmd_category == 2)
            {
                A_UINT32 sw_pin = swat_atoi(argv[3]);
                A_UINT32 pin_num;
    
                retVal = qcom_gpio_get_interrupt_pin_num(sw_pin, &pin_num);
    
                SWAT_PTF("GPIO Test: SW PIN %d, PIN Num %d, retval %d\n", sw_pin, pin_num, retVal);
            }
            else if (cmd_category == 3)
            {
                A_UINT32 sw_pin = swat_atoi(argv[3]);
                A_UINT32 update_param = swat_atoi(argv[4]);
                A_UINT32 pin_num;
    
                retVal = qcom_gpio_get_interrupt_pin_num(sw_pin, &pin_num);
    
                if (retVal == -1)
                {
                    SWAT_PTF("SW_INT %d to pin failed %d\n", sw_pin, pin_num);
                    return SWAT_OK;
                }
    
                if (update_param == 1)
                {
                    A_UINT32 pull_type = swat_atoi(argv[5]);
                    A_UINT32 strength =  swat_atoi(argv[6]);
                    A_UINT32 open_drain =  swat_atoi(argv[7]);
                    qcom_gpio_pin_pad(pin_num, pull_type, strength, open_drain); 
                    
                    SWAT_PTF("GPIO Test: SW PIN %d, PIN Num %d, retVal %d\n", sw_pin, pin_num, retVal);
                    SWAT_PTF("GPIO Test: pull type %d, strength %d, open_drain %d\n", pull_type, strength, open_drain);
                }
                else if (update_param == 2)
                {
                    A_UINT32 in_out =  swat_atoi(argv[5]);
                    qcom_gpio_pin_dir(pin_num, in_out); 
                    SWAT_PTF("GPIO Test: SW PIN %d, PIN Num %d, inout %d\n", sw_pin, pin_num, in_out);
                }
                else if (update_param == 3)
                {
                    A_UINT32 source =  swat_atoi(argv[5]);
                    qcom_gpio_pin_source(pin_num, source); 
                    SWAT_PTF("GPIO Test: SW PIN %d, PIN Num %d, source %d\n", sw_pin, pin_num, source);
                }
                
             
            }
            else if (cmd_category == 4)
            {
                A_UINT32 sw_pin = swat_atoi(argv[3]);
                A_UINT32 update_param = swat_atoi(argv[4]);
                A_UINT32 pin_num;
    
                retVal = qcom_gpio_get_interrupt_pin_num(sw_pin, &pin_num);
    
                if (retVal == -1)
                {
                    SWAT_PTF("SW_INT %d to pin failed %d\n", sw_pin, pin_num);
                    return SWAT_OK;
                }
    
                qcom_gpio_interrupt_info_t  gpio_interrupt;
    
                gpio_interrupt.pin = pin_num;
                gpio_interrupt.gpio_pin_int_handler_fn = gpio_pin_int_handler_test;
                gpio_interrupt.arg = NULL;
    
                if (update_param == 1)
                {
                    qcom_gpio_interrupt_register(&gpio_interrupt);
                }
                else if (update_param == 2)
                {
                    qcom_gpio_interrupt_deregister(&gpio_interrupt);
                }
            }
            else if(cmd_category == 5)
            {
                A_UINT32 pin_num = swat_atoi(argv[3]);
                A_UINT32 regVal = 0, fn_ptr = 0;
                qcom_gpio_interrupt_info_t  *gpio_interrupt;
    
                typedef struct {
                    gpio_interrupt_info_t  *gpio_info_list[GPIO_PIN_COUNT];
                } gpio_int_dispatcher_t;
                
                extern gpio_int_dispatcher_t *gpio_int_dispatcher;
    
                regVal = *(A_UINT32 *)(0x14028 + (4 * pin_num));
                gpio_interrupt = (qcom_gpio_interrupt_info_t *)gpio_int_dispatcher->gpio_info_list[pin_num];
    
                if (gpio_interrupt != NULL)
                    fn_ptr = (A_UINT32)(gpio_interrupt->gpio_pin_int_handler_fn);
    
                SWAT_PTF("Pin %d, regVal %x, fnptr %d\n", pin_num, regVal, fn_ptr);
            }
            return SWAT_OK;
        }
#endif /* GPIO_TEST */

#ifdef ENABLE_JSON
    if(!A_STRCMP(argv[1],"--jsondecode"))
    {
        swat_json_decode(argc, argv);
        return SWAT_OK;
    }
	
    if(!A_STRCMP(argv[1],"--jsonquery"))
    {
        swat_json_query(argc,argv);
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1],"--jsonfree"))
    {
        swat_json_free(argc,argv);
        return SWAT_OK;
    }
#endif //#ifdef ENABLE_JSON	

    if(!A_STRCMP(argv[1],"--uartsync"))
    {
        extern A_BOOL Console_serial_restored;
        if (argc < 3) {
            //default to enable uartsync
            Console_serial_restored = TRUE;
        }
        if (argc >= 3) {
            Console_serial_restored = !!atoi(argv[2]);
        }
        SWAT_PTF("uartsync: %s\n", (Console_serial_restored ? "enabled" : "disabled"));
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1],"--setmainttimeout"))
    {
        
        A_UINT32 timeout = swat_atoi(argv[2]);

        swat_wmiconfig_maint_timeout_set(&timeout,sizeof(timeout));
        
        return SWAT_OK;
    }
    
    return SWAT_NOFOUND;
}

A_INT32
swat_wmiconfig_nl_cal_handle(A_INT32 argc, A_CHAR * argv[])
{
 /*enable or disable the noise floor cal */


   A_INT32 mode ;

   if (!A_STRCMP(argv[1], "--nf_cal")) {
       if(argc == 3)
         {
            if (!A_STRCMP(argv[2], "start")) 
                mode = 1;
             else if (!A_STRCMP(argv[2], "stop")) 
                mode = 0;
         }
       else 
         SWAT_PTF("nf_cal usage: wmiconfig --nf_cal start|stop");

       qcom_ar6000_set_Nf(mode);

      return SWAT_OK;

    }
   return SWAT_NOFOUND;


}

#ifdef SWAT_QURT
#include "qurt/qurt.h"

const char qurt_test_thread_name[] = "swat_qurt_test_thread";

typedef struct
{
	int thread_index;
	qurt_mutex_t *lock;
	qurt_signal_t *signal;
	qurt_pipe_t pipe;
} swat_qurt_test_t;

void swat_qurt_test_mutex(qurt_mutex_t *lock, int thread_index)
{
	qurt_mutex_lock(lock);
	SWAT_PTF("%s %d get mutex lock\n", qurt_test_thread_name, thread_index);
	SWAT_PTF("%s %d sleep 200 ms\n", qurt_test_thread_name, thread_index);
	qurt_thread_sleep(200);
	SWAT_PTF("%s %d free mutex lock\n", qurt_test_thread_name, thread_index);
	qurt_mutex_unlock(lock);
}

void swat_qurt_test_timer(void *arg)
{
	SWAT_PTF("\nqurt timer expired %ds\n", (int)arg);
}

void swat_qurt_test_thread(void* arg)
{
	int i;
	swat_qurt_test_t *qurt_test = (swat_qurt_test_t *)arg;
	
	SWAT_PTF("%s %d start\n", qurt_test_thread_name, qurt_test->thread_index);

	qurt_pipe_send(qurt_test->pipe, &qurt_test->thread_index);
	
	for (i = 0; i < 8; i++)
	{
		swat_qurt_test_mutex(qurt_test->lock, qurt_test->thread_index);
		qurt_thread_sleep(500);
	}

	SWAT_PTF("%s %d exit\n", qurt_test_thread_name, qurt_test->thread_index);
	
	qurt_signal_set(qurt_test->signal, 1 << (qurt_test->thread_index - 1));
	
    /* Thread Delete */
    qurt_thread_stop();
}
#endif

A_INT32
swat_wmiconfig_misc_handle(A_INT32 argc, A_CHAR * argv[])
{
    A_INT32 data = 0;
    A_INT32 data0 = 0;
    A_INT32 data1 = 0;

    if (!A_STRCMP(argv[1], "--help")) {
        swat_wmiconfig_help();
        return SWAT_OK;
    }

	if (!A_STRCMP(argv[1], "--pwm")) {
		if(argc<3){
			SWAT_PTF("wrong params, go for help\r\n");
			SWAT_PTF("wmiconfig --pwm help\r\n");
			return SWAT_ERROR;
		}
		if((argc==3)&&(!A_STRCMP(argv[2], "help"))){
			SWAT_PTF("pwm <module> <start|stop> <port group> \r\n");
			SWAT_PTF("pwm <module> config <freq> <duty cycle>< phase> <port id> <src clk>\r\n");
			SWAT_PTF("Ex:	(1)start:		--pwm 0 start 0x2\r\n");
			SWAT_PTF("	(2)config:	 \r--pwm 0 config 20000 5000 0 1 0 \r\n");
			SWAT_PTF("Parameters:\r\n");
			SWAT_PTF("1 <module>\r\r: 0 for pwm port module 1 for sdm module(not recommand)\r\n");
			SWAT_PTF("2 <port id>        : 0~7\r\n");
			SWAT_PTF("3 <port group>        : 0x1~0xff every bit represents one channel 0x1 for port_id 0 0x2 for port_id 1 0x3 for port_id 0 and port_id 1...\r\n");
			SWAT_PTF("4 <freq>:\r\r3~132000000(0.03Hz~1.32MHz) for module 0, 0~200 for module 1\r\n");
			SWAT_PTF("5 <<start|stop>    : enable/disable port<port id> pwm output\r\n");
			SWAT_PTF("6 <duty cycle>        : 0~10000 for module 0 0~255 for module 1\r\n");
			SWAT_PTF("7 <phase>        : 0~10000 for module 0\r\n");
			SWAT_PTF("8 <src clk>        : 0: CPU_CLK; 1: REF_CLK; 2: LPO_CLK; other to default value 0\r\n");
			return SWAT_OK;
		}
		A_UINT8 module = atoi(argv[2]);
		if((module>1)||(argc<5)){
			//extern void test_pwm(A_UINT32 duty);
			//test_pwm(module);
			SWAT_PTF("wrong params, go for help\r\n");
			SWAT_PTF("wmiconfig --pwm help\r\n");
			return SWAT_ERROR;
		}
		if ((!A_STRCMP(argv[3], "start"))||(!A_STRCMP(argv[3], "stop"))){
			int port = 0;
			sscanf(argv[4],"%i",&port);
			
			if(port>255){
				SWAT_PTF("wrong params, go for help\r\n");
				SWAT_PTF("wmiconfig --pwm help\r\n");
				return SWAT_ERROR;
			}
			
			A_UINT8 enable;
			if((!A_STRCMP(argv[3], "start")))
				enable=1;
			else
				enable=0;
			swat_wmiconfig_pwm_ctrl(module,enable,port);
		}else if(!A_STRCMP(argv[3], "config")){
			if(argc<9){
				SWAT_PTF("wrong params, go for help\r\n");
				SWAT_PTF("wmiconfig --pwm help\r\n");
				return SWAT_ERROR;
			}
			A_UINT32 freq = atoi(argv[4]);
			A_UINT32 duty_cycle =atoi(argv[5]);
			A_UINT32 phase =atoi(argv[6]);
			A_UINT8 port = atoi(argv[7]);
			//if((port>7)||(module&&((freq>1024)||(duty_cycle>255)))){
			if((port>7)||(module&&((freq>1024)||(duty_cycle>255)))||((!module)&&((freq<3)||(freq>1300000000)||(phase>10000)||(duty_cycle>10000)))){
				SWAT_PTF("wrong params, go for help\r\n");
				SWAT_PTF("wmiconfig --pwm help\r\n");
				return SWAT_ERROR;
			}
			A_UINT8 src_clk;
			if(argc<9)
				src_clk =0;
			else
				src_clk = (atoi(argv[8])>3?0:atoi(argv[8]));
			swat_wmiconfig_pwm_set(module,freq, duty_cycle,phase,port,src_clk);

		}else {
			SWAT_PTF("wrong params, go for help\r\n");
			SWAT_PTF("wmiconfig --pwm help\r\n");
			return SWAT_ERROR;
		}
		return SWAT_OK;
	}
	if (!A_STRCMP(argv[1], "--i2c")) {
		if(argc<3){
			SWAT_PTF("wrong params, go for help\r\n");
			SWAT_PTF("wmiconfig --i2c help\r\n");
			return SWAT_ERROR;
		}
		if((argc==3)&&(!A_STRCMP(argv[2], "help"))){
			SWAT_PTF("i2c master <eeprom|loop> <mode|config*> <read|write|config value*> <data>\r\n");
			SWAT_PTF("i2c slave <start|stop|config*> \r\n");
			SWAT_PTF("Ex:	(1)master:	--i2c master eeprom 0 write 100\r\n");
			SWAT_PTF("		(2)slave:	 	--i2c slave start\r\n");
			SWAT_PTF("Parameters:\r\n");
			SWAT_PTF("1 <mode>    : eeprom 0~2 for device num, loop 0~2 for csr/reg/fifo\r\n");
			SWAT_PTF("2 <data>        : DEC input, 6 bit for eeprom and csr, 4 byte for reg, 8 byte for fifo\r\n");
			SWAT_PTF("3 <config>      : master config 1~5, slave 1~2, 0 for default\r\n");
			SWAT_PTF("default SP240 configuration. Warning: call before read/write and make sure the setting is correct, or maybe cause a crash.\r\n");
			SWAT_PTF("NOTE: i2c master and i2c slave CAN NOT run in a same board\r\n");
			return SWAT_OK;
		}
		if (!A_STRCMP(argv[2], "master")){
			if(argc<6){
				SWAT_PTF("wrong params, go for help\r\n");
				SWAT_PTF("wmiconfig --i2c help\r\n");
				return SWAT_ERROR;
			}
			A_UINT8 mode =atoi(argv[4]);
			if(((A_STRCMP(argv[3], "eeprom"))&&(A_STRCMP(argv[3], "loop")))||((A_STRCMP(argv[5], "read"))&&(A_STRCMP(argv[5], "write")))||(mode>2)){
			    if(A_STRCMP(argv[4], "config")&&(atoi(argv[4])>5)&&(atoi(argv[4])<1)){
				    SWAT_PTF("wrong params, go for help\r\n");
				    SWAT_PTF("wmiconfig --i2c help\r\n");
				    return SWAT_ERROR;
			    }
			}
			if (!A_STRCMP(argv[3], "eeprom")){
				static A_UINT8 config =0;
				if(!A_STRCMP(argv[4], "config")){
					config = atoi(argv[5]);
					if(!config)
						SWAT_PTF("reset configuration\r\n");
					else
					       SWAT_PTF("config to %dth pin\r\n",config);
					return SWAT_OK;
				}
				if(!A_STRCMP(argv[5], "read")){
					A_UINT32 data =0;
					swat_wmiconfig_i2c_eeprom_read(mode,0, (A_UINT8 *)&data,config);
				}else{
					if(argc!=7){
					    SWAT_PTF("wrong params, go for help\r\n");
						SWAT_PTF("wmiconfig --i2c help\r\n");
						return SWAT_ERROR;
					}
					A_UINT32 data = atoi(argv[6]);
					swat_wmiconfig_i2c_eeprom_write(mode,0, (A_UINT8 *)&data,config);
				}
	
			}else{
				static A_UINT8 config =0;
				if(!A_STRCMP(argv[4], "config")){
					config = atoi(argv[5]);
					if(!config)
						SWAT_PTF("reset configuration\r\n");
					else
					    SWAT_PTF("config to %dth pin\r\n",config);
					return SWAT_OK;
				}
				switch(mode){
					case 0:
					if(!A_STRCMP(argv[5], "read")){
						A_UINT32 data =0;
						swat_wmiconfig_i2cm_loop_bread((A_UINT8 *)&data,config);
						data>>=2;
						SWAT_PTF("read data: 0x %2x\n",data);
					}else{
					    if(argc!=7){
							SWAT_PTF("wrong params, go for help\r\n");
							SWAT_PTF("wmiconfig --i2c help\r\n");
							return SWAT_ERROR;
						}
						A_UINT32 data = atoi(argv[6]);
						if(data>0x3f){
			                        SWAT_PTF("csr data: 0~0x3f, go for help\r\n");
			                        SWAT_PTF("wmiconfig --i2c help\r\n");
			                        return SWAT_ERROR;
		                }
						data<<=2;
						data|=0x2;
						swat_wmiconfig_i2cm_loop_bwrite((A_UINT8 *)&data,config);
					}
					break;
					case 1:
					if(!A_STRCMP(argv[5], "read")){
						A_UINT32 data =0;
						swat_wmiconfig_i2cm_loop_rread((A_UINT8 *)&data,config);
						SWAT_PTF("read data: 0x %x\n",data);
					}else{
						if(argc!=7){
							SWAT_PTF("wrong params, go for help\r\n");
							SWAT_PTF("wmiconfig --i2c help\r\n");
							return SWAT_ERROR;
						}
						A_UINT32 data = atoi(argv[6]);
						swat_wmiconfig_i2cm_loop_rwrite((A_UINT8 *)&data,config);
					}
						break;
					case 2:
					if(!A_STRCMP(argv[5], "read")){
						A_UINT32 data[8] ={0};
						swat_wmiconfig_i2cm_loop_fread((A_UINT32 *)data,config);
						SWAT_PTF("read data: 0x %x %x %x %x %x %x %x %x\r\n",data[7],data[6],data[5],data[4],data[3],data[2],data[1],data[0]);
					}else{
						if(argc!=14){
							SWAT_PTF("wrong params, go for help\r\n");
							SWAT_PTF("wmiconfig --i2c help\r\n");
							return SWAT_ERROR;
						}
						A_UINT32 data[8] ={0XFF};
						data[0] = atoi(argv[13]);
						data[1] = atoi(argv[12]);
						data[2] = atoi(argv[11]);
						data[3] = atoi(argv[10]);
						data[4] = atoi(argv[9]);
						data[5] = atoi(argv[8]);
						data[6] = atoi(argv[7]);
						data[7] = atoi(argv[6]);
						swat_wmiconfig_i2cm_loop_fwrite((A_UINT32 *)data,config);
						SWAT_PTF("data: 0x %2x %2x %2x %2x %2x %2x %2x %2x\r\n",data[7],data[6],data[5],data[4],data[3],data[2],data[1],data[0]);
					}
						break;						
				}

			}
		}
		else if ((!A_STRCMP(argv[2], "slave"))&&(argc>3)){
			static A_UINT8 config =0;
			if(!A_STRCMP(argv[3], "config")){
				if(argc==5){
					config = atoi(argv[4]);
					if(config>0&&config<3)
					    SWAT_PTF("config to %dth pin\r\n",config);
					else
						config=0;
					return SWAT_OK;
				}
			}else if (!A_STRCMP(argv[3], "start")){
				swat_wmiconfig_i2cs_install(config);
			}else if (!A_STRCMP(argv[3], "stop")){
				swat_wmiconfig_i2cs_uninstall(config);

			}else{
				SWAT_PTF("wrong params, go for help\r\n");
				SWAT_PTF("wmiconfig --i2c help\r\n");
				return SWAT_ERROR;
			}
				
		}else{
				SWAT_PTF("wrong params, go for help\r\n");
				SWAT_PTF("wmiconfig --i2c help\r\n");
				return SWAT_ERROR;
		}
		return SWAT_OK;
	}
    if (!A_STRCMP(argv[1], "--pmparams")) {
        A_UINT32 i;
        A_UINT8 iflag = 0, npflag = 0, dpflag = 0, txwpflag = 0, ntxwflag = 0, pspflag = 0;
        A_INT32 idlePeriod;
        A_INT32 psPollNum;
        A_INT32 dtimPolicy;
        A_INT32 tx_wakeup_policy;
        A_INT32 num_tx_to_wakeup;
        A_INT32 ps_fail_event_policy;

        if (argc < 3) {
            return SWAT_ERROR;
        }

        for (i = 2; i < argc; i++) {
            if (!A_STRCMP(argv[i], "--idle")) {
                if ((i + 1) == argc) {
                    return SWAT_ERROR;
                }
                A_SSCANF(argv[i + 1], "%u", &idlePeriod);
                iflag = 1;
                i++;
            } else if (!A_STRCMP(argv[i], "--np")) {
                if ((i + 1) == argc) {
                    return SWAT_ERROR;
                }
                A_SSCANF(argv[i + 1], "%d", &psPollNum);
                npflag = 1;
                i++;
            } else if (!A_STRCMP(argv[i], "--dp")) {
                if ((i + 1) == argc) {
                    return SWAT_ERROR;
                }
                A_SSCANF(argv[i + 1], "%d", &dtimPolicy);
                dpflag = 1;
                i++;
            } else if (!A_STRCMP(argv[i], "--txwp")) {
                if ((i + 1) == argc) {
                    return SWAT_ERROR;
                }
                A_SSCANF(argv[i + 1], "%d", &tx_wakeup_policy);
                txwpflag = 1;
                i++;
            } else if (!A_STRCMP(argv[i], "--ntxw")) {
                if ((i + 1) == argc) {
                    return SWAT_ERROR;
                }
                A_SSCANF(argv[i + 1], "%d", &num_tx_to_wakeup);
                ntxwflag = 1;
                i++;
            } else if (!A_STRCMP(argv[i], "--psfp")) {
                if ((i + 1) == argc) {
                    return SWAT_ERROR;
                }
                A_SSCANF(argv[i + 1], "%d", &ps_fail_event_policy);
                pspflag = 1;
                i++;
            } else if ((i % 2) == 0) {
                SWAT_PTF("wmiconfig --pmparams --idle <time in ms>\n");
                SWAT_PTF("                     --np < > \n");
                SWAT_PTF("                     --dp <1=Ignore 2=Normal 3=Stick 4=Auto>\n");
                SWAT_PTF("                     --txwp <1=wake 2=do not wake>\n");
                SWAT_PTF("                     --ntxw < > \n");
                SWAT_PTF("                     --psfp <1=send fail event 2=Ignore event>\n");
                return SWAT_ERROR;
            }
        }

        if (!iflag) {
            idlePeriod = 0;
        }

        if (!npflag) {
            psPollNum = 10;
        }

        if (!dpflag) {
            dtimPolicy = 3;
        }

        if (!txwpflag) {
            tx_wakeup_policy = 2;
        }

        if (!ntxwflag) {
            num_tx_to_wakeup = 1;
        }

        if (!pspflag) {
            ps_fail_event_policy = 2;
        }

        swat_wmiconfig_pmparams(currentDeviceId, idlePeriod, psPollNum, dtimPolicy, tx_wakeup_policy,
                                num_tx_to_wakeup, ps_fail_event_policy);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ebt")) {
#define SWAT_EBT_MAC_FILTER_DISABLE  0
#define SWAT_EBT_MAC_FILTER_ENABLE   1
#define SWAT_EBT_MAC_FILTER_UNCHANGE 2
        A_UINT8 ebt, mac_filter;
        if (argc < 3) {
            /* Get */
            swat_wmiconfig_pm_ebt_mac_params(currentDeviceId, 0, &ebt, &mac_filter);
            SWAT_PTF("ebt: %d, mac_filter: %d\n", ebt, mac_filter);
            return SWAT_OK;
        }
        if (argc != 4)
            return SWAT_ERROR;
        A_SSCANF(argv[2], "%d", &data);
        if (data != SWAT_EBT_MAC_FILTER_DISABLE &&
            data != SWAT_EBT_MAC_FILTER_ENABLE &&
            data != SWAT_EBT_MAC_FILTER_UNCHANGE) {
            return SWAT_ERROR;
        }
        ebt = data;

        A_SSCANF(argv[3], "%d", &data);
        if (data != SWAT_EBT_MAC_FILTER_DISABLE &&
            data != SWAT_EBT_MAC_FILTER_ENABLE &&
            data != SWAT_EBT_MAC_FILTER_UNCHANGE) {
            return SWAT_ERROR;
        }
        mac_filter = data;

        /* Set */
        swat_wmiconfig_pm_ebt_mac_params(currentDeviceId, 1, &ebt, &mac_filter);

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--pwrmode")) {
        if (argc < 3) {
            return SWAT_ERROR;
        }

        A_SSCANF(argv[2], "%d", &data);

        if (data == 0) {
            swat_wmiconfig_power_mode_set(currentDeviceId, 2);   //MAX_PERF_POWER
        } else if (data == 1) {
            A_UINT32 wifiMode;
            qcom_op_get_mode(currentDeviceId, &wifiMode);
            if (wifiMode == 1) {
				A_UINT8 pnum=0;
				A_UINT8 *pmac;
				#define AP_MAX_STA_NUM	10
				pmac = swat_mem_malloc(6*AP_MAX_STA_NUM);
				if(!pmac)
				{
					SWAT_PTF("malloc error\n");
					return SWAT_OK;
				}
				qcom_ap_get_sta_info(currentDeviceId, &pnum, pmac);
				if(pnum>0)
				{
					SWAT_PTF("AP power save mode is not supported, %d STA is connected \n", pnum);
					swat_mem_free(pmac);
					return SWAT_OK;
				}
				swat_mem_free(pmac);
                SWAT_PTF("AP power save mode is not recommended \n");
                //return SWAT_ERROR;
            }

            swat_wmiconfig_power_mode_set(currentDeviceId, 1);   //REC_POWER
        } else {
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1], "--keepawake")){
    	if (argc != 3) {
    		SWAT_PTF("    --keepawake <time in ms 0~8>\n");
            return SWAT_ERROR;
        }
        A_SSCANF(argv[2], "%u", &data);
        if(data > 8)
        {
        	SWAT_PTF("    --keepawake <time in ms 0~8>\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_keep_awake_set(data);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--driver")) {
        if (argc != 3) {
            return SWAT_ERROR;
        }

        if (!A_STRCMP(argv[2], "up")) {
            swat_wmiconfig_reset();
        } else if (!A_STRCMP(argv[2], "down")) {
            ;
        } else {
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--reset")) {
        swat_wmiconfig_reset();
        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--rssi")) {
        swat_wmiconfig_rssi_get(currentDeviceId);
        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--bmiss")) {
        if (argc != 3) {
            return SWAT_ERROR;
        }
        A_SSCANF(argv[2], "%d", &data);
        if (qcom_set_bmiss_time(currentDeviceId, (A_UINT16)data, 0) != A_OK)
            return SWAT_ERROR;
        return SWAT_OK;
    }
    if(!A_STRCMP(argv[1], "--appleie")){
      if (argc == 3){
			if (!swat_strcmp((A_CHAR *) argv[2], "enable")){
				swat_wmiconfig_app_ie(currentDeviceId, 1);
				return SWAT_OK;
			}
			else if (!swat_strcmp((A_CHAR *) argv[2], "disable")){
				swat_wmiconfig_app_ie(currentDeviceId, 0);
				return SWAT_OK;
			}
		}
			
		SWAT_PTF("    --appleie <enable/disable>\n");
        return SWAT_OK;        
    }
    if (!A_STRCMP(argv[1], "--ani")) {
        if (argc != 3) {
            return SWAT_ERROR;
        }
        A_SSCANF(argv[2], "%d", &data);
        qcom_ani_enable((A_BOOL)data);
        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--settxpower")) {
        if (argc != 3) {
            return SWAT_ERROR;
        }

        A_SSCANF(argv[2], "%d", &data);
        swat_wmiconfig_tx_power_set(currentDeviceId, (A_UINT32) data);

        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--lpl")) {
        if (argc != 3) {
            return SWAT_ERROR;
        }

        A_SSCANF(argv[2], "%d", &data);
		//For LPL feature, it is general for both dev 0 and 1.  lpl_init(gdevp).
        swat_wmiconfig_lpl_set(0, (A_UINT32) data);

        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--gtx")) {
        if (argc != 3) {
            return SWAT_ERROR;
        }

        A_SSCANF(argv[2], "%d", &data);
        swat_wmiconfig_gtx_set(currentDeviceId, (A_UINT32) data);

        return SWAT_OK;
    }
    /* --setrate <rate> , where rate can be 1,2,5,6,9,11,12,18,24,36,48,54 mbps or MCS */
    if (!A_STRCMP(argv[1], "--setrate"))
    {

        if(argc==3) {
            A_SSCANF(argv[2], "%d", &data);
            swat_wmiconfig_rate_set(currentDeviceId, 0, (A_UINT32) data);
        }
        if (argc == 4 || argc == 5) {
            if (!A_STRCMP(argv[2], "mcs")) {
                A_UINT32 mcs = 1;
                A_SSCANF(argv[3], "%d", &data);
                if (argc == 5 && !A_STRCMP(argv[4], "ht40"))
                    mcs = 2;
                swat_wmiconfig_rate_set(currentDeviceId, mcs, (A_UINT32) data);
            }
        }

        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--suspend")) {
        swat_wmiconfig_suspenable(currentDeviceId);
        return SWAT_OK;
    }
	
    if (!A_STRCMP(argv[1], "--suspflag")) {
        A_UINT8 flag;
		swat_wmiconfig_suspflag_get(currentDeviceId, &flag);
		if (flag == 1)
		{
			SWAT_PTF("reset from timer - software \n");
		}
		else if (flag == 2)
		{
			SWAT_PTF("reset from GPIO - hardware \n");
		}
		else
		{
			SWAT_PTF("normal reset - no suspend\n");
		}
		
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--suspstart")) {
        A_UINT32 susp_time;
        A_UINT32 wifiMode;

        extern A_BOOL _storerecall_suspend_enabled(void);
        if (!_storerecall_suspend_enabled()) {
            SWAT_PTF("error, should run \"wmiconfig --suspend\" first \n");
            return SWAT_ERROR;
        }

        swat_wmiconfig_devmode_get(currentDeviceId, &wifiMode);
        if (wifiMode == 0) {
            SWAT_PTF("NOT supported \n");
            return SWAT_ERROR;
        }

        if (argc == 3) {
            /* xiny */
             susp_time = swat_atoi(argv[2]);

            swat_wmiconfig_suspstart(susp_time);
        } else {
            SWAT_PTF("    --suspstart <time>\n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--version")) {
        swat_wmiconfig_version();
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--partition_index")) {
        swat_wmiconfig_partition_index();
        return SWAT_OK;
    }
	
    if (!A_STRCMP(argv[1], "--print")) {
        A_UINT8 printEnable;
        if (argc != 3) {
           return SWAT_ERROR;
        }
        printEnable = atoi(argv[2]);
        qcom_enable_print(printEnable);
        //this is a test string. when firmware prints are disabled, this string is not shown on shell
        A_PRINTF("Firmware prints %sabled\n", (printEnable ? "en" : "dis"));
        SWAT_PTF("shell> ");
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--allow_aggr")) {
        if (argc != 4) {
			SWAT_PTF("usage: --allow_aggr <tx_tid_mask> <rx_tid_mask>\n");
            return SWAT_ERROR;
        }

        data0 = atoul(argv[2]);
        data1 = atoul(argv[3]);
        if (data0 > 0xff || data1 > 0xff) {
           SWAT_PTF("Invalid parameters, should no more than 0xFF\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_allow_aggr(currentDeviceId, (A_UINT16) data0, (A_UINT16) data1);
        return SWAT_OK;
    }

	if (!strcmp(argv[1], "--promisc")) {
		if (argc >= 3) {
			A_UINT8 promiscuousEn;
            extern void swat_wmiconfig_procmisc_filter_addr(A_UINT8 enable, A_UINT8 addr[]);
			promiscuousEn = atoi(argv[2]);
			if(promiscuousEn){
                if (argc == 4) {
#ifndef ETH_ALEN
#define ETH_ALEN 6
#endif
                    int aidata[ETH_ALEN], rt;
                    A_UINT8 mac[ETH_ALEN];
                    rt = sscanf(argv[3], "%2X:%2X:%2X:%2X:%2X:%2X", \
                                &aidata[0], &aidata[1], &aidata[2], \
                                &aidata[3], &aidata[4], &aidata[5]);
                    if (rt < 0)
                        return SWAT_ERROR;
                    for (rt = 0; rt < ETH_ALEN; rt++)
                    {
                        mac[rt] = (A_UINT8)aidata[rt];
                    }
                    swat_wmiconfig_procmisc_filter_addr(1, mac);
                }
				swat_wmiconfig_promiscuous_test(TRUE);
			}
			else{
				swat_wmiconfig_promiscuous_test(FALSE);
                swat_wmiconfig_procmisc_filter_addr(0, NULL);
			}
			return SWAT_OK;
		}
		else {
			SWAT_PTF("	  --promisc <value> [<mac addr>]\n");
			return SWAT_ERROR;
		}
	}

    if (!strcmp(argv[1], "--mcastfilter")){
        if (argc == 3) 
        {
    	    if (!A_STRCMP(argv[2], "enable")) 
    	    {
                swat_wmiconfig_mcast_filter_set(1);
            } 
    	    else if (!A_STRCMP(argv[2], "disable")) 
    	    {
                swat_wmiconfig_mcast_filter_set(0);
            } 
    	    else 
    	    {
    	        SWAT_PTF("Invalid parameter\n");;
                return SWAT_ERROR;
            }
    	    return SWAT_OK;
        }
        else {
            SWAT_PTF("Missing parameters!\n");
            return SWAT_ERROR;
        }		
    }
    
	if (!strcmp(argv[1], "--device")) {
	    if (argc == 3) {
                extern A_UINT8 gNumOfWlanDevices;
                if (gNumOfWlanDevices > 1) {
                    //currentDeviceId = atoi(argv[2]);
                    if(atoi(argv[2]) < gNumOfWlanDevices){
							currentDeviceId = atoi(argv[2]);
                        swat_wmiconfig_set_current_devid(currentDeviceId);
                        SWAT_PTF("configure device switch to %d\n", currentDeviceId);
                        return SWAT_OK;
                    }
                    else{
                        SWAT_PTF("Max device id is %d\n", gNumOfWlanDevices-1);
                        return SWAT_ERROR;
                    }
                }
                else {
                    SWAT_PTF("Board is working on single device mode.\n");
                    return SWAT_OK;
                }
	    }
	    else {
	        SWAT_PTF("    --device <value>\n");
	        return SWAT_ERROR;
	    }
	}
	if (!strcmp(argv[1], "--roaming")){
		if (argc == 3){
			if (!swat_strcmp((A_CHAR *) argv[2], "enable")){
				swat_wmiconfig_roaming_enable(currentDeviceId, 1);
				return SWAT_OK;
			}
			else if (!swat_strcmp((A_CHAR *) argv[2], "disable")){
				swat_wmiconfig_roaming_enable(currentDeviceId, 0);
				return SWAT_OK;
			}
		}
			
		SWAT_PTF("    --roaming <enable/disable>\n");
		return SWAT_ERROR;
	}

	if (!strcmp(argv[1], "--unblock")){
    	if (argc == 3){
            int handle=atoi(argv[2]);
            swat_unblock_socket(handle);
            return SWAT_OK;
    	}
    		
    	SWAT_PTF("    --unblock <handle>\n");
    	return SWAT_ERROR;
	}
	if (!strcmp(argv[1], "--settime")){
		if (argc == 9) {
			tRtcTime rtc_time;
			rtc_time.year = atoi(argv[2]);
			rtc_time.mon = atoi(argv[3]);
			rtc_time.yday = atoi(argv[4]);
			rtc_time.wday = atoi(argv[5]);
			rtc_time.hour = atoi(argv[6]);
			rtc_time.min = atoi(argv[7]);
			rtc_time.Sec = atoi(argv[8]);
			qcom_set_time(rtc_time);
			return SWAT_OK;
		}
		else {
			SWAT_PTF("    --settime <year> <month> <day> <wday> <hour> <min> <sec>\n"
					 "             <month>: 0(Jan) ~ 11(Dec)\n"
					 "             <wday> : 0(Sun) ~ 6(Sat)\n");
			return SWAT_ERROR;
		}
	}
	if (!strcmp(argv[1], "--readmem")){
            A_UINT32 val;
            if (argc == 3){
                A_UINT32 addr=strtoul(argv[2], NULL, 0);
                if (addr == 0) {
                    SWAT_PTF("    addr is 0 or malformed\n");
                    return SWAT_ERROR;
                }
                if (addr & 0x3) {
                    SWAT_PTF("    address value must be 4-byte aligned\n");
                    return SWAT_ERROR;
                }
                val = *(volatile A_UINT32 *)addr;
                SWAT_PTF("0x%x / %u\n", val, val);
                return SWAT_OK;
            }
            SWAT_PTF("    --readmem <addr>\n");
            return SWAT_ERROR;
	}
	if (!strcmp(argv[1], "--writemem")){
            A_UINT32 val;
            if (argc == 4){
                A_UINT32 addr=strtoul(argv[2], NULL, 0);
                if (addr == 0) {
                    SWAT_PTF("    addr is 0 or malformed\n");
                    return SWAT_ERROR;
                }
                if (addr & 0x3) {
                    SWAT_PTF("    address value must be 4-byte aligned\n");
                    return SWAT_ERROR;
                }
                val = strtoul(argv[3], NULL, 0);
                *((volatile A_UINT32 *)addr) = val;
                return SWAT_OK;
            }
            SWAT_PTF("    --writemem <addr> <val>\n");
            return SWAT_ERROR;
	}

#ifdef DIVERSITY_ENABLE
	if (!strcmp(argv[1], "--hwantdiv")){
		char *hwantdivHelp="    --hwantdiv { set <div> <adjust> | get }";

		extern void swat_ant_div_set(int, int);
		extern void swat_ant_div_get(void);
		
		if (argc > 2) {
			if(!strcmp(argv[2], "get"))
			{
				swat_ant_div_get();
			}
			else if(!strcmp(argv[2], "set"))
			{
				if(argc == 5)
				{
					int div, adjust;
					div = atoi(argv[3]);
					adjust = atoi(argv[4]);

					swat_ant_div_set(div, adjust);

				}
				else
				{
					SWAT_PTF("%s\n", hwantdivHelp);
				}
			}
			else
			{
				SWAT_PTF("%s\n", hwantdivHelp);
				return SWAT_ERROR;
			}
			return SWAT_OK;
		}
		else {
			SWAT_PTF("%s\n", hwantdivHelp);
			return SWAT_ERROR;
		}
	}
#endif
	if (!strcmp(argv[1], "--mqtt")){
	    int rc = NONE_ERROR;
	    MQTTConnectParams connect_para;
	    MQTTPublishParams pub_params;
	    static MQTTSubscribeParams sub_params;

		if (argc > 2) {
			if(!strcmp(argv[2], "connect")) {
                if (argc < 5) {
                    SWAT_PTF("ERROR: connect para incomplete.\n");
                    return SWAT_ERROR;
                }

                memcpy(&connect_para, &MQTTConnectParamsDefault, sizeof(MQTTConnectParamsDefault));
                connect_para.pHostURL = argv[3];
                connect_para.port = atoi(argv[4]);

                if (argc >= 6) {
#ifdef SWAT_SSL 
                    if(strcmp(argv[5], "ssl_disable")) {
                        connect_para.pDeviceCertLocation = argv[5];
                    } else {
                        connect_para.pDeviceCertLocation = NULL;
                    }
#endif
                }

                if (argc >= 7) {
                    connect_para.pClientID = argv[6];
                    if (sizeof(connect_para.pClientID) > 23) {
                         SWAT_PTF("ERROR: client ID too long.\n");
                         return SWAT_ERROR;
                    }
                }

                if (argc >= 8) {
                    connect_para.KeepAliveInterval_sec = atoi(argv[7]);
                }

                if (argc >= 9) {
                    connect_para.enableAutoReconnect = ((atoi(argv[8])>0) ? 1 : 0);
                }

                if (argc >= 10) {
                    connect_para.isCleansession = ((atoi(argv[9])>0) ? 1 : 0);
                }


                if (argc >= 11) {
                    connect_para.pUserName = argv[10];
                }

                if (argc >= 12) {
                    connect_para.pPassword = argv[11];
                }

                rc = qcom_mqtt_connect(&connect_para);
                if (NONE_ERROR != rc) {
                    SWAT_PTF("ERROR: %d mqtt connect failed.\n", rc);
                    return SWAT_ERROR;
                }
                
				extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
                extern void swat_mqtt_demo_loop(A_UINT32 timeout_ms);

                qcom_task_start(swat_mqtt_demo_loop, 1000, 2048, 10);

                return  SWAT_OK;
			}
			else if(!strcmp(argv[2], "pub")) {
                if (argc < 5) {
                    SWAT_PTF("ERROR: publish para incomplete.\n");
                    return SWAT_ERROR;
                }

                memset(&pub_params, 0, sizeof(pub_params));
                pub_params.pTopic = argv[3];
                pub_params.MessageParams.PayloadLen = strlen(argv[4]);
                pub_params.MessageParams.pPayload = argv[4];

                if (argc >= 6) {
                    pub_params.MessageParams.id = atoi(argv[5]);
                }

                if (argc >= 7) {
                    pub_params.MessageParams.qos = atoi(argv[6]);
                    if ((pub_params.MessageParams.qos > 2) || (pub_params.MessageParams.qos < 0)) {
                         SWAT_PTF("ERROR: invalid qos.\n");
                         return SWAT_ERROR;
                    }
                }

                if (argc >= 8) {
                    pub_params.MessageParams.isRetained = ((atoi(argv[7])>0) ? 1 : 0);
                }

                if (argc >= 9) {
                    pub_params.MessageParams.isDuplicate = ((atoi(argv[8])>0) ? 1 : 0);
                }

                if (NONE_ERROR == qcom_mqtt_publish(&pub_params))
                    return  SWAT_OK;
                else
                    return SWAT_ERROR;
				
			}
			else if (!strcmp(argv[2], "sub")) {
                if (argc < 5) {
                    SWAT_PTF("ERROR: subscribe para incomplete.\n");
                    return SWAT_ERROR;
                }

				/*the filter should be kept while subscribe is running*/
				int topic_len = strlen(argv[3]);
				char *topic = qcom_mem_alloc(topic_len+1);
				memset(topic, 0, topic_len+1);
				memcpy(topic, argv[3], topic_len);

                sub_params.pTopic = topic;
                sub_params.qos= atoi(argv[4]);
                
                if ((sub_params.qos > 2) || (sub_params.qos < 0)) {
                     SWAT_PTF("ERROR: invalid qos.\n");
                     return SWAT_ERROR;
                }

                extern int swat_mqtt_cb_handler(MQTTCallbackParams params);
                sub_params.mHandler = swat_mqtt_cb_handler;

                SWAT_PTF("subscribe topics %s qos=%d\n", sub_params.pTopic, sub_params.qos);
                if (NONE_ERROR == qcom_mqtt_subscribe(&sub_params))
                    return SWAT_OK;
                else
                    return SWAT_ERROR;
            }
            else if(!strcmp(argv[2], "unsub")) {
                if (argc < 4) {
                    SWAT_PTF("ERROR: unsubscribe para incomplete.\n");
                    return SWAT_ERROR;
                }

                if (NONE_ERROR == qcom_mqtt_unsubscribe(argv[3])) {
                    if (sub_params.pTopic) {
                        qcom_mem_free(sub_params.pTopic);
                        sub_params.pTopic= NULL;
                    } 
                    return  SWAT_OK;
                }
                else
                    return SWAT_ERROR;
            }
            else if(!strcmp(argv[2], "disc")) {
                if (sub_params.pTopic) {
                    qcom_mem_free(sub_params.pTopic);
                    sub_params.pTopic= NULL;
                }

                if (NONE_ERROR == qcom_mqtt_disconnect())
                    return  SWAT_OK;
                else
                    return SWAT_ERROR;
            }
			else {
				SWAT_PTF("unknow command.\n");
				return SWAT_ERROR;
			}
		}
		else {
			SWAT_PTF("ERROR: mqtt incomplete parameter.\n");
			return SWAT_ERROR;
		}	
    }
    if (!strcmp(argv[1], "--get_cal2g")) {
        swat_wmiconfig_get_2g_cal();
        return SWAT_OK;	     
    }	

    if (!strcmp(argv[1], "--udp_dbg")) {
        extern int qcom_free_buf_sync_threshold;
        int val=-1;
        if(argc >= 3)
        {
            val = atoi(argv[2]);
            if (val >= 0)
            {
                qcom_free_buf_sync_threshold = val;
            }
        }
        if(val < 0)
            SWAT_PTF("parameter error!\n");
        return SWAT_OK;	     
    }	

#ifdef SWAT_QURT
    if (!strcmp(argv[1], "--qurt_test")) {
		//int res;
		qurt_mutex_t lock = 0;
		qurt_signal_t signal = 0;
		qurt_pipe_t pipe = NULL;
		#define QURT_TEST_THREADS (4)
		swat_qurt_test_t test[QURT_TEST_THREADS];
		int mask = 0;
		if (QURT_EOK != qurt_mutex_create(&lock))
		{
			SWAT_PTF("qurt_mutex_create FAIL!\n");
			goto error;
		}
		if (QURT_EOK != qurt_signal_create(&signal))
		{
			SWAT_PTF("qurt_signal_create FAIL!\n");
			goto error;
		}
		
		{
			// PIPE
			qurt_pipe_attr_t attr;
			
			/* Set up the mailbox parameters.                              */
			qurt_pipe_attr_init(&attr);
			qurt_pipe_attr_set_elements(&attr, QURT_TEST_THREADS);
			qurt_pipe_attr_set_element_size(&attr, sizeof(int));
			
			if (QURT_EOK != qurt_pipe_create(&pipe, &attr))
			{
				SWAT_PTF("qurt_pipe_create FAIL!\n");
				goto error;
			}
		}
		{
			qurt_timer_attr_t attr;
   			qurt_time_t       expiration;
			qurt_timer_t timer1 = 0;
			qurt_timer_t timer2 = 0;
   
			qurt_timer_attr_init(&attr);
			
			/* Convert the timer expiration in ms to Ticks. */
			expiration = qurt_timer_convert_time_to_ticks(1000, QURT_TIME_MSEC); // 1s

			/* Configure the timer attributes.            */
			qurt_timer_attr_init(&attr);
			qurt_timer_attr_set_duration(&attr, expiration);
			qurt_timer_attr_set_callback(&attr, swat_qurt_test_timer, (void *)1); // 1s
			qurt_timer_attr_set_reload(&attr, expiration);
			qurt_timer_attr_set_option(&attr, (QURT_TIMER_PERIODIC));
			
			if (QURT_EOK != qurt_timer_create (&timer1, &attr))
			{
				SWAT_PTF("qurt_timer_create FAIL!\n");
				goto error;
			}

			SWAT_PTF("qurt timer test start!\n");

			qurt_timer_start(timer1);

			// Change callback mode to singal mode
			mask = 1;
			qurt_timer_attr_set_signal(&attr, &signal, mask);
			qurt_timer_attr_set_duration(&attr, expiration * 8 + expiration / 2); // 8 times for timer1
			qurt_timer_attr_set_reload(&attr, 0);
			qurt_timer_attr_set_option(&attr, (QURT_TIMER_ONESHOT | QURT_TIMER_AUTO_START));
			
			if (QURT_EOK != qurt_timer_create (&timer2, &attr))
			{
				SWAT_PTF("qurt_timer_create FAIL!\n");
				qurt_timer_stop(timer1);
				qurt_timer_delete(timer1);
				goto error;
			}

			qurt_signal_wait(&signal, mask, QURT_SIGNAL_ATTR_WAIT_ANY | QURT_SIGNAL_ATTR_CLEAR_MASK);

			qurt_timer_stop(timer1);
			qurt_timer_delete(timer2);
			qurt_timer_delete(timer1);
			mask = 0;
			SWAT_PTF("\nqurt timer test SUCCESS!\n\n");
		}		
		
		{
			// Thread
			int i;
			qurt_thread_attr_t       thread_attr;
		    qurt_thread_t            tid;
			
		    qurt_thread_attr_init (&thread_attr);
		    qurt_thread_attr_set_name (&thread_attr, qurt_test_thread_name);
		    qurt_thread_attr_set_priority (&thread_attr, 16);
		    qurt_thread_attr_set_stack_size (&thread_attr, 2048);
			for (i = 0; i < QURT_TEST_THREADS; i++)
			{
				test[i].thread_index = i + 1;
				test[i].lock = &lock;
				test[i].signal = &signal;
				test[i].pipe = pipe;
				if (QURT_EOK != qurt_thread_create (&tid, &thread_attr, swat_qurt_test_thread, (void *)&test[i]))
				{
					SWAT_PTF("%s (%d) FAIL!\n", qurt_test_thread_name, i + 1);
				}
				else
				{
					mask |= 1 << i;
				}
			}
		}
		
		if (mask)
		{
			qurt_signal_wait(&signal, mask, QURT_SIGNAL_ATTR_WAIT_ALL | QURT_SIGNAL_ATTR_CLEAR_MASK);
			SWAT_PTF("\n\nShell RXed all signals from %ss\n\n", qurt_test_thread_name);
		}

error:
		if (pipe != NULL)
		{
			int data = 0;
			while (QURT_EOK == qurt_pipe_try_receive(pipe, &data))
			{	
				SWAT_PTF("Shell RXed pipe data from %s %d\n", qurt_test_thread_name, data);
			}
			
			qurt_pipe_delete(pipe);
		}
		
		if (signal != 0)
		{
			qurt_signal_delete(&signal);
		}
		
		if (lock != 0)
		{
			qurt_mutex_delete(&lock);
		}
		
        return SWAT_OK;	
    }
#endif
    return SWAT_NOFOUND;
}

#if defined(SWAT_WMICONFIG_SOFTAP)

A_INT32
swat_wmiconfig_softAp_handle(A_INT32 argc, A_CHAR * argv[])
{

    A_INT32 data = 0;
	A_UINT32 wifiMode = QCOM_WLAN_DEV_MODE_INVALID;
    qcom_op_get_mode(currentDeviceId, &wifiMode);

    if (!A_STRCMP(argv[1], "--mode")) {
		if (argc < 3)
		{
            SWAT_PTF("wmiconfig --mode <ap|station>\n");
            return SWAT_ERROR;
		}
		
        if (!swat_strcmp((A_CHAR *) argv[2], "ap"))
        {
            SWAT_PTF("Disabling PowerSave\n");

            qcom_power_set_mode(currentDeviceId,MAX_PERF_POWER);
        }
        else if (!swat_strcmp((A_CHAR *) argv[2], "station"))
        {
            SWAT_PTF("Enabling PowerSave\n");

            qcom_power_set_mode(currentDeviceId,REC_POWER);
        }
		else
		{
            SWAT_PTF("wmiconfig --mode <ap|station>\n");
            return SWAT_ERROR;
		}

        if (3 == argc) {

            swat_wmiconfig_dev_mode_set(currentDeviceId, (A_CHAR *) argv[2]);
			if(gDeviceContextPtr[currentDeviceId]->wpsFlag)
			 	gDeviceContextPtr[currentDeviceId]->wpsFlag = 0; //clear WPS flag
        } else if (4 == argc) {
            if (!A_STRCMP(argv[2], "ap")) {
                if (!A_STRCMP(argv[3], "hidden")) {
                    swat_wmiconfig_ap_hidden_set(currentDeviceId);
                } else if (!A_STRCMP(argv[3], "wps")) {
                    swat_wmiconfig_ap_wps_set(currentDeviceId); //will set WPS flag
                } else {
                    SWAT_PTF("wmiconfig --mode ap <hidden|wps>\n");
                    return SWAT_ERROR;
                }

            	swat_wmiconfig_dev_mode_set(currentDeviceId, (A_CHAR *) argv[2]);

            } else {
                SWAT_PTF("wmiconfig --mode ap <hidden|wps>\n");
                return SWAT_ERROR;
            }
        } else {
            SWAT_PTF("wmiconfig --mode <ap|station>\n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--channel")) {
        if (argc == 3) {
            A_SSCANF(argv[2], "%d", &data);
			if ((data < 1) || (data > 165))
			{
			  SWAT_PTF("invalid channel: should range between 1-165\n");
			  return SWAT_ERROR;
			}
            swat_wmiconfig_channel_set(currentDeviceId, (A_UINT32) data);
        } else {
            SWAT_PTF("wmiconfig --channel <channel-id>\n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--wmode")) {
        if (argc != 3 && argc != 4) {
            SWAT_PTF("wmiconfig --wmode <b|g|n|ht40 [<above|below>]|a [<ht20|ht40>]>\n");
            return SWAT_ERROR;
        }

#define QCOM_HT40_MODE_OFFSET       6
#define QCOM_HT40_MODE_DEFAULT      0
#define QCOM_HT40_MODE_ABOVE        1
#define QCOM_HT40_MODE_BELOW        2
        if (0 == A_STRCMP(argv[2], "a")) {
                        A_UINT8 is_11n = 0, bw40 = 0, sgi20 = 0, sgi40 = 0, max_ampdu = 0;
                        qcom_hw_set_phy_mode(currentDeviceId, WMI_WLAN_11A_MODE);
                        if (4 == argc) {
                            if (0 == A_STRCMP(argv[3], "ht20")) {
                                is_11n = 1;
                                sgi20 = 1;
                                max_ampdu = 2;
                            } else if (0 == A_STRCMP(argv[3], "ht40")) {
                                is_11n = 1;
                                bw40 = 1;
                                sgi20 = 1;
                                sgi40 = 1;
                                max_ampdu = 2;
                            }
                        }
                        qcom_set_ht_cap(currentDeviceId, 1, is_11n, bw40, sgi20, sgi40, 0, max_ampdu);
                        gDeviceContextPtr[currentDeviceId]->phymode = QCOM_11A_MODE;
                    } else if (0 == A_STRCMP(argv[2], "b")) {
                        swat_wmiconfig_wifi_mode_set(currentDeviceId, (A_UINT8)QCOM_11B_MODE);
                    } else if (0 == A_STRCMP(argv[2], "g")) {
                        swat_wmiconfig_wifi_mode_set(currentDeviceId, (A_UINT8)QCOM_11G_MODE);
                    } else if (0 == A_STRCMP(argv[2], "n")) {
                        swat_wmiconfig_wifi_mode_set(currentDeviceId, (A_UINT8)QCOM_11N_MODE);
                    } else if (0 == A_STRCMP(argv[2], "ht40")) {
                        A_UINT8 is_11n = 1, bw40 = 1, sgi20 = 1, sgi40 = 1, max_ampdu = 2;
                        qcom_hw_set_phy_mode(currentDeviceId, WMI_WLAN_11G_MODE);
                        if (4 == argc) {
                            if (0 == A_STRCMP(argv[3], "above")) {
                                bw40 |= QCOM_HT40_MODE_ABOVE << QCOM_HT40_MODE_OFFSET;
                            } else if (0 == A_STRCMP(argv[3], "below")) {
                                bw40 |= QCOM_HT40_MODE_BELOW << QCOM_HT40_MODE_OFFSET;
                            }
                        }
                        qcom_set_ht_cap(currentDeviceId, 0, is_11n, bw40, sgi20, sgi40, 0, max_ampdu);
                        gDeviceContextPtr[currentDeviceId]->phymode = QCOM_11G_MODE;
                    } else {
              SWAT_PTF("Unknown wmode\n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }
#if !defined(REV74_TEST_ENV2)
    if (!A_STRCMP(argv[1], "--listen")) {
        if (3 == argc) {
            A_SSCANF(argv[2], "%d", &data);
            swat_wmiconfig_listen_time_set(currentDeviceId, (A_UINT32) data);
        } else {
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--scanctrl")) {
        if (3 == argc || 4 == argc) {
            A_UINT8 fgScan, bgScan;
            qcom_scan_params_t scanParam;
            A_SSCANF(argv[2], "%d", &data);
            fgScan = (A_UINT8)data;
            if (!fgScan) {
                scanParam.fgStartPeriod     = 0xffff;
                scanParam.fgEndPeriod       = 0;
            } else {
                scanParam.fgStartPeriod     = 0;
                scanParam.fgEndPeriod       = 0;
            }
            if (4 == argc) {
                A_SSCANF(argv[3], "%d", &data);
                bgScan = (A_UINT8)data;
                if (!bgScan) {
                    scanParam.bgPeriod      = 0xffff;
                } else {
                    scanParam.bgPeriod      = 60;
                }
            } else {
                scanParam.bgPeriod          = 0;
            }
            scanParam.maxActChDwellTimeInMs = 0;
            scanParam.pasChDwellTimeInMs    = 0;
            scanParam.shortScanRatio        = 3; 		
            scanParam.minActChDwellTimeInMs = 20; 
            scanParam.maxActScanPerSsid     = 1;
            scanParam.maxDfsChActTimeInMs   = 2000;
            scanParam.scanCtrlFlags         = 0x2f;
            swat_wmiconfig_scan_param_set(currentDeviceId, &scanParam);
        } else {
            SWAT_PTF("Invalid parameter format\n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--setscanpara")) {
        if (12 == argc) {
            qcom_scan_params_t scan_params;
            A_SSCANF(argv[2], "%d", &data);
            scan_params.maxActChDwellTimeInMs = (A_UINT16) data;
            A_SSCANF(argv[3], "%d", &data);
            scan_params.pasChDwellTimeInMs    = (A_UINT16) data;
            A_SSCANF(argv[4], "%d", &data);
            scan_params.fgStartPeriod = (A_UINT16) data;
            A_SSCANF(argv[5], "%d", &data);
            scan_params.fgEndPeriod = (A_UINT16) data;
            A_SSCANF(argv[6], "%d", &data);
            scan_params.bgPeriod = (A_UINT16) data;
            A_SSCANF(argv[7], "%d", &data);
            scan_params.shortScanRatio = (A_UINT8) data;
            A_SSCANF(argv[8], "%d", &data);
            scan_params.scanCtrlFlags = (A_UINT8) data;
            A_SSCANF(argv[9], "%d", &data);
            scan_params.minActChDwellTimeInMs = (A_UINT16) data; 
            A_SSCANF(argv[10], "%d", &data);
            scan_params.maxActScanPerSsid = (A_UINT16) data;
            A_SSCANF(argv[11], "%d", &data);
            scan_params.maxDfsChActTimeInMs = (A_UINT32) data;
            swat_wmiconfig_scan_param_set(currentDeviceId, &scan_params); 
        } else {
            if (4 == argc)
            {
              qcom_scan_params_t scan_params; 
              A_SSCANF(argv[2], "%d", &data);
              scan_params.maxActChDwellTimeInMs = (A_UINT16) data;
              A_SSCANF(argv[3], "%d", &data);
              scan_params.pasChDwellTimeInMs    = (A_UINT16) data;
              scan_params.fgStartPeriod = 0;
              scan_params.fgEndPeriod   = 0;
              scan_params.bgPeriod      = 0;
              scan_params.shortScanRatio= 3;
              scan_params.scanCtrlFlags = 0x2f;
              scan_params.minActChDwellTimeInMs = 20;
              scan_params.maxActScanPerSsid = 1;
              scan_params.maxDfsChActTimeInMs = 2000;
              swat_wmiconfig_scan_param_set(currentDeviceId, &scan_params);
              return SWAT_OK;
            }
            SWAT_PTF("wmiconfig --setscanpara <max_act_ch_dwell_time_ms> <pas_act_chan_dwell_time_ms> <fg_start_period(in secs)> <fg_end_period (in secs)> <bg_period (in secs)> <short_scan_ratio> <scan_ctrl_flags>  <min_active_chan_dwell_time_ms> <max_act_scan_per_ssid> <max_dfs_ch_act_time_in_ms> \n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--setscan")) {
        QCOM_BSS_SCAN_INFO* pOut;
        A_UINT16 count;

        if (7 <= argc) {
            qcom_start_scan_params_t scan_params;
            A_UINT16 maxArgCount = 0, argI = 0;
            A_SSCANF(argv[2], "%d", &data);
            scan_params.forceFgScan = (A_BOOL) data;
            A_SSCANF(argv[3], "%d", &data);
            scan_params.homeDwellTimeInMs    = (A_UINT32) data;
            A_SSCANF(argv[4], "%d", &data);
            scan_params.forceScanIntervalInMs = (A_UINT32) data;
            A_SSCANF(argv[5], "%d", &data);
            scan_params.scanType = (A_UINT8) data;
            A_SSCANF(argv[6], "%d", &data);
            scan_params.numChannels = (A_UINT8) data;
            if (scan_params.numChannels > 12)
            {
               SWAT_PTF("cannot set more than 12 channels to scan\n");
            }
            maxArgCount = scan_params.numChannels + 7;
            argI = 7;
            if (argc != maxArgCount)
            {
               SWAT_PTF("wmiconfig --setscan <forceFgScan> <homeDwellTimeInMs> <forceScanIntervalInMs> <scanType> <numChannels> [<channelInMhz 1> <channelInMhz 2>...<channelInMhz N>]\n");
               return SWAT_ERROR;
            }
            while (argI < maxArgCount)
            {
               A_SSCANF(argv[argI], "%d", &data);
               scan_params.channelList[argI-7] = (A_UINT16) data;
               argI++;
            }
//  TODO: USE_QCOM_REL_3_3_API is for KF, what's for Ruby?
//#ifndef USE_QCOM_REL_3_3_API
#if 1
            qcom_set_scan(currentDeviceId, &scan_params); 
#else
            qcom_set_scan(currentDeviceId); 
#endif 
        } else {
            SWAT_PTF("wmiconfig --setscan <forceFgScan> <homeDwellTimeInMs> <forceScanIntervalInMs> <scanType> <numChannels> [<channel> <channel>... upto numChannels]\n");
            return SWAT_ERROR;
        }
        if(qcom_get_scan(currentDeviceId, &pOut, &count) == A_OK){
           QCOM_BSS_SCAN_INFO* info = pOut;
           int i,j;

           tx_thread_sleep(1000);
           printf("count : %d\n", count);
           for (i = 0; i < count; i++)
           {
               printf("ssid = ");
               {
                   for(j = 0;j < info[i].ssid_len;j++)
                   {
                      printf("%c",info[i].ssid[j]);
                   }
                   printf("\n");
               }  
               printf("bssid = %.2x:%.2x:%.2x:%.2x:%.2x:%.2x\n",
                     info[i].bssid[0], info[i].bssid[1],
                     info[i].bssid[2], info[i].bssid[3],
                     info[i].bssid[4], info[i].bssid[5]);

               printf("channel = %d\n",info[i].channel);

               if (i != (count-1))
                  printf("\n");
           }
           printf("shell> ");
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ap")) {
        if (argc < 3) {
            SWAT_PTF("Missing parameter\n");
            return SWAT_ERROR;
        }

        if (!A_STRCMP(argv[2], "bconint")) {
            if (4 == argc) {
                A_SSCANF(argv[3], "%d", &data);
                swat_wmiconfig_bcon_int_set(currentDeviceId, (A_UINT16) data);
            } else {
                SWAT_PTF("Invalid paramter format\n");
                return SWAT_ERROR;
            }

        }
        if (!A_STRCMP(argv[2], "country")) {
            A_CHAR country_code[3];

			if (wifiMode != QCOM_WLAN_DEV_MODE_AP)
			{
                SWAT_PTF("Country Code only supported in AP mode\n");
				return SWAT_ERROR;
			}
			
            if (4 == argc) {
                country_code[0] = argv[3][0];
                country_code[1] = argv[3][1];
                country_code[2] = 0x20;
                /* Removed country code QAPI. However, retain this application
                 * API so as to not break CST scripts/tests
                 */
                swat_wmiconfig_country_code_set(currentDeviceId, country_code);
            } else {
                SWAT_PTF("Invalid paramter format\n");
                return SWAT_ERROR;
            }
        }

        if (!A_STRCMP(argv[2], "countryie")) {

			if (wifiMode != QCOM_WLAN_DEV_MODE_AP)
			{
                SWAT_PTF("Country IE only supported in AP mode\n");
				return SWAT_ERROR;
			}
			
            if (4 == argc) {
                if (!swat_strcmp((A_CHAR *) argv[3], "enable")){
        			swat_wmiconfig_country_ie_enable(currentDeviceId, 1);
        			return SWAT_OK;
        		}
        		else if (!swat_strcmp((A_CHAR *) argv[3], "disable")){
        			swat_wmiconfig_country_ie_enable(currentDeviceId, 0);
        			return SWAT_OK;
        		}
                else{
                    SWAT_PTF("wmiconfig --ap countryie <enable/disable>\n");
                }
            } else {
                SWAT_PTF("wmiconfig --ap countryie <enable/disable>\n");
                return SWAT_ERROR;
            }
        }

		if (!A_STRCMP(argv[2], "inact")) {
            if (4 == argc) {
                A_SSCANF(argv[3], "%d", &data);
                swat_wmiconfig_inact_set(currentDeviceId, (A_UINT16) data);
            } else {
                SWAT_PTF("Invalid paramter format\n");
                return SWAT_ERROR;
            }

        }

        if (!A_STRCMP(argv[2], "setmaxstanum")) {

            if (4 == argc) {
                 A_SSCANF(argv[3], "%d", &data);
                if (A_OK != qcom_ap_set_max_station_number(currentDeviceId, (A_UINT32) data)) {
                    SWAT_PTF("setmaxstanum %d command fail, stanum 0-4 \n",data);
                    return SWAT_OK;
                }
            } else {
                SWAT_PTF("Invalid paramter format\n");
                return SWAT_OK;
            }
        }

		if (!A_STRCMP(argv[2], "stainfo")) {
            if (3 == argc) {
                swat_wmiconfig_sta_info(currentDeviceId);
            } else {
                SWAT_PTF("Invalid paramter format\n");
                return SWAT_ERROR;
            }

        }

        return SWAT_OK;
    }
#endif
    return SWAT_NOFOUND;
}
#endif

#if defined(SWAT_WMICONFIG_WEP)

A_INT32
swat_wmiconfig_wep_handle(A_INT32 argc, A_CHAR * argv[])
{

    A_UINT32 key_index;
	A_UINT32 mode;

    if (!A_STRCMP(argv[1], "--wepkey")) {

        if (argc == 4) {
            A_SSCANF(argv[2], "%d", &key_index);

            if ((key_index > 4) || (key_index < 1)) {
                SWAT_PTF("key_index: 1-4 \n");
                return SWAT_ERROR;
            }

            swat_wmiconfig_wep_key_set(currentDeviceId, argv[3], (A_CHAR) key_index);
        } else {
            SWAT_PTF("	  --wepkey <key_index> <key>\n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--wep")) {

        if (argc == 4) {
            A_SSCANF(argv[2], "%d", &key_index);
            if ((key_index > 4) || (key_index < 1)) {
                SWAT_PTF("key_index: 1-4 \n");
                return SWAT_ERROR;
            }
			if (!A_STRCMP(argv[3], "open"))
			{
			  mode = 0;
			}
			else if (!A_STRCMP(argv[3], "shared"))
			{
			  mode = 1;
			}
            	       else if (!A_STRCMP(argv[3], "auto"))
			{
			  mode = 2;
			}
			else
			{
			  SWAT_PTF("<open> or <shared> \n");
			  return SWAT_ERROR;
			}
            /* support open mode now*/
            swat_wmiconfig_wep_key_index_set(currentDeviceId, key_index, mode);
        }else if (argc == 3){
        	A_SSCANF(argv[2], "%d", &key_index);
            if ((key_index > 4) || (key_index < 1)) {
                SWAT_PTF("key_index: 1-4 \n");
                return SWAT_ERROR;
            }
			swat_wmiconfig_wep_key_index_set(currentDeviceId, key_index, 2);
    	}
		else {
            SWAT_PTF("	  --wep <def_keyix> <mode>\n");
            return SWAT_ERROR;
        }
        return SWAT_OK;
    }

		return SWAT_NOFOUND;

}
#endif

#if defined(SWAT_WMICONFIG_WPA)

A_INT32
swat_wmiconfig_wpa_handle(A_INT32 argc, A_CHAR * argv[])
{
     A_UINT32 str_len = 0 , i = 0; 
     A_CHAR psd_buf[65] ;
      if (!A_STRCMP(argv[1], "--p")) {
        if (3 == argc) {
           str_len = strlen(argv[2]); 
            if((str_len < 8)||(str_len > 64))
               {
                     SWAT_PTF("PSK passcode len should in 8~63 in ASCII and 8~64 in HEX \n");
                     return SWAT_ERROR;
               }
            else if(str_len == 64)
               {
                /*wpa2-psk passwd len is 64, the passwd must be NON ASCII char*/
                  A_STRNCPY(psd_buf,argv[2],64);      
                   for(i=0;i<64;i++)
                     {
                       if((psd_buf[i]<48)||((psd_buf[i]>57)&&(psd_buf[i]<64))||((psd_buf[i]>90)&&(psd_buf[i]<97))||(psd_buf[i]>123))
                       {
                        SWAT_PTF("PSK passcode len is 64 and can't contain Non 16 HEX char.\n");
                        return SWAT_ERROR;
                       }
                     }
                 swat_wmiconfig_wep_passowrd_set(currentDeviceId, argv[2]);

               }
            else 
               {
                 swat_wmiconfig_wep_passowrd_set(currentDeviceId, argv[2]);
               }
        } else {
            SWAT_PTF("	  wmiconfig --p <passphrase>\n");
            return SWAT_ERROR;
        }

        return SWAT_OK;
    }

#if !defined(REV74_TEST_ENV2)
    if (!A_STRCMP(argv[1], "--wpa")) {
        if (argc == 5) {
            swat_wmiconfig_wpa_set(currentDeviceId, argv[2], argv[3], argv[4]);
        } else {
            SWAT_PTF("	  --wpa <ver> <ucipher> <mcipher>\n");
            return SWAT_ERROR;
        }
        return SWAT_OK;
    }
#endif
    return SWAT_NOFOUND;
}


#endif

#if 1

#if defined(SWAT_WMICONFIG_WPS)
A_INT32 wps_config_state_mode;
A_UINT32 wps_config_state_mode_is_set;
A_INT32
swat_wmiconfig_wps_handle(A_INT32 argc, A_CHAR * argv[])
{
    if (!A_STRCMP(argv[1], "--wps")) {
        A_UINT32 connect = 0;

        if (argc < 4) {
            SWAT_PTF
                ("wmiconfig --wps <connect:0-not connect after wps 1:connect after wps> <mode:pin/push> [pin:<=8 characters] <*ssid> <*mac:xx:xx:xx:xx:xx:xx> <*channel>\n");
            SWAT_PTF("wmiconfig --wps config <0-not configured 1:configured>\n");
            return SWAT_ERROR;
        }

        /* Since WPS function is working on device 1, so enable the wps
         * function on device 1 to unblock the test
         */
#if 0
        if (0 != currentDeviceId) {
            SWAT_PTF("Only dev0 support WPS!\n");
            return SWAT_OK;
        }
#endif
      	A_UINT8 chipState=0;
		A_UINT32 chipMode=0;
		qcom_op_get_mode(currentDeviceId, &chipMode);
		qcom_get_state(currentDeviceId, &chipState);
      	if((gDeviceContextPtr[currentDeviceId]->wpsFlag!=1) && 
					(chipMode==QCOM_WLAN_DEV_MODE_AP))
  		{
			SWAT_PTF("WPS NOT enabled\n");
			return SWAT_ERROR;
  		}
		if(chipMode==QCOM_WLAN_DEV_MODE_AP && chipState!=QCOM_WLAN_LINK_STATE_CONNECTED_STATE)
		{
			SWAT_PTF("AP not connected\n");
			return SWAT_ERROR;
		}

        if (!A_STRCMP(argv[2], "config")) {
            A_INT32 mode;
            A_SSCANF(argv[3], "%d", &mode);
            wps_config_state_mode = mode;
            wps_config_state_mode_is_set = 1;
        } else {
            A_SSCANF(argv[2], "%d", &connect);
            if (!A_STRCMP(argv[3], "pin")) {
                if (argc < 5) {
                    SWAT_PTF("Parameter number is too less.\n");
                    return SWAT_ERROR;
                }

                if (strlen(argv[4]) > 8) {
                    SWAT_PTF("pin length must be less than 8.\n");
                    return SWAT_ERROR;
                }
                if(argc == 8)
                {
                    int aidata[6];
                    char ssid[WMI_MAX_SSID_LEN];
                    int rt;
                    int channel;
                    A_UINT8 macaddress[6];

                    if(strlen(argv[5]) > WMI_MAX_SSID_LEN)
    				{
    					SWAT_PTF("Invalid ssid length\n");
    					return A_ERROR;
    				}
                    strcpy(ssid,argv[5]);

                    rt = sscanf(argv[6], "%2X:%2X:%2X:%2X:%2X:%2X", \
                                &aidata[0], &aidata[1], &aidata[2], \
                                &aidata[3], &aidata[4], &aidata[5]);

                    if (rt < 0)
                    {
                        SWAT_PTF("wrong mac format.\n");
                        return A_ERROR;
                    }

                    for (rt = 0; rt < 6; rt++)
                    {
                        macaddress[rt] = (A_UINT8)aidata[rt];
                    }

                    rt = sscanf(argv[7], "%d", &channel);
                    if((channel < 1) || (channel > 165) || rt < 0)
                    {
                        SWAT_PTF("wrong channel.\n");
                        return A_ERROR;
                    }

                    swat_wmiconfig_wps_enable(currentDeviceId, 1);
                    if (wps_config_state_mode_is_set) {
                        swat_wmiconfig_wps_config_state_set(currentDeviceId, wps_config_state_mode);
                        wps_config_state_mode_is_set = 0;
                    }
                    swat_wmiconfig_wps_start_without_scan(currentDeviceId,connect,0,(A_INT8*)argv[4],(A_INT8*)ssid,channel,macaddress);
                }else if(argc > 5 && argc < 8)
                {
                    SWAT_PTF("Parameter number is too less.\n");
                }else{
                    swat_wmiconfig_wps_enable(currentDeviceId, 1);
                    if (wps_config_state_mode_is_set) {
                        swat_wmiconfig_wps_config_state_set(currentDeviceId, wps_config_state_mode);
                        wps_config_state_mode_is_set = 0;
                    }
                    swat_wmiconfig_wps_start(currentDeviceId, connect, 0, (A_INT8 *)argv[4]);
                }
            } else if (!A_STRCMP(argv[3], "push")) {
       
                swat_wmiconfig_wps_enable(currentDeviceId, 1);
                if (wps_config_state_mode_is_set) {
                    swat_wmiconfig_wps_config_state_set(currentDeviceId, wps_config_state_mode);
                    wps_config_state_mode_is_set = 0;
                }
                swat_wmiconfig_wps_start(currentDeviceId, connect, 1, 0);
            } else {
                SWAT_PTF("Invalid wps mode.\n");
                return SWAT_ERROR;
            }
        }

        return SWAT_OK;
    }

    return SWAT_NOFOUND;
}
#endif

#if defined(SWAT_WMICONFIG_P2P)
A_INT32
swat_wmiconfig_p2p_handle(A_INT32 argc, A_CHAR * argv[])
{
    extern void swat_wmiconfig_p2p(int argc, char *argv[]);
    if (!A_STRCMP(argv[1], "--p2p")) {
#if defined(P2P_ENABLED)
         swat_wmiconfig_p2p(argc - 1, &argv[1]);
        return SWAT_OK;
#endif
    }

    return SWAT_NOFOUND;

}
#endif
#if defined(SWAT_WMICONFIG_IP)

A_INT32
swat_wmiconfig_ip_handle(A_INT32 argc, A_CHAR * argv[])
{
    if (!A_STRCMP(argv[1], "--ipstatic")) {
        if (argc < 5) {
            SWAT_PTF("--ipstatic x.x.x.x(ip) x.x.x.x(msk) x.x.x.x(gw)\n");
            return SWAT_ERROR;
        }
		A_UINT32 wifiMode=0;
        qcom_op_get_mode(currentDeviceId, &wifiMode);
	 	if (wifiMode == 1) {
                SWAT_PTF("Make sure gateway and DHCP pool are in the same segment\n");
        }
        swat_wmiconfig_ipstatic(currentDeviceId, argv[2], argv[3], argv[4]);

        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ipdhcp")) {
		A_UINT32 wifiMode = QCOM_WLAN_DEV_MODE_INVALID;
        qcom_op_get_mode(currentDeviceId, &wifiMode);

		if ((wifiMode != QCOM_WLAN_DEV_MODE_STATION)
            &&(wifiMode != QCOM_WLAN_DEV_MODE_ADHOC))
		{
            SWAT_PTF("DHCP only supported in station and adhoc mode to get IP address\n");
	        return SWAT_ERROR;
		}
		
        swat_wmiconfig_ipdhcp(currentDeviceId);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ipauto")) {
        if(argc == 3)
        {
           swat_wmiconfig_ipauto(currentDeviceId, argv[2]); 
        }
        else
        {
            swat_wmiconfig_ipauto(currentDeviceId, NULL); 
        }
        
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ipdhcp_release")) {
        qcom_dhcps_release_pool(currentDeviceId);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--tcp_backoff_retry")) {
        A_INT32 retry = atoi(argv[2]);
        if(retry > 12 || retry < 4){
            SWAT_PTF("TCP max retries between 4 and 12\n");
            return SWAT_ERROR;
        }
        qcom_tcp_set_exp_backoff(currentDeviceId, retry);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ipconfig")) {
        swat_wmiconfig_ipconfig(currentDeviceId);
        return SWAT_OK;
    }

#ifdef SWAT_DNS_ENABLED

    if (!A_STRCMP(argv[1], "--ip_gethostbyname")) {
        if (argc < 3) {
            SWAT_PTF("--ip_gethostbyname <hostname>\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns(currentDeviceId, argv[2]);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ip_cleardnsentry")) {
        if (argc < 2) {
            SWAT_PTF("--dns entry clear error!\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns_clear();
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ip_gethostbyname2")) {
        if (argc < 4) {
            SWAT_PTF("--ip_gethostbyname2 [<host name> <domain_type]  = resolve hostname for domain_type (ipv4 =2 ipv6 =3)\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns2(currentDeviceId, argv[2], atoi(argv[3]), 1); /* 1:gethostbyname2, 2:resolvehostname */
        return SWAT_OK;
    }
    
    if (!A_STRCMP(argv[1], "--ip_gethostbyname3")) {
        if (argc < 4) {
            SWAT_PTF("--ip_gethostbyname3 [<host name> <domain_type>]  = resolve hostname for domain_type (ipv4 =2 ipv6 =3)\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns3(currentDeviceId, argv[2], atoi(argv[3]), 1); /* 1:gethostbyname2, 2:resolvehostname */
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ip_resolve_hostname")) {
        if (argc < 4) {
            SWAT_PTF("--ip_resolve_hostname [<host name> <domain_type]  = resolve hostname for domain_type (ipv4 =2 ipv6 =3)\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns2(currentDeviceId, argv[2], atoi(argv[3]), 2); /* 1:gethostbyname2, 2:resolvehostname */
        return SWAT_OK;
    }
#endif

#ifdef BRIDGE_ENABLED
	if (!A_STRCMP(argv[1], "--ipbridgemode")) {
    		swat_wmiconfig_bridge_mode_enable(currentDeviceId);
    		bridge_mode = 1;
            
		return SWAT_OK;
	}
#endif
    if (!A_STRCMP(argv[1], "--pingId")) {
        if (argc < 3) {
        //SWAT_PTF("--pingId <new_pingId>\n");
        return SWAT_ERROR;
        }
        swat_wmiconfig_set_ping_id(currentDeviceId, atoi(argv[2]));
        return SWAT_OK;
    }	

   	if (!A_STRCMP(argv[1], "--ipv4_route")) {
		swat_ipv4_route(currentDeviceId,argc,argv);
		return SWAT_OK;
	}
    if (!A_STRCMP(argv[1], "--ipv6_route")) {
		swat_ipv6_route(currentDeviceId,argc,argv);
		return SWAT_OK;
	}
    if (!A_STRCMP(argv[1],"--ipv6")){
        if (argc < 3) {
            SWAT_PTF("--ipv6 enable/disable\n");
            return SWAT_ERROR;
        }
        swat_set_ipv6_status(currentDeviceId,argv[2]);
        return SWAT_OK;
    }

#ifdef SWAT_DNS_ENABLED 
    if (!A_STRCMP(argv[1], "--ip_dns_client")) {
        if (argc < 3) {
            SWAT_PTF("--ip_dns_client start/stop\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns_enable(currentDeviceId, argv[2]);
        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--ip_dns_server_enable")) {
        A_UINT32 dns_wifiMode;
        if (argc < 3) {
            SWAT_PTF("--ip_dns_server_enable <enable/disable>\n");
            return SWAT_ERROR;
        }
        qcom_op_get_mode(currentDeviceId, &dns_wifiMode);
         if (dns_wifiMode == 0) {
             SWAT_PTF("DNS Server is not possible in station mode\n");
             return SWAT_ERROR;
            }
        swat_wmiconfig_dnss_enable(currentDeviceId, argv[2]);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ip_dns_local_domain")) {
        if (argc < 3) {
            SWAT_PTF("--ip_dns_local_domain local_domain\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns_domain(currentDeviceId, argv[2]);
        return SWAT_OK;
    }
    if (!A_STRCMP(argv[1], "--ip_dns")) {
        if (argc < 5) {
            SWAT_PTF("--ip_dns <add/delete> <hostname> <ipaddr>\n");
            return SWAT_ERROR;
        }

		if(!A_STRCMP(argv[2], "add")){
        swat_wmiconfig_dns_entry_add(currentDeviceId, argv[3], argv[4]);
        return SWAT_OK;
    }
		else if(!A_STRCMP(argv[2], "delete")){
        swat_wmiconfig_dns_entry_del(currentDeviceId, argv[3], argv[4]);
        return SWAT_OK;
		}
		else{
            SWAT_PTF("--ip_dns <add/delete> <hostname> <ipaddr>\n");
            return SWAT_ERROR;
        }
    }

    if (!A_STRCMP(argv[1], "--ip_dns_server_addr")) {
        if (argc < 3) {
            SWAT_PTF("--ip_dns_server_addr <xx.xx.xx.xx>\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns_svr_add(currentDeviceId, argv[2]);
        return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--ip_dns_delete_server_addr")) {
        if (argc < 3) {
            SWAT_PTF("--ip_dns_delete_server_addr <xx.xx.xx.xx>\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns_svr_del(currentDeviceId, argv[2]);
        return SWAT_OK;
    }
	
    if (!A_STRCMP(argv[1], "--ip_dns_timeout")) {
        if (argc < 3) {
            SWAT_PTF("--ip_dns_timeout <timeout (in sec)>\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_dns_set_timeout(currentDeviceId, atoi(argv[2]));
        return SWAT_OK;
    }
#endif
    if (!A_STRCMP(argv[1], "--ipdhcppool")) {
        A_INT32 data;
        A_UINT32 wifiMode;
        if (argc != 5) {
            SWAT_PTF("Missing parameter\n");
            return SWAT_ERROR;
        }

        data = atoi(argv[4]); 
        qcom_op_get_mode(currentDeviceId, &wifiMode);
	 if (wifiMode == 0) {
                SWAT_PTF("Setting DHCP pool is not allowed MODE_STA \n");
                return SWAT_ERROR;
        }
        SWAT_PTF("Make sure gateway and DHCP pool are in the same segment\n");
        swat_wmiconfig_dhcp_pool(currentDeviceId, argv[2], argv[3], data);
        return SWAT_OK;
    }
    
    if(!A_STRCMP(argv[1],"--dhcpscb")){
        if(argc < 3)
        {
            SWAT_PTF("incomplete params:e.g wmiconfig --dhcpscb <start/stop>\n");
            return SWAT_ERROR;
        }       
        swat_wmiconfig_dhcps_cb_enable(currentDeviceId, argv[2]);

        return SWAT_OK;
    }
    if(!A_STRCMP(argv[1],"--dhcpccb")){
         if(argc < 3)
         {
             SWAT_PTF("incomplete params:e.g wmiconfig --dhcpccb <start/stop>\n");
            return SWAT_ERROR;
         }       
         swat_wmiconfig_dhcpc_cb_enable(currentDeviceId, argv[2]);

         return SWAT_OK;
    }
    if(!A_STRCMP(argv[1],"--autoipcb")){
         if(argc < 3)
         {
             SWAT_PTF("incomplete params:e.g wmiconfig --autoipcb <start/stop>\n");
            return SWAT_ERROR;
         }       
         swat_wmiconfig_autoip_cb_enable(currentDeviceId, argv[2]);

         return SWAT_OK;
    }

    if (!A_STRCMP(argv[1], "--iphostname")) {
        if (argc < 3) {
            SWAT_PTF("--iphostname <new_host_name>\n");
            return SWAT_ERROR;
        }
        swat_wmiconfig_set_hostname(currentDeviceId, argv[2]);
        return SWAT_OK;
    }
#ifdef SWAT_WMICONFIG_SNTP
    if (!A_STRCMP(argv[1], "--ip_sntp_client"))
    {
                if(argc < 3)
                {
                  SWAT_PTF("incomplete params\n");
                  return SWAT_ERROR;
                }
        swat_wmiconfig_sntp_enable(currentDeviceId, argv[2]);
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1], "--ip_sntp_srvr"))
    {
               if(argc < 4)
               {
                 SWAT_PTF("incomplete params\n");
                 return SWAT_ERROR;
               }
        swat_wmiconfig_sntp_srv(currentDeviceId, argv[2],argv[3]);
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1], "--ip_sntp_zone"))
    {
               if(argc < 5)
               {
                 SWAT_PTF("incomplete params\n");
                 return SWAT_ERROR;
               }

               if(strlen(argv[3]) > 3)
               {
                 SWAT_PTF("Error : Invalid string.only dse is valid\n");
                 return SWAT_ERROR;
               }
        swat_wmiconfig_sntp_zone(currentDeviceId, argv[2],argv[4]);
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1], "--ip_sntp_get_time"))
    {
        swat_wmiconfig_sntp_get_time(currentDeviceId);
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1], "--ip_sntp_sync"))
    {
        A_UINT8 enable = 0;
        A_UINT32 mseconds = 0;
        A_UINT8 type = 0;
        if(argc == 5 && !strcmp(argv[2], "start"))
        {
            enable = 1;
            type = atoi(argv[3]);
            mseconds = (A_UINT32)atoi(argv[4]);
                        
            if((type != 0 && type != 1) || (type == 1 && mseconds <= 0))
            {
                SWAT_PTF("invalid params \n");
                return SWAT_ERROR; 
            }
        }
        else if(argc == 3 && !strcmp(argv[2], "stop"))
        {
            enable = 0;
        }
        else
        {
            SWAT_PTF("incomplete params\n");
            return SWAT_ERROR;
        }
            
        swat_wmiconfig_sntp_sync(enable, type, mseconds);
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1], "--ip_sntp_get_time_of_day"))
    {
        swat_wmiconfig_sntp_get_time_of_day(currentDeviceId);
        return SWAT_OK;
    }

    if(!A_STRCMP(argv[1], "--ip_show_sntpconfig"))
    {
        swat_wmiconfig_sntp_show_config(currentDeviceId);
        return SWAT_OK;
    }
#endif /* SWAT_SNTP_CLIENT */
#ifdef ENABLE_HTTP_SERVER
    if (strcmp(argv[1],"--ip_http_server") == 0)
    {
        swat_http_server(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_https_server") == 0)
    {
        swat_http_server(argc, argv);
        return SWAT_OK;
    }

    if (strcmp(argv[1],"--ip_http_post") == 0)
    {
        swat_http_server_post(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_http_get") == 0)
    {
        swat_http_server_get(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_http_put") == 0)
    {
        swat_http_server_put(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_http_delete") == 0)
    {
        swat_http_server_delete(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_http_set_custom_uri") == 0)
    {
        swat_wmiconfig_set_http_custom_uri();
        return SWAT_OK;
    }

    if (strcmp(argv[1],"--ip_httpsvr_ota") == 0)
    {
        if(argc < 3)
        {
            SWAT_PTF("incomplete params:e.g wmiconfig --ip_httpsvr_ota <enable/disable>\n");
            return SWAT_ERROR;
        } 

        swat_wmiconfig_httpsvr_ota_enable(argv[2]);
        return SWAT_OK;
    }
    
    if (strcmp(argv[1],"--ip_http_set_redirected_url") == 0)
    {
        
        swat_http_set_redirected_url(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_http_redirect_unknown_url") == 0)
    {
        swat_http_redirect_unknown_url(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_http_server_add_redirected_html") == 0)
    {
        swat_http_server_add_redirected_page(argc, argv);
        return SWAT_OK;
    }
    if (strcmp(argv[1],"--ip_http_request_restriction") == 0)
    {
        swat_http_restrict_request(argc, argv);
        return SWAT_OK;
    }
#endif /* ENABLE_HTTP_SERVER */

#ifdef ENABLE_HTTP_CLIENT
    if(strcmp(argv[1], "--ip_http_client") == 0)
    {
        httpc_command_parser(argc, argv);
        return SWAT_OK;
    }
	
    if (strncmp(argv[1], "--ip_httpclient2_", 17) == 0)
    {
        httpclient2_process(argc, argv);
        return SWAT_OK;
    }
#endif /* ENABLE_HTTP_CLIENT */

#if !defined(REV74_TEST_ENV2)
 if(strcmp(argv[1], "--ip6rtprfx") == 0)
    {
        swat_ip6_set_router_prefix(argc,argv);
        return SWAT_OK;
    }
#endif
#ifdef SWAT_SSL
    if(strcmp(argv[1], "--ssl_start") == 0)
    {
        ssl_start(argc,argv);
        return SWAT_OK;
    }
    if(strcmp(argv[1], "--ssl_stop") == 0)
    {
        ssl_stop(argc,argv);
        return SWAT_OK;
    }
    if(strcmp(argv[1], "--ssl_config") == 0)
    {
        ssl_config(argc,argv);
        return SWAT_OK;
    }

    if(strcmp(argv[1], "--ssl_add_cert") == 0)
    {
        ssl_add_cert(argc,argv);
        return SWAT_OK;
    }
	if(strcmp(argv[1], "--ssl_store_cert") == 0)
    {
        ssl_store_cert(argc,argv);
        return SWAT_OK;
    }
	if(strcmp(argv[1], "--ssl_delete_cert") == 0)
    {
        ssl_delete_cert(argc,argv);
        return SWAT_OK;
    }
	if(strcmp(argv[1], "--ssl_list_cert") == 0)
    {
        ssl_list_cert(argc,argv);
        return SWAT_OK;
    }
	if(strcmp(argv[1], "--ssl_set_cert_check_tm") == 0)
    {
        ssl_set_cert_check_tm(argc,argv);
        return SWAT_OK;
    }
	if (strcmp(argv[1], "--ssl_api_test_threads") == 0)
    {
        if (argc >= 3)
        {
	        if (strcmp(argv[2], "stop") == 0)
	    	{
		    	swat_ssl_test_threads_stop();
	        	return SWAT_OK;
	    	}
			else if ((strcmp(argv[2], "start") == 0) && argc >= 5)
			{
				A_UINT32 server_ip;
				A_INT32 inet_aton(const A_CHAR *name, A_UINT32 * ipaddr_ptr);
				A_UINT32 num_threads = 0;
				
				num_threads = atoi(argv[4]);
				
				if (num_threads >0 && num_threads <= 8 && inet_aton(argv[3], &server_ip))
				{
					swat_ssl_test_threads_start(num_threads, server_ip);
	        		return SWAT_OK;
				}
			}
        }
		
        SWAT_PTF("Usage: wmiconfig --ssl_api_test_threads <start <Ipv4Addr> <numThreads:1-8>> | <stop>\n");
        return SWAT_ERROR;
    }
#endif
#ifdef SWAT_OTA
if(!A_STRCMP(argv[1],"--ota_upgrade"))
    {
       swat_ota_upgrade(currentDeviceId,argc,argv);
        return SWAT_OK;
    }
if(!A_STRCMP(argv[1],"--ota_read"))
   {
       swat_ota_read(argc,argv);
        return SWAT_OK;
   }

if(!A_STRCMP(argv[1],"--ota_done"))
   {
        swat_ota_done(argc,argv);
        return SWAT_OK;
   }
if(!A_STRCMP(argv[1],"--ota_cust"))
   {
        swat_ota_cust(argc,argv);
        return SWAT_OK;
   }
if(!A_STRCMP(argv[1],"--ota_format"))
   {
        swat_ota_format(argc,argv);
        return SWAT_OK;
   }
if(!A_STRCMP(argv[1],"--ota_ftp"))
   {
        swat_ota_ftp_upgrade(currentDeviceId, argc,argv);
        return SWAT_OK;
   }
if(!A_STRCMP(argv[1],"--ota_http"))
   {
        swat_ota_https_upgrade(currentDeviceId, argc, argv);
        return SWAT_OK;
   }
if(!A_STRCMP(argv[1],"--ota_https"))
   {
        swat_ota_https_upgrade(currentDeviceId, argc, argv);
        return SWAT_OK;
   }
#endif

//SSDP Extensions
#ifdef SSDP_ENABLED
    if(!A_STRCMP(argv[1],"--ssdp_init"))	
    {
        swat_ssdp_init(argc, argv);
        return SWAT_OK;
    }
    if(!A_STRCMP(argv[1],"--ssdp_enable"))
    {
        swat_ssdp_enable(argc, argv);
        return SWAT_OK;
    }
    if(!A_STRCMP(argv[1],"--ssdp_change_notify_period"))
    {
        swat_ssdp_change_notify_period(argc, argv);
        return SWAT_OK;	
    }
#endif //#ifdef SSDP_ENABLED

if(!A_STRCMP(argv[1],"--settcptimeout"))
   {
         swat_tcp_conn_timeout(argc,argv);
        return SWAT_OK;
    }


    return SWAT_NOFOUND;
}
#endif


#if defined(SWAT_BENCH)
#define IPERF_SERVER 0
#define IPERF_CLIENT 1
#define IPERF_DEFAULT_PORT 5001
#define IPERF_DEFAULT_RUNTIME 10
#define IPERF_MAX_PACKET_SIZE_TCP 1452 /* Max performance in Ruby without splitting packets */
#define IPERF_MAX_PACKET_SIZE_UDP 1400 /* Max UDP in Ruby */
#define  IPERF_DEFAULT_UDPRate  (1024 * 1024); // Default UDP Rate, 1 Mbit/sec

#define IPERF_kKilo_to_Unit  1024;
#define IPERF_kMega_to_Unit  (1024 * 1024);
#define IPERF_kGiga_to_Unit  (1024 * 1024 * 1024);

#define IPERF_kkilo_to_Unit  1000;
#define IPERF_kmega_to_Unit (1000 * 1000);
#define IPERF_kgiga_to_Unit (1000 * 1000 * 1000);

A_UINT32  byte_atoi( const char *inString ) {
    A_UINT32 theNum = 0;
    char suffix = '\0';

     if (!inString)
	    return 0;
    /* scan the number and any suffices */
    swat_sscanf( (char *)inString, "%d%c", &theNum, &suffix );

    /* convert according to [Gg Mm Kk] */
    switch ( suffix ) {
        case 'G':  theNum *= IPERF_kGiga_to_Unit;  break;
        case 'M':  theNum *= IPERF_kMega_to_Unit;  break;
        case 'K':  theNum *= IPERF_kKilo_to_Unit;  break;
        case 'g':  theNum *= IPERF_kgiga_to_Unit;  break;
        case 'm':  theNum *= IPERF_kmega_to_Unit;  break;
        case 'k':  theNum *= IPERF_kgiga_to_Unit;  break;
        default: break;
    }
    return  theNum;
} /* end byte_atof */

A_INT32
swat_iperf_handle(A_INT32 argc, A_CHAR * argv[])
{
    A_UINT32 protocol = TEST_PROT_TCP;
    A_UINT32 port = IPERF_DEFAULT_PORT;
    A_UINT32 seconds = IPERF_DEFAULT_RUNTIME;
    A_UINT32 pktSize = IPERF_MAX_PACKET_SIZE_TCP;
    A_UINT32 mode = 0;
    A_INT32 operation_mode = -1;  
    A_UINT32 udpRate = IPERF_DEFAULT_UDPRate;
    A_UINT8 mcastEnabled = 0;

    A_UINT32 ipAddress = 0;
    A_UINT32 bindAddress = 0;
    A_UINT32 numOfPkts = 0;
	A_UINT32 interval = IPERF_DEFAULT_RUNTIME;
	A_INT32 ret;
	char opt = 0;
	char *param;
	A_UINT32 aidata[4];

	IP6_ADDR_T ip6Address; /* TBD, not used now */
    A_MEMSET(&ip6Address,0,sizeof(IP6_ADDR_T));


	A_INT32 parsed_argc = 1;

	while (parsed_argc < argc) {
		A_UINT32 i = 0;

		if(argv[parsed_argc][i] == '-') {
			i++;
			opt = argv[parsed_argc][i];
			i++;

			if(opt == '\0') {
				/* Garbage parameter */
				parsed_argc++;
				continue;
			}
			if(opt == '-') {
				app_printf("error: please use short parameter notation only\n"); /* TBD, support for long parameter notation */
				return -1;
			}
		}

		/* opt should have the option now, continue if 0.
		 * Useful when parameter is not attached to option,
		 * i.e. -i1 comparing to -i 1
		 */
		if(opt == 0) {
			/* Garbage parameter */
			parsed_argc++;
			continue;
		}

		if(argv[parsed_argc][i] == '\0') {
			/* Wait for parameter option in next argv */
			param = NULL;
		} else {
			param = &argv[parsed_argc][i];
		}

		switch(opt) {
		case 'u':
			protocol = TEST_PROT_UDP;
			pktSize = IPERF_MAX_PACKET_SIZE_UDP;
			break;

		case 's':
			operation_mode = IPERF_SERVER;
			break;

		case 'c':
			if(!param) {
				parsed_argc++;
				continue;
			}

			operation_mode = IPERF_CLIENT;
		    ret = swat_sscanf(param, "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
		    if (ret < 0) {
		    	app_printf("error: invalid IP address\n");
		       return -1;
		    }
		    ipAddress = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
		     /* Check Multicast IP Or Local IP */
		     if ((aidata[0] & 0xf0) == 0xE0) //224.xxx.xxx.xxx - 239.xxx.xxx.xxx
		     {
			  mcastEnabled =1;
		     }
		    break;

		case 'p':
			if(!param) {
				parsed_argc++;
				continue;
			}

			port = swat_atoi(param);
			if(port > 64*1024) {
				app_printf("error: invalid port\n");
				return -1;
			}
			break;

		case 'i':
			if(!param) {
				parsed_argc++;
				continue;
			}

			interval = swat_atoi(param);
			break;

		case 'l':
			if(!param) {
				parsed_argc++;
				continue;
			}

			pktSize = swat_atoi(param);
			if(pktSize > IPERF_MAX_PACKET_SIZE_UDP) /* Limitation in Ruby stack */
				pktSize = IPERF_MAX_PACKET_SIZE_UDP;
			break;

		case 't':
			if(!param) {
				parsed_argc++;
				continue;
			}

			seconds = swat_atoi(param);
			mode = 0;
			break;

		case 'n':
			if(!param) {
				parsed_argc++;
				continue;
			}

			numOfPkts = 1 + swat_atoi(param) / pktSize;
			mode = 1;
			break;
		case 'b':
			if(!param) {
				parsed_argc++;
				continue;
			}

			udpRate = byte_atoi(param);
			break;
	       case 'B':
			if(!param) {
				parsed_argc++;
				continue;
			}

			ret = swat_sscanf(param, "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
			if (ret < 0) {
			    app_printf("error: invalid IP address\n");
			    return -1;
			}
			bindAddress = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
			/* Check Multicast IP Or Local IP */
			if ((aidata[0] & 0xf0) == 0xE0) //224.xxx.xxx.xxx - 239.xxx.xxx.xxx
			{
			    mcastEnabled =1;
			}
			break;

		case 'h':
			app_printf("Usage: iperf [-s|-c host] [options]\n");
			app_printf("       iperf [-h] [-v]\n");
			return 0;

		case 'v':
			app_printf("iperf demo v0.1 for QCA401x\n");
			return 0;

		default:
			/* Silently ignore */
			break;
		}

		opt = 0;
		parsed_argc++;
	}

	if (operation_mode == IPERF_CLIENT && ipAddress) {
	    if ((protocol == TEST_PROT_UDP) && mcastEnabled)
		swat_bench_tx_test(ipAddress, ip6Address, port, protocol,
				   0, pktSize, mode, seconds, numOfPkts, bindAddress, 0, 0, 1 /*iperf mode*/, interval, udpRate);
	    else
		swat_bench_tx_test(ipAddress, ip6Address, port, protocol,
			0, pktSize, mode, seconds, numOfPkts, 0, 0, 0, 1 /*iperf mode*/, interval, udpRate);	        
	} else if (operation_mode == IPERF_SERVER) {
	    if (mcastEnabled)
		swat_bench_rx_test(protocol, 0, port, 0/*localIp*/, bindAddress /*mcastIp*/,
				NULL, NULL, 0, 0, 1 /*iperf mode*/, interval);
	    else
		swat_bench_rx_test(protocol, 0, port, 0/*localIp*/, 0/*mcastIp*/,
				NULL, NULL, 0, 0, 1 /*iperf mode*/, interval);	        
	} else {
		app_printf("Usage: iperf [-s|-c host] [options]\n");
		app_printf("Try `iperf -h` for more information.\n");
		return -1;
	}
}

A_INT32
swat_benchtx_handle(A_INT32 argc, A_CHAR * argv[])
{
    A_INT32 ret = 0;
    A_UINT32 aidata[4] = {0};
    A_UINT32 ipAddress = 0;
    A_UINT32 local_ipAddress = 0;
    A_UINT32 port = 0;
    A_UINT32 pktSize = 0;
    A_UINT32 maxPktSize = MAX_IPV4_PKT_PAYLOAD_LEN;
    A_UINT32 seconds = 0;
    A_UINT32 numOfPkts = 0;
    A_UINT32 mode = 0;
    A_UINT32 protocol = 0;
    A_UINT32 delay = 0;
    A_UINT32 ip_hdr_incl = 0;
    IP6_ADDR_T ip6Address;
    A_UINT32 ssl_inst_index = 0;
#ifdef SWAT_SSL   
    SSL_INST *ssl;
#endif

    if (((void *) 0 == argv) || (argc < 8)) {
        goto ERROR;
    }

    A_MEMSET(&ip6Address,0,sizeof(IP6_ADDR_T));
    /* IP Address */
    if(v6_enabled){
        Inet6Pton(argv[1], &ip6Address);
    }else{
    ret = swat_sscanf(argv[1], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
    if (ret < 0) {
       goto ERROR;
    }

    }
    ipAddress = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];

    /* Port */
    port = swat_atoi(argv[2]);

    /* TCP or UDP or raw */
    if ((0 != swat_strcmp(argv[3], "tcp"))
        && (0 != swat_strcmp(argv[3], "udp")) && (0 != swat_strcmp(argv[3],"raw"))
        &&(0 != swat_strcmp(argv[3], "ssl"))&& (0 != swat_strcmp(argv[3], "dtls")))
    {
       goto ERROR;
    }
    ret = swat_strcmp(argv[3], "tcp");
    if (0 == ret) {
        protocol = 0;
    }
    ret = swat_strcmp(argv[3], "udp");
    if (0 == ret) {
	   if(argc == 9){
            /* IP Address */
            ret = swat_sscanf(argv[8], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
            if (ret < 0)
            {
                goto ERROR;
            }
            local_ipAddress = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
        }
        protocol = 1;
    }
    ret = swat_strcmp(argv[3], "dtls");
    if (0 == ret) {
	   if(argc == 9){
            /* IP Address */
            ret = swat_sscanf(argv[8], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
            if (ret < 0)
            {
                goto ERROR;
            }
            local_ipAddress = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
        }
        protocol = 1;
    }
#if SWAT_BENCH_RAW
    ret = swat_strcmp(argv[3], "raw");
    if (0 == ret)
    {
        if(argc >= 9){
            /* IP Address */
            ret = swat_sscanf(argv[8], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
            if (ret < 0)
            {
                goto ERROR;
            }
            local_ipAddress = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
        }
        protocol = 2;

        if((port == 6) || (port == 17) || (port < 0) || (port > 255))
        {
           goto ERROR;
        }

    }

    if(argc == 10)
    {
    ret = swat_strcmp(argv[9],"ip_hdr_inc");
    if(0 == ret)
        {
               ip_hdr_incl = 1;
        }
    else{
               goto ERROR;
        }
    }
#endif /*SWAT_BENCH_RAW*/
#ifdef SWAT_SSL
    if((swat_strcmp(argv[3], "ssl") == 0) || (swat_strcmp(argv[3], "dtls") == 0)){
       if (swat_strcmp(argv[3], "ssl") == 0){
           protocol = TEST_PROT_SSL;
       } else {
           protocol = TEST_PROT_DTLS;           
       }
       if((ip_hdr_incl) || (argc < 9)){
          printf(" SSL index missing\r\n");
          goto ERROR;
       }
       ssl_inst_index = swat_atoi(argv[8]);
       if(NULL == (ssl =swat_find_ssl_inst(ssl_inst_index))){
          printf(" Invalid SSL index %d: No SSL ctx found\r\n", ssl_inst_index);
          goto ERROR;
       } else if (ssl->state != SSL_FREE) {
          printf("Error: One SSL connection is running with SSL instance %d \r\n", ssl_inst_index);          
          return -1;
       }

    }
#endif

    /* Packet Size */
    pktSize = swat_atoi(argv[4]);

    if (v6_enabled == FALSE){
        if ((protocol == TEST_PROT_DTLS) || (protocol == TEST_PROT_SSL)){
            maxPktSize = MAX_IPV4_SSL_PKT_PAYLOAD_LEN;
        }    
    } else if (v6_enabled == TRUE){
        if ((protocol == TEST_PROT_DTLS) || (protocol == TEST_PROT_SSL)){
            maxPktSize = MAX_IPV6_SSL_PKT_PAYLOAD_LEN;
        }         
    }

    if ((0 == pktSize) || (pktSize > maxPktSize)) {
       goto ERROR;
    }

    /* Test Mode */
    mode = swat_atoi(argv[5]);
    if ((0 != mode)
        && (1 != mode)) {
        goto ERROR;
    }
    if (0 == mode) {
        seconds = swat_atoi(argv[6]);
        if (0 == seconds) {
            goto ERROR;
        }
    }
    if (1 == mode) {
        numOfPkts = swat_atoi(argv[6]);
        if (0 == numOfPkts) {
            goto ERROR;
        }
    }

    /* Do nothing */
    delay = swat_atoi(argv[7]);
    swat_bench_tx_test(ipAddress, ip6Address, port, protocol, ssl_inst_index, pktSize, mode, seconds, numOfPkts, local_ipAddress, ip_hdr_incl, delay, 0, 0, 0);

    return 0;
ERROR:
        SWAT_PTF("Invalid/missing param\n");
        SWAT_PTF("benchtx <Tx IP> <port> <protocol tcp/udp/ssl> <size> <test mode> <packets|time> <delay> <ssl index>\r\n");
        SWAT_PTF("Ex: benchtx 192.168.1.100 6000 tcp 64 1 1000 0\r\n");
        SWAT_PTF("Ex: benchtx 225.0.0.1 7000 udp 1200 1 1000 0 192.168.1.100\r\n");
        SWAT_PTF("Parameters:\r\n");
        SWAT_PTF("1 <Tx IP>       : IP address (v4 or v6)\r\n");
        SWAT_PTF("2 <port>        : Listening Port for tcp/udp\r\n");
        SWAT_PTF("3 <protocol>    : tcp/udp/ssl/dtls\r\n");
        SWAT_PTF("4 <size>        : Packet size in bytes.  ((ssl or dtls)+ipv6 <=1120, ssl or dtls <=1350, others<=1400)\r\n");
        SWAT_PTF("5 <test mode>   : 0:Time 1:Packets\r\n");
        SWAT_PTF("6 <packets|time>: Seconds or Packets number\r\n");
        SWAT_PTF("7 <delay>       : Always 0\r\n");
        SWAT_PTF("8 <ssl index>   : SSL Index\r\n");
#if SWAT_BENCH_RAW
        SWAT_PTF("benchtx <Tx IP> <prot> <raw> <msg size> <test mode> <number of packets | time (sec)> <delay in msec> <local IP> [ip_hdr_inc*]\r\n");
#endif
        return -1;
}

A_INT32
swat_benchrx_handle(A_INT32 argc, A_CHAR * argv[])
{
    A_INT32 ret = 0;
    A_UINT32 aidata[4];
    A_UINT32 ipAddress = 0;
    A_UINT32 localIp = 0;
    A_UINT32 mcastIp = 0;
    A_UINT32 port = 0;
    A_UINT32 protocol = 0;
    A_UINT32 ip_hdr_inc = 0;
    A_UINT32 rawmode = 0;
    IP6_ADDR_T localIp6;
    IP6_ADDR_T mcastIp6;
    A_UINT8 mcastEnabled = 0;
    A_UINT32 ssl_inst_index = 0;

    if (((void *) 0 == argv) || (argc > 5) || (argc < 3)) {
        goto ERROR;
    }

    A_MEMSET(&localIp6,0,sizeof(IP6_ADDR_T));
    A_MEMSET(&mcastIp6,0,sizeof(IP6_ADDR_T));

    /* TCP or UDP */
    if ((0 != swat_strcmp(argv[1], "tcp"))
        && (0 != swat_strcmp(argv[1], "udp"))
        && (0 != swat_strcmp(argv[1], "raw"))
        && (0 != swat_strcmp(argv[1], "ssl"))
        && (0 != swat_strcmp(argv[1], "dtls")))
    {
        goto ERROR;
    }
    ret = swat_strcmp(argv[1], "tcp");
    if (0 == ret) {
        protocol = 0;
    }
    ret = swat_strcmp(argv[1], "udp");
    if (0 == ret) {
        protocol = 1;
    }

#if SWAT_BENCH_RAW
    ret = swat_strcmp(argv[1], "raw");
    if (0 == ret)
    {
        protocol = 2;
        ret = swat_strcmp(argv[2],"eapol");
        if(ret == 0)
                rawmode = ETH_RAW;
        else
                rawmode = IP_RAW;
    }
#endif
#ifdef SWAT_SSL
    if(swat_strcmp("ssl", argv[1]) == 0)
    {
        protocol = TEST_PROT_SSL;
    }

    if(swat_strcmp("dtls", argv[1]) == 0)
    {
        protocol = TEST_PROT_DTLS;
    }
#endif
    /* Port */
    if(rawmode == ETH_RAW)
    {
        port = ATH_ETH_P_PAE;
    }
	else
	{
    	port = swat_atoi(argv[2]);	

        if((rawmode == IP_RAW) && ((port < 0) || (port > 255)))
        {
           goto ERROR;
        }
	}

    /* IP Check */
    if (4 == argc && (TEST_PROT_SSL != protocol)) {
        if(strcasecmp("ip_hdr_inc",argv[3]) == 0)
		{
		    ip_hdr_inc = 1;
		}else{
            ret = swat_sscanf(argv[3], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
            if (ret < 0) {
                goto ERROR;
            }
            ipAddress = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];

            /* Check Multicast IP Or Local IP */
            if ((aidata[0] & 0xf0) == 0xE0) //224.xxx.xxx.xxx - 239.xxx.xxx.xxx
            {
                mcastIp = ipAddress;
            } else {
                localIp = ipAddress;
            }
        }
    }

    if (5 <= argc && (TEST_PROT_SSL != protocol))
    {
        ret = swat_strcmp(argv[4],"ip_hdr_inc");
        if(ret == 0)
        {
#if SWAT_BENCH_RAW
            ip_hdr_inc = 1;
            ret = swat_sscanf(argv[3], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
            if (ret < 0)
            {
                goto ERROR;
            }
            localIp = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
#endif
         }else{
#if SWAT_BENCH_RAW
            if(argc == 6){
                ret = swat_strcmp(argv[5],"ip_hdr_inc");
                if(ret == 0)
                {
                    ip_hdr_inc = 1;
                }
                else{
                    goto ERROR;
                }
            }
#endif
            if(v6_enabled){
                Inet6Pton(argv[3], &mcastIp6);
                Inet6Pton(argv[4], &localIp6);
            }else{
            ret = swat_sscanf(argv[3], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
            if (ret < 0)
            {
                   goto ERROR;
            }
            mcastIp = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];

            if((aidata[0] & 0xf0) != 0xE0)
            {
                  goto ERROR;
            }

            ret = swat_sscanf(argv[4], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
            if (ret < 0)
            {
                  goto ERROR;
            }
            localIp = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
        }
            mcastEnabled = 1;
        }
    }/* argc >= 5 */
#ifdef SWAT_SSL
    if(((TEST_PROT_SSL == protocol)  || (TEST_PROT_DTLS == protocol))&& !ip_hdr_inc)
    {
	if(argc == 4)
	{    
           ssl_inst_index = swat_atoi(argv[3]);
           if(NULL == swat_find_ssl_inst(ssl_inst_index))
           {
              printf(" Invalid SSL index %d: No SSL ctx found\r\n", ssl_inst_index);
              goto ERROR;
           }
       }    
       else
       {
           printf(" Invalid SSL index\r\n");
           goto ERROR;
       }
    }
#endif


    if(mcastEnabled)
        swat_bench_rx_test(protocol, ssl_inst_index, port, localIp, mcastIp, &localIp6, &mcastIp6, ip_hdr_inc, rawmode, 0, 0);
    else
        swat_bench_rx_test(protocol, ssl_inst_index, port, localIp, mcastIp, NULL, NULL, ip_hdr_inc, rawmode, 0, 0);

    return 0;
ERROR:
    SWAT_PTF("Incorrect/missing param\n");
    SWAT_PTF("benchrx <protocol> <port> <multicast IP*> <local IP*> <SSL index>\r\n");
    SWAT_PTF("Ex: benchrx tcp 6000 192.168.1.100 \r\n");
    SWAT_PTF("Parameters:\r\n");
    SWAT_PTF("1 <protocol>    : tcp/udp/ssl\r\n");
    SWAT_PTF("2 <port>        : Listening Port\r\n");
    SWAT_PTF("3 <multicast IP>: Multicast IP Address\r\n");
    SWAT_PTF("4 <local IP>    : Local IP Address\r\n");
    SWAT_PTF("5 <ssl index>   : ssl_index\r\n");
#if SWAT_BENCH_RAW
    SWAT_PTF("benchrx <protocol> <port> <multicast IP*> <local IP*> [ip_hdr_inc]\r\n");
#endif /*SWAT_BENCH_RAW*/

    return -1;
}

A_INT32
swat_benchquit_handle(A_INT32 argc, A_CHAR * argv[])
{
    if (argc > 1) {
        SWAT_PTF("input benchquit\n");
        return -1;
    }

    swat_bench_quit_config();

#if 0
    qcom_msleep(1000);

    extern void qcom_task_kill_all(void);
    qcom_task_kill_all();

    extern void swat_socket_close_all(void);
    swat_socket_close_all();
#endif

    return 0;
}

A_INT32
swat_benchdbg_handle(A_INT32 argc, A_CHAR * argv[])
{

    /* swat_bench_dbg(); */
    return 0;
}

#endif

#if defined(SWAT_PING) || defined(SWAT_SSL)
extern A_INT32 isdigit(A_INT32 c);

A_INT32
inet_aton(const A_CHAR *name,
          /* [IN] dotted decimal IP address */
          A_UINT32 * ipaddr_ptr
          /* [OUT] binary IP address */
    )
{                               /* Body */

    A_INT8 ipok = FALSE;

    A_UINT32 dots;

    A_UINT32 byte;

    A_UINT32 addr;

    addr = 0;

    dots = 0;

    for (;;) {

        if (!isdigit(*name))
            break;

        byte = 0;

        while (isdigit(*name)) {

            byte *= 10;

            byte += *name - '0';

            if (byte > 255)
                break;

            name++;

        }                       /* Endwhile */

        if (byte > 255)
            break;

        addr <<= 8;

        addr += byte;

        if (*name == '.') {

            dots++;

            if (dots > 3)
                break;

            name++;

            continue;

        }



      if ((*name == '\0') && (dots == 3)) {

         ipok = TRUE;

      } /* Endif */



      break;



   } /* Endfor */



   if (!ipok) {

      return 0;

   } /* Endif */



   if (ipaddr_ptr) {

      *ipaddr_ptr = addr;

   } /* Endif */



    return -1;

}                               /* Endbody */
#endif

#if defined(SWAT_PING)
A_INT32
swat_ping_handle(A_INT32 argc, A_CHAR *argv[], A_CHAR v6)
{
    A_UINT32 hostaddr;
    A_UINT8 host6addr[16];
    A_INT32 error;
    A_UINT32 count, size, interval;
    A_UINT32 i;

    if (argc < 2) {
        SWAT_PTF("Usage: %s <host_ip> [ -c <count> -s <size> -i <interval> ]\n", argv[0]);

        return -1;
    }

    if(v6){
        error = Inet6Pton(argv[1], &host6addr);
    }else{
    error = inet_aton(argv[1], &hostaddr);
    }

    if (!v6 && error != -1) {
        if (strlen(argv[1]) > 32) {
            SWAT_PTF("host name cannot be more then 32 Bytes\n");

            return -1;
        } else {
#if !defined (REV74_TEST_ENV2)
            if (A_OK != qcom_dnsc_get_host_by_name(argv[1], &hostaddr) ) {
                SWAT_PTF("Can't get IP addr by host name %s\n", argv[1]);
                return -1;
            } else {
                SWAT_PTF("Get IP addr %s by host name %s\n", (char *)_inet_ntoa(hostaddr), argv[1]);
            }
#endif
        }
    }

    if (v6 && error !=0 ) {
        if (strlen(argv[1]) > 32) {
            SWAT_PTF("host name cannot be more then 32 Bytes\n");

            return -1;
        } else {
#if !defined (REV74_TEST_ENV2)
	  if (A_OK != qcom_dnsc_get_host_by_name2(argv[1], (A_UINT32*)(host6addr), AF_INET6, 2)) {
                SWAT_PTF("Can't get IPv6 addr by host name %s\n", argv[1]);
                return -1;
            } else {
	        A_UINT8 ip6_str[48];
	        inet6_ntoa((char *)&host6addr,(char *)ip6_str);
                SWAT_PTF("Get IPv6 addr %s by host name %s\n", ip6_str, argv[1]);
            }
#endif
        }
    }

    count = 1;
    size = 64;
    interval = 0;

    if (argc > 2 || argc <= 8) {
        for (i = 2; i < argc; i += 2) {
            if (!A_STRCMP(argv[i], "-c")) {
                if ((i + 1) == argc) {
                    SWAT_PTF("Missing parameter\n");
                    return -1;
                }
                A_SSCANF(argv[i + 1], "%u", &count);
            } else if (!A_STRCMP(argv[i], "-s")) {
                if ((i + 1) == argc) {
                    SWAT_PTF("Missing parameter\n");
                    return -1;
                }
                A_SSCANF(argv[i + 1], "%u", &size);
            } else if (!A_STRCMP(argv[i], "-i")) {
                if ((i + 1) == argc) {
                    SWAT_PTF("Missing parameter\n");
                    return -1;
                }
                A_SSCANF(argv[i + 1], "%u", &interval);
            }
        }
    } else {
        SWAT_PTF("Usage: %s <host> [ -c <count> -s <size> -i <interval> ] \n", argv[0]);

        return -1;
    }

    if (size > 7000) {          /*CFG_PACKET_SIZE_MAX_TX */
        SWAT_PTF("Error: Invalid Parameter %s \n", argv[5]);
        return -1;
    }
 
    for (i = 0; i < count; i++) {
        if(v6){
            if(qcom_ping6(host6addr, size) == A_OK){
                A_UINT8 ip6_str[48];
                inet6_ntoa((char *)&host6addr,(char *)ip6_str);
                SWAT_PTF("Ping reply from %s : time<1ms\r\n",ip6_str);
            }else{
                SWAT_PTF("Request timed out\r\n");
            }
        }else{
            if(qcom_ping(hostaddr, size) == A_OK){
                SWAT_PTF("Ping reply from %d.%d.%d.%d: time<1ms\r\n",
                    hostaddr >> 24 & 0xFF, hostaddr >> 16 & 0xFF, hostaddr >> 8 & 0xFF, hostaddr & 0xFF);

            }else{
                SWAT_PTF("Request timed out\r\n");
            }
        }
        if ((count > 0) && (i < (count - 1))) {
            qcom_thread_msleep(interval); /*Sleep to wait for Reply packets */
        }

    }
    return 0;
}

#endif


A_INT32 swat_ip6_set_router_prefix(A_INT32 argc,char *argv[])
{

	unsigned char  v6addr[16];
	A_INT32 retval=-1;
	int prefixlen = 0;
    int prefix_lifetime = 0;
    int valid_lifetime = 0;
    A_UINT32 wifiMode;

	if(argc < 6)
	{
		printf("incomplete params\n");
		return A_ERROR;
	}

    qcom_op_get_mode(currentDeviceId, &wifiMode);
    if ((wifiMode != QCOM_WLAN_DEV_MODE_AP) && (wifiMode != QCOM_WLAN_DEV_MODE_ADHOC))
    {
		printf("ipv6 rt prfx support not possible in station \n");
		return A_ERROR;
	}

    retval = Inet6Pton(argv[2],v6addr);
    if(retval == 1)
	{
           printf("Invalid ipv6 prefix \n");
           return (A_ERROR);
	}
	prefixlen =  atoi(argv[3]);
    prefix_lifetime = atoi(argv[4]);
    valid_lifetime =  atoi(argv[5]);
    qcom_ip6config_router_prefix(currentDeviceId, v6addr,prefixlen,prefix_lifetime,valid_lifetime);
    return 0;
}

#endif
void
swat_time()
{
		  #if 0
          cmnos_printf("[%dh%dm%ds:]", (swat_time_get_ms() / 1000) / 3600,
                 ((swat_time_get_ms() / 1000) % 3600) / 60, (swat_time_get_ms() / 1000) % 60);
		  #endif
}


A_INT32
swat_http_server(A_INT32 argc, A_CHAR * argv[])
{
    A_INT32 enable;
    void *ssl_ctx = NULL;

    if(argc < 3)
    {
       SWAT_PTF("Incomplete params\n");
       return A_ERROR;
    }
    if(strcmp(argv[1], "--ip_http_server") == 0){
    if(strcmp(argv[2], "start") == 0)
    {
        enable = 1;
    }
    else if(strcmp(argv[2], "stop") == 0)
    {
        enable = 0;
    }
    else
    {
        SWAT_PTF("Supported commands: start/stop");
        return (A_ERROR);
    }
    }
    else if(strcmp(argv[1], "--ip_https_server") == 0){
        A_UINT32 ssl_inst_index;
        SSL_INST *ssl;
        if(argc < 4)
        {
           SWAT_PTF("SSL index missing");
           return (A_ERROR);
        }
        if(strcmp(argv[2], "start") == 0)
        {
            enable = 3;
        }
        else if(strcmp(argv[2], "stop") == 0)
        {
            enable = 2;
        }
        else
        {
            SWAT_PTF("Supported commands: start/stop");
            return (A_ERROR);
        }
        ssl_inst_index = swat_atoi(argv[3]);
        if(NULL == (ssl = swat_find_ssl_inst(ssl_inst_index))){
            return A_ERROR;
        }

        if (ssl->sslCtx == NULL || SSL_SERVER != ssl->role)
        {
            printf("ERROR: SSL %s not started\n", argv[2]);
            return A_ERROR;
        }

        ssl_ctx = (void *)ssl->sslCtx; 
    }

    swat_wmiconfig_http_server(currentDeviceId, enable, ssl_ctx);
}

A_INT32 swat_http_server_post(A_INT32 argc,char *argv[])
{
#if defined HTTP_ENABLED
    A_INT32 command = HTTP_POST_METHOD; /* Command 1 for HTTP server post */
#endif
    A_INT32 objtype, objlen;

    if(argc < 7)
    {
       SWAT_PTF("Incomplete params\n");
       return A_ERROR;
    }

    objtype = atoi(argv[4]);
    objlen = atoi(argv[5]);
    if((objtype != 1) && (objtype != 2) && (objtype != 3))
    {
        SWAT_PTF("Error: <objtype> can be 1 ,2 or 3\n");
        return A_ERROR;
    }

#if defined HTTP_ENABLED
    if (A_OK != qcom_http_server_method(command, (A_UINT8*)argv[2], (A_UINT8*)argv[3], objtype, objlen, (A_UINT8*)argv[6]))
    {
        SWAT_PTF("Error! HTTP POST failed\n");
    }
#endif
    return A_OK;
}

A_INT32 
swat_http_server_get(A_INT32 argc,char *argv[])
{
#if defined HTTP_ENABLED
    A_INT32 command = HTTP_GET_METHOD; /* Command 0 for HTTP server get */
#endif
    A_INT32 objtype, objlen;
    A_UINT8 *value;

    if(argc < 4)
    {
       SWAT_PTF("Incomplete params\n");
       return A_ERROR;
    }
    objtype = 0;
    objlen = 0;
    value = swat_mem_malloc(1500);
    if (!value)
        return A_ERROR;

#if defined HTTP_ENABLED
    if (A_OK != qcom_http_server_method(command, (A_UINT8*)argv[2], (A_UINT8*)argv[3], objtype, objlen, value))
    {
        SWAT_PTF("Error! HTTP GET failed\n");
    }
    else
    {
        SWAT_PTF("Value is %s\n", value);
    }
#endif
    swat_mem_free(value);
    return A_OK;
}

A_INT32 
swat_http_server_put(A_INT32 argc,char *argv[])
{
#if defined HTTP_ENABLED
    A_INT32 command = HTTP_PUT_METHOD; 
#endif
	A_INT32 objtype, objlen;

	if(argc < 7)
	{
	   SWAT_PTF("Incomplete params\n");
	   return A_ERROR;
	}

	objtype = atoi(argv[4]);
	objlen = atoi(argv[5]);
	if((objtype != 1) && (objtype != 2) && (objtype != 3))
	{
		SWAT_PTF("Error: <objtype> can be 1 ,2 or 3\n");
		return A_ERROR;
	}
	
#if defined HTTP_ENABLED
	if (A_OK != qcom_http_server_method(command, (A_UINT8*)argv[2], (A_UINT8*)argv[3], objtype, objlen, (A_UINT8*)argv[6]))
	{
		SWAT_PTF("Error! HTTP PUT failed\n");
	}
#endif
	return A_OK;
}

A_INT32 
swat_http_server_delete(A_INT32 argc,char *argv[])
{
#if defined HTTP_ENABLED
    A_INT32 command = HTTP_DELETE_METHOD; /* Command 0 for HTTP server get */

#endif
    if(argc < 4)
    {
       SWAT_PTF("Incomplete params\n");
       return A_ERROR;
    }

#if defined HTTP_ENABLED
    if (A_OK != qcom_http_server_method(command, (A_UINT8*)argv[2], (A_UINT8*)argv[3], 0, 0, NULL))
    {
        SWAT_PTF("Error! HTTP DELETE failed\n");
    }
#endif
	return A_OK;
}

A_INT32
swat_http_set_redirected_url(A_INT32 argc, A_CHAR *argv[])
{
    if(argc != 3)
    {
        SWAT_PTF("Invalid command\n");
        return A_ERROR;
    }
    if(strlen(argv[2]) >= 256)
    {
        SWAT_PTF("URL length must be no more than 256 bytes!\n");
        return A_ERROR;
    }
    
    qcom_http_set_redirected_url(currentDeviceId, argv[2]);
    return A_OK;
}

A_INT32
swat_http_redirect_unknown_url(A_INT32 argc, A_CHAR *argv[])
{
    if(argc != 3)
    {
        SWAT_PTF("Invalid command\n");
        return A_ERROR;
    }
    A_INT32 enable = 0;
    if(strcmp(argv[2], "enable") == 0)
    {
        enable = 1;
    }
    else if(strcmp(argv[2], "disable") == 0)
    {
        enable = 0;
    }
    else
    {
        SWAT_PTF("--ip_http_redirect_unknown_url enable/disable\n");
        return SWAT_ERROR;
    }
    qcom_http_redirect_unknown_url_enable(currentDeviceId, enable);
    return A_OK;
}

A_INT32
swat_http_server_add_redirected_page(A_INT32 argc,char *argv[])
{
    A_UINT16 data_len = 0;
    A_UINT8 *header_data = NULL;
    A_UINT16 header_len = 0;
    
    if(argc != 3)
    {
        SWAT_PTF("Invalid command\n");
        return A_ERROR;
    }
    if(strlen(argv[2]) >= 256)
    {
        SWAT_PTF("URL length must be no more than 256 bytes!\n");
        return A_ERROR;
    }
    data_len = strlen(test_html[0]);
    
    if (swat_http_server_header_form(currentDeviceId, &header_data, &header_len) == A_OK)
    {
        qcom_http_server_add_redirected_page(header_len, header_data, data_len, test_html[0], argv[2]);
    }

    return A_OK;
}

A_INT32
swat_http_restrict_request(A_INT32 argc,char *argv[])
{
    if(argc != 3)
    {
        SWAT_PTF("Invalid command\n");
        return A_ERROR;
    }
    A_INT32 enable = 0;
    if(strcmp(argv[2], "enable") == 0)
    {
        enable = 1;
    }
    else if(strcmp(argv[2], "disable") == 0)
    {
        enable = 0;
    }
    else
    {
        SWAT_PTF("--ip_http_request_restriction enable/disable\n");
        return SWAT_ERROR;
    }
    qcom_restrict_http_request(currentDeviceId, enable);
    
    return A_OK;
}

HTTPC_PARAMS      httpc; // Since stack size is less

typedef struct http_rsp_cont{
    struct http_rsp_cont * next;
    A_UINT32    flag;
    A_UINT32    totalLen;
	A_UINT32    length;
	A_UINT8     data[1];
}HTTP_RSP_CONT;

#define HTTP_CLIENT_MAX_NUM (4)
struct swat_http_client_s
{
	A_INT32 client;
	A_UINT32 num;
	A_BOOL cb_enable;
	A_UINT32 total_len;
} http_client[HTTP_CLIENT_MAX_NUM] = {{-1, 0, FALSE, 0}, {-1, 0, FALSE, 0}, {-1, 0, FALSE, 0}, {-1, 0, FALSE, 0}};

void http_client_cb_demo(void* arg, A_INT32 state, void* http_resp)
{
	static A_UINT32 total_size = 0;
	(void) arg;
	HTTPC_RESPONSE* temp = (HTTPC_RESPONSE *)http_resp;
	struct swat_http_client_s* hc = (struct swat_http_client_s *)arg;
	A_UINT32* total_len = &total_size;

	if (arg)
	{
		total_len = &hc->total_len;
	}

	if (state >= 0)
	{
		A_INT32 resp_code = temp->resp_code;
		
		if(temp->more)
		{
			if (temp->length != 4)
			{
				SWAT_PTF("%s: rsp length is not matched with the requirement!\n", __func__);
			}

			HTTP_RSP_CONT **httcRspHead = (HTTP_RSP_CONT **)(*(A_UINT32*)(temp->data));
			HTTP_RSP_CONT *httcRsp = *httcRspHead;
			HTTP_RSP_CONT *prev, *pRsp;
			A_UINT32 temp_len = 0;

			prev = httcRsp;

			if (!prev)
			{
				SWAT_PTF("NO data error\n");
				return;
			}
			
			while (prev)
			{
				pRsp = prev->next;

				if(prev->data)
				{
					temp_len += prev->length;
				}

				prev = pRsp;
			}
			
			if(httcRsp->totalLen != temp_len)
			{
				SWAT_PTF("%s: data is corrupt, length not match!\n", __func__);
			}
			else
			{
				*total_len += httcRsp->totalLen;
			}

			prev = httcRsp;
			while (prev)
			{
				pRsp = prev->next;

				if(prev->data)
				{
					SWAT_PTF_NO_TIME("%s", prev->data);
				}
				
				free (prev);
				
				prev = pRsp;
			}
			
			*httcRspHead = NULL;	
		}
		else
		{
            if (temp->length)
            {
				SWAT_PTF_NO_TIME("%s", temp->data);

				*total_len += temp->length;
            }
		}
		
		if (state == 0 && *total_len)
		{
			SWAT_PTF("=========> http client Received: total size %d, Resp_code %d\n", *total_len, resp_code);
			*total_len = 0; // Finished
		}
	}
	else
	{
		SWAT_PTF("HTTP Client Receive error: %d\n", state);
		*total_len = 0;

		// Should close http connection
	}
}

void httpc_method(HTTPC_PARAMS *p_httpc)
{
    A_UINT32  error = A_OK;
    A_UINT8* value = swat_mem_malloc(1500); //TBD:change the number to micro

    if (!value) {
        SWAT_PTF("No memory for page\n");
        return;
    }

	/*
	 *  For HTTP with SSL case we are passing the SSL context with data
	 * instead of NULL so as to differentiate between HTTPS and HTTP in the lower layers.
	 * For normal HTTP, SSL context will be NULL 
	 */		
	if (ssl_flag_for_https == 1)
	{
		httpc.ssl_ctx = httpc.data;
	}

#if defined HTTP_ENABLED
    error = qcom_http_client_method(httpc.command, httpc.url, httpc.data, value, httpc.ssl_ctx);
#endif
    if(error != A_OK){
        SWAT_PTF("HTTPC Command failed\n");
    }
	else
    {
        /* For GET and POST alone, print the output */
        if ((p_httpc->command == 1) || (p_httpc->command == 2) || (p_httpc->command == 9) || (p_httpc->command == 10))
        {
            http_client_cb_demo(NULL, 0, value);
        }
    }

    swat_mem_free(value);
    return;
}


A_CHAR * swat_build_httpc_body(A_UINT32 len)
{
	A_CHAR * body = NULL;
    A_INT32 i;

    body = qcom_mem_alloc(len+1);
    if (body) {
        for (i=0; i<len; i++) {
            *(body + i) = 'A' + i % 26;
        }
        *(body + len) = '\0';
     } else {
         SWAT_PTF("malloc failed\n");
     }
	 
     return body;
}

A_INT32 httpc_command_parser(A_INT32 argc, A_CHAR* argv[] )
{
    A_INT32           return_code = A_OK;

    if (argc < 3)
    {
        /*Incorrect number of params, exit*/
        return_code = A_ERROR;
    }
    else
    {
        memset((void *)&httpc, 0, sizeof(HTTPC_PARAMS));

        if (argv[3])
        {
        	if(strlen((char*)argv[3]) >= HTTPCLIENT_MAX_URL_LENGTH)
	        {
	            SWAT_PTF("Maximum %d bytes supported as argument\n", HTTPCLIENT_MAX_URL_LENGTH);
	            return A_ERROR;
	        }
            else
            {
            	strcpy((char*)httpc.url, argv[3]);
            }
        }

        if (argv[4]){
            strcpy((char*)httpc.data, argv[4]);
        }else{
            httpc.data[0] =  '\0';
        }


#if defined HTTP_ENABLED
        if(strcmp(argv[2], "connect") == 0)
        {

			httpc.command = HTTPC_CONNECT_CMD;

            if(argc < 4){
                SWAT_PTF("wmiconfig --ip_http_client connect server_addr\n");
                return A_ERROR;
            }

            if(strlen((char*)argv[3]) >= 64)
            {
               SWAT_PTF("Maximum 64 bytes supported as Connect URL\n");
               return A_ERROR;
            }
            
            if(strncmp(argv[3], "https://", 8) == 0)
   	        {
	            A_UINT32 ssl_inst_index = 0;
	            SSL_INST *ssl = NULL;
		        int size = sizeof(httpc.url) - 8;
		    /*  Setting the flag in HTTPS */
	    	    ssl_flag_for_https = 1;
	            memmove(httpc.url, httpc.url + 8, size);
	            httpc.command = HTTPC_CONNECT_SSL_CMD;

				if(argc < 6){
	             SWAT_PTF("SSL context index is missing for https\n");
	             return A_ERROR;
	            }           
	            ssl_inst_index = swat_atoi(argv[5]);
	            if(NULL == (ssl = swat_find_ssl_inst(ssl_inst_index))){
	                SWAT_PTF("ssl_ctx error\n"); 
	                return A_ERROR;
	            }

	            if (ssl->sslCtx == NULL || SSL_CLIENT != ssl->role)
	            {
	                SWAT_PTF("ssl_ctx error\n"); 
	                return A_ERROR;
	            } 
	            httpc.ssl_ctx = (void *)ssl->sslCtx;
	        }
	        else if(strncmp(argv[3], "http://", 7) == 0)
    	    {
    	        int size = sizeof(httpc.url) - 7;
	        /* Resetting the flag in HTTP case. */
	        ssl_flag_for_https = 0;
    	        memmove(httpc.url, httpc.url + 7, size);
      	    }
            
            if(argc >= 4){
               qcom_http_client_method(httpc.command, httpc.url, httpc.data, NULL, httpc.ssl_ctx);
            }else{
                return_code = A_ERROR;
            }
        }
        else if(strcmp(argv[2], "get") == 0)
        {
            httpc.command = HTTPC_GET_CMD;
            if(argc >= 4){
                httpc_method(&httpc);
            }else
                return_code = A_ERROR;
        }
        else if(strcmp(argv[2], "post") == 0)
        {
            httpc.command = HTTPC_POST_CMD;
            if(argc >= 4)
                httpc_method(&httpc);
            else
                return_code = A_ERROR;
        }

        else if(strcmp(argv[2], "header") == 0)
        {
            httpc.command = HTTPC_HEADER_CMD;
            if(argc >= 5)
                httpc_method(&httpc);
            else
                return_code = A_ERROR;
        }

        else if(strcmp(argv[2], "clearheader") == 0)
        {
            httpc.command = HTTPC_CLEAR_HEADER_CMD;
            httpc_method(&httpc);
        }

        else if(strcmp(argv[2], "body") == 0)
        {
            A_CHAR *body = NULL;
            A_UINT32 len = DEFAULT_HTTPC_BODY_LEN;
			
            if (argc > 3) {
                len = atoi(argv[3]);
            }

            if (len > max_httpc_body_len)
                len = max_httpc_body_len;

            body = swat_build_httpc_body(len);
            if (!body)
                return SWAT_ERROR;

            qcom_http_client_body(HTTPC_BODY_CMD, (A_UINT8 *)body, A_STRLEN(body));

            qcom_mem_free(body);
        }

        else if(strcmp(argv[2], "query") == 0)
        {
            httpc.command = HTTPC_DATA_CMD;
            if(argc >= 5)
                httpc_method(&httpc);
            else
                return_code = A_ERROR;
        }
        else if(strcmp(argv[2], "disc") == 0)
        {
            int res;
            httpc.command = HTTPC_DISCONNECT_CMD;
            res = qcom_http_client_method(httpc.command, httpc.url, httpc.data, NULL, httpc.ssl_ctx);
            if(res == 0)
                SWAT_PTF("Disconnect success\n");
            else
                SWAT_PTF("Disconnect failed\n");
        }
        else if(strcmp(argv[2], "put") == 0)
        {
            httpc.command = HTTPC_PUT_CMD;
            if(argc >= 4){
                httpc_method(&httpc);
            }else
                return_code = A_ERROR;
        }
        else if(strcmp(argv[2], "patch") == 0)
        {
            httpc.command = HTTPC_PATCH_CMD;
            if(argc >= 4)
                httpc_method(&httpc);
            else
                return_code = A_ERROR;
        }
        else if(strcmp(argv[2], "reg_callback") == 0)
        {
			if (argc < 4 || strcmp(argv[3], "enable") != 0)
			{
            	qcom_http_client_register_cb(NULL, NULL);
			}
			else
			{
            	qcom_http_client_register_cb(http_client_cb_demo, NULL);
			}
        }
        else
        {
            SWAT_PTF("Unknown Command \"%s\"\n", argv[2]);
            return_code = A_ERROR;
        }
#endif
    }
    if (return_code == A_ERROR)
    {
        SWAT_PTF ("USAGE: wmiconfig --ip_http_client [<connect/get/post/query/disc/put/patch> <data1> <data2>]\n");
    }
    return return_code;
} /* Endbody */

// wmiconfig --ip_httpclient2_connect <server> <port> <timeout> [<ssl_ctx_index>] [<callback_enable>]
// wmiconfig --ip_httpclient2_disconnect <client_num>
// wmiconfig --ip_httpclient2_get <client_num> <url>
// wmiconfig --ip_httpclient2_put <client_num> <url>
// wmiconfig --ip_httpclient2_post <client_num> <url>
// wmiconfig --ip_httpclient2_patch <client_num> <url>
// wmiconfig --ip_httpclient2_setbody <client_num>
// wmiconfig --ip_httpclient2_addheader <client_num> <hdr_name> <hdr_value>
// wmiconfig --ip_httpclient2_clearheader <client_num>
// wmiconfig --ip_httpclient2_setparam <client_num> <key> <value>
A_INT32 httpclient2_process(A_INT32 argc, A_CHAR* argv[])
{
	A_STATUS error = A_OK;
	struct swat_http_client_s* arg = NULL;
	A_CHAR* command = argv[1] + 17;
	A_UINT32 num = 0;
	HTTPC_REQUEST_CMD_E req_cmd = 0;
	
	if (argc < 3)
	{
		SWAT_PTF("Missing parameters\n");
		return SWAT_ERROR;
	}
	
    if (strcmp(command, "connect") == 0)
	{
		A_UINT16 port = 0;
		A_UINT32 server_offset = 0;
		void* ssl_ctx = NULL;
		http_client_cb_t cb = NULL;
		struct swat_http_client_s* arg = NULL;
		A_UINT32 timeout = 0;
		A_UINT32 i;
		
        if(argc < 5){
            SWAT_PTF("wmiconfig --ip_httpclient2_connect server_addr port timeout [ssl_index] [callback_enable]\n");
            return SWAT_ERROR;
        }
		
		for (i = 0; i < HTTP_CLIENT_MAX_NUM; i++)
		{
			if (http_client[i].client == -1)
			{
				arg = &http_client[i];
				
				arg->num = i + 1; // important
				arg->cb_enable = FALSE;
				arg->total_len = 0;
				
				break;
			}
		}

		if (!arg)
		{
            SWAT_PTF("No More HTTP CLIENT\n");
			return SWAT_ERROR;
		}

        if(strlen((char*)argv[2]) >= 64)
        {
           SWAT_PTF("Maximum 64 bytes supported as Connect URL\n");
           return SWAT_ERROR;
        }
		
		port = swat_atoi(argv[3]);
		
		if (port == 0)
		{
			port = 80;
		}

		timeout = swat_atoi(argv[4]);
        
        if(strncmp(argv[2], "https://", 8) == 0)
	    {
            A_UINT32 ssl_inst_index = 0;
            SSL_INST *ssl = NULL;
			
	        server_offset = 8;

			if (argc < 6){
	             SWAT_PTF("SSL context index is missing for https\n");
	             return SWAT_ERROR;
            }
			
            ssl_inst_index = swat_atoi(argv[5]);
			
            if(NULL == (ssl = swat_find_ssl_inst(ssl_inst_index))){
                SWAT_PTF("ssl_ctx error\n"); 
                return SWAT_ERROR;
            }

            if (ssl->sslCtx == NULL || SSL_CLIENT != ssl->role)
            {
                SWAT_PTF("ssl_ctx error\n"); 
                return SWAT_ERROR;
            } 
            ssl_ctx = (void *)ssl->sslCtx;
        }
        else if(strncmp(argv[2], "http://", 7) == 0)
	    {
	        server_offset = 7;
  	    }

		if (ssl_ctx)
		{
			if (argc > 6 && strcmp(argv[6], "callback_enable") == 0)
			{
				arg->cb_enable = TRUE;
				cb = http_client_cb_demo;
			}
		}
		else if (argc > 5 && strcmp(argv[5], "callback_enable") == 0)
	    {
			arg->cb_enable = TRUE;
	        cb = http_client_cb_demo;
  	    }
		
		arg->client = qcom_http_client_connect((const A_CHAR *)(argv[2] + server_offset), port, timeout, ssl_ctx, cb, (void *)arg);

		if (arg->client == -1)
		{
			arg->num = 0;
			arg->cb_enable = FALSE;
			
            SWAT_PTF("http client connect failed\n");
		} 
		else if (arg->client == A_NO_HTTP_SESSION) 
		{
		    SWAT_PTF("There is no available http client session\r\n");
		}
		else
		{
            SWAT_PTF("http client connect success <client num> = %d%s\n", arg->num, arg->cb_enable ? ", callback enabled" : " ");
		}
        return SWAT_OK;
	}

	num = swat_atoi(argv[2]);
	if (num > 0 && num <= HTTP_CLIENT_MAX_NUM)
	{
		arg = &http_client[num - 1];

		if (arg->client == -1 || arg->num != num)
		{
			arg = NULL;	
		}
	}
	
	
	if (strcmp(command, "disconnect") == 0)
	{
		// wmiconfig --ip_httpclient2_disconnect <client num>
		if (!arg)
		{
			SWAT_PTF("<client num> error\n");
	        return SWAT_ERROR;
		}
		
		qcom_http_client_disconnect(arg->client);

		arg->client = -1;
		arg->num = 0;
		arg->cb_enable = FALSE;
		arg->total_len = 0;

		return SWAT_OK;
	}

	if (strcmp(command, "get") == 0)
	{
		req_cmd = HTTP_CLIENT_GET_CMD;
	}
	else if (strcmp(command, "put") == 0)
	{
		req_cmd = HTTP_CLIENT_PUT_CMD;
	}
	else if (strcmp(command, "post") == 0)
	{
		req_cmd = HTTP_CLIENT_POST_CMD;
	}
	else if (strcmp(command, "patch") == 0)
	{
		req_cmd = HTTP_CLIENT_PATCH_CMD;
	}

	if (req_cmd)
	{
    	A_CHAR* out = NULL;
		
	    if (argc < 4) {
			SWAT_PTF("Missing <url>\n");
	        return SWAT_ERROR;
	    }
		
		if (!arg)
		{
			SWAT_PTF("<client num> error\n");
	        return SWAT_ERROR;
		}
		
		if (!arg->cb_enable)
		{
			out = swat_mem_malloc(1500);
			
		    if (!out) {
		        SWAT_PTF("No memory for page\n");
		        return SWAT_ERROR;
		    }
		}
		
    	error = qcom_http_client_request(arg->client, req_cmd, (const A_CHAR *) argv[3], out);

		if (out)
		{
			if (error == A_OK)
			{
				http_client_cb_demo(arg, 0, out);
			}
			
    		swat_mem_free(out);
		}
	}
    else if (strcmp(command, "setbody") == 0)
	{
        A_CHAR *body = NULL;
        A_UINT32 len = DEFAULT_HTTPC_BODY_LEN;
		
        if (!arg)
        {
            SWAT_PTF("<client num> error\n");
            return SWAT_ERROR;
        }

        if (argc > 3) {
            len = atoi(argv[3]);
        }

        if (len > max_httpc_body_len)
            len = max_httpc_body_len;

		body = swat_build_httpc_body(len);
        if (!body)
		    return SWAT_ERROR;

        //SWAT_PTF("len = %d : %d\n%s\n", len, A_STRLEN(body), body);
        
		error = qcom_http_client_set_body(arg->client, (const A_CHAR*)body, A_STRLEN(body));

        qcom_mem_free(body);
	}
	else if (strcmp(command, "addheader") == 0)
	{
		if (!arg)
		{
			SWAT_PTF("<client num> error\n");
	        return SWAT_ERROR;
		}
		
		if (argc < 5)
		{
			SWAT_PTF("Missing parameters\n");
			return SWAT_ERROR;
		}
		error = qcom_http_client_add_header(arg->client, argv[3], argv[4]);
	}
	else if (strcmp(command, "clearheader") == 0)
	{
		if (!arg)
		{
			SWAT_PTF("<client num> error\n");
	        return SWAT_ERROR;
		}
		
		error = qcom_http_client_clear_header(arg->client);
	}
	else if(strcmp(command, "setparam") == 0)
	{
		if (!arg)
		{
			SWAT_PTF("<client num> error\n");
	        return SWAT_ERROR;
		}
		
		if (argc < 5)
		{
			SWAT_PTF("Missing parameters\n");
			return SWAT_ERROR;
		}
		error = qcom_http_client_set_param(arg->client, argv[3], argv[4]);
	}
	else
	{
		SWAT_PTF("Unknown http client command.\n");
		return SWAT_NOFOUND;
	}

	if (error != A_OK)
	{
        SWAT_PTF("http client %s failed\n", command);
	}

	return SWAT_OK;
}

#ifdef SSDP_ENABLED
A_INT32 swat_ssdp_init(A_INT32 argc, A_CHAR *argv[])
{
	A_INT32 return_code = A_OK;
	/*Domain name and service type*/
	A_UINT8 domain_name[] = "SmartHomeAlliance-org";
	A_UINT8 service_type[] = "smartHomeService";
	A_UINT8 uuid_str[] = "5CF96A99-5CF9-6A99-0E1F-5CF96A990E10";
	A_UINT32 notifyPeriod = 0; 
	A_UINT32 ipAddress = 0;
	A_UINT32 submask = 0;
	A_UINT32 gateway = 0;
	
	int res = 0;
	if(argc < 7)
	{
		SWAT_PTF("USAGE: wmiconfig --ssdp_init <notification period> <dev_type> <location_str> <target_str> <server_str>\n");
		return_code = A_ERROR;
	}
	else
	{
		notifyPeriod = (A_UINT32)(atoi(argv[2]));

		/*
			Short notification periods will create lots of network load. Each notification cycle creates 
			4 messages and sending them frequently would be sub optimal.
		*/
		if(notifyPeriod < 120) 
		{
			SWAT_PTF("\n Notification period should be at least 120 seconds\n");
			return_code = A_ERROR;	
		}
		else
		{	
			/*Need to send the current interface address along with SSDP query*/
			qcom_ipconfig(currentDeviceId, IP_CONFIG_QUERY,&ipAddress, &submask, &gateway);
			res = qcom_ssdp_init(notifyPeriod,ipAddress,(A_UINT8 *)argv[4],
								(A_UINT8 *)argv[6],(A_UINT8 *)argv[5],
								(A_UINT8 *)domain_name,(A_UINT8 *)service_type,
								(A_UINT8 *)argv[3],(A_UINT8 *)uuid_str);
			if(res)
			{
				SWAT_PTF("SSDP Init returns error %d\n", res);
				return_code = A_ERROR;
			}
		}
	}

	return return_code;
}
A_INT32 swat_ssdp_enable(A_INT32 argc, A_CHAR *argv[])
{
	A_INT32 return_code = A_OK;
	A_UINT32 enable =0;
	int res = 0;
	if(argc < 3)
	{
		SWAT_PTF("USAGE: wmiconfig --ssdp_enable [0/1]");
		return_code = A_ERROR;
	}
	else
	{
		enable = atoi(argv[2]);
		if( (enable != 0) && (enable != 1) )
		{
			SWAT_PTF("SSDP enable should be 0(STOP) or 1(START)");
			return_code = A_ERROR;
		}
		else
		{
			res = qcom_ssdp_enable(enable);
			if(res)
			{
				SWAT_PTF("SSDP Enable returns error %d \n", res);
				return_code = A_ERROR;
			}
		}
	}
	return return_code;
}
A_INT32 swat_ssdp_change_notify_period(A_INT32 argc, A_CHAR *argv[])
{
	A_INT32 return_code = A_OK;
	A_UINT32 notifyPeriod = 0;
	int res = 0;
	if(argc < 3)
	{
		SWAT_PTF("USAGE: wmiconfig --ssdp_change_notify_period [notification period]\n");
		return_code = A_ERROR;
	}
	else
	{
		notifyPeriod = atoi(argv[2]);
		/*
			Short notification periods will create lots of network load. Each notification cycle creates 
			4 messages and sending them frequently would be sub optimal.
		*/
		if(notifyPeriod < 120) 
		{
			SWAT_PTF("\n Notification period should be at least 120 seconds\n");
			return_code = A_ERROR;	
		}
		else
		{			
			res = qcom_ssdp_notify_change(notifyPeriod);
			if(res)
			{
				SWAT_PTF("SSDP Notify Change returns error %d\n", res);
				return_code = A_ERROR;
			}
		}
	}

	return return_code;
}
#endif //#ifdef SSDP_ENABLED


#if 0 //defined(P2P_ENABLED)

/* Printf to the serial port */
#if defined(P2P_PRINTS_ENABLED) /* { */
#define P2P_PRINTF(args...)  A_PRINTF(args)
#else /* } { */
#define P2P_PRINTF(args...)
#endif /* } */

int swat_wmiconfig_p2p(int argc, char *argv[])
{
      if(argc < 2){
#if defined(P2P_PRINTS_ENABLED)
          P2P_PRINTF("<on|off> | <find|stop> | <cancel> | <nodelist> | <auth|connect peer-mac push|display|keypad pin>\n");
          P2P_PRINTF("<set> <p2pmode p2pdevp|p2pclient|p2pgo> | <gointent val>\n");
          P2P_PRINTF("p2p invite <ssid> <mac> <wps-method>\n");
          P2P_PRINTF("join <mac> <push | keypad | display> <wpspin> \n");
          P2P_PRINTF("prov <mac> <display> \n");
#endif
          return -1;
      }
      if(!strcmp(argv[1], "on")) {
          extern void qca_p2p_enable(A_UINT8 device_id, int enable);
          qca_p2p_enable(currentDeviceId, 1);
      }
      else if(!strcmp(argv[1], "find")) {

       #define P2P_STANDARD_TIMEOUT 300

        typedef enum p2p_disc_type {
        P2P_DISC_START_WITH_FULL,
        P2P_DISC_ONLY_SOCIAL,
        P2P_DISC_PROGRESSIVE
        } P2P_DISC_TYPE;

        P2P_DISC_TYPE type;
        A_UINT32 timeout;

        extern void qca_p2p_device_discover(A_UINT8 device_id, P2P_DISC_TYPE type,A_UINT32 timeout);
        if(argc == 3)
        {
             if(strcmp(argv[2],"1") == 0)
             {
                   type    = P2P_DISC_START_WITH_FULL;
                   timeout = P2P_STANDARD_TIMEOUT;
             }
             else if(strcmp(argv[2],"2") == 0)
             {
                   type    = P2P_DISC_ONLY_SOCIAL;
                   timeout = P2P_STANDARD_TIMEOUT;
             }
             else if(strcmp(argv[2],"3") == 0)
             {
                   type    = P2P_DISC_PROGRESSIVE;
                   timeout = P2P_STANDARD_TIMEOUT;
             }
             else
             {
                   P2P_PRINTF("\n wrong option enter option 1,2 or 3\n");
                   return;
             }
        }
        else if(argc == 4)
        {
             if(strcmp(argv[2],"1") == 0)
             {
                   type    = P2P_DISC_START_WITH_FULL;
                   timeout = atoi(argv[3]);
             }
             else if(strcmp(argv[2],"2") == 0)
             {
                   type    = P2P_DISC_ONLY_SOCIAL;
                   timeout = atoi(argv[3]);
             }
             else if(strcmp(argv[2],"3") == 0)
             {
                   type    = P2P_DISC_PROGRESSIVE;
                   timeout = atoi(argv[3]);
             }
             else
             {
                   P2P_PRINTF("\n wrong option enter option 1,2 or 3\n");
                   return;
             }
        }
        else{
                   type    = P2P_DISC_ONLY_SOCIAL;
                   timeout = P2P_STANDARD_TIMEOUT;
        }
         qca_p2p_device_discover(currentDeviceId, type,timeout);
    }

    else if(!strcmp(argv[1], "nodelist")) {
         extern void qca_p2p_device_list_shown(A_UINT8 device_id);
         qca_p2p_device_list_shown(currentDeviceId);
    }
    else if(!strcmp(argv[1],"setconfig"))
    {
           extern void qca_p2p_set_config(A_UINT8 device_id, A_UINT8 go_intent,A_UINT8 listen_ch,A_UINT8 oper_ch,char *op_ch,A_UINT8 country[3],A_UINT32 node_age_to);
           A_UINT8 go_intent, listen_channel, oper_channel;
           A_UINT32 node_age_timeout;
           A_UINT8 country[3];
           if(argc < 7){
              P2P_PRINTF("error cmd \n");
              return -1;
           }
           go_intent        = atoi(argv[2]);
           listen_channel   = atoi(argv[3]);
           oper_channel     = atoi(argv[4]);
           strcpy((char *)country,argv[5]);
           node_age_timeout = atoi(argv[6]);
           qca_p2p_set_config(currentDeviceId, go_intent,listen_channel,oper_channel,argv[4],country,node_age_timeout);
    }
    else if(!strcmp(argv[1], "connect")) {
           extern void qca_p2p_connect_client(A_UINT8 device_id, int wps_method, unsigned char *peer_mac,unsigned char* wps_pin,A_UINT8 persistent_flag);
           int rt;
           int wps_method;
           int aidata[6];
           unsigned char peer_mac[6],wps_pin[9];
           A_UINT8 persistent_flag = 0;

           if(argc < 4){
              P2P_PRINTF("error cmd\n");
              return -1;
           }

           if(!strcmp(argv[3], "push")) {
              wps_method = 4;     // WPS_PBC
              if(argc == 5)
              {
                 if(!strcmp(argv[4],"persistent")) {
                        persistent_flag = 1;
              }
              strcpy((char *)wps_pin,(char *)"\0");
              }
           }
           else if(!strcmp(argv[3], "display")) {
              wps_method = 2;     // WPS_PIN_DISPLAY
              strcpy((char *)wps_pin,(char *)argv[4]);
           }
           else if(!strcmp(argv[3], "keypad")) {
              wps_method = 3;     // WPS_PIN_KEYPAD
              strcpy((char *)wps_pin,(char*)argv[4]);
           }
           else{
              P2P_PRINTF("wps mode error.\n");
              return -1;
           }

           rt = sscanf(argv[2], "%2x:%2x:%2x:%2x:%2x:%2x", \
                                &aidata[0], &aidata[1], &aidata[2], \
                                &aidata[3], &aidata[4], &aidata[5]);
           if (rt < 0)
           {
                P2P_PRINTF("wrong mac format.\n");
                return -1;
           }

           for (rt = 0; rt < 6; rt++)
           {
              peer_mac[rt] = (unsigned char)aidata[rt];
           }
        qca_p2p_connect_client(currentDeviceId, wps_method, peer_mac,(unsigned char*) wps_pin, persistent_flag);
    }
    else if(!strcmp(argv[1], "auth")) {
        extern void qca_p2p_auth(A_UINT8 device_id, int wps_method, unsigned char *peer_mac,unsigned char* wps_pin,A_UINT8 persistent_flag);
        int rt;
        int wps_method;
        int aidata[6];
        unsigned char peer_mac[6],wps_pin[9];
        A_UINT8 persistent_flag = 0;

        if(argc < 4){
            P2P_PRINTF("error cmd\n");
            return -1;
        }

        if(!strcmp(argv[3], "push")) {
            wps_method = 4;     // WPS_PBC
            if(argc == 5)
            {
                 if(!strcmp(argv[4],"persistent")) {
                        persistent_flag = 1;
                 }
                 strcpy((char *)wps_pin,(char *)"\0");
            }
        }
        else if(!strcmp(argv[3], "deauth")) {
            wps_method = 1;
        }
        else if(!strcmp(argv[3], "display")) {
            wps_method = 2;     // WPS_PIN_DISPLAY
            strcpy((char *)wps_pin,(char *)argv[4]);
        }
        else if(!strcmp(argv[3], "keypad")) {
            wps_method = 3;     // WPS_PIN_KEYPAD
            strcpy((char *)wps_pin,(char*)argv[4]);
        }
        else{
            P2P_PRINTF("wps mode error.\n");
            return -1;
        }

        rt = sscanf(argv[2], "%2x:%2x:%2x:%2x:%2x:%2x", \
                                &aidata[0], &aidata[1], &aidata[2], \
                                &aidata[3], &aidata[4], &aidata[5]);
        if (rt < 0)
        {
            P2P_PRINTF("wrong mac format.\n");
            return -1;
        }

        for (rt = 0; rt < 6; rt++)
        {
            peer_mac[rt] = (unsigned char)aidata[rt];
        }
        qca_p2p_auth(currentDeviceId, wps_method, peer_mac,(unsigned char*) wps_pin, persistent_flag);
    }
    else if(!strcmp(argv[1], "autogo")) {
        extern void qca_p2p_auto_go(A_UINT8 device_id, A_UINT8 flag);
        A_UINT8 persistent_flag = 0;
        if(argc > 2)
        {
                if(!strcmp(argv[2],"persistent")) {
                    persistent_flag = 1;
                }
        }
        qca_p2p_auto_go(currentDeviceId, persistent_flag);
    }
    else if(!strcmp(argv[1],"passphrase"))
    {
        extern void qca_set_passphrase(A_UINT8 device_id, char *pphrase,char *ssid) ;

        if(argc < 4)
        {
            P2P_PRINTF("\n Usage : wmiconfig --p2p passphrase <passphrase> <SSID> \n");
            return -1;
        }
        qca_set_passphrase(currentDeviceId, argv[2],argv[3]) ;
    }
    else if(!strcmp(argv[1], "prov")) {
        extern void qca_p2p_prov(A_UINT8 device_id, int wps_method, unsigned char *pmac);
        int rt;
        int wps_method;
        int aidata[6];
        unsigned char peer_mac[6];

        if(argc < 4){
            P2P_PRINTF("err or cmd\n");
            return -1;
        }

        rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                                &aidata[0], &aidata[1], &aidata[2], \
                                &aidata[3], &aidata[4], &aidata[5]);
        if (rt < 0)
        {
            P2P_PRINTF("wrong mac format.\n");
            return -1;
        }

        for (rt = 0; rt < 6; rt++)
        {
            peer_mac[rt] = (unsigned char)aidata[rt];
        }

        if(!strcmp(argv[3], "push")) {
            wps_method = 4;//WPS_PBC
        }
        else if(!strcmp(argv[3], "display")) {
            wps_method = 2;//WPS_PIN_DISPLAY
        }
        else if(!strcmp(argv[3], "keypad")) {
            wps_method = 3;//WPS_PIN_KEYPAD
        }
        else{
            P2P_PRINTF("wps mode error.\n");
            return -1;
        }
        qca_p2p_prov(currentDeviceId, wps_method, peer_mac);
    }
    else if(!strcmp(argv[1], "join")) {
        extern void qca_p2p_join(A_UINT8 device_id, int wps_method, unsigned char *pmac, unsigned char *wps_pin);
        int rt;
        int wps_method;
        int aidata[6];
        unsigned char peer_mac[6],wps_pin[9];

        if(argc < 4){
            P2P_PRINTF("error cmd\n");
            return -1;
        }

        rt = sscanf(argv[2], "%2X:%2X:%2X:%2X:%2X:%2X", \
                                &aidata[0], &aidata[1], &aidata[2], \
                                &aidata[3], &aidata[4], &aidata[5]);
        if (rt < 0)
        {
            P2P_PRINTF("wrong mac format.\n");
            return -1;
        }

        for (rt = 0; rt < 6; rt++)
        {
            peer_mac[rt] = (unsigned char)aidata[rt];
        }

        if(!strcmp(argv[3], "push")) {
            wps_method = 4;//WPS_PBC
            strcpy((char *)wps_pin,(char *)"\0");
        }
        else if(!strcmp(argv[3], "display")) {
            wps_method = 2;//WPS_PIN_DISPLAY
            strcpy((char *)wps_pin,(char*)argv[4]);
        }
        else if(!strcmp(argv[3], "keypad")) {
            wps_method = 3;//WPS_PIN_KEYPAD
            strcpy((char *)wps_pin,(char*)argv[4]);
        }
        else{
            P2P_PRINTF("wps mode error.\n");
            return -1;
        }
        qca_p2p_join(currentDeviceId, wps_method, peer_mac, wps_pin);
    }
    else if(!strcmp(argv[1], "invite")) {
        extern void qca_p2p_invite(A_UINT8 device_id, unsigned char *ssid, unsigned char *pmac, A_UINT8 method, unsigned char *wps_pin);
        int rt;
        A_UINT8 wps_method;
        int aidata[6];
        unsigned char peer_mac[6],wps_pin[9];
        unsigned char ssid[32];

        if(argc < 5){
            P2P_PRINTF("error cmd\n");
            return -1;
        }

        A_MEMSET(ssid, 0, 32);

        memcpy((char *)ssid,argv[2],strlen(argv[2]));

        rt = sscanf(argv[3], "%2X:%2X:%2X:%2X:%2X:%2X", \
                                &aidata[0], &aidata[1], &aidata[2], \
                                &aidata[3], &aidata[4], &aidata[5]);
        if (rt < 0)
        {
            P2P_PRINTF("wrong mac format.\n");
            return -1;
        }

        for (rt = 0; rt < 6; rt++)
        {
            peer_mac[rt] = (unsigned char)aidata[rt];
        }

        if(!strcmp(argv[4], "push")) {
            wps_method = 4;//WPS_PBC
            strcpy((char *)wps_pin,(char *)"\0");
        }
        else if(!strcmp(argv[4], "display")) {
            wps_method = 2;//WPS_PIN_DISPLAY
            strcpy((char *)wps_pin,(char*)argv[4]);
        }
        else if(!strcmp(argv[4], "keypad")) {
            wps_method = 3;//WPS_PIN_KEYPAD
            strcpy((char *)wps_pin,(char*)argv[4]);
        }
        else{
            P2P_PRINTF("wps mode error.\n");
            return -1;
        }
        qca_p2p_invite(currentDeviceId, ssid, peer_mac, wps_method, wps_pin);
    }
#if defined(P2P_HOSTLESS_PS_EN)
    else if(!strcmp(argv[1], "setnoa")) {
        A_UINT8 count_or_type;
        int duration = 0,interval = 0,start_or_offset =0;;
        extern void qca_setnoa(A_UINT8 count_or_type,int start_or_offset,int duration,int interval);
        if(argc < 3)
        {
            P2P_PRINTF("error cmd\n");
            return -1;
        }
        else{
            count_or_type   = atoi(argv[2]);
            if(argc > 3)
            {
               start_or_offset = atoi(argv[3]) * 1000;
               duration        = atoi(argv[4]) * 1000;
               interval        = atoi(argv[5]) * 1000;
            }
            qca_setnoa(count_or_type,start_or_offset,duration,interval);
        }

    }
#endif
    else{
           P2P_PRINTF("Error : P2P Command not found \n");
    }

      return 0;
}
#endif

#if defined(WLAN_BTCOEX_ENABLED)
int
swat_wmiconfig_btcoex_handle(int argc, char *argv[])
{
    int return_code;

    if (strcmp(argv[1],"--setbtcoexfeant") == 0)
    {
        return_code = ath_set_btcoex_fe_ant(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexcolocatedbt") == 0)
    {
        return_code = ath_set_btcoex_colocated_bt_dev(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexscoconfig") == 0)
    {
        return_code = ath_set_btcoex_sco_config(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexa2dpconfig") == 0)
    {
        return_code = ath_set_btcoex_a2dp_config(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexhidconfig") == 0)
    {
        return_code = ath_set_btcoex_hid_config(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexaclcoexconfig") == 0)
    {
        return_code = ath_set_btcoex_aclcoex_config(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexbtinquirypageconfig") == 0)
    {
        return_code = ath_set_btcoex_btinquiry_page_config(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexdebug") == 0)
    {
        return_code = ath_set_btcoex_debug(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexbtoperatingstatus") == 0)
    {
        return_code = ath_set_btcoex_bt_operating_status(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--getbtcoexconfig") == 0)
    {
        return_code = ath_get_btcoex_config(currentDeviceId, argc, argv);
    }
    else if (strcmp(argv[1],"--setbtcoexscheme") == 0)
    {
        return_code = ath_set_btcoex_scheme(currentDeviceId, argc, argv);
    }
    else
        return_code = 0;

    return return_code;
}
#endif /* WLAN_BTCOEX_ENABLED */


#ifdef ENABLE_MODULE 
ADD_PARAMS add_params_cb = NULL;

/*Module callback registration*/
int swat_module_register_cb(void *callback)
{
    add_params_cb = callback;
    return A_OK;
}

void swat_add_params()
{
    int sum;

    A_PRINTF("\nBegin of Function call back Test:\n");

    if(add_params_cb)
    {
        sum = add_params_cb(1,2,3,4);
        if (sum == 10) {
            A_PRINTF("Function call back: Success\n");
        } else {
            A_PRINTF("Function call back: FAIL\n");
        }
    }
    else
        A_PRINTF("Please register module function callbak\n");
    A_PRINTF("End of Function call back Test\n");

}

A_INT32 module_handler(A_INT32 argc, A_CHAR *argv[])
{
    if (A_STRCMP(argv[0], "insmod") == 0) {
        if (argc != 3) {
            A_PRINTF("Usage: %s part_id module_name\n", argv[0]);
            return -1;
        }

        if (qcom_load_module(atoi(argv[1]), argv[2]) < 0) {
            A_PRINTF("Failed to load module");
            return -1;
        }
    } else if (A_STRCMP(argv[0], "rmmod") == 0) {
        if (argc != 2) {
            A_PRINTF("Usage: %s module_name\n", argv[0]);
            return -1;
        }

        if (qcom_remove_module(argv[1]) < 0) {
            A_PRINTF("Failed to remove module");
            return -1;
        }
    } else if (A_STRCMP(argv[0], "lsmod") == 0) {
        if (qcom_show_module() < 0) {
            A_PRINTF("Failed to list module information");
            return -1;
        }
    }else if (A_STRCMP(argv[0], "rmmodcb") == 0) {
          swat_module_register_cb(NULL);
          A_PRINTF("Unregister module Function call back");
    }else if (A_STRCMP(argv[0], "runmodcb") == 0) {
          swat_add_params();
    }else
        A_PRINTF("Unknown commands\n");
   
    return 0;
}

#endif

