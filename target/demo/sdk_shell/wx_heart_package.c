//
#include "qcom_common.h"
#include "qcom_adc_api.h"
#include "adc_layer_wmi.h"
#include "swat_wmiconfig_network.h"

#include "socket_api.h"
#include "swat_parse.h"
#include "wx_heart_package.h"
#include "ble_uart.h"
#include "wx_airkiss.h"
#include "i2c_test.h"
#include "upgrade_task.h"

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

A_UINT8 desk_pos_msg[21] = { 0xc0,
							 0x07,
							 0x17,
							 0x03,
							 0x04,
							 0x05,
							 0x80,
							 0x31,
							 0x09,//len
							 0x07,
							 0x17,
							 0x03,//dev id
							 0x00,
							 0x01,
							 0x00,//pos info
							 0x00,
							 0x00,
							 0x00,
							 0x00,//crc sum
							 0x00,
							 0xc1
};
A_UINT8 desk_temphum_msg[25] = { 0xc0,
							 0x07,
							 0x17,
							 0x03,
							 0x04,
							 0x05,
							 0x80,
							 0x36,
							 0x0d,//len
							 0x07,
							 0x17,
							 0x03,//dev id
							 0x00,
							 0x01,
							 0x00,//temperatue
							 0x00,
							 0x00,
							 0x00,
							0x00,//humitity
							0x00,
							0x00,
							0x00,
							 0x00,//crc sum
							 0x00,
							 0xc1
};
A_UINT8 desk_upgrade_msg[19] = { 0xc0,
							 0x07,
							 0x17,
							 0x03,
							 0x04,
							 0x05,
							 0x80,
							 0x37,
							 0x07,//len
							 0x07,
							 0x17,
							 0x03,//dev id
							 0x00,
							 0x01,
							 0x00,//main ver
							 0x00,//second ver
							 0x00,//crc sum
							 0x00,
							 0xc1
};

A_UINT8 desk_ack_rx_msg[13] = { 
                             0xc0,
							 0x07,
							 0x17,
							 0x03,
							 0x04,
							 0x05,
							 0x80,
							 0x31,
							 0x01,//len
							 0x00,
							 0x00,//crc sum
							 0x00,
							 0xc1
};

void add_pos_info(A_INT32 pos)
{
	desk_pos_msg[14] = pos >> 0;
	desk_pos_msg[15] = pos >> 8;
	desk_pos_msg[16] = pos >> 16;
	desk_pos_msg[17] = pos >> 24;
}
void add_temp_humitity(A_INT32 temp, A_INT32 humitity)
{
	if(temp > 0)
	{
		desk_temphum_msg[14] = temp >> 0;
		desk_temphum_msg[15] = temp >> 8;
		desk_temphum_msg[16] = temp >> 16;
		desk_temphum_msg[17] = temp >> 24;
	}
	else
	{
		temp = temp * -1;
		desk_temphum_msg[14] = temp >> 0;
		desk_temphum_msg[15] = temp >> 8;
		desk_temphum_msg[16] = temp >> 16;
		desk_temphum_msg[17] = 0xff;

	}

	desk_temphum_msg[18] = humitity >> 0;
	desk_temphum_msg[19] = humitity >> 8;
	desk_temphum_msg[20] = humitity >> 16;
	desk_temphum_msg[21] = humitity >> 24;

}

void add_dev_id(A_UINT8 *pid)
{
	desk_pos_msg[9] = pid[0];
	desk_pos_msg[10] = pid[1];
	desk_pos_msg[11] = pid[2];
	desk_pos_msg[12] = pid[3];
	desk_pos_msg[13] = pid[4];
	
	desk_pos_msg[1] = desk_pos_msg[9];
	desk_pos_msg[2] = desk_pos_msg[10];
	desk_pos_msg[3] = desk_pos_msg[11];
	desk_pos_msg[4] = desk_pos_msg[12];
	desk_pos_msg[5] = desk_pos_msg[13];

}
void add_temp_hum_dev_id(A_UINT8 *pid)
{
	desk_temphum_msg[9] = pid[0];
	desk_temphum_msg[10] = pid[1];
	desk_temphum_msg[11] = pid[2];
	desk_temphum_msg[12] = pid[3];
	desk_temphum_msg[13] = pid[4];

	desk_temphum_msg[1] = desk_temphum_msg[9];
	desk_temphum_msg[2] = desk_temphum_msg[10];
	desk_temphum_msg[3] = desk_temphum_msg[11];
	desk_temphum_msg[4] = desk_temphum_msg[12];
	desk_temphum_msg[5] = desk_temphum_msg[13];

}
void add_upgrade_dev_id(A_UINT8 *pid)
{
	desk_upgrade_msg[9] = pid[0];
	desk_upgrade_msg[10] = pid[1];
	desk_upgrade_msg[11] = pid[2];
	desk_upgrade_msg[12] = pid[3];
	desk_upgrade_msg[13] = pid[4];

	desk_upgrade_msg[1] = desk_upgrade_msg[9];
	desk_upgrade_msg[2] = desk_upgrade_msg[10];
	desk_upgrade_msg[3] = desk_upgrade_msg[11];
	desk_upgrade_msg[4] = desk_upgrade_msg[12];
	desk_upgrade_msg[5] = desk_upgrade_msg[13];

}

