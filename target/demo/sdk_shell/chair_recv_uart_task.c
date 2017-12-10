#include "qcom_common.h"
#include "qcom_gpio.h"
#include "qcom_uart.h"
#include "select_api.h"
//#include "swat_parse.h"
//#include "qcom_cli.h"
#include "adc_layer_wmi.h"
#include "swat_wmiconfig_network.h"
#include "swat_parse.h"

#include "socket_api.h"
#include "wx_airkiss.h"

#include "myqueue.h"
//#include<stdlib.h>
//#include "qurt_mutex.h"
#include "ble_uart.h"

void add_chair_id(uint8 *pid);
void add_vol(uint8 VOL);
void add_pose(A_UINT8 pose);
void add_key_val(uint8 *pkey);

extern int qcom_task_start(void (*fn) (unsigned int), unsigned int arg, int stk_size, int tk_ms);
extern void qcom_task_exit();
void send_chair_state_cmd(A_INT8 val,uint16 did);

static TX_MUTEX mutex_fd;
#define TSK_SEM_INIT tx_mutex_create(&mutex_fd, "mutex_fd", TX_NO_INHERIT);
#define TSK_SEM_LOCK {int status =  tx_mutex_get(&mutex_fd, TX_WAIT_FOREVER); \
  if (status != TX_SUCCESS) {\
    printf("lock fd the task mutex failed !!!\n");\
  }\
}
    
#define TSK_SEM_UNLOCK { int status =  tx_mutex_put(&mutex_fd);\
  if (status != TX_SUCCESS) {\
    printf("unlock fd the task mutex failed !!!\n");\
  }\
}

#define BLE_UART_FRAME_LEN (16)

#define DEFINE_FRAME_LEN   (16)

#define DEFINE_FRAME_SIZE DEFINE_FRAME_LEN
//char *p[] = { "没人","正常","左倾","右倾","前倾","左前倾","右前倾" };
char *p[] = { "no_body","normal","forward","left","right","left_forward","right_forward" };
A_UINT8 last_index = 10;

uint8 skip_nbyte_find_head;
static uint8 buf_frame[32];
uint8 cmd_type;
static uint8 key_frame[32];

bool find_sync_head(uint8 *data, uint8 len)
{
	uint8 i;

	for(i=0;i<len-3;i++)
	{
		if(data[i] == 0xA1 && data[i+1] == 0xA2 && data[i+2] == 0xA3)
		{
			skip_nbyte_find_head = i;
			return TRUE;
		}
	}

	return FALSE;
}
int get_key_frame = FALSE;
bool ppr,chg,status;
int adc;
int print_log_cnt;

int get_recv_id=FALSE;
#define MAX_SUPPORT_DEV_NODE (8)
#define ID_LEN (5)
typedef struct
{
	uint8_t ids[ID_LEN];
	uint16 did;
	uint8 vol_per;
	uint8 pose;
	bool data_valid;
	uint8 types;//0 chair ; 1 light
}NODES;

NODES nodes_info[MAX_SUPPORT_DEV_NODE];
uint8_t node_cnt=0;
uint16 cur_did;
uint8 cur_pos;
bool is_exist_id(uint8_t *ids)
{
	if(node_cnt == 0)
	{
		return FALSE;
	}

	uint8 i;
	for(i=0;i<node_cnt;i++)
	{
		if(nodes_info[i].ids[0] == ids[0] && \
			nodes_info[i].ids[1] == ids[1] && \
			nodes_info[i].ids[2] == ids[2] && \
			nodes_info[i].ids[3] == ids[3] && \
			nodes_info[i].ids[4] == ids[4])
		{
			cur_pos = i;
			return TRUE;
		}
	}
	return FALSE;
}

void add_node(uint8_t *ids,uint16 did,uint8 vol_per,uint8 type)
{
	if(node_cnt > MAX_SUPPORT_DEV_NODE)
	{
		printf("^^^^^over max_support_dev_node\n");
		return;
	}
	
	nodes_info[node_cnt].ids[0] = ids[0];
	nodes_info[node_cnt].ids[1] = ids[1];
	nodes_info[node_cnt].ids[2] = ids[2]; 
	nodes_info[node_cnt].ids[3] = ids[3]; 
	nodes_info[node_cnt].ids[4] = ids[4];

	nodes_info[node_cnt].did = did;
	nodes_info[node_cnt].vol_per = vol_per;
	nodes_info[node_cnt].types = type;

	printf("->add id:%02x,%02x,%02x,%02x,%02x,did:%04x\n",ids[0],ids[1],ids[2],ids[3],ids[4],did);

	node_cnt++;

}

