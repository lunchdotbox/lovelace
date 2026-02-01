#ifndef ENGINE_ENTITIES_H
#define ENGINE_ENTITIES_H

#include <cglm/io.h>
#include <elc/core.h>
#include <endian.h>
#include <stddef.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <vulkan/vulkan_core.h>

#include "../physics/particle.h"
#include "../graphics/simple_draw.h"

#define INITIAL_ARCHETYPE_MAX_ENTITIES 128
#define INITIAL_ENTITIES_MAX_ENTITIES 1024
#define INITIAL_ENTITIES_BUCKETS 128
#define ENTITIES_MAX_ARCHETYPES_PER_BUCKET 1

typedef struct TransformComponent {
    versor rotation;
    vec3 position;
} TransformComponent;

#undef _COMPONENTS_MACRO
#define _COMPONENTS_MACRO\
    X(TRANSFORM, TransformComponent)\
    X(PHYSICS, PhysicsComponent)\
    X(RENDERED, RenderedComponent)

#undef X
#define X(id, data) COMPONENT_##id,
typedef enum Component {
    _COMPONENTS_MACRO
} Component;
#undef X
#define X(id, data) case COMPONENT_##id: return sizeof(data);
ELC_INLINE size_t getComponentSize(Component component) {
    switch (component) {
        _COMPONENTS_MACRO
    }
}
#undef X
#undef _COMPONENTS_MACRO

// each bucket should have a component list where the different archetypes are seperated by section lengths at the start of each section
// each bucket should have a statically sized list of archetypes
// each bucket should have a statically sized list of archetype component list hashes
// each archetype should have a pointer to it's component count and a list of components that is seperate from but identical to the archetype's component section
// each archetype should have a pointer to it's component count and a list of pointers to entity component data, as well as an entity count
// each entity should have the same pointer to it's component count and list of components that the entity's archetype has
// each entity should have the same pointer to it's component count and list of pointers to entity component data that the entity's archetype has
// each entity should have an index into it's archetype's component data

// getting the component data of an entity would go: index into entity array -> index into array of pointers -> index into component data
// getting a component data list from component list would go: index into hash table -> iterate components -> index into list of data lists

typedef struct ArchetypeBucket ArchetypeBucket;
typedef struct EntityMapping EntityMapping;

typedef struct Entities {
    ArchetypeBucket* buckets;
    EntityMapping* entities;
    u32 n_entities, max_entities;
    u32 first_free, last_free;
    u32 n_buckets;
} Entities;

Entities createEntitySystem();
u32 createEntityUnsorted(Entities* entities, Component* components, u32 n_components, void** data);
u32 createEntity(Entities* entities, Component* components, u32 n_components, void** data);
void destroyEntity(Entities* entities, u32 id);
void addComponent(Entities* entities, u32 id, Component new, const void* data);

#endif
