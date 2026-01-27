#ifndef ENGINE_ENTITIES_H // TODO: make this reusable to generate multiple entity systems
#define ENGINE_ENTITIES_H

#include <elc/core.h>
#include <endian.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>
#include <vulkan/vulkan_core.h>

#include "../physics/particle.h"
#include "../graphics/simple_draw.h"

#ifndef INITIAL_ARCHETYPE_MAX_ENTITIES
#define INITIAL_ARCHETYPE_MAX_ENTITIES 128
#endif

#ifndef INITIAL_ENTITIES_MAX_ARCHETYPES
#define INITIAL_ENTITIES_MAX_ARCHETYPES 32
#endif

#ifndef INITIAL_ENTITIES_MAX_ENTITIES
#define INITIAL_ENTITIES_MAX_ENTITIES 1024
#endif

#ifndef INITIAL_ENTITIES_BUCKETS
#define INITIAL_ENTITIES_BUCKETS 128
#endif

#ifndef ENTITIES_ARCHETYPE_MAP_MAX_SATURATION
#define ENTITIES_ARCHETYPE_MAP_MAX_SATURATION 75
#endif

#if INITIAL_ARCHETYPE_MAX_ENTITIES <= 0
#error INITIAL_ARCHETYPE_MAX_ENTITIES must be greater than 0
#endif

#if INITIAL_ENTITIES_MAX_ARCHETYPES <= 0
#error INITIAL_ENTITIES_MAX_ARCHETYPES must be greater than 0
#endif

#if INITIAL_ENTITIES_MAX_ENTITIES <= 0
#error INITIAL_ENTITIES_MAX_ENTITIES must be greater than 0
#endif

#if INITIAL_ENTITIES_BUCKETS <= 0
#error INITIAL_ENTITIES_BUCKETS must be greater than 0
#endif

#if ENTITIES_ARCHETYPE_MAP_MAX_SATURATION <= 0
#error ENTITIES_ARCHETYPE_MAP_MAX_SATURATION must be greater than 0
#endif

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
// each archetype should have a pointer to it's component count and a list of components that is seperate from but identical to the archetype's component section
// each archetype should have a pointer to it's component count and a list of pointers to entity component data, as well as an entity count
// each entity should have the same pointer to it's component count and list of components that the entity's archetype has
// each entity should have the same pointer to it's component count and list of pointers to entity component data that the entity's archetype has
// each entity should have an index into it's archetype's component data

// getting component data of an entity would go: index into entity array -> index into array of pointers -> index into component data
// getting component data lists from component list would go: index into hash table -> index into

typedef struct ComponentArchetype {
    Component* components; // TODO: optimize to put component list directly inside bucket data
    void** entities;
    u32 n_components;
    u32 n_entities, max_entities;
} ComponentArchetype;

typedef struct ArchetypeBucket {
    u32* archetypes;
    u32 n_archetypes;
} ArchetypeBucket;

typedef struct EntityMapping {
    u32 archetype_id;
    u32 data_index;
} EntityMapping;

typedef struct Entities {
    ArchetypeBucket* buckets;
    EntityMapping* entities;
    ComponentArchetype* archetypes;
    u32 n_entities, max_entities;
    u32 n_archetypes, max_archetypes;
    u32 n_buckets, n_used_archetypes;
} Entities;

Entities createEntitySystem() {
    Entities entities = {.max_entities = INITIAL_ENTITIES_MAX_ENTITIES, .max_archetypes = INITIAL_ENTITIES_MAX_ARCHETYPES, .n_buckets = INITIAL_ENTITIES_BUCKETS};
    entities.entities = malloc(entities.max_entities * sizeof(EntityMapping));
    entities.archetypes = malloc(entities.max_archetypes * sizeof(ComponentArchetype));
    entities.buckets = calloc(entities.n_buckets, sizeof(ArchetypeBucket));
    return entities;
}

bool hasCorrectComponents(ComponentArchetype archetype, const Component* components, u32 n_components) {
    if (archetype.n_components != n_components) return false;
    for (u32 i = 0; i < n_components; i++) if (archetype.components[i] != components[i]) return false;
    return true;
}

bool hasCorrectComponentsWithNew(ComponentArchetype archetype, const Component *components, u32 n_components, Component new) {
    if (archetype.n_components != n_components + 1) return false;
    for (u32 i = 0; i < n_components; i++) if (archetype.components[i] != components[i] && archetype.components[i] != new) return false;
    return true;
}

