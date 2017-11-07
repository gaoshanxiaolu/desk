/* Copyright (c) 2017 Qualcomm Technologies, Inc.
 * All Rights Reserved.
 * Confidential and Proprietary - Qualcomm Technologies, Inc.
 */
/* 
 *
 * Copyright (C) 2010--2016 Olaf Bergmann <bergmann@tzi.org>
 *
 */
/*
 * For this file, which was received with alternative licensing options for  
 * distribution, Qualcomm Technologies, Inc. has selected the BSD license.
 */
#include <string.h>
#include <stdio.h>
#include "stdint.h"
#include <stddef.h>
#include "base.h"
#include "socket.h"
#include "socket_api.h"
#include "qcom_network.h"
#include "qcom_system.h"
#include "qcom_sec.h"
#include "qcom_scan.h"
#include "qcom_timer.h"
#include "qcom_internal.h"
#include "qcom_sntp.h"
#include "qcom_mem.h"
#include <errno.h>
//#include "qcom_utils.h"
#include "qcom_coap.h"
#include "swat_wmiconfig_network.h"


extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();

extern long int strtol (__const char *__restrict __nptr, char **__restrict __endptr, int __base);
extern int atoi(const char *buf);
extern int isdigit(int c);

extern int qcom_sprintf(char *s, const char *fmt, ...);


/*************************
  ****COAP SERVER DEMO****
  *************************/

#ifndef min
#define min(a,b) ((a) < (b) ? (a) : (b))
#endif

/* temporary storage for dynamic resource representations */
static int quit = 0;

/* changeable clock base (see handle_put_time()) */
static time_t clock_offset;
static time_t my_clock_base = 0;

struct coap_resource_t *time_resource = NULL;

#define COAP_CLIENT_CONTEXT_MAX  4

typedef struct coap_observe_t {
  coap_token_t token;
  coap_list_t *optlist;
  coap_tick_t obs_time;
  unsigned char msg_type;
} coap_observe_t;

typedef struct coap_client_config_t{
    coap_tick_t wait_ms;
    unsigned int obs_seconds;
    char uri_buffer[256];  //just for test
    char a_type[64]; // accepted content type. just for test
    char t_type[64]; // given resource content type. just for test.
    SSL_CTX* sslCtx;
    int stop;
    int ready;
    int new_req;
    coap_observe_t observer;
    coap_req_t req;
}coap_client_config_t;

coap_client_config_t *coap_client[COAP_CLIENT_CONTEXT_MAX] = {0};

typedef struct coap_server_context_t{
    char addr_str[NI_MAXHOST];
    char port_str[NI_MAXSERV];
    char group_str[NI_MAXHOST];
    coap_address_t src_addr;
    SSL_CTX* sslCtx;
    unsigned int flags;
}coap_server_context_t;


/* SIGINT handler: set quit to 1 for graceful termination */
static void
coap_server_stop() {
  quit = 1;
}

#define INDEX "This is a test server made with libcoap!"

static void
hnd_get_index(coap_context_t *ctx,
              struct coap_resource_t *resource,
              const coap_endpoint_t *local_interface,
              coap_address_t *peer,
              coap_pdu_t *request,
              str *token,
              coap_pdu_t *response) {
  unsigned char buf[3];

  response->hdr->code = COAP_RESPONSE_CODE(205);

  qcom_coap_add_option(response,
                  COAP_OPTION_CONTENT_TYPE,
                  qcom_coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

  qcom_coap_add_option(response,
                  COAP_OPTION_MAXAGE,
                  qcom_coap_encode_var_bytes(buf, 0x2ffff), buf);

  qcom_coap_add_data(response, strlen(INDEX), (unsigned char *)INDEX);
}

static void
hnd_get_time(coap_context_t  *ctx,
             struct coap_resource_t *resource,
             const coap_endpoint_t *local_interface,
             coap_address_t *peer,
             coap_pdu_t *request,
             str *token,
             coap_pdu_t *response) {
  coap_opt_iterator_t opt_iter;
  coap_opt_t *option;
  unsigned char buf[40];
  size_t len;
  time_t now;
  coap_tick_t t;

  /* if my_clock_base was deleted, we pretend to have no such resource */
  response->hdr->code =
    my_clock_base ? COAP_RESPONSE_CODE(205) : COAP_RESPONSE_CODE(404);

  if (qcom_coap_find_observer(resource, peer, token)) {
    /* FIXME: need to check for resource->dirty? */
    qcom_coap_add_option(response,
                    COAP_OPTION_OBSERVE,
                    qcom_coap_encode_var_bytes(buf, ctx->observe), buf);
  }

  if (my_clock_base)
    qcom_coap_add_option(response,
                    COAP_OPTION_CONTENT_FORMAT,
                    qcom_coap_encode_var_bytes(buf, COAP_MEDIATYPE_TEXT_PLAIN), buf);

  qcom_coap_add_option(response,
                  COAP_OPTION_MAXAGE,
                  qcom_coap_encode_var_bytes(buf, 0x01), buf);

  if (my_clock_base) {
    /* calculate current time */
    qcom_coap_ticks(&t);
    now = my_clock_base + t;

    if (request != NULL
        && (option = qcom_coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter))
        && memcmp(COAP_OPT_VALUE(option), "ticks",
        min(5, COAP_OPT_LENGTH(option))) == 0) {
          /* output ticks */
          len = snprintf((char *)buf,
                         min(sizeof(buf),
                             response->max_size - response->length),
                             "%u s, %u ms", (unsigned int)(now/COAP_TICKS_PER_SECOND), (unsigned int)(now%COAP_TICKS_PER_SECOND));
          qcom_coap_add_data(response, len, buf);

   } 
   else {
     tSntpTime time;
     char *months[12] = {"Jan","Feb","Mar","Apr","May","Jun","Jul","Aug","Sep","Oct","Nov","Dec"};
     char *Day[7] = {"Sun","Mon","Tue","Wed","Thu","Fri","Sat"};

     qcom_sntp_get_time(1, &time);
     len = snprintf((char*)buf, min(sizeof(buf), response->max_size - response->length), "Timestamp : %s %s %d ,%d %d:%d:%d", Day[time.wday], months[time.mon], time.yday,
                time.year, time.hour, time.min, time.Sec);
     qcom_coap_add_data(response, len, buf);
  }
 }
}

