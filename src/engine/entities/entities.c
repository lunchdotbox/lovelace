#include "entities.h"
#include <elc/core.h>

typedef struct EntityListsStart {
    u32 n_components;
    u32 n_entities;
    u32 max_entities;
    u32* entity_ids;
} EntityListsStart;

typedef struct ArchetypeBucket {
    u64 component_hashes[ENTITIES_MAX_ARCHETYPES_PER_BUCKET];
    void* archetype_data[ENTITIES_MAX_ARCHETYPES_PER_BUCKET];
#if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
    u8 n_archetypes;
#endif
} ArchetypeBucket;

typedef struct EntityMapping {
    void* archetype_data;
    u32 data_index;
} EntityMapping;

ELC_INLINE void zeroArchetypeBuckets(ArchetypeBucket* buckets, u32 n_buckets) {
    #if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
        for (u32 i = 0; i < n_buckets; i++) buckets[i].n_archetypes = 0;
    #else
        for (u32 i = 0; i < n_buckets; i++) buckets[i].archetype_data[0] = NULL;
    #endif
}

Entities createEntitySystem() {
    Entities entities = {.max_entities = INITIAL_ENTITIES_MAX_ENTITIES, .n_buckets = INITIAL_ENTITIES_BUCKETS};
    entities.entities = malloc(entities.max_entities * sizeof(EntityMapping));
    entities.buckets = malloc(entities.n_buckets * sizeof(ArchetypeBucket));
    zeroArchetypeBuckets(entities.buckets, entities.n_buckets);
    return entities;
}

ELC_INLINE bool hasCorrectComponents(const Component* components_a, u32 n_components_a, const Component* components_b, u32 n_components_b) {
    if (n_components_a != n_components_b) return false;
    for (u32 i = 0; i < n_components_a; i++) if (components_a[i] != components_b[i]) return false;
    return true;
}

ELC_INLINE bool hasCorrectComponentsWithNew(const Component *components_a, u32 n_components_a, const Component *components_b, u32 n_components_b, Component new) {
    if (n_components_a != n_components_b + 1) return false;
    for (u32 i = 0; i < n_components_b; i++) if (components_a[i] != components_b[i] && components_a[i] != new) return false;
    return true;
}

ELC_INLINE void sortComponents(Component* components, u32 n_components, void** data) {
    if (n_components == 0) return;
    while (true) {
        bool should_break = true;
        for (u32 i = 0; i < n_components - 1; i++)
            if (components[i] > components[i + 1]) {
                SWAP(components[i], components[i + 1])
                if (data != NULL) SWAP(data[i], data[i + 1]);
                should_break = false;
            }
        if (should_break) break;
    }
}

ELC_INLINE u64 hashComponentsWithNew(Component* components, u32 n_components, Component new) {
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
    if (!has_hashed_new) {
        hash ^= (u64)(u32)new;
        hash *= ELC_MATH_FNV_PRIME;
    }
    return hash;
}

ELC_INLINE u64 hashEntityComponents(Component* components, u32 n_components) {
    u64 hash = ELC_MATH_FNV_OFFSET; // TODO: optimize by making this use a u32 directly
    for (u64 i = 0; i < n_components; i++) {
        hash ^= (u64)(u32)components[i];
        hash *= ELC_MATH_FNV_PRIME;
    }
    return hash;
}

ELC_INLINE void createNewComponentList(Component* components, u32 n_components, Component new, Component* memory) {
    for (u32 i = 0; i <= n_components; i++) {
        if (i >= n_components) {
            memory[i] = new;
            break;
        }
        if (components[i] > new) {
            memory[i] = new;
            memcpy(&memory[i + 1], &components[i], ((n_components - 1) - i) * sizeof(Component));
            break;
        }
        memory[i] = components[i];
    }
}

