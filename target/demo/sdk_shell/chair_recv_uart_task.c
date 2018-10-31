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

#define DEFINE_FRAME_SIZE (DEFINE_FRAME_LEN+2)
//char *p[] = { "没人","正常","左倾","右倾","前倾","左前倾","右前倾" };
char *p[] = { "no_body","normal","forward","left","right","left_forward","right_forward" };
A_UINT8 last_index = 10;

uint8 skip_nbyte_find_head;
static uint8 buf_frame[32];
uint8 cmd_type;
static uint8 key_frame[32];
int mv,sv;
int get_v_flag = 0;
enum DEV_TYPE dev_type = UNDEF;

bool find_sync_head(uint8 *data, uint8 len)
{
	uint8 i;

	for(i=0;i<len-1;i++)
	{
		if(data[i] == 0x1b && data[i+1] == 0x2a)
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
		cur_pos = 0;
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

	cur_pos = node_cnt;
	node_cnt++;

}

void upgrade_node(uint8 pos,uint16 did)
{
	nodes_info[pos].vol_per = did;

	//printf("->upgrade volper, volper:%04x\n",did);

}

void upgrade_pose_info(uint8 pose,uint16 did)
{
	/*uint8 i;
	bool get_did;

	get_did = FALSE;
	
	for(i=0;i<node_cnt;i++)
	{
		if(nodes_info[i].did == did)
		{
			get_did = TRUE;
			break;
		}
	}*/

	//if(get_did)
	{
		nodes_info[cur_pos].pose = pose;
		nodes_info[cur_pos].data_valid = TRUE;
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
static  uint16 ccitt_table[256] = {
0x0000, 0x1021, 0x2042, 0x3063, 0x4084, 0x50A5, 0x60C6, 0x70E7,
0x8108, 0x9129, 0xA14A, 0xB16B, 0xC18C, 0xD1AD, 0xE1CE, 0xF1EF,
0x1231, 0x0210, 0x3273, 0x2252, 0x52B5, 0x4294, 0x72F7, 0x62D6,
0x9339, 0x8318, 0xB37B, 0xA35A, 0xD3BD, 0xC39C, 0xF3FF, 0xE3DE,
0x2462, 0x3443, 0x0420, 0x1401, 0x64E6, 0x74C7, 0x44A4, 0x5485,
0xA56A, 0xB54B, 0x8528, 0x9509, 0xE5EE, 0xF5CF, 0xC5AC, 0xD58D,
0x3653, 0x2672, 0x1611, 0x0630, 0x76D7, 0x66F6, 0x5695, 0x46B4,
0xB75B, 0xA77A, 0x9719, 0x8738, 0xF7DF, 0xE7FE, 0xD79D, 0xC7BC,
0x48C4, 0x58E5, 0x6886, 0x78A7, 0x0840, 0x1861, 0x2802, 0x3823,
0xC9CC, 0xD9ED, 0xE98E, 0xF9AF, 0x8948, 0x9969, 0xA90A, 0xB92B,
0x5AF5, 0x4AD4, 0x7AB7, 0x6A96, 0x1A71, 0x0A50, 0x3A33, 0x2A12,
0xDBFD, 0xCBDC, 0xFBBF, 0xEB9E, 0x9B79, 0x8B58, 0xBB3B, 0xAB1A,
0x6CA6, 0x7C87, 0x4CE4, 0x5CC5, 0x2C22, 0x3C03, 0x0C60, 0x1C41,
0xEDAE, 0xFD8F, 0xCDEC, 0xDDCD, 0xAD2A, 0xBD0B, 0x8D68, 0x9D49,
0x7E97, 0x6EB6, 0x5ED5, 0x4EF4, 0x3E13, 0x2E32, 0x1E51, 0x0E70,
0xFF9F, 0xEFBE, 0xDFDD, 0xCFFC, 0xBF1B, 0xAF3A, 0x9F59, 0x8F78,
0x9188, 0x81A9, 0xB1CA, 0xA1EB, 0xD10C, 0xC12D, 0xF14E, 0xE16F,
0x1080, 0x00A1, 0x30C2, 0x20E3, 0x5004, 0x4025, 0x7046, 0x6067,
0x83B9, 0x9398, 0xA3FB, 0xB3DA, 0xC33D, 0xD31C, 0xE37F, 0xF35E,
0x02B1, 0x1290, 0x22F3, 0x32D2, 0x4235, 0x5214, 0x6277, 0x7256,
0xB5EA, 0xA5CB, 0x95A8, 0x8589, 0xF56E, 0xE54F, 0xD52C, 0xC50D,
0x34E2, 0x24C3, 0x14A0, 0x0481, 0x7466, 0x6447, 0x5424, 0x4405,
0xA7DB, 0xB7FA, 0x8799, 0x97B8, 0xE75F, 0xF77E, 0xC71D, 0xD73C,
0x26D3, 0x36F2, 0x0691, 0x16B0, 0x6657, 0x7676, 0x4615, 0x5634,
0xD94C, 0xC96D, 0xF90E, 0xE92F, 0x99C8, 0x89E9, 0xB98A, 0xA9AB,
0x5844, 0x4865, 0x7806, 0x6827, 0x18C0, 0x08E1, 0x3882, 0x28A3,
0xCB7D, 0xDB5C, 0xEB3F, 0xFB1E, 0x8BF9, 0x9BD8, 0xABBB, 0xBB9A,
0x4A75, 0x5A54, 0x6A37, 0x7A16, 0x0AF1, 0x1AD0, 0x2AB3, 0x3A92,
0xFD2E, 0xED0F, 0xDD6C, 0xCD4D, 0xBDAA, 0xAD8B, 0x9DE8, 0x8DC9,
0x7C26, 0x6C07, 0x5C64, 0x4C45, 0x3CA2, 0x2C83, 0x1CE0, 0x0CC1,
0xEF1F, 0xFF3E, 0xCF5D, 0xDF7C, 0xAF9B, 0xBFBA, 0x8FD9, 0x9FF8,
0x6E17, 0x7E36, 0x4E55, 0x5E74, 0x2E93, 0x3EB2, 0x0ED1, 0x1EF0
};
uint16 crc_ccitt(uint8 *q, uint8 len);

uint16 crc_ccitt(uint8 *q, uint8 len)
{
	uint16 crc = 0;

	while (len-- > 0)
		crc = ccitt_table[(crc >> 8 ^ *q++) & 0xff] ^ (crc << 8);
	return ~crc;

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

		/*int m;
		for(m=0;m<DEFINE_FRAME_SIZE;m++)
			printf("%02x",buf_frame[m]);
		printf("\n");*/
		find_head = find_sync_head(buf_frame,DEFINE_FRAME_SIZE);

		if(!find_head)
		{
			BQPopBytes(buf_frame,DEFINE_FRAME_SIZE - 1);
			printf("skip 17\n");
			return;
		}

		if(skip_nbyte_find_head > 0)
		{
			BQPopBytes(buf_frame,skip_nbyte_find_head);
			printf("skip1 %d\n",skip_nbyte_find_head);
			return;
		}

		uint16 sum = 0;
		//for(i=0;i<size-4;i++)
		//	sum += objdata[i+2];
		sum = crc_ccitt(buf_frame+2,DEFINE_FRAME_SIZE-4);
		
		uint16 stdsum = (buf_frame[DEFINE_FRAME_SIZE-2] << 8) | buf_frame[DEFINE_FRAME_SIZE-1];
		
		if(stdsum != sum)
		{
			printf("uart crc check error\n");
			return;
		}

		cmd_type = buf_frame[14];
		
		if(cmd_type == 0x66)
		{
			mv = buf_frame[2];
			sv = buf_frame[3];
			if(buf_frame[4] == 1)
			{
				get_v_flag = TRUE;
				printf(">>>>> GW ble version %d.%d\r\n",mv,sv);
				dev_type = C_GW;

			}
			else if(buf_frame[4] == 2)
			{
				get_v_flag = TRUE;
				printf(">>>>> NODE ble version %d.%d\r\n",mv,sv);
				dev_type = C_NODE;

			}
			else
			{
				printf("dev type error\r\n");
			}


		}
		else
		{
			get_key_frame = TRUE;
		}

		BQCommitLastPeek();

		
		
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

	//printf("\n");

}
/*static void print_debug(A_UINT8 *data, A_UINT16 len)
{
    int i;

    for(i=0;i<len;i++)
    {
        printf("0x%02x,",data[i]);
    }
    printf("\r\n");
}*/

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
//static qcom_timer_t nobody_timeout;
A_INT8 body_flag = 0;

void add_have_body(void)
{
	if(!body_flag)
	{
		//qcom_timer_start(&nobody_timeout);
		body_flag = 1;
	}
	
}

void remove_body(void)
{
	if(body_flag)
	{
		//qcom_timer_stop(&nobody_timeout);
		
		body_flag = 0;
	}
	
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

/*
1.椅子/桌子判断是否有人坐在椅子上。连续5s（或者连续5个数据包）抓到的都是无人的坐姿，进入无人状态，
如果抓到5s（或者连续5个数据包）有人的坐姿，立即进入有人状态，并且3s重新开始计数。
2.当从有人状态进入无人状态，桌子升高到预设的高度（记忆位1）。
3.如果从无人进入有人状态， 桌子降到预设高度（记忆位2）。
4.如果无人状态持续2分钟，桌子也降到预设高度（记忆位2）。
*/
//#define DEBUG2DISPLAY
int body_cnt=0,nobody_cnt=0;
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
			nobody_cnt++;
            body_cnt = 0;
		}
		else if (val & NORMAL)
		{
			index = 1;
			add_have_body();
            body_cnt++;
            nobody_cnt=0;
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
            body_cnt++;
            nobody_cnt=0;

		}
		//if(last_index != index)
		//{
			//if(last_index > 8)
			//	last_index = 0;
			//uint8 tmp_ids[ID_LEN];
			//if(get_id_value(cur_did,tmp_ids))
			//{
			//	printf("id:%02x%02x%02x%02x%02x---> pose %s\r\n",tmp_ids[0],tmp_ids[1],tmp_ids[2],tmp_ids[3],tmp_ids[4],p[index]);
			//}
			//else
			//{
			//	printf("not get id, %04x,pose %s\r\n",cur_did,p[index]);
			//}

			//update_chair_status();
			
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
#if 0
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
#endif

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
#if 0
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
#else
int parse_gw_uart_frame(void)
{

	uint16 i;
	uint8 id[5];
	uint8 vol = buf_frame[13];
    uint8 body_flag = buf_frame[14];
	uint8 seq = buf_frame[15];

    printf("--->%x %x %x\r\n",vol,body_flag,seq);
    if((vol & 0x80) == 0x80)//magic device
    {
        if(seq)//control mode
        {
            if(body_flag == 1)
            {
                if(get_desk_run_state()!= DOWN_DESK)
                {
                    down_desk();
                    printf("magic down desk");
                }
            }
            else if(body_flag == 2)
            {
                if(get_desk_run_state() != UP_DESK)
                {
                    up_desk();
                    printf("magic up desk");
                }

            }
            else
            {
                if(get_desk_run_state() != STOP)
                {
                    stop_desk();
                    printf("magic stop desk");
                }

            }
            
        }
        return 1;
    }
	for(i=0;i<6;i++)
		key_frame[i] = buf_frame[i+7];

	for(i=0;i<5;i++)
		id[i] = buf_frame[i+2];

	printf("id=%02x%02x%02x%02x%02x,keys=%2x%2x%2x%2x%2x%2x,vol=%2x,seq=%d\n",id[0],id[1],id[2],id[3],id[4],key_frame[0],key_frame[1],key_frame[2],key_frame[3],key_frame[4],key_frame[5],vol,seq);
	//add_chair_id(key_frame);
	if(!is_exist_id(id))
	{
		add_node(id,0,vol,0);
	}
	else//chair power off, when join net, malloc new did,so upgrade did info
	{
		upgrade_node(cur_pos,vol);
	}

    return 0;
}

#endif
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

    printf("send_query_version_cmd\r\n");
	
	if(len != CMD_LEN)
	{
		printf("send_query_version_cmd error,len=%d\r\n",len);
	}
}

extern int desk_is_moved;
int need_move_desk;
void nobody_timeout_callback(unsigned int argc, void *argv)
{
	body_flag = 0;
	
	need_move_desk = 1;

	
}

void   smart_chair_timeout_move_task()
{
	printf("\r\nenter smart_chair_timeout_move_task \r\n");
	
	while(1)
	{
		if(!need_move_desk)
		{
			qcom_thread_msleep(500);
			continue;
		}

		
		
		if(!desk_is_moved)
		{
			printf("nobody timeout, move desk\n");
			up_desk();
			qcom_thread_msleep(200);
			stop_desk();
			qcom_thread_msleep(200);
			down_desk();
			qcom_thread_msleep(200);
			stop_desk();
			qcom_thread_msleep(200);

		}
		else
		{
			printf("nobody timeout,  desk is moving\n");
		}

        need_move_desk = 0;


	}
}

void   smart_chair_debug2disp_task()
{
    int dstate=0,is_exist_man,cnt=0;
    int t_cnt=0,dcnt=0;
    
	printf("\r\nenter smart_chair_debug2disp_task \r\n");
    //椅子有人和没人广播的时间是不一样的。不能用包的个数来做，需要用的时间来做
	
	while(1)
	{
        printf("body %d %d %d %d",body_cnt,nobody_cnt,t_cnt,dcnt);
        if(dstate == 0)
        {
            if(body_cnt)
            {
                is_exist_man = 1;
                dstate = 1;
                printf("init body\r\n");
                t_cnt=0;
            }
            else if (nobody_cnt)
            {
                is_exist_man = 0;
                dstate = 2;
                cnt = 0;
                printf("init nobody\r\n");
                t_cnt=0;
            }
        }
        else if(dstate == 1)
        {
            if(nobody_cnt > 0)
            {
                t_cnt++;

                if(t_cnt > 4)
                {
                    t_cnt = 0;
                    //set_mx_sig_val(X2);
                    dstate = 2;
                    cnt = 0;
                    printf("body->nobody\r\n");
                }
            }

            dcnt++;
            if(dcnt > 3600)
            {
                dcnt = 0;
                need_move_desk = 1;
                printf("body 1h,MOVE DESK\r\n");
                while(1)
                {
                    if(need_move_desk)
                    {
                        qcom_thread_msleep(1000);
                        continue;
                    }
                    qcom_thread_msleep(1000*20);
                    set_mx_sig_val(X2);
                    printf("body 1h goto x2\r\n");
                    break;
                }
            }
        }
        else if(dstate == 2)//no body status
        {
            if(body_cnt > 0)
            {
                t_cnt++;
                if(t_cnt > 4)
                {
                    t_cnt = 0;
                    set_mx_sig_val(X1);
                    dstate = 1;
                    printf("nobody->body goto x1\r\n");
                    continue;
                }
            }

            cnt++;
            if(cnt > 120)//超过2分钟，清除坐下时间点
            {
                dcnt = 0;
                printf("clear timepooint\r\n");
            }

            if(cnt > 3600*5)
            {
                set_mx_sig_val(X1);
                cnt = 0;
                printf("nobody 5h,goto x1\r\n");
            }
        }
            
        qcom_thread_msleep(1000);
	}
}

#if 0
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
#else
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

   //qcom_timer_init(&nobody_timeout,nobody_timeout_callback,NULL,1000*60,ONESHOT);

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

				parse_uart_data();

				if(get_key_frame)
				{
					if(parse_gw_uart_frame())
					{
                        continue;
					}
					
					//if(get_key_frame)
					//{
						gen_grid();
						deci_pose();
					//}
				}

            }
			else
			{
             A_PRINTF("UART something is error!\n");           		
        	}
        }
    }
}

