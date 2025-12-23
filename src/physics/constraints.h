#ifndef CONSTRAINTS_H
#define CONSTRAINTS_H

#include "particle.h"

typedef struct Constraint {
    vec3 linear_a;
    vec3 angular_a;
    vec3 linear_b;
    vec3 angular_b;
    float effective_mass;
    float bias;
} Constraint;

typedef struct BallJoint {
    Constraint constraints[3];
} BallJoint;

BallJoint createBallJoint(Particle body_a, Particle body_b, vec3 anchor_a, vec3 anchor_b, float dt);
void solveConstraint(Constraint c, Particle* body_a, Particle* body_b);
void solveBallJoint(BallJoint joint, Particle* body_a, Particle* body_b);

#endif