static void
hnd_put_time(coap_context_t *ctx,
             struct coap_resource_t *resource,
             const coap_endpoint_t *local_interface,
             coap_address_t *peer,
             coap_pdu_t *request,
             str *token,
             coap_pdu_t *response) {
  size_t size;
  unsigned char *data;
  char buf[16] = {0};

  /* FIXME: re-set my_clock_base to clock_offset if my_clock_base == 0
   * and request is empty. When not empty, set to value in request payload
   * (insist on query ?ticks). Return Created or Ok.
   */

  /* if my_clock_base was deleted, we pretend to have no such resource */
  response->hdr->code =
    my_clock_base ? COAP_RESPONSE_CODE(204) : COAP_RESPONSE_CODE(201);

  resource->dirty = 1;

  qcom_coap_get_data(request, &size, &data);


  if (size == 0){        /* re-init */
    my_clock_base = clock_offset;
  } else { 
    memcpy(buf, data, size);
    buf[size] = '\0';
    COAP_DEBUG("PUT Time: %s %d\n", buf, atoi(buf));
    my_clock_base = atoi(buf)*COAP_TICKS_PER_SECOND;
  }
}

static void
hnd_delete_time(coap_context_t *ctx,
                struct coap_resource_t *resource,
                const coap_endpoint_t *local_interface,
                coap_address_t *peer,
                coap_pdu_t *request,
                str *token,
                coap_pdu_t *response) {                
  my_clock_base = 0;    /* mark clock as "deleted" */
 
  response->hdr->code =
    my_clock_base ? COAP_RESPONSE_CODE(500) : COAP_RESPONSE_CODE(202);

  /* type = request->hdr->type == COAP_MESSAGE_CON  */
  /*   ? COAP_MESSAGE_ACK : COAP_MESSAGE_NON; */
}


static void
hnd_post_test(coap_context_t *ctx,
                struct coap_resource_t *resource,
                const coap_endpoint_t *local_interface,
                coap_address_t *peer,
                coap_pdu_t *request,
                str *token,
                coap_pdu_t *response) {                
    size_t size;
    unsigned char *data;
    int i;
    char buf[64] = {0};
    /* FIXME: re-set my_clock_base to clock_offset if my_clock_base == 0
     * and request is empty. When not empty, set to value in request payload
     * (insist on query ?ticks). Return Created or Ok.
     */
    
    /* if my_clock_base was deleted, we pretend to have no such resource */
    response->hdr->code = COAP_RESPONSE_CODE(204);
     
    qcom_coap_get_data(request, &size, &data);
    
    if (size == 0){        /* re-init */
      COAP_DEBUG("Post Test!");
    } else {
      for (i=size; i>=0; i--){
        buf[size-i] = *(data+i-1); 
      }
      COAP_DEBUG("Post Test: %s\n", buf);
    }
}

static void
hnd_block1_test(coap_context_t *ctx,
                struct coap_resource_t *resource,
                const coap_endpoint_t *local_interface,
                coap_address_t *peer,
                coap_pdu_t *request,
                str *token,
                coap_pdu_t *response) {                
    size_t size;
    unsigned char *data;
    char buf[64] = {0};
    unsigned char buf_block[4] = {0};
    coap_opt_t *block_opt;
    coap_opt_iterator_t opt_iter;
    coap_block_t block;

    qcom_coap_get_data(request, &size, &data);
    
    if (size > 0) {
      size = (size<=63)?size:63;
      memcpy(buf, data, size);
      buf[size] = '\0';
      COAP_DEBUG("Block1 Test: %s\n", buf);
    }
    
    block_opt = qcom_coap_check_option(request, COAP_OPTION_BLOCK1, &opt_iter);
    
    if (block_opt) { /* handle Block1 */
    block.m = COAP_OPT_BLOCK_MORE(block_opt)?1:0;    
    block.szx = COAP_OPT_BLOCK_SZX(block_opt);
    block.num = COAP_OPT_BLOCK_NUM(block_opt);    

    response->hdr->code = block.m?COAP_RESPONSE_CODE(231):COAP_RESPONSE_CODE(204);

    COAP_DEBUG("Block1 num %u, m %u, size %u\n", block.num, block.m, (1<<(block.szx+4)));     
    memset(buf, 0, sizeof(buf));
    qcom_coap_add_option(response,
                COAP_OPTION_BLOCK1,
                qcom_coap_encode_var_bytes(buf_block,
                ( (block.num<< 4)) | (block.m<< 3 )| block.szx), buf_block);
    }
}

