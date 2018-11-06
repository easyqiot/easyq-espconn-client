#include "easyq.h"
#include "debug.h"

// Internal
#include "_easyq.c"

os_event_t easyq_task_queue[EASYQ_TASK_QUEUE_SIZE];


/* State Machine
 
+------------+-----------+------------+--------+-----------+--+
| state\req  |  CONNECT  | DISCONNECT | DELETE | RECONNECT |  |
+------------+-----------+------------+--------+-----------+--+
| IDLE       | CONECTED  | ERR        | NULL   | ERR       |  |
| CONNECT    | ERR       | IDLE       | NULL   | ERR       |  |
| CONNECTED  | ERR       | IDLE       | NULL   | ERR       |  |
| DISCONNECT | ERR       | ERR        | ERR    | ERR       |  |
| DELETE     | ERR       | ERR        | ERR    | ERR       |  |
| RECONNECT  | CONNECTED | IDLE       | ERR    | ERR       |  |
+------------+-----------+------------+--------+-----------+--+

*/
LOCAL EasyQError ICACHE_FLASH_ATTR
_easyq_validate_transition(EasyQStatus state, EasyQStatus command) {
	switch (state) {
		case EASYQ_IDLE:
			if (command == EASYQ_DISCONNECT || command == EASYQ_RECONNECT) {
				return EASYQ_ERR_NOT_CONNECTED;
			}
			break;
		case EASYQ_CONNECT:
			if (command == EASYQ_CONNECT || command == EASYQ_RECONNECT) {
				return EASYQ_ERR_ALREADY_CONNECTING;
			}
			break;
		case EASYQ_CONNECTED:
			if (command == EASYQ_CONNECT || command == EASYQ_RECONNECT) {
				return EASYQ_ERR_ALREADY_CONNECTED;
			}
			break;
		case EASYQ_DISCONNECT:
			return EASYQ_ERR_DISCONNECTING;
		case EASYQ_DELETE:
			return EASYQ_ERR_DELETING;
	}
	return EASYQ_OK;
}


void ICACHE_FLASH_ATTR
easyq_task(os_event_t *e)
{
    if (e->par == 0) {
        return;
	}

    EasyQSession* eq = (EasyQSession*)e->par;
    switch (eq->status) {
	case EASYQ_IDLE:
		INFO("EasyQ: IDLE\r\n");
		break;
    case EASYQ_CONNECT:
		INFO("EASYQ: Trying connect to %s:%d\r\n", eq->hostname, eq->port);
		_easyq_connect(eq);
        break;
    case EASYQ_RECONNECT:
		break;
	case EASYQ_CONNECTED:
		break;
    case EASYQ_DISCONNECT:
    case EASYQ_DELETE:
		INFO("EASYQ: deleting %s:%d\r\n", eq->hostname, eq->port);
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
	return EASYQ_OK;
}


/* Schedules a disconnect request
 */
EasyQError ICACHE_FLASH_ATTR 
easyq_disconnect(EasyQSession *eq) {
	EasyQError err = _easyq_validate_transition(eq->status, EASYQ_DISCONNECT);
	if (err != EASYQ_OK) {
		return err;
	}
	eq->status = EASYQ_DISCONNECT;
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
	return EASYQ_OK;
}


/* Alocate memory and inititalize the task
 */
EasyQError ICACHE_FLASH_ATTR 
easyq_init(EasyQSession *eq, const char *hostname, uint16_t port) {
	if (eq->hostname) {
		return EASYQ_ERR_ALREADY_INITIALIZED;
	}

	size_t hostname_len = os_strlen(hostname);
	eq->hostname = (char *)os_malloc(hostname_len + 1);
	os_strcpy(eq->hostname, hostname);
	eq->hostname[hostname_len] = '\0';
	eq->port = port;
	eq->ticks = 0;
	eq->status = EASYQ_IDLE;
	INFO("Preparing EasyQ Server: %s:%d\r\n", hostname, port);
    system_os_task(easyq_task, EASYQ_TASK_PRIO, easyq_task_queue, 
			EASYQ_TASK_QUEUE_SIZE);
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
	return EASYQ_OK;
}


/* Disconnect and dealocate the memory.
 */
EasyQError ICACHE_FLASH_ATTR 
easyq_delete(EasyQSession *eq) {
	EasyQError err = _easyq_validate_transition(eq->status, EASYQ_DELETE);
	if (err != EASYQ_OK) {
		return err;
	}
	INFO("Deleting EasyQ\r\n");
	eq->status = EASYQ_DELETE;
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
	// TODO: delete task
	return EASYQ_OK;
}

