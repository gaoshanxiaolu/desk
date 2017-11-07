/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#include "qcom_common.h"
#include "qcom_adc_api.h"
#include "adc_layer_wmi.h"

#include "socket_api.h"
#include "swat_parse.h"

extern adc_context_t aCtxt;
extern TX_EVENT_FLAGS_GROUP wmi_adc_event;

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

#define MULTI_CHANNEL 1
#define BUF_LEN 1400
#define ADC_DEBUG 0
int adc_open()
{
#if MULTI_CHANNEL
    ADC_CHAN_CFG_T adc_channel[8];
    adc_channel[0].adch = 0;
    adc_channel[0].input_type = 0;

    adc_channel[1].adch = 1;
    adc_channel[1].input_type = 0;

    adc_channel[2].adch = 2;
    adc_channel[2].input_type = 0;

    adc_channel[3].adch=3;
    adc_channel[3].input_type = 0;

    adc_channel[4].adch = 4;
    adc_channel[4].input_type = 0;

    adc_channel[5].adch = 5;
    adc_channel[5].input_type = 0;

    adc_channel[6].adch = 6;
    adc_channel[6].input_type = 0;

    adc_channel[7].adch = 7;
    adc_channel[7].input_type = 0;
#else

    ADC_CHAN_CFG_T adc_channel[1];
    adc_channel[0].adch = 0;
    adc_channel[0].input_type=0;

#endif

    qcom_adc_init((A_UINT8)GAIN_SCALE_1V8, (A_UINT8)10, (A_UINT8)ADC_FREQ_25_KHZ, (A_UINT8)SINGLE_ENDED, FALSE);

#if MULTI_CHANNEL
    //multi channel
    qcom_adc_config(1, 0, 1, adc_channel, 8);

#else
    //single channel
    qcom_adc_config(0, 0, 1, adc_channel, 1); 
#endif

    //qcom_adc_timer_config(0x13c, TIME_UNIT_10MS);

    qcom_adc_dma_config(500, 11156000);
    //qcom_adc_dma_config(500, 18200);

    //qcom_adc_calibration(10, SINGLE_ENDED);

	
}

int adc_conv()
{
    qcom_adc_conversion(TRUE);
}

unsigned long  s_addr = 0;
u_short  sin_port = 0;

int adc_server_init(int argc, char *argv[])
{	
    A_INT32 ret = 0;
    A_UINT32 aidata[4];
    if (((void *) 0 == argv) || (argc < 4)) {
        return;
    }

    ret = swat_sscanf(argv[2], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
    if (ret < 0) {
       return;
    }

    s_addr = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];

    /* Port */
    sin_port = swat_atoi(argv[3]);

    SWAT_PTF("server ");
    IPV4_ORG_PTF(s_addr);
    SWAT_PTF("  %d\r\n",sin_port);
	
}