ELC_INLINE void resizeArchetypeMap(Entities* entities, u32 n_buckets) {
    begin: ArchetypeBucket* new_buckets = malloc(n_buckets * sizeof(ArchetypeBucket));
    zeroArchetypeBuckets(new_buckets, n_buckets);

    for (u32 i = 0; i < entities->n_buckets; i++) {
        ArchetypeBucket* src = &entities->buckets[i];
#if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
        for (u32 j = 0; j < src->n_archetypes; j++) {
            u64 hash = src->component_hashes[j];
            u32 index = hash % n_buckets;

            ArchetypeBucket* dst = &new_buckets[index];

            if (dst->n_archetypes >= ENTITIES_MAX_ARCHETYPES_PER_BUCKET) {
                free(new_buckets);
                n_buckets *= ELC_MATH_GOLDEN_RATIO;
                goto begin;
            }

            dst->component_hashes[dst->n_archetypes] = src->component_hashes[j];
            dst->entity_lists[dst->n_archetypes] = src->entity_lists[j];
        }
#else
        if (src->archetype_data[0] != NULL) {
            u64 hash = src->component_hashes[0];
            u32 index = hash % n_buckets;

            ArchetypeBucket* dst = &new_buckets[index];

            if (dst->archetype_data[0] != NULL) {
                free(new_buckets);
                n_buckets *= ELC_MATH_GOLDEN_RATIO;
                goto begin;
            }

            dst->component_hashes[0] = src->component_hashes[0];
            dst->archetype_data[0] = src->archetype_data[0];
        }
#endif
    }

    entities->buckets = new_buckets;
    entities->n_buckets = n_buckets;
}

ELC_INLINE void* addArchetype(ArchetypeBucket* bucket, Component* components, u32 n_components, u64 hash, u32 max_entities) {
#if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
    u32 index = bucket->n_archetypes++;
#else
    u32 index = 0;
#endif
    bucket->component_hashes[index] = hash;
    bucket->archetype_data[index] = malloc(sizeof(EntityListsStart) + (n_components * sizeof(Component)) + (n_components * sizeof(void*)));
    void** entity_list = bucket->archetype_data[index] + sizeof(EntityListsStart) + (n_components * sizeof(Component));
    for (u32 i = 0; i < n_components; i++) entity_list[i] = malloc(max_entities * getComponentSize(components[i]));
    *(EntityListsStart*)bucket->archetype_data[index] = (EntityListsStart){.n_components = n_components, .max_entities = max_entities, .entity_ids = malloc(max_entities * sizeof(u32))};

    return bucket->archetype_data[index];
}

