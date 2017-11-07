/*
 * Copyright (c) 2011 Qualcomm Atheros, Inc.
 * All Rights Reserved.
 * Qualcomm Atheros Confidential and Proprietary.
 */


#include <fwconfig_AR6006.h>
#include <qcom/qcom_common.h>
#include <qcom/socket_api.h>
#include <qcom/select_api.h>
#include <qcom/qcom_i2s.h>
#include <qcom/qcom_i2c_master.h>
#include <qcom/qcom_gpio.h>

#include "threadx/tx_api.h"
#include "threadx/tx_thread.h"

#include "swat_parse.h"
#include "misc_cdr.h"

#if 1 //def SWAT_I2S_TEST
#define CONFIG_I2S_CDR 1

#define CS4270	1

//extern void qcom_thread_msleep(unsigned long ms);
extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

#define I2S_STEREO	0
#define I2S_MONO	1
#define I2S_BUF_SIZE 1024
#define I2S_BUFFER_CNT	32
#define I2S_ERR 0
#define I2S_OK 1

#define UDA1380_ADDR (0x18)
#define UDA1380_CNT (9)
#define TCP_RX_BUF_LEN  I2S_BUF_SIZE
#define TCP_TX_BUF_LEN  (I2S_BUF_SIZE + 16)

#define IPv4_STR_PRT(addr) qcom_printf("%d.%d.%d.%d", (addr)>>24&0xff, (addr)>>16&0xff, (addr)>>8&0xff, (addr)&0xff)

#define I2S_TCP	1
//#define I2S_UDP	1

static int gl4_port_for_i2s;
static unsigned int gip_for_i2s;
static int socket_conn = -1;
#ifdef I2S_UDP
struct sockaddr_in udp_clnt_addr;
#endif
int i2s_ctl_flag;
#define I2S_INIT 			0x8
#define I2S_SEND_FINI 	0x4
#define I2S_SEND_STOP 	0x2
#define I2S_REC_STOP 	0x1
#define I2S_REC_START 	0x0
#define I2S_PLAY_START 	0x10
#define I2S_PLAY_STOP 	0x20

#define I2S0_SCK	16
#define I2S0_SDI	17
#define I2S0_SDO	18
#define I2S0_WS		19
#define I2S0_MCLK	20
#define I2S1_SCK	27
#define I2S1_SDI	30
#define I2S1_SDO	31
#define I2S1_WS		32
#define I2S1_MCLK	33

static int cnt_i2s_blk;
typedef enum {
    I2S_SPEAKER = 1,
    I2S_LINEIN	 =2
} CODEC_MODE;

volatile int i2s_tx_completed[2]={0};//1;
static int i2s_socket_recv_cnt[2]={0};
static int i2s_socket_send_cnt[2]={0};
volatile int i2s_data_arrived[2]={0};
static i2s_api_config_s app_i2s_config_recorder={0, I2S_FREQ_48K, 16/*dsize*/, I2S_BUF_SIZE/*i2s_buf_size*/, I2S_BUFFER_CNT, 0, 1/*master mode*/};
static i2s_api_config_s app_i2s_config_play={0, I2S_FREQ_48K, 16/*dsize*/, I2S_BUF_SIZE/*i2s_buf_size*/, 0, I2S_BUFFER_CNT, 1/*master mode*/};

static void i2s_rx_intr_cb(void *ctx ,A_UINT8 * bufp, A_UINT32 size);
static void i2s_txcomp_intr_cb(void *ctx);