void upgrade_node(uint8 pos,uint16 did)
{
	nodes_info[pos].did = did;

	printf("->upgrade did, did:%04x\n",did);

}

void upgrade_pose_info(uint8 pose,uint16 did)
{
	uint8 i;
	bool get_did;

	get_did = FALSE;
	
	for(i=0;i<node_cnt;i++)
	{
		if(nodes_info[i].did == did)
		{
			get_did = TRUE;
			break;
		}
	}

	if(get_did)
	{
		nodes_info[i].pose = pose;
		nodes_info[i].data_valid = TRUE;
	}
}

int upgrade_vol_per_info(uint8 vol_per,uint16 did)
{
	uint8 i;
	bool get_did;

	get_did = FALSE;
	
	for(i=0;i<node_cnt;i++)
	{
		if(nodes_info[i].did == did)
		{
			get_did = TRUE;
			break;
		}
	}

	if(get_did)
	{
		nodes_info[i].vol_per = vol_per;
		return 0;
	}
	else
	{
		printf("get did %04x, but not know did match id value\n",did);
		return -1;
	}
}

bool get_id_value(uint16 did,uint8 *dst_ids)
{
	uint8 i;
	bool get_did;

	get_did = FALSE;
	
	for(i=0;i<node_cnt;i++)
	{
		if(nodes_info[i].did == did)
		{
			get_did = TRUE;
			break;
		}
	}

	if(get_did)
	{
		*dst_ids++ = nodes_info[i].ids[0];
		*dst_ids++ = nodes_info[i].ids[1];
		*dst_ids++ = nodes_info[i].ids[2];
		*dst_ids++ = nodes_info[i].ids[3];
		*dst_ids++ = nodes_info[i].ids[4];
	}

	return get_did;

}

int is_exist_light(uint16 *did)
{
	int i;
	for(i=0;i<node_cnt;i++)
	{
		if(nodes_info[i].types == 1)
		{
			*did = nodes_info[i].did;
			return 0;
		}
	}
	return -1;
}
void parse_uart_data(void)
{
	uint16 len;
	bool find_head;

	get_key_frame = FALSE;
	
	len = BQGetDataSize();
	
	if( len >= DEFINE_FRAME_SIZE)
	{
		BQPeekBytes(buf_frame,DEFINE_FRAME_SIZE);

		find_head = find_sync_head(buf_frame,DEFINE_FRAME_SIZE);

		if(!find_head)
		{
			BQPopBytes(buf_frame,DEFINE_FRAME_SIZE - 2);
			return;
		}

		if(skip_nbyte_find_head > 0)
		{
			BQPopBytes(buf_frame,skip_nbyte_find_head);
			return;
		}

		if((buf_frame[13] != 0xA5 || buf_frame[14] != 0xA6 || buf_frame[15] != 0xA7))//frame tail check
		{
			BQCommitLastPeek();
			return;
		}

		cmd_type = buf_frame[3];
		
		if(cmd_type == 0)//bypass to phone
		{
			memcpy(key_frame,&buf_frame[4],6);
		}
		else
		{
			printf("ble i2c read key failed\r\n");
		}

		adc = buf_frame[10] << 8 | buf_frame[11];
		ppr = buf_frame[12] >> 2;
		chg = (buf_frame[12] & 0x02) >> 1;
		status = buf_frame[12] & 0x01;
		if(print_log_cnt++ > 10)
		{
			print_log_cnt = 0;
			printf("ppr=%d,chg=%d,adc=%dmv,%s\r\n",ppr,chg,adc,status?"charge on":"charge off");
			printf("%02x,%02x,%02x,%02x,%02x,%02x,\r\n",key_frame[0],key_frame[1],key_frame[2],key_frame[3],key_frame[4],key_frame[5]);
			//printf("cur pose %s\r\n",p[last_index]);
		}

		BQCommitLastPeek();

		get_key_frame = TRUE;
		
	}

}

#define MAX_GRID_COLUMN 6
#define MAX_GRID_ROW 8

unsigned char map[MAX_GRID_ROW][MAX_GRID_COLUMN] = {
	{22,	14,	1,	41,	33,	30},
	{21,	13,	2,	42,	34,	29},
	{20,	12,	3,	43,	35,	28},
	{19,	11,	4,	44,	36,	27},
	{18,	10,	5,	45,	37,	26},
	{17,	9,	6,	46,	38, 25},
	{16,	8,	7,	47,	39,	24},
	{0,	0,	0,	0,	0,	0}//noused
};

int grid[MAX_GRID_ROW][MAX_GRID_COLUMN];

