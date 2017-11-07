/*
  * Copyright (c) 2013 Qualcomm Atheros, Inc..
  * All Rights Reserved.
  * Qualcomm Atheros Confidential and Proprietary.
  */

#include "qcom_common.h"
#include "swat_wmiconfig_common.h"
#include "swat_bench_core.h"
#include "qcom_multicast_filter.h"


#define ADD_SRVR_ADDR 1
#define DEL_SRVR_ADDR 2
#define SSL_FILENAME_LEN   20
#define SUBMASK_NUM 8

/* Set defaults for Base & range on AUtoIP address pool */
unsigned long   dBASE_AUTO_IP_ADDRESS = AUTOIP_BASE_ADDR;    /* 169.254.1.0 */
unsigned long   dMAX_AUTO_IP_ADDRESS  = AUTOIP_MAX_ADDR;     /* 169.254.254.255 */

//extern A_STATUS qcom_ip6_address_get(A_UINT8 device_id, IP6_ADDR_T *v6Global, IP6_ADDR_T *v6Link, IP6_ADDR_T *v6DefGw,IP6_ADDR_T *v6GlobalExtd, A_INT32 *LinkPrefix,
//		      A_INT32 *GlbPrefix, A_INT32 *DefgwPrefix, A_INT32 *GlbPrefixExtd);
extern char * print_ip6(IP6_ADDR_T * addr, char * str);

extern	int Inet6Pton(char * src, void * dst);
extern char * swat_strchr(char *str, char chr);
A_UINT32
_inet_addr(A_CHAR *str)
{
    A_UINT32 ipaddr;
    A_UINT32 data[4];
    A_UINT32 ret;

    ret = A_SSCANF(str, "%3d.%3d.%3d.%3d", data, data + 1, data + 2, data + 3);
    if (ret < 0) {
        return 0;
    } else {
        ipaddr = data[0] << 24 | data[1] << 16 | data[2] << 8 | data[3];
    }
  
    return ipaddr;
}

char str[20];

A_CHAR *
_inet_ntoa(A_UINT32 ip)
{
    A_MEMSET(str, 0, sizeof (str));
    qcom_sprintf(str, "%u.%u.%u.%u",
            (unsigned) (ip >> 24),
            (unsigned) ((ip >> 16) & 0xff), (unsigned) ((ip >> 8) & 0xff), (unsigned) (ip & 0xff));
    return str;
}

void
swat_wmiconfig_set_ping_id(A_UINT8 device_id,A_UINT32 PingId)
{
    SWAT_PTF("Change the ping ID to be : %d\n", PingId);
    qcom_set_ping_id(PingId);
    return;
}

A_BOOL
swat_check_netmask(A_UINT32 submask)
{
	A_UINT32 i;
	A_UINT32 flone = 31;
	A_UINT32 fhzero =0;
	A_BOOL valid;
	
	for(i=0;i<32;i++)
	{
		if((submask>>i)&0x1)
		{
			flone = i;
			break;
		}
	}
	for(i=0;i<32;i++)
	{
		if(!((submask<<i)&0x80000000))
		{
			fhzero = 31-i;
			break;
		}
	}
	valid = (flone>=fhzero)&&(fhzero<31);
	
	return valid;
}
		

    void
swat_wmiconfig_ipstatic(A_UINT8 device_id, A_CHAR * pIpAddr, A_CHAR * pNetMask, A_CHAR * pGateway)
{
    A_UINT32 address;
    A_UINT32 submask;
    A_UINT32 gateway;
    A_UINT32 valid_addr[4];
    A_UINT32 rc;


	submask = _inet_addr(pNetMask);
     if(!swat_check_netmask(submask))
      {
          SWAT_PTF("Invalid submask.\n");
          return;
      }


     /*C tpye address ip address region should be in 1~254*/
     rc = A_SSCANF(pIpAddr, "%3d.%3d.%3d.%3d", valid_addr, valid_addr + 1, valid_addr + 2, valid_addr+ 3);

     if(rc < 0)
      {
         return;
      }
     else if((valid_addr[3] >= 255)||(valid_addr[3] == 0))
      {
    
          SWAT_PTF("Invalid Address.\n");
          return ;
      }
  
    
    address = _inet_addr(pIpAddr);
    gateway = _inet_addr(pGateway);

    qcom_ipconfig(device_id, IP_CONFIG_SET, &address, &submask, &gateway);
}

    void
swat_wmiconfig_ipdhcp(A_UINT8 device_id)
{
    A_UINT32 param;
    qcom_ipconfig(device_id, IP_CONFIG_DHCP, &param, &param, &param);
}

    void
swat_wmiconfig_ipauto(A_UINT8 device_id, A_CHAR * pIpAddr)
{
    A_UINT32 param;
    A_UINT32 address = 0;

    if(pIpAddr != NULL)
    {
        address = _inet_addr(pIpAddr);
        
        if(address < dBASE_AUTO_IP_ADDRESS || address > dMAX_AUTO_IP_ADDRESS)
        {
            SWAT_PTF("Invalid link local IPv4 address\n");
            return;
        }
    }
    
    qcom_ipconfig(device_id, IP_CONFIG_AUTO, &address, &param, &param);
}


    void
swat_wmiconfig_dns(A_UINT8 device_id, A_CHAR *name)
{
    A_UINT32 ipAddress;
    if (qcom_dnsc_get_host_by_name(name, &ipAddress)== A_OK) {
        A_CHAR *ipaddr;
        ipaddr = _inet_ntoa(ipAddress);
        SWAT_PTF("Get IP address of host %s \n", name);
        SWAT_PTF("ip address is %s\n", (char *)_inet_ntoa(ipAddress));
    } else {
        SWAT_PTF("The IP of host %s is not gotten\n", name);
    }

    return;
}

void
swat_wmiconfig_dns2(A_UINT8 device_id, A_CHAR * name, A_INT32 domain, A_UINT32 mode)
{
    IP6_ADDR_T ip6Addr;
    char ip6buf[48];
    char *ip6ptr =  NULL;
    A_UINT32 ipAddress;
    A_CHAR *ipaddr;
    A_STATUS ret;

   ret = qcom_dnsc_get_host_by_name2(name, (A_UINT32*)(&ip6Addr), domain,  mode);
   if (ret != A_OK){
        SWAT_PTF("The IP of host %s is not gotten\n", name);
        return;
   }

   SWAT_PTF("Get IP address of host %s \n", name);
   if (domain == AF_INET6){
        ip6ptr = print_ip6(&ip6Addr, ip6buf);
        SWAT_PTF("ip address is %s\n", ip6buf);
    } else {
        memcpy(&ipAddress, &ip6Addr, 4);
        ipAddress = ntohl(ipAddress);
        ipaddr = _inet_ntoa(ipAddress);
        SWAT_PTF("ip address is %s\n", ipaddr);
    }

    return;
}

void
swat_wmiconfig_dns3(A_UINT8 device_id, A_CHAR * name, A_INT32 domain, A_UINT32 mode)
{
    IP6_ADDR_T ip6Addr;
    char ip6buf[48];
    char *ip6ptr =  NULL;
    A_UINT32 ipAddress;
    A_CHAR *ipaddr;
    struct hostent *dns_tmp = NULL;
    int i = 0;
    
        dns_tmp = qcom_dns_get(mode, domain, name); //mode: 0-GETHOSTBYNAME; 1-GETHOSTBYNAME2; 2-RESOLVEHOSTNAME
        if (NULL == dns_tmp) {
            qcom_printf("Can't get IP addr by host name %s\n", name);
            return ;
        }
        SWAT_PTF("Get IP address of host %s \nip address is", name);
        if (domain == AF_INET6){
            for(;i < MAX_DNSADDRS;i++){
                if(dns_tmp->h_addr_list[i]){
                    memcpy(&ip6Addr, dns_tmp->h_addr_list[i], 16);
                    ip6ptr = print_ip6(&ip6Addr, ip6buf);
                    SWAT_PTF("  %s", ip6buf);
                 }
            }
            
            SWAT_PTF("\n");
        }else{

            for(i=0;i < MAX_DNSADDRS;i++){
                if(dns_tmp->h_addr_list[i]){
                    memcpy(&ipAddress, dns_tmp->h_addr_list[i], 4);
                    ipAddress = ntohl(ipAddress);
                    ipaddr = _inet_ntoa(ipAddress);
                    SWAT_PTF("  %s", ipaddr);
                }
            }
            
             SWAT_PTF("\n");
        }

    return;
}

    void
swat_wmiconfig_dns_enable(A_UINT8 device_id, A_CHAR *enable)
{
    int flag;
    if (!strcmp(enable, "start"))
        flag = 1;
    else if (!strcmp(enable, "stop"))
        flag = 0;
    else {
        SWAT_PTF("input parameter should be start or stop !\n");
        return;
    }
    /* 1: enable; 0:diable; */
    qcom_dnsc_enable(flag);
    return;
}

    void
swat_wmiconfig_dns_svr_add(A_UINT8 device_id, A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
    A_UINT8 dnsSerIp[16];
    DnsSvrAddr_t dns[MAX_DNSADDRS];
	A_UINT32 number = 0;
	A_UINT32 i = 0;
    A_MEMSET(dnsSerIp, 0, 16);
    A_MEMSET(dns,0,MAX_DNSADDRS*sizeof(DnsSvrAddr_t));
    qcom_dns_server_address_get(dns, &number);
    if(!Inet6Pton(ipaddr, dnsSerIp)){
		for ( ; i < number; i++)
		{
			if ((dns[i].type == AF_INET6) && (!memcmp(dns[i].addr6, dnsSerIp, 16)))
			{
				return;
			}
		}
        qcom_dnsc_add_server_address(dnsSerIp, AF_INET6);
        return;
    } else if (0 != (ip = _inet_addr(ipaddr))){
		for ( ; i < number; i++)
		{
			if ((dns[i].type == AF_INET) && (dns[i].addr4 == ip))
			{
				return;
			}
		}
        A_MEMCPY(dnsSerIp, (char*)&ip, 4);
        qcom_dnsc_add_server_address(dnsSerIp, AF_INET);
        return;
    } else {
        SWAT_PTF("input ip addr is not valid!\n");
    }

    return;
}

void
swat_wmiconfig_set_hostname(A_UINT8 device_id, A_CHAR *host_name)
{
    SWAT_PTF("Change the DHCP hostname to be : %s \n", host_name);
    qcom_set_hostname(0,host_name);
    return;
}

    void
swat_wmiconfig_dns_svr_del(A_UINT8 device_id, A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
    A_UINT8 dnsSerIp[16];
    A_MEMSET(dnsSerIp, 0, 16);
    if(!Inet6Pton(ipaddr, dnsSerIp)){
        qcom_dnsc_del_server_address(dnsSerIp, AF_INET6);
        return;
    } else if (0 != (ip = _inet_addr(ipaddr))){
        A_MEMCPY(dnsSerIp, (char*)&ip, 4);
        qcom_dnsc_del_server_address(dnsSerIp, AF_INET);
        return;
    } else {
        SWAT_PTF("input ip addr is not valid!\n");
    }

    return;
}
    void
swat_wmiconfig_dnss_enable(A_UINT8 device_id, A_CHAR *enable)
{
    int flag;
    if (!strcmp(enable, "enable"))
        flag = 1;
    else if (!strcmp(enable, "disable"))
        flag = 0;
    else {
        SWAT_PTF("input parameter should be start or stop !\n");
        return;
    }
    qcom_dnss_enable(flag);
    return;
}
	void
