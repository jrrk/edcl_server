/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Access to a hardware via Ethernet EDCL interface implementaion.
 */

#ifndef __DEBUGGER_EDCL_H__
#define __DEBUGGER_EDCL_H__

//#include "iclass.h"
//#include "iservice.h"
#include "udp.h"
#include <inttypes.h>

namespace debugger {

  /*
struct EdclService  {
public:
    EdclService();
  */
    /** ITap interface */
     int EdclService_read(uint64_t addr, int bytes, uint8_t *obuf);
     int EdclService_write(uint64_t addr, int bytes, uint8_t *ibuf);

     //private:
    int EdclService_write16(uint8_t *buf, int off, uint16_t v);
    int EdclService_write32(uint8_t *buf, int off, uint32_t v);
    uint32_t EdclService_read32(uint8_t *buf);

    /** This is limitation of the MAC fifo. Protocol allows increase the
     * following value up to 242 words. */
    static const int EDCL_PAYLOAD_MAX_WORDS32 = 8;
    static const int EDCL_PAYLOAD_MAX_BYTES  = 4*EDCL_PAYLOAD_MAX_WORDS32;

    //};

}  // namespace debugger

#endif  // __DEBUGGER_EDCL_H__
