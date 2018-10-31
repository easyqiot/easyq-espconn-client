#ifndef EASYQ_H_
#define EASYQ_H_

#include "mem.h"
#include "osapi.h"
#include "c_types.h"
#include "ip_addr.h"
#include "espconn.h"


typedef enum {
	EASYQ_IDLE = 0,
	EASYQ_CONNECTING,
	EASYQ_CONNECTED
} EasyQStatus;

typedef struct easy_session {
	struct espconn *tcpconn;
	char *hostname;
	uint16_t port;
} EasyQSession;

typedef void (*EasyQCallback)(EasyQStatus status);

void ICACHE_FLASH_ATTR easyq_connect(EasyQSession *eq, EasyQCallback callback); 

void ICACHE_FLASH_ATTR easyq_init(EasyQSession *eq, const char *hostname, 
		uint16_t port);

#endif

