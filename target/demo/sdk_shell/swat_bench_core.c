/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom/basetypes.h"
#include "qcom/socket_api.h"
#include "swat_bench_core.h"
#include "swat_parse.h"
#include "string.h"
A_UINT32 quitBenchVal = 0;

STREAM_CXT_t cxtTcpTxPara[CALC_STREAM_NUMBER];
STREAM_CXT_t cxtTcpRxPara[CALC_STREAM_NUMBER];
STREAM_CXT_t cxtUdpTxPara[CALC_STREAM_NUMBER];
STREAM_CXT_t cxtUdpRxPara[CALC_STREAM_NUMBER];
STREAM_CXT_t cxtRawTxPara[CALC_STREAM_NUMBER];
STREAM_CXT_t cxtRawRxPara[CALC_STREAM_NUMBER];

STREAM_INDEX_t tcpTxIndex[CALC_STREAM_NUMBER];
STREAM_INDEX_t tcpRxIndex[CALC_STREAM_NUMBER];
STREAM_INDEX_t udpTxIndex[CALC_STREAM_NUMBER];
STREAM_INDEX_t udpRxIndex[CALC_STREAM_NUMBER];
STREAM_INDEX_t rawTxIndex[CALC_STREAM_NUMBER];
STREAM_INDEX_t rawRxIndex[CALC_STREAM_NUMBER];

A_UINT8
swat_cxt_index_find(STREAM_INDEX_t * pStreamIndex)
{
    if (FALSE == pStreamIndex->isUsed) {
        return 0;
    }

    return CALC_STREAM_NUMBER_INVALID;
}

void
swat_cxt_index_configure(STREAM_INDEX_t * pStreamIndex)
{
    pStreamIndex->isUsed = TRUE;
}

void
swat_cxt_index_free(STREAM_INDEX_t * pStreamIndex)
{
    pStreamIndex->isUsed = FALSE;
}

void
swat_database_initial(STREAM_CXT_t * pCxtPara)
{
    /* Bench Para */
    pCxtPara->param.ipAddress = STREAM_IP_ADDRESS_DEF;
    pCxtPara->param.port = STREAM_PORT_DEF;
    pCxtPara->param.pktSize = STREAM_PKT_SIZE_DEF;
    pCxtPara->param.seconds = STREAM_SECONDS_DEF;
    pCxtPara->param.numOfPkts = STREAM_NUM_OF_PKTS_DEF;
    pCxtPara->param.mode = STREAM_MODE_DEF;
    pCxtPara->param.protocol = STREAM_PROTOCOL_DEF;
    pCxtPara->param.delay = STREAM_DELAY_DEF;
    pCxtPara->param.local_ipAddress = STREAM_IP_ADDRESS_DEF;
    pCxtPara->param.ip_hdr_inc = STREAM_RAW_IP_HDR_INC;

    /* Calc Para */
    pCxtPara->calc[DEFAULT_FD_INDEX].firstTime.milliseconds = CALC_TIME_DEF;
    pCxtPara->calc[DEFAULT_FD_INDEX].lastTime.milliseconds = CALC_TIME_DEF;
    pCxtPara->calc[DEFAULT_FD_INDEX].bytes = CALC_BYTES_DEF;
    pCxtPara->calc[DEFAULT_FD_INDEX].kbytes = CALC_KBYTES_DEF;
}

void
swat_database_print(STREAM_CXT_t * pCxtPara)
{

    SWAT_PTF("\nindex:%d,socketLocal:%d,clientFd:%d\n", pCxtPara->index,
             pCxtPara->socketLocal, pCxtPara->clientFd[DEFAULT_FD_INDEX]);

    /* Bench Para */
    IPV4_PTF(pCxtPara->param.ipAddress);

    SWAT_PTF(":%d,%d,size:%d,mode %d,time %d s/pkts %d \n",
             pCxtPara->param.port,
             pCxtPara->param.protocol,
             pCxtPara->param.pktSize,
             pCxtPara->param.mode, pCxtPara->param.seconds, pCxtPara->param.numOfPkts);

    /* Calc Para */

    SWAT_PTF("firsttime:%d,lasttime:%d,bytes:%d,Kbytes:%d \n",
             pCxtPara->calc[DEFAULT_FD_INDEX].firstTime.milliseconds, pCxtPara->calc[DEFAULT_FD_INDEX].lastTime.milliseconds,
             pCxtPara->calc[DEFAULT_FD_INDEX].bytes, pCxtPara->calc[DEFAULT_FD_INDEX].kbytes);

}

