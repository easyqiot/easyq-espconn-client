
#include "easyq.h"
#include "debug.h"


void ICACHE_FLASH_ATTR easyq_connect(EasyQSession *eq, EasyQCallback callback) {
// TODO: Continue from here
//	espconn_gethostbyname(c.tcpconn, host, &dweet_ip, dns_done );	

}

EasyQSession * ICACHE_FLASH_ATTR easyq_init(const char *hostname, 
		uint16_t port) {
	size_t hostname_len = os_strlen(hostname);
	INFO("Hostname: %s %d\r\n", hostname, hostname_len);
	EasyQSession * eq = (EasyQSession *)os_malloc(sizeof(EasyQSession));
	eq->hostname = (char *)os_malloc(hostname_len + 1);
	os_strcpy(eq->hostname, hostname);
	eq->hostname[hostname_len] = '\0';
	eq->port = port;
	INFO("Port: %d\r\n", eq->port);
	//eq->conn = (struct espconn*) os_malloc(sizeof(struct espconn));
	
    eq->conn = (struct espconn *)os_zalloc(sizeof(struct espconn));
    eq->conn->type = ESPCONN_TCP;
    eq->conn->state = ESPCONN_NONE;
    eq->conn->proto.tcp = (esp_tcp *)os_zalloc(sizeof(esp_tcp));
    eq->conn->proto.tcp->local_port = espconn_port();
    eq->conn->proto.tcp->remote_port = port;
    eq->conn->reverse = mqttClient;
    espconn_regist_connectcb(eq->conn, mqtt_tcpclient_connect_cb);
    espconn_regist_reconcb(eq->conn, mqtt_tcpclient_recon_cb);

	return eq;
}
