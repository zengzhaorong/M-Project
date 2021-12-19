#include <stdio.h>
#include <string.h>
#include <stdlib.h>
#include <time.h>
#include "protocol.h"
#include "ringbuffer.h"
#include "public.h"

/* protocol format:
 * | 0xFF | "ABCD" | .... | .... | .... | .... | 0xFE |
 * | HEAD | VERIFY | SEQ  | CMD  | LEN  | DATA | TAIL |
 * |1 byte| 4 byte |1 byte|1 byte|4 byte|N byte|1 byte|
 *
 */

 /* CMD :
  * 0x01: login (reserve)
  *
  * 0x02: logout (reserve)
  *
  * 0x03: heart beat
  *
  */

struct proto_object protoObject[MAX_PROTO_OBJ];
static uint8_t tmp_protoBuf[PROTO_PACK_MAX_LEN];


int proto_0x01_login(int handle, uint8_t *usr_name, uint8_t *password)
{
	uint8_t *protoBuf = NULL;
	int data_len = 0;
	int buf_size = 0;
	int packLen = 0;

	if(handle < 0 || handle >=MAX_PROTO_OBJ)
		return -1;
	if(protoObject[handle].send_func == NULL)
		return -1;

	/* user name */
	memcpy(tmp_protoBuf +data_len, usr_name, 32);
	data_len += 32;

	/* pass word */
	memcpy(tmp_protoBuf +data_len, password, 32);
	data_len += 32;

	protoBuf = protoObject[handle].send_buf;
	buf_size = protoObject[handle].buf_size;
	proto_makeupPacket(0, 0x01, data_len, tmp_protoBuf, protoBuf, buf_size, &packLen);

	protoObject[handle].send_func(protoObject[handle].arg, protoBuf, packLen);
	
	return 0;
}

int proto_0x02_logout(int handle)
{
	uint8_t *protoBuf = NULL;
	int data_len = 0;
	int buf_size = 0;
	int packLen = 0;

	if(handle < 0 || handle >=MAX_PROTO_OBJ)
		return -1;
	if(protoObject[handle].send_func == NULL)
		return -1;

	protoBuf = protoObject[handle].send_buf;
	buf_size = protoObject[handle].buf_size;
	proto_makeupPacket(0, 0x02, data_len, tmp_protoBuf, protoBuf, buf_size, &packLen);

	protoObject[handle].send_func(protoObject[handle].arg, protoBuf, packLen);
	
	return 0;
}

int proto_0x03_sendHeartBeat(int handle)
{
	uint8_t *protoBuf = NULL;
	int buf_size = 0;
	int packLen = 0;
	uint32_t time_now;
	int data_len = 0;

	if(handle < 0 || handle >=MAX_PROTO_OBJ)
		return -1;
	if(protoObject[handle].send_func == NULL)
		return -1;

	time_now = (uint32_t)time(NULL);
	data_len += 4;

	protoBuf = protoObject[handle].send_buf;
	buf_size = protoObject[handle].buf_size;
	proto_makeupPacket(0, 0x03, data_len, (uint8_t *)&time_now, protoBuf, buf_size, &packLen);

	protoObject[handle].send_func(protoObject[handle].arg, protoBuf, packLen);
	
	return 0;
}

int proto_0x04_switch_monitor(int handle, uint8_t onoff)
{
	uint8_t *protoBuf = NULL;
	int buf_size = 0;
	int packLen = 0;

	if(handle < 0 || handle >=MAX_PROTO_OBJ)
		return -1;
	if(protoObject[handle].send_func == NULL)
		return -1;

	protoBuf = protoObject[handle].send_buf;
	buf_size = protoObject[handle].buf_size;
	proto_makeupPacket(0, 0x04, 1, (uint8_t *)&onoff, protoBuf, buf_size, &packLen);

	protoObject[handle].send_func(protoObject[handle].arg, protoBuf, packLen);
	
	return 0;
}