#endif
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

#if 1 //keep socket connect
static int conn_serv_ok = 0;
static A_INT32 socketLocal=0;
extern int need_move_desk;
static A_UINT8 is_need_ack = FALSE;
static A_UINT8 desk_ack_rx_msg[13] = { 
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

void print_chair_debug(A_UINT8 *data, A_UINT16 len)
{
    int i;

    for(i=0;i<len;i++)
    {
        printf("0x%02x,",data[i]);
    }
    printf("\r\n");
}

int parse_chair_rx_data(A_UINT8 *data, A_UINT16 len )
{	
	is_need_ack = FALSE;
    
	if(data[0] != 0xc0)
	{
		printf("chair frame head failed\r\n");
		print_chair_debug(data,len);
		return 7;
	}

	if(data[len - 1] != 0xc1)
	{
		printf("chair frame tail failed\r\n");
		print_chair_debug(data,len);
		return 7;
	}

	if(len != 18 && len != 13 && len != 12 && len != 23)
	{
		printf("chair frame len failed\r\n");
		print_chair_debug(data,len);
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
		//print_debug(data,len);
	}
	
	cmd = data[7];
	desk_ack_rx_msg[1]=data[1];//copy id
	desk_ack_rx_msg[2]=data[2];
	desk_ack_rx_msg[3]=data[3];
	desk_ack_rx_msg[4]=data[4];
	desk_ack_rx_msg[5]=data[5];
	
	if(cmd == 0x38)
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
		return 0;
	}
	return 7;
	
	
}