void sortComponents(Component* components, u32 n_components) {
    if (n_components == 0) return;
    while (true) {
        bool should_break = true;
        for (u32 i = 0; i < n_components - 1; i++)
            if (components[i] > components[i + 1]) {
                SWAP(components[i], components[i + 1])
                should_break = false;
            }
        if (should_break) break;
    }
}

u64 hashComponentsWithNew(Component* components, u32 n_components, Component new) {
    u64 hash = ELC_MATH_FNV_OFFSET; // TODO: optimize by making this use a u32 directly
    bool has_hashed_new = false;
    for (u64 i = 0; i < n_components; i++) {
        if (components[i] > new && !has_hashed_new) {
            hash ^= (u64)(u32)new;
            hash *= ELC_MATH_FNV_PRIME;
            has_hashed_new = true;
        }
        hash ^= (u64)(u32)components[i];
        hash *= ELC_MATH_FNV_PRIME;
    }
    return hash;
}

u64 hashEntityComponents(Component* components, u32 n_components) {
    u64 hash = ELC_MATH_FNV_OFFSET; // TODO: optimize by making this use a u32 directly
    for (u64 i = 0; i < n_components; i++) {
        hash ^= (u64)(u32)components[i];
        hash *= ELC_MATH_FNV_PRIME;
    }
    return hash;
}

ComponentArchetype createArchetype(Component* components, u32 n_components, u32 max_entities) {
    ComponentArchetype archetype = {.n_components = n_components, .max_entities = max_entities};
    archetype.components = malloc(archetype.n_components * sizeof(Component));
    memcpy(archetype.components, components, n_components * sizeof(Component));
    archetype.entities = malloc(archetype.n_components * sizeof(void*));
    for (u32 i = 0; i < archetype.n_components; i++) archetype.entities[i] = malloc(archetype.max_entities * getComponentSize(archetype.components[i]));
    return archetype;
}

ComponentArchetype createArchetypeWithNew(Component* components, u32 n_components, u32 max_entities, Component new) {
    ComponentArchetype archetype = {.n_components = n_components + 1, .max_entities = max_entities};
    archetype.components = malloc(archetype.n_components * sizeof(Component));
    for (u32 i = 0; i < archetype.n_components; i++) {
        if (i >= n_components) {
            archetype.components[i] = new;
            break;
        }
        if (components[i] > new) {
            archetype.components[i] = new;
            memcpy(&archetype.components[i + 1], &components[i], ((n_components - 1) - i) * sizeof(Component));
            break;
        }
        archetype.components[i] = components[i];
    }
    archetype.entities = malloc(archetype.n_components * sizeof(void*));
    for (u32 i = 0; i < archetype.n_components; i++) archetype.entities[i] = malloc(archetype.max_entities * getComponentSize(archetype.components[i]));
    return archetype;
}

void allocateEntitiesArchetypes(Entities* entities, u32 max_archetypes) {
    entities->max_archetypes = max_archetypes;
    entities->archetypes = malloc(entities->max_archetypes * sizeof(ComponentArchetype));
}

void resizeArchetypeMap(Entities* entities, u32 n_buckets) {
    ArchetypeBucket* new_buckets = calloc(n_buckets, sizeof(ArchetypeBucket));
    for (u32 i = 0; i < entities->n_buckets; i++) {
        ArchetypeBucket* src = &entities->buckets[i];
        for (u32 j = 0; j < src->n_archetypes; j++) {
            ComponentArchetype* archetype = &entities->archetypes[src->archetypes[j]];
            u64 hash = hashEntityComponents(archetype->components, archetype->n_components);
            u32 index = hash % n_buckets;
            ArchetypeBucket* dst = &new_buckets[index];

            if (dst->archetypes == NULL) dst->archetypes = malloc((dst->n_archetypes + 1) * sizeof(u32)); // TODO: make this array statically sized so we dont have to reallocate so much
            else dst->archetypes = realloc(dst->archetypes, (dst->n_archetypes + 1) * sizeof(u32));
            dst->archetypes[dst->n_archetypes++] = src->archetypes[j];
        }
    }
}