static void
hnd_block2_test(coap_context_t *ctx,
                struct coap_resource_t *resource,
                const coap_endpoint_t *local_interface,
                coap_address_t *peer,
                coap_pdu_t *request,
                str *token,
                coap_pdu_t *response) {                
    unsigned char buf[32] = {0};
    coap_opt_t *block_opt;
    coap_opt_iterator_t opt_iter;
    coap_block_t block;
         
    
    block_opt = qcom_coap_check_option(request, COAP_OPTION_BLOCK2, &opt_iter);
    if (!block_opt){
        block.num = 0;
        block.szx = (qcom_coap_fls(16 >> 4) - 1) & 0x07;
        block.m = strlen(INDEX) > 16;
    } else {
        block.num = COAP_OPT_BLOCK_NUM(block_opt);
        block.szx = (qcom_coap_fls(16 >> 4) - 1) & 0x07;
        block.m = ((block.num+1) * (1 << (block.szx + 4)) < strlen(INDEX));
    }
    COAP_DEBUG("Block2 num %u, m %u, size %u\n", block.num, block.m, (1<<(block.szx+4)));     
    
    response->hdr->code = COAP_RESPONSE_CODE(205);
    
    qcom_coap_add_option(response,
                COAP_OPTION_BLOCK2,
                qcom_coap_encode_var_bytes(buf,
                (block.num << 4) | (block.m<< 3) | block.szx), buf);

    qcom_coap_add_block(response, strlen(INDEX),  (unsigned char*)INDEX, block.num,  block.szx);
}


static void
hnd_get_async(coap_context_t *ctx,
              struct coap_resource_t *resource,
              const coap_endpoint_t *local_interface,
              coap_address_t *peer,
              coap_pdu_t *request,
              str *token,
              coap_pdu_t *response) {
  coap_opt_iterator_t opt_iter;
  coap_opt_t *option;
  unsigned long delay = 5;
  size_t size;

  option = qcom_coap_check_option(request, COAP_OPTION_URI_QUERY, &opt_iter);
  if (option) {
    unsigned char *p = COAP_OPT_VALUE(option);

    delay = 0;
    for (size = COAP_OPT_LENGTH(option); size; --size, ++p)
      delay = delay * 10 + (*p - '0');
  }

  qcom_coap_register_async(ctx,
                              peer,
                              request,
                              COAP_ASYNC_SEPARATE | COAP_ASYNC_CONFIRM,
                              (void *)(COAP_TICKS_PER_SECOND * delay));
}

static void
init_resources(coap_context_t *ctx) {
  coap_resource_t *r;

  r = qcom_coap_resource_init(NULL, 0, 0);
  qcom_coap_register_handler(r, COAP_REQUEST_GET, hnd_get_index);

  qcom_coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  qcom_coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"General Info\"", 14, 0);
  qcom_coap_add_resource(ctx, r);

  /* store clock base to use in /time */
  my_clock_base = clock_offset;

  r = qcom_coap_resource_init((unsigned char *)"time", 4, COAP_RESOURCE_FLAGS_NOTIFY_CON);
  qcom_coap_register_handler(r, COAP_REQUEST_GET, hnd_get_time);
  qcom_coap_register_handler(r, COAP_REQUEST_PUT, hnd_put_time);
  qcom_coap_register_handler(r, COAP_REQUEST_DELETE, hnd_delete_time);

  qcom_coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  qcom_coap_add_attr(r, (unsigned char *)"title", 5, (unsigned char *)"\"Internal Clock\"", 16, 0);
  qcom_coap_add_attr(r, (unsigned char *)"rt", 2, (unsigned char *)"\"Ticks\"", 7, 0);
  r->observable = 1;
  qcom_coap_add_attr(r, (unsigned char *)"if", 2, (unsigned char *)"\"clock\"", 7, 0);

  qcom_coap_add_resource(ctx, r);
  time_resource = r;

  r = qcom_coap_resource_init((unsigned char *)"async", 5, 0);
  qcom_coap_register_handler(r, COAP_REQUEST_GET, hnd_get_async);

  qcom_coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  qcom_coap_add_resource(ctx, r);

  
  r = qcom_coap_resource_init((unsigned char *)"post_test", 9, 0);
  qcom_coap_register_handler(r, COAP_REQUEST_POST, hnd_post_test);

  qcom_coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  qcom_coap_add_resource(ctx, r);

  r = qcom_coap_resource_init((unsigned char *)"block1_test", 11, 0);
  qcom_coap_register_handler(r, COAP_REQUEST_POST, hnd_block1_test);

  qcom_coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  qcom_coap_add_resource(ctx, r);

    r = qcom_coap_resource_init((unsigned char *)"block2_test", 11, 0);
  qcom_coap_register_handler(r, COAP_REQUEST_GET, hnd_block2_test);

  qcom_coap_add_attr(r, (unsigned char *)"ct", 2, (unsigned char *)"0", 1, 0);
  qcom_coap_add_resource(ctx, r);
}

#ifdef DTLS_ENABLED
static void
fill_keystore(coap_context_t *ctx) {
  coap_keystore_item_t *psk;
  static unsigned char user[] = "Client_identity";
  size_t user_length = sizeof(user) - 1;
  static unsigned char key[] = "secretPSK";
  size_t key_length = sizeof(key) - 1;

  psk = coap_keystore_new_psk(NULL, 0, user, user_length,
                              key, key_length, 0);
  if (!psk || !coap_keystore_store_item(ctx->keystore, psk, NULL)) {
    coap_log(LOG_WARNING, "cannot store key\n");
  }
}
#endif


int 
swat_coap_server_free(coap_server_context_t *pCoapServer){
    if (!pCoapServer)
      return 0;

    qcom_mem_free(pCoapServer);
    
   return 0;
}

