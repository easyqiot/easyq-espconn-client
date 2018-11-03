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
    { SYSTEM_PARTITION_BOOTLOADER, 	0x0, 0x1000},
    { SYSTEM_PARTITION_OTA_1, 0x1000, SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_OTA_2, SYSTEM_PARTITION_OTA_2_ADDR, SYSTEM_PARTITION_OTA_SIZE},
    { SYSTEM_PARTITION_RF_CAL, SYSTEM_PARTITION_RF_CAL_ADDR, 0x1000},
    { SYSTEM_PARTITION_PHY_DATA, SYSTEM_PARTITION_PHY_DATA_ADDR, 0x1000},
    { SYSTEM_PARTITION_SYSTEM_PARAMETER, SYSTEM_PARTITION_SYSTEM_PARAMETER_ADDR, 0x3000},
};

void ICACHE_FLASH_ATTR user_pre_init(void)
{
    if(!system_partition_table_regist(
				at_partition_table, 
				sizeof(at_partition_table)/sizeof(at_partition_table[0]),
				SPI_FLASH_SIZE_MAP)) {
		FATAL("system_partition_table_regist fail\r\n");
		while(1);
	}
}


EasyQSession eq;
static ETSTimer sntp_timer;

void sntpfn()
{
    uint32_t ts = 0;
    ts = sntp_get_current_timestamp();
    INFO("current time : %s\n", sntp_get_real_time(ts));
    if (ts == 0) {
        ERROR("did not get a valid time from sntp server\n");
    } else {
        os_timer_disarm(&sntp_timer);
        easyq_connect(&eq);
    }
}

void wifiConnectCb(uint8_t status)
{
    if(status == STATION_GOT_IP){
        sntp_setservername(0, "pool.ntp.org");        // set sntp server after got ip address
        sntp_init();
        os_timer_disarm(&sntp_timer);
        os_timer_setfn(&sntp_timer, (os_timer_func_t *)sntpfn, NULL);
        os_timer_arm(&sntp_timer, 1000, 1);//1s
    } else {
        easyq_disconnect(&eq);
    }
}

//void easyq_connect_cb(EasyQSession* eq)
//{
//    INFO("EasyQ: Connected\r\n");
//    easyq_pull(eq, "q1");
//    easyq_push(eq, "q1", "hello\0");
//}
//
//void easyq_disconnected_cb(EasyQSession* eq)
//{
//    INFO("EASYQ: Disconnected\r\n");
//}
//
//void easyq_push_cb(EasyQSession* eq)
//{
//    INFO("EASYQ: SENT\r\n");
//}
//
//void easyq_pull_cb(EasyQSession* eq, EasyQueue * queue, const char *data, uint32_t data_len)
//{
//    INFO("Receive Message: %s, data: %s \r\n", queue->title, data);
//}

void user_init(void)
{
    uart_init(BIT_RATE_115200, BIT_RATE_115200);
    os_delay_us(60000);
	easyq_init(&eq, "192.168.8.44", 1085);

//    easyq_onconnected(&eq, easyq_connect_cb);
//    easyq_ondisconnected(&eq, easyq_disconnect_cb);
//    easyq_onpublished(&eq, easyq_push_cb);
//    easyq_ondata(&eq, easyq_pull_cb);

    WIFI_Connect("Jigoodi", "himopolooK905602", wifiConnectCb);
    INFO("\r\nSystem started ...\r\n");
}
