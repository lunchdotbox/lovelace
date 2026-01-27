#ifndef ENGINE_PHYSICS_COLLISION_H
#define ENGINE_PHYSICS_COLLISION_H

#include "particle.h"
#include <elc/core.h>

typedef struct CollisionMesh {
    vec3s* points;
    uint64_t n_points;
} CollisionMesh;

typedef struct CollisionResult {
    Particle* body_a;
    Particle* body_b;
    vec3 normal;
    vec3 local_normal;
    vec3 contact_a;
    vec3 contact_b;
    vec3 local_a;
    vec3 local_b;
    float time_of_impact;
    float penetration;
} CollisionResult;

bool gilbertJohnsonKeerthi(Particle* body_a, CollisionMesh mesh_a, Particle* body_b, CollisionMesh mesh_b, CollisionResult* result);
bool intersectConservativeAdvance(Particle* body_a, CollisionMesh mesh_a, Particle* body_b, CollisionMesh mesh_b, float dt, CollisionResult* result);
void resolveCollision(CollisionResult* collision);

#endif