void
coap_server_start(unsigned int serverCtx) {
  coap_context_t  *ctx = NULL;
  unsigned int wait_ms;
  int result;
  int start_fail = 1;
  str server;
  coap_server_context_t *pCoapServer = (coap_server_context_t *)serverCtx;
  
  server.length = strlen(pCoapServer->addr_str);
  server.s = (unsigned char*)pCoapServer->addr_str;
  /* resolve local address where server should be sent */
  if (qcom_coap_resolve_address(&server, &pCoapServer->src_addr, pCoapServer->flags &COAP_ENDPOINT_IPV6) < 0) {
      goto EXIT;
  }
  pCoapServer->src_addr.addr.sin.sin_port = htons(atoi(pCoapServer->port_str));  
  
  ctx = qcom_coap_new_context();
  if (!ctx) {
    goto EXIT;
  }

  result = qcom_coap_conext_init(ctx, &pCoapServer->src_addr, pCoapServer->flags, NULL);

  if (result != A_COAP_OK) {
    COAP_ALERT("cannot create context\n");
    goto EXIT;
  }

  result = qcom_coap_server_setup(ctx,  pCoapServer->sslCtx, pCoapServer->group_str, pCoapServer->flags);
   if (result != A_COAP_OK) {
    COAP_ALERT("coap server setup fail\n");
    goto EXIT;
  }
   
  init_resources(ctx);

  start_fail = 0;
  COAP_DEBUG("coap server start Success\n");
  wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;

  quit = 0;
  while (!quit) {
    result = qcom_coap_run_once(ctx, wait_ms);

    if (result < 0) {
      break;
    } else if ((unsigned int)result < wait_ms) {
      wait_ms -= result;
    } else {
      if (time_resource) {
        time_resource->dirty = 1;
      }
      wait_ms = COAP_RESOURCE_CHECK_TIME * 1000;
    }
    /* check if we have to send asynchronous responses */
    qcom_coap_check_async(ctx, ctx->endpoint);
    /* check if we have to send observe responses */
    qcom_coap_check_notify(ctx);
  }

EXIT:
  if (start_fail){
    COAP_ALERT("coap server start FAIL\n");
  }
  qcom_coap_free_context(ctx);
  swat_coap_server_free(pCoapServer);
  qcom_task_exit();

  return;
}

static void coap_server_init(coap_server_context_t *pCoapServer){
    memcpy(pCoapServer->addr_str, "::", NI_MAXHOST);
    memcpy(pCoapServer->port_str, "5683", NI_MAXSERV);
}


A_COAP_STATUS
coap_server_test(int argc, char **argv) {
  coap_log_t log_level = LOG_WARNING;
  coap_server_context_t *pCoapServer = NULL;
  int i= 2; 
  int ret;
  SSL_INST *ssl;
  A_UINT32 ssl_inst_index;
  
  unsigned int coap_flags = COAP_ENDPOINT_SERVER;

  if ((argc == 3) && (strcmp(argv[2], "stop")== 0)){
    coap_server_stop();
    return A_COAP_OK;
  } else if (argc <= 4){
    COAP_ALERT("Error Coap Server Command!\r\n");
    return A_COAP_ERROR;
  }

  pCoapServer = (coap_server_context_t *)qcom_mem_alloc(sizeof(coap_server_context_t));

  if (!pCoapServer){
    COAP_ALERT("No Memory!\r\n");    
    return A_COAP_ERROR;
  }

  coap_server_init(pCoapServer);
  
  qcom_coap_ticks((coap_tick_t*)&clock_offset);

  if(strcmp(argv[i], "udp") == 0)
  {
      coap_flags |= COAP_ENDPOINT_UDP;
       i++;
  }
  else if(strcmp(argv[i], "tcp") == 0)
  {
      coap_flags |= COAP_ENDPOINT_TCP;
      i++;
  }
  else
  {
    COAP_ALERT("unknown protocol, udp or tcp");
    goto COAP_SERVER_ERROR;
  }

  while ((argc > i)&&(strlen(argv[i]) == 2)) {
    switch (*(argv[i]+1)) {
    case 'A' :
      strncpy(pCoapServer->addr_str, argv[++i], NI_MAXHOST-1);
      break;
    case 'g' :
      strncpy(pCoapServer->group_str, argv[++i], NI_MAXHOST-1);
      coap_flags |= COAP_ENDPOINT_GROUP ;
      break;
    case 'p' :
      strncpy(pCoapServer->port_str, argv[++i], NI_MAXSERV-1);
      break;
    case 'v' :
      if (strcmp(argv[++i], "ipv6") == 0){
          coap_flags |= COAP_ENDPOINT_IPV6 ;
      } else  if (strcmp(argv[i], "ipv4") == 0){
          coap_flags |= COAP_ENDPOINT_IPV4 ;
      } else {
        COAP_ALERT("wrong ip protocol!");
        goto COAP_SERVER_ERROR;
      }
      break;
    case 'd' :
      log_level = strtol(argv[++i], NULL, 10);
      break;
    case 'k' :
      ssl_inst_index = atoi(argv[++i]);
      if(NULL == (ssl =  swat_find_ssl_inst(ssl_inst_index))){
           return A_ERROR;
      }

     if (ssl->sslCtx == NULL || SSL_SERVER != ssl->role) {
         printf("ERROR: SSL Server %d not started\n", ssl_inst_index);
         return A_ERROR;
     }
     pCoapServer->sslCtx = ssl->sslCtx;
     break;
    default:
      goto COAP_SERVER_ERROR;
    }
    i++;
  }


  qcom_coap_set_log_level(log_level);
  if (pCoapServer->sslCtx){
      if (coap_flags&COAP_ENDPOINT_TCP){
          coap_flags |= COAP_ENDPOINT_TLS;
      } else {
          coap_flags |= COAP_ENDPOINT_DTLS;
      }
  }
  pCoapServer->flags = coap_flags;
  ret = qcom_task_start(coap_server_start, (unsigned int)pCoapServer, 4096, 50);
  if (0 == ret) {
        return A_COAP_OK;
  }

COAP_SERVER_ERROR:
  COAP_DEBUG("coap server start FAIL\n");
  qcom_mem_free(pCoapServer);;
  return A_COAP_ERROR;

}

