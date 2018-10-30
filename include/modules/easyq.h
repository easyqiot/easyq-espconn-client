#ifndef EASYQ_H_
#define EASYQ_H_

typedef enum {
	idle = 0,
	connecting,
	connected
} EasyQStatus;

typedef struct easy_session {
	struct espconn *tcpconn;
} EasyQSession;

typedef void (*EasyQCallback)(EasyQStatus status);

void ICACHE_FLASH_ATTR easyq_connect(EasyQSession *c, EasyQCallback callback); 

#endif

