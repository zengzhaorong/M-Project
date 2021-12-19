#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <unistd.h>
#include <pthread.h>
#include <fcntl.h>
#include <sys/socket.h>
#include <arpa/inet.h>
#include "socket_client.h"
#include "mainwindow.h"
#include "config.h"


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "ringbuffer.h"
#ifdef __cplusplus
}
#endif


struct client_mngr_info client_mngr = {0};
int global_seq;

extern struct main_mngr_info main_mngr;


int client_0x01_login(struct clientInfo *client, uint8_t *data, int len, uint8_t *ack_data, int size, int *ack_len)
{
	int ret = 0;

	/* return value */
	memcpy(&ret, data, 4);
	if(ret == 0)
	{
		client->state = STATE_LOGIN;
		printf("Congratulation! Login success.\n");

	}

	return 0;
}

int client_0x03_heartbeat(struct clientInfo *client, uint8_t *data, int len, uint8_t *ack_data, int size, int *ack_len)
{
	uint32_t svrTime;
	int tmplen = 0;
	int ret;

	if(data==NULL || len<=0)
		return -1;

	memcpy(&ret, data +tmplen, 4);
	tmplen += 4;

	/* beijing time */
	memcpy(&svrTime, data +tmplen, 4);
	tmplen += 4;


	return 0;
}

int client_0x10_sendCaptureFrame(struct clientInfo *client, uint8_t *data, int len, uint8_t *ack_data, int size, int *ack_len)
{
	int format = 0;
	int frame_len = 0;
	int offset = 0;
	
	/* format */
	memcpy(&format, data +offset, 4);
	offset += 4;

	/* frame len */
	memcpy(&frame_len, data +offset, 4);
	offset += 4;

	/* update frame data */
	newframe_update(data +offset, frame_len);

	return 0;
}


int client_init(struct clientInfo *client, char *srv_ip, int srv_port)
{
	int flags = 0;
	int ret;

	printf("%s: [server] ip: %s, port: %d\n", __FUNCTION__, srv_ip, srv_port);

	if(srv_ip==NULL || srv_port<=0)
		return -1;

	memset(client, 0, sizeof(struct clientInfo));

	client->state = STATE_DISCONNECT;

	client->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(client->fd < 0)
	{
		ret = -2;
		goto ERR_1;
	}

	pthread_mutex_init(&client->send_mutex, NULL);

	flags = fcntl(client->fd, F_GETFL, 0);
	fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);

	client->srv_addr.sin_family = AF_INET;
	ret = inet_pton(AF_INET, srv_ip, &client->srv_addr.sin_addr);
	if(ret != 1)
	{
		ret = -3;
		goto ERR_2;
	}
	client->srv_addr.sin_port = htons(srv_port);

	ret = ringbuf_init(&client->recvRingBuf, CLI_RECVBUF_SIZE);
	if(ret != 0)
	{
		ret = -4;
		goto ERR_2;
	}

	proto_init();

	return 0;

ERR_2:
	close(client->fd);

ERR_1:

	return ret;
}

void client_deinit(struct clientInfo *client)
{
	ringbuf_deinit(&client->recvRingBuf);
	close(client->fd);
	printf("%s: close socket fd=%d.\n", __FUNCTION__, client->fd);
}

int client_sendData(void *arg, uint8_t *data, int len)
{
	struct clientInfo *client = (struct clientInfo *)arg;
	int total = 0;
	int ret;

	if(data == NULL)
		return -1;

	/* add sequence */
	data[PROTO_SEQ_OFFSET] = global_seq++;

	// lock
	pthread_mutex_lock(&client->send_mutex);
	do{
		ret = send(client->fd, data +total, len -total, 0);
		if(ret < 0)
		{
			usleep(1000);
			continue;
		}
		total += ret;
	}while(total < len);
	// unlock
	pthread_mutex_unlock(&client->send_mutex);

	return total;
}