unsigned char sum_row(unsigned char row)
{
	unsigned char i;
	unsigned char sum = 0;
	for (i = 0; i < MAX_GRID_COLUMN; i++)
	{
		sum += grid[row][i];
	}
	return sum;
}
unsigned char sum_col(unsigned char col)
{
	unsigned char i;
	unsigned char sum = 0;
	for (i = 0; i < MAX_GRID_ROW-1; i++)
	{
		sum += grid[i][col];
	}
	return sum;
}

unsigned char left_right_match(unsigned char col1, unsigned char col2)
{
	unsigned char i;
	unsigned char sum = 0;
	for (i = 0; i < MAX_GRID_ROW - 1; i++)
	{
		if (grid[i][col1] == grid[i][col2])
		{
			sum++;
		}
	}

	if (sum > 4)
		return TRUE;
	return FALSE;
}

unsigned char forward_back_match(unsigned char row1, unsigned char row2)
{
	unsigned char i;
	unsigned char sum = 0;
	for (i = 0; i < MAX_GRID_COLUMN; i++)
	{
		if (grid[row1][i] == grid[row2][i])
		{
			sum++;
		}
	}

	if (sum > 3)
		return TRUE;
	return FALSE;
}

typedef enum
{
	NOBODY = 0x01,
	NORMAL = 0x02,
	LEFT   = 0x04,
	RIGHT  = 0x08,
	FORWARD= 0x10
}POSE_MODE;


POSE_MODE is_normal_pose(void)
{
	unsigned char a1, a2, a3, a4;
	a1 = 0; a2 = 0; a3 = 0; a4 = 0;
	a1 += grid[0][0];
	a1 += grid[0][1];
	a1 += grid[0][2];
	a1 += grid[1][0];
	a1 += grid[1][1];
	a1 += grid[2][0];

	a2 += grid[0][3];
	a2 += grid[0][4];
	a2 += grid[0][5];
	a2 += grid[1][4];
	a2 += grid[1][5];
	a2 += grid[2][5];

	a3 += grid[6][0];
	a3 += grid[6][1];
	a3 += grid[6][2];
	a3 += grid[5][0];
	a3 += grid[5][1];
	a3 += grid[4][0];

	a4 += grid[6][3];
	a4 += grid[6][4];
	a4 += grid[6][5];
	a4 += grid[5][4];
	a4 += grid[5][5];
	a4 += grid[4][5];

	if (a1 > 0 && a2 > 0 && a3 > 0 && a4 > 0)
	{
		return NORMAL;
	}

	return NOBODY;
}

void gen_grid(void)
{
	int Count = 0;
	int RowNo = 0;
	int ColumnNo = 0;

	memset(grid, 0, sizeof(grid));

	//printf("\n");

	for ( RowNo = 0;RowNo < MAX_GRID_ROW; RowNo++)
	{
		for (ColumnNo = 0 ;ColumnNo < MAX_GRID_COLUMN; ColumnNo++)
		{
			Count = map[RowNo][ColumnNo];
			if (((key_frame[Count / 8]) & (0x01 << (7 - Count % 8))))
			{
				grid[RowNo][ColumnNo] = 0;
				//printf("*");
			}
			else
			{
				grid[RowNo][ColumnNo] = 1;
				//printf("+");
			}
		}
		//printf("\n");
	}

	printf("\n");

}
static void print_debug(A_UINT8 *data, A_UINT16 len)
{
    int i;

    for(i=0;i<len;i++)
    {
        printf("0x%02x,",data[i]);
    }
    printf("\r\n");
}