/*************************
  ****COAP CLIENT DEMO****
  *************************/
static int
swat_cmdline_blocksize(char *arg, coap_block_t* block, unsigned int *flags) {
  unsigned short size;

  again:
  size = 0;
  while(*arg && *arg != ',')
    size = size * 10 + (*arg++ - '0');

  if (*arg == ',') {
    arg++;
    block->num = size;
    goto again;
  }

  if (size)
    block->szx = (qcom_coap_fls(size >> 4) - 1) & 0x07;

  *flags |= COAP_FLAGS_BLOCK;
  return 1;
}

static void
swat_cmdline_subscribe(char *arg, unsigned int *obs_seconds, unsigned int *flags) {
  *obs_seconds = atoi(arg);
  *flags |= COAP_FLAGS_SUBSCRIBE;
}

void
swat_cmdline_token(char *arg, coap_token_t *token) {
  token->length = min(COAP_TOKEN_MAX_LEN, strlen(arg));
  strncpy((char *)token->s, arg, token->length);
}

/**
 * Calculates decimal value from hexadecimal ASCII character given in
 * @p c. The caller must ensure that @p c actually represents a valid
 * heaxdecimal character, e.g. with isxdigit(3).
 *
 * @hideinitializer
 */
#define hexchar_to_dec(c) ((c) & 0x40 ? ((c) & 0x0F) + 9 : ((c) & 0x0F))

static method_t
swat_cmdline_method(char *arg) {
  static char *methods[] =
    { 0, "get", "post", "put", "delete", 0};
  unsigned char i;

  for (i=1; methods[i] && strcasecmp(arg,methods[i]) != 0 ; ++i)
    ;

  return i;     /* note that we do not prevent illegal methods */
}

int
swat_coap_add_observe(coap_observe_t *obs, unsigned char msg_type, coap_token_t *token, coap_list_t *optlist, unsigned int obs_seconds){

    if (!obs){
        return A_COAP_ERROR;
    }
    memset(obs, 0, sizeof(coap_observe_t));
    obs->msg_type = msg_type;
    if (token){
        memcpy(&obs->token, token, sizeof(coap_token_t));
    }

    if(optlist){
        obs->optlist = optlist;
    }
    obs->obs_time= obs_seconds*COAP_TICKS_PER_SECOND;
    return A_COAP_OK;
}

int 
swat_coap_client_free(coap_client_config_t *pCoapClient){
    coap_req_t *req;
    coap_observe_t *obs;
    if (!pCoapClient)
      return 0;

    
    req = &pCoapClient->req;

    if (req->payload.length >0 && req->payload.s){
        qcom_mem_free(req->payload.s);
        req->payload.length = 0;
        req->payload.s = NULL;
    }
    obs= &pCoapClient->observer;
    if (obs->optlist){
        qcom_coap_delete_list(&obs->optlist);
    }
    
    qcom_mem_free(pCoapClient);
    
   return 0;
}

static void swat_coap_client_init(coap_client_config_t *pCoapClient){
    if (!pCoapClient)
        return;
    
    memset(pCoapClient, 0, sizeof(coap_client_config_t));
    pCoapClient->wait_ms= 90*1000; //90 seconds
    pCoapClient->obs_seconds = 30; 
}

void
swat_coap_set_content_type(char *arg, unsigned short key, coap_list_t **optlist) {
  static content_type_t content_types[] = {
    {  0, "plain" },
    {  0, "text/plain" },
    { 40, "link" },
    { 40, "link-format" },
    { 40, "application/link-format" },
    { 41, "xml" },
    { 41, "application/xml" },
    { 42, "binary" },
    { 42, "octet-stream" },
    { 42, "application/octet-stream" },
    { 47, "exi" },
    { 47, "application/exi" },
    { 50, "json" },
    { 50, "application/json" },
    { 60, "cbor" },
    { 60, "application/cbor" },
    { 255, NULL }
  };
  coap_list_t *node;
  unsigned char i, value[10];
  int valcnt = 0;
  unsigned char buf[2];
  char *p, *q = arg;

  while (q && *q) {
    p = strchr(q, ',');

    if (isdigit(*q)) {
      if (p)
        *p = '\0';
      value[valcnt++] = atoi(q);
    } else {
      for (i=0;
           content_types[i].media_type &&
           strncmp(q, content_types[i].media_type, p ? (size_t)(p-q) : strlen(q)) != 0 ;
           ++i)
      ;

      if (content_types[i].media_type) {
        value[valcnt] = content_types[i].code;
        valcnt++;
      } else {
        COAP_WARNING("W: unknown content-format '%s'\n",arg);
      }
    }

    if (!p || key == COAP_OPTION_CONTENT_TYPE)
      break;

    q = p+1;
  }

  for (i = 0; i < valcnt; ++i) {
    node = qcom_coap_new_option(key, qcom_coap_encode_var_bytes(buf, value[i]), buf);
    if (node) {
      qcom_coap_insert_option(optlist, node);
    }
  }
}