int client_recvData(struct clientInfo *client)
{
	uint8_t *tmpBuf = client->tmpBuf;
	int len, space;
	int ret = 0;

	if(client == NULL)
		return -1;

	space = ringbuf_space(&client->recvRingBuf);

	memset(tmpBuf, 0, PROTO_PACK_MAX_LEN);
	len = recv(client->fd, tmpBuf, PROTO_PACK_MAX_LEN>space ? space:PROTO_PACK_MAX_LEN, 0);
	if(len > 0)
	{
		ret = ringbuf_write(&client->recvRingBuf, tmpBuf, len);
	}

	return ret;
}

int client_protoAnaly(struct clientInfo *client, uint8_t *pack, uint32_t pack_len)
{
	uint8_t *ack_buf = client->ack_buf;
	uint8_t *tmpBuf = client->tmpBuf;
	uint8_t seq = 0, cmd = 0;
	int data_len = 0;
	uint8_t *data = 0;
	int ack_len = 0;
	int tmpLen = 0;
	int ret;

	if(pack==NULL || pack_len<=0)
		return -1;

	ret = proto_analyPacket(pack, pack_len, &seq, &cmd, &data_len, &data);
	if(ret != 0)
		return -1;
	//printf("get proto cmd: 0x%02x\n", cmd);

	switch(cmd)
	{
		case 0x01:
			ret = client_0x01_login(client, data, data_len, ack_buf, PROTO_PACK_MAX_LEN, &ack_len);
			break;

		case 0x03:
			ret = client_0x03_heartbeat(client, data, data_len, ack_buf, PROTO_PACK_MAX_LEN, &ack_len);
			break;

		case 0x10:
			ret = client_0x10_sendCaptureFrame(client, data, data_len, ack_buf, PROTO_PACK_MAX_LEN, &ack_len);
			break;

		default:
			printf("ERROR: protocol cmd[0x%02x] not exist!\n", cmd);
			break;
	}

	/* send ack data */
	if(ret==0 && ack_len>0)
	{
		proto_makeupPacket(seq, cmd, ack_len, ack_buf, tmpBuf, PROTO_PACK_MAX_LEN, &tmpLen);
		client_sendData(client, tmpBuf, tmpLen);
	}

	return 0;
}

int client_protoHandle(struct clientInfo *client)
{
	int recv_ret;
	int det_ret;

	if(client == NULL)
		return -1;

	recv_ret = client_recvData(client);
	det_ret = proto_detectPack(&client->recvRingBuf, &client->detectInfo, client->packBuf, \
							sizeof(client->packBuf), &client->packLen);
	if(det_ret == 0)
	{
		//printf("detect protocol pack len: %d\n", client->packLen);
		client_protoAnaly(client, client->packBuf, client->packLen);
	}

	if(recv_ret<=0 && det_ret!=0)
	{
		usleep(30*1000);
	}

	return 0;
}


