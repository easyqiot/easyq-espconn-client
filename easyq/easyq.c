#include "easyq.h"
#include "debug.h"


/* Local */
LOCAL void ICACHE_FLASH_ATTR
_easyq_tcpconn_delete(EasyQSession *eq) {
    if (eq->tcpconn != NULL) {
        espconn_delete(eq->tcpconn);
        if (eq->tcpconn->proto.tcp)
            os_free(eq->tcpconn->proto.tcp);
        os_free(eq->tcpconn);
        eq->tcpconn = NULL;
    }
}


LOCAL void ICACHE_FLASH_ATTR
_easyq_delete(EasyQSession *eq) {
	_easyq_tcpconn_delete(eq);
    os_free(eq->hostname);
}


LOCAL void ICACHE_FLASH_ATTR 
_easyq_disconnect(EasyQSession *eq) {
    os_timer_disarm(&eq->timer);
    espconn_disconnect(eq->tcpconn);
}


LOCAL void ICACHE_FLASH_ATTR
_easyq_tcpclient_disconnect_cb(void *arg) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;

	if (eq->ondisconnect) {
		eq->ondisconnect(eq);
	}

	if(EASYQ_DELETE == eq->status) {
		_easyq_delete(eq);
		return;
    }

    if(EASYQ_DISCONNECT == eq->status) {
		_easyq_tcpconn_delete(eq);
        eq->status = EASYQ_IDLE;
    }
    else {
        eq->status = EASYQ_CONNECT;
    }

    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


void ICACHE_FLASH_ATTR
_easyq_tcpclient_recon_cb(void *arg, sint8 errType) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	_easyq_tcpconn_delete(eq);
	eq->status = EASYQ_RECONNECT;
	if (eq->onconnectionerror) {
		eq->onconnectionerror(eq);
	}
}


void ICACHE_FLASH_ATTR
_easyq_tcpclient_connect_cb(void *arg) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	eq->status = EASYQ_CONNECTED;
	espconn_regist_disconcb(eq->tcpconn, _easyq_tcpclient_disconnect_cb);
	if (eq->onconnect) {
		eq->onconnect(eq);
	}
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


LOCAL void ICACHE_FLASH_ATTR
_easyq_dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;

    if (ipaddr == NULL)
    {
        ERROR("DNS: Found, but got no ip, try to reconnect\r\n");
		_easyq_tcpconn_delete(eq);
        eq->status = EASYQ_CONNECT;
		system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
        return;
    }

    INFO("DNS: found ip %d.%d.%d.%d\n",
         *((uint8 *) &ipaddr->addr),
         *((uint8 *) &ipaddr->addr + 1),
         *((uint8 *) &ipaddr->addr + 2),
         *((uint8 *) &ipaddr->addr + 3));

    if (eq->ip.addr == 0 && ipaddr->addr != 0)
    {
        os_memcpy(eq->tcpconn->proto.tcp->remote_ip, &ipaddr->addr, 4);
        espconn_connect(eq->tcpconn);
    }
}


void ICACHE_FLASH_ATTR 
_easyq_timer(void *arg) {
    EasyQSession *eq = (EasyQSession*)arg;
	eq->ticks++;
	INFO("Timer tick: %lu\r\n", eq->ticks);
	if (eq->ticks % 10 == 0) {
		system_print_meminfo();
	}
	if (eq->status == EASYQ_RECONNECT) {
		eq->status = EASYQ_CONNECT;
		system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
	}
}


LOCAL void ICACHE_FLASH_ATTR 
_easyq_connect(EasyQSession *eq) {
    eq->tcpconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    eq->tcpconn->type = ESPCONN_TCP;
    eq->tcpconn->state = ESPCONN_NONE;
    eq->tcpconn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    eq->tcpconn->proto.tcp->local_port = espconn_port();
    eq->tcpconn->proto.tcp->remote_port = eq->port;
    eq->tcpconn->reverse = eq; 
    espconn_regist_connectcb(eq->tcpconn, _easyq_tcpclient_connect_cb);
    espconn_regist_reconcb(eq->tcpconn, _easyq_tcpclient_recon_cb);

    os_timer_disarm(&eq->timer);
    os_timer_setfn(&eq->timer, (os_timer_func_t *)_easyq_timer, eq);
    os_timer_arm(&eq->timer, 1000, 1);


    if (UTILS_StrToIP(eq->hostname, &eq->tcpconn->proto.tcp->remote_ip)) {
        espconn_connect(eq->tcpconn);
    }
    else {
        espconn_gethostbyname(eq->tcpconn, eq->hostname, &eq->ip, 
				_easyq_dns_found);
    }
}

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


/* 
 * Public functions
 */

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