void i2s_gpio_preinit_set(int port)
{
	/*This function should be executed in case user sets the I2S pins to wrong state. eg, tunable setting. 
	** It should be initialized correcty before i2s init fucntion. If user set I2S0_SDI/I2S1_SDI to output mode,
	** I2S could not work properly unless setting it to input.
	*/
	if(port == 0)
	{
		qcom_gpio_pin_dir(I2S0_SDI,1);//input
		qcom_gpio_pin_dir(I2S0_SCK,0);//output
		qcom_gpio_pin_dir(I2S0_SDO,0);
		qcom_gpio_pin_dir(I2S0_WS,0);
		qcom_gpio_pin_dir(I2S0_MCLK,0);
	}
	else if(port == 1)
	{
		qcom_gpio_pin_dir(I2S1_SDI,1);//input
		qcom_gpio_pin_dir(I2S1_SCK,0);
		qcom_gpio_pin_dir(I2S1_SDO,0);
		qcom_gpio_pin_dir(I2S1_WS,0);
		qcom_gpio_pin_dir(I2S1_MCLK,0);
	}
}
int i2s_codec_config(int port, CODEC_MODE mode)
{
#ifndef CS4270	
//these registers has 2 bytes length
	#define UDA_REG_0		0x4
	#define UDA_REG_2		0x5
	#define UDA_REG_22	0x6
	#define UDA_REG_21	0x8
	static unsigned char uda1380_reg[UDA1380_CNT] =
	{0x7F,0x01,0x13,0x14,0x00,0x02,0x22,0x23,0x21};

	static unsigned short int uda1380_cfg1[UDA1380_CNT] =
	{0,0,0,0,0x0f02,0xa5df,0x040e,0x1,0x0303 };

	static unsigned short int uda1380_cfg2[UDA1380_CNT] =
	{0,0,0,0,0x0f02,0xa5df,0x0401,0x1,0x0303};
#endif
	i2s_api_config_s *app_i2s_config_demo;
	/*if(!(i2s_ctl_flag&I2S_INIT))*/{

	  	if(mode == I2S_SPEAKER)
		{
			app_i2s_config_play.port = port;
			app_i2s_config_demo = &app_i2s_config_play;
		#ifdef CONFIG_I2S_CDR
			if(I2S_ERR==(cdr_i2s_init(app_i2s_config_demo,i2s_txcomp_intr_cb, NULL)))
		#else
			if(I2S_ERR==(qcom_i2s_init(app_i2s_config_demo,i2s_txcomp_intr_cb, NULL)))
		#endif
			{
				//qcom_printf("%s: qcom_i2s_init error!\n", __func__);
				return I2S_ERR;
			}
		}
		else
		{
			app_i2s_config_recorder.port = port;
			app_i2s_config_demo = &app_i2s_config_recorder;
		#ifdef CONFIG_I2S_CDR
			if(I2S_ERR==(cdr_i2s_init(app_i2s_config_demo,NULL, i2s_rx_intr_cb)))
		#else
			if(I2S_ERR==(qcom_i2s_init(app_i2s_config_demo,NULL,i2s_rx_intr_cb)))
		#endif
			{
				QCOM_DEBUG_PRINTF("%s: qcom_i2s_init error!\n", __func__);
				return I2S_ERR;
			}
		}
#ifndef CS4270		
		qcom_i2cm_init(I2CM_PIN_PAIR_2,I2C_FREQ_200K,0);
#endif
		i2s_ctl_flag|=I2S_INIT;
	}
#ifndef CS4270		
	int i;
	if(I2S_SPEAKER == mode){
		for(i=0;i<(UDA1380_CNT);i++){
			qcom_thread_msleep(10);
			//qcom_printf("kkkwrite %x with data %x\n ",uda1380_reg[i],(uda1380_cfg1[i]));
			if(i==UDA_REG_0||i==UDA_REG_2||i==UDA_REG_22||i==UDA_REG_21)
				{
					if(qcom_i2cm_write(I2CM_PIN_PAIR_2,UDA1380_ADDR,uda1380_reg[i],1, (A_UINT8 *)&(uda1380_cfg1[i]),2)<0)
					{
					//qcom_printf("i2c write 2 bytes error i=%d\n",i);
					//return I2S_ERR;
					}
				}
			else{
				if(qcom_i2cm_write(I2CM_PIN_PAIR_2,UDA1380_ADDR,uda1380_reg[i],1, (A_UINT8 *)&(uda1380_cfg1[i]),1)<0)
					{
					//qcom_printf("i2c write error i=%d\n",i);
					//return I2S_ERR;
					}
				}
		}
	}
	else if(I2S_LINEIN == mode){
				
		for(i=0;i<(UDA1380_CNT);i++){
			//if(I2S_ERR == uda1380_write(uda1380_reg[i], uda1380_cfg2[i]))
			qcom_thread_msleep(10);
			if(i==UDA_REG_0||i==UDA_REG_2||i==UDA_REG_22||i==UDA_REG_21)
				{
					if(qcom_i2cm_write(I2CM_PIN_PAIR_2,UDA1380_ADDR,uda1380_reg[i],1, (A_UINT8 *)&(uda1380_cfg2[i]),2)<0)
					{
					qcom_printf("i2c write 2 bytes error i=%d\n",i);
					return I2S_ERR;
					}
				}
			else{
				if(qcom_i2cm_write(I2CM_PIN_PAIR_2,UDA1380_ADDR,uda1380_reg[i],1, (A_UINT8 *)&(uda1380_cfg2[i]),1)<0)
					{
					qcom_printf("i2c write error i=%d\n",i);
					return I2S_ERR;
					}
				}
		}
	}
#endif
	qcom_thread_msleep(100);
	return I2S_OK;
}

