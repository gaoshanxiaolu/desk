#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
//#include <stdio.h>
#include <math.h>
#include <stdlib.h>
#include "wx_airkiss.h"
#include "chair_recv_uart_task.h"
#include "csr1k_host_boot.h"

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

extern int down_firmware_ok;
extern int down_firmware_len;
extern int main_ver;
extern int second_ver;
extern int recv_crc32_std_value;
static A_UINT32 s_addr_local=0;

int upgrade_status;//0 idle,1 upgrade on going, 2 upgrade success 3 upgrade failed

void download(unsigned long  s_addr,int main_v, int second_v)
{
	A_CHAR *cmd_name = "wmiconfig";
	A_CHAR *cmd_p1 = "--ota_upgrade";
	A_CHAR *cmd_p5 = "0";
	A_CHAR *config_cmd[7];
	A_CHAR file_name[64];
	A_CHAR ip_s[32];
	int i;

	memset(file_name,0,64);
	memset(ip_s,0,32);

	qcom_sprintf(file_name,"hehedesk_smartdesk_v%d.%d.bin",main_v,second_v);
	qcom_sprintf(ip_s,"%s", (char *)_inet_ntoa(s_addr));
	
	config_cmd[0] = cmd_name;
	config_cmd[1] = cmd_p1;
	config_cmd[2] = ip_s;
	config_cmd[3] = file_name;
	config_cmd[4] = cmd_p5;
	config_cmd[5] = cmd_p5;
	config_cmd[6] = cmd_p5;
	for(i=0;i<7;i++)
		printf("%s ",config_cmd[i]);
	printf("\r\n");
	swat_wmiconfig_handle(7,config_cmd);

}