void
swat_database_set(STREAM_CXT_t * pCxtPara,
                  A_UINT32 ipAddress,
                  IP6_ADDR_T* ip6Address,
                  A_UINT32 mcAddress,
                  IP6_ADDR_T* localIp6,
                  IP6_ADDR_T* mcastIp6,
                  A_UINT32 port,
                  A_UINT32 protocol,
                  A_UINT32 ssl_inst_index,
                  A_UINT32 pktSize, A_UINT32 mode, A_UINT32 seconds, A_UINT32 numOfPkts, A_UINT32 direction,
                  A_UINT32 local_ipAddress, A_UINT32 ip_hdr_inc, A_UINT32 rawmode, A_UINT32 delay, 
                  A_UINT32 iperf_mode, A_UINT32 interval, A_UINT32 udpRate)
{
    /* Stream Para */
    pCxtPara->param.ipAddress = ipAddress;

    if(ip6Address == NULL)
        A_MEMSET(&(pCxtPara->param.ip6Address),0,sizeof(IP6_ADDR_T));
    else
        memcpy(&(pCxtPara->param.ip6Address),ip6Address->addr,sizeof(IP6_ADDR_T));
     if(localIp6 == NULL)
        A_MEMSET(&(pCxtPara->param.localIp6),0,sizeof(IP6_ADDR_T));
    else
        memcpy(&(pCxtPara->param.localIp6),localIp6->addr,sizeof(IP6_ADDR_T));
    if(mcastIp6 == NULL)
        A_MEMSET(&(pCxtPara->param.mcastIp6),0,sizeof(IP6_ADDR_T));
    else{
        pCxtPara->param.mcast_enabled = 1;
        memcpy(&(pCxtPara->param.mcastIp6),mcastIp6->addr,sizeof(IP6_ADDR_T));
    }

    if(mcAddress){
        pCxtPara->param.mcast_enabled = 1;
    }
    pCxtPara->param.mcAddress = mcAddress;
    
    pCxtPara->param.port = port;
    pCxtPara->param.protocol = protocol;
    pCxtPara->param.pktSize   = pktSize;
    pCxtPara->param.mode      = mode;
    pCxtPara->param.seconds = seconds;
    pCxtPara->param.numOfPkts = numOfPkts;
    pCxtPara->param.direction = direction;
    pCxtPara->param.local_ipAddress = local_ipAddress;
    pCxtPara->param.ip_hdr_inc      = ip_hdr_inc;
    pCxtPara->param.rawmode         = rawmode;
    pCxtPara->param.delay = delay;
    pCxtPara->param.ssl_inst_index = ssl_inst_index;
    pCxtPara->param.is_v6_enabled = v6_enabled;
    pCxtPara->param.iperf_mode = iperf_mode;
    pCxtPara->param.iperf_display_interval = interval;
    pCxtPara->param.iperf_udp_rate = udpRate;
}

void
swat_get_first_time(STREAM_CXT_t * pCxtPara, A_UINT32 index)
{
    pCxtPara->calc[index].firstTime.milliseconds = swat_time_get_ms();
}

void
swat_get_last_time(STREAM_CXT_t * pCxtPara, A_UINT32 index)
{
    pCxtPara->calc[index].lastTime.milliseconds = swat_time_get_ms();
}

void
swat_bytes_calc(STREAM_CXT_t * pCxtPara, A_UINT32 bytes, A_UINT32 index)
{
    pCxtPara->calc[index].bytes += bytes;
    if (0 != pCxtPara->calc[index].bytes / 1024) {
        pCxtPara->calc[index].kbytes += pCxtPara->calc[index].bytes / 1024;
        pCxtPara->calc[index].bytes = pCxtPara->calc[index].bytes % 1024;
    }
}

void
swat_socket_close(STREAM_CXT_t * pCxtPara)
{
    if (-1 != pCxtPara->socketLocal) {
        swat_close(pCxtPara->socketLocal);
        pCxtPara->socketLocal = -1;
    }
}

