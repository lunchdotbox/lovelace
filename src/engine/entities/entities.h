#ifndef ENGINE_ENTITIES_ENTITIES_H
#define ENGINE_ENTITIES_ENTITIES_H

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

typedef u32 Component;
typedef u32 Entity;

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
typedef struct ComponentInfo ComponentInfo;
typedef struct ArchetypeCacheBucket ArchetypeCacheBucket;

typedef struct Entities {
    ComponentInfo* components;
    ArchetypeBucket* buckets;
    EntityMapping* entities;
    ArchetypeCacheBucket* cache;
    u32 n_entities, max_entities;
    u32 n_components, max_components;
    u32 n_buckets;
    u32 n_cache;
} Entities;

Entities createEntitySystem();
void destroyEntitySystem(Entities* entities);
Component registerComponent(Entities* entities, size_t size);
void registerComponentReserved(Entities* entities, Component* reserved, size_t size);
Component reserveComponents(Entities* entities, u32 count);
void sortComponents(Component* components, u32 n_components, void** data);
Entity createEntityUnsorted(Entities* entities, Component* components, u32 n_components, void** data);
Entity createEntity(Entities* entities, Component* components, u32 n_components, void** data);
void destroyEntity(Entities* entities, Entity id);
void addComponent(Entities* entities, Entity id, Component new, const void* data);
void* findArchetypesUnsorted(Entities* entities, const Component* components, u32 n_components);
void* findArchetypes(Entities* entities, Component* components, u32 n_components);
void* getComponentData(void* archetype, Component component);
u32 getArchetypeEntities(void* archetype);

#endif
