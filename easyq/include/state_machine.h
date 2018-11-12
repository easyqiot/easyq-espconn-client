#ifndef EASYQ_STATE_MACHINE_H_
#define EASYQ_STATE_MACHINE_H_

void ICACHE_FLASH_ATTR
_easyq_start_task();


EasyQError ICACHE_FLASH_ATTR
_easyq_post(EasyQSession *eq, EasyQSignal signal);


#endif