void
swat_buffer_free(A_UINT8 ** pDataBuffer)
{
    if (NULL != *pDataBuffer) {
        swat_mem_free(*pDataBuffer);
        *pDataBuffer = NULL;
    }
}

A_UINT32
swat_check_time(STREAM_CXT_t * pCxtPara)
{
    A_UINT32 msInterval = 0;
    A_UINT32 totalInterval = 0;

    msInterval = (pCxtPara->calc[DEFAULT_FD_INDEX].lastTime.milliseconds - pCxtPara->calc[DEFAULT_FD_INDEX].firstTime.milliseconds);
    totalInterval = msInterval;

    if (totalInterval >= pCxtPara->param.seconds * 1000) {
        return 1;
    } else {
        return 0;
    }
}

void
swat_test_iperf_result_print(STREAM_CXT_t * pCxtPara, A_UINT32 index, A_UINT32 prev, A_UINT32 cur)
{
    A_UINT32 throughput_Kbps = 0;
    A_UINT32 msInterval;
    A_UINT32 bytes, rem_bytes = 0;
	char *transfer_unit = " ";
	char *bandwidth_unit = " ";
	A_UINT32 sec_val1, sec_val2;

    msInterval = cur - prev;
    pCxtPara->calc[index].kbytes += pCxtPara->calc[index].bytes / 1024;

    if (msInterval > 0) {
    	throughput_Kbps = (pCxtPara->calc[index].bytes / msInterval) * 8;
    	bytes = pCxtPara->calc[index].bytes;
    	sec_val1 = pCxtPara->param.iperf_time_sec;
    	sec_val2 = pCxtPara->param.iperf_time_sec + pCxtPara->param.iperf_display_interval;

    	if (bytes > 1024*1024) {
    		transfer_unit = "M";
    		rem_bytes = ((bytes % (1024*1024)) * 100) / (1024*1024);
    		bytes /= 1024*1024;
    	}
    	else if (bytes > 1024) {
    		transfer_unit = "K";
    		rem_bytes = ((bytes % (1024)) * 100) / 1024;
    		bytes /= 1024;
    	}

    } else {
    	msInterval = (pCxtPara->calc[index].lastTime.milliseconds - pCxtPara->calc[index].firstTime.milliseconds);

    	if (msInterval == 0) {
    		return; /* error */
    	}

    	pCxtPara->param.iperf_time_sec -= pCxtPara->param.iperf_display_interval;

    	/* Final stats */
    	throughput_Kbps = (pCxtPara->calc[index].kbytes /
    			(msInterval/1000)) * 8;

    	sec_val1 = 0;
    	sec_val2 = msInterval/1000;
    	bytes = pCxtPara->calc[index].kbytes; /* Note: working with KB */

    	if (bytes > 1024*1024) {
    		transfer_unit = "G";
    		rem_bytes = ((bytes % (1024*1024)) * 100) / (1024*1024);
    		bytes /= 1024*1024;
    	}
    	else if (bytes > 1024) {
    		transfer_unit = "M";
    		rem_bytes = ((bytes % (1024)) * 100) / 1024;
    		bytes /= 1024;
    	} else if (bytes) {
    		transfer_unit = "K";
    	}
    }

	if (throughput_Kbps > 1000) {
		bandwidth_unit = "M";
	}
	else if (pCxtPara->calc[index].bytes > 0) {
		bandwidth_unit = "K";
	}
       if (throughput_Kbps > 1000)  {
	app_printf("[%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec\n",
			pCxtPara->param.iperf_stream_id,
			sec_val1, sec_val2,
			bytes, rem_bytes, transfer_unit,
			throughput_Kbps/1000, (throughput_Kbps%1000)/10, bandwidth_unit);
       } else if (pCxtPara->calc[index].bytes > 0) {
	app_printf("[%3d] %2d.0-%2d.0 sec %3d.%02d %sBytes %d.%02d %sbits/sec\n",
			pCxtPara->param.iperf_stream_id,
			sec_val1, sec_val2,
			bytes, rem_bytes, transfer_unit,
			throughput_Kbps, 0, bandwidth_unit);       
       }

	/* Clear for next time */
	pCxtPara->calc[index].bytes = 0;
	pCxtPara->param.iperf_time_sec += pCxtPara->param.iperf_display_interval;
}

