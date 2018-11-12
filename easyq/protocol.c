#include "protocol.h"
#include "easyq.h"
#include "debug.h"

#include <mem.h>
#include <espconn.h>
#include <string.h>


LOCAL void ICACHE_FLASH_ATTR 
_easyq_proto_process_message(EasyQSession *eq) {
	if (eq->recvbuffer_length < 16 || 
			os_strncmp(eq->recv_buffer, "MESSAGE ", 8) != 0) {
		ERROR("Invalid message length: %d or format: %s \r\n", 
				eq->recvbuffer_length, eq->recv_buffer);
        return;
    }

    char *queue = strrchr(eq->recv_buffer, ' ');
    if (queue == NULL) {
		ERROR("Invalid queue\r\n");
		return;
    }
	queue++;
	char *message = eq->recv_buffer + 8;
	uint16_t message_len = queue - message - 6;
	message[message_len] = 0;
	if (eq->onmessage) {
		eq->onmessage(eq, queue, message, message_len);
	}
}



LOCAL void ICACHE_FLASH_ATTR
_easyq_proto_logged_in(EasyQSession *eq, char *session_id, uint8_t id_len) { 
	INFO("Loggen In, session ID: %s\r\n", session_id);
	_easyq_task_post(eq, EASYQ_SIG_CONNECTED);
	// TODO: Store session id inside the eq session.
}


LOCAL void ICACHE_FLASH_ATTR
_easyq_proto_server_error(EasyQSession *eq, char *error, uint8_t error_len) { 
	INFO("SERVER ERROR: %s\r\n", error);
}


void
_easyq_tcpclient_recv_cb(void *arg, char *pdata, unsigned short len) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	
	if (pdata[len-1] == '\n') {
		len--;
	}

	if (pdata[len-1] != ';') {
		ERROR("EASYQ: INVALID MESSAGE\r\n");
		return;
	}
	len--;
	eq->recv_buffer[len] = 0;

	os_memcpy(eq->recv_buffer, pdata, len);
	eq->recvbuffer_length = len;
	if(os_strncmp(eq->recv_buffer, "HI ", 3) == 0) {
		// Logged In
		_easyq_proto_logged_in(eq, eq->recv_buffer + 3, len - 3);
	} 
	else if(os_strncmp(eq->recv_buffer, "ERROR: ", 7) == 0) {
		// Server Error 
		_easyq_proto_server_error(eq, eq->recv_buffer + 7, len - 7);
	} 
	else {
		// Message received
		_easyq_proto_process_message(eq);
	}
}


void 
_easyq_proto_send_buffer(EasyQSession *eq) {
	int8_t err = espconn_send(eq->tcpconn, eq->send_buffer, 
			eq->sendbuffer_length);
	if (err != ESPCONN_OK) {
		ERROR("TCP SEND ERROR: %d, Retrying\r\n", err);
		// TODO: use timer to send data some more times.
	}
}


void
_easyq_tcpclient_sent_cb(void *arg) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	
	// Clear the buffer for future sending.
	os_memset(eq->send_buffer, 0, EASYQ_SEND_BUFFER_SIZE);
	_easyq_task_post(eq, EASYQ_SIG_SENT);	
}


void 
_easyq_tcpclient_connection_error_cb(void *arg, sint8 errType) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	_easyq_task_post(eq, EASYQ_SIG_RECONNECT);
}


void
_easyq_tcpclient_disconnect_cb(void *arg) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	_easyq_task_post(eq, EASYQ_SIG_DISCONNECTED);
}


void
_easyq_tcpclient_connect_cb(void *arg) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;
	espconn_regist_recvcb(eq->tcpconn, _easyq_tcpclient_recv_cb);
	espconn_regist_sentcb(eq->tcpconn, _easyq_tcpclient_sent_cb);
	espconn_regist_disconcb(eq->tcpconn, _easyq_tcpclient_disconnect_cb);
	
	os_memset(eq->send_buffer, 0, EASYQ_SEND_BUFFER_SIZE);
	eq->sendbuffer_length = os_strlen(eq->login) + 8;
	os_snprintf(eq->send_buffer, eq->sendbuffer_length, 
			"LOGIN %s;\n", eq->login);
	_easyq_proto_send_buffer(eq);
}


void ICACHE_FLASH_ATTR
_easyq_proto_delete(EasyQSession *eq) {
    if (eq->tcpconn != NULL) {
        espconn_delete(eq->tcpconn);
        if (eq->tcpconn->proto.tcp)
            os_free(eq->tcpconn->proto.tcp);
        os_free(eq->tcpconn);
        eq->tcpconn = NULL;
    }
}


LOCAL void ICACHE_FLASH_ATTR
_easyq_dns_found(const char *name, ip_addr_t *ipaddr, void *arg) {
    struct espconn *tcpconn = (struct espconn *)arg;
    EasyQSession *eq = (EasyQSession *)tcpconn->reverse;

    if (ipaddr == NULL)
    {
        ERROR("DNS: Found, but got no ip, try to reconnect\r\n");
		_easyq_proto_delete(eq);
		_easyq_task_post(eq, EASYQ_SIG_CONNECT);
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
_easyq_proto_connect(EasyQSession *eq) {
    eq->tcpconn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    eq->tcpconn->type = ESPCONN_TCP;
    eq->tcpconn->state = ESPCONN_NONE;
    eq->tcpconn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    eq->tcpconn->proto.tcp->local_port = espconn_port();
    eq->tcpconn->proto.tcp->remote_port = eq->port;
    eq->tcpconn->reverse = eq; 
    espconn_regist_connectcb(eq->tcpconn, _easyq_tcpclient_connect_cb);
    espconn_regist_reconcb(eq->tcpconn, _easyq_tcpclient_connection_error_cb);

    if (UTILS_StrToIP(eq->hostname, &eq->tcpconn->proto.tcp->remote_ip)) {
        espconn_connect(eq->tcpconn);
    }
    else {
        espconn_gethostbyname(eq->tcpconn, eq->hostname, &eq->ip, 
				_easyq_dns_found);
    }
}


void ICACHE_FLASH_ATTR
_easyq_proto_reconnect(EasyQSession *eq) { 
	// Giving a chance to users to examine what happened before deleting the
	// espconn structure.
	if (eq->onconnectionerror) {
		eq->onconnectionerror(eq);
	}
	_easyq_proto_delete(eq);
}


void ICACHE_FLASH_ATTR 
_easyq_proto_disconnect(EasyQSession *eq) {
    espconn_disconnect(eq->tcpconn);
}

