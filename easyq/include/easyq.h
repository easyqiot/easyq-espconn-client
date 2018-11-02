#ifndef EASYQ_H_
#define EASYQ_H_

#include "mem.h"
#include "osapi.h"
#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"
#include "utils.h"


typedef enum {
	EASYQ_IDLE = 0,
	EASYQ_CONNECTING,
	EASYQ_CONNECTED,
	EASYQ_RECONNECT,
	EASYQ_DISCONNECTING,
	EASYQ_DATA_RECEIVED,
	EASYQ_DELETING,
	EASYQ_DELETED,
	EASYQ_RECONNECT_REQ,
	EASYQ_RECONNECT_DISCONNECTING,
	EASYQ_DISCONNECTED
} EasyQStatus;

typedef struct easy_session {
	struct espconn *tcpconn;
	char *hostname;
	uint16_t port;
	EasyQStatus status;
	ip_addr_t ip;
	ETSTimer timer;
} EasyQSession;

typedef void (*EasyQCallback)(EasyQStatus status);

void ICACHE_FLASH_ATTR easyq_connect(EasyQSession *eq); 

void ICACHE_FLASH_ATTR easyq_init(EasyQSession *eq, const char *hostname, 
		uint16_t port);

#endif

