#include "physics_scene.h"

#include "constraints.h"
#include "mesh_inertia.h"
#include "particle.h"
#include "../formats/wavefront.h"
#include <elc/core.h>
#include <vulkan/vulkan_core.h>

PhysicsScene createPhysicsScene(u32 max_particles, u32 max_constraints) {
    PhysicsScene scene = {.max_constraints = max_constraints, .max_particles = max_particles};
    scene.constraints = malloc(scene.max_constraints * sizeof(BallJointConstraint));
    scene.particles = malloc(scene.max_particles * sizeof(Particle));
    return scene;
}

void destroyPhysicsScene(PhysicsScene scene) {
    free(scene.constraints), free(scene.particles);
}

PhysicsRenderer createPhysicsRenderer(u32 max_particles, u32 max_constraints) {
    PhysicsRenderer renderer = {.scene = createPhysicsScene(max_particles, max_constraints)};
    renderer.info = malloc(renderer.scene.max_particles * sizeof(ObjectRenderInfo));
    for (u32 i = 0; i < max_particles; i++) renderer.info[i] = (ObjectRenderInfo){0};
    return renderer;
}

void destroyPhysicsRenderer(PhysicsRenderer renderer) {
    free(renderer.info), destroyPhysicsScene(renderer.scene);
}

Particle* addPhysicsParticle(PhysicsScene* scene, Particle particle) {
    if (scene->n_particles + 1 > scene->max_particles) puts("error: not enough space in particle array"), exit(1); // TODO: add error handling or expanding array
    scene->particles[scene->n_particles++] = particle;
    return &scene->particles[scene->n_particles - 1];
}

Particle* addPhysicsObject(PhysicsRenderer* renderer, Particle particle, ObjectRenderInfo info) {
    renderer->info[renderer->scene.n_particles] = info;
    return addPhysicsParticle(&renderer->scene, particle);
}

void addPhysicsConstraint(PhysicsScene* scene, BallJointConstraint constraint) { // TODO: rename to add more types of constraints
    if (scene->n_constraints + 1 > scene->max_constraints) puts("error: not enough space in constraint array"), exit(1); // TODO: add error handling or expanding array
    scene->constraints[scene->n_constraints++] = constraint;
}

void stepPhysicsScene(PhysicsScene scene, float dt, u32 iterations, u32 substeps) {
    dt /= substeps;
    for (u32 i = 0; i < substeps; i++) {
        for (u32 j = 0; j < scene.n_particles; j++) applyParticleGravity(&scene.particles[j], dt);

        for (u32 j = 0; j < iterations; j++) {
            for (u32 k = 0; k < scene.n_constraints; k++) {
                BallJoint constraint = createBallJoint(*scene.constraints[k].particle_a, *scene.constraints[k].particle_b, scene.constraints[k].anchor_a, scene.constraints[k].anchor_b, dt);
                solveBallJoint(constraint, scene.constraints[k].particle_a, scene.constraints[k].particle_b);
            }
        }

        for (u32 j = 0; j < scene.n_particles; j++) applyParticleVelocity(&scene.particles[j], dt);
    }
}

void drawPhysicsRenderer(VkCommandBuffer command, Device device, DiffuseRenderer renderer, PhysicsRenderer physics) {
    for (u32 i = 0; i < physics.scene.n_particles; i++) {
        if (physics.info[i].model.index_buffer == VK_NULL_HANDLE) continue;
        mat4 transform;
        glm_quat_mat4(physics.scene.particles[i].rotation, transform);
        glm_translate(transform, physics.scene.particles[i].position);
        drawTexturedModel(command, renderer, device, physics.info[i].model, physics.info[i].texture_id, transform);
    }
}