swat_wmiconfig_dns_domain(A_UINT8 device_id, A_CHAR *local_name)
{
    qcom_dns_local_domain(local_name);
	SWAT_PTF("set DNS local domain name as %s\n", local_name);
    return;
}

    void
swat_wmiconfig_dns_entry_add(A_UINT8 device_id, A_CHAR* hostname, A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
	A_UINT8 ipv6[16];
	A_MEMSET(ipv6, 0, 16);

	if(!Inet6Pton(ipaddr, ipv6)) {
		SWAT_PTF("input ip addr is V6!\n");
		qcom_dns_6entry_create(hostname, ipv6);
		return;
	}
	else if(0 != (ip = _inet_addr(ipaddr))){
        SWAT_PTF("input ip addr is V4!\n");
		qcom_dns_entry_create(hostname, ip);
        return;
    }
	else {
        SWAT_PTF("input ip addr is not valid!\n");
        return;
	}

}
    void
swat_wmiconfig_dns_entry_del(A_UINT8 device_id, A_CHAR* hostname, A_CHAR *ipaddr)
{
    A_UINT32 ip = 0;
	A_UINT8 ipv6[16];
	A_MEMSET(ipv6, 0, 16);

    if(!Inet6Pton(ipaddr, ipv6)) {
		SWAT_PTF("input ip addr is V6!\n");
		qcom_dns_6entry_delete(hostname, ipv6);
		return;
	}
	else if (0 != (ip = _inet_addr(ipaddr))) {
        SWAT_PTF("input ip addr is V4!\n");
		qcom_dns_entry_delete(hostname, ip);
        return;
    }
	else {
        SWAT_PTF("input ip addr is not valid!\n");
        return;
	}
}

void swat_wmiconfig_dns_set_timeout(A_UINT8 device_id, int timeout)
{
    qcom_dns_set_timeout(timeout);
/*
    qcom_param_set(
            device_id,
            QCOM_PARAM_GROUP_NETWORK,
            QCOM_PARAM_GROUP_NETWORK_DNS_TIMEOUT_SECS,
            &timeout,
            4,
            TRUE);
*/
}

void
swat_wmiconfig_ipconfig(A_UINT8 device_id)
{
    A_UINT8 macAddr[6];
    A_MEMSET(&macAddr, 0, sizeof (macAddr));
    A_UINT32 ipAddress;
    A_UINT32 submask;
    A_UINT32 gateway;
    DnsSvrAddr_t dns[MAX_DNSADDRS];
	A_UINT32 number=0;
    IP6_ADDR_T v6Global;
    IP6_ADDR_T v6GlobalExtd;
    IP6_ADDR_T v6LinkLocal;
    IP6_ADDR_T v6DefGw;
    char ip6buf[48];
    char *ip6ptr =  NULL;
    A_INT32 LinkPrefix = 0;
    A_INT32 GlobalPrefix = 0;
    A_INT32 DefGwPrefix = 0;
    A_INT32 GlobalPrefixExtd = 0;
    int i, found=0;
    
    qcom_mac_get(device_id, (A_UINT8 *) & macAddr);
    qcom_ipconfig(device_id, IP_CONFIG_QUERY, &ipAddress, &submask, &gateway);
    A_MEMSET(dns,0,MAX_DNSADDRS*sizeof(DnsSvrAddr_t));
    qcom_dns_server_address_get(dns, &number);

    SWAT_PTF("mac addr= %02x:%02x:%02x:%02x:%02x:%02x\n",
            macAddr[0], macAddr[1], macAddr[2], macAddr[3], macAddr[4], macAddr[5]);
    SWAT_PTF("IP  addr: ");
    IPV4_ORG_PTF(ipAddress);
    SWAT_PTF("\n");
    ORG_PTR("Mask    : ");
    IPV4_PTF(submask);
    SWAT_PTF("\n");
    SWAT_PTF("Gateway : ");
    IPV4_PTF(gateway);
    SWAT_PTF("\n");
    SWAT_PTF("Dns     : ");
    for(i=0;i<MAX_DNSADDRS;i++)
    {
        if (dns[i].type  == AF_INET){
            IPV4_PTF(dns[i].addr4);
            SWAT_PTF("  ");
            found++;
        } else if (dns[i].type  == AF_INET6){
            ip6ptr = print_ip6((IP6_ADDR_T*)&dns[i].addr6,ip6buf);
            if(ip6ptr)
            {
                printf("%s", ip6ptr);
                SWAT_PTF("  ");
                found++;
            }
        }

    }
    if (0 == found)
        IPV4_PTF(dns[0].addr4);        
    SWAT_PTF("\n");
    qcom_ip6_address_get(device_id,(A_UINT8*) &v6Global,(A_UINT8*)&v6LinkLocal,(A_UINT8*)&v6DefGw,(A_UINT8*)&v6GlobalExtd,&LinkPrefix,&GlobalPrefix,&DefGwPrefix,&GlobalPrefixExtd);
    if(v6LinkLocal.addr[0] )
	{
        ip6ptr = print_ip6(&v6LinkLocal,ip6buf);
	    if(ip6ptr)
        {
             if(LinkPrefix)
                 printf("Link-local IPv6 Address ..... : %s/%d \n", ip6ptr,LinkPrefix);
	         else
                 printf("Link-local IPv6 Address ..... : %s \n", ip6ptr);
	    }
        ip6ptr = NULL;
        ip6ptr = print_ip6(&v6Global,ip6buf);
        if(ip6ptr)
        {
            if(GlobalPrefix)
	            printf("Global IPv6 Address ......... : %s/%d \n", ip6ptr,GlobalPrefix);
  	        else
  	            printf("Global IPv6 Address ......... : %s \n", ip6ptr);
	    }
	    ip6ptr = NULL;
        ip6ptr = print_ip6(&v6DefGw, ip6buf);
	    if(ip6ptr)
	    {
	      if(DefGwPrefix)
	      printf("Default Gateway  ............ : %s/%d \n", ip6ptr,DefGwPrefix);
	      else
	      printf("Default Gateway  ............ : %s \n", ip6ptr);

	    }
            ip6ptr = NULL;
            ip6ptr = print_ip6(&v6GlobalExtd,ip6buf);
            if(ip6ptr)
            {
              if(GlobalPrefixExtd)
	      printf("Global IPv6 Address 2 ....... : %s/%d \n", ip6ptr,GlobalPrefixExtd);
	      else
	      printf("Global IPv6 Address 2 ....... : %s \n", ip6ptr);
	    }

	}
}

    void
swat_wmiconfig_dhcp_pool(A_UINT8 device_id, A_CHAR * pStartaddr, A_CHAR * pEndaddr, A_UINT32 leasetime)
{
    A_UINT32 startaddr, endaddr;

    startaddr = _inet_addr(pStartaddr);
    endaddr = _inet_addr(pEndaddr);

    qcom_dhcps_set_pool(device_id, startaddr, endaddr, leasetime);
}

#ifdef SWAT_WMICONFIG_SNTP
void swat_wmiconfig_sntp_enable(A_UINT8 device_id, char* enable)
{
    int flag;
    if(!strcmp(enable, "start"))
    {
        flag = 1;
    }
    else if(!strcmp(enable, "stop"))
        flag = 0;
    else
    {
        SWAT_PTF("input parameter should be start or stop !\n");
        return;
    }

    qcom_enable_sntp_client(flag);
    return;
}

void swat_wmiconfig_sntp_srv(A_UINT8 device_id, char* add_del,char* srv_addr)
{

    int flag;
    if(!strcmp(add_del,"add"))
        flag = ADD_SRVR_ADDR;
    else if(!strcmp(add_del,"delete"))
        flag = DEL_SRVR_ADDR;
    else
    {
        SWAT_PTF("input parameter should be add or delete !\n");
        return;
    }

    qcom_sntp_srvr_addr(flag,srv_addr);
    return;
}

void swat_wmiconfig_sntp_zone(A_UINT8 device_id, char* utc,char* dse_en_dis)
{
    int flag;
    int hour = 0,min = 0;
    int add_sub;
    char hr[3],mn[3],parsing_hour_min[10];

    if(strlen(utc) > 9){
         SWAT_PTF("Error : Invalid UTC string. Valid string(UTC+hour:min).Eg UTC+05:30\n\r");
         return;
     }

    if(strlen(dse_en_dis) > 8){
         SWAT_PTF("Error : Invalid DSE string.Valid string enable/disable \n\r");
         return;
      }

    strcpy(parsing_hour_min,utc);

    // UTC+xx:xx / UTC-xx:xx
    if(strlen(parsing_hour_min) != 9){
       SWAT_PTF("Error : UTC time format should be UTC+XX:XX or UTC-XX:XX\n");
       SWAT_PTF("Hour from 00 to -12/+13, minute should be 0, 30 or 45\n\r");
       return;
    }

    hr[0] = parsing_hour_min[4];
    hr[1] = parsing_hour_min[5];
    hr[2] = '\0';
    hour  = (hr[0] - '0')*10+ (hr[1] - '0');
    mn[0] = parsing_hour_min[7];
    mn[1] = parsing_hour_min[8];
    mn[2] = '\0';
    min   = (mn[0] - '0')*10+ (mn[1] - '0');

    if( (min!=0) && (min!=30) && (min!=45) ){
       SWAT_PTF("Error : UTC time offset in minutes should be 0, 30 or 45\n\r");
       return;
    }

    // valid time zone : -12,-11,...,+13,+14
    if(parsing_hour_min[3] == '+'){
         add_sub = TRUE; // time is to be added
         if((hour>14)||((14==hour)&&(min>0))){
            SWAT_PTF("Error : UTC time offset in hour from -12 to +14\n\r");
            return;
         }
      }
    else if(parsing_hour_min[3] == '-'){
         add_sub = FALSE; // time is to be subtracted
         if((hour>12)||((12==hour)&&(min>0))){
            SWAT_PTF("Error : UTC time offset in hour from -12 to +14\n\r");
            return;
         }
      }
    else{
         SWAT_PTF("Error : Only + / - operation is allowed\n\r");
         return;
        }


    if(!strcmp(dse_en_dis,"enable"))
        flag = TRUE;
    else if(!strcmp(dse_en_dis,"disable"))
        flag = FALSE;
    else
    {
        SWAT_PTF("DSE(day light saving) input parameter should be enable or disable !\n");
        return;
    }

    qcom_sntp_zone(hour,min,add_sub,flag);
    return;
}

void swat_wmiconfig_sntp_sync(A_UINT8 enable, A_UINT8 type, A_UINT32 mseconds)
{
    if(enable == 1)
    {
        qcom_sntp_sync_start(type, mseconds);
    }
    else
    {       
        qcom_sntp_sync_stop();
    }
}

