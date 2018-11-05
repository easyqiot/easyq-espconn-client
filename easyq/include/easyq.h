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
	EASYQ_CONNECTING,
	EASYQ_CONNECTED,
	EASYQ_DISCONNECT,
	EASYQ_DISCONNECTED,
	// Not Nedded 
	EASYQ_DATA_RECEIVED,
	EASYQ_DELETING,
	EASYQ_DELETED,
	EASYQ_RECONNECT_DISCONNECTING,
} EasyQStatus;


typedef enum {
	EASYQ_OK,
	EASYQ_ERR_ALREADY_INITIALIZED,
	EASYQ_ERR_ALREADY_CONNECTED,
	EASYQ_ERR_ALREADY_CONNECTING,
	EASYQ_ERR_DISCONNECTING,
	EASYQ_ERR_DELETING,
} EasyQError;	


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
void ICACHE_FLASH_ATTR easyq_disconnect(EasyQSession *eq); 
void ICACHE_FLASH_ATTR easyq_init(EasyQSession *eq, const char *hostname, 
		uint16_t port);

#endif

