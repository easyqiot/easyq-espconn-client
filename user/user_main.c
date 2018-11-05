// SDK
#include "ets_sys.h"
#include "osapi.h"
#include "gpio.h"
#include "mem.h"
#include "user_interface.h"

// EasyQ
#include "easyq.h"

// Modules
#include "wifi.h"
#include "debug.h"

// User
#include "partition.c"


EasyQSession eq;


void wifiConnectCb(uint8_t status) {
    if(status == STATION_GOT_IP) {
        easyq_connect(&eq);
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
    INFO("System started ...\r\n");
}