void
swat_test_result_print(STREAM_CXT_t * pCxtPara, A_UINT32 index)
{
    A_UINT32 throughput = 0;
    A_UINT32 totalBytes = 0;
    A_UINT32 totalKbytes = 0;
    A_UINT32 msInterval = 0;
    A_UINT32 totalInterval = 0;
    A_UINT32 sInterval = 0;//seconds

    msInterval = (pCxtPara->calc[index].lastTime.milliseconds - pCxtPara->calc[index].firstTime.milliseconds);
    totalInterval = msInterval;
    sInterval = totalInterval/1000;
    if (totalInterval > 0) {
        if ((0 == pCxtPara->calc[index].bytes)
            && (0 == pCxtPara->calc[index].kbytes)) {
            throughput = 0;
        } else {
        	/*when calc.kbytes >0x7FFFF, the value of (pCxtPara->calc.kbytes * 1024 * 8)
        		will overflow */
        	if (pCxtPara->calc[index].kbytes < 0x7FFFF){
	            throughput =
	                ((pCxtPara->calc[index].kbytes * 1024 * 8) / (totalInterval)) +
	                ((pCxtPara->calc[index].bytes * 8) / (totalInterval));
    		}
			else{
				throughput =((pCxtPara->calc[index].kbytes * 8) / (sInterval))
					 	+ ((pCxtPara->calc[index].bytes * 8/1024) / (sInterval));
			}
	        totalBytes = pCxtPara->calc[index].bytes;
            totalKbytes = pCxtPara->calc[index].kbytes;
        }
    } else {
        throughput = 0;
    }

    switch (pCxtPara->param.protocol) {
    case TEST_PROT_UDP:
    case TEST_PROT_DTLS:
        {
            if (TEST_DIR_RX == pCxtPara->param.direction) {
                SWAT_PTF("\nResults for %s test:\n\n", "UDP Receive");
            } else {
                SWAT_PTF("\nResults for %s test:\n\n", "UDP Transmit");
            }

            break;
        }
    case TEST_PROT_SSL:
    case TEST_PROT_TCP:
        {
            if (TEST_DIR_RX == pCxtPara->param.direction) {
                SWAT_PTF("\nResults for %s test:\n\n", "TCP Receive");
            } else {
                SWAT_PTF("\nResults for %s test:\n\n", "TCP Transmit");
            }

            break;
        }
#if SWAT_BENCH_RAW
    	case TEST_PROT_RAW:
        {
            if (TEST_DIR_RX == pCxtPara->param.direction)
            {
                SWAT_PTF("\nResults for %s test:\n\n", "RAW Receive");
            }
            else
            {
                SWAT_PTF("\nResults for %s test:\n\n", "RAW Transmit");
            }

            break;
        }
#endif
        default:
        {
            SWAT_PTF("\nUnknown Protocol:\n\n");
            break;
        }
    }

    SWAT_PTF("\t%llu Bytes in %d seconds %d ms  \n", (A_UINT64)(totalKbytes * 1024 + totalBytes),
             totalInterval / 1000, totalInterval % 1000);
    SWAT_PTF("\t%d KBytes %d bytes in %d seconds %d ms  \n\n", totalKbytes, totalBytes,
             totalInterval / 1000, totalInterval % 1000);
    SWAT_PTF("\t throughput %d kb/sec\n", throughput);
}

A_UINT32
swat_bench_quit()
{
    return quitBenchVal;
}

void
swat_bench_quit_init()
{
    quitBenchVal = 0;
}

void
swat_bench_quit_config()
{
    quitBenchVal = 1;
}

