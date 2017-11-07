#include "qcom_common.h" 
#include "swat_wmiconfig_common.h" 
#include "qcom_gpio.h" 
  
#include "socket_api.h" 
#include "select_api.h" 

#include "qcom_set_Nf.h"

#include "qcom_timer.h"

#include "threadx/tx_api.h"
#include "threadx/tx_thread.h"

extern A_UINT64 qcom_time_us();

extern A_UINT8 currentDeviceId;

#define BUFFER_SIZE 100
extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();


TX_SEMAPHORE utimer_so_sem;

int us_timer_flag = 0;

int us_time_num = 15625;
unsigned long u_time_1 = 0;
unsigned long u_time_2 = 0;
unsigned long u_time_3 = 0;

unsigned int tick1 = 0;
unsigned int tick2 = 0;
unsigned int tick3 = 0;

unsigned int gpio_flag = 0;

qcom_timer_t us_timer;

unsigned int test_flag = 0;
unsigned int period_flag = 0;
unsigned int ad_data[4] = {0};
unsigned int ip_address = 0;
unsigned int port = 0;

void us_timer_handler(unsigned int alarm, void *arg)
{
    unsigned int send_status;

    gpio_flag = ~gpio_flag;
    qcom_gpio_pin_set(18,gpio_flag);

    if(period_flag)
        u_time_1 = u_time_2;

    u_time_2 = qcom_time_us();
 //   SWAT_PTF("us 1_is %d, us 2 is %d.the diff us timer is %d.\n",u_time_1,u_time_2,u_time_2-u_time_1); 
    send_status = tx_semaphore_put(&utimer_so_sem);

}



void us_timer_config(int timer_flag)
{        /*GPIO output square ware to test 15.625ms*/
        /*using GPIO 18 */
        qcom_gpio_pin_mux(18, 0);
        qcom_gpio_pin_dir(18, 0);
        qcom_gpio_pin_set(18,1);

       u_time_1 = qcom_time_us();
        if(timer_flag)
          qcom_timer_init(&us_timer,us_timer_handler,NULL,us_time_num,1);//period
        else 
          qcom_timer_init(&us_timer,us_timer_handler,NULL,us_time_num,0);//oneshot

        qcom_timer_us_start(&us_timer);
        us_timer_flag  = 1 ;
}

void us_timer_socket_send()
{

	A_INT32 socket_udp = -1;
        A_UINT32 rcv_status;
        
        A_UINT32 sendto_status;

        struct sockaddr_in sin_addr;
    	
        A_CHAR sendBuffer[BUFFER_SIZE];     /* Buffer for  string */

  	socket_udp = qcom_socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_udp < 0)
	{
		goto done;
	}

        sin_addr.sin_family = AF_INET;
  	sin_addr.sin_addr.s_addr = htonl(ip_address);
  	sin_addr.sin_port = htons(port);

        tx_semaphore_create(&utimer_so_sem, "utimer sem", 0);
       
        memset(sendBuffer, 0xfe, BUFFER_SIZE);

        while(1)
            {
                rcv_status = tx_semaphore_get(&utimer_so_sem, TX_WAIT_FOREVER); 
                if (rcv_status == TX_SUCCESS)
		{
                   u_time_3 = qcom_time_us();
                   //SWAT_PTF("sem get time is %d,the time between send and get sem is %d.\n",u_time_3,u_time_3-u_time_2);
                   sendto_status = qcom_sendto(socket_udp, sendBuffer, strlen(sendBuffer),0, (struct sockaddr *)&sin_addr, sizeof(sin_addr));                 
                  if(test_flag)
                    {
                       SWAT_PTF("test break.\n");
                       break;
                    } 
		}
            }

    	if (tx_semaphore_delete(&utimer_so_sem)!= TX_SUCCESS)
         {   
		SWAT_PTF("delete sem error\n");
         }
        qcom_thread_msleep(100);
	SWAT_PTF("task exit\n");
    
done:

	if (socket_udp > 0)
	{
		qcom_socket_close(socket_udp);
	}
	qcom_task_exit();
	return;

}