void adc_tcp_app_test()
{
    A_UINT32* len = NULL;
    A_UINT8* done = NULL;
    A_UINT8* more = NULL;
    A_UINT8* adc_buf = NULL;
    ULONG flags;
    int index = 0;
    A_INT32 socketLocal;

    A_UINT32 read_cnt = 0;
    struct sockaddr_in remoteAddr;
    int ret;

    if(s_addr==0 || sin_port==0)
    	return;

    len = swat_mem_malloc(sizeof(A_UINT32));
    done = swat_mem_malloc(sizeof(A_UINT8));
    more = swat_mem_malloc(sizeof(A_UINT8));
    adc_buf = swat_mem_malloc(BUF_LEN);

    if (NULL == adc_buf) {
        SWAT_PTF("ADC RX data buffer malloc error\r\n");
        return;
    }

    socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);

    swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
    remoteAddr.sin_addr.s_addr = htonl(s_addr);
    remoteAddr.sin_port = htons(sin_port);
    remoteAddr.sin_family = AF_INET;
    ret = swat_connect(socketLocal, (struct sockaddr *) &remoteAddr,
                     sizeof (struct  sockaddr_in));

    if (ret < 0) {
        /* Close Socket */
        SWAT_PTF("Connect Failed\r\n");
        //qcom_socket_close(socketLocal);
        goto QUIT;
    }

    qcom_adc_conversion(TRUE);

    while(1)
    {		
    	tx_event_flags_get(&wmi_adc_event, 0x10, TX_OR_CLEAR, &flags, TX_WAIT_FOREVER);
    	
    	if(aCtxt.convErr || *done)
    	{
            SWAT_PTF("convErr %d  %d \n",aCtxt.convErr,aCtxt.convDone);
            goto QUIT;
    	}
    	read_cnt=0;
    	
    	do{
            //SWAT_PTF("adc app read data\n");
            read_cnt++;
            for(index=0;index<aCtxt.channelCnt;index++)
            {
                qcom_adc_recv_data(index,adc_buf,BUF_LEN,len,more,done);

                if(*len>0)
                {
                    //send out data via wifi
                    swat_send(socketLocal, (char*) adc_buf,*len, 0);
                }
            }		
            if(*done)
            {
                goto QUIT;
            }
            if(*more==0)
            {
                //goto NEXT;
                break;	
            }
            if(read_cnt>100)
            {	
                qcom_thread_msleep(1);
                read_cnt=0;
            }
    	}while(*len>0);
    }

    QUIT:
    SWAT_PTF("adc tcp app quit\n");
    swat_mem_free(adc_buf);
    swat_mem_free(len);
    swat_mem_free(done);
    swat_mem_free(more);
    swat_close(socketLocal);
    qcom_task_exit();
	
}


void adc_udp_app_test()
{
    A_UINT32* len = NULL;
    A_UINT8* done = NULL;
    A_UINT8* more = NULL;
    A_UINT8* adc_buf = NULL;
    ULONG flags;
    int index = 0;
    A_INT32 socketLocal;

    struct sockaddr_in remoteAddr;
    struct sockaddr_in localAddr;
    A_UINT32 ipAddress;
    A_UINT32 submask;
    A_UINT32 gateway;
    A_INT32 fromSize = 0;

    if(s_addr==0 || sin_port==0)
    	return;

    len = swat_mem_malloc(sizeof(A_UINT32));
    done = swat_mem_malloc(sizeof(A_UINT8));
    more = swat_mem_malloc(sizeof(A_UINT8));
    adc_buf = swat_mem_malloc(BUF_LEN);

    if (NULL == adc_buf) {
        SWAT_PTF("ADC RX data buffer malloc error\r\n");
        return;
    }

    socketLocal = swat_socket(PF_INET, SOCK_DGRAM, 0);

    swat_mem_set(&localAddr, 0, sizeof(struct sockaddr_in));
    qcom_ipconfig(currentDeviceId, IP_CONFIG_QUERY, &ipAddress, &submask, &gateway);
    localAddr.sin_addr.s_addr = htonl(ipAddress);
    localAddr.sin_family      = AF_INET;
    /*bind addr when in concurrency mode*/
    if((swat_bind(socketLocal, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in))) < 0){         
        /* Close Socket */
        SWAT_PTF("Bind Failed\n");
        //qcom_socket_close(socketLocal);
        goto QUIT;
    }

    swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
    remoteAddr.sin_addr.s_addr = htonl(s_addr);
    remoteAddr.sin_port = htons(sin_port);
    remoteAddr.sin_family = AF_INET;
    fromSize = sizeof (struct sockaddr_in);

    qcom_adc_conversion(TRUE);
	
    while(1)
    {		
        tx_event_flags_get(&wmi_adc_event, 0x10, TX_OR_CLEAR, &flags, TX_WAIT_FOREVER);

    	if(aCtxt.convErr || *done)
    	{
            SWAT_PTF("convErr %d  %d \n",aCtxt.convErr,aCtxt.convDone);
            goto QUIT;
    	}
    	do{
            //qcom_printf("adc app read data\n");
            for(index=0;index<aCtxt.channelCnt;index++)
            {
                qcom_adc_recv_data(index,adc_buf,BUF_LEN,len,more,done);

                if(*len>0)
                {
                    //send out data via wifi
                    swat_sendto(socketLocal, (char*)adc_buf,*len,0,(struct sockaddr*)&remoteAddr, sizeof (struct sockaddr_in));
                }
            }
            if(*done)
            {
                goto QUIT;
            }
            if(*more==0)
            {
                //goto NEXT;
                break;	
            }
    	}while(*len>0);

    }

    QUIT:
    SWAT_PTF("adc udp app quit\n");
    swat_mem_free(adc_buf);
    swat_mem_free(len);
    swat_mem_free(done);
    swat_mem_free(more);
    swat_close(socketLocal);
    qcom_task_exit();
	
}

