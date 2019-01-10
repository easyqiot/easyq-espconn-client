#include <c_types.h>
#include <user_interface.h>
#include "debug.h"
#include "params.h"


bool ICACHE_FLASH_ATTR 
params_save(Params* params) {
	params->magic = 'x';
	return system_param_save_with_protect(PARAMS_SECTOR, params, 
			sizeof(Params));
}


bool ICACHE_FLASH_ATTR 
params_load(Params* params) {
	bool ok = system_param_load(PARAMS_SECTOR, 0,
			params, sizeof(Params));
	return ok && params->magic == 'x';
}

