/**
 * \file object.h
 *
 * \brief This is the object header of a public persistent object 
 */

#ifndef __STAMNOS_DPO_COMMON_OBJECT_H
#define __STAMNOS_DPO_COMMON_OBJECT_H

#include <stdint.h>

namespace dpo {

typedef uint16_t ObjectType;

class Object: public dpo::cc::common::Object {
public:

	ObjectType magic_;

};

} // namespace dpo

#endif // __STAMNOS_DPO_COMMON_OBJECT_H
