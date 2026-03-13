#include "entities.h"
#include <elc/core.h>

#ifndef _SYSTEM_COMPONENTS_MACRO
#define _SYSTEM_COMPONENTS_MACRO X(COMPONENT_EMPTY, ELCEmptyStruct)
#endif

#ifndef _SYSTEM_NAMING_MACRO
#define _SYSTEM_NAMING_MACRO(base) base
#endif

#undef X
#define X(name, type) name,
enum {
    _SYSTEM_COMPONENTS_MACRO
    _SYSTEM_NAMING_MACRO(SYSTEM_COMPONENTS_MAX_ENUM_)
};
#undef X
#define X(name, type) registerComponentReserved(entities, &reserved, sizeof(type));
ELC_INLINE Component _SYSTEM_NAMING_MACRO(registerComponents)(Entities* entities) {
    Component reserved = reserveComponents(entities, _SYSTEM_NAMING_MACRO(SYSTEM_COMPONENTS_MAX_ENUM_));
    _SYSTEM_COMPONENTS_MACRO
    return reserved;
}
#undef X

#undef _SYSTEM_NAMING_MACRO
#undef _SYSTEM_COMPONENTS_MACRO