void swat_wmiconfig_sntp_get_time(A_UINT8 device_id)
{
    tSntpTime time;
    char *months[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    char *Day[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

    qcom_sntp_get_time(device_id, &time);

    SWAT_PTF("\nTimestamp : %s %s %d ,%d %d:%d:%d\n\n\r", Day[time.wday],
                months[time.mon], time.yday,
                time.year, time.hour,
                time.min, time.Sec);

    return;
}

void swat_wmiconfig_sntp_show_config(A_UINT8 device_id)
{
    SNTP_QUERY_SRVR_ADDRESS srvAddr;
    int i;

    qcom_sntp_query_srvr_address(device_id,&srvAddr);

    for(i = 0;i < MAX_SNTP_SERVERS;i++)
    {
        if(srvAddr.SntpAddr[i].resolve != DNS_NAME_NOT_SPECIFIED)
        {
           SWAT_PTF("SNTP SERVER ADDRESS %s \n",srvAddr.SntpAddr[i].addr);
        }
    }
    return;
}

void swat_wmiconfig_sntp_get_time_of_day(A_UINT8 device_id)
{
   tSntpTM time;
    qcom_sntp_get_time_of_day(device_id, &time);
    SWAT_PTF("Seconds = %d \n",time.tv_sec);

    return;
}
#endif

/*Callback function called by PHOST when a client sends a command to HTTP server*/
void swat_process_tlv(void* cxt, void* buf)
{
    HTTP_EVENT_T* ev = (HTTP_EVENT_T*)buf;
    int numTLV;
    unsigned char* data;

    if(!buf)
        return;

    numTLV = ev->numTLV;
    data = ev->data;

    while(numTLV){
        short type;
        short length;
        unsigned char* val;

        /*Parse through all TLVs*/
        GET_TLV_TYPE(data, type);
        GET_TLV_LENGTH(data, length);
        val = GET_TLV_VAL(data);
        switch(type){
            case HTTP_TYPE_URI:
                SWAT_PTF("URI: %s\n",val); 
                break;
            case HTTP_TYPE_NAME:
                SWAT_PTF("NAME: %s\n",val); 
                break;
            case HTTP_TYPE_VALUE: 
                SWAT_PTF("VALUE: %s\n",val); 
                break;
            default:
                SWAT_PTF("Invalid TLV\n");
                break;

        }
        data = GET_NEXT_TLV(data, length);
        numTLV--;
    }
}

/*HTTP Server extensions -- application callbacks to all HTTP methods*/
void swat_http_post_callback(void* cxt, void* buf)
{
	SWAT_PTF("\n HTTP POST Callback\n");
	swat_process_tlv(cxt,buf);
}

void swat_http_put_callback(void* cxt, void* buf)
{
	SWAT_PTF("\n HTTP PUT Callback\n");
	swat_process_tlv(cxt,buf);
}
void swat_http_delete_callback(void* cxt, void* buf)
{
	SWAT_PTF("\n HTTP DELETE Callback\n");
	swat_process_tlv(cxt,buf);
}

int swat_http_server_header_form(A_UINT8 device_id, A_UINT8 **header_data, A_UINT16 *header_len)
{ 
#define HTTPSERVER_TIMEOUT 20
#define MAX_HTTPSERVER_HEADER_LEN 350;

    tSntpTime time;
    char *months[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
    char *Day[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

    char *data = NULL;
    A_INT32 n;
 
    *header_len = MAX_HTTPSERVER_HEADER_LEN;
    data = (char *)qcom_mem_alloc(*header_len);
	    
    *header_data = (A_UINT8*)data;
    if (data == NULL)
    {
        return A_ERROR;
    }
         
    memset(data, 0, *header_len);
    n = sprintf(data, "HTTP/1.1 200 OK\r\n");
    data += n;
	  
    qcom_sntp_get_time(device_id, &time);	
    n = sprintf(data, "Date: %s, %u %s %u %u:%u:%u GMT\r\n", Day[time.wday], time.yday, months[time.mon],
  		time.year, time.hour, time.min, time.Sec);
    data += n;
    n = sprintf(data, "Server: %s\r\n", "Qualcomm Atheros IOT web server");
    data += n;
    n = sprintf(data, "Connection: Keep-Alive\r\n");
    data += n;
    n = sprintf(data, "Keep-Alive: %d\r\n", HTTPSERVER_TIMEOUT);
    data += n;
    n = sprintf(data, "Content-Type: %s\r\n", "text/html");
    data += n;
    n = sprintf(data, "Transfer-Encoding: chunked\r\n\r\n");
    data += n;	

    *data = '\0';
    *header_len = (A_UINT16)((int)data - (int)(*header_data) + 1);

    return A_OK;
}
// Make sure the length of path uri < 32, and the last member of the ARRAY is NULL.
const char* const http_custom_uri[] = {"demo/ap_info", "custom/demo1", "custom/demo2","reset.cgi",NULL};

void swat_wmiconfig_set_http_custom_uri()
{
	qcom_set_http_custom_uri(currentDeviceId, http_custom_uri);
}

void swat_http_get_callback(void* cxt, void* buf)
{
    //SWAT_PTF("\nHTTP GET Callback\n");
    HTTP_EVENT_T* ev = (HTTP_EVENT_T*)buf;
    int numTLV;
    unsigned char* data;
    A_UINT8 url[32] = {0};
    A_UINT16 sess_index = 0xff;
    A_UINT8 *http_data = NULL;
    A_UINT16 data_len = 0;
    A_UINT8 *header_data =  NULL;
    A_UINT16 header_len = 0;

    if (!buf)
        return;

    numTLV = ev->numTLV;
    data = ev->data;

    while(numTLV){
        short type;
        short length;
        unsigned char* val;

        /*Parse through all TLVs*/
        GET_TLV_TYPE(data, type);
        GET_TLV_LENGTH(data, length);
        val = GET_TLV_VAL(data);
        switch(type){
            case HTTP_TYPE_URI:
                //SWAT_PTF("URI: %s\n",val);
	    		A_MEMCPY(url, val, length);
                break;
            case HTTP_TYPE_NAME:
                //SWAT_PTF("NAME: %s\n",val); 
                break;
            case HTTP_TYPE_VALUE: 
                //SWAT_PTF("VALUE: %s\n",val); 
                break;
            case HTTP_TYPE_SESS_INDEX:
  		        A_MEMCPY(&sess_index, val, sizeof(A_UINT16));
  		        //SWAT_PTF("Session Index : %d\n",sess_index); 
		        break;
            default:
                SWAT_PTF("Invalid TLV\n");
                break;
        }
		
        data = GET_NEXT_TLV(data, length);
        numTLV--;
    }
    
    if(!A_STRCMP((char *)url, "reset.cgi")){
        char * data = "System Reset successfuly";
        if (swat_http_server_header_form(currentDeviceId, &header_data, &header_len) == A_OK) {
            qcom_http_get_datasend(sess_index, A_STRLEN(data), url, (A_UINT8*)data, header_len, header_data);
            qcom_mem_free(header_data);
        }
        SWAT_PTF("\nSystem Reset from web client\n");
        qcom_thread_msleep(2000);     
        swat_wmiconfig_reset();
    }
    else{
        if (qcom_http_collect_scan_result(currentDeviceId, &http_data, &data_len) == A_OK)
        {
            if (swat_http_server_header_form(currentDeviceId, &header_data, &header_len) == A_OK) {
                qcom_http_get_datasend(sess_index, data_len, url, http_data, header_len, header_data);
    	        qcom_mem_free(header_data);
    	    }
    		
        	qcom_mem_free(http_data);
        }

     }
}

void swat_httpsvr_ota_callback(QCOM_OTA_STATUS_CODE_t status,A_INT32 image_size)
{
    if(status == QCOM_OTA_START)
        A_PRINTF("HTTP Server OTA start\n");
    else if(status == QCOM_OTA_OK)
        A_PRINTF("HTTP Server OTA successfully image size:%d\n",image_size);
    else
       A_PRINTF("HTTP Server OTA error : %d\n",status);
}

void swat_wmiconfig_http_server(A_UINT8 device_id, int enable, void *ctx)
{
#ifdef HTTP_ENABLED
    int res;
    res = qcom_http_server(enable, ctx);
    if(enable == 1){
        if(res == 0){
            SWAT_PTF("HTTP SERVER start successfully\n");
            /*Server started, set callback for post events*/
            qcom_http_set_post_cb(NULL, swat_http_post_callback);
            qcom_http_set_put_cb(NULL, swat_http_put_callback);
            qcom_http_set_delete_cb(NULL, swat_http_delete_callback);            
		    qcom_http_set_get_cb(NULL, swat_http_get_callback);
            qcom_httpsvr_ota_register_cb(swat_httpsvr_ota_callback);           
        }else{
            SWAT_PTF("HTTP SERVER start failed\n");
        }
    }else if(enable == 0){
        if(res == 0){
            SWAT_PTF("HTTP SERVER stop successfully\n");
        }else{
            SWAT_PTF("HTTP SERVER stop failed\n");
        }
    }else if(enable == 3){
        if(res == 0){
            SWAT_PTF("HTTPS SERVER start successfully\n");
            /*Server started, set callback for post events*/
            qcom_http_set_post_cb(NULL, swat_http_post_callback);
            qcom_http_set_put_cb(NULL, swat_http_put_callback);
            qcom_http_set_delete_cb(NULL, swat_http_delete_callback);            
		    qcom_http_set_get_cb(NULL, swat_http_get_callback);
            qcom_httpsvr_ota_register_cb(swat_httpsvr_ota_callback); 
        }else{
            SWAT_PTF("HTTPS SERVER start failed\n");
        }
    }else if(enable == 2){
        if(res == 0){
            SWAT_PTF("HTTPS SERVER stop successfully\n");
        }else{
            SWAT_PTF("HTTPS SERVER stop failed\n");
        }
    }

#endif
}

#ifdef BRIDGE_ENABLED

//extern A_STATUS qcom_bridge_mode_enable(A_UINT16 bridgemode);

void
swat_wmiconfig_bridge_mode_enable(A_UINT8 device_id)
{
	qcom_bridge_mode_enable(1);

    return;
}
#endif


/************************************************************************
* NAME: ipv4_route
*
* DESCRIPTION: Add, delete or show IPv4 route table
*               wmiconfig --ipv4route add <addr> <mask> <gw>
*               wmiconfig --ipv4route del <addr> <mask> <ifindex>
*               wmiconfig --ipv4route show
************************************************************************/
A_INT32 swat_ipv4_route(A_UINT8 device_id,A_INT32 argc, char* argv[])
{ /* Body */
    A_INT32         error = A_OK;
    A_UINT32        ifIndex, addr, mask, gw;
    A_UINT32        command;
    IPV4_ROUTE_LIST_T routelist;

    do
    {
        if(argc < 3)
        {
            printf("incomplete params\n");
            error = A_ERROR;
            break;
        }

        if(strcmp(argv[2], "add") == 0)
        {
            if(argc < 6)
            {
                printf("incomplete params\n");
                error = A_ERROR;
                break;
            }
            
            command = ROUTE_ADD;
            addr=_inet_addr(argv[3]);
            mask=_inet_addr(argv[4]);
            gw=_inet_addr(argv[5]);
            //ifIndex = atoi(argv[6]);
        }
        else if(strcmp(argv[2], "del") == 0)
        {
            if(argc < 6)
            {
                printf("incomplete params\n");
                error = A_ERROR;
                break;
            }
            
            command = ROUTE_DEL;
            addr=_inet_addr(argv[3]);
            mask=_inet_addr(argv[4]);
            ifIndex = atoi(argv[5]);
        }
        else if(strcmp(argv[2], "show") == 0)
        {
            command = ROUTE_SHOW;
        }
        else
        {
            //printf("Unknown Command \"%s\"\n", argv[2]);
            error = A_ERROR;
            break;
        }

        if(A_OK == qcom_ip4_route(device_id, command, &addr, &mask, &gw, &ifIndex, &routelist))
        {
            if(command == ROUTE_SHOW)
            {
                int i = 0;
                    printf(" %d\n..IPaddr.......mask.........nexthop...iface..\n",routelist.rtcount);
                    for(i = 0; i < routelist.rtcount; i++)
                    {
                        printf("%s  ",_inet_ntoa(routelist.route[i].address));
                        printf("%s  ",_inet_ntoa(routelist.route[i].mask));
                        printf("%s  ",_inet_ntoa(routelist.route[i].gateway));
                        printf("%d  \n",routelist.route[i].ifIndex - 1);
                    }
            }
        }
        else
        {
            printf("Route %s failed\n", (command == 0)?"addition":((command ==1)?"deletion":"show"));
        }
    }while(0);
    if (error == A_ERROR)
    {
        printf ("USAGE: wmiconfig --ipv4_route add <address> <mask> <gw>\n");
        printf ("       wmiconfig --ipv4_route del <address> <mask> <ifIndex>\n");
        printf ("       wmiconfig --ipv4_route show\n");
    }
    return error;
} /* Endbody */


/************************************************************************
* NAME: ipv6_route
*
* DESCRIPTION: Add, delete or show IPv4 route table
*               wmiconfig --ipv6route add <addr> <prfx len> <gw>
*               wmiconfig --ipv6route del <addr> <ifindex>
*               wmiconfig --ipv6route show
************************************************************************/
A_INT32 swat_ipv6_route(A_UINT8 device_id, A_INT32 argc, char* argv[])
{ /* Body */
    A_INT32         error = A_OK;
    A_UINT32        ifIndex, prefixLen;
    A_UINT8         ip6addr[16];
    A_UINT8         gateway[16];
    A_UINT32        command;
    IPV6_ROUTE_LIST_T routelist;

    do
    {
        if(argc < 3)
        {
            printf("incomplete params\n");
            error = A_ERROR;
            break;
        }

        if(strcmp(argv[2], "show") != 0)
        {
            if(argc < 6)
            {
                printf("incomplete params\n");
                error = A_ERROR;
                break;
            }
            
            error=Inet6Pton(argv[3],ip6addr);
            if(error!=0)
            {
                printf("Invalid IP address\n");
                error = A_ERROR;
                break;
            }

            prefixLen = atoi(argv[4]);
        }
        if(strcmp(argv[2], "add") == 0)
        {
            if(argc < 6)
            {
                printf("incomplete params\n");
                error = A_ERROR;
                break;
            }
            
            command = ROUTE_ADD;
            //ifIndex = atoi(argv[6]);

            error=Inet6Pton(argv[5],gateway);
            if(error!=0)
            {
                printf("Invalid Gateway address\n");
                error = A_ERROR;
                break;
            }
        }
        else if(strcmp(argv[2], "del") == 0)
        {
            if(argc < 6)
            {
                printf("incomplete params\n");
                error = A_ERROR;
                break;
            }
            
            command = ROUTE_DEL;
            ifIndex = atoi(argv[5]);
        }
        else if(strcmp(argv[2], "show") == 0)
        {
            command = ROUTE_SHOW;
        }
        else
        {
            printf("Unknown Command \"%s\"\n", argv[2]);
            error = A_ERROR;
            break;
        }

        if(A_OK == qcom_ip6_route(device_id, command, ip6addr, &prefixLen, gateway, &ifIndex, &routelist))
        {
            if(command == ROUTE_SHOW)
            {
                int i = 0;
                char ip_str[48];
                {
                    printf("\n..IPaddr.......prefixLen.........nexthop...iface..\n");
                    for(i = 0; i < routelist.rtcount; i++)
                    {
                        printf("%s  %d ", inet6_ntoa((char *)(&routelist.route[i].address), (char *)ip_str),
                               routelist.route[i].prefixlen);
                        printf("%s  %d\n", inet6_ntoa((char *)(&routelist.route[i].nexthop), (char *)ip_str),
                               routelist.route[i].ifindex);
                    }
                }
            }
        }
        else
        {
            printf("Route %s failed\n", (command == 0)?"addition":((command ==1)?"deletion":"show"));
        }
    }while(0);

    if (error == A_ERROR)
    {
        printf ("USAGE: wmiconfig --ipv6_route add <address> <prefixLen> <gw>\n");
        printf ("       wmiconfig --ipv6_route del <address> <prefixLen> <ifIndex>\n");
        printf ("       wmiconfig --ipv6_route show\n");
    }
    return error;
} /* Endbody */
A_INT32 swat_set_ipv6_status(A_UINT8 device_id, char* status){
    int flag;
    if (!strcmp(status, "enable"))
     flag = 1;
    else if (!strcmp(status, "disable"))
     flag = 0;
    else {
    SWAT_PTF("input paramenter should be enable or disable!\n");
     return A_ERROR;
    }
    qcom_set_ipv6_status(device_id,flag);
    return A_OK;
}
#ifdef SWAT_SSL
#include "qcom_ssl.h"
#include "qcom/socket_api.h"
#include "qcom/select_api.h"

SSL_INST_LIST *ssl_inst_list_head = NULL;
A_UINT32 ssl_inst_index_mask = 0;

A_UINT8 *ssl_cert_data_buf;
A_UINT16 ssl_cert_data_buf_len;

/* Number of SSL context created by application is tracked using session index */
static A_UINT32 swat_alloc_ssl_index(void){
  A_UINT32 i =0;
  if(ssl_inst_index_mask == 0xFFFFFFFF)
     return 0;

  for(i=0;i<32;i++){
     if(!((ssl_inst_index_mask) & (0x1 << i))){
      ssl_inst_index_mask |= (0x1 << i);
      return (i+1);
     }
  }
  return 0;
}

static void swat_free_ssl_index(A_UINT32 index){
   if(!index)
     return;
  ssl_inst_index_mask &= ~(0x1 << (index - 1));
  return;
}

/* Allocate SSL session instance 
 * each session allocate is maintained in a list
 * with index as identifier
 */
SSL_INST *swat_alloc_ssl_inst(A_UINT32 *index)
{
   SSL_INST_LIST *ssl_inst_ptr;
   ssl_inst_ptr = (SSL_INST_LIST *)mem_alloc(sizeof(SSL_INST_LIST));
   if(NULL == ssl_inst_ptr)
     return NULL;
   /* Init ssl session instance */
   memset(ssl_inst_ptr, 0, sizeof(SSL_INST_LIST));
   ssl_inst_ptr->next = NULL;

   /* Allocate index: max 32*/
   if(0 == (ssl_inst_ptr->index = swat_alloc_ssl_index())){
      mem_free(ssl_inst_ptr);
      return NULL;
   }
   *index = ssl_inst_ptr->index;
   /* Add to head of list always*/
   if(NULL == ssl_inst_list_head){
      ssl_inst_list_head = ssl_inst_ptr;
   }else{
      ssl_inst_ptr->next = ssl_inst_list_head;
      ssl_inst_list_head = ssl_inst_ptr;
   }
   return &(ssl_inst_ptr->ssl_inst);
}

/* Function to fetch SSL session instance using index */
SSL_INST *swat_find_ssl_inst(A_UINT32 index)
{
   SSL_INST_LIST *ssl_inst_ptr;
   ssl_inst_ptr = ssl_inst_list_head;
   while(NULL != ssl_inst_ptr){
      if(ssl_inst_ptr->index == index){
        break;
      }
      ssl_inst_ptr = ssl_inst_ptr->next;
   }
   if(NULL == ssl_inst_ptr)
      return NULL;

   return &(ssl_inst_ptr->ssl_inst);
}

/* Free ssl session instace having index */
void swat_free_ssl_inst(A_UINT32 index)
{
   SSL_INST_LIST *ssl_inst_ptr = ssl_inst_list_head;
   SSL_INST_LIST *ssl_inst_prev = NULL;

   while(NULL != ssl_inst_ptr){
     if(ssl_inst_ptr->index == index){
        /* instance found at head of the list*/
        if(ssl_inst_prev == NULL){
           ssl_inst_list_head = ssl_inst_ptr->next;
        }else{
           /* Link to list if instance found in middle */
           ssl_inst_prev->next = ssl_inst_ptr->next;
        }
        break;
    }else{
        ssl_inst_prev = ssl_inst_ptr;
        ssl_inst_ptr = ssl_inst_ptr->next;
    }
  }
  /* free the instance */
  if(NULL != ssl_inst_ptr){
     swat_free_ssl_index(index);
     mem_free(ssl_inst_ptr);
  }
  return;
}

A_INT32 ssl_get_cert_handler(A_INT32 argc, char* argv[])
{
    A_INT32 res = A_ERROR;
//    DNC_CFG_CMD dnsCfg;
//    DNC_RESP_INFO dnsRespInfo;
    struct sockaddr_in hostAddr;
    A_UINT32 socketHandle = 0, ipAddress = 0;
    int reqLen;
    CERT_HEADER_T *req;
    CERT_HEADER_T *header;
    A_UINT8 *buf;
    int certNameLen, numRead = 0, index, port = 1443;
    char *host, *certName, *flashName = NULL;
    q_fd_set sockSet,master;
    struct timeval tmo;

    // Free certificate buffer if allocated
    if (ssl_cert_data_buf)
    {
      mem_free(ssl_cert_data_buf);
    }
    ssl_cert_data_buf_len = 0;


    // Parse the arguments
    if(argc < 3)
    {
        if (argc > 1)
        {
          printf("Incomplete parameters\n");
        }
        printf("Usage: %s <name> <host> -p <port -s <fname>\n", argv[0]);
        printf("  <name>  = Name of the certificate or CA list file to retrieve\n");
        printf("  <host>  = Host name or IP address of certificate server\n");
        printf("  <port>  = Optional TCP port number\n");
        printf("  <fname> = Optional file name used if certificate is stored in FLASH\n");
        return A_ERROR;
    }
    certName = argv[1];
    host = argv[2];
    for(index = 3; index < argc ; index++)
    {
        if(argv[index][0] == '-')
        {
            switch(argv[index][1])
            {
            case 'p':
                index++;
                port = atoi(argv[index]);
                break;
            case 's':
                index++;
                flashName = argv[index];
                if (strlen(flashName) >= SSL_FILENAME_LEN)
                {
                    printf("ERROR: the length of certificate file name to be stored should less than %d\n", SSL_FILENAME_LEN);
                    return A_ERROR;
                }
                break;
            default:
                printf("Unknown option: %s\n", argv[index]);
                return A_ERROR;
            }
        }
    }

    do
    {
#if 0
        // resolve the IP address of the certificate server
        if (0 == ath_inet_aton(host, &dnsRespInfo.ipaddrs_list[0]))
        {
            if (strlen(host) >= sizeof(dnsCfg.ahostname))
            {
                printf("ERROR: host name too long\n");
                break;
            }
            strcpy((char*)dnsCfg.ahostname, host);
            dnsCfg.domain = ATH_AF_INET;
            dnsCfg.mode =  RESOLVEHOSTNAME;
            if (A_OK != custom_ip_resolve_hostname(handle, &dnsCfg, &dnsRespInfo))
            {
                printf("ERROR: Unable to resolve server name\r\n");
                break;
            }
        }
#endif
        ipAddress = _inet_addr(host);
        // Create socket
        if((socketHandle = swat_socket(AF_INET, SOCK_STREAM, 0)) == A_ERROR)
        {
            printf("ERROR: Unable to create socket\n");
            break;
        }

        // Connect to certificate server
        memset(&hostAddr, 0, sizeof(hostAddr));
        hostAddr.sin_addr.s_addr = htonl(ipAddress);//dnsRespInfo.ipaddrs_list[0];
        hostAddr.sin_port = htons(port);
        hostAddr.sin_family = AF_INET;
        res = swat_connect( socketHandle,(struct sockaddr *)(&hostAddr), sizeof(hostAddr));
        if(res != A_OK)
        {
            printf("ERROR: Connection failed (%d).\n", res);
            break;
        }

        // Build and send request
        certNameLen = strlen(certName);
        reqLen = CERT_HEADER_LEN + certNameLen;
        req = (CERT_HEADER_T*) mem_alloc(reqLen);

        if (req == NULL)
        {
            printf("ERROR: Out of memory.\n");
            break;
        }
        req->id[0] = 'C';
        req->id[1] = 'R';
        req->id[2] = 'T';
        req->id[3] = '0';
        req->length = HTONL(certNameLen);
        memcpy(&req->data[0], certName, certNameLen);
        res = swat_send(socketHandle, (char*)req, reqLen, 0);
        mem_free(req);
        if (res < 0 )
        {
            printf("ERROR: send error = %d\n", res);
            break;
        }
    } while (0);

    // Read the response
    swat_fd_zero(&master);
    swat_fd_set(socketHandle, &master);
    tmo.tv_sec	= 10;
    tmo.tv_usec = 0;
    if((buf = mem_alloc(1500)) == NULL){
        SWAT_PTF("Buf MALLOC ERR\n");
        return A_ERROR;
    }
    do
    {
        sockSet = master;
        res = swat_select(socketHandle + 1, &sockSet, NULL, NULL, &tmo);
         if (0 != res) {
            if (swat_fd_isset(socketHandle, &sockSet)) {
            res = swat_recv(socketHandle, (char*)buf, 1500, 0);
            printf("RX: %d\n", res);
            if (res > 0)
            {
                if (ssl_cert_data_buf_len == 0)
                {
                    if (buf[0] != 'C' || buf[1] != 'R' || buf[2] != 'T')
                    {
                        printf("ERROR: Bad MAGIC received in header\n");
                        break;
                    }
                    header = (CERT_HEADER_T*)buf;
                    header->length =  NTOHL(header->length);
                    if (header->length == 0)
                    {
                        break;
                    }
                    ssl_cert_data_buf = mem_alloc(header->length);
                    if(ssl_cert_data_buf == NULL)
                    {
                        printf("ERROR: Out of memory error\n");
                        res = A_ERROR;
                        break;
                    }
                    ssl_cert_data_buf_len = header->length;
                    res -= 8;
                    memcpy(ssl_cert_data_buf, header->data, res);
                    numRead = res;
                }
                else
                {
                    if (res + numRead <= ssl_cert_data_buf_len)
                    {
                        memcpy(&ssl_cert_data_buf[numRead], buf, res);
                        numRead += res;
                        res = ssl_cert_data_buf_len;
                    }
                    else
                    {
                        printf("ERROR: read failed\n");
                        res = A_ERROR;
                        break;
                    }
                }
            }
            else
            {
                printf("ERROR: no response\n");
                res = A_ERROR;
                break;
            }
            }
        }
    } while (numRead < ssl_cert_data_buf_len);
    mem_free(buf);
    if (socketHandle)
    {
        qcom_socket_close( socketHandle);
    }
    if (res == ssl_cert_data_buf_len && res != 0)
    {
        printf("Received %d bytes from %s:%d\n", ssl_cert_data_buf_len, host, port);

        if (flashName != NULL)
        {
            // store certificate in FLASH
            if (A_OK == qcom_SSL_storeCert(flashName, ssl_cert_data_buf, ssl_cert_data_buf_len))
            {
                printf("'%s' is stored in FLASH\n", flashName);
            }
            else
            {
                printf("ERROR: failed to store in %s\n", flashName);
                res = A_ERROR;
            }
        }
    }
/*
    if (ssl_cert_data_buf)
    {
      mem_free(ssl_cert_data_buf);
    }
    ssl_cert_data_buf_len = 0;
*/
    return res;
}


A_INT32 ssl_start(A_INT32 argc, char *argv[])
{
    SSL_INST *ssl;
    SSL_ROLE_T role;
    A_UINT32 ssl_inst_index;

    if(argc < 3)
    {
       printf("ERROR: Incomplete params\n");
       return A_ERROR;
    }

    if (0 == strcmp(argv[2], "server"))
    {
        role = SSL_SERVER;
    }
    else if (0 == strcmp(argv[2], "client"))
    {
        role = SSL_CLIENT;
    }
    else
    {
        printf("ERROR: Invalid parameter: %s\n", argv[2]);
        return A_ERROR;
    }

    if(NULL == (ssl = swat_alloc_ssl_inst(&ssl_inst_index))){
        printf("ERROR: Out of Memory\n");
        return A_ERROR;
    }
    // Create SSL context
    memset(ssl, 0, sizeof(SSL_INST));
    ssl->role = role;
    ssl->sslCtx = qcom_SSL_ctx_new(role, SSL_INBUF_SIZE, SSL_OUTBUF_SIZE, 0);
    if (ssl->sslCtx == NULL)
    {
        printf("ERROR: Unable to create SSL context\n");
        return A_ERROR;
    }

    // Reset config struct
    memset(&ssl->config, 0, sizeof(SSL_CONFIG));

    // Done
    printf("SSL %s started : Index %d\n", argv[2], ssl_inst_index);
    return A_OK;
}

A_INT32 ssl_stop(A_INT32 argc, char *argv[])
{
    SSL_INST *ssl;
    A_INT32 res = A_OK;
    SSL_ROLE_T role;
    A_UINT32 ssl_inst_index = 0;

    if(argc < 4)
    {
       printf("ERROR: Incomplete params\n");
       return A_ERROR;
    }

    if (0 == strcmp(argv[2], "server"))
    {
        role = SSL_SERVER;
    }
    else if (0 == strcmp(argv[2], "client"))
    {
        role = SSL_CLIENT;
    }
    else
    {
        printf("ERROR: Invalid parameter: %s\n", argv[2]);
        return A_ERROR;
    }
    ssl_inst_index = swat_atoi(argv[3]);
    if(NULL == (ssl =  swat_find_ssl_inst(ssl_inst_index))){
        return A_ERROR;
    }

    if (ssl->sslCtx == NULL || role != ssl->role)
    {
        printf("ERROR: SSL %s not started\n", argv[2]);
        return A_ERROR;
    }

    if (ssl->ssl)
    {
        if (ssl->role == SSL_CLIENT)
        {
            ssl->state = SSL_SHUTDOWN;
            printf("SSL %s stopping : Index %d\n", argv[2], ssl_inst_index);
            return res;
        }
        else
        {
            qcom_SSL_shutdown(ssl->ssl);
            ssl->ssl = NULL;
        }
    }

    if (ssl->sslCtx)
    {
        qcom_SSL_ctx_free(ssl->sslCtx);
        ssl->sslCtx = NULL;
    }
    swat_free_ssl_inst(ssl_inst_index);
    printf("SSL %s stopped: Index %d\n", argv[2], ssl_inst_index);
    return res;
}


A_INT32 ssl_config(A_INT32 argc, char *argv[])
{
    SSL_INST *ssl;
    A_INT32 res;
    int argn, cipher_count = 0;
    SSL_ROLE_T role;
    A_UINT32 ssl_inst_index = 0;

    if(argc < 4)
    {
       printf("ERROR: Incomplete params\n");
       return A_ERROR;
    }

    if (0 == strcmp(argv[2], "server"))
    {
        role = SSL_SERVER;
    }
    else if (0 == strcmp(argv[2], "client"))
    {
        role = SSL_CLIENT;
    }
    else
    {
        printf("ERROR: Invalid parameter: %s\n", argv[2]);
        return A_ERROR;
    }

    ssl_inst_index = swat_atoi(argv[3]);

    if(NULL == (ssl =  swat_find_ssl_inst(ssl_inst_index))){
        return A_ERROR;
    }

    if (ssl->sslCtx == NULL || role != ssl->role)
    {
        printf("ERROR: SSL %s not started\n", argv[2]);
        return A_ERROR;
    }

    argn = 4;
    ssl->config_set = 0;
    
    while (argn < argc)
    {
        if (cipher_count>0){
            goto ADD_CIPHER_SUITE;
        }
        
        if (argn == argc-1)
        {
            printf("ERROR: Incomplete params\n");
            return A_ERROR;
        }
                
        if (0 == strcmp("protocol", argv[argn]))
        {
            // Setting of protocol option is supported for SSL client only
            if (strcmp(argv[2], "client"))
            {
                printf("ERROR: Protocol option is not supported for %s\n", argv[2]);
                return A_ERROR;
            }
            argn++;
            if (0 == strcmp("SSL3", argv[argn]))
            {
                ssl->config.protocol = SSL_PROTOCOL_SSL_3_0;
            }
            else if (0 == strcmp("TLS1.0", argv[argn]))
            {
                ssl->config.protocol = SSL_PROTOCOL_TLS_1_0;
            }
            else if (0 == strcmp("TLS1.1", argv[argn]))
            {
                ssl->config.protocol = SSL_PROTOCOL_TLS_1_1;
            }
            else if (0 == strcmp("TLS1.2", argv[argn]))
            {
                ssl->config.protocol = SSL_PROTOCOL_TLS_1_2;
            }
            else if (0 == strcmp("DTLS1.0", argv[argn]))
            {
                ssl->config.protocol = SSL_PROTOCOL_DTLS_1_0;
            }
            else if (0 == strcmp("DTLS1.2", argv[argn]))
            {
                ssl->config.protocol = SSL_PROTOCOL_DTLS_1_2;
            } 
            else
            {
                printf("ERROR: Invalid protocol: %s\n", argv[argn]);
                return A_ERROR;
            }
        }

        if (0 == strcmp("time", argv[argn]))
        {
            argn++;
            if (0 == strcmp("1", argv[argn]))
            {
                ssl->config.verify.timeValidity = 1;
            }
            else if (0 == strcmp("0", argv[argn]))
            {
                ssl->config.verify.timeValidity = 0;
            }
            else
            {
                printf("ERROR: Invalid option: %s\n", argv[argn]);
                return A_ERROR;
            }
        }

        if (0 == strcmp("alert", argv[argn]))
        {
            argn++;
            if (0 == strcmp("1", argv[argn]))
            {
                ssl->config.verify.sendAlert = 1;
            }
            else if (0 == strcmp("0", argv[argn]))
            {
                ssl->config.verify.sendAlert = 0;
            }
            else
            {
                printf("ERROR: Invalid option: %s\n", argv[argn]);
                return A_ERROR;
            }
        }

        if (0 == strcmp("domain", argv[argn]))
        {
            argn++;
            if (0 == strcmp("0", argv[argn]))
            {
                ssl->config.verify.domain = 0;
                ssl->config.matchName[0] = '\0';
            }
            else
            {
                ssl->config.verify.domain = 1;
                if (strlen(argv[argn]) >= sizeof(ssl->config.matchName))
                {
                    printf("ERROR: %s too long (max %d chars)\n", argv[argn], sizeof(ssl->config.matchName));
                    return A_ERROR;
                }
                strcpy(ssl->config.matchName, argv[argn]);
            }
        }

        if (0 == strcmp("sni", argv[argn]))
        {
            argn++;
            if (0 == strcmp("0", argv[argn]))
            {
                ssl->config.sni_name[0] = '\0';
            }
            else
            {
                if((ssl->role != SSL_CLIENT)
                    ||( (ssl->config.protocol != SSL_PROTOCOL_TLS_1_0)
                    &&(ssl->config.protocol != SSL_PROTOCOL_TLS_1_1)
                    &&(ssl->config.protocol != SSL_PROTOCOL_TLS_1_2)
                    &&(ssl->config.protocol != SSL_PROTOCOL_SSL_3_0))) {
    			    printf("Warning: ignoring SNI option, this only valid for SSLv3/TLS client\n");
    			} else {
    			   if (strlen(argv[argn]) >= TLS_SERVER_NAME_MAX_LEN)
                    {
                        printf("ERROR: %s too long (max %d chars)\n", argv[argn], TLS_SERVER_NAME_MAX_LEN - 1);
                        return A_ERROR;
                    }
    				strcpy(ssl->config.sni_name, argv[argn]);
    			}
            }
        
        }

ADD_CIPHER_SUITE:
        if ((0 == strcmp("cipher", argv[argn])) || (cipher_count>0))
        {
            if (0 == strcmp("cipher", argv[argn])){
                argn++;
            }
                        
            if (cipher_count == 0)
            {
                memset(ssl->config.cipher, 0, sizeof(ssl->config.cipher));
            }
            if (cipher_count == SSL_CIPHERSUITE_LIST_DEPTH)
            {
                printf("ERROR: Too many cipher options %s (max %d)\n", argv[argn], SSL_CIPHERSUITE_LIST_DEPTH);
                return A_ERROR;
            }
            if ((0 == strcmp("TLS_RSA_WITH_AES_256_GCM_SHA384", argv[argn])) ||(atoi(argv[argn]) == TLS_RSA_WITH_AES_256_GCM_SHA384))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_256_GCM_SHA384;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_256_CBC_SHA256", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_256_CBC_SHA256))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_256_CBC_SHA256;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_256_CBC_SHA", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_256_CBC_SHA))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_256_CBC_SHA;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_128_GCM_SHA256", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_128_GCM_SHA256))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_128_GCM_SHA256;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_128_CBC_SHA256", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_128_CBC_SHA256))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_128_CBC_SHA256;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_128_CBC_SHA", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_128_CBC_SHA))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_128_CBC_SHA;
            }
            else if ((0 == strcmp("TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256", argv[argn])) || (atoi(argv[argn]) == TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256))
            {
                ssl->config.cipher[cipher_count] = TLS_ECDHE_RSA_WITH_CHACHA20_POLY1305_SHA256;
            }
            else if ((0 == strcmp("SHARKSSL_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256", argv[argn])) || (atoi(argv[argn]) == TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256))
            {
                ssl->config.cipher[cipher_count] = TLS_ECDHE_ECDSA_WITH_CHACHA20_POLY1305_SHA256;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_128_CCM", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_128_CCM))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_128_CCM;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_128_CCM_8", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_128_CCM_8))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_128_CCM_8;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_256_CCM", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_256_CCM))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_256_CCM;
            }
            else if ((0 == strcmp("TLS_RSA_WITH_AES_256_CCM_8", argv[argn])) || (atoi(argv[argn]) == TLS_RSA_WITH_AES_256_CCM_8))
            {
                ssl->config.cipher[cipher_count] = TLS_RSA_WITH_AES_256_CCM_8;
            }
	        else if ((0 == strcmp("TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256", argv[argn])) 
		              || (atoi(argv[argn]) == TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256))
            {
                ssl->config.cipher[cipher_count] = TLS_ECDHE_ECDSA_WITH_AES_128_GCM_SHA256;
            }
            else if ((0 == strcmp("TLS_DHE_RSA_WITH_AES_128_CBC_SHA", argv[argn])) || (atoi(argv[argn]) == TLS_DHE_RSA_WITH_AES_128_CBC_SHA))
            {
                ssl->config.cipher[cipher_count] = TLS_DHE_RSA_WITH_AES_128_CBC_SHA;
            }
			else if ((0 == strcmp("TLS_DHE_RSA_WITH_AES_256_CBC_SHA", argv[argn])) || (atoi(argv[argn]) == TLS_DHE_RSA_WITH_AES_256_CBC_SHA))
            {
                ssl->config.cipher[cipher_count] = TLS_DHE_RSA_WITH_AES_256_CBC_SHA;
            }
			else if ((0 == strcmp("TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8", argv[argn])) || (atoi(argv[argn]) == TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8))
            {
                ssl->config.cipher[cipher_count] = TLS_ECDHE_ECDSA_WITH_AES_128_CCM_8;
            }
            else 
            {
                printf("ERROR: Invalid protocol: %s\n", argv[argn]);
                return A_ERROR;
            }
            cipher_count++;
        }

        argn++;
    }

	ssl->config_set = 1;

    if (ssl->ssl != NULL)
    {

        /*
		        We should not create an SSL connection object.
        		We should create it after TCP connection, it's already done in tcp_bench_rx and tcp_bench_tx
        		If create too early, it will cause some other configure issue, such as we can't set CA list after the connection object created.
        	*/
       	/*
		        ssl->ssl = qcom_SSL_new(ssl->sslCtx);
		        if (ssl->ssl == NULL)
		        {
		            printf("ERROR: SSL configure failed (Unable to create SSL context)\n");
		            return A_ERROR;;
		        }
	 	*/
	    // configure the SSL connection
	    res = qcom_SSL_configure(ssl->ssl, &ssl->config);
	    if (res < A_OK)
	    {
	        printf("ERROR: SSL configure failed (%d)\n", res);
	        return A_ERROR;
	    }
	    printf("SSL %s configuration changed\n", argv[2]);
    }
    else
    {
	 res = qcom_SSL_context_configure(ssl->sslCtx, &ssl->config);
	 if (res < A_OK)
	 {
	      printf("ERROR: SSL context configure failed (%d)\n", res);
	      return A_ERROR;
	 }
	 printf("SSL %s context configuration changed\n", argv[2]);
    }
    return A_OK;
}