void adc_app_test()
{
    A_UINT32* len = swat_mem_malloc(sizeof(A_UINT32));
    A_UINT8* done = swat_mem_malloc(sizeof(A_UINT8));
    A_UINT8* more = swat_mem_malloc(sizeof(A_UINT8));
    A_UINT8* adc_buf = swat_mem_malloc(BUF_LEN);
    ULONG flags;
    int index=0;

    if (NULL == adc_buf) {
        SWAT_PTF("TCP RX data buffer malloc error\r\n");
        return;
    }

    qcom_adc_conversion(TRUE);

    while(1)
    {		
        tx_event_flags_get(&wmi_adc_event, 0x10, TX_OR_CLEAR, &flags, TX_WAIT_FOREVER);

        if(aCtxt.convErr || *done)
        {
            SWAT_PTF("convErr %d  %d \n",aCtxt.convErr,aCtxt.convDone);
            goto QUIT;
        }
        do{
            for(index=0;index<aCtxt.channelCnt;index++)
            {
                qcom_adc_recv_data(index,adc_buf,BUF_LEN,len,more,done);
            }		
            if(*done)
            {
                goto QUIT;
            }
            if(*more==0)
            {
                //goto NEXT;
                break;	
            }
        }while(*len>0);	
    }

    QUIT:
    SWAT_PTF("adc app quit\n");
    swat_mem_free(adc_buf);
    swat_mem_free(len);
    swat_mem_free(done);
    swat_mem_free(more);
    qcom_task_exit();
	
}



void cli_adc_tcp_app_test(void)
{
    qcom_task_start(adc_tcp_app_test, 2, 2048, 50);
}

void cli_adc_udp_app_test(void)
{
    qcom_task_start(adc_udp_app_test, 2, 2048, 50);
}

void cli_adc_app_test(void)
{
    qcom_task_start(adc_app_test, 2, 2048, 50);
}

/******************************************

    for CST test
	
*******************************************/

void adc_open_cst(int argc, char *argv[])
{
    A_UINT8 freq = 0;
    A_UINT8 accuracy = 0;
    int ret = -1;

    if (((void *) 0 == argv) || (argc < 4)) 
    {
        SWAT_PTF("Missing parameter!\r\n");
        return;
    }

    freq = swat_atoi(argv[2]);
    accuracy = swat_atoi(argv[3]);

    if(freq>7 || accuracy<6 || accuracy>16 )
    {
        SWAT_PTF("invalid paramenters!\n");
        return;
    }

    ret = qcom_adc_init((A_UINT8)GAIN_SCALE_1V8, accuracy, freq, (A_UINT8)SINGLE_ENDED, FALSE);

    if(ret<0)
    {
        SWAT_PTF("ADC init failed!\n");
    }
    else
    {
        SWAT_PTF("ADC init done!\n");
    }

}

void adc_config_cst(int argc, char *argv[])
{
    A_UINT8 sn = 0;
    A_UINT8 diff = 0;
    A_UINT32 data_cnt = 0;
    A_UINT8 channel_cnt = 0;
    A_UINT8 scan = 0;
    A_UINT8 index = 0;
    ADC_CHAN_CFG_T adc_channel[8];

    if (((void *) 0 == argv) || (argc < 5)) {
        SWAT_PTF("Missing parameter!\r\n");
        return;
    }

    sn = swat_atoi(argv[2]);
    diff = swat_atoi(argv[3]);
    data_cnt = swat_atoi(argv[4]);

    if( (sn+diff*2 > 8) || (sn+diff*2 == 0))
    {
        SWAT_PTF("invalid paramenters!\n");
        return;
    }
    channel_cnt = sn+diff;

    for(index=0; index<channel_cnt; index++)
    {
        adc_channel[index].adch = index;
        adc_channel[index].input_type = SINGLE_ENDED;
    }
    if(channel_cnt == 1)
    {
        scan=0;
    }
    else if(channel_cnt > 1)
    {
        scan=1;
    }
    qcom_adc_config(scan, 0, 1, adc_channel, channel_cnt);

    qcom_adc_dma_config(1400, data_cnt);
	
}

