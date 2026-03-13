#ifndef ENGINE_PHYSICS_PHYSICS_H
#define ENGINE_PHYSICS_PHYSICS_H

#include "particle.h"
#include "constraints.h"

#define _SYSTEMS_NAMING_MACRO(base) base##Physics
#define _SYSTEMS_COMPONENTS_MACRO\
    X(COMPONENT_PHYSICS, PhysicsComponent)\
    X(COMPONENT_CONSTRAINT, ConstraintComponent)

#include "../entities/system.h"

void stepPhysicsSystem(Entities* entities, u32 offset, float dt) {
    ArchetypeList archetypes
}

#endif
