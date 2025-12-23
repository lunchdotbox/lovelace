/* https://www.iust.ac.ir/files/mech/ayatgh_c5664/files/internal_combustion_engines_heywood.pdf */

#ifndef TUBULAR_FLUID_H
#define TUBULAR_FLUID_H

#include <cglm/cglm.h>
#include <elc/core.h>
#include <math.h>

typedef struct FluidSystemPart {
    vec2 momentum;
    float average_speed;
    float particle_count;
    float volume;
} FluidSystemPart;

float heatCapacityRatio(u32 freedom) {
    return 1.0f + (2.0f / freedom);
}

float chokedFlowLimit(u32 freedom) {
    float hcr = heatCapacityRatio(freedom);
    return powf(2.0f / (hcr + 1), hcr / (hcr - 1));
}

float chokedFlowRate(u32 freedom) {

}

float calculateFluidFlowRate(float k_flow, float p_a, float p_b, float t_a, float t_b, float hcr, float choked_limit, float choked_cache) {
    const float r = 8.31446261815324;

    if (k_flow == 0.0f) return 0.0f;

    float direction = (p_a > p_b) ? 1.0f : -1.0f;
    float ti_a = (p_a > p_b) ? t_a : t_b;
    float pi_a = (p_a > p_b) ? p_a : p_b;
    float p_t = (p_a > p_b) ? p_b : p_a;

    float flow_rate;
    float p_ratio = p_t / pi_a;
    if (p_ratio <= choked_limit) flow_rate = choked_cache / sqrtf(r * ti_a);
    else {
        float s = powf(p_ratio, 1.0f / hcr);

        flow_rate = (2.0f * hcr) / (hcr - 1);
        flow_rate *= s * (s - p_ratio);
        flow_rate = sqrtf(MAX(flow_rate, 0.0f) / (r * ti_a));
    }

    return flow_rate * (direction * pi_a);
}

#endif
