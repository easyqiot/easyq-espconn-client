#ifndef EASYQ_PROTOCOL_H_
#define EASYQ_PROTOCOL_H_

/* State Machine


           +---------+                      +------------+
           |         +---SIG-CONNECT-------->            |
           |  Idle   |                      | Connecting |
     +----->         |                      |            |
     |     +---------+                      +---------+--+
     |                                                |
     |                                       CB-CONNECTED
     |                                                |
     |   +--------------+                   +---------v--+
     |   |              <----CB-CONN-ERROR--+            |
     |   | Reconnecting |                   | Connected  |
     |   |              +----CB-CONNECTED--->            <-----+
     |   +--------------+                   +-+-------+--+     |
     |                                        |       |        |
  CB-DISCONNECTED                   SIG-DISCONNECT    |        |
     |                                        |       |    CB-SENT
     |   +---------------+                    |       |        |
     +---+               <--------------------+       |        |
         | Disconnecting |                            |        |
         |               |                       SIG-SEND      |
         +---------------+                            |        |
                                             +--------v--+     |
                                             |           |     |
                                             | Sending   +-----+
                                             |           |
                                             +-----------+

*/


#include <c_types.h>
#include "easyq.h"

void ICACHE_FLASH_ATTR _easyq_proto_connect(EasyQSession *eq);


#endif