void
swat_bench_dbg()
{
    A_UINT32 index = 0;

    A_UINT32 ret = 0;

    extern A_UINT32 allocram_remaining_bytes;
    SWAT_PTF("### SWAT MEM FREE : %d.\n", allocram_remaining_bytes);
    for (index = 0; index < CALC_STREAM_NUMBER; index++) {

        /*tcpTx */
        SWAT_PTF("\ntcpTx[%d]:\n", index);
        ret = swat_cxt_index_find(&tcpTxIndex[index]);

        if (CALC_STREAM_NUMBER_INVALID == ret) {
            swat_database_print(&cxtTcpTxPara[index]);
        }

        /*tcpRx */
        SWAT_PTF("\ntcpRx[%d]:\n", index);
        ret = swat_cxt_index_find(&tcpRxIndex[index]);

        if (CALC_STREAM_NUMBER_INVALID == ret) {
            swat_database_print(&cxtTcpRxPara[index]);
        }
        /*udpTx */
        SWAT_PTF("\nudpTx[%d]:\n", index);
        ret = swat_cxt_index_find(&udpTxIndex[index]);

        if (CALC_STREAM_NUMBER_INVALID == ret) {
            swat_database_print(&cxtUdpTxPara[index]);
        }

        /*udpRx */
        SWAT_PTF("\nudpRx[%d]:\n", index);
        ret = swat_cxt_index_find(&udpRxIndex[index]);

        if (CALC_STREAM_NUMBER_INVALID == ret) {
            swat_database_print(&cxtUdpRxPara[index]);
		}
#if SWAT_BENCH_RAW
		/*rawTx*/
		SWAT_PTF("\nrawTx[%d]:\n",index);
		ret = swat_cxt_index_find(&rawTxIndex[index]);

		if (CALC_STREAM_NUMBER_INVALID == ret)
		{
		    swat_database_print(&cxtRawTxPara[index]);
		}

		/*rawRx*/
		SWAT_PTF("\nrawRx[%d]:\n",index);
		ret = swat_cxt_index_find(&rawRxIndex[index]);

		if (CALC_STREAM_NUMBER_INVALID == ret)
		{
		   swat_database_print(&cxtRawRxPara[index]);
		}
#endif
	}
}


char
hextoa(int val)
{
   val &= 0x0f;
   if(val < 10)
      return (char)(val + '0');
   else
      return (char)(val + 55);   /* converts 10-15 -> "A-F" */
}


char * print_ip6(IP6_ADDR_T * addr, char * str)
{
   int         i;
   unsigned short *   up;
   char *      cp;
   unsigned short     word;
   int         skip = 0;   /* skipping zeros flag */

   if(addr == NULL)     /* trap NULL pointers */
      return NULL;

   up = (unsigned short *)addr;
   cp = str;

   for(i = 0; i < 8; i++)  /* parse 8 16-bit words */
   {
      word =htons(*up);
      up++;

      /* If word has a zero value, see if we can skip it */
      if(word == 0)
      {
         /* if we haven't already skipped a zero string... */
         if(skip < 2)
         {
            /* if we aren't already skipping one, start */
            if(!skip)
            {
               skip++;
               if (i == 0)
                  *cp++ = ':';
            }
            continue;
         }
      }
      else
      {
         if(skip == 1)  /* If we were skipping zeros... */
         {
            skip++;        /* ...stop now */
            *cp++ = ':';   /* make an extra colon */
         }
      }

      if(word & 0xF000)
         *cp++ = hextoa(word >> 12);
      if(word & 0xFF00)
         *cp++ = hextoa((word & 0x0F00) >> 8);
      if(word & 0xFFF0)
         *cp++ = hextoa((word & 0x00F0) >> 4);
      *cp++ = hextoa(word & 0x000F);
      *cp++ = ':';
   }
   if(skip == 1)  /* were we skipping trailing zeros? */
   {
      *cp++ = ':';
      *cp = 0;
   }
   else
      *--cp = 0;  /* turn trailing colon into null */
   return str;
}


int ishexdigit(char digit)
{
   if((digit >= '0' ) && (digit <= '9'))
     return 1;

   digit |= 0x20;       /* mask letters to lowercase */
   if ((digit >= 'a') && (digit <= 'f'))
      return 1;
   else
      return 0;
}

unsigned int hexnibble(char digit)
{
   if (digit <= '9')
      return (digit-'0'    );

   digit &= ~0x20;   /* letter make uppercase */
   return (digit-'A')+10 ;
}


