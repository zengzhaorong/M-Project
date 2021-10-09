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


/* C++ include C */
#ifdef __cplusplus
extern "C" {
#endif
#include "ringbuffer.h"
#include "capture.h"
#ifdef __cplusplus
}
#endif


struct clientInfo client_info;
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

int client_init(struct clientInfo *client, char *srv_ip, int srv_port)
{
	int flags = 0;
	int ret;

	memset(client, 0, sizeof(struct clientInfo));

	client->state = STATE_DISCONNECT;

	client->fd = socket(AF_INET, SOCK_STREAM, IPPROTO_TCP);
	if(client->fd < 0)
	{
		ret = -1;
		goto ERR_1;
	}

	pthread_mutex_init(&client->send_mutex, NULL);

	flags = fcntl(client->fd, F_GETFL, 0);
	fcntl(client->fd, F_SETFL, flags | O_NONBLOCK);

	client->srv_addr.sin_family = AF_INET;
	inet_pton(AF_INET, srv_ip, &client->srv_addr.sin_addr);
	client->srv_addr.sin_port = htons(srv_port);

	ret = ringbuf_init(&client->recvRingBuf, CLI_RECVBUF_SIZE);
	if(ret != 0)
	{
		ret = -2;
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
	struct clientInfo *client = &client_info;
	time_t heartbeat_time = 0;
	time_t login_time = 0;
	time_t tmpTime;
	int ret;

	ret = client_init(client, (char *)CONFIG_SERVER_IP(main_mngr.config_ini), CONFIG_SERVER_PORT(main_mngr.config_ini));
	if(ret != 0)
	{
		return NULL;
	}

	while(1)
	{
		switch (client->state)
		{
			case STATE_DISABLE:
				//printf("%s %d: state STATE_DISABLE ...\n", __FUNCTION__, __LINE__);
				break;

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

}


int start_socket_client_task(void)
{
	pthread_t tid;
	int ret;

	ret = pthread_create(&tid, NULL, socket_client_thread, NULL);
	if(ret != 0)
	{
		return -1;
	}

	return 0;
}





