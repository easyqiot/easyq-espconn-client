#include <mem.h>
//#include "espconn.h"
//#include "c_types.h"

#include "easyq.h"
#include "debug.h"
//#include "utils.h"  


/* Schedules a connect request 
 */
EasyQError ICACHE_FLASH_ATTR 
easyq_connect(EasyQSession *eq) {
	return _easyq_task_post(eq, EASYQ_SIG_CONNECT);
}


/* Schedules a disconnect request
 */
EasyQError ICACHE_FLASH_ATTR 
easyq_disconnect(EasyQSession *eq) {
	return _easyq_task_post(eq, EASYQ_SIG_DISCONNECT);
}


/* Alocate memory and inititalize the task
 */
EasyQError ICACHE_FLASH_ATTR 
easyq_init(EasyQSession *eq, const char *hostname, uint16_t port, 
		const char *login) {
	if (eq->hostname) {
		return EASYQ_ERR_ALREADY_INITIALIZED;
	}

	size_t len = os_strlen(hostname);
	eq->hostname = (char *)os_malloc(len + 1);
	os_strcpy(eq->hostname, hostname);
	eq->hostname[len] = 0;

	len = os_strlen(login);
	eq->login = (char *)os_malloc(len + 1);
	os_strcpy(eq->login, login);
	eq->login[len] = 0;

	eq->recv_buffer = (char*)os_malloc(EASYQ_RECV_BUFFER_SIZE);
	eq->send_buffer = (char*)os_malloc(EASYQ_SEND_BUFFER_SIZE);

	eq->port = port;
	eq->ticks = 0;
	eq->status = EASYQ_IDLE;
	_easyq_task_start();
	return EASYQ_OK;
}


///* Disconnect and dealocate the memory.
// */
//EasyQError ICACHE_FLASH_ATTR 
//easyq_delete(EasyQSession *eq) {
//	EasyQError err = _easyq_validate_transition(eq->status, EASYQ_DELETE);
//	if (err != EASYQ_OK) {
//		return err;
//	}
//	eq->status = EASYQ_DELETE;
//    system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
//	return EASYQ_OK;
//}
//
//
//EasyQError ICACHE_FLASH_ATTR
//easyq_wait_for_send(EasyQSession *eq, uint8_t tries) {
//	while (eq->status != EASYQ_CONNECTED) {
//		tries--;
//		if (tries < 1) {
//			return EASYQ_NOT_READY_FOR_SEND;
//		}
//		INFO("EASYQ: waiting for send, try: %d\r\n", tries);
//		os_delay_us(10000);
//	}
//	return EASYQ_OK;
//}
//
//
//EasyQError ICACHE_FLASH_ATTR
//easyq_pull(EasyQSession *eq, const char *queue) { 
//	size_t qlen = os_strlen(queue);
//	os_sprintf(eq->send_buffer, "PULL FROM %s;\n", queue);
//	eq->sendbuffer_length = 13 + qlen;
//	eq->status = EASYQ_SEND;
//	system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
//	return EASYQ_OK;
//}
//
//
//EasyQError ICACHE_FLASH_ATTR
//easyq_pull_all(EasyQSession *eq, const char **queues, size_t queue_count) { 
//	int i;
//	int len = 0;
//	char temp[32];
//	
//	EasyQError err = easyq_wait_for_send();
//	if (err) {
//		return err;
//	}
//
//	for (i = 0; i < queue_count; i++) {
//		INFO("Pulling: %s\r\n", queues[i]);
//		size_t l = os_strlen(queues[i]);
//		if (len+l+12 >= EASYQ_SEND_BUFFER_SIZE) {
//			break;		
//		}
//		os_sprintf(temp, "PULL FROM %s;", queues[i]);
//		os_strcat(eq->send_buffer, temp);
//		len += (12 + l);
//	}
//	eq->send_buffer[len] = '\n';
//
//	eq->send_buffer[len+1] = '\0';
//	INFO("%s\r\n", eq->send_buffer);
//
//	eq->sendbuffer_length = len + 1;
//	eq->status = EASYQ_SEND;
//	system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
//	return EASYQ_OK;
//}
//
//
//
//EasyQError ICACHE_FLASH_ATTR
//easyq_ignore(EasyQSession *eq, const char *queue) {
//	size_t qlen = os_strlen(queue);
//
//	EasyQError err = easyq_wait_for_send();
//	if (err) {
//		return err;
//	}
//
//	os_sprintf(eq->send_buffer, "IGNORE %s;\n", queue);
//	eq->sendbuffer_length = 9 + qlen;
//	eq->status = EASYQ_SEND;
//	system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
//	return EASYQ_OK;
//}
//
//
//EasyQError ICACHE_FLASH_ATTR
//easyq_push(EasyQSession *eq, const char *queue, const char *message) {
//	size_t qlen = os_strlen(queue);
//	size_t msglen = os_strlen(message);
//
//	EasyQError err = easyq_wait_for_send();
//	if (err) {
//		return err;
//	}
//
//	os_sprintf(eq->send_buffer, "PUSH %s INTO %s;\n", message, queue);
//	eq->sendbuffer_length = 13 + qlen + msglen;
//	eq->status = EASYQ_SEND;
//	system_os_post(EASYQ_TASK_PRIO, 0, (os_param_t)eq);
//	return EASYQ_OK;
//}
//


