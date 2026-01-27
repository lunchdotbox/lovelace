#ifndef ENGINE_MATH_TRANSITION_H
#define ENGINE_MATH_TRANSITION_H

#include <elc/core.h>

ELC_INLINE double easeInCubic(double t) {
    return t * t * t;
}

ELC_INLINE double easeInQuad(double t) {
    return t * t;
}

ELC_INLINE double easeOutQuad(double t) {
    return -1.0 * t * (t - 2.0);
}

ELC_INLINE double easeOutElastic(double t) {
    if (t <= 0.0) return 0.0f;
    else if (t >= 1.0) return 1.0f;
    return pow(2.0, -10.0 * t) * sin((t * 10.0 - 0.75) * (2.0 * GLM_PI) / 3.0) + 1.0;
}

#endif
