
#include "easyq.h"
#include "debug.h"

#define EASYQ_TASK_QUEUE_SIZE	1
#define EASYQ_TASK_PRIO			2
#define EASYQ_BUF_SIZE			1024	

os_event_t easyq_task_queue[EASYQ_TASK_QUEUE_SIZE];

void ICACHE_FLASH_ATTR
easyq_tcpconn_delete(EasyQSession *eq)
{
    if (eq->tcpconn != NULL) {
        INFO("Free memory\r\n");
        espconn_delete(eq->tcpconn);
        if (eq->tcpconn->proto.tcp)
            os_free(eq->tcpconn->proto.tcp);
        os_free(eq->tcpconn);
        eq->tcpconn = NULL;
    }
}


void ICACHE_FLASH_ATTR
easyq_delete(EasyQSession *eq)
{
    eq->status = EASYQ_DELETING;
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
    os_timer_disarm(&eq->timer);
}


void ICACHE_FLASH_ATTR
easyq_tcpclient_disconnect_cb(void *arg)
{

    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
    INFO("TCP: Disconnected callback\r\n");
    if(EASYQ_DISCONNECTING == eq->status) {
        eq->status = EASYQ_DISCONNECTED;
    }
    else if(EASYQ_DELETING == eq->status) {
        eq->status = EASYQ_DELETED;
    }
    else {
        eq->status = EASYQ_RECONNECT_REQ;
    }

    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


void ICACHE_FLASH_ATTR
easyq_tcpclient_connect_cb(void *arg)
{
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	eq->status = EASYQ_CONNECTED;
	espconn_regist_disconcb(eq->tcpconn, easyq_tcpclient_disconnect_cb);
    INFO("TCP: Connected to %s:%d\r\n", eq->hostname, eq->port);
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


void ICACHE_FLASH_ATTR
easyq_tcpclient_recon_cb(void *arg, sint8 errType)
{
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
    INFO("TCP: Reconnect to %s:%d\r\n", eq->hostname, eq->port);
	eq->status = EASYQ_RECONNECT_REQ;
}


LOCAL void ICACHE_FLASH_ATTR
easyq_dns_found(const char *name, ip_addr_t *ipaddr, void *arg)
{
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;

    if (ipaddr == NULL)
    {
        INFO("DNS: Found, but got no ip, try to reconnect\r\n");
        eq->status = EASYQ_RECONNECT_REQ;
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
        eq->status = EASYQ_CONNECTING;
        INFO("TCP: connecting...\r\n");
    }

    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}


void ICACHE_FLASH_ATTR easyq_timer(void *arg)
{
    EasyQSession* eq = (EasyQSession*)arg;
	if (eq->status == EASYQ_RECONNECT_REQ) {
        eq->status = EASYQ_RECONNECT;
        system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
    }
	else { 
		INFO("Nothing to do\r\n");
	}
}


void ICACHE_FLASH_ATTR easyq_connect(EasyQSession *eq) {
	eq->status = EASYQ_CONNECTING;
	if (eq->tcpconn) {
		easyq_tcpconn_delete(eq);
	}
    eq->tcpconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    eq->tcpconn->type = ESPCONN_TCP;
    eq->tcpconn->state = ESPCONN_NONE;
    eq->tcpconn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    eq->tcpconn->proto.tcp->local_port = espconn_port();
    eq->tcpconn->proto.tcp->remote_port = eq->port;
    eq->tcpconn->reverse = eq; 
    espconn_regist_connectcb(eq->tcpconn, easyq_tcpclient_connect_cb);
    espconn_regist_reconcb(eq->tcpconn, easyq_tcpclient_recon_cb);

    os_timer_disarm(&eq->timer);
    os_timer_setfn(&eq->timer, (os_timer_func_t *)easyq_timer, eq);
    os_timer_arm(&eq->timer, 1000, 1);

    if (UTILS_StrToIP(eq->hostname, &eq->tcpconn->proto.tcp->remote_ip)) {
        INFO("TCP: Connect to ip  %s:%d\r\n", eq->hostname, eq->port);
        espconn_connect(eq->tcpconn);
    }
    else {
        INFO("TCP: Connect to domain %s:%d\r\n", eq->hostname, eq->port);
        espconn_gethostbyname(eq->tcpconn, eq->hostname, &eq->ip, easyq_dns_found);
    }
}


void ICACHE_FLASH_ATTR
easyq_task(os_event_t *e)
{
    EasyQSession* eq = (EasyQSession*)e->par;
    uint8_t dataBuffer[EASYQ_BUF_SIZE];
    uint16_t dataLen;
    if (e->par == 0)
        return;
    switch (eq->status) {
		case EASYQ_IDLE:
			INFO("EasyQ IDLE\r\n");
	    case EASYQ_RECONNECT_REQ:
	        break;
	    case EASYQ_RECONNECT:
	        easyq_tcpconn_delete(eq);
	        easyq_connect(eq);
	        INFO("TCP: Reconnect to: %s:%d\r\n", eq->hostname, eq->port);
	        eq->status = EASYQ_CONNECTING;
	        break;
	    case EASYQ_DELETING:
	    case EASYQ_DISCONNECTING:
	    case EASYQ_RECONNECT_DISCONNECTING:
	        espconn_disconnect(eq->tcpconn);
	        break;
	    case EASYQ_DISCONNECTED:
	        INFO("EASYQ: Disconnected\r\n");
	        easyq_tcpconn_delete(eq);
	        break;
	    case EASYQ_DELETED:
	        INFO("EASYQ: Deleted client\r\n"); 
			easyq_delete(eq);
	        break;
	    }
}


void ICACHE_FLASH_ATTR
easyq_disconnect(EasyQSession *eq)
{
    eq->status = EASYQ_DISCONNECTING;
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
    os_timer_disarm(&eq->timer);
}


void ICACHE_FLASH_ATTR easyq_init(EasyQSession * eq, const char *hostname, 
		uint16_t port) {
	size_t hostname_len = os_strlen(hostname);
	eq->hostname = (char *)os_malloc(hostname_len + 1);
	os_strcpy(eq->hostname, hostname);
	eq->hostname[hostname_len] = '\0';
	eq->port = port;
	eq->status = EASYQ_IDLE;
	INFO("Preparing EasyQ Server: %s:%d\r\n", hostname, port);

    //QUEUE_Init(&eq->message_queue, QUEUE_BUFFER_SIZE);

    system_os_task(easyq_task, EASYQ_TASK_PRIO, easyq_task_queue, EASYQ_TASK_QUEUE_SIZE);
    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
}
