/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      UDP transport level implementation.
 */

#ifndef __DEBUGGER_UDP_SERVICE_H__
#define __DEBUGGER_UDP_SERVICE_H__

#include "api_types.h"
#include "attribute.h"
#include "irawlistener.h"
//#include <vector>

namespace debugger {

    void UdpService_UdpService();

    /** IService interface */
    void UdpService_postinitService();

    /** IUdp interface */
    AttributeType UdpService_getConnectionSettings();

    void UdpService_setTargetSettings(const AttributeType *target);

    /**
     * @brief Setup socket mode.
     * @param[in] mode New value:
     *                     true: Blocking mode
     *                     false: Non-Blocking mode
     * @return true value on success.
     */
    bool UdpService_setBlockingMode(socket_def h, bool mode);

    int UdpService_sendData(const uint8_t *msg, int len);

    int UdpService_readData(const uint8_t *buf, int maxlen);

    int UdpService_registerListener(IRawListener *ilistener);

    int UdpService_createDatagramSocket();
    void UdpService_closeDatagramSocket();

}  // namespace debugger

#endif  // __DEBUGGER_UDP_SERVICE_H__