void adc_conv_cst(int argc, char *argv[])
{
    A_UINT8 start=0;

    if (((void *) 0 == argv) || (argc < 3)) {

        SWAT_PTF("Missing parameter!\r\n");
        return;
    }

    start=swat_atoi(argv[2]);

    if(start>2)
    {
        SWAT_PTF("invalid paramenters!\n");
        return;
    }
    if(start == 0)
    {
        aCtxt.convAbort = 1;
        //qcom_adc_conversion(start);	
        //qcom_adc_close();
    }
}


void adc_app_test_cst()
{
    A_UINT32* len = NULL;
    A_UINT8* done = NULL;
    A_UINT8* more = NULL;
    A_UINT8* adc_buf = NULL;
    ULONG flags;
    int index = 0;
    A_INT32 socketLocal;
    struct sockaddr_in remoteAddr;
    int ret;

    if(s_addr==0 || sin_port==0)
        return;

    len = swat_mem_malloc(sizeof(A_UINT32));
    done = swat_mem_malloc(sizeof(A_UINT8));
    more = swat_mem_malloc(sizeof(A_UINT8));
    adc_buf = swat_mem_malloc(BUF_LEN);

    if (NULL == adc_buf) {
        SWAT_PTF("TCP RX data buffer malloc error\r\n");
        return;
    }

    socketLocal = qcom_socket(AF_INET, SOCK_STREAM, 0);

    swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
    remoteAddr.sin_addr.s_addr = htonl(s_addr);
    remoteAddr.sin_port = htons(sin_port);
    remoteAddr.sin_family = AF_INET;
    ret = swat_connect(socketLocal, (struct sockaddr *) &remoteAddr, sizeof (struct  sockaddr_in));

    if (ret < 0) {
        /* Close Socket */
        SWAT_PTF("Connect Failed\r\n");
        //swat_close(socketLocal);
        goto QUIT;
    }

    //start conversion
    qcom_adc_conversion(TRUE);

    while(1)
    {		
        tx_event_flags_get(&wmi_adc_event, 0x10, TX_OR_CLEAR, &flags, TX_WAIT_FOREVER);

        if(aCtxt.convErr)
        {
            SWAT_PTF("convErr %d  %d \n",aCtxt.convErr,aCtxt.convDone);
            goto QUIT;
        }
        if(aCtxt.convAbort)
        {
            SWAT_PTF("conv abort\n");
            qcom_adc_close();
            goto QUIT;
        }
        do{
            for(index=0;index<aCtxt.channelCnt;index++)
            {
                qcom_adc_recv_data(index,adc_buf,BUF_LEN,len,more,done);

                if(*len>0)
                {
                    //send out data via wifi
                    swat_send(socketLocal, (char*)adc_buf, *len, 0);
                }
        	}
        	if(*done)
        	{
        	    SWAT_PTF("adc conversion done\n");
        	    goto QUIT;
        	}
        	if(*more == 0)
        	{
        	    break;
        	}
        }while(*len>0);
    }


    QUIT:
    SWAT_PTF("adc app quit\n");
    swat_mem_free(adc_buf);
    swat_mem_free(len);
    swat_mem_free(done);
    swat_mem_free(more);
    swat_close(socketLocal);
    qcom_adc_close();
    qcom_task_exit();
	
}

A_UINT8 freq = 0;
A_UINT8 accuracy = 12;
A_UINT32 dataCnt=0;

