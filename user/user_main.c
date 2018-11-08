// SDK
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "mem.h"
#include "user_interface.h"

// LIB: EasyQ
#include "easyq.h" 
#include "debug.h"

// Internal 
#include "partition.h"
#include "wifi.h"
#include "config.h"


EasyQSession eq;
ETSTimer status_timer;

static bool sw2_state;


void ICACHE_FLASH_ATTR
status_timer_func() {
	char str[20];
	float vdd = system_get_vdd33() / 1024.0;

	os_sprintf(str, "VDD: %d.%03d", (int)vdd, (int)(vdd*1000)%1000);
	easyq_push(&eq, STATUS_QUEUE, str);
}


void ICACHE_FLASH_ATTR
easyq_message_cb(void *arg, char *queue, char *msg) {
	INFO("EASYQ: Message: %s From: %s\r\n", msg, queue);
	if (strcmp(msg, "mem") == 0) {
		system_print_meminfo();
	}
}


void ICACHE_FLASH_ATTR
easyq_connect_cb(void *arg) {
	INFO("EASYQ: Connected to %s:%d\r\n", eq.hostname, eq.port);
	easyq_pull(&eq, COMMAND_QUEUE);

    os_timer_disarm(&status_timer);
    os_timer_setfn(&status_timer, (os_timer_func_t *)status_timer_func, NULL);
    os_timer_arm(&status_timer, 1000, 1);
}


void ICACHE_FLASH_ATTR
easyq_connection_error_cb(void *arg) {
	EasyQSession *e = (EasyQSession*) arg;
    os_timer_disarm(&status_timer);
	INFO("EASYQ: Connection error: %s:%d\r\n", e->hostname, e->port);
	INFO("EASYQ: Reconnecting to %s:%d\r\n", e->hostname, e->port);
}


void easyq_disconnect_cb(void *arg)
{
	EasyQSession *e = (EasyQSession*) arg;
    os_timer_disarm(&status_timer);
	INFO("EASYQ: Disconnected from %s:%d\r\n", e->hostname, e->port);
}


void wifi_connect_cb(uint8_t status) {
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



void ICACHE_FLASH_ATTR
sw2_interrupt() {
	ETS_GPIO_INTR_DISABLE();
	uint16_t status;
	
	// Clear the interrupt
	status = GPIO_REG_READ(GPIO_STATUS_ADDRESS);
	GPIO_REG_WRITE(GPIO_STATUS_W1TC_ADDRESS, status);

	bool level = GPIO_INPUT_GET(SW2_NUM);
	if (level ^ sw2_state) {
		sw2_state = level;
		INFO("\r\nSW2: %s\r\n", level? "ON": "OFF"); 
	}

	os_delay_us(500000);
	
	ETS_GPIO_INTR_ENABLE();
}


void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);

	PIN_FUNC_SELECT(SW2_MUX, SW2_FUNC);
	//PIN_PULLUP_DIS(SW2_MUX);
	GPIO_DIS_OUTPUT(GPIO_ID_PIN(SW2_NUM));
	ETS_GPIO_INTR_DISABLE();
	ETS_GPIO_INTR_ATTACH(sw2_interrupt, NULL);
	gpio_pin_intr_state_set(GPIO_ID_PIN(SW2_NUM), GPIO_PIN_INTR_ANYEDGE);
	ETS_GPIO_INTR_ENABLE();

	EasyQError err = easyq_init(&eq, EASYQ_HOSTNAME, EASYQ_PORT, EASYQ_LOGIN);
	if (err != EASYQ_OK) {
		ERROR("EASYQ INIT ERROR: %d\r\n", err);
		return;
	}
    eq.onconnect = easyq_connect_cb;
    eq.ondisconnect = easyq_disconnect_cb;
	eq.onconnectionerror = easyq_connection_error_cb;
	eq.onmessage = easyq_message_cb;

    WIFI_Connect(WIFI_SSID, WIFI_PSK, wifi_connect_cb);
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


