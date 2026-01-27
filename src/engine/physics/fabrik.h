#ifndef ENGINE_PHYSICS_FABRIK_H
#define ENGINE_PHYSICS_FABRIK_H

#include <cglm/types-struct.h>
#include <cglm/vec3.h>
#include <elc/core.h>

void computeInverseKinematics(vec3s* positions, u32 n_positions, float* lengths, vec3 start, vec3 end, u32 iterations);

#endif
