/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Core API methods implementation.
 */

#include <string>
#include "api_core.h"
#include "api_types.h"
#include "iclass.h"
#include "ihap.h"
#include "ithread.h"
#include "iclock.h"

namespace debugger {

class CoreService : public IService {
public:
    CoreService(const char *name) : IService("CoreService") {}
};

static CoreService core_("core");
static AttributeType Config_;
static AttributeType listClasses_(Attr_List);
static AttributeType listHap_(Attr_List);
static AttributeType listPlugins_(Attr_List);
extern mutex_def mutex_printf;

}  // namespace debugger
