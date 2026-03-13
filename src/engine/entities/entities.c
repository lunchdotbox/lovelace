#include "entities.h"

#include <cglm/io.h>
#include <elc/core.h>
#include <stddef.h>
#include <stdint.h>
#include <stdio.h>
#include <stdlib.h>

#define INITIAL_ARCHETYPE_MAX_ENTITIES 128
#define INITIAL_ENTITIES_MAX_ENTITIES 1024
#define INITIAL_ENTITIES_BUCKETS 128
#define ENTITIES_MAX_ARCHETYPES_PER_BUCKET 4
#define INITIAL_BUCKET_LIST_MAX 16
#define INITIAL_ENTITIES_MAX_COMPONENTS 16
#define ARCHETYPE_ITERATOR_INITIAL_MAX_STACK 16
#define INITIAL_COMPONENT_MAX_ARCHETYPES 8
#define INITIAL_LOOKUP_CACHE_MAX_ARCHETYPES 8
#define ENTITIES_MAX_LISTS_PER_CACHE 4
#define INITIAL_ENTITIES_CACHE_BUCKETS 64

typedef struct ArchetypeStart {
    u32 n_components;
    u32 n_entities;
    u32 max_entities;
    u32* entity_ids;
} ArchetypeStart;

typedef struct ArchetypeBucket {
    u64 component_hashes[ENTITIES_MAX_ARCHETYPES_PER_BUCKET];
    void* archetype_data[ENTITIES_MAX_ARCHETYPES_PER_BUCKET];
    u64* component_hash_list;
    void** archetype_data_list;
    u32 n_archetypes, max_archetypes;
} ArchetypeBucket;

typedef struct EntityMapping {
    void* archetype_data;
    u32 data_index;
} EntityMapping;

typedef struct Archetype {
    void* data;
    u64 hash;
} Archetype;

typedef struct ComponentInfo {
    size_t size;
    void** cache_entries;
    void** archetypes;
    u32 n_cache_entries, max_cache_entries;
    u32 n_archetypes, max_archetypes;
} ComponentInfo;

typedef struct EntryAndHash {
    void* entry;
    u64 hash;
} EntryAndHash;

typedef struct ArchetypeCacheStart {
    u32 n_archetypes, max_archetypes;
    u32 n_components;
} ArchetypeCacheStart;

typedef struct ArchetypeCacheBucket {
    void* entries[ENTITIES_MAX_LISTS_PER_CACHE];
    u64 component_hashes[ENTITIES_MAX_LISTS_PER_CACHE];
    void** entry_list;
    u64* component_hash_list;
    u32 n_entries, max_entries;
} ArchetypeCacheBucket;

Entities createEntitySystem() {
    Entities entities = {.max_entities = INITIAL_ENTITIES_MAX_ENTITIES, .n_buckets = INITIAL_ENTITIES_BUCKETS, .max_components = INITIAL_ENTITIES_MAX_COMPONENTS, .n_cache = INITIAL_ENTITIES_CACHE_BUCKETS};
    entities.entities = malloc(entities.max_entities * sizeof(EntityMapping));
    entities.buckets = calloc(entities.n_buckets, sizeof(ArchetypeBucket));
    entities.components = malloc(entities.max_components * sizeof(ComponentInfo));
    entities.cache = calloc(entities.n_cache, sizeof(ArchetypeCacheBucket));
    return entities;
}

ELC_INLINE Archetype getBucketArchetype(ArchetypeBucket* bucket, u32 index) {
    if (index < ENTITIES_MAX_ARCHETYPES_PER_BUCKET) return (Archetype){.data = bucket->archetype_data[index], .hash = bucket->component_hashes[index]};
    else {
        u32 list_index = index - ENTITIES_MAX_ARCHETYPES_PER_BUCKET;
        return (Archetype){.data = bucket->archetype_data_list[list_index], .hash = bucket->component_hash_list[list_index]};
    }
}

