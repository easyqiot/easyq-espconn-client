#ifndef EASYQ_H_
#define EASYQ_H_

#include "c_types.h"

typedef enum {
	EASYQ_IDLE = 0,
	EASYQ_CONNECTING,
	EASYQ_CONNECTED
} EasyQStatus;

typedef struct easy_session {
	struct espconn *tcpconn;
} EasyQSession;

typedef void (*EasyQCallback)(EasyQStatus status);

void ICACHE_FLASH_ATTR easyq_connect(EasyQSession *c, const char *host, 
		unsigned short port, EasyQCallback callback); 

#endif

