#define DESC_READ_RESULT_OK 0
#define DESC_READ_RESULT_OVERFLOW -1
#define DESC_READ_RESULT_EOF -2
#define DESC_READ_RESULT_IOERROR -4


typedef void new_cnxn_handler(int control, int ipaddress, const char *hostname);

int remote_listen(int port);
void remote_deafen(int control);
void remote_poll(int control, new_cnxn_handler *on_new_connection);

void remote_disconnect(int descriptor);
int remote_read(int descriptor, int max, char inbuf[]);
bool remote_write(int descriptor, char *txt, int length);