int proto_0x10_sendCaptureFrame(int handle, int format, uint8_t *frame, int len)
{
	uint8_t *protoBuf = NULL;
	int data_len = 0;
	int buf_size = 0;
	int packLen = 0;

	if(handle < 0 || handle >=MAX_PROTO_OBJ)
		return -1;
	if(protoObject[handle].send_func == NULL)
		return -1;

	/* format */
	memcpy(tmp_protoBuf +data_len, &format, 4);
	data_len += 4;

	/* data len */
	memcpy(tmp_protoBuf +data_len, &len, 4);
	data_len += 4;

	/* frame data */
	memcpy(tmp_protoBuf +data_len, frame, len);
	data_len += len;

	protoBuf = protoObject[handle].send_buf;
	buf_size = protoObject[handle].buf_size;
	proto_makeupPacket(0, 0x10, data_len, tmp_protoBuf, protoBuf, buf_size, &packLen);

	protoObject[handle].send_func(protoObject[handle].arg, protoBuf, packLen);
	
	return 0;
}


int proto_makeupPacket(uint8_t seq, uint8_t cmd, int len, uint8_t *data, \
								uint8_t *outbuf, int size, int *outlen)
{
	uint8_t *packBuf = outbuf;
	int packLen = 0;

	if((len!=0 && data==NULL) || outbuf==NULL || outlen==NULL)
		return -1;

	/* if outbuf is not enough */
	if(PROTO_DATA_OFFSET +len +1 > size)
	{
		printf("ERROR: %s: outbuf size [%d:%d] is not enough !!!\n", __FUNCTION__, size, PROTO_DATA_OFFSET +len +1);
		return -2;
	}

	packBuf[PROTO_HEAD_OFFSET] = PROTO_HEAD;
	packLen += 1;
	
	memcpy(packBuf +PROTO_VERIFY_OFFSET, PROTO_VERIFY, 4);
	packLen += 4;

	packBuf[PROTO_SEQ_OFFSET] = seq;
	packLen += 1;
	
	packBuf[PROTO_CMD_OFFSET] = cmd;
	packLen += 1;

	memcpy(packBuf +PROTO_LEN_OFFSET, &len, 4);
	packLen += 4;

	memcpy(packBuf +PROTO_DATA_OFFSET, data, len);
	packLen += len;

	packBuf[PROTO_DATA_OFFSET +len] = PROTO_TAIL;
	packLen += 1;

	*outlen = packLen;

	return 0;
}

int proto_analyPacket(uint8_t *pack, int packLen, uint8_t *seq, \
								uint8_t *cmd, int *len, uint8_t **data)
{

	if(pack==NULL || seq==NULL || cmd==NULL || len==NULL || data==NULL)
		return -1;

	if(packLen < PROTO_PACK_MIN_LEN)
		return -2;

	*seq = pack[PROTO_SEQ_OFFSET];

	*cmd = pack[PROTO_CMD_OFFSET];

	memcpy(len, pack +PROTO_LEN_OFFSET, 4);

	if(*len +PROTO_PACK_MIN_LEN != packLen)
	{
		return -1;
	}

	if(*len > 0)
		*data = pack + PROTO_DATA_OFFSET;

	return 0;
}

