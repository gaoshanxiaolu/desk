//
#include "qcom_common.h"
#include "swat_parse.h"
#include "wx_airkiss.h"
#include "ble_uart.h"

extern A_INT32 swat_iwconfig_scan_handle(A_INT32 argc, A_CHAR * argv[]);
extern SCAN_RESULTS pstScanResults[100];
extern A_INT8 bDisableScan;

#ifdef ENABLE_AIRKISS 
extern int do_airkiss(int argc, char *argv[]);
#endif
extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();
extern int sensor_demo_app2(int type);
void ad_key_thread()
{
	/*A_CHAR *cmd_name = "adc";
	A_CHAR *cmd_p1 = "light_demo2";
	A_CHAR *config_cmd[2];
	
	config_cmd[0] = cmd_name;
	config_cmd[1] = cmd_p1;*/
	/*int key_adc_value;
	A_INT8 key_adc_state = 3;
	A_INT8 ad_count_threshold = 8;
	int ad_threshold = 500;
	A_INT8 key_pushdown = 0;
	A_INT16 ad_ref_value = 0;
	A_INT8 ad_ref_count = 0;*/
	A_INT8 delay_for_conwifi = 3;

	while(delay_for_conwifi--)
	{
		qcom_thread_msleep(100);
	}
	conwifi(1,NULL);
	
	while(1)
	{
		/*key_adc_value = sensor_demo_app2(1);
		switch(key_adc_state)
		{
			case 0:
				if(key_adc_value < ad_ref_value - ad_threshold)
				{
					key_adc_state = 1;
				}
			break;
			case 1:
				if(key_adc_value < ad_ref_value - ad_threshold)
				{
					key_adc_state = 2;
				}
			break;
			case 2:
				if(key_adc_value > ad_ref_value - ad_threshold)
				{
					key_adc_state = 0;
					key_pushdown = 1;
				}
			break;
			case 3:
				if(ad_ref_count < ad_count_threshold)
				{
					ad_ref_count++;
					ad_ref_value += key_adc_value;
				}
				else
				{
					ad_ref_count = 0;
					ad_ref_value =  (ad_ref_value / ad_count_threshold);
					key_adc_state = 0;
					qcom_printf("ref ad value = %d, threshold = %d\r\n",ad_ref_value,ad_threshold);
				}
			break;
			default:
				qcom_printf("ad key not define state\r\n");
			break;

		}
		if(key_pushdown)
		{
			key_pushdown = 0;
			qcom_printf("key_pushdown \r\n");
#ifdef ENABLE_AIRKISS 
			//set_led_state(LED_FAST_BLINK);
			//do_airkiss(1,NULL);
			qcom_printf("exit airkiss +++++key_pushdown \r\n");

#endif
		}*/
		
		qcom_thread_msleep(5000);
	}
	
	qcom_task_exit();
}
void scan_thread()
{
	A_INT32 scan_count;
	A_INT16 i,j,count,isStartKeyApp;
	count = 0;
	isStartKeyApp = 0;
	memset(pstScanResults,0,sizeof(pstScanResults[0])*100);
	while(!bDisableScan)
	{
		scan_count = swat_iwconfig_scan_handle(1,NULL);


		qcom_printf("scan_counts = %d \r\n",scan_count);

		for(i=0;i<scan_count;i++)
		{
			qcom_printf("ssid = ");
			{
				for(j = 0;j <pstScanResults[i].ssid_len;j++)
				{
					printf("%c",pstScanResults[i].ssid[j]);
				}
				qcom_printf("\n");
			}

			qcom_printf("channel = %d\r\n",pstScanResults[i].channel);
			//qcom_printf("count = %d\r\n",pstScanResults[i].count);
			//qcom_printf("rssi = %d\r\n",pstScanResults[i].rssi);
			qcom_printf("\r\n");

		}
		count++;
		if(count == 3 && !isStartKeyApp)
		{
			isStartKeyApp = 1;
			qcom_task_start(ad_key_thread, 2, 2048, 50);
		}
		//qcom_thread_msleep(100);
	}
	
	qcom_task_exit();
}

void ad_key_app(void)
{
	#if 1
	qcom_task_start(ad_key_thread, 2, 1024, 100);
	#else
	qcom_task_start(scan_thread, 2, 2048, 50);
	#endif
}