static unsigned int crc32_tab[] =
{
    0x00000000L, 0x77073096L, 0xee0e612cL, 0x990951baL, 0x076dc419L,
    0x706af48fL, 0xe963a535L, 0x9e6495a3L, 0x0edb8832L, 0x79dcb8a4L,
    0xe0d5e91eL, 0x97d2d988L, 0x09b64c2bL, 0x7eb17cbdL, 0xe7b82d07L,
    0x90bf1d91L, 0x1db71064L, 0x6ab020f2L, 0xf3b97148L, 0x84be41deL,
    0x1adad47dL, 0x6ddde4ebL, 0xf4d4b551L, 0x83d385c7L, 0x136c9856L,
    0x646ba8c0L, 0xfd62f97aL, 0x8a65c9ecL, 0x14015c4fL, 0x63066cd9L,
    0xfa0f3d63L, 0x8d080df5L, 0x3b6e20c8L, 0x4c69105eL, 0xd56041e4L,
    0xa2677172L, 0x3c03e4d1L, 0x4b04d447L, 0xd20d85fdL, 0xa50ab56bL,
    0x35b5a8faL, 0x42b2986cL, 0xdbbbc9d6L, 0xacbcf940L, 0x32d86ce3L,
    0x45df5c75L, 0xdcd60dcfL, 0xabd13d59L, 0x26d930acL, 0x51de003aL,
    0xc8d75180L, 0xbfd06116L, 0x21b4f4b5L, 0x56b3c423L, 0xcfba9599L,
    0xb8bda50fL, 0x2802b89eL, 0x5f058808L, 0xc60cd9b2L, 0xb10be924L,
    0x2f6f7c87L, 0x58684c11L, 0xc1611dabL, 0xb6662d3dL, 0x76dc4190L,
    0x01db7106L, 0x98d220bcL, 0xefd5102aL, 0x71b18589L, 0x06b6b51fL,
    0x9fbfe4a5L, 0xe8b8d433L, 0x7807c9a2L, 0x0f00f934L, 0x9609a88eL,
    0xe10e9818L, 0x7f6a0dbbL, 0x086d3d2dL, 0x91646c97L, 0xe6635c01L,
    0x6b6b51f4L, 0x1c6c6162L, 0x856530d8L, 0xf262004eL, 0x6c0695edL,
    0x1b01a57bL, 0x8208f4c1L, 0xf50fc457L, 0x65b0d9c6L, 0x12b7e950L,
    0x8bbeb8eaL, 0xfcb9887cL, 0x62dd1ddfL, 0x15da2d49L, 0x8cd37cf3L,
    0xfbd44c65L, 0x4db26158L, 0x3ab551ceL, 0xa3bc0074L, 0xd4bb30e2L,
    0x4adfa541L, 0x3dd895d7L, 0xa4d1c46dL, 0xd3d6f4fbL, 0x4369e96aL,
    0x346ed9fcL, 0xad678846L, 0xda60b8d0L, 0x44042d73L, 0x33031de5L,
    0xaa0a4c5fL, 0xdd0d7cc9L, 0x5005713cL, 0x270241aaL, 0xbe0b1010L,
    0xc90c2086L, 0x5768b525L, 0x206f85b3L, 0xb966d409L, 0xce61e49fL,
    0x5edef90eL, 0x29d9c998L, 0xb0d09822L, 0xc7d7a8b4L, 0x59b33d17L,
    0x2eb40d81L, 0xb7bd5c3bL, 0xc0ba6cadL, 0xedb88320L, 0x9abfb3b6L,
    0x03b6e20cL, 0x74b1d29aL, 0xead54739L, 0x9dd277afL, 0x04db2615L,
    0x73dc1683L, 0xe3630b12L, 0x94643b84L, 0x0d6d6a3eL, 0x7a6a5aa8L,
    0xe40ecf0bL, 0x9309ff9dL, 0x0a00ae27L, 0x7d079eb1L, 0xf00f9344L,
    0x8708a3d2L, 0x1e01f268L, 0x6906c2feL, 0xf762575dL, 0x806567cbL,
    0x196c3671L, 0x6e6b06e7L, 0xfed41b76L, 0x89d32be0L, 0x10da7a5aL,
    0x67dd4accL, 0xf9b9df6fL, 0x8ebeeff9L, 0x17b7be43L, 0x60b08ed5L,
    0xd6d6a3e8L, 0xa1d1937eL, 0x38d8c2c4L, 0x4fdff252L, 0xd1bb67f1L,
    0xa6bc5767L, 0x3fb506ddL, 0x48b2364bL, 0xd80d2bdaL, 0xaf0a1b4cL,
    0x36034af6L, 0x41047a60L, 0xdf60efc3L, 0xa867df55L, 0x316e8eefL,
    0x4669be79L, 0xcb61b38cL, 0xbc66831aL, 0x256fd2a0L, 0x5268e236L,
    0xcc0c7795L, 0xbb0b4703L, 0x220216b9L, 0x5505262fL, 0xc5ba3bbeL,
    0xb2bd0b28L, 0x2bb45a92L, 0x5cb36a04L, 0xc2d7ffa7L, 0xb5d0cf31L,
    0x2cd99e8bL, 0x5bdeae1dL, 0x9b64c2b0L, 0xec63f226L, 0x756aa39cL,
    0x026d930aL, 0x9c0906a9L, 0xeb0e363fL, 0x72076785L, 0x05005713L,
    0x95bf4a82L, 0xe2b87a14L, 0x7bb12baeL, 0x0cb61b38L, 0x92d28e9bL,
    0xe5d5be0dL, 0x7cdcefb7L, 0x0bdbdf21L, 0x86d3d2d4L, 0xf1d4e242L,
    0x68ddb3f8L, 0x1fda836eL, 0x81be16cdL, 0xf6b9265bL, 0x6fb077e1L,
    0x18b74777L, 0x88085ae6L, 0xff0f6a70L, 0x66063bcaL, 0x11010b5cL,
    0x8f659effL, 0xf862ae69L, 0x616bffd3L, 0x166ccf45L, 0xa00ae278L,
    0xd70dd2eeL, 0x4e048354L, 0x3903b3c2L, 0xa7672661L, 0xd06016f7L,
    0x4969474dL, 0x3e6e77dbL, 0xaed16a4aL, 0xd9d65adcL, 0x40df0b66L,
    0x37d83bf0L, 0xa9bcae53L, 0xdebb9ec5L, 0x47b2cf7fL, 0x30b5ffe9L,
    0xbdbdf21cL, 0xcabac28aL, 0x53b39330L, 0x24b4a3a6L, 0xbad03605L,
    0xcdd70693L, 0x54de5729L, 0x23d967bfL, 0xb3667a2eL, 0xc4614ab8L,
    0x5d681b02L, 0x2a6f2b94L, 0xb40bbe37L, 0xc30c8ea1L, 0x5a05df1bL,
    0x2d02ef8dL
};