void audio_socket_addr_init(struct sockaddr_in *psrv_add)
{
	memset(psrv_add, 0, sizeof(struct sockaddr_in));
	psrv_add->sin_addr.s_addr = htonl(gip_for_i2s);
	psrv_add->sin_port = htons(gl4_port_for_i2s);
	psrv_add->sin_family = AF_INET;
	return;
}
TX_QUEUE  i2s_msg_queue;
TX_QUEUE  i2s1_msg_queue;

void audio_play(unsigned int port)
{
	A_UINT32 queue_status;
	TX_QUEUE *p_msg_queue;
	
	i2s_gpio_preinit_set(port);
	if(I2S_ERR == i2s_codec_config(port, I2S_SPEAKER)){
		//printf("%s: config error!\n", __func__);
		 qcom_task_exit();
		return ;
	}

	int ret = -1, nRecv = 0, queue_create_status = -1, len, nTime=0;
   	//char recvBuf[TCP_RX_BUF_LEN] = {0};
   	char *recvBuf;
	char	*sendBuf;
  	q_fd_set sockSet, masterSet;
  	int fd_act=0, err_times=0, nBytes=0, nCount;
  	struct timeval tmo;
  	int socket_serv = -1;
  	int socket_clnt = -1;
  	struct sockaddr_in sock_add;
  	struct sockaddr_in clnt_addr;
  	char ack[3]= "OK";
	int nSend;
	char file_name[32];
	int file_size;

    if(port == 0)
        p_msg_queue=&i2s_msg_queue;
    else
        p_msg_queue=&i2s1_msg_queue;
    
    A_MEMZERO(p_msg_queue, sizeof(i2s_msg_queue));
    queue_create_status = tx_queue_create(p_msg_queue, "i2s msg queue", sizeof(queue_status)/4, &queue_status, sizeof(queue_status));
    if ( queue_create_status != TX_SUCCESS) 
    {
        qcom_printf("tx queure create error\n");
        return ;
    }

	recvBuf = qcom_mem_alloc(TCP_RX_BUF_LEN);
	if(NULL==recvBuf)
		qcom_printf("null recvBuf\n");
   memset(recvBuf, 0, sizeof(TCP_RX_BUF_LEN));
 	 /* create TCP socket */
  	socket_serv = qcom_socket(AF_INET, SOCK_STREAM, 0);
  	if(socket_serv <=0)
  	{
  		//printf("%s: socket error!\n", __func__);
     	 goto done;
  	}
	
 	 /* bind to a port */
  	audio_socket_addr_init(&sock_add);
  	ret = qcom_bind(socket_serv, (struct sockaddr *)&sock_add, sizeof(struct sockaddr_in));
	if(ret<0)
		qcom_printf("%s: bind error!\n", __func__);

#if 1
    	/* wait for connection */
  	FD_ZERO(&masterSet);
  	FD_SET(socket_serv, &masterSet);
  	tmo.tv_sec = 120;
   	tmo.tv_usec = 0;
#endif

  	/* listen on the port */
  	ret = qcom_listen(socket_serv, 10);
  	qcom_printf("Tcp server is listenning on port %d...\n", gl4_port_for_i2s);

  	sockSet = masterSet;
  	fd_act = qcom_select(2, &sockSet, NULL, NULL, &tmo);
  	if(fd_act <= 0)
  	{
      	QCOM_DEBUG_PRINTF("No connection in %ld seconds\n", tmo.tv_sec);
      	goto done;
  	}

    	/* accept connection from client */
  	socket_clnt = qcom_accept(socket_serv, (struct sockaddr *)&clnt_addr, &len);
  	if(socket_clnt<=0)
      	goto done;

  	qcom_printf("Accept connection from ");
  	IPv4_STR_PRT((int)(ntohl(clnt_addr.sin_addr.s_addr)));
  	qcom_printf(": %d\n", ntohs(clnt_addr.sin_port));

  	/* recv first packet */
  	qcom_thread_msleep(500);
  	nRecv = qcom_recv(socket_clnt, recvBuf, TCP_RX_BUF_LEN, 0);
  	
	if(nRecv<4){
		qcom_printf("bad file:nRecv = %d\n", nRecv);
		goto done;
	}
  	file_size = ntohl(*(int*)recvBuf);
  	memcpy(file_name, recvBuf+4, nRecv-4);
  	file_name[nRecv-4] = 0;
  	qcom_printf("Sending file %s to i2s, size = %d\n", file_name, file_size);
  	qcom_thread_msleep(500);
  	/* send ACK */
  	nSend = qcom_send(socket_clnt, ack, 3, 0);
  	qcom_thread_msleep(800);
  	/* start the loop */
  	nTime = time_ms();
	
	i2s_ctl_flag|=I2S_PLAY_START;

  	while(!(i2s_ctl_flag&I2S_PLAY_STOP))
  	{
#ifdef INT_DEBUG  	
  		if(i2s_tx_completed[port]>I2S_BUFFER_CNT)
  			{
  				qcom_printf("%d %d", i2s_tx_completed[port],i2s_socket_recv_cnt[port]);
  			}
#endif			
  		if((i2s_tx_completed[port]<I2S_BUFFER_CNT) &&(i2s_socket_recv_cnt[port]==I2S_BUFFER_CNT)){ //we assue socket is faster than I2S DMA
			//qcom_thread_msleep(1);
			if (tx_queue_receive(p_msg_queue, &queue_status, TX_WAIT_FOREVER) == TX_SUCCESS)
				{
					continue;
				}
			continue; //i2s write not completed, wait to receive.
		}

		nRecv = qcom_recv(socket_clnt, recvBuf, I2S_BUF_SIZE, 0);
      	if(nRecv > 0)
					;
          	//nSend = qcom_send(socket_clnt, ack, 3, 0);
      	else if(nRecv == 0)
      	{
          qcom_printf("\nRecv %s done %x\n", file_name,nRecv);
          	break;
      	}  
      	/* write data to i2s */
      	sendBuf = recvBuf;
		if(nRecv > 0){
#ifdef CONFIG_I2S_CDR			
			ret = cdr_i2s_xmt_data(port, (A_UINT8 *)sendBuf,nRecv);
#else
			ret = qcom_i2s_xmt_data(port, (A_UINT8 *)sendBuf,nRecv);
#endif
			if(ret!=0) //ret=0 means i2s write success
			{
				//qcom_printf("(%d %d)",i2s_tx_completed,i2s_socket_recv_cnt);
				/* to make sure all data is written into I2S */
				while(ret){
				#ifdef CONFIG_I2S_CDR			
					ret = cdr_i2s_xmt_data(port, (A_UINT8 *)sendBuf,nRecv);
				#else
					ret = qcom_i2s_xmt_data(port, (A_UINT8 *)sendBuf,nRecv);
				#endif
					}
			}
			i2s_socket_recv_cnt[port]++;
			if((i2s_tx_completed[port]>=I2S_BUFFER_CNT)&&(i2s_socket_recv_cnt[port]>=I2S_BUFFER_CNT)){
				i2s_tx_completed[port]=0;
				i2s_socket_recv_cnt[port]=0;
			}

			nBytes += nRecv;
			//ack peer, let it send data.
			nSend = qcom_send(socket_clnt, ack, 3, 0);
			nRecv = 0;
		}
		if(nRecv > 0)
		{
			printf("%s: too full?\n", __func__);
			err_times++;
		}
		else
		{
			nCount++;
			if((nCount%500) == 0)
				qcom_printf("* ");
			if((nCount%20000) == 0)
              qcom_printf("\n");
		}
     if(err_times > 20)
     	{
     	qcom_printf("%s: too many error, break.\n", __func__);
          break;
     	}
    }    

   	nTime = time_ms()-nTime;
  	printf("\ntcp_2_i2s test done. Write %d bytes to i2s in %d seconds\n", nBytes, nTime/1000);
done:
  /* close TCP sockets */
    if (socket_clnt > 0)
        qcom_socket_close(socket_clnt);

    if (socket_serv > 0)
        qcom_socket_close(socket_serv);

    if(queue_create_status == TX_SUCCESS)
    {
        if (tx_queue_delete(p_msg_queue) != TX_SUCCESS)
            qcom_printf("delete queue error\n");
    }
    qcom_mem_free(recvBuf);
    qcom_thread_msleep(100);
    qcom_printf("task exit\n");
    qcom_task_exit();
    return;
}