unsigned int atoh(char * buf)
{
   unsigned int retval = 0;
   char *   cp;
   char  digit;

   cp = buf;

   /* skip past spaces and tabs */
   while (*cp == ' ' || *cp == 9)
      cp++;

   /* while we see digits and the optional 'x' */
   while (ishexdigit(digit = *cp++) || (digit == 'x'))
   {
      /* its questionable what we should do with input like '1x234',
       * or for that matter '1x2x3', what this does is ignore all
       */
      if (digit == 'x')
         retval = 0;
      else
         retval = (retval << 4) + hexnibble(digit);
   }

   return retval;
}




char * inet6_ntoa(char* addr, char * str)
{
   int         i;
   unsigned short *   up;
   char *      cp;
   unsigned short     word;
   int         skip = 0;   /* skipping zeros flag */

   if(addr == NULL)     /* trap NULL pointers */
      return NULL;

   up = (unsigned short *)addr;
   cp = str;

   for(i = 0; i < 8; i++)  /* parse 8 16-bit words */
   {
      word = htons(*up);
      up++;

      /* If word has a zero value, see if we can skip it */
      if(word == 0)
      {
         /* if we haven't already skipped a zero string... */
         if(skip < 2)
         {
            /* if we aren't already skipping one, start */
            if(!skip)
            {
               skip++;
               if (i == 0)
                  *cp++ = ':';
            }
            continue;
         }
      }
      else
      {
         if(skip == 1)  /* If we were skipping zeros... */
         {
            skip++;        /* ...stop now */
            *cp++ = ':';   /* make an extra colon */
         }
      }

      if(word & 0xF000)
         *cp++ = hextoa(word >> 12);
      if(word & 0xFF00)
         *cp++ = hextoa((word & 0x0F00) >> 8);
      if(word & 0xFFF0)
         *cp++ = hextoa((word & 0x00F0) >> 4);
      *cp++ = hextoa(word & 0x000F);
      *cp++ = ':';
   }
   if(skip == 1)  /* were we skipping trailing zeros? */
   {
      *cp++ = ':';
      *cp = 0;
   }
   else
      *--cp = 0;  /* turn trailing colon into null */
   return str;
}

char * 
swat_strchr(char *str, char chr)
{
   do
   {
      if (*str == chr)
         return (str);
   } while (*str++ != '\0');

   return ((char *)NULL);     /* character not found */
}


int
Inet6Pton(char * src, void * dst)
{
   char *   cp;      /* char after previous colon */
   A_UINT16 *      dest;    /* word pointer to dst */
   int            colons;  /* number of colons in src */
   int            words;   /* count of words written to dest */

   /* count the number of colons in the address */
   cp = src;
   colons = 0;
   while(*cp)
   {
      if(*cp++ == ':') colons++;
   }

   if(colons < 2 || colons > 7)
   {
/*      printf("must have 2-7 colons");*/
      return 1;
   }

   /* loop through address text, parseing 16-bit chunks */
   cp = src;
   dest = dst;
   words = 0;

   if(*cp == ':') /* leading colon has implied zero, e.g. "::1" */
   {
      *dest++ = 0;
      words++;
      cp++;       /* bump past leading colon, eg ":!" */
   }

   while(*cp > ' ')
   {
      if(words >= 8)
      {
	 printf("***  inet_pton: logic error?\n");
         return 1;
      }
      if(*cp == ':')   /* found a double-colon? */
      {
         int i;
         for(i = (8 - colons); i > 0; i--)
         {
            *dest++ = 0;   /* add zeros to dest address */
            words++;       /* count total words */
         }
         cp++;             /* bump past double colon */
         if(*cp <= ' ')    /* last colon was last char? */
         {
            *dest++ = 0;   /* add one final zero */
            words++;       /* count total words */
         }
      }
      else
      {
         A_UINT16 wordval;
         A_UINT16 temp;
         wordval = atoh(cp);
	 temp = wordval;
         wordval = htons(temp);    /* get next 16 bit word */
         if((wordval == 0) && (*cp != '0'))  /* check format */
         {
            printf("must be hex numbers or colons \n");
            return 1;
         }
         *dest++ = wordval;
         words++;       /* count total words set in dest */
         cp = swat_strchr((char *)cp, ':');   /* find next colon */
         if(cp)                  /* bump past colon */
            cp++;
         else                 /* no more colons? */
            break;            /* done with parsing */
      }
   }
   if(words != 8)
   {
      printf("too short - missing colon?\n");
      return 1;
   }
   return 0;
}