int pose_judeg(void)
{
	int pose = 0;
	int sum = 0;
	int cols[6];
	int rows[7];
	int RowNo,ColumnNo;
	for ( RowNo = 0; RowNo < MAX_GRID_ROW - 1; RowNo++)
	{
		for ( ColumnNo = 0; ColumnNo < MAX_GRID_COLUMN; ColumnNo++)
		{
			sum += grid[RowNo][ColumnNo];

		}
	}

	if (sum < 5)
	{
		pose |= NOBODY;
		return pose;
	}

	int ls, rs, fs, bs;
	ls = 0;
	rs = 0;
	fs = 0;
	bs = 0;

	ls += sum_col(0);
	ls += sum_col(1);
	ls += sum_col(2);

	rs += sum_col(3);
	rs += sum_col(4);
	rs += sum_col(5);
	cols[0] = sum_col(0);
	cols[1] = sum_col(1);
	cols[2] = sum_col(2);
	cols[3] = sum_col(3);
	cols[4] = sum_col(4);
	cols[5] = sum_col(5);

	fs += sum_row(0);
	fs += sum_row(1);
	fs += sum_row(2);
	fs += sum_row(3);
	fs += sum_row(4);
	//bs += sum_row(3);
	bs += sum_row(5);
	bs += sum_row(6);
	rows[0] = sum_row(0);
	rows[1] = sum_row(1);
	rows[2] = sum_row(2);
	rows[3] = sum_row(3);
	rows[4] = sum_row(4);
	rows[5] = sum_row(5);
	rows[6] = sum_row(6);

	unsigned char lf1, lf2, lf3;
	lf1 = left_right_match(0, 5);
	lf2 = left_right_match(1, 4);
	lf3 = left_right_match(2, 3);

	unsigned char fb1, fb2, fb3;
	fb1 = forward_back_match(0, 6);
	fb2 = forward_back_match(1, 5);
	fb3 = forward_back_match(2, 4);

#define left_right_threshold 1
	int cnt = 0;
	if (cols[0] > cols[5] + left_right_threshold)
		cnt++;
	if (cols[1] > cols[4] + left_right_threshold)
		cnt++;
	if (cols[2] > cols[3] + left_right_threshold)
		cnt++;

	if (cnt >= 2)
	{
		pose =(pose | LEFT);
	}

	cnt = 0;
	if (cols[0] + left_right_threshold < cols[5] )
		cnt++;
	if (cols[1] + left_right_threshold < cols[4])
		cnt++;
	if (cols[2] + left_right_threshold < cols[3])
		cnt++;

	if (cnt >= 2)
	{
		pose = (pose | RIGHT);
	}

#define  forward_back_threshold 1
	cnt = 0;
	if (rows[0] > rows[5] + forward_back_threshold)
		cnt++;
	if (rows[1] > rows[4] + forward_back_threshold)
		cnt++;
	if (rows[2] > rows[3] + forward_back_threshold)
		cnt++;

	if (cnt >= 2)
	{
		pose = (pose | FORWARD);
	}


	/*if (lf1 && lf2 && lf3)
	{
		pose = (pose | NORMAL);
	}*/

	if (is_normal_pose() == NORMAL)
	{
		pose = (pose | NORMAL);
		//return pose;
	}

	if (fs > bs && rows[5] < 2 && rows[6] < 2)
	{
		pose = (pose | FORWARD);
		//pose = FORWARD;
		//return pose;
	}

	/*unsigned char threshold = (float)sum * (0.2);
	if (threshold > 7)
	{
		threshold = 7;
	}

	if (ls > rs + threshold)
	{
		pose = (pose | LEFT);
		//pose = LEFT;
		//return pose;
	}

	if (ls + threshold < rs)
	{
		pose = (pose | RIGHT);
		//pose = RIGHT;
		//return pose;
	}*/

	/*if (fs > bs + threshold)
	{
		pose = FORWARD;
		return pose;
	}*/

	return pose;
}
A_INT8 pose_need_send_cnt=0;
A_INT8 pose_send_cnt=0;
static qcom_timer_t nobody_timeout;
A_INT8 body_flag = 0;

void add_have_body(void)
{
	if(!body_flag)
	{
		qcom_timer_start(&nobody_timeout);
	}
	body_flag = 1;
}

void remove_body(void)
{
	if(body_flag)
	{
		qcom_timer_stop(&nobody_timeout);
	}
	body_flag = 0;

}
void update_chair_status(void);

int cmd_update_chair_status(int argc, char * argv[])
{
	body_flag = atoi(argv[1]);
	update_chair_status();
	printf("send chair_status %d\n",body_flag);
}

void update_chair_status(void)
{
	int lret;
	uint16 light_did;
	lret = is_exist_light(&light_did);
	if(lret >= 0)
	{
		send_chair_state_cmd(body_flag,light_did);
	}
	else
	{
		printf("not exist light device\n");
	}
}

void deci_pose(void)
{
	int val;
		val = pose_judeg();
		A_UINT8 index = 0;
		if (val & NOBODY)
		{
			index = 0;
			remove_body();
			//goto disp;
		}
		else if (val & NORMAL)
		{
			index = 1;
			add_have_body();
		}
		else
		{
			int a, b, c;
			a = 0; b = 0; c = 0;
			if ((val & FORWARD))
				a = 1;
			if (val & LEFT)
				b = 1;
			if (val & RIGHT)
				c = 1;
	
			if (a && b)
				index = 5;
			else if (a && c)
				index = 6;
			else
			{
				if (a)
					index = 2;
				else if (b)
					index = 3;
				else if (c)
					index = 4;
				else
					index = 1;
			}

			if(index == 5)
			{
				index = 3;
			}
			else if(index == 6)
			{
				index = 4;
			}
			add_have_body();

		}

		//if(last_index != index)
		//{
			//if(last_index > 8)
			//	last_index = 0;
			uint8 tmp_ids[ID_LEN];
			if(get_id_value(cur_did,tmp_ids))
			{
				printf("id:%02x%02x%02x%02x%02x---> pose %s\r\n",tmp_ids[0],tmp_ids[1],tmp_ids[2],tmp_ids[3],tmp_ids[4],p[index]);
			}
			else
			{
				printf("not get id, %04x,pose %s\r\n",cur_did,p[index]);
			}

			update_chair_status();
			
			//last_index = index;
			//if(get_recv_id)
			//{
			//	pose_need_send_cnt++;
			//	add_pose(index);
			//}

			upgrade_pose_info(index,cur_did);
			
		//}

}

