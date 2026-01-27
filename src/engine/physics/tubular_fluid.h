/* https://www.iust.ac.ir/files/mech/ayatgh_c5664/files/internal_combustion_engines_heywood.pdf */

#ifndef ENGINE_PHYSICS_TUBULAR_FLUID_H
#define ENGINE_PHYSICS_TUBULAR_FLUID_H

#include <cglm/cglm.h>
#include <cglm/util.h>
#include <elc/core.h>

typedef struct GasSystem {
    dvec2 momentum;
    double total_speed; // E_k
    double particle_count; // n_mol
    double volume; // V
    double width, height;
    double dx, dy;
    double choked_limit;
    double choked_cache;
    u32 freedom;
} GasSystem;

double approximateDensity(GasSystem gas);
double pressure(GasSystem gas);
double mass(GasSystem gas);
double velocityX(GasSystem gas);
double velocityY(GasSystem gas);
double temperature(GasSystem gas);
double totalEnergy(GasSystem gas);
void dissipateExcessVelocity(GasSystem* gas);
void updateVelocity(GasSystem* gas, u32 freedom, double dt, double beta);
void dissipateVelocity(GasSystem* gas, double dt, double tc);
double gasSystemGainN(GasSystem* gas, double dn, double epm);
double gasSystemLoseN(GasSystem* gas, double dn, double epm);
double gasSystemFlow(GasSystem* gas_a, GasSystem* gas_b, dvec2 dir, double area_a, double area_b, double k_flow, double dt);
GasSystem createGasSystem(double p, double v, double t, double width, double height, double dx, double dy, u32 freedom);

#endif