A_COAP_STATUS swat_coap_client_add_options(coap_list_t **optlist, coap_client_config_t *config)
{   
     coap_req_t *req;
     
     if (!optlist || !config){
        return A_COAP_ERROR;
     }
     
     req = &config->req;
     qcom_coap_add_uri(&req->uri, optlist);

     if (strlen(config->a_type) > 0){
        swat_coap_set_content_type(config->a_type,COAP_OPTION_ACCEPT, optlist);
     }

     if (strlen(config->t_type) > 0){
        swat_coap_set_content_type(config->t_type,COAP_OPTION_CONTENT_TYPE, optlist);
     }
 
    /* set block option if requested at commandline */
    if (req->flags & COAP_FLAGS_BLOCK){
      qcom_coap_set_blocksize(req->method, &req->block, &req->payload, optlist);
    }
  
    if (req->flags & COAP_FLAGS_SUBSCRIBE){
      qcom_coap_insert_option(optlist, qcom_coap_new_option(COAP_OPTION_SUBSCRIPTION, 0, NULL));
    }
     return A_COAP_OK;     
}

void
app_handler(struct coap_context_t *ctx,
                const coap_endpoint_t *local_interface,
                const coap_address_t *remote,
                coap_pdu_t *sent,
                coap_pdu_t *received,
                const coap_tid_t id,
                unsigned int resp_code) {

    size_t len;
    unsigned char *databuf;
    char tmpbuf[256] = {0}; // just for test
    int ack_data = 1;
    int ready = 1;
    int sub_ack = 0;

    if (resp_code&COAP_RESP_ACK_SUBSCRIBE){
        //subscribe sucess;
        resp_code &= ~COAP_RESP_ACK_SUBSCRIBE;
        sub_ack = 1;
        printf("subcribe success\r\n");
    }
    
    if (resp_code== COAP_RESP_ACK_ONLY){
        ack_data = 0;
        printf("ack only\r\n");
        goto COAP_READY;
        //ack without data;
    }  
    if (resp_code== COAP_RESP_TIMEOUT){
        ack_data = 0;
        printf("send timeout\r\n");
        //ack without data;
        goto COAP_READY;
    }  
    
    if (COAP_RESPONSE_CLASS(received->hdr->code) == 2) {

        if (resp_code == COAP_RESP_BLOCK2_MORE){
              ready = 0;
              printf("block2 more \r\n");
              //more block will be received.
        } else if (resp_code == COAP_RESP_BLOCK2_LAST){
               printf("block2 last \r\n");
              //the last block received.
          } else if (resp_code == COAP_RESP_BLOCK1_MORE){
              ready = 0;
              ack_data = 0;
              printf("block1 more \r\n");
              //more block will be sent.
          } else if (resp_code == COAP_RESP_BLOCK1_LAST){
               ack_data = 0;
               printf("block1 last \r\n");
              //the last block sent out.
          }else if (resp_code == COAP_RESP_ACK_DATA){
                printf("ack data \r\n");
              //acked with data
          } 
        if (qcom_coap_get_data(received, &len, &databuf)){
               memcpy(tmpbuf, databuf, len);
               printf("received data: %s\r\n",tmpbuf);
         }    

      }else {      /* no 2.05 */
      /* check if an error was signaled and output payload if so */
          if (resp_code == COAP_RESP_CLIENT_ERROR) {
              printf("client error \r\n");
              //client error
          } else if (resp_code == COAP_RESP_SERVER_ERROR){
              printf("server error \r\n");
              //server error
          }
    }

COAP_READY:
    if (ready == 1){
        memset(&ctx->request, 0, sizeof(coap_req_t));
    }

}