A_UINT32 one_frame_len;
A_CHAR one_frame_data[64];
A_UINT8 all_data[512];
static unsigned char HexToAsc(unsigned char aChar){
    if((aChar>=0x30)&&(aChar<=0x39))
        aChar -= 0x30;
    else if((aChar>=0x41)&&(aChar<=0x46))
        aChar -= 0x37;
    else if((aChar>=0x61)&&(aChar<=0x66))
        aChar -= 0x57;
    else aChar = 0xff;
    return aChar; 
}

A_UINT32 check_gw_uart_head_tail(A_UINT8 *pbuf,A_UINT32 len)
{
	A_UINT32 i,start_pos;
	int state;
	state = 0;
	for(i=0;i<len;i++)
	{
		if(state == 0)
		{
			if(pbuf[i] == '+')
			{
				state = 1;
				start_pos = i;
			}
		}
		else if(state == 1)
		{
			if(pbuf[i] == '-')
			{
				state = 2;
				break;
			}
		}
	}

	if(state != 2)
	{
		return 0;
	}

	one_frame_len = i - start_pos + 1;
	memset(one_frame_data,0,64);
	memcpy(one_frame_data,pbuf+start_pos,one_frame_len);
	printf("frame data : %s, len = %d\r\n",one_frame_data,one_frame_len);
	return i + 1;
}

A_CHAR is_get_one_frame(void)
{
	uint16 len;
	A_UINT32 pos;
	
	len = BQGetDataSize();
	
	if( len >= 13)
	{
		
		BQPeekBytes(all_data,len);

		pos = check_gw_uart_head_tail(all_data,len);

		if(!pos)
		{
			if(len > 50)
			{
				BQPopBytes(all_data,len - 1);
			}
			return 0;
		}
		else
		{
			BQPopBytes(all_data,pos);
			return pos;
		}
		
	}

	return 0;

}
int mv,sv;
int get_v_flag = 0;
enum DEV_TYPE dev_type = UNDEF;

enum DEV_TYPE get_dev_type(void)
{
	return dev_type;
}
int query_version_flag(void)
{
	return get_v_flag;
}

void clear_version_flag(void)
{
	get_v_flag = FALSE;
}
uint16 get_main_ver(void)
{
	return mv;
}
uint16 get_second_ver(void)
{
	return sv;
}
void parse_gw_uart_frame(void)
{
	//char *string, *stopstring;
	uint16_t did;
	uint8_t ch,i,j,crc,type;

	get_key_frame = FALSE;
	
	if(one_frame_len == 22)//+0280ffffffffffff64e6-
	{
		did = 0;
		for(i=1;i<5;i++)
		{
			did <<= 4;
			did |= HexToAsc(one_frame_data[i]);
		}

		cur_did = did;
		
		//big_little(&did);

		for(j=0;j<6;j++)
		{
			ch = 0;
			for(i=5+j*2;i<7+j*2;i++)
			{
				ch <<= 4;
				ch |= HexToAsc(one_frame_data[i]);
			}
			key_frame[j] = ch;
		}

		type=0;
		for(i=17;i<19;i++)
		{
			type <<= 4;
			type |= HexToAsc(one_frame_data[i]);
		}
		
		crc = 0;
		for(i=19;i<21;i++)
		{
			crc <<= 4;
			crc |= HexToAsc(one_frame_data[i]);
		}

		//printf("id=%4x,key=%2x,key=%2x,key=%2x,key=%2x,key=%2x,key=%2x,type=%2x,crc=%2x\n",did,key_frame[0],key_frame[1],key_frame[2],key_frame[3],key_frame[4],key_frame[5],type,crc);

		if(type == 0x88 || type == 0xf8)
		{
			//add_chair_id(key_frame);
			if(!is_exist_id(key_frame))
			{
				add_node(key_frame,did,100,type == 0x88 ? 0 : 1);
			}
			else//chair power off, when join net, malloc new did,so upgrade did info
			{
				upgrade_node(cur_pos,did);
			}
		}
		else if(type <= 100)
		{
			//add_key_val(key_frame);
			//add_vol(type);
			int ret;
			ret = upgrade_vol_per_info(type,did);
			if(ret >= 0)
			{
				get_key_frame = TRUE;
			}
		}
		else
		{
			printf("recv ble type error %d\n",type);
		}
	}
	else if(one_frame_len == 25)//+New device 0x8002 added-
	{
		for(i=13;i<17;i++)
		{
			did <<= 4;
			did |= HexToAsc(one_frame_data[i]);
		}
		//printf("add device , addr = 0x%4x\r\n",did);
	}
	else if(one_frame_len == 13)//+HB0006@0001-
	{
		//printf("csr heart info %s\r\n",one_frame_data);
		char * ptr = (char*)strstr((const char*)one_frame_data,"+GW");
		
		if(ptr != NULL)
		{
			if(sscanf(ptr,\
					(const char*)"+GW%d@%d-",\
						&mv,&sv) == 2)
				{
					get_v_flag = TRUE;
					printf(">>>>> GW ble version %d.%d\r\n",mv,sv);
					dev_type = C_GW;
				}	
			return;
		}

		ptr = (char*)strstr((const char*)one_frame_data,"+ND");
		
		if(ptr != NULL)
		{
			if(sscanf(ptr,\
					(const char*)"+ND%d@%d-",\
						&mv,&sv) == 2)
				{
					get_v_flag = TRUE;
					printf(">>>>> NODE ble version %d.%d\r\n",mv,sv);
					dev_type = C_NODE;
				}	
			//continue;
		}


	}
	else
	{
		printf("csr data len error\r\n");
	}
}