ELC_INLINE EntryAndHash getCacheEntry(const ArchetypeCacheBucket* bucket, u32 index) {
    if (index < ENTITIES_MAX_LISTS_PER_CACHE) return (EntryAndHash){.entry = bucket->entries[index], .hash = bucket->component_hashes[index]};
    else {
        u32 list_index = index - ENTITIES_MAX_LISTS_PER_CACHE;
        return (EntryAndHash){.entry = bucket->entry_list[list_index], .hash = bucket->component_hash_list[list_index]};
    }
}

void destroyEntitySystem(Entities* entities) {
    for (u32 i = 0; i < entities->n_buckets; i++) {
        ArchetypeBucket* bucket = &entities->buckets[i];
        for (u32 j = 0; j < bucket->n_archetypes; j++) {
            Archetype archetype = getBucketArchetype(bucket, j);
            ArchetypeStart* start = archetype.data;
            void** entity_list = archetype.data + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component));
            for (u32 k = 0; k < start->n_components; k++) free(entity_list[k]);
            free(start->entity_ids), free(archetype.data);
        }
        if (bucket->archetype_data_list != NULL) free(bucket->archetype_data_list), free(bucket->component_hash_list);
    }

    for (u32 i = 0; i < entities->n_cache; i++) {
        ArchetypeCacheBucket* bucket = &entities->cache[i];
        for (u32 j = 0; j < bucket->n_entries; i++) {

        }
    }

    for (u32 i = 0; i < entities->n_components; i++) free(entities->components[i].archetypes);

    free(entities->buckets), free(entities->entities), free(entities->components), free(entities->cache);
}

ELC_INLINE void resizeEntitiesComponents(Entities* entities, u32 max_components) {
    entities->components = realloc(entities->components, max_components * sizeof(ComponentInfo));
    entities->max_components = max_components;
}

ELC_INLINE ComponentInfo createComponentInfo(size_t size) {
    ComponentInfo info = {.size = size, .max_archetypes = INITIAL_COMPONENT_MAX_ARCHETYPES};
    info.archetypes = malloc(info.n_archetypes * sizeof(void*));
    return info;
}

ELC_INLINE void resizeComponentArchetypes(ComponentInfo* info, u32 max_archetypes) {
    info->archetypes = realloc(info->archetypes, max_archetypes * sizeof(void*));
    info->max_archetypes = max_archetypes;
}

ELC_INLINE u32 addComponentArchetype(ComponentInfo* info, void* archetype) {
    if (info->n_archetypes + 1 > info->max_archetypes) resizeComponentArchetypes(info, info->n_archetypes * ELC_MATH_GOLDEN_RATIO);
    info->archetypes[info->n_archetypes] = archetype;
    return info->n_archetypes++;
}

Component registerComponent(Entities* entities, size_t size) {
    if (entities->n_components + 1 > entities->max_components) resizeEntitiesComponents(entities, entities->max_components * ELC_MATH_GOLDEN_RATIO);
    entities->components[entities->n_components] = createComponentInfo(size);
    return entities->n_components++;
}

void registerComponentReserved(Entities* entities, Component* reserved, size_t size) {
    entities->components[(*reserved)++] = createComponentInfo(size);
}

Component reserveComponents(Entities* entities, u32 count) {
    while (entities->n_components + count > entities->max_components) resizeEntitiesComponents(entities, entities->max_components * ELC_MATH_GOLDEN_RATIO);
    entities->n_components += count;
    return entities->n_components - count;
}