#include "cert-kingfisher.inc"
extern const A_UINT8 sharkSslRSACertKingfisher[1016];
A_INT32 ssl_add_cert(A_INT32 argc,char *argv[])
{
    SSL_INST *ssl, *refSsl;
    SSL_CERT_TYPE_T type;
    char *name = NULL;
    SSL_ROLE_T role;
    A_UINT32 ssl_inst_index;
    SSL_CTX *refSslCtx = NULL;

    if(argc < 5)
    {
       printf("Incomplete params\n");
       return A_ERROR;
    }

    if (0 == strcmp(argv[2], "server"))
    {
        role = SSL_SERVER;
    }
    else if (0 == strcmp(argv[2], "client"))
    {
        role = SSL_CLIENT;
    }
    else
    {
        printf("ERROR: Invalid parameter: %s\n", argv[2]);
        return A_ERROR;
    }

    ssl_inst_index = swat_atoi(argv[3]);

    if(NULL == (ssl = swat_find_ssl_inst(ssl_inst_index))){
        printf("ERROR: Invalid parameter\n");
        return A_ERROR;
    }

    if (ssl->sslCtx == NULL || role != ssl->role)
    {
        printf("ERROR: SSL %s not started\n", argv[2]);
        return A_ERROR;
    }


    if (0 == strcmp("certificate", argv[4]))
    {
        type = SSL_CERTIFICATE;
    }
    else if (0 == strcmp("calist", argv[4]))
    {
        type = SSL_CA_LIST;
    }
    else
    {
        printf("ERROR: Invalid parameter: %s\n", argv[4]);
        return A_ERROR;
    }

    if (argc > 5)
    {
        if ((strlen(argv[5]) == 1) && ((swat_atoi(argv[5])>0)&& (swat_atoi(argv[5])<9))){  //the index of ssl context which share its certificate or calist.
            if((NULL == (refSsl = swat_find_ssl_inst(swat_atoi(argv[5])))) || (refSsl->role != ssl->role)){
                printf("ERROR: Invalid parameter\n");
                return A_ERROR;
            }
            refSslCtx = refSsl->sslCtx;
            if (qcom_SSL_reuseCert(ssl->sslCtx, refSsl->sslCtx) < A_OK){
                printf("ERROR: Unable to reuse cert of ssl ctx %d\n" , swat_atoi(argv[5]));
                return A_ERROR;
            }
            printf("Reuse cert of ssl ctx %d sucess\n", swat_atoi(argv[5]));
            return A_OK;
        } else {
            name = argv[5];
        }
    }

    // Load/add certificate
    if (name != NULL)
    {
        if (qcom_SSL_loadCert(ssl->sslCtx, type, name) < A_OK)
        {
            printf("ERROR: Unable to load %s from FLASH\n" , name);
            return A_ERROR;
        }
        printf("%s loaded from FLASH\n", name);
    }
    else
    {
        A_UINT8 *cert = ssl_cert_data_buf;
        A_UINT16 cert_len = ssl_cert_data_buf_len;
        if (type == SSL_CERTIFICATE)
        {
            if (cert_len == 0)
            {
                // Load the default certificate
                printf("Note: using the default certificate\n");
                cert = (A_UINT8*)sharkSslRSACertKingfisher;
                cert_len = sizeof(sharkSslRSACertKingfisher);
            }
            if (qcom_SSL_addCert(ssl->sslCtx, cert, cert_len) < A_OK)
            {
                printf("ERROR: Unable to add certificate\n");
                return A_OK;
            }
            printf("Certificate added to SSL server\n");
        }
        else
        {
            if (cert_len == 0)
            {
                // Load the default CA list
                printf("Note: using the default CA list\n");
                //cert = (A_UINT8*)ssl_default_calist;
                //cert_len = ssl_default_calist_len;
            }
            if (qcom_SSL_setCaList(ssl->sslCtx, cert, cert_len)< A_OK)
            {
                printf("ERROR: Unable to set CA list\n");
                return A_ERROR;
            }
            printf("CA list added to SSL client\n");
        }
    }
    return A_OK;
}