void cli_audio_play(unsigned int port)
{

	gip_for_i2s = INADDR_ANY;
	if(gl4_port_for_i2s<100 ||gl4_port_for_i2s>30000)
  		gl4_port_for_i2s = 6000;
  	qcom_task_start(audio_play, (unsigned int)port, 2048, 10);

  	return;
}



/**********************************************************
 **********************************************************
 ********************   recorder  process   ********************
 **********************************************************
 **********************************************************/


void i2s_rx_intr_cb(void *ctx ,A_UINT8 * bufp, A_UINT32 size)
{
	int status=1;
	int which_port = *(int *)ctx;
	
	i2s_data_arrived[which_port]++;
	if(which_port==0)
		tx_queue_send(&i2s_msg_queue, &status, TX_NO_WAIT);
	else
		tx_queue_send(&i2s1_msg_queue, &status, TX_NO_WAIT);
}
static void i2s_txcomp_intr_cb(void *ctx)
{
	int status=1;
	int which_port = *(int *)ctx;
	
	i2s_tx_completed[which_port]++;
	if(which_port==0)
		tx_queue_send(&i2s_msg_queue, &status, TX_NO_WAIT);
	else
		tx_queue_send(&i2s1_msg_queue, &status, TX_NO_WAIT);
}

void audio_data_send(A_UINT32 port, unsigned char * bufp, unsigned int size)
{
	char szbuf[16];
	int ret =-1;
	int i2s2tcp_buf_len;

	memset(szbuf, 0, sizeof(szbuf));

	if((i2s_ctl_flag&I2S_SEND_FINI)&&(i2s_ctl_flag&I2S_SEND_STOP)){
#ifdef CONFIG_I2S_CDR		
		cdr_i2s_rcv_control(port, I2S_REC_STOP);
#else
		qcom_i2s_rcv_control(port, I2S_REC_STOP);
#endif
		i2s_ctl_flag |= I2S_REC_STOP;
	}
	else{
		i2s2tcp_buf_len = size;

		if (!(i2s_ctl_flag&I2S_SEND_STOP))
		{
#ifdef I2S_TCP		
			ret = qcom_send(socket_conn, (char *)bufp, size, 0);
#else
			ret = qcom_sendto(socket_conn, (char *)bufp, size, 0, (struct sockaddr*)&udp_clnt_addr, sizeof (struct sockaddr_in));
#endif
			if(ret == TX_BUFF_FAIL)
			{
				qcom_printf("buf full? size=%d\n", size);

#ifdef I2S_TCP					
				qcom_send(socket_conn, (char *)bufp, size, 0);
#else
				qcom_sendto(socket_conn, (char *)bufp, size, 0, (struct sockaddr*)&udp_clnt_addr, sizeof (struct sockaddr_in));
#endif
			}
			else if(ret<size)
				qcom_printf("{%d}",ret);//qcom_thread_msleep(2);

			cnt_i2s_blk++;
			if((cnt_i2s_blk%500) == 0)
				qcom_printf("# ");
			if((cnt_i2s_blk%5000) == 0)
            	qcom_printf("\n");
		}
			  /* no more data */
		else
		{
			memset(szbuf, 0, sizeof(szbuf));
			sprintf(szbuf, "end of file");
			i2s2tcp_buf_len = strlen(szbuf);
#ifdef I2S_TCP				
			qcom_send(socket_conn, szbuf, i2s2tcp_buf_len, 0);
#else
			qcom_sendto(socket_conn, szbuf, i2s2tcp_buf_len, 0, (struct sockaddr*)&udp_clnt_addr, sizeof (struct sockaddr_in));
#endif
			i2s_ctl_flag |= I2S_SEND_FINI;
		}
  	}

}

