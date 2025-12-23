/* https://www.iust.ac.ir/files/mech/ayatgh_c5664/files/internal_combustion_engines_heywood.pdf */

#ifndef TUBULAR_FLUID_H
#define TUBULAR_FLUID_H

#include <cglm/cglm.h>
#include <elc/core.h>

typedef struct FluidSystemPart {
    vec2 momentum;
    float average_speed;
    float particle_count;
    float volume;
} FluidSystemPart;

float calculateFluidFlowRate(FluidSystemPart part_a, FluidSystemPart part_b) { /* this function is really just a guess and probably not correct */
    float p_a = (part_a.particle_count * part_a.average_speed) / part_a.volume; /* pressure of fluid system part A */
    float p_b = (part_b.particle_count * part_b.average_speed) / part_b.volume; /* pressure of fluid system part B */

    float t_a = part_a.average_speed / part_a.volume;
    float t_b = part_b.average_speed / part_b.volume;

    float pd = p_a - p_b; /* pressure difference between the two parts */
    float oa = 0.1f; /* oriface area in square meters */
    float dc = 0.6; /* discharge coefficient */
    float r = 287;

    float d_a = p_a / (r * t_a);
    float d_b = p_b / (r * t_b);

    if (MAX(p_a, p_b) / MIN(p_a, p_b) > 2.0f) {

    }
}

#endif
