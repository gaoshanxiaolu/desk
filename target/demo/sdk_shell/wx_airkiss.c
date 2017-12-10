 //\u5e73\u53f0\u76f8\u5173\u5934\u6587\u4ef6
//#include "athtypes.h"
#include "qcom_common.h"
#include "socket_api.h"
#include "malloc_api.h"
#include "qcom_nvram.h"

//#include <qcom/qcom_timer.h>

#include "ble_uart.h"


//\u5305\u542bAirKiss\u5934\u6587\u4ef6
#include "airkiss.h"
#include "wx_airkiss.h"
#include "wx_heart_package.h"

//\u5f53\u524d\u76d1\u542c\u7684\u65e0\u7ebf\u4fe1\u9053
static A_UINT32 cur_channel = 0;
static A_UINT8 current_DeviceId = 0;

//\u7528\u4e8e\u5207\u6362\u4fe1\u9053\u7684\u5b9a\u65f6\u5668\uff0c\u5e73\u53f0\u76f8\u5173
//static qcom_timer_t time_serv;

//AirKiss\u8fc7\u7a0b\u4e2d\u9700\u8981\u7684RAM\u8d44\u6e90\uff0c\u5b8c\u6210AirKiss\u540e\u53ef\u4ee5\u4f9b\u5176\u4ed6\u4ee3\u7801\u4f7f\u7528
static airkiss_context_t akcontex;

//\u53e6\u4e00\u79cd\u66f4\u8282\u7701\u8d44\u6e90\u7684\u4f7f\u7528\u65b9\u6cd5\uff0c\u901a\u8fc7malloc\u52a8\u6001\u7533\u8bf7RAM\u8d44\u6e90\uff0c\u5b8c\u6210\u540e\u5229\u7528free\u91ca\u653e\uff0c\u9700\u8981\u5e73\u53f0\u652f\u6301
//\u793a\u4f8b\uff1a
//airkiss_context_t *akcontexprt;
//akcontexprt =
//(airkiss_context_t *)os_malloc(sizeof(airkiss_context_t));

//\u5b9a\u4e49AirKiss\u5e93\u9700\u8981\u7528\u5230\u7684\u4e00\u4e9b\u6807\u51c6\u51fd\u6570\uff0c\u7531\u5bf9\u5e94\u7684\u786c\u4ef6\u5e73\u53f0\u63d0\u4f9b\uff0c\u524d\u4e09\u4e2a\u4e3a\u5fc5\u8981\u51fd\u6570
const static airkiss_config_t akconf =
{
	.memset = memset,
	.memcpy = memcpy,
	.memcmp = memcmp,
	.printf = qcom_printf
	//(airkiss_memset_fn)&memset,
	//(airkiss_memcpy_fn)&memcpy,
	//(airkiss_memcmp_fn)&memcmp,
	//0
};

static airkiss_result_t result;

static A_INT32 airkiss_state = AIRKISS_IDLE;

static A_UINT8 airkiss_finish_flag = 0;

A_UINT8 conn_route_flag;

static A_UINT8 debug_set_channel_flag = 1;

SCAN_RESULTS pstScanResults[100];
A_INT8 bScanFinish = 0;
A_UINT16 nScanCounts = 0;
A_INT8 bDisableScan = 0;

A_CHAR *channel_list[13] = {"1","2","3","4","5","6","7","8","9","10","11","12","13"};

/*
 * \u5e73\u53f0\u76f8\u5173\u5b9a\u65f6\u5668\u4e2d\u65ad\u5904\u7406\u51fd\u6570\uff0c100ms\u4e2d\u65ad\u540e\u5207\u6362\u4fe1\u9053
 */
extern A_INT32 swat_wmiconfig_handle(A_INT32 argc, A_CHAR *argv[]);
extern A_INT32 swat_iwconfig_scan_handle(A_INT32 argc, A_CHAR * argv[]);

void set_channel(A_CHAR *ch)
{
	A_CHAR *cmd_name = "wmiconfig";
	A_CHAR *cmd_p1 = "--channel";

	A_CHAR *cmd_config[3];
	cmd_config[0] = cmd_name;
	cmd_config[1] = cmd_p1;
	cmd_config[2] = ch;
	swat_wmiconfig_handle(3,cmd_config);
}
A_INT8 airkiss_timeout = 0;
A_INT8 airkiss_success = 0;

