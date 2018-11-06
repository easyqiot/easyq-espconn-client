#ifndef EASYQ_H_
#define EASYQ_H_

#include "mem.h"
#include "osapi.h"
#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "utils.h"


#define EASYQ_TASK_QUEUE_SIZE	1
#define EASYQ_TASK_PRIO			2
#define EASYQ_BUF_SIZE			1024	


typedef enum {
	EASYQ_IDLE = 0,
	EASYQ_CONNECT,
	EASYQ_CONNECTED,
	EASYQ_DISCONNECT,
	EASYQ_DELETE,
	EASYQ_RECONNECT,
} EasyQStatus;


typedef enum {
	EASYQ_OK,
	EASYQ_ERR_ALREADY_INITIALIZED,		// 1
	EASYQ_ERR_ALREADY_CONNECTED,		// 2
	EASYQ_ERR_ALREADY_CONNECTING,		// 3
	EASYQ_ERR_DISCONNECTING,			// 4
	EASYQ_ERR_DELETING,					// 5
	EASYQ_ERR_NOT_CONNECTED,			// 6
} EasyQError;	


typedef void (*EasyQCallback)(void *eq);


typedef struct easy_session {
	struct espconn *tcpconn;
	char *hostname;
	uint16_t port;
	EasyQStatus status;
	ip_addr_t ip;
	ETSTimer timer;
	uint64_t ticks;
	EasyQCallback onconnect;
	EasyQCallback ondisconnect;
	EasyQCallback onconnectionerror;
} EasyQSession;


EasyQError ICACHE_FLASH_ATTR 
easyq_connect(EasyQSession *eq); 

EasyQError ICACHE_FLASH_ATTR 
easyq_disconnect(EasyQSession *eq); 

EasyQError ICACHE_FLASH_ATTR 
easyq_init(EasyQSession *eq, const char *hostname, uint16_t port);


#endif

