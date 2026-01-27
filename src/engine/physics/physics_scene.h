#ifndef ENGINE_PHYSICS_SCENE_H
#define ENGINE_PHYSICS_SCENE_H

#include "constraints.h"
#include "particle.h"
#include "../graphics/model.h"
#include <cglm/affine-pre.h>
#include <cglm/quat.h>
#include <vulkan/vulkan_core.h>
#include "../graphics/simple_draw.h"

typedef struct PhysicsScene {
    Particle* particles;
    BallJointConstraint* constraints;
    u32 n_particles, max_particles;
    u32 n_constraints, max_constraints;
} PhysicsScene;

typedef struct ObjectRenderInfo {
    Model model;
    u32 texture_id;
} ObjectRenderInfo;

typedef struct PhysicsRenderer {
    ObjectRenderInfo* info;
    PhysicsScene scene;
} PhysicsRenderer; // TODO: maybe cleanup all the rendering stuff into a different folder

PhysicsScene createPhysicsScene(u32 max_particles, u32 max_constraints);
void destroyPhysicsScene(PhysicsScene scene);
PhysicsRenderer createPhysicsRenderer(u32 max_particles, u32 max_constraints);
void destroyPhysicsRenderer(PhysicsRenderer renderer);
Particle* addPhysicsParticle(PhysicsScene* scene, Particle particle);
Particle* addPhysicsObject(PhysicsRenderer* renderer, Particle particle, ObjectRenderInfo info);
void addPhysicsConstraint(PhysicsScene* scene, BallJointConstraint constraint);
void stepPhysicsScene(PhysicsScene scene, float dt, u32 iterations, u32 substeps);
void drawPhysicsRenderer(VkCommandBuffer command, Device device, DiffuseRenderer renderer, PhysicsRenderer physics);

#endif