A_STATUS adc_set_parameters(int argc, char *argv[])
{
    if (((void *) 0 == argv) || (argc < 5)) {
        SWAT_PTF("Missing parameter!\r\n");
        return -1;
    }

    freq = swat_atoi(argv[2]);
    accuracy = swat_atoi(argv[3]); 
    dataCnt = swat_atoi(argv[4]);

    if(freq>7 || accuracy<6 || accuracy>12 )
    {
        SWAT_PTF("invalid paramenters!\n");
        return -1;
    }

    return A_OK;

}
void adc_motion_sensor_app()
{
    int ret = -1;
    A_UINT32 data_cnt = dataCnt;
    ADC_CHAN_CFG_T adc_channel[3];

    ret = qcom_adc_init((A_UINT8)GAIN_SCALE_1V8, accuracy, freq, (A_UINT8)SINGLE_ENDED, FALSE);

    if(ret<0)
    {
        SWAT_PTF("ADC init failed!\n");
        return;
    }
    else
    {
        SWAT_PTF("ADC init done!\n");
    }


    adc_channel[0].adch = 0;
    adc_channel[0].input_type = 0;

    adc_channel[1].adch = 1;
    adc_channel[1].input_type = 0;

    adc_channel[2].adch = 2;
    adc_channel[2].input_type = 0;

    qcom_adc_config(1, 0, 1, adc_channel, 3);

    qcom_adc_dma_config(1400, data_cnt);

    adc_app_test_cst();

}


void adc_light_sensor_app()
{

    ADC_CHAN_CFG_T adc_channel[1];
    int ret = -1;
    A_UINT32 data_cnt = dataCnt;

    ret = qcom_adc_init((A_UINT8)GAIN_SCALE_3V3, accuracy, freq, (A_UINT8)SINGLE_ENDED, FALSE);

    if(ret<0)
    {
        SWAT_PTF("ADC init failed!\n");
        return;
    }
    else
    {
        SWAT_PTF("ADC init done!\n");
    }


    adc_channel[0].adch = 0;
    adc_channel[0].input_type=0;


    qcom_adc_config(1, 0, 1, adc_channel, 1);

    qcom_adc_dma_config(1400, data_cnt);

    adc_app_test_cst();
	
}


void adc_pot_sensor_app()
{

    ADC_CHAN_CFG_T adc_channel[1];
    A_UINT32 data_cnt = dataCnt;
    int ret = -1;

    ret = qcom_adc_init((A_UINT8)GAIN_SCALE_1V8, accuracy, freq, (A_UINT8)SINGLE_ENDED, FALSE);

    if(ret<0)
    {
        SWAT_PTF("ADC init failed!\n");
        return;
    }
    else
    {
        SWAT_PTF("ADC init done!\n");
    }

    adc_channel[0].adch = 0;
    adc_channel[0].input_type = 0;

    qcom_adc_config(1, 0, 1, adc_channel, 1);

    qcom_adc_dma_config(1400, data_cnt);

    adc_app_test_cst();
	
}


void cli_adc_app_test_cst()
{
    qcom_task_start(adc_app_test_cst, 2, 2048, 50);
}

void cli_adc_motion_sensor_app(void)
{
    qcom_task_start(adc_motion_sensor_app, 2, 2048, 50);
}


void cli_adc_light_sensor_app(void)
{
    qcom_task_start(adc_light_sensor_app, 2, 2048, 50);
}

void cli_adc_pot_sensor_app(void)
{
    qcom_task_start(adc_pot_sensor_app, 2, 2048, 50);
}



/****************

light     ADC0      GPIO16

motion
X       ADC1        GPIO17
Y       ADC2        GPIO18
Z       ADC3        GPIO19

POT     ADC4       GPIO14


******************/

static int light_inited = 0;
void light_init()
{
    int freq = 0;	/* sample rate: 32.75KHz */
    int accuracy = 12;	/* 12 bit */

    if(light_inited)
        return;
    light_inited = 1;
    	
    ADC_CHAN_CFG_T adc_channel[1];
    adc_channel[0].adch = 0;
    adc_channel[0].input_type=0;

    qcom_adc_init(GAIN_SCALE_3V3, accuracy, freq, SINGLE_ENDED, FALSE);	

    qcom_adc_config(1, 0, 1, adc_channel, 1);
    qcom_adc_dma_config(1400, 4);	/* get 4 samples */

}