A_INT32 ssl_store_cert(A_INT32 argc,char *argv[])
{
    char *name;

    if(argc < 3)
    {
       printf("Incomplete params\n");
       return A_ERROR;
    }
    name = argv[2];

    if (ssl_cert_data_buf_len == 0)
    {
        printf("ERROR: no certificate data.\nHint: Use the wmiconfig --ssl_get_cert to read a certificate from a certificate server.\n");
        return A_ERROR;
    }
    if (strlen(name) >= SSL_FILENAME_LEN)
    {
        printf("ERROR: failed to store %s, the length of certificate file name should less than %d\n", name, SSL_FILENAME_LEN);
        return A_ERROR;
    }
    if (A_OK == qcom_SSL_storeCert(name, ssl_cert_data_buf, ssl_cert_data_buf_len))
    {
        printf("%s is stored in FLASH\n", name);
        return A_OK;
    }
    else
    {
        printf("ERROR: failed to store %s\n", name);
        return A_ERROR;
    }
}

A_INT32 ssl_delete_cert(A_INT32 argc,char *argv[])
{
    char *name;

    if(argc < 3)
    {
        ssl_cert_data_buf_len = 0;
        printf("Deleted the certificate data stored in RAM.\n");
        return A_OK;
    }
    name = argv[2];

    if (A_OK == qcom_SSL_storeCert(name, NULL, 0))
    {
        printf("Deleted %s from FLASH\n", name);
        return A_OK;
    }
    else
    {
        printf("ERROR: failed to delete %s\n", name);
        return A_ERROR;
    }
}


