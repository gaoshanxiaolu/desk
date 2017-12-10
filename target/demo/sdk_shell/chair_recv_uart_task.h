

A_INT32 start_smart_chair_uart_app(A_INT32 argc, A_CHAR *argv[]);
A_INT32 start_smart_chair_socket_tx_app(A_INT32 argc, A_CHAR *argv[]);
A_INT32 start_smart_chair_gw_uart_app(A_INT32 argc, A_CHAR *argv[]);
int query_version_flag(void);
int get_main_ver(void);
void clear_version_flag(void);
int cmd_update_chair_status(int argc, char * argv[]);

int get_second_ver(void);
enum DEV_TYPE get_dev_type(void);
void send_query_version_cmd(void);