void us_timer_socket_echo()
{

	A_INT32 ret = -1;
	A_INT32 socket_udp = -1;
	A_INT32 from_size;
        A_UINT32 rcv_status;

        
        A_UINT32 sendto_bytes;

        A_UINT32 recvlen=0;
        struct sockaddr_in local_addr;
        struct sockaddr_in from_addr;
    	
        A_CHAR recvBuffer[BUFFER_SIZE];     /* Buffer for  string */

  	socket_udp = qcom_socket(AF_INET, SOCK_DGRAM, 0);

    if (socket_udp < 0)
	{
		goto done;
	}

        local_addr.sin_family = AF_INET;
  	local_addr.sin_addr.s_addr = htonl(INADDR_ANY);
  	local_addr.sin_port = htons(port);

         ret = qcom_bind(socket_udp, (struct sockaddr *)&local_addr, sizeof(struct sockaddr_in)); 

	if (ret < 0)
	{
		SWAT_PTF("Bind fail at %d.\n", socket_udp);
		goto done;
	}

        tx_semaphore_create(&utimer_so_sem, "utimer sem", 0);
	
        while(1)
            {
                rcv_status = tx_semaphore_get(&utimer_so_sem, TX_WAIT_FOREVER); 
                if (rcv_status == TX_SUCCESS)
		{
                   u_time_3 = qcom_time_us();
                   //SWAT_PTF("sem get time is %d,the time between send and get sem is %d.\n",u_time_3,u_time_3-u_time_2);

                   from_size = sizeof(from_addr);
                   memset(recvBuffer,0,BUFFER_SIZE);
                   recvlen= qcom_recvfrom(socket_udp, recvBuffer, sizeof(recvBuffer),0, (struct sockaddr *)&from_addr,&from_size); 
                    
                   SWAT_PTF("UDP recv: %s , len is %d.\n",recvBuffer,recvlen);
         
                 if (A_STRNCMP(recvBuffer, "stop", 4) == 0)
                     { 
                        SWAT_PTF("Sender want end the connection\n"); 
                        break; 
                     }

                 if(recvlen > 0)
                     sendto_bytes = qcom_sendto(socket_udp, recvBuffer, strlen(recvBuffer),0, (struct sockaddr *)&from_addr, sizeof(from_addr));                 
                 if(sendto_bytes > 0) 
                   SWAT_PTF("sendto len is %d.\n",sendto_bytes);

                 if(test_flag)
                    {
                       SWAT_PTF("test break.\n");
                       break;
                    } 
		}
            }
    	if (tx_semaphore_delete(&utimer_so_sem)!= TX_SUCCESS)
          {   
     	    SWAT_PTF("delete sem error\n");
          }
        qcom_thread_msleep(100);
	SWAT_PTF("task exit\n");
    
done:

	if (socket_udp > 0)
	{
		qcom_socket_close(socket_udp);
	}
	qcom_task_exit();
	return;

}


int cmd_timer_socket_test(int argc, char *argv[])
{
   if(argc < 2)
   {
     SWAT_PTF("usage:timer_socket --us_timer {echo|send|run} {period|oneshot} {test_flag} {IP} {PORT}\n");
     SWAT_PTF("EX: timer_socket --us_timer send 1 0 tx_ip port : send data every 15.625ms.\n");
     SWAT_PTF("EX: timer_socket --us_timer echo 0 1 rx_ip port: recv data and echo when 15.625ms expired.\n");
     SWAT_PTF("EX: timer_socket --us_timer run  1 0 0 0");

     return -1;

   }


   if (!A_STRCMP(argv[1], "--us_timer"))
     {
        if(argc < 6)
         {
            SWAT_PTF("command not right\n");
            return -1;
         }
        period_flag = atoi(argv[3]);   //  1: period; 0: oneshot

        if(us_timer_flag)
         {
           SWAT_PTF("us timer existed, stop it before started again.\n");
           return -1;
         }
        else
        us_timer_config(period_flag); 

        test_flag = atoi(argv[4]); // used to stop while test

        sscanf(argv[5], "%3d.%3d.%3d.%3d", &ad_data[0], &ad_data[1], &ad_data[2], &ad_data[3]);

        ip_address = (ad_data[0] << 24) | (ad_data[1] << 16) | (ad_data[2] << 8) | ad_data[3];
  
        port = atoi(argv[6]);

        if (!A_STRCMP(argv[2], "echo"))         // receive the UDP data and send back at 15.625ms
	   qcom_task_start(us_timer_socket_echo, 0, 2048, 10);

        else if (!A_STRCMP(argv[2], "send"))    // just send UDP data when 15.625ms arrive
	   qcom_task_start(us_timer_socket_send, 0, 2048, 10);
        else if (!A_STRCMP(argv[2], "run"))    
            SWAT_PTF("only run timer.\n");


     }
   else if (!A_STRCMP(argv[1], "--exit"))
    {
	qcom_timer_stop(&us_timer);
        us_timer_flag  = 0 ;
        SWAT_PTF("timer stop.\n");
	return SWAT_OK;

    }

}

