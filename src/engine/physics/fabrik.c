#include "fabrik.h"

void computeInverseKinematics(vec3s* positions, u32 n_positions, float* lengths, vec3 start, vec3 end, u32 iterations) {
    for (u32 i = 0; i < iterations; i++) {
        glm_vec3_copy(end, positions[n_positions - 1].raw);
        for (u32 j = n_positions - 1; j > 1; j--) {
            vec3 direction;
            glm_vec3_sub(positions[j - 1].raw, positions[j].raw, direction);
            glm_vec3_normalize(direction);
            glm_vec3_scale(direction, lengths[j - 1], direction);
            glm_vec3_add(positions[j].raw, direction, positions[j - 1].raw);
        }

        glm_vec3_copy(start, positions[0].raw);
        for (u32 j = 0; j < n_positions - 1; j++) {
            vec3 direction;
            glm_vec3_sub(positions[j + 1].raw, positions[j].raw, direction);
            glm_vec3_normalize(direction);
            glm_vec3_scale(direction, lengths[j], direction);
            glm_vec3_add(positions[j].raw, direction, positions[j + 1].raw);
        }
    }
}