void time_callback(unsigned int argc, void *argv)
{
	airkiss_timeout = 1;
	//\u5207\u6362\u4fe1\u9053
	//int status;
	/*if (cur_channel >= 13)
		cur_channel = 1;
	else
		cur_channel++;
	
	if(debug_set_channel_flag)
	{
		qcom_printf("cur_channel = %d\r\n",cur_channel);
		set_channel(channel_list[cur_channel-1]);
		//status = qcom_set_channel(current_DeviceId,cur_channel);
	}*/
	//if(status != A_OK)
	//{
	//	qcom_printf("set cur_channel failed\r\n");
	//}
    //airkiss_change_channel(&akcontex);//\u6e05\u7f13\u5b58
}

/*
 * airkiss\u6210\u529f\u540e\u8bfb\u53d6\u914d\u7f6e\u4fe1\u606f\uff0c\u5e73\u53f0\u65e0\u5173\uff0c\u4fee\u6539\u6253\u5370\u51fd\u6570\u5373\u53ef
 */
static void airkiss_finish(void)
{
	A_INT8 err;
	err = airkiss_get_result(&akcontex, &result);
	if (err == 0)
	{
		//qcom_printf("airkiss_get_result() ok!");
		
		qcom_printf("ssid_length = %d pwd_length = %d, random = 0x%02x\r\n",result.ssid_length, result.pwd_length, result.random);
		qcom_printf("ssid = %s, pwd = %s,\r\n",result.ssid, result.pwd);
	}
	else
	{
		qcom_printf("airkiss_get_result() failed !\r\n");
	}
}

/*
 * \u6df7\u6742\u6a21\u5f0f\u4e0b\u6293\u5230\u7684802.11\u7f51\u7edc\u5e27\u53ca\u957f\u5ea6\uff0c\u5e73\u53f0\u76f8\u5173
 */
static void wifi_promiscuous_rx(unsigned char *buf, int len)
{
	A_INT8 ret;
	//qcom_printf("enter promisc callback\r\n");
	if(buf == NULL || len == 0)
	{
		qcom_printf("len = %d buf == NULL || len == 0\r\n",len);
		return;
	}
	//qcom_printf("len = %d\r\n",len);
	/*int i;
	for(i=0;i<len;i++)
	{
		//qcom_printf("0x%x,",buf[i]);
	}*/
	//qcom_printf("\r\n");
	//\u5c06\u7f51\u7edc\u5e27\u4f20\u5165airkiss\u5e93\u8fdb\u884c\u5904\u7406
	ret = airkiss_recv(&akcontex, buf, len);
	//\u5224\u65ad\u8fd4\u56de\u503c\uff0c\u786e\u5b9a\u662f\u5426\u9501\u5b9a\u4fe1\u9053\u6216\u8005\u8bfb\u53d6\u7ed3\u679c
	if ( ret == AIRKISS_STATUS_CHANNEL_LOCKED)
	{
		//qcom_timer_stop(&time_serv);
		airkiss_finish_flag = 2;
		qcom_printf("channel locked!!!\r\n");
	}
	else if ( ret == AIRKISS_STATUS_COMPLETE )
	{
		airkiss_finish();
		//qcom_timer_delete(&time_serv);
		qcom_promiscuous_enable(0);//\u5173\u95ed\u6df7\u6742\u6a21\u5f0f\uff0c\u5e73\u53f0\u76f8\u5173
		airkiss_finish_flag = 1;
	}
	//qcom_thread_msleep(100);
	return;
}

/*
 * \u521d\u59cb\u5316\u5e76\u5f00\u59cb\u8fdb\u5165AirKiss\u6d41\u7a0b\uff0c\u5e73\u53f0\u76f8\u5173
 */