void coap_client_start(unsigned int index)
{
    int result = -1;
    coap_context_t  *ctx = NULL;
    coap_pdu_t  *pdu;
    str server;
    unsigned short port = COAP_DEFAULT_PORT;
    coap_req_t *req;
    coap_address_t dst;
    coap_client_config_t *pCoapClient = coap_client[index];
    coap_observe_t *obs = NULL;
    coap_tick_t start;//, now;
    int oncePrint;

    if (!pCoapClient){
        goto finish;
    }

    req = &pCoapClient->req;
    obs = &pCoapClient->observer;

    server = req->uri.host;
    port = req->uri.port;

    /* resolve destination address where server should be sent */
    if (qcom_coap_resolve_address(&server, &dst, req->flags &COAP_ENDPOINT_IPV6) < 0) {
        goto finish;
    }
  
    dst.addr.sin.sin_port = htons(port);
    
    ctx = qcom_coap_new_context();
    if (!ctx) {
      goto finish;
    }
  
    if (qcom_coap_conext_init(ctx, NULL, req->flags, app_handler) != A_COAP_OK) {
      COAP_ALERT("cannot create context\n");
      goto finish;
    }
  
    if (qcom_coap_client_connect(ctx, pCoapClient->sslCtx, &dst, req->flags) == A_COAP_ERROR){
        goto finish;
    }
    COAP_DEBUG("coap client start Success\n");

COAP_REQUEST:
   swat_coap_client_add_options(&req->optlist,pCoapClient);
  
   if (! (pdu = qcom_coap_new_request(ctx, req->method, req->msgtype, &req->token, &req->optlist))){
       goto finish;
   }

   if (qcom_coap_request_payload(ctx, pdu, req->payload.s, req->payload.length, &req->block, req->flags)){
       goto finish;
   }
  
  if (pdu->hdr->type == COAP_MESSAGE_CON){
      req->tid = qcom_coap_send_confirmed(ctx, ctx->endpoint, &dst, pdu);
  } else {
      req->tid = qcom_coap_send(ctx, ctx->endpoint, &dst, pdu);
  }
  
  if (pdu->hdr->type != COAP_MESSAGE_CON || req->tid == COAP_INVALID_TID){
      qcom_coap_delete_list(&req->optlist);
      qcom_coap_delete_pdu(pdu);
  } else if (req->flags&COAP_FLAGS_SUBSCRIBE) {
      swat_coap_add_observe(obs, req->msgtype, &req->token, req->optlist, pCoapClient->obs_seconds);
      req->optlist = NULL; //optlist will be freed when cancel subscribe.
  }
  
   qcom_coap_ticks(&start);
   while (pCoapClient->stop == FALSE) {
       unsigned int wait_ms = (obs->obs_time>0) ? min(obs->obs_time, pCoapClient->wait_ms) : pCoapClient->wait_ms;
     if(qcom_coap_client_can_exit(ctx) && (obs->obs_time==0)){
         wait_ms = 1000;
     }
     
     result = qcom_coap_run_once(ctx, wait_ms);
     if (result > 0){
        if(obs->obs_time > 0){
            if (obs->obs_time > result){
                obs->obs_time -= result;
            } else if (obs->obs_time <= result){
                 obs->obs_time = 0;
                 qcom_coap_clear_obs(ctx,ctx->endpoint, &ctx->endpoint->dst,obs->msg_type, &obs->token, obs->optlist);
                 qcom_coap_delete_list(&obs->optlist);
                 memset(obs, 0, sizeof(coap_observe_t));
            }
         }
     } else {
         break;
    }
     
     if(qcom_coap_client_can_exit(ctx)){
         if (oncePrint == 0){
             COAP_ALERT("coap %d is available for new request!\n", index+1);
             oncePrint = 1;
             pCoapClient->ready = 1;

             if (req->payload.length >0 && req->payload.s){
                 qcom_mem_free(req->payload.s);
                 req->payload.length = 0;
                 req->payload.s=NULL;
             }

             if (req->optlist){
                qcom_coap_delete_list(&req->optlist);
            }
         }
     }

    if (pCoapClient->new_req){
        pCoapClient->ready = 0;
        pCoapClient->new_req = 0;
        oncePrint = 0;        
        goto COAP_REQUEST;
    }
  }
  
   result = 0;
  
  finish:
  
   if (result < 0){
     COAP_ALERT("coap client %d start FAIL\n", index+1);  
   }
   qcom_coap_free_context( ctx );
   if (req->optlist){
      qcom_coap_delete_list(&req->optlist);
   }

   if (obs&&obs->optlist){
       qcom_coap_delete_list(&obs->optlist);
   }
   
   swat_coap_client_free(pCoapClient);
   coap_client[index] = NULL;
   qcom_task_exit();
  
   return;

}

/*
 * By default, any CLI option need a parameter. 
 * If one option don't need a parameter, add it here.
 */
int coap_is_cli_option_need_parameter(char cli_option)
{
    switch (cli_option)
    {
    case 'N':
        return 0;

    default:
        break;
    }
    return 1;
}