A_INT32 ssl_list_cert(A_INT32 argc,char *argv[])
{
    SSL_FILE_NAME_LIST *fileNames;
    A_INT32 i, numFiles, namesLen = sizeof(SSL_FILE_NAME_LIST); // 10 files

    fileNames = mem_alloc(namesLen);
    if (fileNames == NULL)
    {
        printf("ALLOC ERROR\n");
        return A_ERROR;
    }

    numFiles = qcom_SSL_listCert(fileNames);
    if (numFiles < 0)
    {
        printf("ERROR: failed to list files (%d)\n", numFiles);
        mem_free(fileNames);
        return A_ERROR;
    }

    printf("%d %s stored in FLASH\n", numFiles, numFiles == 1 ? "file" : "files");
    for (i=0; i<numFiles; i++)
    {
        char str[21];
        if (fileNames->name[i][0])
        {
            strncpy(str, (char*)fileNames->name[i], sizeof(str)-1);
            str[sizeof(str)-1] = '\0';
            printf("%s\n", str);
        }
    }
    mem_free(fileNames);
    return A_OK;
}

A_INT32 ssl_set_cert_check_tm(A_INT32 argc,char *argv[])
{
    tmx_t c_tm;

    if(argc < 8)
    {
        goto P_ERROR;
    }
    
    c_tm.year = atoi(argv[2]);

    c_tm.mon = atoi(argv[3]);
    if ((c_tm.mon > 12) || (c_tm.mon < 0)) {
       goto P_ERROR; 
    }
    
    c_tm.mday= atoi(argv[4]);
    if ((c_tm.mday > 31) || (c_tm.mday < 0)) {
       goto P_ERROR; 
    }
    
    c_tm.hour= atoi(argv[5]);
    if ((c_tm.hour > 24) || (c_tm.hour < 0)) {
       goto P_ERROR; 
    }
    
    c_tm.min= atoi(argv[6]);
    if ((c_tm.min > 59) || (c_tm.min < 0)) {
       goto P_ERROR; 
    }
    
    c_tm.sec= atoi(argv[7]);
    if ((c_tm.sec > 59) || (c_tm.sec < 0)) {
       goto P_ERROR; 
    }

    SWAT_PTF("set ssl tm = %d-%02d-%02d %02d:%02d:%02u\n", c_tm.year, c_tm.mon, c_tm.mday, c_tm.hour, c_tm.min, c_tm.sec);

    return qcom_SSL_set_tm(&c_tm);
        
P_ERROR:
    SWAT_PTF("Incomplete params\n");
    SWAT_PTF("Usage: wmiconfig --ssl_set_cert_check_tm <year> <month> <day> <hour> <minute> <seconds>\n");
    return A_ERROR;
}