void start_airkiss(void)
{
	A_INT8 ret;
	//\u5982\u679c\u6709\u5f00\u542fAES\u529f\u80fd\uff0c\u5b9a\u4e49AES\u5bc6\u7801\uff0c\u6ce8\u610f\u4e0e\u624b\u673a\u7aef\u7684\u5bc6\u7801\u4e00\u81f4
#if AIRKISS_ENABLE_CRYPT
	const char* key = "Wechatiothardwav";
#endif
	printf("airkiss lib version = %s\r\n",airkiss_version()); 
	//\u8c03\u7528\u63a5\u53e3\u521d\u59cb\u5316AirKiss\u6d41\u7a0b\uff0c\u6bcf\u6b21\u8c03\u7528\u8be5\u63a5\u53e3\uff0c\u6d41\u7a0b\u91cd\u65b0\u5f00\u59cb\uff0cakconf\u9700\u8981\u9884\u5148\u8bbe\u7f6e\u597d\u53c2\u6570
	ret = airkiss_init(&akcontex, &akconf);
	//\u5224\u65ad\u8fd4\u56de\u503c\u662f\u5426\u6b63\u786e
	if (ret < 0)
	{
		qcom_printf("Airkiss init failed!\r\n");
		return;
	}

#if AIRKISS_ENABLE_CRYPT
//\u5982\u679c\u4f7f\u7528AES\u52a0\u5bc6\u529f\u80fd\u9700\u8981\u8bbe\u7f6e\u597dAES\u5bc6\u94a5\uff0c\u6ce8\u610f\u5305\u542b\u6b63\u786e\u7684\u5e93\u6587\u4ef6\uff0c\u5934\u6587\u4ef6\u4e2d\u7684\u5b8f\u8981\u6253\u5f00
	airkiss_set_key(&akcontex, key, strlen(key));
#endif
	qcom_printf("Finish init airkiss!\r\n");
	//\u4ee5\u4e0b\u4e0e\u786c\u4ef6\u5e73\u53f0\u76f8\u5173\uff0c\u8bbe\u7f6e\u6a21\u5757\u4e3aSTATION\u6a21\u5f0f\u5e76\u5f00\u542f\u6df7\u6742\u6a21\u5f0f\uff0c\u542f\u52a8\u5b9a\u65f6\u5668\u7528\u4e8e\u5b9a\u65f6\u5207\u6362\u4fe1\u9053
	if(conn_route_flag)
	{
		qcom_disconnect(current_DeviceId);
		conn_route_flag = 0;
	}
	//qcom_op_set_mode(current_DeviceId,QCOM_WLAN_DEV_MODE_STATION);
	//cur_channel = 1;
	//qcom_set_channel(current_DeviceId,cur_channel); 
	//qcom_timer_init(&time_serv,time_callback,NULL,1000*180,PERIODIC);
	
	
	qcom_set_promiscuous_rx_cb((QCOM_PROMISCUOUS_CB)wifi_promiscuous_rx);
	qcom_promiscuous_enable(1);
	//qcom_timer_start(&time_serv);

	//qcom_printf("end start airkiss!\r\n");
}

void airkiss_response(A_CHAR *buf, A_INT32 length)
{
	struct sockaddr_in wx_resp_addr;
	A_INT16 repeat = 20;
	A_INT32 socketLocal;

	A_MEMZERO(&wx_resp_addr, sizeof(wx_resp_addr));
    wx_resp_addr.sin_addr.s_addr= htonl(0xffffffff);
    wx_resp_addr.sin_port = htons(10000);
    wx_resp_addr.sin_family = AF_INET;

	qcom_printf("Send airkiss_response, length %d, 0x%x\n", length, buf[0]);

	if((socketLocal = qcom_socket( AF_INET, SOCK_DGRAM, 0)) == A_ERROR) {
		qcom_printf("qcom_socket failed\r\n");
		return;
    }

	while(repeat){
		repeat--;
		qcom_sendto(socketLocal, buf, length, 0,(struct sockaddr *)(&wx_resp_addr), sizeof(wx_resp_addr)) ;
		qcom_thread_msleep(50);
	}

	qcom_socket_close(socketLocal);

	qcom_printf("Finish airkiss_response\n");
	
	return;
}