u32 addArchetype(Entities* entities, ComponentArchetype archetype) {
    if (entities->n_archetypes + 1 > entities->max_archetypes) allocateEntitiesArchetypes(entities, entities->max_archetypes * ELC_MATH_GOLDEN_RATIO);
    entities->archetypes[entities->n_archetypes++] = archetype, entities->n_used_archetypes++;
    if (entities->n_used_archetypes * 100 > entities->n_buckets * ENTITIES_ARCHETYPE_MAP_MAX_SATURATION) {

    }
    return entities->n_archetypes - 1;
}

u32 findNextArchetypeWithNew(Entities* entities, Component* components, u32 n_components, Component new) {
    u64 hash = hashComponentsWithNew(components, n_components, new);
    u32 index = hash % entities->n_buckets;

    ArchetypeBucket* bucket = &entities->buckets[index];

    for (u32 i = 0; i < bucket->n_archetypes; i++)
        if (hasCorrectComponentsWithNew(entities->archetypes[bucket->archetypes[i]], components, n_components, new)) // TODO: move the components list back to directly in the hash table to make this loop faster
            return bucket->archetypes[i];

    if (bucket->archetypes == NULL) bucket->archetypes = malloc((bucket->n_archetypes + 1) * sizeof(u32));
    else bucket->archetypes = realloc(bucket->archetypes, (bucket->n_archetypes + 1) * sizeof(u32));
    bucket->archetypes[bucket->n_archetypes++] = addArchetype(entities, createArchetypeWithNew(components, n_components, INITIAL_ARCHETYPE_MAX_ENTITIES, new));

    return bucket->archetypes[bucket->n_archetypes - 1];
}

u32 findNextArchetype(Entities* entities, Component* components, u32 n_components) { // TODO: compact the archetype finding functions into fewer functions
    u64 hash = hashEntityComponents(components, n_components);
    u32 index = hash % entities->n_buckets;

    ArchetypeBucket* bucket = &entities->buckets[index];

    for (u32 i = 0; i < bucket->n_archetypes; i++)
        if (hasCorrectComponents(entities->archetypes[bucket->archetypes[i]], components, n_components))
            return bucket->archetypes[i];

    if (bucket->archetypes == NULL) bucket->archetypes = malloc((bucket->n_archetypes + 1) * sizeof(ComponentArchetype));
    else bucket->archetypes = realloc(bucket->archetypes, (bucket->n_archetypes + 1));
    bucket->archetypes[bucket->n_archetypes++] = addArchetype(entities, createArchetype(components, n_components, INITIAL_ARCHETYPE_MAX_ENTITIES));

    return bucket->archetypes[bucket->n_archetypes - 1];
}

void allocateArchetypeEntities(ComponentArchetype* archetype, u32 max_entities) {
    archetype->max_entities = max_entities;
    for (u32 i = 0; i < archetype->n_components; i++) archetype->entities[i] = realloc(archetype->entities[i], archetype->max_entities * getComponentSize(archetype->components[i]));
}

void allocateEntityMappings(Entities* entities, u32 max_entities) {
    entities->max_entities = max_entities;
    entities->entities = realloc(entities->entities, entities->max_entities * sizeof(EntityMapping));
}

u32 addEntityMapping(Entities* entities, EntityMapping mapping) {
    if (entities->n_entities + 1 > entities->max_entities) allocateEntityMappings(entities, entities->max_entities * ELC_MATH_GOLDEN_RATIO);
    entities->entities[entities->n_entities++] = mapping;
    return entities->n_entities - 1;
}

u32 createEntity(Entities* entities, Component* components, u32 n_components, void** data) {
    u32 archetype_id = findNextArchetype(entities, components, n_components);
    ComponentArchetype* archetype = &entities->archetypes[archetype_id];

    if (archetype->n_entities + 1 > archetype->max_entities) allocateArchetypeEntities(archetype, archetype->max_entities * ELC_MATH_GOLDEN_RATIO);

    for (u32 i = 0; i < n_components; i++) {
        size_t component_size = getComponentSize(archetype->components[i]);
        memcpy(archetype->entities[i] + (component_size * archetype->n_entities), data[i], component_size);
    }

    EntityMapping mapping = {
        .archetype_id = archetype_id,
        .data_index = archetype->n_entities++,
    };

    return addEntityMapping(entities, mapping);
}

#endif