void add_dev_ack_id(A_UINT8 *pid)
{
	desk_ack_rx_msg[1] = pid[0];
	desk_ack_rx_msg[2] = pid[1];
	desk_ack_rx_msg[3] = pid[2];
	desk_ack_rx_msg[4] = pid[3];
	desk_ack_rx_msg[5] = pid[4];
}
void delay_us(A_INT32 nus)
{		

	A_INT32 i,j;
	for(i=0;i<nus*20;i++)
	{
		j++;
	}
}

void delay_ms_(A_INT32 nms)
{
	A_INT32 i;
	for(i=0;i<nms;i++) delay_us(1000);
}

void add_crc_sum(void)
{
	int i;
	A_UINT16 sum;

	sum = 0;
	for(i=1;i<sizeof(desk_pos_msg) - 3;i++)
	{
		sum += desk_pos_msg[i];
	}

	desk_pos_msg[18] = sum >> 8;
	desk_pos_msg[19] = sum >> 0;

}
A_UINT16 crc_sum(A_UINT8 *data, A_UINT16 len)
{
	int i;
	A_UINT16 sum;

	sum = 0;
	for(i=0;i<len;i++)
	{
		sum += data[i];
	}

    return sum;

}
enum MX_SIGNEL pos_map_index[4] = { X1,X2,X3,X4};
enum MX_SIGNEL rec_pos_map_index[4] = { M1,M2,M3,M4};
A_UINT8 is_need_ack = FALSE;


A_INT32 desk_test(A_INT32 argc, A_CHAR *argv[])
{
    if(argc < 2)
    {
        qcom_printf("desk_test m1/m2/m3/m4 or 1/2/3/4\r\n");    
        return -1;
    }
    
    if(!A_STRCMP(argv[1],"1"))
    {
        set_mx_sig_val(pos_map_index[0]);
    }
	else if(!A_STRCMP(argv[1],"2"))
	{
		set_mx_sig_val(pos_map_index[1]);
    }
	else if(!A_STRCMP(argv[1],"3"))
	{
		set_mx_sig_val(pos_map_index[2]);
    }
	else if(!A_STRCMP(argv[1],"4"))
	{
		set_mx_sig_val(pos_map_index[3]);
    }
	else if(!A_STRCMP(argv[1],"m1"))
	{
		set_mx_sig_val(rec_pos_map_index[0]);
    }
	else if(!A_STRCMP(argv[1],"m2"))
	{
		set_mx_sig_val(rec_pos_map_index[1]);
    }
	else if(!A_STRCMP(argv[1],"m3"))
	{
		set_mx_sig_val(rec_pos_map_index[2]);
    }
	else if(!A_STRCMP(argv[1],"m4"))
	{
		set_mx_sig_val(rec_pos_map_index[3]);
    }
	else
	{
        qcom_printf("desk_test m1/m2/m3/m4 or 1/2/3/4\r\n");    
        return -1;

	}

	return 0;
	
}

void print_debug(A_UINT8 *data, A_UINT16 len)
{
    int i;

    for(i=0;i<len;i++)
    {
        printf("0x%02x,",data[i]);
    }
    printf("\r\n");
}
 int main_ver;
 int second_ver;
 int recv_crc32_std_value;