ELC_INLINE ComponentInfo getComponentInfo(Entities* entities, Component component) {
    return entities->components[component];
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

ELC_INLINE void sortComponentsInternal(Component* components, u32 n_components, void** data) {
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

void sortComponents(Component* components, u32 n_components, void** data) {
    sortComponentsInternal(components, n_components, data);
}

ELC_INLINE u64 hashComponentsWithNew(const Component* components, u32 n_components, Component new) {
    u64 hash = ELC_MATH_FNV_OFFSET;
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

ELC_INLINE u64 hashEntityComponents(const Component* components, u32 n_components) {
    u64 hash = ELC_MATH_FNV_OFFSET;
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

ELC_INLINE void addArchetype(ArchetypeBucket* bucket, void* data, u64 hash) {
    if (bucket->n_archetypes < ENTITIES_MAX_ARCHETYPES_PER_BUCKET) {
        bucket->archetype_data[bucket->n_archetypes] = data;
        bucket->component_hashes[bucket->n_archetypes++] = hash;
    } else {
        if (bucket->archetype_data_list == NULL) {
            bucket->max_archetypes = INITIAL_BUCKET_LIST_MAX;
            bucket->archetype_data_list = malloc(bucket->max_archetypes * sizeof(void*));
            bucket->component_hash_list = malloc(bucket->max_archetypes * sizeof(u64));
        } else {
            if ((bucket->n_archetypes - INITIAL_BUCKET_LIST_MAX) + 1 > bucket->max_archetypes) {
                bucket->max_archetypes *= ELC_MATH_GOLDEN_RATIO;
                bucket->archetype_data_list = realloc(bucket->archetype_data_list, bucket->max_archetypes * sizeof(void*));
                bucket->component_hash_list = realloc(bucket->component_hash_list, bucket->max_archetypes * sizeof(u64));
            }
        }
        bucket->archetype_data_list[bucket->n_archetypes - INITIAL_BUCKET_LIST_MAX] = data;
        bucket->component_hash_list[bucket->n_archetypes++ - INITIAL_BUCKET_LIST_MAX] = hash;
    }
}

ELC_INLINE void freeArchetypeMap(ArchetypeBucket* buckets, u32 n_buckets) {
    for (u32 i = 0; i < n_buckets; i++) if (buckets->archetype_data_list != NULL) free(buckets->archetype_data_list), free(buckets->component_hash_list);

    free(buckets);
}

ELC_INLINE void resizeArchetypeMap(Entities* entities, u32 n_buckets) {
    ArchetypeBucket* new_buckets = calloc(n_buckets, sizeof(ArchetypeBucket));

    for (u32 i = 0; i < entities->n_buckets; i++) {
        ArchetypeBucket* src = &entities->buckets[i];
        for (u32 j = 0; j < src->n_archetypes; j++) {
            Archetype archetype = getBucketArchetype(src, i);
            u32 index = archetype.hash % n_buckets;

            addArchetype(&new_buckets[index], archetype.data, archetype.hash);
        }
    }

    freeArchetypeMap(entities->buckets, entities->n_buckets);

    entities->buckets = new_buckets;
    entities->n_buckets = n_buckets;
}

ELC_INLINE void* createArchetype(Entities* entities, Component* components, u32 n_components, u32 max_entities) {
    void* result = malloc(sizeof(ArchetypeStart) + (n_components * sizeof(Component)) + (n_components * sizeof(void*)));
    void** entity_list = result + sizeof(ArchetypeStart) + (n_components * sizeof(Component));
    *(ArchetypeStart*)result = (ArchetypeStart){.n_components = n_components, .max_entities = max_entities, .entity_ids = malloc(max_entities * sizeof(u32))};
    for (u32 i = 0; i < n_components; i++) entity_list[i] = malloc(max_entities * getComponentInfo(entities, components[i]).size);
    memcpy(result + sizeof(ArchetypeStart), components, n_components * sizeof(Component));

    for (u32 i = 0; i < n_components; i++) addComponentArchetype(&entities->components[components[i]], result);

    return result;
}

ELC_INLINE void* createArchetypeWithNew(Entities* entities, Component* components, u32 n_components, u32 max_entities, Component new) {
    void* result = malloc(sizeof(ArchetypeStart) + ((n_components + 1) * sizeof(Component)) + ((n_components + 1) * sizeof(void*)));
    void** entity_list = result + sizeof(ArchetypeStart) + ((n_components + 1) * sizeof(Component));
    *(ArchetypeStart*)result = (ArchetypeStart){.n_components = n_components + 1, .max_entities = max_entities, .entity_ids = malloc(max_entities * sizeof(u32))};
    for (u32 i = 0; i <= n_components; i++) {
        if (i >= n_components) entity_list[i] = malloc(max_entities * getComponentInfo(entities, new).size);
        else if (components[i] > new) {
            entity_list[i] = malloc(max_entities * getComponentInfo(entities, new).size);
            for (u32 j = 0; j < (n_components - 1) - i; j++) entity_list[i + j + 1] = malloc(max_entities * getComponentInfo(entities, components[i + j]).size);
            break;
        } else entity_list[i] = malloc(max_entities * getComponentInfo(entities, components[i]).size);
    }
    createNewComponentList(components, n_components, new, result + sizeof(ArchetypeStart));

    for (u32 i = 0; i < n_components; i++) addComponentArchetype(&entities->components[components[i]], result);

    return result;
}

ELC_INLINE void* findNextArchetypeWithNew(Entities* entities, Component* components, u32 n_components, Component new) {
    u64 hash = hashComponentsWithNew(components, n_components, new);
    u32 index = hash % entities->n_buckets;

    ArchetypeBucket* bucket = &entities->buckets[index];

    for (u32 i = 0; i < bucket->n_archetypes; i++) {
        Archetype archetype = getBucketArchetype(bucket, i);
        ArchetypeStart* start = archetype.data;
        if (archetype.hash == hash && hasCorrectComponentsWithNew(archetype.data + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component)), start->n_components, components, n_components, new))
            return archetype.data;
    }

    void* archetype = createArchetypeWithNew(entities, components, n_components, INITIAL_ARCHETYPE_MAX_ENTITIES, new);
    addArchetype(bucket, archetype, hash);

    return archetype;
}

ELC_INLINE void* findNextArchetype(Entities* entities, Component* components, u32 n_components) {
    u64 hash = hashEntityComponents(components, n_components);
    u32 index = hash % entities->n_buckets;

    ArchetypeBucket* bucket = &entities->buckets[index];

    for (u32 i = 0; i < bucket->n_archetypes; i++) {
        Archetype archetype = getBucketArchetype(bucket, i);
        ArchetypeStart* start = archetype.data;
        if (archetype.hash == hash && hasCorrectComponents(archetype.data + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component)), start->n_components, components, n_components))
            return archetype.data;
    }

    void* archetype = createArchetype(entities, components, n_components, INITIAL_ARCHETYPE_MAX_ENTITIES);
    addArchetype(bucket, archetype, hash);

    return archetype;
}