void light_read()
{
    unsigned int len=0;
    unsigned char done=0, more=0, adc_buf[8];
    int i, val=0;
    int sum;

    qcom_adc_conversion(TRUE);

    qcom_thread_msleep(2);
    qcom_adc_recv_data(0, adc_buf, 8, &len, &more, &done);
    //qcom_adc_conversion(FALSE);

    val = 0;
    SWAT_PTF("adc0 data: ");
    for(i=0, sum=0;i<4;i++)
    {
        SWAT_PTF("%02x%02x ", adc_buf[2*i+1], adc_buf[2*i]);
        val = adc_buf[i * 2 + 1];
        val <<= 8;
        val |= adc_buf[i * 2];
        //val >>= 4;
        sum += val;
    }
    SWAT_PTF("result = 0x%x (%u)\n",sum>>2 , sum>>2);	

}

static int sensors_inited = 0;
void app_3sensors_init()
{
    int freq = 0;	/* sample rate: 32.75KHz */
    int accuracy = 12;	/* 10 bit */

    if(sensors_inited)
        return;
    sensors_inited = 1;
    	
    ADC_CHAN_CFG_T adc_channel[5];
	
    qcom_adc_init(GAIN_SCALE_3V3, accuracy, freq, SINGLE_ENDED, FALSE);	

    adc_channel[0].adch = 0;
    adc_channel[0].input_type=0;

    adc_channel[1].adch = 1;
    adc_channel[1].input_type=0;

    adc_channel[2].adch = 2;
    adc_channel[2].input_type=0;

    adc_channel[3].adch = 3;
    adc_channel[3].input_type=0;

    adc_channel[4].adch = 4;
    adc_channel[4].input_type=0;

    qcom_adc_config(1, 0, 1, adc_channel, 5);
    qcom_adc_dma_config(1400, 4*5);	/* get 4 samples per channel */

}

void app_3sensors_read()
{

    unsigned int len=0;
    unsigned char done=0, more=0, adc_buf[5][8]={{0},{0},{0},{0},{0}};
    int i,j, val=0;
    int sum;

    //qcom_adc_dma_config(1400, 4);	/* get 4 samples */
    qcom_adc_conversion(TRUE);

    qcom_thread_msleep(20);
    qcom_adc_recv_data(0, adc_buf[0], 8, &len, &more, &done);

    qcom_adc_recv_data(1, adc_buf[1], 8, &len, &more, &done); 

    qcom_adc_recv_data(2, adc_buf[2], 8, &len, &more, &done); 

    qcom_adc_recv_data(3, adc_buf[3], 8, &len, &more, &done); 

    qcom_adc_recv_data(4, adc_buf[4], 8, &len, &more, &done); 
    //qcom_adc_conversion(FALSE);


    for(j=0; j<5; j++)
    {
        SWAT_PTF("adc%d data: ", j);
        
        for(i=0, sum=0; i<4; i++)
        {
            SWAT_PTF("%02x%02x ", adc_buf[j][2*i+1], adc_buf[j][2*i]);
            val = adc_buf[j][i * 2 + 1];
            val <<= 8;
            val |= adc_buf[j][i * 2];
            //val >>= 4;
            sum += val;	
        }
        SWAT_PTF("result = 0x%x (%u)\n",sum>>2 , sum>>2);	
    }

}


int sensor_demo_app2(int type)
{
    switch(type){
        case 1:
        {
            light_init();
            light_read();
            break;
        }
        case 2:
        {
            app_3sensors_init();
            app_3sensors_read();
            break;
        }	 
       case 3:
        {
            break;
        }	    
        case 4:
        {
            break;
        }
        default:
            break;		
    }
    return 0;
}

