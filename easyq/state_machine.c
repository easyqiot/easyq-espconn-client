
#include <osapi.h>  

#include "easyq.h"
#include "debug.h"

/* State Machine


           +---------+                      +------------+
           |         +---SIG-CONNECT-------->            |
           |  Idle   |                      | Connecting |
     +----->         |                   +--+            |
     |     +---------+                   |  +---------+--+
     |                                   |            |
     |                                   |   CB-CONNECTED
     |                                   |            |
     |   +--------------+                V  +---------v--+
     |   |              <--CB-CONN-ERROR----+            |
     |   | Reconnecting |                   | Connected  |
     |   |              +--CB-CONNECTED----->            <-----+
     |   +--------------+                   +-+-------+--+     |
     |                                        |       |        |
  CB-DISCONNECTED                   SIG-DISCONNECT    |        |
     |                                        |       |    CB-SENT
     |   +---------------+                    |       |        |
     +---+               <--------------------+       |        |
         | Disconnecting |                            |        |
         |               |                       SIG-SEND      |
         +---------------+                            |        |
                                             +--------v--+     |
                                             |           |     |
                                             | Sending   +-----+
                                             |           |
                                             +-----------+

*/


// FIXME: May be static
os_event_t easyq_task_queue[EASYQ_TASK_QUEUE_SIZE];


void ICACHE_FLASH_ATTR 
_easyq_timer_cb(void *arg) {
    EasyQSession *eq = (EasyQSession*)arg;
	eq->reconnect_ticks++;
	//if (eq->ticks % 10 == 0) {
	//	INFO("\r\nFree Heap: %lu\r\n", system_get_free_heap_size());
	//}	
	INFO("Status: %d, Ticks: %d\r\n", eq->status, eq->reconnect_ticks);
	if (eq->status == EASYQ_RECONNECTING) {
		INFO("Reconnecting\r\n");
		_easyq_proto_delete(eq);
		_easyq_proto_connect(eq);
	}
}


void ICACHE_FLASH_ATTR
_easyq_task_cb(os_event_t *e)
{
    if (e->par == 0) {
        return;
	}

    EasyQSession* eq = (EasyQSession*)e->par;
    switch (e->sig) {
    case EASYQ_SIG_CONNECT:
		eq->status = EASYQ_CONNECTING;
		_easyq_proto_connect(eq);
        break;

    case EASYQ_SIG_CONNECTED:
	    os_timer_disarm(&eq->timer);
		eq->status = EASYQ_CONNECTED;
		if (eq->onconnect) {
			eq->onconnect(eq);
		}
		break;

    case EASYQ_SIG_RECONNECT:
		eq->status = EASYQ_RECONNECTING;
		// Giving a chance to users to examine what happened before deleting the
		// espconn structure.
		if (eq->onconnectionerror) {
			eq->onconnectionerror(eq);
		}

	    os_timer_disarm(&eq->timer);
	    os_timer_setfn(&eq->timer, (os_timer_func_t *)_easyq_timer_cb, eq);
	    os_timer_arm(&eq->timer, EASYQ_RECONNECT_INTERVAL, 1);
		break;

	case EASYQ_SIG_DISCONNECT:
		eq->status = EASYQ_DISCONNECTING;
		_easyq_proto_disconnect(eq);
		break;

	case EASYQ_SIG_DISCONNECTED:
		_easyq_proto_delete(eq);
	    eq->status = EASYQ_IDLE;
		if (eq->ondisconnect) {
			eq->ondisconnect(eq);
		}
		break;

	case EASYQ_SIG_SEND:
		eq->status = EASYQ_SENDING;
		_easyq_proto_send_buffer(eq);
		break;

	case EASYQ_SIG_SENT:

		// Ignore connecting because we must send auth info before get 
		// connected.
		if (eq->status != EASYQ_CONNECTING &&
				eq->status != EASYQ_RECONNECTING) {
			eq->status = EASYQ_CONNECTED;
		}
		break;

	case EASYQ_SIG_DELETE:
		_easyq_delete(eq);
	}
}


void ICACHE_FLASH_ATTR
_easyq_task_start() {
    system_os_task(_easyq_task_cb, EASYQ_TASK_PRIO, easyq_task_queue, 
			EASYQ_TASK_QUEUE_SIZE);
}


EasyQError ICACHE_FLASH_ATTR
_easyq_task_post(EasyQSession *eq, EasyQSignal signal) {
	switch (eq->status) {
		case EASYQ_IDLE:
			if (signal != EASYQ_SIG_CONNECT && signal != EASYQ_SIG_DELETE) {
				return EASYQ_ERR_NOT_CONNECTED;
			}
			break;
		case EASYQ_CONNECTING:
			if (signal != EASYQ_SIG_CONNECTED && 
					signal != EASYQ_SIG_DISCONNECT &&
					signal != EASYQ_SIG_RECONNECT) {
				return EASYQ_ERR_ALREADY_CONNECTING;
			}
			break;
		case EASYQ_CONNECTED:
			if (signal != EASYQ_SIG_RECONNECT && 
					signal != EASYQ_SIG_DISCONNECT &&
					signal != EASYQ_SIG_SEND) {
				return EASYQ_ERR_ALREADY_CONNECTED;
			}
			break;
		case EASYQ_DISCONNECTING:
			if (signal != EASYQ_SIG_DISCONNECTED) {
				return EASYQ_ERR_DISCONNECTING;
			}
			break;
		case EASYQ_RECONNECTING:
			if (signal != EASYQ_SIG_CONNECTED) {
				return EASYQ_ERR_ALREADY_CONNECTING;
			}
			break;
		case EASYQ_SENDING:
			if (signal != EASYQ_SIG_SENT && signal != EASYQ_SIG_RECONNECT) {
				return EASYQ_ERR_ALREADY_SENDING;
			}
			break;
	}

    system_os_post(EASYQ_TASK_PRIO, signal, (os_param_t)eq);
	return EASYQ_OK;
}