ELC_INLINE void allocateArchetypeEntities(Entities* entities, void* archetype, u32 max_entities) {
    ArchetypeStart* start = archetype;
    void** entity_list = archetype + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component));
    Component* component_list = archetype + sizeof(ArchetypeStart);
    start->max_entities = max_entities;
    for (u32 i = 0; i < start->n_components; i++) entity_list[i] = realloc(entity_list[i], max_entities * getComponentInfo(entities, component_list[i]).size);
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

ELC_INLINE Entity createEntityInternal(Entities* entities, Component* components, u32 n_components, void** data) {
    void* archetype = findNextArchetype(entities, components, n_components);

    ArchetypeStart* start = archetype;
    void** entity_list = archetype + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component));
    Component* component_list = archetype + sizeof(ArchetypeStart);

    if (start->n_entities + 1 > start->max_entities) allocateArchetypeEntities(entities, archetype, start->max_entities * ELC_MATH_GOLDEN_RATIO);

    if (data != NULL) for (u32 i = 0; i < n_components; i++) if (data[i] != NULL) {
        size_t component_size = getComponentInfo(entities, component_list[i]).size;
        memcpy(entity_list[i] + (start->n_entities * component_size), data[i], component_size);
    }

    start->entity_ids[start->n_entities] = entities->n_entities;

    EntityMapping mapping = {
        .data_index = start->n_entities++,
        .archetype_data = archetype,
    };

    return addEntityMapping(entities, mapping);
}

