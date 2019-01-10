#ifndef _PARAMS_H__
#define _PARAMS_H__

#include "c_types.h"
#include "partition.h"

#define PARAMS_SECTOR SYSTEM_PARTITION_PARAMS_ADDR / 4096 


typedef struct {
	uint8_t magic;
	uint8_t wifi_ssid[32];
	uint8_t wifi_psk[32];
	uint8_t easyq_host[32];
} Params;


bool ICACHE_FLASH_ATTR 
params_save(Params* params);

bool ICACHE_FLASH_ATTR 
params_load(Params* params);

#endif