extern int upgrade_status;
extern int need_move_desk;
int parse_rx_data(A_UINT8 *data, A_UINT16 len )
{
	int tmp_m,tmp_s;
	unsigned int tmp32;
	//printf("recv : len = %d,",len);
	//print_debug(data,len);
	
	is_need_ack = FALSE;
    
	if(data[0] != 0xc0)
	{
		printf("check frame head failed\r\n");
		return 7;
	}

	if(data[len - 1] != 0xc1)
	{
		printf("check frame tail failed\r\n");
		return 7;
	}

	if(len != 18 && len != 13 && len != 12 && len != 23)
	{
		printf("check frame len failed\r\n");
		return 7;
	}

	A_UINT8 cmd,param;

	param = 0;

	if(len == 18)
	{
		param = data[14];
	}
	else if(len == 13 || len == 12)
	{
		param = data[9];
	}
	else
	{
		print_debug(data,len);
	}
	
	cmd = data[7];
	
	if(cmd == 0x30)
	{
		is_need_ack = TRUE;
		printf("recv cmd, to goto pos %d\r\n",param);
		desk_ack_rx_msg[7] = 0xb0;

		if(get_desk_run_state() == UP_DESK || get_desk_run_state() == DOWN_DESK )
		{
			return 1;
		}
		if(param <= 4 && param > 0)
		{
			set_mx_sig_val(pos_map_index[param-1]);
			return 0;
		}
		return 7;
		
	}
	else if(cmd == 0x32)
	{
		desk_ack_rx_msg[7] = 0xb2;
		is_need_ack = TRUE;
		printf("recv cmd, record pos %d\r\n",param);
		if(get_desk_run_state() == UP_DESK || get_desk_run_state() == DOWN_DESK )
		{
			return 1;
		}

		if(param <= 4 && param > 0)
		{
			set_mx_sig_val(rec_pos_map_index[param-1]);
			
			return 0;
		}
		return 7;

	}
	else if(cmd == 0x33)
	{
		//desk up
		desk_ack_rx_msg[7] = 0xb3;
		is_need_ack = TRUE;
		printf("recv cmd, desk up\r\n");
		if(get_desk_run_state() == UP_DESK)
		{
			return 1;
		}
		else if(get_desk_run_state() == DOWN_DESK)
		{
			stop_desk();
			printf("stop_desk\r\n");
			delay_ms_(10);
		}

		up_desk();
		printf("up_desk\r\n");

		return 0;

	}
	else if(cmd == 0x34)
	{
		//desk down
		desk_ack_rx_msg[7] = 0xb4;
		is_need_ack = TRUE;
		printf("recv cmd, desk down\r\n");
		if(get_desk_run_state() == DOWN_DESK)
		{
			return 1;
		}
		else if(get_desk_run_state() == UP_DESK)
		{
			stop_desk();
			printf("stop_desk\r\n");
			delay_ms_(10);
		}

		down_desk();
		printf("down_desk\r\n");

		return 0;

	}
	else if(cmd == 0x35)
	{
		//stop down
		desk_ack_rx_msg[7] = 0xb5;
		is_need_ack = TRUE;
		printf("recv cmd, desk stop\r\n");
		if(get_desk_run_state() == STOP)
		{
			return 1;
		}
		else
		{
			stop_desk();
			printf("stop_desk\r\n");
		}
		
		return 0;

	}
	else if(cmd == 0x38)
	{
		desk_ack_rx_msg[7] = 0xb8;
		is_need_ack = TRUE;
		if(get_desk_run_state() != STOP)
		{
			printf("isruning, cancel setdown timeout\r\n");
			return 1;
		}
		need_move_desk = 1;
		printf("setdown timeout\r\n");
	}

	else if(cmd == 0xb1)
	{
		//printf("recv weixin server desk pos report ack , ret = %d\r\n",param);
		return 0;
	}
	else if(cmd == 0xb6)
	{
		//printf("recv weixin server desk temperatue and humitity ack , ret = %d\r\n",param);
		return 0;
	}
	else if(cmd == 0xb7)
	{
		tmp_m = data[14];
		tmp_s = data[15];
		tmp32 = data[16] << 24 | data[17] << 16 | data[18] << 8 | data[19];
		printf("*********v%d.%d,%8x\n",tmp_m,tmp_s,tmp32);
		if(tmp_m > SMART_DESK_MAIN_V || tmp_s > SMART_DESK_SECOND_V)
		{
			if(upgrade_status != 1)
			{
				main_ver = tmp_m;
				second_ver = tmp_s;
				recv_crc32_std_value = tmp32;
				set_state(1);
				//start_upgrade_task(1,NULL);
			}
		}
		else
		{
			printf("no need upgrade\n");
		}
	}

	return 7;
	
	
}
int conn_serv_ok = 0;
A_INT32 socketLocal=0;