void chair_send_socket_thread()
{
	int res;
	int cnt;
	uint8 send_node_cnt;

	cnt = 0;
	SWAT_PTF("enter chair send socket\r\n");
	
	while(1)
	{
		if((conn_serv_ok==0) || (socketLocal==0))
		{
			qcom_thread_msleep(200);
			continue;
		}

		if(get_dev_type() != C_GW)
		{
			static int dev_cnt = 0;
			if(dev_cnt++ > 10)
			{
				dev_cnt = 0;
				printf("dev_cnt error \r\n");
			}

			qcom_thread_msleep(1000);
			continue;
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
			static int no_node = 0;
		static int no_node_data = 0;

		if(node_cnt == 0)
		{
			
			if(no_node++ > 10)
			{
				no_node = 0;
				printf("no node \r\n");
			}
			qcom_thread_msleep(500);
			continue;
		}
		no_node=0;

		if(nodes_info[send_node_cnt].data_valid)
		{
			nodes_info[send_node_cnt].data_valid = FALSE;
			add_chair_id(nodes_info[send_node_cnt].ids);
			add_pose(nodes_info[send_node_cnt].pose);
			add_vol(nodes_info[send_node_cnt].vol_per);
		}
		else
		{
			
			if(no_node_data++ > 10)
			{
				no_node_data = 0;
				printf("no node data\r\n");
			}

			goto next_node;
		}
		no_node_data =0;

		/*static A_CHAR pose=0;
		add_pose(pose);
		pose++;
		if(pose > 6)
			pose = 0;*/
		
		A_INT32 send_len = qcom_send(socketLocal, socket_chair_tx_buffer, sizeof(socket_chair_tx_buffer), 0);
		if(send_len < 0)
		{
			printf("chair send error res = %d\n",res);
			qcom_socket_close(socketLocal);
			socketLocal = 0;
			conn_serv_ok = 0;
			continue;
		}
		else if(send_len != sizeof(socket_chair_tx_buffer))
		{
			printf("send len error %d\r\n",send_len);
		}
		//printf("chair data:");
		//print_debug((uint8*)socket_chair_tx_buffer, sizeof(socket_chair_tx_buffer));
		
next_node:
		//pose_send_cnt++;
		send_node_cnt++;

		if(send_node_cnt >= node_cnt)
		{
			send_node_cnt = 0;
			qcom_thread_msleep(1000);
		}
	}

}

void chair_recv_socket_thread()
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
	SWAT_PTF("enter chair recv socket\r\n");
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
				printf("chiar socket create failed\r\n");
				tx_thread_sleep(1000);
				continue;

			}

			ret = swat_connect(socketLocal, (struct sockaddr *) &remoteAddr,
							 sizeof (struct  sockaddr_in));
			
			if (ret < 0) 
			{
				/* Close Socket */
				SWAT_PTF("chair Connect Failed\r\n");
				qcom_socket_close(socketLocal);
				socketLocal = 0;
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
				ret = parse_chair_rx_data(msgRecv,receive_len);
				desk_ack_rx_msg[9] = ret;
				if(is_need_ack)
				{
					//printf("ack : ");
					//print_debug(desk_ack_rx_msg,sizeof(desk_ack_rx_msg));
                    res = qcom_send(socketLocal, (char *)desk_ack_rx_msg,sizeof(desk_ack_rx_msg), 0);
                    if(res < 0)
                    {
                        printf("chair ack send error res = %d\n",res);
                        qcom_socket_close(socketLocal);
                        conn_serv_ok = 0;
                        continue;
                    }
				}
			}
            else if (receive_len < 0)
            {
                printf("chair socket_recv error\r\n");
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

#elif 0 //not keep socket connect
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
			static int dev_cnt = 0;
			if(dev_cnt++ > 10)
			{
				dev_cnt = 0;
				printf("dev_cnt error \r\n");
			}

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
			static int no_node = 0;
		static int no_node_data = 0;

		if(node_cnt == 0)
		{
			
			if(no_node++ > 10)
			{
				no_node = 0;
				printf("no node \r\n");
			}
			qcom_thread_msleep(500);
			continue;
		}
		no_node=0;

		if(nodes_info[send_node_cnt].data_valid)
		{
			nodes_info[send_node_cnt].data_valid = FALSE;
			add_chair_id(nodes_info[send_node_cnt].ids);
			add_pose(nodes_info[send_node_cnt].pose);
			add_vol(nodes_info[send_node_cnt].vol_per);
		}
		else
		{
			
			if(no_node_data++ > 10)
			{
				no_node_data = 0;
				printf("no node data\r\n");
			}

			goto next_node;
		}
		no_node_data =0;

		socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
		if(socketLocal == A_ERROR)
		{
			printf("thread create socket failed\r\n");
			qcom_thread_msleep(500);
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
		//printf("chair data:");
		//print_debug((uint8*)socket_chair_tx_buffer, sizeof(socket_chair_tx_buffer));
		
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
#else

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
	int not_connected;

	swat_mem_set(&remoteAddr, 0, sizeof (struct sockaddr_in));
	remoteAddr.sin_port = htons(3338);
	remoteAddr.sin_family = AF_INET;
	socketLocal = 0;
	s_addr = 0;
	send_node_cnt = 0;
	not_connected = 0;

	printf("\n enter smart chair send thread\r\n");
	
	while(1)
	{
		if(get_dev_type() != C_GW)
		{
			static int dev_cnt = 0;
			if(dev_cnt++ > 10)
			{
				dev_cnt = 0;
				printf("dev_cnt error \r\n");
			}

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
			static int no_node = 0;
		static int no_node_data = 0;

		if(node_cnt == 0)
		{
			
			if(no_node++ > 10)
			{
				no_node = 0;
				printf("no node \r\n");
			}
			qcom_thread_msleep(500);
			continue;
		}
		no_node=0;

		if(nodes_info[send_node_cnt].data_valid)
		{
			nodes_info[send_node_cnt].data_valid = FALSE;
			add_chair_id(nodes_info[send_node_cnt].ids);
			add_pose(nodes_info[send_node_cnt].pose);
			add_vol(nodes_info[send_node_cnt].vol_per);
		}
		else
		{
			
			if(no_node_data++ > 10)
			{
				no_node_data = 0;
				printf("no node data\r\n");
			}

			goto next_node;
		}
		no_node_data =0;

		if(!not_connected)
		{
			socketLocal = swat_socket(AF_INET, SOCK_STREAM, 0);
			if(socketLocal == A_ERROR)
			{
				printf("thread create socket failed\r\n");
				qcom_thread_msleep(500);
				continue;
			}

			ret = swat_connect(socketLocal, (struct sockaddr *) &remoteAddr,
							 sizeof (struct  sockaddr_in));
			
			if (ret < 0) 
			{
				/* Close Socket */
				printf("chair Connect Failed\r\n");
				qcom_socket_close(socketLocal);
				qcom_thread_msleep(1000);
				continue;
			}

			not_connected = 1;
		}
		
		/*static A_CHAR pose=0;
		add_pose(pose);
		pose++;
		if(pose > 6)
			pose = 0;*/
		if(not_connected)
		{
			A_INT32 send_len = qcom_send(socketLocal, socket_chair_tx_buffer, sizeof(socket_chair_tx_buffer), 0);
			if(send_len != sizeof(socket_chair_tx_buffer))
			{
				printf("chair send error %d\r\n",send_len);
			}
			else if(send_len < 0)
			{
				qcom_socket_close(socketLocal);
				qcom_thread_msleep(100);

			}
			//printf("chair data:");
			//print_debug((uint8*)socket_chair_tx_buffer, sizeof(socket_chair_tx_buffer));
			
			//qcom_socket_close(socketLocal);
		}

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

#endif
A_INT32 start_smart_chair_node_uart_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(smart_chair_node_uart_read_task, 2, 2048, 80);
}

A_INT32 start_smart_chair_socket_tx_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(chair_send_socket_thread, 2, 2048, 80);
	qcom_task_start(chair_recv_socket_thread, 2, 2048, 80);
}

A_INT32 start_smart_chair_gw_uart_app(A_INT32 argc, A_CHAR *argv[])
{
	qcom_task_start(smart_chair_gateway_uart_read_task, 2, 2048, 80);
		qcom_task_start(smart_chair_timeout_move_task, 2, 2048, 80);
        #ifdef DEBUG2DISPLAY
        qcom_task_start(smart_chair_debug2disp_task, 2, 2048, 80);
        #endif
}