#define need_read_len 1024
A_INT32 chair_uart_fd = -1;
static int uart_fd_lock = 0;
 

void   smart_chair_node_uart_read_task()
{
    A_UINT32 uart_length = need_read_len;
    A_CHAR uart_buff[need_read_len];
	A_UINT32 buf_len;

    q_fd_set fd;
    struct timeval tmo;
    int ret;
    qcom_uart_para com_uart_cfg;
	
    com_uart_cfg.BaudRate=     2400; /* 1Mbits */
    com_uart_cfg.number=       8;
    com_uart_cfg.StopBits =    1;
    com_uart_cfg.parity =      0;
    com_uart_cfg.FlowControl = 0;

   qcom_single_uart_init((A_CHAR *)"UART1");

    chair_uart_fd = qcom_uart_open((A_CHAR *)"UART1");
    if (chair_uart_fd == -1) {
        A_PRINTF("qcom_uart_open uart1 failed...\r\n");
        return;
    }

    qcom_set_uart_config((A_CHAR *)"UART1",&com_uart_cfg);

   printf("\r\nenter chair uart data thread \r\n");

   buf_len = 0;
    while (1)
    {
        FD_ZERO(&fd);
        FD_SET(chair_uart_fd, &fd);
        tmo.tv_sec = 30;
        tmo.tv_usec = 0;

		memset(uart_buff, 0x00,  sizeof(uart_buff));
        ret = qcom_select(2, &fd, NULL, NULL, &tmo);
        if (ret == 0) 
		{
            //A_PRINTF("UART receive timeout\n");
        } 
		else 
		{
            if(FD_ISSET(chair_uart_fd, &fd)) 
			{
		  		uart_length = need_read_len;
                qcom_uart_read(chair_uart_fd, uart_buff, &uart_length);
                if (uart_length) 
				{
					BQSafeQueueBytes((const uint8 *)uart_buff, uart_length);
					//printf("%d\n",uart_length);
                }

				parse_uart_data();

				if(get_key_frame)
				{
					gen_grid();
					deci_pose();
				}
            }
			else
			{
             A_PRINTF("UART something is error!\n");           		
        	}
        }
    }
}
#define CMD_LEN 8

A_CHAR ver_cmds[CMD_LEN] = {0x11,0x12,0x01,0x66,0x66,0x66,0x13,0x14};
A_CHAR chair_state_cmds[CMD_LEN] = {0x11,0x12,0x02,0x66,0x66,0x66,0x13,0x14};

void send_chair_state_cmd(A_INT8 val,uint16 did)
{
	A_UINT32 len;
	//A_INT32 ret;

	chair_state_cmds[2] = (val == 0) ? 0x03 : 0x02;
	chair_state_cmds[3] = did >> 8;
	chair_state_cmds[4] = did &0x00ff;

	if(chair_uart_fd < 0)
		return;
	len = CMD_LEN;

	//if(uart_fd_lock)
	//	return;

	uart_fd_lock = 1;
	
	TSK_SEM_LOCK;
	qcom_uart_write(chair_uart_fd, chair_state_cmds, &len);

	uart_fd_lock = 0;

	TSK_SEM_LOCK;

	int i;
	for(i=0;i<CMD_LEN;i++)
		printf("%02x ",chair_state_cmds[i]);
	printf("\n");
	
	if(len != CMD_LEN)
	{
		printf("send_chair_state_cmd error,len=%d\r\n",len);
	}
}