void adc_help()
{

#if ADC_DEBUG  
    SWAT_PTF("adc {open | config | conv | app | close } \r\n"); 
#endif

    SWAT_PTF("\nUsage:\r\n");

    SWAT_PTF("adc conv <start> \r\n");
    SWAT_PTF("adc server <server IP> <port> \r\n");

#if ADC_DEBUG  
    SWAT_PTF("adc open <freq> <accuracy> \r\n");
    SWAT_PTF("adc config <sn> <diff> <data count> \r\n");    
    SWAT_PTF("adc app \r\n");
    SWAT_PTF("adc close \r\n");
#endif    
    SWAT_PTF("adc light <freq> <accuracy> <data count> \r\n");
    SWAT_PTF("adc pot <freq> <accuracy> <data count> \r\n");
    SWAT_PTF("adc motion <freq> <accuracy> <data count> \r\n");

    SWAT_PTF("\nParameters:\r\n");
    SWAT_PTF("1 <freq>        : 0~7\r\n");
    SWAT_PTF("2 <accuracy>    : 6~12\r\n");    
    SWAT_PTF("3 <data count>  : >0 \r\n");
    SWAT_PTF("4 <start>       : 0:stop  1: start\r\n");
#if ADC_DEBUG      
    SWAT_PTF("5 <sn>          : 1~8\r\n");
    SWAT_PTF("6 <diff>        : 1~4\r\n");
#endif

    SWAT_PTF("demo app2 usage: \r\n");
    SWAT_PTF("adc light_demo2 \r\n");
    SWAT_PTF("adc sensors_demo \r\n");
    SWAT_PTF("      --Light sensor(ADC Pin0), motion sensor(ADC Pin1, Pin2&Pin3) and POT(ADC Pin4) sensor work at the same time.\r\n");
     
}

int cmd_adc_test(int argc, char *argv[])
 {
    int ret = -1;

    if(argc == 1)
    {
        adc_help(); 
       	return -1;
    }

    if(!A_STRCMP(argv[1], "help"))
    {
       	adc_help();
    }
    else if(!A_STRCMP(argv[1], "light_demo2"))
    {
        sensor_demo_app2(1);

        qcom_thread_msleep(50);
    }
    else if(!A_STRCMP(argv[1], "sensors_demo"))
    {
        sensor_demo_app2(2);

        qcom_thread_msleep(50);
    }
#if ADC_DEBUG    
    else if(!A_STRCMP(argv[1], "openz"))
    {
       	adc_open();
    }
    else if(!A_STRCMP(argv[1], "init"))
    {
        qcom_adc_init(GAIN_SCALE_1V8, 12, ADC_FREQ_25_KHZ, 1, 0);
    }
    else if(!A_STRCMP(argv[1], "apptcp"))
    {
        cli_adc_tcp_app_test();
    }
    else if(!A_STRCMP(argv[1], "appudp"))
    {
        cli_adc_udp_app_test();
    }
    else if(!A_STRCMP(argv[1], "apptest"))
    {
        cli_adc_app_test();
    }

    else if(!A_STRCMP(argv[1], "open"))
    {
    	adc_open_cst(argc, argv);
    }
    else if(!A_STRCMP(argv[1], "config"))
    {
    	adc_config_cst(argc, argv);
    }
#endif     
    else if(!A_STRCMP(argv[1], "conv"))
    {
    	adc_conv_cst(argc, argv);
    }
    else if(!A_STRCMP(argv[1], "server"))
    {			
    	adc_server_init(argc, argv);
    }
#if ADC_DEBUG      
    else if(!A_STRCMP(argv[1], "app"))
    {
    	cli_adc_app_test_cst();
    }
#endif    
    else if(!A_STRCMP(argv[1], "light"))
    {
        ret = adc_set_parameters(argc, argv);
        if(ret < 0)
        {
            return ret;
        }
    	cli_adc_light_sensor_app();
    }
    else if(!A_STRCMP(argv[1], "pot"))
    {
        ret = adc_set_parameters(argc, argv);
        if(ret < 0)
        {
            return ret;
        }
    	cli_adc_pot_sensor_app();
    }
    else if(!A_STRCMP(argv[1], "motion"))
    {
        ret = adc_set_parameters(argc, argv);
        if(ret < 0)
        {
            return ret;
        }
    	cli_adc_motion_sensor_app();
    }
#if ADC_DEBUG    
    else if(!A_STRCMP(argv[1], "close"))
    {	
    	qcom_adc_close();
    }
#endif    
    else 
    {
       	adc_help();			
       	return -1;
    } 

    return 0;
 }