void desk_send_socket_thread()
{
	int res;
	int cnt;

	cnt = 0;
	SWAT_PTF("enter desk send socket with weixin\r\n");
	
	while(1)
	{
		if(conn_serv_ok && socketLocal)
		{
			add_pos_info(get_desk_pos_value());
			add_crc_sum();
			static int pri1=0;
			if(pri1 ++ > 13)
			{
				pri1 = 0;
				printf("send pos : ");
				print_debug(desk_pos_msg,sizeof(desk_pos_msg));
			}
			res = qcom_send(socketLocal, (char *)desk_pos_msg,sizeof(desk_pos_msg), 0);
			if(res < 0)
			{
				printf("send error res = %d\n",res);
				qcom_socket_close(socketLocal);
				conn_serv_ok = 0;
				continue;
			}

			add_temp_humitity(get_temperatue(),get_humidity());
			//add_crc_sum();
			static int print_cnt=0;
			if(print_cnt++ > 18)
			{
				print_cnt = 0;
				printf("send temp_humitity : ");
				print_debug(desk_temphum_msg,sizeof(desk_temphum_msg));
			}
			res = qcom_send(socketLocal, (char *)desk_temphum_msg,sizeof(desk_temphum_msg), 0);
			if(res < 0)
			{
				printf("send error res = %d\n",res);
				qcom_socket_close(socketLocal);
				conn_serv_ok = 0;
				continue;
			}

			if(cnt == 0)
			{
				cnt = 600;
				desk_upgrade_msg[14] = SMART_DESK_MAIN_V;
				desk_upgrade_msg[15] = SMART_DESK_SECOND_V;
				printf("send desk_upgrade_msg : ");
				print_debug(desk_upgrade_msg,sizeof(desk_upgrade_msg));

				res = qcom_send(socketLocal, (char *)desk_upgrade_msg,sizeof(desk_upgrade_msg), 0);
				if(res < 0)
				{
					printf("send error res = %d\n",res);
					qcom_socket_close(socketLocal);
					conn_serv_ok = 0;
					continue;
				}

			}

			cnt--;

		}
		
		tx_thread_sleep(1500);

		
	}

}