volatile A_UINT32 swat_ssl_test_threads_state = 0;
const A_CHAR ssl_test_thread_name[] = "SWAT SSL APIs Test Thread";
A_UINT32 ssl_test_server_ip = 0;
A_UINT32 ssl_test_local_ip = 0;
int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
void swat_ssl_test_thread(A_UINT32 index)
{
	A_UINT32 count = 0;
    SSL_CONFIG config;
    A_INT32 ret = -1;
    struct sockaddr_in remoteAddr;
    struct sockaddr_in localAddr;
	char pDataBuffer[400];
	char *msg = NULL;
	
	SWAT_PTF("%s %d start\n", ssl_test_thread_name, index);

	{
		int i;
		for (i = 0; i < 400; i++)
		{
			pDataBuffer[i] = (i % 10) + '0';
		}
	}

	// SSL CTX config
	memset(&config, 0, sizeof(config));
	config.protocol = SSL_PROTOCOL_TLS_1_2;

	// Local address
	memset(&localAddr, 0, sizeof(struct sockaddr_in));
    localAddr.sin_addr.s_addr = htonl(ssl_test_local_ip);
    localAddr.sin_family      = AF_INET;

	// remote address
    swat_mem_set(&remoteAddr, 0, sizeof(struct sockaddr_in));
    remoteAddr.sin_addr.s_addr = htonl(ssl_test_server_ip);
    remoteAddr.sin_port = htons(7999 + index);
    remoteAddr.sin_family = AF_INET;
	
	while (swat_ssl_test_threads_state)
	{
		SSL_CTX* sslCtx = NULL;
		SSL * ssl = NULL;
		A_INT32 local_sock = -1;
		
		count++;
		
		if ((sslCtx = qcom_SSL_ctx_new(SSL_CLIENT, SSL_OUTBUF_SIZE, SSL_OUTBUF_SIZE, 0)) == NULL)
		{
			msg = "ssl ctx";
			goto finished;
		}
		
		if (qcom_SSL_context_configure(sslCtx, &config) < A_OK)
		{
			msg = "ssl ctx config";
			goto finished;
		}
			
		ssl = qcom_SSL_new(sslCtx);
		if (!ssl)
		{
			msg = "ssl";
			goto finished;
		}
			
		local_sock = qcom_socket(AF_INET, SOCK_STREAM, 0);

		if (local_sock == -1)
		{
			msg = "socket";
			tx_thread_sleep(500);
			goto finished;
		}

		/*bind addr when in concurrency mode*/
        if (qcom_bind(local_sock, (struct sockaddr *)&localAddr, sizeof(struct sockaddr_in)) < 0)
		{
			msg = "bind";
	        goto finished;
        }

        if (qcom_connect(local_sock, (struct sockaddr *)&remoteAddr, sizeof (struct sockaddr_in)) < 0)
        {
			msg = "connect";
	        goto finished;
        }
		
		// Add socket handle to SSL connection
        if (qcom_SSL_set_fd(ssl, local_sock) < 0)
        {
			msg = "ssl set fd";
	        goto finished;
        }

		if (qcom_SSL_connect(ssl) < 0)
		{
			msg = "ssl connect";
	        goto finished;
		}

		if (qcom_SSL_write(ssl, pDataBuffer, 400) != 400)
		{
			msg = "ssl write";
			goto finished;
		}
		
		ret = 0;

finished:
		
		if (ssl)
		{
			qcom_SSL_shutdown(ssl);
			ssl = NULL;
		}

		if (sslCtx)
		{
			qcom_SSL_ctx_free(sslCtx);
			sslCtx = NULL;
		}

		if (local_sock != -1)
		{
		    qcom_socket_close(local_sock);
			local_sock = -1;
			tx_thread_sleep(200);
		}
		
		if (ret)
		{
			SWAT_PTF("%s %d %s FAIL\n", ssl_test_thread_name, index, msg);
			msg = NULL;
			// Failed: sleep
			tx_thread_sleep(500);
		}
		
		ret = -1;
	}

	SWAT_PTF("%s %d exit, run count %u\n", ssl_test_thread_name, index, count);
	
    /* Thread Delete */
    swat_task_delete();
}

void swat_ssl_test_threads_start(A_INT32 max_thread, A_UINT32 server_ip)
{
	A_INT32 i;

	if (swat_ssl_test_threads_state)
	{
		SWAT_PTF("Can't re-create %ss.\n", ssl_test_thread_name);
		return;
	}

	// Local address
	{
	    A_UINT32 submask;
	    A_UINT32 gateway;
	    qcom_ipconfig(currentDeviceId, IP_CONFIG_QUERY, &ssl_test_local_ip, &submask, &gateway);
	}

	if ((ssl_test_local_ip & 0xffffff00) != (server_ip & 0xffffff00))
	{
		SWAT_PTF("Error: %s local ip address %x, server ip address is %x.\n", ssl_test_thread_name, ssl_test_local_ip, server_ip);
		return;
	}
	
	swat_ssl_test_threads_state = 1;
	ssl_test_server_ip = server_ip;

	for (i = 1; i < max_thread + 1; i++)
	{
		if (qcom_task_start(swat_ssl_test_thread, i, 2048, 50) != 0)
		{
			SWAT_PTF("Create %s %d FAIL\n", ssl_test_thread_name, i);
		}
	}
}

void swat_ssl_test_threads_stop()
{
	swat_ssl_test_threads_state = 0;
}

#endif

A_INT32 swat_ota_upgrade(A_UINT8 device_id,A_INT32 argc, char* argv[])
{
    A_UINT32 addr;
    A_CHAR filename[100];
    A_UINT8 mode;
    A_UINT8 preserve_last;
    A_UINT8 protocol;
    A_UINT32 resp_code,length;

    if(argc < 7)
    {
        SWAT_PTF("Incomplete params\n");
        SWAT_PTF("Usage: wmiconfig --ota_upgrade <ip> <filename> <mode> <preseve_last> <protocol> \n");
        return A_ERROR;
    }

    addr=_inet_addr(argv[2]);

    if(strlen(argv[3]) >= 100)
    {
        SWAT_PTF("Error : Invalid Filename Length\n");
        return A_ERROR;
    }

    strcpy(filename,argv[3]);
    mode= atoi(argv[4]);

    if(mode > 2)
    {
        SWAT_PTF("Error : Invalid Mode [valid values are 0 or 1 ]\n\r");
        return A_ERROR;
    }

    preserve_last= atoi(argv[5]);

    if(preserve_last > 2)
    {
        SWAT_PTF("Error : Invalid preserve_last [valid values are 0 or 1 ]\n\r");
        return A_ERROR;
    }

    protocol= atoi(argv[6]);

    if(protocol > 2)
    {
        SWAT_PTF("Error : Invalid protocol [valid values are 0 or 1 ]\n\r");
        return A_ERROR;
    }

    qcom_ota_upgrade(device_id,addr,filename,mode,preserve_last,protocol,&resp_code,&length);

    if(resp_code!=A_OK){
       SWAT_PTF("OTA Download Failed, ERR=%d",resp_code);
       return A_ERROR;
   }else{
       SWAT_PTF("OTA Download Successful Image Size:%d\n ",length);
   }


}

#define OTA_READ_BUF_SIZE	256

A_INT32 swat_ota_read(A_INT32 argc,char *argv[]){
    A_ULONG offset;
    A_ULONG len, total_read = 0;
    A_UCHAR *data;
    A_INT32 error = 0;
    A_UINT32 ret_len=0;

    if(argc < 4)
    {
        SWAT_PTF(" Incomplete Params \n Usage: wmiconfig --ota_read <offset> <size>!\n");
        SWAT_PTF("e.g wmiconfig --ota_read 100 20 to read 20 bytes from and offset 100 w.r.t start of the image!\n");
        return A_ERROR;
    }

    data = mem_alloc(OTA_READ_BUF_SIZE);
    if (!data) {
    	SWAT_PTF("Insufficient memory\n");
    	return A_ERROR;
    }

    offset = atoi(argv[2]);
    len = atoi(argv[3]);

    SWAT_PTF("Partition data:\n");

    while (total_read < len) {
        A_UINT32 index;
        A_UINT32 bytes_to_read;

        if (len - total_read > OTA_READ_BUF_SIZE) {
        	bytes_to_read = OTA_READ_BUF_SIZE;
        } else {
        	bytes_to_read = len - total_read;
        }

    	error = qcom_read_ota_area(offset, bytes_to_read, data, &ret_len);

        if (error) {
        	SWAT_PTF("OTA Read Error: %d\n",error);
        	break;
        }

        for (index = 0; index < ret_len; index++) {
            if ((index & 15) == 0) {
                SWAT_PTF("\n");
            }
            SWAT_PTF("%02x ", data[index]);
        }

        total_read += ret_len;
        offset += ret_len;
    }

    mem_free(data);
    return error;
}

A_INT32 swat_ota_done(A_INT32 argc,char *argv[])
{
	A_UINT32 resp;
	A_UINT32 good_image;

	if (argc < 3) {
		SWAT_PTF(" Incomplete Params \n Usage: wmiconfig --ota_done <good_image>\n");
		return A_ERROR;
	}

	good_image = atoi(argv[2]);
	resp = qcom_ota_done(good_image ? 1 : 0);

	if (resp == 1) {
		SWAT_PTF("OTA Completed Successfully");
	} else {
		SWAT_PTF("OTA Failed Err:%d  \n", resp);
	}

	return resp;
}

#define SWAT_OTA_CUST