A_COAP_STATUS
coap_client_test(int argc, char **argv) 
{
  coap_log_t log_level = LOG_WARNING;
  int i= 3, j=0, index =-1; 
  int ret = 0;
  SSL_INST *ssl;
  A_UINT32 ssl_inst_index;
  coap_req_t *req = NULL;
  coap_client_config_t *pCoapClient = NULL;
  unsigned int coap_flags = COAP_ENDPOINT_CLIENT;

  if (argc < 4)
  {
      COAP_ALERT("Coap client need more parameters!");
      return A_COAP_ERROR;
  }
  
  index = atoi(argv[2]);

  if (index<0 || index>COAP_CLIENT_CONTEXT_MAX){
      printf("Invalid coap client index!\r\n");
  }
  
  if ((argc == 4) && (strcmp(argv[3], "stop")== 0)){
      if (index< 1 || index>COAP_CLIENT_CONTEXT_MAX){
          printf("Invalid coap client index!\r\n");
          return A_COAP_ERROR;
      }
      if ( coap_client[index-1]){
        coap_client[index-1]->stop = TRUE;
      } else {
          printf("coap client %d stopped\r\n", index);        
      }
      return A_COAP_OK;
  }  
  
  if (index == 0){
      for (j=0; j<COAP_CLIENT_CONTEXT_MAX; j++){
          if (coap_client[j] == NULL){
              index = j;
              break;
          }
      }

      if (j >= COAP_CLIENT_CONTEXT_MAX){
          COAP_ALERT("No free COAP client!");
          return A_COAP_ERROR;
      }
      pCoapClient = (coap_client_config_t *)qcom_mem_alloc(sizeof(coap_client_config_t));

      if (pCoapClient == NULL){
          COAP_ALERT("Coap client memory alloc fail!");
          return A_COAP_ERROR;
      }
      swat_coap_client_init(pCoapClient);   
      req = &pCoapClient->req;
      coap_client[j] = pCoapClient;
      
      if(strcmp(argv[i], "udp") == 0)
      {
          coap_flags |= COAP_ENDPOINT_UDP;
          i++;
      }
      else if(strcmp(argv[i], "tcp") == 0)
      {
          coap_flags |= COAP_ENDPOINT_TCP;
          i++;
      }
      else
      {
          COAP_ALERT("unknown protocol, udp or tcp");
          goto COAP_CLIENT_ERROR;
      }
  } else {
      if (coap_client[index-1] == NULL){
          COAP_ALERT("This COAP client isn't exist!");
          return A_COAP_ERROR;
      }
      pCoapClient = coap_client[index-1];

      if (pCoapClient->ready == 0){
        COAP_ALERT("coap client %d is busy!\r\n", index);
        return A_COAP_ERROR;
      }
      
      req = &pCoapClient->req;
      req->msgtype = COAP_MESSAGE_CON;
      memset(req, 0, sizeof(coap_req_t));
      memset(pCoapClient->uri_buffer, 0, 256);
      memset(pCoapClient->a_type, 0, 64);
      memset(pCoapClient->t_type, 0, 64);
      
  }

  /* CLI "coap_client tcp|udp ...", parse the "..." part  */
  while ((argc > 2) && (i<argc) && (strlen(argv[i]) == 2)) {
    char cli_option = *(argv[i]+1); 
    if ((1 == coap_is_cli_option_need_parameter(cli_option))
        && (argv[++i] == NULL))
    {
      COAP_ALERT("Missing parameter after -%c", cli_option);
      goto COAP_CLIENT_ERROR;
    }

    /* note that "i" now point to the value of the option */
    switch (cli_option) {
    case 'b' :
      swat_cmdline_blocksize(argv[i], &req->block, &coap_flags);
      break;
    case 'B' :
      pCoapClient->wait_ms= atoi(argv[i])*1000;
      break;
    case 'e' :
      req->payload.length = strlen(argv[i]);
      req->payload.s = (unsigned char *)qcom_mem_alloc(req->payload.length + 1);
      if (req->payload.s == NULL){
        COAP_ALERT("Coap client payload memory alloc fail!");
        goto COAP_CLIENT_ERROR;
      }
      strncpy((char*)req->payload.s, argv[i], req->payload.length);      
      break;
    case 'k' :
     ssl_inst_index = atoi(argv[i]);
     ssl = swat_find_ssl_inst(ssl_inst_index);
     if (ssl == NULL || ssl->sslCtx == NULL || SSL_CLIENT != ssl->role)
     {
        COAP_ALERT("ERROR: SSL Client %d not started\n", ssl_inst_index);
        goto COAP_CLIENT_ERROR;
     }

     pCoapClient->sslCtx = ssl->sslCtx;
      break;
    case 'm' :
      req->method = swat_cmdline_method(argv[i]);
      break;
    case 'v' :
      if (strcmp(argv[i], "ipv6") == 0){
          coap_flags |=COAP_ENDPOINT_IPV6;
      } else  if (strcmp(argv[i], "ipv4") == 0){
          coap_flags |=COAP_ENDPOINT_IPV4;
      } else {
          COAP_ALERT("wrong ip protocol!");
          goto COAP_CLIENT_ERROR;
      }
      break;
    case 'N' :
      req->msgtype = COAP_MESSAGE_NON;
      break;
    case 's' :
      swat_cmdline_subscribe(argv[i], &pCoapClient->obs_seconds, &coap_flags);
      break;
    case 'A' :
      strncpy(pCoapClient->a_type, argv[i], 64);
      break;
    case 't' :
      strncpy(pCoapClient->t_type, argv[i], 64);
      break;
    case 'T' :
      swat_cmdline_token(argv[i], &req->token);
      break;
    case 'd' :
      log_level = strtol(argv[i], NULL, 10);
      break;
    default:
        COAP_ALERT("Unsupport option -%c!", cli_option);
        goto COAP_CLIENT_ERROR;
    }
    i++;    
  }

  qcom_coap_set_log_level(log_level);

  if (i < argc && i>4) {
    if ((strncmp(argv[i], "coap://", 7)==0) || (strncmp(argv[i], "coaps://", 8)==0) || (*argv[i] == '/')){
        memcpy(pCoapClient->uri_buffer, argv[i], strlen(argv[i]));
    } else if (*argv[i] != '/'){
        pCoapClient->uri_buffer[0] = '/';
        memcpy(&pCoapClient->uri_buffer[1], argv[i], strlen(argv[i]));        
    }
    qcom_coap_parse_uri(pCoapClient->uri_buffer, &req->uri);
    if (qcom_coap_uri_scheme_is_secure(&req->uri)) {
        if (pCoapClient->sslCtx == NULL){
            COAP_ALERT("Error Command!");
            goto COAP_CLIENT_ERROR;
        }
        
        if (coap_flags&COAP_ENDPOINT_UDP){
            coap_flags |=COAP_ENDPOINT_DTLS;
        } else {
            coap_flags |=COAP_ENDPOINT_TLS;        
        }
    }
  } else {
    COAP_ALERT("Error Command!");
    goto COAP_CLIENT_ERROR;
  }

  req->flags = coap_flags;
  if (atoi(argv[2])>0 && atoi(argv[2])<=COAP_CLIENT_CONTEXT_MAX){
    pCoapClient->new_req = 1;
    return A_COAP_OK;
  }

  ret = qcom_task_start(coap_client_start, (unsigned int)index, 4096*2, 50);
  if (0 == ret) {
      COAP_ALERT("coap client %d start\n", index+1);      
      return A_COAP_OK;
  }

COAP_CLIENT_ERROR:
    if (atoi(argv[2]) == 0){ //create new coap client fail
      COAP_ALERT("\ncoap client start FAIL");
      swat_coap_client_free(pCoapClient);
      coap_client[index] = NULL;
    } else { //Error command with exist coap client
       if (req && (req->payload.length >0) && (req->payload.s)){
           qcom_mem_free(req->payload.s);
           req->payload.length = 0;
           req->payload.s=NULL;
       }
    }
  return A_COAP_ERROR;
}


int
swat_wmiconfig_coap_handle(A_INT32 argc, A_CHAR * argv[])
{
   if (!strcmp(argv[1], "--coap_client")) {
        coap_client_test(argc, argv);
        return 1;
    } else if (!strcmp(argv[1], "--coap_server")){
       coap_server_test(argc, argv);
       return 1;
    }
    return 0;
}

