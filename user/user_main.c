// SDK
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "mem.h"
#include "user_interface.h"

// EasyQ
#include "easyq.h"

// Modules
#include "partition.h"
#include "wifi.h"
#include "debug.h"


EasyQSession eq;


void wifiConnectCb(uint8_t status) {
	EasyQError err;
    if(status == STATION_GOT_IP) {
        err = easyq_connect(&eq);
		if (err != EASYQ_OK) {
			ERROR("EASYQ CONNECT ERROR: %d\r\n", err);
			easyq_disconnect(&eq);
		}
    } else {
        easyq_disconnect(&eq);
    }
}


void easyq_connect_cb(void *arg) {
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Connected to %s:%d\r\n", e->hostname, e->port);
    //easyq_pull(eq, "q1");
    //easyq_push(eq, "q1", "hello\0");
}


void easyq_connection_error_cb(void *arg) {
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Connection error: %s:%d\r\n", e->hostname, e->port);
	INFO("EASYQ: Reconnecting to %s:%d\r\n", e->hostname, e->port);
}


void easyq_disconnect_cb(void *arg)
{
	EasyQSession *e = (EasyQSession*) arg;
	INFO("EASYQ: Disconnected from %s:%d\r\n", e->hostname, e->port);
}


void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	EasyQError err = easyq_init(&eq, "192.168.8.44", 1085);
	if (err != EASYQ_OK) {
		ERROR("EASYQ INIT ERROR: %d\r\n", err);
		return;
	}
    eq.onconnect = easyq_connect_cb;
    eq.ondisconnect = easyq_disconnect_cb;
	eq.onconnectionerror = easyq_connection_error_cb;

    WIFI_Connect("Jigoodi", "himopolooK905602", wifiConnectCb);
    INFO("System started ...\r\n");
}


void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		FATAL("system_partition_table_regist fail\r\n");
		while(1);
	}
}


