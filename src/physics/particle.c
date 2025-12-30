#include "particle.h"
#include <cglm/mat3.h>

void inertiaTensorWorld(Particle particle, mat3 tensor) {
    if (particle.mass == 0.0f) {
        glm_mat3_identity(tensor);
        return;
    }
    mat3 orient, inertia;
    glm_quat_mat3(particle.rotation, orient);
    glm_mat3_copy(particle.inertia, inertia);
    glm_mat3_scale(inertia, particle.mass);
    glm_mat3_mul(orient, inertia, tensor);
    glm_mat3_transpose(orient);
    glm_mat3_mul(tensor, orient, tensor);
}

float getInertiaScalar(Particle particle, vec3 point, vec3 normal) {
    mat3 inertia;
    vec3 ang, accel;
    glm_vec3_cross(point, normal, ang);
    inertiaTensorWorld(particle, inertia);
    glm_mat3_mulv(inertia, ang, accel);
    return glm_vec3_dot(accel, ang);
}

void applyParticleVelocity(Particle* particle, float dt) {
    glm_vec3_muladds(particle->velocity, dt, particle->position);

    vec3 alpha;
    mat3 inertia, inv_inertia;
    inertiaTensorWorld(*particle, inertia);
    glm_mat3_mulv(inertia, particle->omega, alpha);
    glm_vec3_cross(particle->omega, alpha, alpha);
    glm_mat3_inv(inertia, inv_inertia);
    glm_mat3_mulv(inv_inertia, alpha, alpha);
    glm_vec3_muladds(alpha, dt, particle->omega);

    vec3 axis;
    glm_vec3_scale(particle->omega, dt * 0.5f, axis);
    if (!glm_vec3_eq_eps(axis, 0.0f)) {
        versor rotation = {VEC3_SPLIT(axis), 0.0f};
        glm_quat_mul(rotation, particle->rotation, rotation);
        glm_quat_add(particle->rotation, rotation, particle->rotation);
        glm_quat_normalize(particle->rotation);
    }
}

void worldToBodySpace(Particle* body, vec3 point, vec3 result) {
    glm_vec3_sub(point, body->position, result);
    versor inverse_rotation;
    glm_quat_inv(body->rotation, inverse_rotation);
    glm_quat_rotatev(inverse_rotation, result, result);
}

void applyAngularImpulse(Particle* body, vec3 impulse) {
    mat3 inertia;
    inertiaTensorWorld(*body, inertia);
    vec3 to_add;
    glm_mat3_mulv(inertia, impulse, to_add);
    glm_vec3_add(body->omega, to_add, body->omega);
}

void applyLinearImpulse(Particle* body, vec3 impulse) {
    if (body->mass == 0.0f) return;
    glm_vec3_muladds(impulse, body->mass, body->velocity);
}

void applyGeneralImpulse(Particle* body, vec3 impulse, vec3 location) {
    applyLinearImpulse(body, impulse);
    vec3 contact, angular;
    glm_vec3_cross(location, impulse, angular);
    applyAngularImpulse(body, angular);
}

void applyParticleGravity(Particle* particle, float dt) {
    if (particle->mass == 0.0f) return;
    particle->velocity[1] += 9.8f * dt;
}