int proto_detectPack(struct ringbuffer *ringbuf, struct detect_info *detect, \
							uint8_t *proto_data, int size, int *proto_len)
{
	char buf[256];
	int len;
	char veri_buf[] = PROTO_VERIFY;
	int tmp_protoLen;
	uint8_t byte;

	if(ringbuf==NULL || proto_data==NULL || proto_len==NULL || size<PROTO_PACK_MIN_LEN)
		return -1;

	tmp_protoLen = *proto_len;

	/* get and check protocol head */
	if(!detect->head)
	{
		while(ringbuf_datalen(ringbuf) > 0)
		{
			ringbuf_read(ringbuf, &byte, 1);
			if(byte == PROTO_HEAD)
			{
				proto_data[0] = byte;
				tmp_protoLen = 1;
				detect->head = 1;
				//printf("********* detect head\n");
				break;
			}
		}
	}

	/* get and check verify code */
	if(detect->head && !detect->verify)
	{
		while(ringbuf_datalen(ringbuf) > 0)
		{
			ringbuf_read(ringbuf, &byte, 1);
			if(byte == veri_buf[tmp_protoLen-1])
			{
				proto_data[tmp_protoLen] = byte;
				tmp_protoLen ++;
				if(tmp_protoLen == 1+strlen(PROTO_VERIFY))
				{
					detect->verify = 1;
					//printf("********* detect verify\n");
					break;
				}
			}
			else
			{
				if(byte == PROTO_HEAD)
				{
					proto_data[0] = byte;
					tmp_protoLen = 1;
					detect->head = 1;
				}
				else
				{
					tmp_protoLen = 0;
					detect->head = 0;
				}
			}
		}
	}

	/* get other protocol data */
	if(detect->head && detect->verify)
	{
		while(ringbuf_datalen(ringbuf) > 0)
		{
			if(tmp_protoLen < PROTO_DATA_OFFSET)	// read data_len
			{
				len = ringbuf_read(ringbuf, buf, sizeof(buf) < PROTO_DATA_OFFSET -tmp_protoLen ? \
													sizeof(buf) : PROTO_DATA_OFFSET -tmp_protoLen);
				if(len > 0)
				{
					memcpy(proto_data +tmp_protoLen, buf, len);
					tmp_protoLen += len;
				}
				if(tmp_protoLen >= PROTO_DATA_OFFSET)
				{
					memcpy(&len, proto_data +PROTO_LEN_OFFSET, 4);
					detect->pack_len = PROTO_DATA_OFFSET +len +1;
					if(detect->pack_len > size)
					{
						printf("ERROR: %s: pack len[%d] > buf size[%d]\n", __FUNCTION__, size, detect->pack_len);
						memset(detect, 0, sizeof(struct detect_info));
					}
				}
			}
			else	// read data
			{
				len = ringbuf_read(ringbuf, buf, sizeof(buf) < detect->pack_len -tmp_protoLen ? \
													sizeof(buf) : detect->pack_len -tmp_protoLen);
				if(len > 0)
				{
					memcpy(proto_data +tmp_protoLen, buf, len);
					tmp_protoLen += len;
					if(tmp_protoLen == detect->pack_len)
					{
						if(proto_data[tmp_protoLen-1] != PROTO_TAIL)
						{
							printf("%s : packet data error, no detect tail!\n", __FUNCTION__);
							memset(detect, 0, sizeof(struct detect_info));
							tmp_protoLen = 0;
							break;
						}
						*proto_len = tmp_protoLen;
						memset(detect, 0, sizeof(struct detect_info));
						//printf("%s : get complete protocol packet, len: %d\n", __FUNCTION__, *proto_len);
						return 0;
					}
				}
			}
		}
	}

	*proto_len = tmp_protoLen;

	return -1;
}

int proto_get_handle_list(int *list_buf, int buf_size, int *num)
{
	int i, j;

	if(list_buf==NULL || num==NULL)
		return -1;

	for(i=0,j=0; i<MAX_PROTO_OBJ; i++)
	{
		if(protoObject[i].used)		// can use this
		{
			list_buf[j++] = i;
			if(j >= buf_size)
				break;
		}
	}
	
	*num = j;

	return 0;
}

/* return: handle */
int proto_register(void *arg, send_func_t send_func, int buf_size)
{
	int handle = -1;
	int i;

	for(i=0; i<MAX_PROTO_OBJ; i++)
	{
		if(protoObject[i].used == 0)
		{
			protoObject[i].arg = arg;
			protoObject[i].send_func = send_func;

			protoObject[i].buf_size = buf_size;
			protoObject[i].send_buf = malloc(buf_size);
			if(protoObject[i].send_buf == NULL)
				return -1;
			
			protoObject[i].used = 1;
			handle = i;
			break;
		}
	}

	return handle;
}

void proto_unregister(int handle)
{
	protoObject[handle].used = 0;
	protoObject[handle].send_func = NULL;
	
	if(protoObject[handle].send_buf != NULL)
	{
		free(protoObject[handle].send_buf);
		protoObject[handle].send_buf = NULL;
	}
	
}

int proto_init(void)
{
	memset(&protoObject, 0, sizeof(protoObject));

	return 0;
}