Entity createEntityUnsorted(Entities* entities, Component *components, u32 n_components, void **data) {
    return createEntityInternal(entities, components, n_components, data);
}

Entity createEntity(Entities* entities, Component* components, u32 n_components, void** data) {
    sortComponentsInternal(components, n_components, data);
    return createEntityInternal(entities, components, n_components, data);
}

void destroyEntity(Entities* entities, Entity id) {
    EntityMapping* mapping = &entities->entities[id];
    ArchetypeStart* start = mapping->archetype_data;
    void** entity_list = mapping->archetype_data + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component));
    Component* component_list = mapping->archetype_data + sizeof(ArchetypeStart);

    if (mapping->data_index != start->n_entities - 1) {
        u32 swap_id = start->entity_ids[start->n_entities - 1];
        EntityMapping* swap = &entities->entities[swap_id];

        for (u32 i = 0; i < start->n_components; i++) {
            size_t component_size = getComponentInfo(entities, component_list[i]).size;
            memcpy(entity_list[i] + (mapping->data_index * component_size), entity_list[i] + (swap->data_index * component_size), component_size);
        }

        swap->data_index = mapping->data_index;
    }

    if (start->n_entities - 1 < start->max_entities / ELC_MATH_GOLDEN_RATIO) allocateArchetypeEntities(entities, mapping->archetype_data, MAX(start->max_entities / ELC_MATH_GOLDEN_RATIO, INITIAL_ARCHETYPE_MAX_ENTITIES));
    start->n_entities--;

    if (id == entities->n_entities - 1) {
        entities->n_entities--;
        if (entities->n_entities > 0) for (u32 i = entities->n_entities - 1; i >= 0 && entities->entities[i].archetype_data == NULL; i--) entities->n_entities--;
    } else entities->entities[id].archetype_data = NULL;

    if (entities->n_entities < entities->max_entities / ELC_MATH_GOLDEN_RATIO) allocateEntityMappings(entities, MAX(entities->max_entities / ELC_MATH_GOLDEN_RATIO, INITIAL_ENTITIES_MAX_ENTITIES));
}

void addComponent(Entities* entities, Entity id, Component new, const void* data) {
    EntityMapping* mapping = &entities->entities[id];
    Component* components = mapping->archetype_data + sizeof(ArchetypeStart);
    ArchetypeStart* start = mapping->archetype_data;
    void** entity_list = mapping->archetype_data + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component));

    void* archetype = findNextArchetypeWithNew(entities, components, start->n_components, new);

    ArchetypeStart* new_start = archetype;
    void** new_entity_list = archetype + sizeof(ArchetypeStart) + (new_start->n_components * sizeof(Component));
    Component* new_components = archetype + sizeof(ArchetypeStart);

    if (new_start->n_entities + 1 > new_start->max_entities) allocateArchetypeEntities(entities, archetype, new_start->max_entities * ELC_MATH_GOLDEN_RATIO);

    for (u32 i = 0, index = 0; i < start->n_components; i++, index++) { // possible bug here
        if (new_components[i] == new) {
            if (data != NULL) {
                size_t component_size = getComponentInfo(entities, new).size;
                memcpy(new_entity_list[index] + (new_start->n_entities * component_size), data, component_size);
            }
            i--;
            continue;
        }

        size_t component_size = getComponentInfo(entities, components[i]).size;
        memcpy(new_entity_list[index] + (new_start->n_entities * component_size), entity_list[i] + (mapping->data_index * component_size), component_size);
    }

    if (mapping->data_index != start->n_entities - 1) {
        u32 swap_id = start->entity_ids[start->n_entities - 1];
        EntityMapping* swap = &entities->entities[swap_id];

        for (u32 i = 0; i < start->n_components; i++) {
            size_t component_size = getComponentInfo(entities, components[i]).size;
            memcpy(entity_list[i] + (mapping->data_index * component_size), entity_list[i] + (swap->data_index * component_size), component_size);
        }

        swap->data_index = mapping->data_index;
    }

    if (start->n_entities - 1 < start->max_entities / ELC_MATH_GOLDEN_RATIO) allocateArchetypeEntities(entities, mapping->archetype_data, MAX(start->max_entities / ELC_MATH_GOLDEN_RATIO, INITIAL_ARCHETYPE_MAX_ENTITIES));
    start->n_entities--;

    mapping->data_index = new_start->n_entities++;
    mapping->archetype_data = archetype;
}

