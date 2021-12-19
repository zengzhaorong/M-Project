#ifndef _SOCKET_CLIENT_H_
#define _SOCKET_CLIENT_H_

#include <netinet/in.h>
#include "type.h"

/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "ringbuffer.h"
#include "protocol.h"
#ifdef __cplusplus
}
#endif

#define CLIENT_NUM_MAX				10
#define CLI_SENDBUF_SIZE			PROTO_PACK_MAX_LEN
#define CLI_RECVBUF_SIZE			(PROTO_PACK_MAX_LEN*3)
#define HEARTBEAT_INTERVAL_S		10

typedef enum
{
	STATE_CLOSE,
	STATE_DISCONNECT,
	STATE_CONNECTED,
	STATE_LOGIN,
}sockState_e;

struct clientInfo
{
	int fd;
	int protoHandle;
	pthread_mutex_t	send_mutex;
	sockState_e state;
	struct sockaddr_in 	srv_addr;		// server ip addr
	struct ringbuffer recvRingBuf;			// socket receive data ring buffer
	struct detect_info detectInfo;
	uint8_t packBuf[PROTO_PACK_MAX_LEN];		// protocol packet data buffer
	int packLen;
	uint8_t ack_buf[PROTO_PACK_MAX_LEN];
	uint8_t tmpBuf[PROTO_PACK_MAX_LEN];
};

struct client_mngr_info
{
	int client_num;
	struct clientInfo *client[CLIENT_NUM_MAX];
	pthread_t client_tid[CLIENT_NUM_MAX];
};

pthread_t start_socket_client_task(char *svr_str);
void client_mngr_join_client(pthread_t tid, struct clientInfo *client);
void client_mngr_set_client_exit(pthread_t tid);
int socket_client_mngr_init(void);


#endif	// _SOCKET_CLIENT_H_
