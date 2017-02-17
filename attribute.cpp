/**
 * @file
 * @copyright  Copyright 2016 GNSS Sensor Ltd. All right reserved.
 * @author     Sergey Khabarov - sergeykhbr@gmail.com
 * @brief      Core attribute methods implementation.
 */

#include "api_core.h"
#include "autobuffer.h"
#include "iservice.h"
#include <cstdlib>

namespace debugger {

static AttributeType NilAttribute;
static AutoBuffer strBuffer;

char *attribute_to_string(const AttributeType *attr);

void AttributeType::attr_free() {
    if (size()) {
        if (is_string()) {
            delete [] u_.string;
        } else if (is_data()) {
            delete [] u_.data;
        } else if (is_list()) {
            for (unsigned i = 0; i < size(); i++) {
                u_.list[i].attr_free();
            }
            delete [] u_.list;
        } else if (is_dict()) {
            for (unsigned i = 0; i < size(); i++) {
                u_.dict[i].key_.attr_free();
                u_.dict[i].value_.attr_free();
            }
            delete [] u_.dict;
        }
    }
    kind_ = Attr_Invalid;
    size_ = 0;
}

void AttributeType::clone(const AttributeType *v) {
    attr_free();

    if (v->is_string()) {
        this->make_string(v->to_string());
    } else if (v->is_data()) {
        this->make_data(v->size(), v->data());
    } else if (v->is_list()) {
        make_list(v->size());
        for (unsigned i = 0; i < v->size(); i++ ) {
            u_.list[i].clone(v->list(i));
        }
    } else if (v->is_dict()) {
        make_dict();
        realloc_dict(v->size());
        for (unsigned i = 0; i < v->size(); i++ ) {
            u_.dict[i].key_.make_string(v->dict_key(i)->to_string());
            u_.dict[i].value_.clone(v->dict_value(i));
        }
    } else {
        this->kind_ = v->kind_;
        this->u_ = v->u_;
        this->size_ = v->size_;
    }
}

AttributeType &AttributeType::operator=(const AttributeType& other) {
    if (&other != this) {
        clone(&other);
    }
    return *this;
}


const AttributeType &AttributeType::operator[](unsigned idx) const {
    if (is_list()) {
        return u_.list[idx];
    } else if (is_dict()) {
        return u_.dict[idx].value_;
    } else {
        RISCV_printf(NULL, LOG_ERROR, "%s", "Non-indexed attribute type");
    }
    return NilAttribute;
}

AttributeType &AttributeType::operator[](unsigned idx) {
    if (is_list()) {
        return u_.list[idx];
    } else if (is_dict()) {
        return u_.dict[idx].value_;
    } else {
        RISCV_printf(NULL, LOG_ERROR, "%s", "Non-indexed attribute type");
    }
    return NilAttribute;
}

const AttributeType &AttributeType::operator[](const char *key) const {
    for (unsigned i = 0; i < size(); i++) {
        if (strcmp(key, u_.dict[i].key_.to_string()) == 0) {
            return u_.dict[i].value_;
        }
    }
    AttributeType *pthis = const_cast<AttributeType*>(this);
    pthis->realloc_dict(size()+1);
    pthis->u_.dict[size()-1].key_.make_string(key);
    pthis->u_.dict[size()-1].value_.make_nil();
    return u_.dict[size()-1].value_;
}

AttributeType &AttributeType::operator[](const char *key) {
    for (unsigned i = 0; i < size(); i++) {
        if (strcmp(key, u_.dict[i].key_.to_string()) == 0) {
            return u_.dict[i].value_;
        }
    }
    realloc_dict(size()+1);
    u_.dict[size()-1].key_.make_string(key);
    u_.dict[size()-1].value_.make_nil();
    return u_.dict[size()-1].value_;
}

const uint8_t &AttributeType::operator()(unsigned idx) const {
    if (idx > size()) {
        RISCV_printf(NULL, LOG_ERROR, "Data index '%d' out of range.", idx);
        return u_.data[0];
    }
    return u_.data[idx];
}

bool AttributeType::has_key(const char *key) const {
    for (unsigned i = 0; i < size(); i++) {
        if (strcmp(u_.dict[i].key_.to_string(), key) == 0
            && !u_.dict[i].value_.is_nil()) {
            return true;
        }
    }
    return false;
}

const AttributeType *AttributeType::dict_key(unsigned idx) const {
    return &u_.dict[idx].key_;
}
AttributeType *AttributeType::dict_key(unsigned idx) {
    return &u_.dict[idx].key_;
}

const AttributeType *AttributeType::dict_value(unsigned idx) const {
    return &u_.dict[idx].value_;
}
AttributeType *AttributeType::dict_value(unsigned idx) {
    return &u_.dict[idx].value_;
}


void AttributeType::make_dict() {
    kind_ = Attr_Dict;
    size_ = 0;
    u_.dict = NULL;
}

void AttributeType::realloc_dict(unsigned length) {
    AttributePairType * t1 = new AttributePairType[length];
    for (unsigned i = 0; i < size_; i++) {
        t1[i].key_.clone(&u_.dict[i].key_);
        t1[i].value_.clone(&u_.dict[i].value_);
    }
    if (size_) {
        delete [] u_.dict;
    }
    u_.dict = t1;
    size_ = length;
}

char *AttributeType::to_config() {
    strBuffer.clear();
    attribute_to_string(this);
    return strBuffer.getBuffer();
}

char *attribute_to_string(const AttributeType *attr) {
    IService *iserv;
    AutoBuffer *buf = &strBuffer;
    if (attr->is_nil()) {
        buf->write_string("None");
    } else if (attr->is_int64() || attr->is_uint64()) {
        buf->write_uint64(attr->to_uint64());
    } else if (attr->is_string()) {
        buf->write_string('\'');
        buf->write_string(attr->to_string());
        buf->write_string('\'');
    } else if (attr->is_bool()) {
        if (attr->to_bool()) {
            buf->write_string("true");
        } else {
            buf->write_string("false");
        }
    } else if (attr->is_list()) {
        AttributeType list_item;
        unsigned list_sz = attr->size();
        buf->write_string('[');
        for (unsigned i = 0; i < list_sz; i++) {
            list_item = (*attr)[i];
            attribute_to_string(&list_item);
            if (i < (list_sz - 1)) {
                buf->write_string(',');
            }
        }
        buf->write_string(']');
    } else if (attr->is_dict()) {
        AttributeType dict_item;
        unsigned dict_sz = attr->size();;
        buf->write_string('{');

        for (unsigned i = 0; i < dict_sz; i++) {
            buf->write_string('\'');
            buf->write_string(attr->u_.dict[i].key_.to_string());
            buf->write_string('\'');
            buf->write_string(':');
            const AttributeType &dict_value = (*attr)[i];
            attribute_to_string(&dict_value);
            if (i < (dict_sz - 1)) {
                buf->write_string(',');
            }
        }
        buf->write_string('}');
    } else if (attr->is_data()) {
        buf->write_string('(');
        if (attr->size() > 0) {
            for (unsigned n = 0; n < attr->size()-1;  n++) {
                buf->write_byte((*attr)(n));
                buf->write_string(',');
            }
            buf->write_byte((*attr)(attr->size()-1));
        }
        buf->write_string(')');
    } else if (attr->is_iface()) {
        
        IFace *iface = attr->to_iface();
        if (strcmp(iface->getFaceName(), IFACE_SERVICE) == 0) {
            iserv = static_cast<IService *>(iface);
            buf->write_string('{');
            buf->write_string("'Type':'");
            buf->write_string(iface->getFaceName());
            buf->write_string("','ModuleName':'");
            buf->write_string(iserv->getObjName());
            buf->write_string("'}");
        } else {
            RISCV_printf(NULL, LOG_ERROR, 
                        "Not implemented interface to dict. method");
        }

    }
    return buf->getBuffer();
}

const char *skip_special_symbols(const char *cfg) {
    const char *pcur = cfg;
    while (*pcur == ' ' || *pcur == '\r' || *pcur == '\n' || *pcur == '\t') {
        pcur++;
    }
    return pcur;
}

}  // namespace debugger