ELC_INLINE void* findArchetypesUncachedInternal(Entities* entities, const Component* components, u32 n_components) {
    ArchetypeCacheStart initial_start = {.max_archetypes = INITIAL_COMPONENT_MAX_ARCHETYPES, .n_components = n_components};
    void* list = malloc(sizeof(ArchetypeCacheStart) + (n_components * sizeof(Component)) + (initial_start.max_archetypes * sizeof(void*)));
    memcpy(list + sizeof(ArchetypeCacheStart), components, n_components * sizeof(Component));
    ArchetypeCacheStart* start = list;
    void** archetypes = list + sizeof(ArchetypeCacheStart) + (n_components * sizeof(Component));
    *start = initial_start;

    Component start_component = components[0];
    ComponentInfo start_info = getComponentInfo(entities, components[0]);
    for (u32 i = 1; i < n_components; i++) { // possible bug
        ComponentInfo info = getComponentInfo(entities, components[i]);
        if (info.n_archetypes < start_info.n_archetypes) {
            start_component = components[i];
            start_info = info;
        }
    }

    for (u32 i = 0; i < start_info.n_archetypes; i++) {
        bool should_add = true;

        void* archetype = start_info.archetypes[i];
        Component* archetype_components = archetype + sizeof(ArchetypeStart);

        for (u32 j = 0; j < n_components; j++) {
            bool should_not_add = true;
            for (u32 k = 0; k < start->n_components; k++) if (archetype_components[k] == components[j]) should_not_add = false;
            if (should_not_add) should_add = false;
        }

        if (should_add) {
            if (start->n_archetypes + 1 > start->max_archetypes) {
                start->max_archetypes *= ELC_MATH_GOLDEN_RATIO;
                list = realloc(list, sizeof(ArchetypeCacheStart) + (n_components * sizeof(Component)) + (start->max_archetypes * sizeof(void*)));
            }

            archetypes[start->n_archetypes++] = archetype;
        }
    }

    start->max_archetypes = start->n_archetypes;
    list = realloc(list, sizeof(ArchetypeCacheStart) + (n_components * sizeof(Component)) + (start->max_archetypes * sizeof(void*)));

    return list;
}

ELC_INLINE void addCacheEntry(ArchetypeCacheBucket* bucket, void* entry, u64 hash) {
    if (bucket->n_entries < ENTITIES_MAX_LISTS_PER_CACHE) {
        bucket->entries[bucket->n_entries] = entry;
        bucket->component_hashes[bucket->n_entries++] = hash;
    } else {
        if (bucket->entry_list == NULL) {
            bucket->max_entries = INITIAL_BUCKET_LIST_MAX;
            bucket->entry_list = malloc(bucket->max_entries * sizeof(void*));
            bucket->component_hash_list = malloc(bucket->max_entries * sizeof(u64));
        } else {
            if ((bucket->n_entries - INITIAL_BUCKET_LIST_MAX) + 1 > bucket->max_entries) {
                bucket->max_entries *= ELC_MATH_GOLDEN_RATIO;
                bucket->entry_list = realloc(bucket->entry_list, bucket->max_entries * sizeof(void*));
                bucket->component_hash_list = realloc(bucket->component_hash_list, bucket->max_entries * sizeof(u64));
            }
        }
        bucket->entry_list[bucket->n_entries - INITIAL_BUCKET_LIST_MAX] = entry;
        bucket->component_hash_list[bucket->n_entries++ - INITIAL_BUCKET_LIST_MAX] = hash;
    }
}

