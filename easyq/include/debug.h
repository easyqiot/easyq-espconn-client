#ifndef _USER_DEBUG_H_
#define _USER_DEBUG_H_

#if defined(GLOBAL_DEBUG_ON)
#include <osapi.h>

#define INFO( format, ... ) os_printf( format, ## __VA_ARGS__ )
#define ERROR( format, ... ) os_printf( format, ## __VA_ARGS__ )
#define FATAL( format, ... ) os_printf( format, ## __VA_ARGS__ )

#else

#define INFO( format, ... )
#define ERROR( format, ... )
#define FATAL( format, ... )

#endif

#endif /* USER_DEBUG_H_ */
