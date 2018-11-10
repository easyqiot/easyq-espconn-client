#ifndef _EASYQ_H_
#define _EASYQ_H_

#include "mem.h"
#include "osapi.h"
#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "utils.h"


#define EASYQ_TASK_QUEUE_SIZE	1
#define EASYQ_TASK_PRIO			2
#define EASYQ_RECV_BUFFER_SIZE  1024
#define EASYQ_SEND_BUFFER_SIZE  1024
#define EASYQ_RECONNECT_INTERVAL	5

typedef enum {
	EASYQ_IDLE = 0,
	EASYQ_CONNECT,
	EASYQ_CONNECTED,
	EASYQ_DISCONNECT,
	EASYQ_DELETE,
	EASYQ_RECONNECT,
	EASYQ_SEND,
	EASYQ_MESSAGE,
} EasyQStatus;


typedef enum {
	EASYQ_OK,
	EASYQ_ERR_ALREADY_INITIALIZED,		// 1
	EASYQ_ERR_ALREADY_CONNECTED,		// 2
	EASYQ_ERR_ALREADY_CONNECTING,		// 3
	EASYQ_ERR_DISCONNECTING,			// 4
	EASYQ_ERR_DELETING,					// 5
	EASYQ_ERR_NOT_CONNECTED,			// 6
	EASYQ_ERR_TCP_SEND,					// 7
} EasyQError;	


typedef void (*EasyQCallback)(void *);
typedef void (*EasyQMessageCallback)(void*, const char*, const char*, uint16_t);

typedef struct easy_session {
	struct espconn *tcpconn;
	char *hostname;
	uint16_t port;
	char *login;
	EasyQStatus status;
	ip_addr_t ip;
	ETSTimer timer;
	uint64_t ticks;
	EasyQCallback onconnect;
	EasyQCallback ondisconnect;
	EasyQCallback onconnectionerror;
	EasyQMessageCallback onmessage;
	char * send_buffer;
	size_t sendbuffer_length;
	char * recv_buffer;
	size_t recvbuffer_length;
} EasyQSession;


EasyQError ICACHE_FLASH_ATTR 
easyq_connect(EasyQSession *eq); 

EasyQError ICACHE_FLASH_ATTR 
easyq_disconnect(EasyQSession *eq); 

EasyQError ICACHE_FLASH_ATTR 
easyq_init(EasyQSession *eq, const char *hostname, uint16_t port, 
		const char *login);

EasyQError ICACHE_FLASH_ATTR 
easyq_delete(EasyQSession *eq);

void ICACHE_FLASH_ATTR
easyq_pull(EasyQSession *eq, const char *queue);

void ICACHE_FLASH_ATTR
easyq_pull_all(EasyQSession *eq, const char **queue, size_t qlen);

void ICACHE_FLASH_ATTR
easyq_ignore(EasyQSession *eq, const char *queue);

void ICACHE_FLASH_ATTR
easyq_push(EasyQSession *eq, const char *queue, const char *message);

#endif