A_UINT32 queue_status=0;

void audio_data_process(A_UINT32 port, A_UINT8 *data_rcv)
{
	unsigned int size=0;
	int ret=-1;
	TX_QUEUE *p_msg_queue;

	if(port == 0)
		p_msg_queue=&i2s_msg_queue;
	else
		p_msg_queue=&i2s1_msg_queue;
	
	if((i2s_data_arrived[port]<I2S_BUFFER_CNT)&&(i2s_socket_send_cnt[port]==I2S_BUFFER_CNT))
	{
		if (tx_queue_receive(p_msg_queue, &queue_status, TX_WAIT_FOREVER) == TX_SUCCESS)
		{
			//qcom_printf("@{%d %d}",i2s_data_arrived,i2s_socket_send_cnt);
			return;
		}
		//qcom_printf("?");
		return;
	}

	do{
#ifdef CONFIG_I2S_CDR		
		ret = cdr_i2s_rcv_data(port, data_rcv, I2S_BUF_SIZE, &size);
#else
		ret = qcom_i2s_rcv_data(port, data_rcv, I2S_BUF_SIZE, &size);
#endif
		if(size<I2S_BUF_SIZE && size!=0)
			qcom_printf("[%d]",size);
		//if(ret==0)
			//qcom_printf("(%d)",size);
	}while(ret==0); //ret=0 means I2S DMA own normally, check here...

	if(size) //i2s read buf, not zero
	{
		audio_data_send( port, data_rcv, I2S_BUF_SIZE);
		i2s_socket_send_cnt[port]++;

		if(size != I2S_BUF_SIZE)
		{
			printf("%s: size %d not match, why ???\n", __func__, size);
		}
		if((i2s_data_arrived[port]>=I2S_BUFFER_CNT)&&(i2s_socket_send_cnt[port]>=I2S_BUFFER_CNT))
		{
			i2s_data_arrived[port]=0;
			i2s_socket_send_cnt[port]=0;
		}
	}
}

