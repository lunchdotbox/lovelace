#ifndef ENGINE_PHYSICS_PARTICLE_H
#define ENGINE_PHYSICS_PARTICLE_H

#include <elc/core.h>

typedef struct Particle {
    vec3 position;
    vec3 velocity;
    float mass;
    versor rotation;
    vec3 omega;
    mat3 inertia;
} Particle;

typedef struct PhysicsComponent {
    vec3 velocity;
    float mass;
    vec3 omega;
    mat3 inertia;
} PhysicsComponent;

void inertiaTensorWorld(Particle particle, mat3 tensor);
float getInertiaScalar(Particle particle, vec3 point, vec3 normal);
void applyParticleVelocity(Particle* particle, float dt);
void worldToBodySpace(Particle* body, vec3 point, vec3 result);
void applyAngularImpulse(Particle* body, vec3 impulse);
void applyLinearImpulse(Particle* body, vec3 impulse);
void applyGeneralImpulse(Particle* body, vec3 impulse, vec3 location);
void applyParticleGravity(Particle* particle, float dt);

#endif