void connect_route(void)
{
	A_CHAR *cmd_name = "wmiconfig";
	A_CHAR *cmd_p1 = "--p";
	A_CHAR *cmd_p2 = "--wpa";
	A_CHAR *cmd_p2_1 = "2";
	A_CHAR *cmd_p2_2 = "CCMP";
	A_CHAR *cmd_p3 = "--connect";
	A_CHAR *cmd_p4 = "--ipdhcp";
	A_CHAR *config_cmd[5];
	
	config_cmd[0] = cmd_name;

	config_cmd[1] = cmd_p1;
	config_cmd[2] = result.pwd;
	swat_wmiconfig_handle(3,config_cmd);

	config_cmd[1] = cmd_p2;
	config_cmd[2] = cmd_p2_1;
	config_cmd[3] = cmd_p2_2;
	config_cmd[4] = cmd_p2_2;
	swat_wmiconfig_handle(5,config_cmd);

	config_cmd[1] = cmd_p3;
	config_cmd[2] = result.ssid;
	swat_wmiconfig_handle(3,config_cmd);

	config_cmd[1] = cmd_p4;
	swat_wmiconfig_handle(2,config_cmd);

	return;
}
A_UINT8 channels_merge[13] = {0},valid_chs;
extern int heart_app_run_flag ;
//char ioe_ssid[32],ioe_pass[32];

#define IOE_CONFIG_ADDR (A_UINT32)(0x009ff000)

typedef struct configs
{
	char ioe_ssid[32];
	char ioe_pass[32];
	char has_ssid;
	char ids[5];
	char has_id;
	enum DEV_TYPE dev_type;
	char reserve[32];
}CONFIG_PARAM;

CONFIG_PARAM cfg_param;

void save_cfg(void)
{
	qcom_nvram_erase(IOE_CONFIG_ADDR, 0x1000);
	qcom_nvram_write(IOE_CONFIG_ADDR, (A_UCHAR*)(void *)&cfg_param, sizeof(CONFIG_PARAM));
}

enum DEV_TYPE get_config_dev_type(void)
{
	if(cfg_param.dev_type == -1)//enum type is char (0xff == -1)
		return UNDEF;
	return cfg_param.dev_type;
}

void save_config_dev_type(enum DEV_TYPE dev_type)
{
	cfg_param.dev_type = dev_type;
	save_cfg();
}

int ioe_wifi_config_get()
{

  
   qcom_nvram_read(IOE_CONFIG_ADDR, (A_UCHAR *)(void *)(&cfg_param), sizeof(CONFIG_PARAM));


   if(cfg_param.has_ssid == 1)
   {
   	A_PRINTF("ssid = %s, pass = %s\n", cfg_param.ioe_ssid, cfg_param.ioe_pass);
	return 0;
   }
  
   return -1;
}
A_BOOL has_ssid_pwd = FALSE;

A_BOOL is_has_ssid_pwd(void)
{
	return has_ssid_pwd;
}

/* write wifi config to flash */
int ioe_wifi_config_set(char *ssid, char *pass)
{
   memset(cfg_param.ioe_ssid, 0, 32);
   memset(cfg_param.ioe_pass, 0, 32);
   strcpy(cfg_param.ioe_ssid, ssid);
   strcpy(cfg_param.ioe_pass, pass);
	cfg_param.has_ssid = 1;
	save_cfg();
	
   return 0;
}

int save_wifi_setting(void)
{
	save_cfg();


}

A_INT32 set_dev_id(A_INT32 argc, A_CHAR *argv[])
{
	A_UCHAR id[5];
	if(argc < 6)
	{
		printf("param error,set_id n1 n2 n3\r\n");
		return 0;
	}

    char* p = argv[1];      
    char* str;      
    long i = strtol(p, &str, 16);
	id[0] = i;

    p = argv[2];           
    i = strtol(p, &str, 16);
	id[1] = i;
	
    p = argv[3];           
    i = strtol(p, &str, 16);
	id[2] = i;

    p = argv[4];           
    i = strtol(p, &str, 16);
	id[3] = i;
	
    p = argv[5];           
    i = strtol(p, &str, 16);
	id[4] = i;

	printf("******you set id is %02x %02x %02x %02x %02x\r\n",id[0],id[1],id[2],id[3],id[4]);
	cfg_param.ids[0] = id[0];
	cfg_param.ids[1] = id[1];
	cfg_param.ids[2] = id[2];
	cfg_param.ids[3] = id[3];
	cfg_param.ids[4] = id[4];
	cfg_param.has_id = 1;
	save_cfg();


	return 0;
}