#ifdef SWAT_OTA_CUST
A_INT32 swat_ota_cust(A_INT32 argc,char *argv[])
{
	A_UINT32 len, i, offset;
	A_UINT32 cmd, rtn;
	if( argc < 3 )
	return A_ERROR;
	cmd = atoi(argv[2]);

	switch(cmd) {
		case 0:
			//ota session start
			{
			A_UINT32 flags, partition_indx;
			if (argc < 5) {
			SWAT_PTF("Invalid Params\nUsage: wmiconfig --ota_cust 0 <flags> <partition_index>\n");
			return A_ERROR;
			}
			flags = atoi(argv[3]);
			partition_indx = atoi(argv[4]);
			rtn = qcom_ota_session_start(flags, partition_indx);
			}
			break;		
		case 1:
			//ota partition get size
			rtn =  qcom_ota_partition_get_size();
			break;
		case 2:
			//ota partition erase
			rtn = qcom_ota_partition_erase();
			break;
		case 3:
			//ota partion parse image hdr
			{
				#define HDR_LEN 24
				A_UINT8 buf[HDR_LEN];
				A_UINT32 data0;				  
				if( (argc < 4) || (strlen(argv[3]) != HDR_LEN*2) ) {
					SWAT_PTF("Invalid Params\nUsage: wmiconfig --ota_cust 3 <xx...xx>\n");
					return A_ERROR;
				}			
				for(i = 0; i < HDR_LEN;i++ ) {
					A_SSCANF(&argv[3][i*2], "%2x", &data0);
					buf[i] = (A_UINT8) data0;
				}	 
				rtn = qcom_ota_parse_image_hdr(buf, &offset);
				SWAT_PTF("offset: %d\n", offset);
			}
			break;
		case 4:
			//ota partition write data
			{
				#define BUF_LEN 50
				A_UINT8 buf[BUF_LEN];
				A_UINT32 data0;				   

				if(argc < 6)
				{
					SWAT_PTF(" Incomplete Params\nUsage: wmiconfig --ota_cust 4 <offset> <len> <xx...xx>\n");
					return A_ERROR;
				}

				offset = atoi(argv[3]);
				len = atoi(argv[4]);
				if( len > BUF_LEN) {
					SWAT_PTF("data must be less than 50 bytes here \n");
					return A_ERROR;
				}
				if( strlen(argv[5])  != len*2) {
					SWAT_PTF("data format is not correct \n");
					return A_ERROR;
				}

				for(i = 0; i < len;i++ ) {
					A_SSCANF(&argv[5][i*2], "%2x", &data0);
					buf[i] = (A_UINT8) data0;
				}
				qcom_ota_partition_write_data(offset, buf, len, &rtn);
			}
			break;
		case 5:
			//ota partition verify checksum
			rtn = qcom_ota_partition_verify_checksum();
			if (rtn != QCOM_OTA_OK)
			{
				SWAT_PTF("Checksum verification failed:%d\n", rtn);
				return A_ERROR;
			}
			break;
		case 6:
			//ota read
			{
				A_ULONG offset;
				A_ULONG len, total_read = 0;
				A_UCHAR *data;
				A_INT32 error = A_OK;
				A_UINT32 ret_len=0;

				if(argc < 5)
				{
					SWAT_PTF(" Incomplete Params\nUsage: wmiconfig --ota_custom 6 <offset> <size>!\n");
					SWAT_PTF("e.g wmiconfig --ota_cust 6 100 20 to read 20 bytes from offset 100 w.r.t start of the image!\n");
					return A_ERROR;
				}

				data = mem_alloc(OTA_READ_BUF_SIZE);
				if (!data) {
					SWAT_PTF("Insufficient memory\n");
					return A_ERROR;
				}

				offset = atoi(argv[3]);
				len = atoi(argv[4]);

				SWAT_PTF("Partition data:\n");

				while (total_read < len) {
					A_UINT32 index;
					A_UINT32 bytes_to_read;

					if (len - total_read > OTA_READ_BUF_SIZE) {
						bytes_to_read = OTA_READ_BUF_SIZE;
					} else {
						bytes_to_read = len - total_read;
					}

					if ( -1 == (ret_len = qcom_ota_partition_read_data(offset, data, bytes_to_read)) ) {
						SWAT_PTF("OTA Read Error\n");
						error = A_ERROR;		
						break;
					}

					for (index = 0; index < ret_len; index++) {
						if ((index & 15) == 0) {
							SWAT_PTF("\n");
						}
						SWAT_PTF("%02x ", data[index]);
					}

					total_read += ret_len;
					offset += ret_len;
				}

				mem_free(data);
				return A_OK;	
			}
			break;

		case 7:
			//ota session end
			{
				A_UINT32 good_image;
				if (argc < 4) {
					SWAT_PTF("Invalid Params\nUsage: wmiconfig --ota_cust 7 <good_image>\n");
					return A_ERROR;
				}
				good_image = atoi(argv[3]);
				rtn = qcom_ota_session_end(good_image ? 1 : 0);
			}
			break;
		
	}

	SWAT_PTF("%d\n", rtn);
	return A_OK;
}

A_INT32 swat_ota_format(A_INT32 argc,char *argv[])
{
    A_STATUS rtn;
    A_UINT32 indx;

    if (argc < 3) {
         SWAT_PTF("Invalid Params\nUsage: wmiconfig --ota_format <partition_index>\n");
	  return A_ERROR;
    }

    indx = atoi(argv[2]); 
    rtn = qcom_ota_partition_format(indx);
    if (rtn == QCOM_OTA_OK) {
        SWAT_PTF("OTA Success");
    } else {
        SWAT_PTF("OTA Err:%d\n", rtn);
    }

    return A_OK;
}

A_INT32 swat_ota_ftp_upgrade(A_UINT8 device_id,A_INT32 argc, char *argv[])
{
    A_UINT32 ip_addr, flags, partition_index;
    A_UINT16 port;
    A_UINT32 resp_code;
    A_UINT32 size;    
 
    if(argc < 9)
    {
		printf("Incomplete params\n");
		printf("Usage: wmiconfig --ota_ftp <ip> <port> <user name> <password> <filename> <flags> <partition_index> \n");
		return A_ERROR;
    }
    
    ip_addr = _inet_addr(argv[2]);    
    port=atoi(argv[3]);
    flags=atoi(argv[7]);
    partition_index=atoi(argv[8]);
 
    resp_code = qcom_ota_ftp_upgrade(device_id,ip_addr,port,argv[4],argv[5],argv[6],flags, partition_index, &size);
    if (A_OK != resp_code)
    {
        printf("OTA Image Download Failed ERR:%d\n",resp_code);
    } else {
        printf("OTA Image Download Completed successfully,Image Size:%d\n",size);
    }
    return A_OK;
}
  
#endif //define SWAT_OTA_CUST

A_INT32 swat_tcp_conn_timeout(A_INT32 argc,char *argv[]){

    A_UINT32 timeout_val;
    if(argc < 2)
      {
          SWAT_PTF(" Incomplete Params. \n");
          SWAT_PTF("e.g wmiconfig --settcptimeout <val in sec.>\n");
          return A_ERROR;
     }

    timeout_val = atoi(argv[2]);
    qcom_tcp_conn_timeout(timeout_val);
    return A_OK;
}

void swat_wmiconfig_dhcps_success_cb(A_UINT8 *mac,A_UINT32 ipAddr){

    SWAT_PTF("DHCP Server issued Client MAC:%02x:%02x:%02x:%02x:%02x:%02x ,IP Addr:",mac[0],mac[1],mac[2],mac[3],mac[4],mac[5]);
    IPV4_ORG_PTF(ipAddr);
    SWAT_PTF("\n");

}

A_INT32 swat_wmiconfig_dhcps_cb_enable(A_UINT8 device_id,char * enable){

    if(!strcmp(enable, "start"))
    {
       qcom_dhcps_register_cb(device_id,swat_wmiconfig_dhcps_success_cb); 
    }
    else if(!strcmp(enable, "stop"))
    {
        qcom_dhcps_register_cb(device_id,NULL); 
    }
    else
     {
      SWAT_PTF("input parameter should be start or stop!\n");
      return A_ERROR;
     }

     return A_OK;
}

void swat_wmiconfig_dns_clear(void)
{
    qcom_dnc_clear(currentDeviceId);
}

void swat_wmiconfig_dhcpc_success_cb(A_UINT32 ipAddr,A_UINT32 mask,A_UINT32 gw){

    SWAT_PTF("IP Addr:");
    IPV4_ORG_PTF(ipAddr);
    SWAT_PTF("\nMask :");
    IPV4_ORG_PTF(mask);
    SWAT_PTF("\nGW:");
    IPV4_ORG_PTF(gw);
    SWAT_PTF("\n");  
}

void swat_wmiconfig_autoip_success_cb(A_UINT32 ipAddr,A_UINT32 mask,A_UINT32 gw){

    SWAT_PTF("IP Addr:");
    IPV4_ORG_PTF(ipAddr);
    SWAT_PTF("\nMask :");
    IPV4_ORG_PTF(mask);
    SWAT_PTF("\nGW:");
    IPV4_ORG_PTF(gw);
    SWAT_PTF("\n");  

    //send dhcp request
    swat_wmiconfig_ipdhcp(currentDeviceId);
}

A_INT32 swat_wmiconfig_dhcpc_cb_enable(A_UINT8 device_id,char * enable){

      if(!strcmp(enable, "start"))
      {
           qcom_dhcpc_register_cb(device_id,swat_wmiconfig_dhcpc_success_cb); 
      }
      else if(!strcmp(enable, "stop"))
      {
           qcom_dhcpc_register_cb(device_id,NULL); 
      }
      else
      {
           SWAT_PTF("input parameter should be start or stop!\n");
          return A_ERROR;
      }
     return A_OK;
}

A_INT32 swat_wmiconfig_autoip_cb_enable(A_UINT8 device_id,char * enable){

      if(!strcmp(enable, "start"))
      {
           qcom_autoip_register_cb(device_id, swat_wmiconfig_autoip_success_cb); 
      }
      else if(!strcmp(enable, "stop"))
      {
           qcom_autoip_register_cb(device_id,NULL); 
      }
      else
      {
           SWAT_PTF("input parameter should be start or stop!\n");
          return A_ERROR;
      }
     return A_OK;
}

void swat_wmiconfig_mcast_filter_set(A_UINT8 config)
{
    qcom_mcast_filter_enable(config);
}

/*_____________________ OTA upgrade through HTTP(s) _________________________*/

A_INT32 swat_ota_https_upgrade(A_UINT8 device_id, A_INT32 argc, char *argv[])
{
        A_UINT32 ip_addr, flags, partition_index;
        A_UINT16 port;
        A_UINT32 resp_code;
        A_UINT32 size;    

        if (argc < 7) {
                printf("Incomplete params\n");
                printf("Usage: wmiconfig --ota_http(or ota_https) <ip> <port> <filename> <flags> <partition_index> \n");
                return A_ERROR;
        }

        ip_addr = _inet_addr(argv[2]);    
        port = atoi(argv[3]);
        flags = atoi(argv[5]);
        partition_index = atoi(argv[6]);

        resp_code = qcom_ota_https_upgrade(device_id, argv[2], ip_addr, port, argv[4], flags, partition_index, &size);
        if (A_OK != resp_code) {
                printf("OTA Image Download Failed ERR:%d\n", resp_code);
        } else {
                printf("OTA Image Download Completed successfully,Image Size:%d\n", size);
        }
        return A_OK;
}

A_INT32 swat_wmiconfig_httpsvr_ota_enable(char * enable)
{   
     A_INT32 ota_enable;
     char* ota_uri = "uploadbin.cgi";
     
     if(!strcmp(enable, "disable"))
     {
          ota_enable = 0;
     }
     else if(!strcmp(enable, "enable"))
     {
          ota_enable = 1;
     }
     else
     {
         SWAT_PTF("input parameter should be diasble or enable!\n");
         return A_ERROR;
     }

    qcom_httpsvr_ota_enable(currentDeviceId,ota_enable);
    qcom_set_httpsvr_ota_uri(currentDeviceId,ota_uri);
    return A_OK;
}