void send_query_version_cmd(void)
{
	A_UINT32 len;
	//A_INT32 ret;

	if(chair_uart_fd < 0)
		return;
	len = CMD_LEN;

	//if(uart_fd_lock)
	//	return;

	uart_fd_lock = 1;

	TSK_SEM_LOCK;
	
	qcom_uart_write(chair_uart_fd, ver_cmds, &len);

	uart_fd_lock = 0;
	
	TSK_SEM_UNLOCK;
	
	if(len != CMD_LEN)
	{
		printf("send_query_version_cmd error,len=%d\r\n",len);
	}
}

void nobody_timeout_callback(unsigned int argc, void *argv)
{
	up_desk();
	qcom_thread_msleep(200);
	down_desk();
	qcom_thread_msleep(200);
}

void   smart_chair_gateway_uart_read_task()
{
    A_UINT32 uart_length = need_read_len;
    A_CHAR uart_buff[need_read_len];
	A_UINT32 buf_len;
	
    q_fd_set fd;
    struct timeval tmo;
    int ret;
    qcom_uart_para com_uart_cfg;
	
    com_uart_cfg.BaudRate=     115200; /* 1Mbits */
    com_uart_cfg.number=       8;
    com_uart_cfg.StopBits =    1;
    com_uart_cfg.parity =      0;
    com_uart_cfg.FlowControl = 0;

   qcom_single_uart_init((A_CHAR *)"UART1");

    chair_uart_fd = qcom_uart_open((A_CHAR *)"UART1");
    if (chair_uart_fd == -1) {
        A_PRINTF("qcom_uart_open uart1 failed...\r\n");
        return;
    }

    qcom_set_uart_config((A_CHAR *)"UART1",&com_uart_cfg);

   printf("\r\nenter chair uart data thread \r\n");

   qcom_timer_init(&nobody_timeout,nobody_timeout_callback,NULL,60*60*2,ONESHOT);

   uart_fd_lock = 0;

   TSK_SEM_INIT;

   /*if (QURT_EOK != qurt_mutex_create(&uart_fd_lock))
   {
	   SWAT_PTF("uart_fd_lock_mutex_create FAIL!\n");
   }*/

   buf_len = 0;
    while (1)
    {
        FD_ZERO(&fd);
        FD_SET(chair_uart_fd, &fd);
        tmo.tv_sec = 30;
        tmo.tv_usec = 0;

		memset(uart_buff, 0x00,  sizeof(uart_buff));
        ret = qcom_select(2, &fd, NULL, NULL, &tmo);
        if (ret == 0) 
		{
            //A_PRINTF("UART receive timeout\n");
        } 
		else 
		{
            if(FD_ISSET(chair_uart_fd, &fd)) 
			{
		  		uart_length = need_read_len;
                qcom_uart_read(chair_uart_fd, uart_buff, &uart_length);
                if (uart_length) 
				{
					BQSafeQueueBytes((const uint8 *)uart_buff, uart_length);
					//printf("%d\n",uart_length);
                }

				if(is_get_one_frame())
				{
					parse_gw_uart_frame();
					
					if(get_key_frame)
					{
						gen_grid();
						deci_pose();
					}
				}

            }
			else
			{
             A_PRINTF("UART something is error!\n");           		
        	}
        }
    }
}

A_CHAR socket_chair_tx_buffer[] = { \
	
	0xc0,//head
	
	0x06,//device id 0615010043
	0x15,
	0x01,
	0x00,
	0x43,
	
	0x02,//send id
	0x01,
	0x00,
	0x00,
	0x01,
	
	0x90,//device type : smartchair
	
	0xa3,//send cmd

	
	0x16,//data len == 22

	0x06,//data //id
	0x15,
	0x01,
	0x00,
	0x43,
	
	0x00,//No. 1
	0x00,
	
	0x00,//No.2
	0x00,
	
	0x00,//No. 3
	0x00,
	
	0x00,//No. 4
	0x00,
	
	0x00,//No. 5
	0x00,
	
	0x00,//No. 6
	0x00,
	
	0x00,//voltage
	0x21,
	
	0x00,//pose
	
	0x00,//serial no
	0x00,
	
	0x00,//CRC
	0x00,
	
	0xC1,//sync tail
};