void audio_recorder_main(unsigned int port)
{
	int ret = -1;
	int socket_serv = -1;
#ifdef I2S_TCP
	int i2s2tcp_buf_len = 0;
	struct sockaddr_in clnt_addr;
	q_fd_set sockSet, masterSet;
  	int fd_act=0;
  	struct timeval tmo;
#endif	
	struct sockaddr_in sock_add;
	A_UINT32 queue_status;
	A_UINT8 *data_rcv;

	TX_QUEUE *p_msg_queue;

	if(port == 0)
		p_msg_queue=&i2s_msg_queue;
	else
		p_msg_queue=&i2s1_msg_queue;
	
	/*Clear cnt*/
	i2s_data_arrived[port]=0;
	i2s_socket_send_cnt[port]=0;
	
	i2s_ctl_flag &= I2S_INIT;
	cnt_i2s_blk = 0;
	socket_conn = -1;
#ifdef I2S_TCP	
	socket_serv = qcom_socket(AF_INET, SOCK_STREAM, 0);
#else
	socket_serv = qcom_socket(AF_INET, SOCK_DGRAM, 0);
#endif
	if (socket_serv < 0)
	{
		goto done;
	}
#ifdef I2S_TCP
	//printf("#########create socket %d.#########\n",socket_serv);
	audio_socket_addr_init(&sock_add);
	ret = qcom_bind(socket_serv, (struct sockaddr *)&sock_add, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		QCOM_DEBUG_PRINTF("Failed to bind socket %d.\n", socket_serv);
		goto done;
	}
#else
	//audio_socket_addr_init(&sock_add);
	memset(&sock_add, 0, sizeof(struct sockaddr_in));
	A_UINT32 ipAddress;
    A_UINT32 submask;
    A_UINT32 gateway;
		
	qcom_ipconfig(currentDeviceId, IP_CONFIG_QUERY, &ipAddress, &submask, &gateway);
  	sock_add.sin_addr.s_addr = htonl(ipAddress);
  	sock_add.sin_port = htons(gl4_port_for_i2s);
  	sock_add.sin_family = AF_INET;
		
	audio_socket_addr_init(&udp_clnt_addr);
qcom_printf("UDP remote addr %lud\n", udp_clnt_addr.sin_addr.s_addr);
//UDP should bind local address
	ret = qcom_bind(socket_serv, (struct sockaddr *)&sock_add, sizeof(struct sockaddr_in));
	if (ret < 0)
	{
		QCOM_DEBUG_PRINTF("Failed to bind UDP remote socket %d.\n", socket_serv);
		goto done;
	}

	socket_conn = socket_serv; //UDP should use local socket
#endif

#ifdef I2S_TCP
	ret = qcom_listen(socket_serv, 10);
	if (ret < 0)
	{
		;
	}
	qcom_printf("Tcp server is listenning on port %d...\n", gl4_port_for_i2s);

  	/* wait for connection */
  	FD_ZERO(&masterSet);
  	FD_SET(socket_serv, &masterSet);
  	tmo.tv_sec = 120;
   	tmo.tv_usec = 0;

	sockSet = masterSet;
  	fd_act = qcom_select(2, &sockSet, NULL, NULL, &tmo);
  	if(fd_act <= 0)
  	{
      	QCOM_DEBUG_PRINTF("No connection in %ld seconds\n", tmo.tv_sec);
      	goto done;
  	}

	socket_conn = qcom_accept(socket_serv, (struct sockaddr *)&clnt_addr, &i2s2tcp_buf_len);
	if (socket_conn < 0)
	{
		qcom_printf("%s: accept error!\n", __func__);
		goto done;
	}

	qcom_printf("Accept connection from ");
	IPv4_STR_PRT((int)(ntohl(clnt_addr.sin_addr.s_addr)));
	printf(": %d\n", ntohs(clnt_addr.sin_port));
#endif
	A_MEMZERO(p_msg_queue, sizeof(i2s_msg_queue));
	if (tx_queue_create(p_msg_queue, "i2s msg queue", sizeof(queue_status)/4, &queue_status, sizeof(queue_status)) != TX_SUCCESS) {
        //qcom_mem_free(i2s_msg_queue);
		printf("tx queure create error\n");
        return ;
    }			
	printf("please start speaking....\n");
	qcom_thread_msleep(1000);
	int nTime = 0;
	nTime = time_ms();
	
#ifdef CONFIG_I2S_CDR	
	cdr_i2s_rcv_control(port, I2S_REC_START);
#else
	qcom_i2s_rcv_control(port,I2S_REC_START/*0*/);
#endif

	data_rcv = qcom_mem_alloc(I2S_BUF_SIZE);
	if(NULL==data_rcv)
		qcom_printf("null data_rcv\n");
	memset(data_rcv, 0, sizeof(I2S_BUF_SIZE));
	while(!(i2s_ctl_flag&I2S_REC_STOP))
	{
		audio_data_process(port, data_rcv);
	}
	qcom_printf("recorder test end....\n");
	nTime = time_ms()-nTime;
	qcom_printf("\n recv %d bytes in %d ms\n",cnt_i2s_blk*I2S_BUF_SIZE,nTime);
	qcom_thread_msleep(100);
	qcom_mem_free(data_rcv);

	if (tx_queue_delete(p_msg_queue) != TX_SUCCESS)
		qcom_printf("delete queue error\n");
	qcom_thread_msleep(100);
	qcom_printf("task exit\n");
	
done:
	if (socket_conn > 0)
	{
		qcom_socket_close(socket_conn);
	}
	if (socket_serv > 0)
	{
		qcom_socket_close(socket_serv);
	}
	qcom_task_exit();
	return;
}