/* crc32 hash */
uint32_t crc32val = 0xFFFFFFFF;
uint32_t crc32_check;

uint32_t crc32(const unsigned char *s, int len)
{
    int i;

    for (i = 0;  i < len;  i++) {
        crc32val = crc32_tab[(crc32val ^ s[i]) & 0xFF] ^ ((crc32val >> 8) & 0x00FFFFFF);
    }

    return labs(crc32val ^ 0xFFFFFFFF);
}

uint32_t c32_sum = 0;

uint32_t crc32_sum(const unsigned char *s, int len)
{
	int i;
	
    for (i = 0;  i < len;  i++) {
        c32_sum = c32_sum + s[i];
    }
	
	return c32_sum;
}
#define STATE_IDLE 255
int state = 0;
void set_state(int val)
{
	state = val;
}
#define READ_CHECK_LEN 1024
void upgrade_task()
{
	int ret;
	A_CHAR *dns=HEHEDNS;

	int cnt_timeout;
	A_ULONG  offset = 0;
	A_INT32 error = 0;
	A_UINT32 ret_len=0,bytes_to_read;
	 A_UCHAR data[READ_CHECK_LEN];
	 A_UINT32 resp;
	 A_UINT32 s_addr;

    printf("\r\n enter upgrade_task\r\n");

    while(TRUE)
    {
		if(state == 0) //use dns to get ip
		{
			ret = qcom_dnsc_get_host_by_name(dns,&s_addr);
			if(ret == A_OK && s_addr > 0)
			{
				printf("upgrade, Get IP address of host %s\r\n", dns);
				printf("upgrade ip address is %s\r\n", (char *)_inet_ntoa(s_addr));
				state = STATE_IDLE;
			}
			else
			{
				qcom_thread_msleep(500);
				continue;
			}
		}
		else if(state == 1)
		{
			down_firmware_ok = 0;
			upgrade_status = 1;
			download(s_addr_local?s_addr_local:s_addr,main_ver,second_ver);
			state = 2;
			cnt_timeout = 0;
		}
		else if(state == 2)
		{
			if(down_firmware_ok < 0)
			{
				printf("download failed\r\n");
				state = STATE_IDLE;
			}
			else if(down_firmware_ok > 0)
			{
				printf("download successful,downlown len = %d\r\n",down_firmware_len);
				offset = 0;
				crc32val = 0xFFFFFFFF;
				c32_sum = 0;
				state = 3;
			}
			tx_thread_sleep(1000);
			cnt_timeout++;
			if(cnt_timeout > 1800)
			{
				printf("download timeout\r\n");
				state = STATE_IDLE;
			}
		}
		else if(state == 3)
		{
			if(offset + READ_CHECK_LEN < down_firmware_len)
			{
				bytes_to_read = READ_CHECK_LEN;
			}
			else
			{
				bytes_to_read = down_firmware_len - offset;
			}
			
			error = qcom_read_ota_area(offset, bytes_to_read, data, &ret_len);
			if (error) {
				SWAT_PTF("OTA Read Error: %d\n",error);
			}

			if(ret_len != bytes_to_read)
			{
				printf("ret_len != bytes_to_read \r\n");
			}

			crc32_check = crc32(data,ret_len);
			//crc32_check = crc32_sum(data,ret_len);

			offset += ret_len;
			
			printf("check len %d, total len %d\n",offset,down_firmware_len);

			if(offset == down_firmware_len)
			{
				if(crc32_check == recv_crc32_std_value)
				{
					printf("||||||||crc32 check pass\r\n");
					resp = qcom_ota_done(1);
					if (resp == 1) 
					{
						SWAT_PTF("OTA Completed Successfully");
						save_wifi_setting();
						qcom_sys_reset();
					} 
					else 
					{
						SWAT_PTF("OTA Failed Err:%d  \n", resp);
						state = STATE_IDLE;
					}

				}
				else
				{
					printf("--------crc32 check failed %8x,%8x\r\n",crc32_check,recv_crc32_std_value);
					resp = qcom_ota_done(0);
					if (resp == 1) 
					{
						SWAT_PTF("OTA Completed Successfully\n");
					} 
					else 
					{
						SWAT_PTF("OTA Failed Err:%d  \n", resp);
						
					}

					state = STATE_IDLE;

				}
			}

			
			tx_thread_sleep(100);

		}
		else
		{
			down_firmware_ok = 0;
			down_firmware_len = 0;
			upgrade_status = 0;
			cnt_timeout = 0;
			offset = 0;
			

			tx_thread_sleep(1000);
		}
    	
    }

	printf("upgrade_task exit\r\n");
	
	qcom_task_exit();
	
}