void desk_recv_socket_thread()
{
	//char *dns = "iot.nebulahightech.com";
	//char *ip = "139.196.182.167";
	//char *ip = "106.14.143.71";
	//char *dns = "airkiss.nebulahightech.com";
	//char *ip = "120.26.128.165";
	char *dns=HEHEDNS;
	
	int receive_len;
	int res;
	struct sockaddr_in remoteAddr;
	int ret;
	//A_UINT32 aidata[4];
    uint32_t ip1;
    uint32_t mask;
    uint32_t gateway;
	A_UINT32 s_addr;
	A_UINT8 msgRecv[50];
    A_UINT8 macAddr[6];
    A_UINT8 dhcp_ok = FALSE;
	A_UINT32 connect_route_cnt;
	A_UINT32 has_connect_ok_failed;
	A_UINT8 get_ip_ok= FALSE;

	connect_route_cnt = 0;
	//ret = swat_sscanf(ip, "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
	//s_addr = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
	swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
	remoteAddr.sin_addr.s_addr = htonl(s_addr);
	remoteAddr.sin_port = htons(12345);
	remoteAddr.sin_family = AF_INET;
	socketLocal = 0;
	has_connect_ok_failed = 0;
	qcom_mac_get(0, (A_UINT8 *) & macAddr);
    //mac_sum = crc_sum(macAddr,6);
    //SWAT_PTF("dev id is %2x %2x %2x %2x %2x %2x\r\n",macAddr[0],macAddr[1],macAddr[2],macAddr[3],macAddr[4],macAddr[5]);
    //add_dev_id(&macAddr[3]);
	char id[8];
	get_dev_id(id);
	if(id[0] == 0xff && id[1] == 0xff && id[2] == 0xff && id[3] == 0xff && id[4] == 0xff)
	{
		id[0] = 0x07;
		id[1] = 0x17;
		id[2] = macAddr[3];
		id[3] = macAddr[4];
		id[4] = macAddr[5];
	}
	add_dev_id((A_UINT8 *)id);
	add_temp_hum_dev_id((A_UINT8 *)id);
	add_upgrade_dev_id((A_UINT8 *)id);
	add_dev_ack_id((A_UINT8 *)id);
	SWAT_PTF("enter desk recv socket with weixin\r\n");
	while(1)
	{
        if(!dhcp_ok)//check router malloc ip addr to wifi module
        {
    		ret = qcom_ip_address_get(0, &ip1, &mask, &gateway);
    		if(ret != A_OK || ip1 == 0 || mask == 0 || gateway == 0)
    		{
    			tx_thread_sleep(1000);
				/*connect_route_cnt++;
				if(connect_route_cnt > 60 && is_has_ssid_pwd())
				{
					connect_route_cnt = 0;
					reset_wifi_config();
					printf("reset wifi\r\n");
					//conwifi(1,NULL);
					qcom_sys_reset();
				}*/
    			continue;
    		}
            dhcp_ok = TRUE;
			//set_led_state(LED_ON);
        }

		if(!get_ip_ok) //use dns to get ip
		{
			ret = qcom_dnsc_get_host_by_name(dns,&s_addr);
			if(ret == A_OK && s_addr > 0)
			{
				get_ip_ok = TRUE;
				remoteAddr.sin_addr.s_addr = htonl(s_addr);
				printf("Get IP address of host %s\r\n", dns);
				printf("ip address is %s\r\n", (char *)_inet_ntoa(s_addr));

			}
			else
			{
				qcom_thread_msleep(500);
				continue;
			}
		}
		

		if(0 == conn_serv_ok)
		{
			socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
			if(socketLocal == A_ERROR)
			{
				printf("desk socket thread create socket failed\r\n");
				tx_thread_sleep(1000);
				continue;

			}

			ret = swat_connect(socketLocal, (struct sockaddr *) &remoteAddr,
							 sizeof (struct  sockaddr_in));
			
			if (ret < 0) 
			{
				/* Close Socket */
				SWAT_PTF("Connect Failed\r\n");
				qcom_socket_close(socketLocal);
				conn_serv_ok = 0;
				tx_thread_sleep(1000);
				has_connect_ok_failed++;
				if(has_connect_ok_failed > 60)
				{
					has_connect_ok_failed = 0;
					//reset_wifi_config();
					//printf("conect 60 failed reset wifi\r\n");
					//conwifi(1,NULL);
					//qcom_sys_reset();
				}
				continue;
			}
			has_connect_ok_failed = 0;

			//qcom_setsockopt(socketLocal,SOL_SOCKET,SO_RCVTIMEO,(char *)&rx_timeout,sizeof(A_INT32));
			//struct timeval timeout;
			//timeout.tv_sec=0;
			//timeout.tv_usec=1000*100;
			//qcom_setsockopt(socketLocal,SOL_SOCKET,SO_RCVTIMEO,(char *)&timeout,sizeof(struct timeval));
			
			conn_serv_ok = 1;
		}

		if(conn_serv_ok)
		{
			receive_len = qcom_recv(socketLocal,(char *)msgRecv,23,0);
	        if (receive_len > 0)
			{        
				ret = parse_rx_data(msgRecv,receive_len);
				desk_ack_rx_msg[9] = ret;
				if(is_need_ack)
				{
					//printf("ack : ");
					//print_debug(desk_ack_rx_msg,sizeof(desk_ack_rx_msg));
                    res = qcom_send(socketLocal, (char *)desk_ack_rx_msg,sizeof(desk_ack_rx_msg), 0);
                    if(res < 0)
                    {
                        printf("ack send error res = %d\n",res);
                        qcom_socket_close(socketLocal);
                        conn_serv_ok = 0;
                        continue;
                    }
				}
			}
            else if (receive_len < 0)
            {
                printf("desk_recv_socket_thread: qcom_recv recv error\r\n");
				qcom_socket_close(socketLocal);
				conn_serv_ok = 0;
				continue;

            }
			else
			{
				static int rx_cnt = 0;
				if(rx_cnt++ > 10)
				{
					rx_cnt = 0;
					printf("desk socket read timeout\r\n");
				}
			}
		}
		
	}
	
	if(socketLocal)
	{
		qcom_socket_close(socketLocal);
	}

	qcom_task_exit();
	
}
A_INT32 start_desk_socket_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(desk_recv_socket_thread, 2, 2048, 50);
	qcom_task_start(desk_send_socket_thread, 2, 2048, 50);
}