void cli_audio_recorder(unsigned int port)
{
	i2s_gpio_preinit_set(port);
	i2s_codec_config(port, I2S_LINEIN);
	//qcom_i2s_mode(app_i2s_config_recorder.port, I2S_STEREO); //mono :1, stereo :0;
	qcom_thread_msleep(500);
#ifdef I2S_TCP
	gip_for_i2s = INADDR_ANY;
#endif
	if(gl4_port_for_i2s<100 ||gl4_port_for_i2s>30000)
		gl4_port_for_i2s = 7000;
	qcom_task_start(audio_recorder_main, (unsigned int)port, 2048, 10);
}

char *i2s_help="i2s [interface] { speak | recorder} { start | stop } [TCP port]\n";

int cmd_i2s_test(int argc, char *argv[])
{
	unsigned int port;
	
	if(argc <4)
    	{
	        qcom_printf("%s", i2s_help);
	        return -1;
   	}
	if(argv[1])
		port = atoi(argv[1]);
	if(port!=0 && port!=1){
		qcom_printf("i2s port should be 0 or 1, set port to %d\n", port);
		return -1;
	}
	if(!A_STRCMP(argv[2], "speak"))
	{
		if (argc<4)
		{
			qcom_printf("%s", i2s_help);
			return -1;
		}

		if(!A_STRCMP(argv[3], "start"))
		{
			i2s_ctl_flag &= (~I2S_PLAY_STOP);
			extern void cli_audio_play(unsigned int port);
			cli_audio_play(port);
			qcom_printf("i2s speaker test start[default TCP port 6000]...\n");
		}
		else if (!A_STRCMP(argv[3], "stop"))
		{
			i2s_ctl_flag|=I2S_PLAY_STOP;
			qcom_printf("i2s speaker test stop...\n");
		}
		if(argv[4])
		{
			gl4_port_for_i2s = atoi(argv[4]);
			if(gl4_port_for_i2s<100 || gl4_port_for_i2s>30000)
				qcom_printf("TCP port should be 100~30000\n");
		}
	}
	else if(!A_STRCMP(argv[2], "recorder"))
	{
		if (argc<4)
		{
			qcom_printf("%s", i2s_help);
			return -1;
		}

		if(!A_STRCMP(argv[3], "start"))
		{
			i2s_ctl_flag &= (~I2S_SEND_STOP);
		#ifdef I2S_UDP
			int ret;
			A_UINT32 aidata[4];
			ret = swat_sscanf(argv[4], "%3d.%3d.%3d.%3d", &aidata[0], &aidata[1], &aidata[2], &aidata[3]);
		    if (ret < 0) {
		       qcom_printf("get ip wrong\n");;
		    }
			gip_for_i2s = (aidata[0] << 24) | (aidata[1] << 16) | (aidata[2] << 8) | aidata[3];
		#endif
			extern void cli_audio_recorder(unsigned int port);

			qcom_printf("i2s recorder test start[default TCP port 7000]...\n");
			cli_audio_recorder(port);
		}
		else if (!A_STRCMP(argv[3], "stop"))
		{
			extern int i2s_ctl_flag;
			i2s_ctl_flag|=I2S_SEND_STOP;
			qcom_printf("i2s recorder test stop...\n");
		}
		if(argv[4])
		{
			gl4_port_for_i2s = atoi(argv[4]);
			if(gl4_port_for_i2s<100 || gl4_port_for_i2s>30000)
				qcom_printf("TCP port should be 100~30000\n");
		}
	}

#if 1
//static i2s_api_config_s app_i2s_config_recorder={0, I2S_FREQ_48K, 16/*dsize*/, I2S_BUF_SIZE/*i2s_buf_size*/, I2S_BUFFER_CNT, 1, 1/*master mode*/};
	else if(!A_STRCMP(argv[1], "config"))
	{
		qcom_printf("Only for internal test use\n");
		i2s_ctl_flag =0;// clear all
		if(argv[2]){
			app_i2s_config_recorder.port=atoi(argv[2]);
			app_i2s_config_play.port=atoi(argv[2]);
			}
		if(argv[3]){
			app_i2s_config_recorder.freq=atoi(argv[3]);
			app_i2s_config_play.freq=atoi(argv[3]);
			}
		if(argv[4]){
			app_i2s_config_recorder.i2s_mode=atoi(argv[4]);
			app_i2s_config_play.i2s_mode=atoi(argv[4]);
			
			}
		if(argv[5]){
			app_i2s_config_recorder.i2s_buf_size=atoi(argv[5]);
			app_i2s_config_play.i2s_buf_size=atoi(argv[5]);
		}
		if(argv[6]){
			app_i2s_config_recorder.num_rx_desc=atoi(argv[6]);
			app_i2s_config_play.num_tx_desc=atoi(argv[6]);
			}
		if(argv[7]){
			app_i2s_config_recorder.dsize=atoi(argv[7]);
			app_i2s_config_play.dsize=atoi(argv[7]);
			}

		qcom_printf("I2S config: port=%d, FREQ=%d, mode=%s, dsize=%d, bufsize=%d, buf cnt=[rx:%d tx%d], \n", app_i2s_config_recorder.port,
			app_i2s_config_recorder.freq, app_i2s_config_recorder.i2s_mode?"master":"slave", app_i2s_config_recorder.dsize,app_i2s_config_recorder.i2s_buf_size,
			app_i2s_config_recorder.num_rx_desc, app_i2s_config_recorder.num_tx_desc);
	}

#endif

	else{
		qcom_printf("%s", i2s_help);
	}

	return;
}
#endif //SWAT_I2S_TEST