ELC_INLINE void* findNextArchetypeWithNew(Entities* entities, Component* components, u32 n_components, Component new) {
    u64 hash = hashComponentsWithNew(components, n_components, new);
    begin: u32 index = hash % entities->n_buckets;

    ArchetypeBucket* bucket = &entities->buckets[index];

#if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
    for (u32 i = 0; i < bucket->n_archetypes; i++) {
#else
    if (bucket->archetype_data[0] != NULL) {
        u32 i = 0;
#endif
        EntityListsStart* start = bucket->archetype_data[i];
        if (bucket->component_hashes[i] == hash && hasCorrectComponentsWithNew(bucket->archetype_data[i] + sizeof(EntityListsStart) + (start->n_components * sizeof(Component)), start->n_components, components, n_components, new))
            return bucket->archetype_data[i];
    }

#if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
    if (bucket->n_archetypes >= ENTITIES_MAX_ARCHETYPES_PER_BUCKET) {
#else
    if (bucket->archetype_data[0] != NULL) {
#endif
        resizeArchetypeMap(entities, entities->n_buckets * ELC_MATH_GOLDEN_RATIO);
        goto begin;
    }

    void* archetype = addArchetype(bucket, components, n_components, hash, INITIAL_ARCHETYPE_MAX_ENTITIES);
    createNewComponentList(components, n_components, new, archetype + sizeof(EntityListsStart));

    return archetype;
}

ELC_INLINE void* findNextArchetype(Entities* entities, Component* components, u32 n_components) {
    u64 hash = hashEntityComponents(components, n_components);
    begin: u32 index = hash % entities->n_buckets;

    ArchetypeBucket* bucket = &entities->buckets[index];

#if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
    for (u32 i = 0; i < bucket->n_archetypes; i++) {
#else
    if (bucket->archetype_data[0] != NULL) {
        u32 i = 0;
#endif
        EntityListsStart* start = bucket->archetype_data[i];
        if (bucket->component_hashes[i] == hash && hasCorrectComponents(bucket->archetype_data[i] + sizeof(EntityListsStart) + (start->n_components * sizeof(Component)), start->n_components, components, n_components))
            return bucket->archetype_data[i];
    }

#if ENTITIES_MAX_ARCHETYPES_PER_BUCKET > 1
    if (bucket->n_archetypes >= ENTITIES_MAX_ARCHETYPES_PER_BUCKET) {
#else
    if (bucket->archetype_data[0] != NULL) {
#endif
        resizeArchetypeMap(entities, entities->n_buckets * ELC_MATH_GOLDEN_RATIO);
        goto begin;
    }

    void* archetype = addArchetype(bucket, components, n_components, hash, INITIAL_ARCHETYPE_MAX_ENTITIES);
    memcpy(archetype + sizeof(EntityListsStart), components, n_components * sizeof(Component));

    return archetype;
}

ELC_INLINE void allocateArchetypeEntities(void* archetype, u32 max_entities) {
    EntityListsStart* start = archetype;
    void** entity_list = archetype + sizeof(EntityListsStart) + (start->n_components * sizeof(Component));
    Component* component_list = archetype + sizeof(EntityListsStart);
    start->max_entities = max_entities;
    for (u32 i = 0; i < start->n_components; i++) entity_list[i] = realloc(entity_list[i], max_entities * getComponentSize(component_list[i]));
    start->entity_ids = realloc(start->entity_ids, start->max_entities * sizeof(u32));
}

ELC_INLINE void allocateEntityMappings(Entities* entities, u32 max_entities) {
    entities->max_entities = max_entities;
    entities->entities = realloc(entities->entities, entities->max_entities * sizeof(EntityMapping));
}

ELC_INLINE u32 addEntityMapping(Entities* entities, EntityMapping mapping) {
    if (entities->n_entities + 1 > entities->max_entities) allocateEntityMappings(entities, entities->max_entities * ELC_MATH_GOLDEN_RATIO);
    entities->entities[entities->n_entities++] = mapping;
    return entities->n_entities - 1;
}

u32 createEntityUnsorted(Entities* entities, Component* components, u32 n_components, void** data) {
    void* archetype = findNextArchetype(entities, components, n_components);

    EntityListsStart* start = archetype;
    void** entity_list = archetype + sizeof(EntityListsStart) + (start->n_components * sizeof(Component));
    Component* component_list = archetype + sizeof(EntityListsStart);

    if (start->n_entities + 1 > start->max_entities) allocateArchetypeEntities(archetype, start->max_entities * ELC_MATH_GOLDEN_RATIO);

    if (data != NULL) for (u32 i = 0; i < n_components; i++) if (data[i] != NULL) {
        size_t component_size = getComponentSize(component_list[i]);
        memcpy(entity_list[i] + (start->n_entities * component_size), data[i], component_size);
    }

    start->entity_ids[start->n_entities] = entities->n_entities;

    EntityMapping mapping = {
        .data_index = start->n_entities++,
        .archetype_data = archetype,
    };

    return addEntityMapping(entities, mapping);
}

u32 createEntity(Entities* entities, Component* components, u32 n_components, void** data) {
    sortComponents(components, n_components, data);
    return createEntityUnsorted(entities, components, n_components, data);
}

void destroyEntity(Entities* entities, u32 id) {
    EntityMapping* mapping = &entities->entities[id];
    EntityListsStart* start = mapping->archetype_data;
    void** entity_list = mapping->archetype_data + sizeof(EntityListsStart) + (start->n_components * sizeof(Component));
    Component* component_list = mapping->archetype_data + sizeof(EntityListsStart);

    if (mapping->data_index != start->n_entities - 1) {
        u32 swap_id = start->entity_ids[start->n_entities - 1];
        EntityMapping* swap = &entities->entities[swap_id];

        for (u32 i = 0; i < start->n_components; i++) {
            size_t component_size = getComponentSize(component_list[i]);
            memcpy(entity_list[i] + (mapping->data_index * component_size), entity_list[i] + (swap->data_index * component_size), component_size);
        }

        swap->data_index = mapping->data_index;
    }

    if (start->n_entities - 1 < start->max_entities / ELC_MATH_GOLDEN_RATIO) allocateArchetypeEntities(mapping->archetype_data, MAX(start->max_entities / ELC_MATH_GOLDEN_RATIO, INITIAL_ARCHETYPE_MAX_ENTITIES));
    start->n_entities--;

    if (id == entities->n_entities - 1) {
        entities->n_entities--;
        if (entities->n_entities > 0) for (u32 i = entities->n_entities - 1; i >= 0 && entities->entities[i].archetype_data == NULL; i--) entities->n_entities--;
    } else entities->entities[id].archetype_data = NULL;

    if (entities->n_entities < entities->max_entities / ELC_MATH_GOLDEN_RATIO) allocateEntityMappings(entities, MAX(entities->max_entities / ELC_MATH_GOLDEN_RATIO, INITIAL_ENTITIES_MAX_ENTITIES));
}

void addComponent(Entities* entities, u32 id, Component new, const void* data) {
    EntityMapping* mapping = &entities->entities[id];
    Component* components = mapping->archetype_data + sizeof(EntityListsStart);
    EntityListsStart* start = mapping->archetype_data;
    void** entity_list = mapping->archetype_data + sizeof(EntityListsStart) + (start->n_components * sizeof(Component));

    void* archetype = findNextArchetypeWithNew(entities, components, start->n_components, new);

    EntityListsStart* new_start = archetype;
    void** new_entity_list = archetype + sizeof(EntityListsStart) + (new_start->n_components * sizeof(Component));
    Component* new_components = archetype + sizeof(EntityListsStart);

    if (new_start->n_entities + 1 > new_start->max_entities) allocateArchetypeEntities(archetype, new_start->max_entities * ELC_MATH_GOLDEN_RATIO);

    for (u32 i = 0, index = 0; i < start->n_components; i++, index++) { // possible bug here
        if (new_components[i] == new) {
            if (data != NULL) {
                size_t component_size = getComponentSize(new);
                memcpy(new_entity_list[index] + (new_start->n_entities * component_size), data, component_size);
            }
            i--;
            continue;
        }

        size_t component_size = getComponentSize(components[i]);
        memcpy(new_entity_list[index] + (new_start->n_entities * component_size), entity_list[i] + (mapping->data_index * component_size), component_size);
    }

    if (mapping->data_index != start->n_entities - 1) {
        u32 swap_id = start->entity_ids[start->n_entities - 1];
        EntityMapping* swap = &entities->entities[swap_id];

        for (u32 i = 0; i < start->n_components; i++) {
            size_t component_size = getComponentSize(components[i]);
            memcpy(entity_list[i] + (mapping->data_index * component_size), entity_list[i] + (swap->data_index * component_size), component_size);
        }

        swap->data_index = mapping->data_index;
    }

    if (start->n_entities - 1 < start->max_entities / ELC_MATH_GOLDEN_RATIO) allocateArchetypeEntities(mapping->archetype_data, MAX(start->max_entities / ELC_MATH_GOLDEN_RATIO, INITIAL_ARCHETYPE_MAX_ENTITIES));
    start->n_entities--;

    mapping->data_index = new_start->n_entities++;
    mapping->archetype_data = archetype;
}
