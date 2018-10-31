#include "easyq.h"

void ICACHE_FLASH_ATTR easyq_connect(EasyQSession *eq, EasyQCallback callback) {
// TODO: Continue from here
//	espconn_gethostbyname(c.tcpconn, host, &dweet_ip, dns_done );	

}

void ICACHE_FLASH_ATTR easyq_init(EasyQSession *eq, const char *hostname, 
		uint16_t port) {
	size_t hostname_len = os_strlen(hostname);
	eq = (EasyQSession *)os_malloc(sizeof(EasyQSession));
	eq->hostname = (char *)os_malloc_dram(hostname_len + 1);
	os_strcpy(eq->hostname, hostname);
	eq->hostname[hostname_len] = '\0';
	eq->port = port;
	eq->tcpconn = (struct espconn*) os_malloc(sizeof(struct espconn));
}
