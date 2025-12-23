#include "constraints.h"
#include <elc/core.h>

BallJoint createBallJoint(Particle body_a, Particle body_b, vec3 anchor_a, vec3 anchor_b, float dt) {
    BallJoint joint;

    float inverse_mass = body_a.mass + body_b.mass;

    vec3 r_a, r_b, world_a, world_b, diff;
    glm_quat_rotatev(body_a.rotation, anchor_a, r_a);
    glm_quat_rotatev(body_b.rotation, anchor_b, r_b);
    glm_vec3_add(body_a.position, r_a, world_a);
    glm_vec3_add(body_b.position, r_b, world_b);
    glm_vec3_sub(world_b, world_a, diff);

    for (u32 i = 0; i < 3; i++) {
        vec3 normal = {0};
        normal[i] = 1.0f;

        glm_vec3_negate_to(normal, joint.constraints[i].linear_a);
        glm_vec3_cross(r_a, joint.constraints[i].linear_a, joint.constraints[i].angular_a);

        glm_vec3_copy(normal, joint.constraints[i].linear_b);
        glm_vec3_cross(r_b, normal, joint.constraints[i].angular_b);

        float normal_inertia = 0.0f;
        if (!glm_eq(body_a.mass, 0.0f)) normal_inertia += getInertiaScalar(body_a, r_a, joint.constraints[i].linear_a);
        if (!glm_eq(body_b.mass, 0.0f)) normal_inertia += getInertiaScalar(body_b, r_b, normal);

        float rigid_denom = inverse_mass + normal_inertia;
        float position_error = glm_vec3_dot(diff, normal);
        float denom = 0.2f / dt;

        joint.constraints[i].effective_mass = 1.0f / rigid_denom;
        joint.constraints[i].bias = denom * position_error;
    }

    return joint;
}

ELC_INLINE float constraintRelativeVelocity(Constraint c, Particle body_a, Particle body_b) {
    return glm_vec3_dot(c.linear_a, body_a.velocity) + glm_vec3_dot(c.angular_a, body_a.omega) + glm_vec3_dot(c.linear_b, body_b.velocity) + glm_vec3_dot(c.angular_b, body_b.omega);
}

void solveConstraint(Constraint c, Particle* body_a, Particle* body_b) {
    float relative_velocity = constraintRelativeVelocity(c, *body_a, *body_b);
    float lambda = -(relative_velocity + c.bias) * c.effective_mass;

    if (body_a->mass != 0.0f) {
        glm_vec3_scale(c.linear_a, lambda, c.linear_a);
        applyLinearImpulse(body_a, c.linear_a);
        glm_vec3_scale(c.angular_a, lambda, c.angular_a);
        applyAngularImpulse(body_a, c.angular_a);
    }

    if (body_b->mass != 0.0f) {
        glm_vec3_scale(c.linear_b, lambda, c.linear_b);
        applyLinearImpulse(body_b, c.linear_b);

        glm_vec3_scale(c.angular_b, lambda, c.angular_b);
        applyAngularImpulse(body_b, c.angular_b);
    }
}

void solveBallJoint(BallJoint joint, Particle* body_a, Particle* body_b) {
    solveConstraint(joint.constraints[0], body_a, body_b);
    solveConstraint(joint.constraints[1], body_a, body_b);
    solveConstraint(joint.constraints[2], body_a, body_b);
}