void upgrade_ble_task()
{
	int cnt = 0;
	
	printf("\r\n enter upgrade_ble_task\r\n");
	
	while(1)
	{
		send_query_version_cmd();

		tx_thread_sleep(3000);
		
		if(!query_version_flag())
		{
			cnt++;
			#define QUERY_TIMEOUT_CNT 20
			//printf("cnt =%d,dev type %d\n\r",cnt,get_config_dev_type());
			if(cnt >= QUERY_TIMEOUT_CNT)//60s 没有查询到ble设备的返回类型,通过保存ble设备类型，强制升级
			{
				cnt= 0;
				if(get_config_dev_type() == UNDEF)
				{
					printf("query timeout, undef dev type\n\r");
				}
				else if(get_config_dev_type() == C_NODE)
				{
					printf("force upgrade ble to node\n\r");
					//NODE_CSR1Kboot();
					
					printf("upgrade ble node finished\n\r");
				}
				else if(get_config_dev_type() == C_GW)
				{
					printf("force upgrade ble to gw\n\r");
					CSR1Kboot();
					
					printf("upgrade ble  gw finished\n\r");
				
				}

			}
		}
		else
		{
			if(get_dev_type() != get_config_dev_type())
			{
				save_config_dev_type(get_dev_type());
			}

			if(get_dev_type() == C_NODE)
			{
				if(get_main_ver() != BLE_NODE_MAIN_V || get_second_ver() != BLE_NODE_SECOND_V)
				{
					printf("upgrade ble  node\n\r");
					//NODE_CSR1Kboot();
					//clear_version_flag();
					cnt = 0;
					printf("upgrade ble node finished\n\r");
				}
				else
				{
					printf("v%d.%d is equal, not need upgrade\n\r",BLE_NODE_MAIN_V,BLE_NODE_SECOND_V);
				}
			}
			else if(get_dev_type() == C_GW)
			{
				if(get_main_ver() != BLE_GW_MAIN_V || get_second_ver() != BLE_GW_SECOND_V)
				{
					printf("upgrade ble  gw...\n\r");
					CSR1Kboot();
					//clear_version_flag();
					cnt = 0;
					printf("upgrade ble  gw finished\n\r");
				}
				else
				{
					printf("v%d.%d is equal, not need upgrade\n\r",BLE_GW_MAIN_V,BLE_GW_SECOND_V);
				}


			}
			//tx_thread_sleep(1000);
		}
		
		clear_version_flag();
		
	}
}

A_INT32 start_upgrade_task(A_INT32 argc, A_CHAR *argv[])
{


	qcom_task_start(upgrade_task, 2, 2048, 80);
	
}

A_INT32 start_upgrade_ble_task(A_INT32 argc, A_CHAR *argv[])
{


	qcom_task_start(upgrade_ble_task, 2, 2048, 80);
	
}

A_INT32 mannual_start_upgrade_task(A_INT32 argc, A_CHAR *argv[])
{
	if(argc >=5)
	{
		s_addr_local = _inet_addr(argv[1]);
		main_ver = atoi(argv[2]);
		second_ver = atoi(argv[3]);
		recv_crc32_std_value = atoi(argv[4]);
	}
	set_state(1);
}

