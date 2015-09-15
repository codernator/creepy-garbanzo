#define DESC_READ_RESULT_OK 0
#define DESC_READ_RESULT_OVERFLOW -1
#define DESC_READ_RESULT_EOF -2
#define DESC_READ_RESULT_IOERROR -4



typedef void handle_new_connection(int control, int ipaddress, const char *hostname);
typedef void handle_drop_connection(DESCRIPTOR_DATA *d);

void init_descriptor(int control, handle_new_connection *);
void disconnect(int descriptor);
int read_from_descriptor(int descriptor, int max, char inbuf[]);
bool write_to_descriptor(int descriptor, char *txt, int length);
int listen_on_port(int port);
void deafen_port(int listen_control);

