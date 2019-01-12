/*
 * wifi.h
 *
 */

#ifndef USER_WIFI_H_
#define USER_WIFI_H_

#include <os_type.h>

#ifndef WIFI_ENABLE_SOFTAP
#define WIFI_ENABLE_SOFTAP  false
#endif

typedef void (*WifiCallback)(uint8_t);
void ICACHE_FLASH_ATTR wifi_connect(uint8_t* ssid, uint8_t* pass, 
		WifiCallback cb);


#endif /* USER_WIFI_H_ */