ELC_INLINE void freeArchetypeCache(ArchetypeCacheBucket* buckets, u32 n_buckets) {
    for (u32 i = 0; i < n_buckets; i++) if (buckets->entry_list != NULL) free(buckets->entry_list), free(buckets->component_hash_list);

    free(buckets);
}

ELC_INLINE void resizeArchetypeCache(Entities* entities, u32 n_cache) {
    ArchetypeCacheBucket* new_cache = calloc(n_cache, sizeof(ArchetypeCacheBucket));

    for (u32 i = 0; i < entities->n_cache; i++) {
        ArchetypeCacheBucket* src = &entities->cache[i];
        for (u32 j = 0; j < src->n_entries; j++) {
            EntryAndHash entry = getCacheEntry(src, i);
            u32 index = entry.hash % n_cache;

            addCacheEntry(&new_cache[index], entry.entry, entry.hash);
        }
    }

    freeArchetypeCache(entities->cache, entities->n_cache);

    entities->cache = new_cache;
    entities->n_cache = n_cache;
}

ELC_INLINE void* findArchetypesCachedInternal(Entities* entities, const Component* components, u32 n_components) {
    u64 hash = hashEntityComponents(components, n_components);
    u32 index = hash % entities->n_cache;

    ArchetypeCacheBucket* bucket = &entities->cache[index];

    for (u32 i = 0; i < bucket->n_entries; i++) {
        EntryAndHash entry = getCacheEntry(bucket, i);
        ArchetypeCacheStart* start = entry.entry;
        if (entry.hash == hash && hasCorrectComponents(entry.entry + sizeof(ArchetypeCacheStart), start->n_components, components, n_components))
            return entry.entry;
    }

    return NULL;
}

ELC_INLINE void* findArchetypesInternal(Entities* entities, const Component* components, u32 n_components) {
    u64 hash = hashEntityComponents(components, n_components);
    u32 index = hash % entities->n_cache;

    ArchetypeCacheBucket* bucket = &entities->cache[index];

    for (u32 i = 0; i < bucket->n_entries; i++) {
        EntryAndHash entry = getCacheEntry(bucket, i);
        ArchetypeCacheStart* start = entry.entry;
        if (entry.hash == hash && hasCorrectComponents(entry.entry + sizeof(ArchetypeCacheStart), start->n_components, components, n_components))
            return entry.entry;
    }

    void* entry = findArchetypesUncachedInternal(entities, components, n_components);
    addCacheEntry(bucket, entry, hash);

    return entry;
}

void* findArchetypesUnsorted(Entities* entities, const Component* components, u32 n_components) {
    return findArchetypesInternal(entities, components, n_components);
}

void* findArchetypes(Entities* entities, Component* components, u32 n_components) {
    sortComponentsInternal(components, n_components, NULL);
    return findArchetypesInternal(entities, components, n_components);
}

ELC_INLINE u32 findComponentListIndex(void* archetype, Component component) {
    ArchetypeStart* start = archetype;
    Component* components = archetype + sizeof(ArchetypeStart);
    u32 index = 0, highest = start->n_components - 1, lowest = 0;
    while (true) {
        if (components[index] == component) return index;
        if (component > components[index]) highest = index;
        if (component < components[index]) lowest = index;
        index = lowest + ((highest - lowest) / 2);
    }
    return UINT32_MAX;
}

void* getComponentData(void* archetype, Component component) {
    ArchetypeStart* start = archetype;
    void** component_lists = archetype + sizeof(ArchetypeStart) + (start->n_components * sizeof(Component));
    return component_lists[findComponentListIndex(archetype, component)];
}

u32 getArchetypeEntities(void* archetype) {
    return ((ArchetypeStart*)archetype)->n_entities;
}
