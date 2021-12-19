#ifndef _SOCKET_SERVER_H_
#define _SOCKET_SERVER_H_

#include <netinet/in.h>

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "ringbuffer.h"
#include "protocol.h"
#ifdef __cplusplus
}
#endif

#define MAX_CLIENT_NUM 				10
#define MAX_LISTEN_NUM 				5
#define SVR_RECVBUF_SIZE			(PROTO_PACK_MAX_LEN*6)
#define SVR_SENDBUF_SIZE			PROTO_PACK_MAX_LEN

typedef enum
{
	STATE_CLOSE,
	STATE_DISCONNECT,
	STATE_CONNECTED,
}sockState_e;


struct clientInfo
{
	int fd;
	sockState_e state;
	int protoHandle;
	pthread_mutex_t	send_mutex;
	struct sockaddr_in addr;
	struct ringbuffer recvRingBuf;			// socket receive data ring buffer
	struct detect_info detectInfo;
	uint8_t packBuf[PROTO_PACK_MAX_LEN];		// protocol packet data buffer
	int packLen;
	uint8_t ack_buf[PROTO_PACK_MAX_LEN];
	uint8_t tmpBuf[PROTO_PACK_MAX_LEN];
};

struct serverInfo
{
	int fd;
	int client_cnt;
	struct sockaddr_in 	srv_addr;		// server ip addr
	struct clientInfo	client[MAX_CLIENT_NUM];
	uint8_t client_used[MAX_CLIENT_NUM];
};


int server_sendData(void *arg, uint8_t *data, int len);
int socket_get_handle_list(int *list_buf, int buf_size, int *num);
int start_socket_server_task(void);


#endif	// _SOCKET_SERVER_H_
