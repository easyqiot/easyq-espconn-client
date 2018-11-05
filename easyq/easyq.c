#include "easyq.h"
#include "debug.h"


/* State Machine
+------------+------------+------------+--------+
| state\req  |  CONNECT   | DISCONNECT | DELETE |
+------------+------------+------------+--------+
| IDLE       | CONNECTING | ERR        | NULL   |
| CONNECTED  | ERR        | IDLE       | NULL   |
| CONNECT    | ERR        | IDLE       | NULL   |
| DISCONNECT | ERR        | ERR        | ERR    |
| DELETE     | ERR        | ERR        | ERR    |
+------------+------------+------------+--------+
*/
LOCAL EasyQError ICACHE_FLASH_ATTR
_easyq_validate_transition(EasyQStatus from, EasyQStatus to) {
	switch (from){
		case EASYQ_CONNECTED:
			return EASYQ_ERR_ALREADY_CONNECTED;
		case EASYQ_CONNECT:
			return EASYQ_ERR_ALREADY_CONNECTING;
		case EASYQ_DISCONNECT:
			return EASYQ_ERR_DISCONNECTING;
		case EASYQ_DELETE:
			return EASYQ_ERR_DELETING;
	}

}

void ICACHE_FLASH_ATTR
_easyq_task(os_event_t *e)
{
    if (e->par == 0) {
        return;
	}

    EasyQSession* eq = (EasyQSession*)e->par;
    switch (eq->status) {
		case EASYQ_IDLE:
			INFO("EasyQ IDLE\r\n");
		case EASYQ_CONNECTED;
			INFO("TCP: Connected to %s:%d\r\n", eq->hostname, eq->port);

	    case EASYQ_CONNECT:
			_easyq_connect(eq);
	        break;
	    case EASYQ_DISCONNECT:
	    case EASYQ_DELETE:
			_easyq_disconnect(eq);
			break;
	    }
}


/* Schedules a connect request
 */
EasyQError ICACHE_FLASH_ATTR 
easyq_connect(EasyQSession *eq) {
	EasyQError err = _easyq_validate_transition(eq->status, EASYQ_CONNECT);
	if (err != EASYQ_OK) {
		return err;
	}
	eq->status = EASYQ_CONNECT;
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


/* Schedules a disconnect request
 */
void ICACHE_FLASH_ATTR easyq_disconnect(EasyQSession *eq) {
	eq->status = EASYQ_DISCONNECT;
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


/* Alocate memory and inititalize the task
 */
void ICACHE_FLASH_ATTR easyq_init(EasyQSession *eq, const char *hostname, 
		uint16_t port) {
	size_t hostname_len = os_strlen(hostname);
	eq->hostname = (char *)os_malloc(hostname_len + 1);
	os_strcpy(eq->hostname, hostname);
	eq->hostname[hostname_len] = '\0';
	eq->port = port;
	eq->status = EASYQ_IDLE;
	INFO("Preparing EasyQ Server: %s:%d\r\n", hostname, port);
    system_os_task(easyq_task, EASYQ_TASK_PRIO, easyq_task_queue, EASYQ_TASK_QUEUE_SIZE);
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


/* Disconnect and dealocate the memory.
 */
void ICACHE_FLASH_ATTR easyq_delete(EasyQSession *eq) {
	INFO("Deleting EasyQ\r\n");
	eq->status = EASYQ_DELETE;
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
	// TODO: delete task
}

