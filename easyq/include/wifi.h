/*
 * wifi.h
 *
 */

#ifndef USER_WIFI_H_
#define USER_WIFI_H_

//#include <osapi.h>
//#include <espconn.h>
//#include <mem.h>
//#include <ets_sys.h>
//#include <c_types.h>
#include <user_interface.h>
#include <os_type.h>


#define WIFI_SOFTAP_CHANNEL		7
#define WIFI_SOFTAP_PSK			"esp-8266"


typedef void (*WifiCallback)(uint8_t);
void ICACHE_FLASH_ATTR wifi_start(uint8_t opmode, const char *device_name, 
		uint8_t* ssid, uint8_t* pass, WifiCallback cb);

struct dhcp_client_info {
	ip_addr_t ip_addr;
	ip_addr_t netmask;
	ip_addr_t gw;
	uint8 flag;
	uint8 pad[3];
};



#endif /* USER_WIFI_H_ */