int get_dev_id(char *id)
{
	//qcom_nvram_read(IOE_CONFIG_ADDR+64, (A_UCHAR *)id, 5);
	if(cfg_param.has_id == 1)
	{
	  id[0] = cfg_param.ids[0];
	id[1] = cfg_param.ids[1];
	 id[2] = cfg_param.ids[2];
	id[3] = cfg_param.ids[3];
	 id[4] =cfg_param.ids[4];
	}
	else
	{
		  id[0] = 0xff;
		id[1] = 0xff;
		 id[2] = 0xff;
		id[3] = 0xff;
		 id[4] =0xff;

	}

	printf("ids=%02x,%02x,%02x,%02x,%02x,\r\n",id[0],id[1],id[2],id[3],id[4]);
	return 0;
}

void reset_wifi_config(void)
{
	//A_UCHAR id[5];
	//qcom_nvram_read(IOE_CONFIG_ADDR+64, (A_UCHAR *)id, 5);
	//qcom_nvram_erase(IOE_CONFIG_ADDR, 0x1000);
	//qcom_nvram_write(IOE_CONFIG_ADDR+64, (A_UCHAR*)id, 5);
	cfg_param.has_ssid = 0;
	save_cfg();
	has_ssid_pwd = FALSE;
}

A_INT32 clear_net(A_INT32 argc, A_CHAR *argv[])
{
	reset_wifi_config();
	printf("clear_net:clear net config\r\n");
}

A_INT32 conwifi(A_INT32 argc, A_CHAR *argv[])
{
	#if 0
	result.pwd = "st3grtb9";
	result.ssid = "ChinaNet-2Cnb";

	connect_route();
	#else
	result.pwd = cfg_param.ioe_pass;
	result.ssid = cfg_param.ioe_ssid;

	
	if(cfg_param.has_ssid == 1)
	{
		set_led_state(LED_SLOW_BLINK);
		connect_route();
		has_ssid_pwd = TRUE;
	}
	else
	{
		set_led_state(LED_FAST_BLINK);
		do_airkiss(1,NULL);
		has_ssid_pwd = TRUE;
	}
	#endif
}

A_BOOL enter_airkiss = FALSE;