void *socket_client_thread(void *arg)
{
	struct clientInfo *client = NULL;
	time_t heartbeat_time = 0;
	time_t login_time = 0;
	time_t tmpTime;
	char svr_str[32];
	char *ip, *port;
	int ret;

	printf("%s: tid=%ld enter ++\n", __FUNCTION__, pthread_self());

	strcpy(svr_str, (char *)arg);

	// get ip and port
	ip = strtok((char *)svr_str, ":");
	port = strtok(NULL, ":");
	if(ip==NULL || port==NULL)
	{
		printf("ERROR: ip or port input illegal!\n");
		goto ERR_0;
	}

	client = (struct clientInfo *)malloc(sizeof(struct clientInfo));
	if(client == NULL)
		goto ERR_0;

	ret = client_init(client, ip, atoi(port));
	if(ret != 0)
	{
		printf("ERROR: client_init failed, ret=%d!\n", ret);
		goto ERR_1;
	}

	client_mngr_join_client(pthread_self(), client);

	while(client->state != STATE_CLOSE)
	{
		switch (client->state)
		{
			case STATE_DISCONNECT:
				ret = connect(client->fd, (struct sockaddr *)&client->srv_addr, sizeof(client->srv_addr));
				if(ret == 0)
				{
					client->protoHandle = proto_register(client, client_sendData, CLI_SENDBUF_SIZE);
					client->state = STATE_CONNECTED;
					printf("********** socket connect successfully, handle: %d.\n", client->protoHandle);
				}
				break;

			case STATE_CONNECTED:
				tmpTime = time(NULL);
				if(abs(tmpTime - login_time) >= 3)
				{
					proto_0x01_login(client->protoHandle, (uint8_t *)"user_name", (uint8_t *)"pass_word");
					login_time = tmpTime;
				}
				break;

			case STATE_LOGIN:
				tmpTime = time(NULL);
				if(abs(tmpTime - heartbeat_time) >= HEARTBEAT_INTERVAL_S)
				{
					proto_0x03_sendHeartBeat(client->protoHandle);
					heartbeat_time = tmpTime;
				}
				break;

			default:
				printf("%s %d: state ERROR !!!\n", __FUNCTION__, __LINE__);
				break;
		}

		if(client->state==STATE_CONNECTED || client->state==STATE_LOGIN)
		{
			client_protoHandle(client);
		}
		else
		{
			usleep(200*1000);
		}
		
	}

	client_deinit(client);

ERR_1:
	free(client);

ERR_0:
	mainwin_video_disconnect(pthread_self());

	printf("%s: exit --\n", __FUNCTION__);
	return NULL;
}


// return thread id
pthread_t start_socket_client_task(char *svr_str)
{
	pthread_t tid;
	static char ip_str[32];
	int ret;

	strcpy(ip_str, svr_str);
	ret = pthread_create(&tid, NULL, socket_client_thread, ip_str);
	if(ret != 0)
	{
		return -1;
	}

	client_mngr_join_client(tid, NULL);

	return tid;
}

void client_mngr_join_client(pthread_t tid, struct clientInfo *client)
{
	int find_flag = 0;
	int idle_index;
	int i;

	for(i=0; i<CLIENT_NUM_MAX; i++)
	{
		if(client_mngr.client_tid[i] == tid)
		{
			client_mngr.client[i] = client;
			find_flag = 0;
			break;
		}
		if(find_flag==0 && client_mngr.client_tid[i]==0)
		{
			idle_index = i;
			find_flag = 1;
		}
	}

	if(find_flag)
	{
		client_mngr.client_tid[idle_index] = tid;
		client_mngr.client[idle_index] = client;
		client_mngr.client_num ++;
	}
	printf("%s: tid=%ld join ++\n", __FUNCTION__, tid);
}

void client_mngr_set_client_exit(pthread_t tid)
{
	int i;

	for(i=0; i<CLIENT_NUM_MAX; i++)
	{
		if(client_mngr.client_tid[i] == tid)
		{
			client_mngr.client_tid[i] = (pthread_t)(-1);
			break;
		}
	}

	printf("%s: set tid=%ld exit --\n", __FUNCTION__, tid);
}

void *socket_client_mngr_thread(void *arg)
{
	int i;

	printf("%s: enter ++\n", __FUNCTION__);

	memset(&client_mngr, 0, sizeof(struct client_mngr_info));

	while(1)
	{
		for(i=0; i<CLIENT_NUM_MAX; i++)
		{
			if(client_mngr.client_tid[i]==(pthread_t)(-1) && client_mngr.client[i]!=NULL)
			{
				client_mngr.client[i]->state = STATE_CLOSE;
				client_mngr.client_tid[i] = 0;
				client_mngr.client[i] = NULL;
			}
		}

		usleep(200 *1000);
	}

	return 0;
}

int socket_client_mngr_init(void)
{
	pthread_t tid;
	int ret;

	ret = pthread_create(&tid, NULL, socket_client_mngr_thread, NULL);
	if(ret != 0)
	{
		return -1;
	}

	return 0;
}