void add_chair_id(uint8 *pid)
{
	socket_chair_tx_buffer[1] = pid[0];
	socket_chair_tx_buffer[2] = pid[1];
	socket_chair_tx_buffer[3] = pid[2];
	socket_chair_tx_buffer[4] = pid[3];
	socket_chair_tx_buffer[5] = pid[4];

	socket_chair_tx_buffer[14] = pid[0];
	socket_chair_tx_buffer[15] = pid[1];
	socket_chair_tx_buffer[16] = pid[2];
	socket_chair_tx_buffer[17] = pid[3];
	socket_chair_tx_buffer[18] = pid[4];

	get_recv_id = TRUE;
}

void add_key_val(uint8 *pkey)
{
	socket_chair_tx_buffer[25] = pkey[0];
	socket_chair_tx_buffer[26] = pkey[1];
	socket_chair_tx_buffer[27] = pkey[2];
	socket_chair_tx_buffer[28] = pkey[3];
	socket_chair_tx_buffer[29] = pkey[4];
	socket_chair_tx_buffer[30] = pkey[5];

}
void add_vol(uint8 VOL)
{
	socket_chair_tx_buffer[31] = VOL;
}
void add_pose(A_UINT8 pose)
{
	socket_chair_tx_buffer[sizeof(socket_chair_tx_buffer)-6] = pose;
}

void socket_send_schair_data_task()
{
	A_INT32 socketLocal;
	struct sockaddr_in remoteAddr;
	int ret;
    uint32_t ip1;
    uint32_t mask;
    uint32_t gateway;
	A_UINT32 s_addr;
	A_UINT8 dhcp_ok = FALSE;
	A_UINT8 get_ip_ok= FALSE;
	A_CHAR *dns = "wx.hehedesk.com";
	uint8 send_node_cnt;

	swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
	remoteAddr.sin_port = htons(3338);
	remoteAddr.sin_family = AF_INET;
	socketLocal = 0;
	s_addr = 0;
	send_node_cnt = 0;

	printf("\n enter smart chair send thread\r\n");
	
	while(1)
	{
		if(get_dev_type() != C_GW)
		{
			qcom_thread_msleep(1000);
			continue;
		}
		
		if(!dhcp_ok)//check router malloc ip addr to wifi module
		{
			ret = qcom_ip_address_get(0, &ip1, &mask, &gateway);
			if(ret != A_OK || ip1 == 0 || mask == 0 || gateway == 0)
			{
				qcom_thread_msleep(500);
				continue;
			}
			dhcp_ok = TRUE;
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
		
		/*if(!get_recv_id)
		{
			qcom_thread_msleep(500);
			continue;
		}*/

		/*if(pose_need_send_cnt == pose_send_cnt)
		{
			qcom_thread_msleep(500);
			continue;
		}*/

		if(node_cnt == 0)
		{
			qcom_thread_msleep(500);
			continue;
		}

		if(nodes_info[send_node_cnt].data_valid)
		{
			add_chair_id(nodes_info[send_node_cnt].ids);
			add_pose(nodes_info[send_node_cnt].pose);
			add_vol(nodes_info[send_node_cnt].vol_per);
		}
		else
		{
			goto next_node;
		}

		socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
		if(socketLocal == A_ERROR)
		{
			printf("thread create socket failed\r\n");
			continue;
		}

		ret = swat_connect(socketLocal, (struct sockaddr *) &remoteAddr,
						 sizeof (struct  sockaddr_in));
		
		if (ret < 0) 
		{
			/* Close Socket */
			printf("Connect Failed\r\n");
			qcom_socket_close(socketLocal);
			qcom_thread_msleep(1000);
			continue;
		}
		
		/*static A_CHAR pose=0;
		add_pose(pose);
		pose++;
		if(pose > 6)
			pose = 0;*/
		
		A_INT32 send_len = qcom_send(socketLocal, socket_chair_tx_buffer, sizeof(socket_chair_tx_buffer), 0);
		if(send_len < 0 || send_len != sizeof(socket_chair_tx_buffer))
		{
			printf("send error %d\r\n",send_len);
		}
		printf("chair data:");
		print_debug((uint8*)socket_chair_tx_buffer, sizeof(socket_chair_tx_buffer));
		
		qcom_socket_close(socketLocal);

next_node:
		//pose_send_cnt++;
		send_node_cnt++;

		if(send_node_cnt >= node_cnt)
		{
			send_node_cnt = 0;
			qcom_thread_msleep(1000);
		}
		
	}
	
	qcom_task_exit();
	
}

A_INT32 start_smart_chair_node_uart_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(smart_chair_node_uart_read_task, 2, 2048, 80);
}

A_INT32 start_smart_chair_socket_tx_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(socket_send_schair_data_task, 2, 2048, 80);
}

A_INT32 start_smart_chair_gw_uart_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(smart_chair_gateway_uart_read_task, 2, 2048, 80);
}