A_INT32 do_airkiss(A_INT32 argc, A_CHAR *argv[])
{
	//qcom_printf("enter do_airkiss");
	A_INT32 data = 1;
	A_UINT16 count = 0;
	A_UINT16 ch_skip_mode = 0;
	A_UINT8 k;

	
	airkiss_success = 0;
	airkiss_timeout = 0;
	//conn_route_flag = 0;
	airkiss_finish_flag = 0;
	airkiss_state = AIRKISS_IDLE;
	//pstScanResults = NULL;
	//bScanFinish = 0;
	//nScanCounts = 0;
    //uint32_t ip;
    //uint32_t mask;
    //uint32_t gateway;
	//ip = 0;mask = 0;gateway = 0;
	A_INT32 ret,get_ip_cnt;
	valid_chs = 0;
	get_ip_cnt = 0;

	if(enter_airkiss)
	{
		return 0;
	}

	enter_airkiss = TRUE;
	printf("enter airkiss\r\n");
	
	if(argc ==4)
	{
		if (!A_STRCMP(argv[1], "--connect"))
		{
			A_CHAR *p = argv[2];
			A_CHAR *ssid = argv[3];
			result.pwd = p;
			result.ssid = ssid;
			connect_route();
			return 0;
		}
	}
	else
	{
		if(argc == 2)
		{
			set_channel(argv[1]);
			debug_set_channel_flag = 0;
		}
		else if(argc == 3)
		{
			A_SSCANF(argv[2], "%d", &data);
		}
		while(!airkiss_success)
		{
			/*if(airkiss_timeout)
			{
				printf("airkiss_timeout = %d\r\n",airkiss_timeout);
				break;
			}*/
			switch(airkiss_state)
			{
				case AIRKISS_IDLE:
					 bDisableScan = 1;
					//nScanCounts = swat_iwconfig_scan_handle(1,NULL);
					if(nScanCounts == 0)
					{
						printf("no scan or scan not get ap\r\n");
						ch_skip_mode = 0;
					}
					else
					{
						ch_skip_mode = 1;
					}
					start_airkiss();
					airkiss_state = AIRKISS_SEARCHING;
				break;
				case AIRKISS_SEARCHING:
					qcom_thread_msleep(200*data);
					if(airkiss_finish_flag == 1)
					{
						airkiss_state = AIRKISS_CONNECTING;
					}
					else if(airkiss_finish_flag != 2)
					{
						if(ch_skip_mode== 0)
						{
							if (cur_channel >= 13)
							{
								cur_channel = 1;
							}
							else
							{
								cur_channel++;
							}
						}
						else
						{
							if(valid_chs == 0)
							{
								for(k=0;k<nScanCounts;k++)
								{
									int needAdd;
									needAdd = 0;
									if(k == 0)
									{
										needAdd=1;
									}
									else
									{
										int m;
										int unmatch_count;
										unmatch_count = 0;
										for(m=0;m<valid_chs;m++)
										{
											if(channels_merge[m] != pstScanResults[k].channel)
											{
												unmatch_count++;
											}
											else
											{
												break;
											}
										}

										if(unmatch_count == valid_chs)
										{
											needAdd=1;
										}
									}
									
									if(needAdd)
									{
	
										channels_merge[valid_chs] = pstScanResults[k].channel;
										valid_chs++;
									}
								}
							}

							
							cur_channel = channels_merge[count];
							count++;
							if(count >= valid_chs)
							{
								count = 0;
							}
						}
						
						if(debug_set_channel_flag)
						{
							//qcom_printf("cur_channel = %d\r\n",cur_channel);
							set_channel(channel_list[cur_channel-1]);
							ret = airkiss_change_channel(&akcontex);
							if(ret < 0)
							{
								qcom_printf("airkiss_change_channel ret = %d\r\n",ret);
							}
						}

					}
				break;
				case AIRKISS_CONNECTING:
					set_led_state(LED_SLOW_BLINK);
					connect_route();
					airkiss_state = AIRKISS_RESPONING;
				break;
				case AIRKISS_RESPONING:
					if(conn_route_flag == 1)
					{
						//ret = qcom_ip_address_get(0, &ip, &mask, &gateway);
						//if(ret == A_OK && ip > 0 && mask > 0 && gateway > 0)
						{
							airkiss_response((A_CHAR *)&(result.random),1);
							airkiss_state = AIRKISS_IDLE;
							airkiss_success = 1;
							//conn_route_flag = 1;
						}
						/*else
						{
							get_ip_cnt++;
							if(get_ip_cnt>20)
							{
								airkiss_state = AIRKISS_IDLE;
								airkiss_success = 1;
								qcom_printf("get ip timeout\r\n");
							}
						}*/
					}
					else
					{
						//if(ip>0)
						//{
							//qcom_printf("ip=%d,mask=%d,gateway=%d\r\n",ip,mask,gateway);
						//}

						qcom_thread_msleep(100);
					}
				break;
				default:
					qcom_printf("not define airkiss state\r\n");
				break;
			}
		}
		//qcom_timer_stop(&time_serv);
		//qcom_timer_delete(&time_serv);
		/*if(pstScanResults)
		{
			free(pstScanResults);
			pstScanResults = NULL;
		}*/
		if(airkiss_success)
		{
			qcom_printf("airkiss success\r\n");
			ioe_wifi_config_set(result.ssid,result.pwd);
		}
		/*if(airkiss_timeout)
		{
			qcom_promiscuous_enable(0);
			qcom_printf("airkiss timeout\r\n");
			set_led_state(LED_OFF);
		}*/
	}

	printf("airkiss exit\r\n");
	
	enter_airkiss = FALSE;
	return 0;
}


