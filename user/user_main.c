#include "ets_sys.h"
#include "driver/uart.h"
#include "osapi.h"
#include "wifi.h"
#include "debug.h"
#include "gpio.h"
#include "user_interface.h"
#include "mem.h"

#include "easyq.h"

#if (SPI_FLASH_SIZE_MAP == 2)
#define SYSTEM_PARTITION_OTA_SIZE							0x6A000
#define SYSTEM_PARTITION_OTA_2_ADDR							0x81000
#define SYSTEM_PARTITION_RF_CAL_ADDR						0xfb000
#define SYSTEM_PARTITION_PHY_DATA_ADDR						0xfc000
#define SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR				0xfd000
#else
#error "The flash map is not supported"
#endif

static const partition_item_t at_partition_table[] = {
    { SYSTEM_PARTITION_BOOTLOADER, 			0x0, 										0x1000},
    { SYSTEM_PARTITION_OTA_1,   			0x1000, 									SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2,   			SYSTEM_PARTITION_OTA_2_ADDR, 				SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL,  			SYSTEM_PARTITION_RF_CAL_ADDR, 				0x1000},
    { SYSTEM_PARTITION_PHY_DATA, 			SYSTEM_PARTITION_PHY_DATA_ADDR, 			0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER,	SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 	0x3000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(
				at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		os_printf("system_partition_table_regist fail\r\n");
		while(1);
	}
}


static ETSTimer eq_timer;
static EasyQSession *eq;


void easyq_callback(EasyQStatus status) {
	switch (status) {
		case EASYQ_CONNECTING:
			INFO("EasyQ connecting.\r\n");
		case EASYQ_CONNECTED:
			INFO("EasyQ connected.\r\n");
		case EASYQ_IDLE:
			INFO("EasyQ Idle.\r\n");
	}
}


void easyq_task() {
	if (eq->tcpconn == NULL) {
		INFO("Not connected, Connecting...\r\n");
		easyq_connect(eq, easyq_callback);
	} else {
		INFO("Nothing to do\r\n");
	}
}

void wifiConnectCb(uint8_t status)
{
    if(status == STATION_GOT_IP){
        os_timer_disarm(&eq_timer);
        os_timer_setfn(&eq_timer, (os_timer_func_t *)easyq_task, NULL);
        os_timer_arm(&eq_timer, 3000, 1); //3s
    } else {
        os_timer_disarm(&eq_timer);
    }
}

void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	easyq_init(eq, "192.168.8.44", 1085);
    WIFI_Connect("yana", "himopolooK905602", wifiConnectCb);
    INFO("\r\nSystem started ...\r\n");
}
