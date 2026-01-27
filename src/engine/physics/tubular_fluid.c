#include "tubular_fluid.h"

#include <elc/core.h>

static const double gas_system_r = 8.31446261815324;
static const double gas_system_kg = 1.0;
static const double gas_system_g = gas_system_kg / 1000.0;
static const double gas_system_mol = 1.0;
static const double gas_system_air_molocule_mass = (28.97 * gas_system_g) / gas_system_mol;

void printTotal(const char* message, GasSystem* gas_a, GasSystem* gas_b) {
    printf("total energy after %s: %.9f\n", message, totalEnergy(*gas_a) + totalEnergy(*gas_b));
}

/*ELC_INLINE*/ double heatCapacityRatio(u32 freedom) {
    return 1.0 + (2.0 / freedom);
}

/*ELC_INLINE*/ double chokedFlowLimit(u32 freedom) {
    double hcr = heatCapacityRatio(freedom);
    return pow(2.0 / (hcr + 1), hcr / (hcr - 1));
}

/*ELC_INLINE*/ double chokedFlowRate(u32 freedom) {
    double hcr = heatCapacityRatio(freedom);
    return sqrt(hcr) * pow(2.0 / (hcr + 1.0), (hcr + 1.0) / (2.0 * (hcr * 1.0)));
}

double approximateDensity(GasSystem gas) {
    return (gas_system_air_molocule_mass * gas.particle_count) / gas.volume;
}

/*ELC_INLINE*/ double kineticEnergy(GasSystem gas, double n) {
    return (gas.total_speed / gas.particle_count) * n;
}

double pressure(GasSystem gas) {
    return (gas.volume != 0.0) ? gas.total_speed / (0.5 * gas.freedom * gas.volume) : 0.0;
}

/*ELC_INLINE*/ double gasSystemC(GasSystem gas) {
    if (gas.particle_count == 0.0 || gas.total_speed == 0.0) return 0.0;

    return sqrt(pressure(gas) * heatCapacityRatio(gas.freedom) / approximateDensity(gas));
}

double mass(GasSystem gas) {
    return gas_system_air_molocule_mass * gas.particle_count;
}

double velocityX(GasSystem gas) {
    if (gas.particle_count == 0.0) return 0.0;
    return gas.momentum[0] / mass(gas);
}

double velocityY(GasSystem gas) {
    if (gas.particle_count == 0.0) return 0.0;
    return gas.momentum[1] / mass(gas);
}

double temperature(GasSystem gas) {
    if (gas.particle_count == 0.0) return 0.0;
    return gas.total_speed / (0.5 * gas.freedom * gas.particle_count * gas_system_r);
}

double totalEnergy(GasSystem gas) {
    if (gas.particle_count == 0.0) return 0.0;

    double inv_mass = 1.0 / mass(gas);
    double v_x = gas.momentum[0] * inv_mass;
    double v_y = gas.momentum[1] * inv_mass;
    double v = pow2(v_x) + pow2(v_y);

    return gas.total_speed + 0.5 * mass(gas) * v;
}

/*ELC_INLINE*/ double bulkKineticEnergy(GasSystem gas) {
    if (mass(gas) == 0.0) return 0.0;

    double v_x = gas.momentum[0] / mass(gas);
    double v_y = gas.momentum[1] / mass(gas);
    double v = pow2(v_x) + pow2(v_y);

    return gas.total_speed * mass(gas) * v;
}

/*ELC_INLINE*/ double freedomToXd(u32 freedom, double x) {
    switch (freedom) {
        case 3: return x * x * x * x * x;
        case 5: return x * x * x * x * x * x * x;
        default: return x;
    }
}

/*ELC_INLINE*/ double dynamicPressure(GasSystem gas, double dx, double dy) {
    if (gas.particle_count == 0.0 || gas.total_speed == 0.0) return 0.0;

    double inv_mass = 1.0 / mass(gas);
    double v = inv_mass * (dx * gas.momentum[0] + dy * gas.momentum[1]);

    if (v <= 0.0) return 0.0;

    double hcr = heatCapacityRatio(gas.freedom);
    double press = pressure(gas);
    double density = approximateDensity(gas);
    double c_sq = press * hcr / density;
    double mach_sq = pow2(v) / c_sq;

    double x = 1.0 + ((hcr - 1.0) / 2.0) * mach_sq;

    return press * (sqrt(freedomToXd(gas.freedom, x)) - 1.0);
}

/*ELC_INLINE*/ double flowRate(double k_flow, double p_a, double p_b, double t_a, double t_b, double hcr, double choked_limit, double choked_cache) {
    if (k_flow == 0.0) return 0.0;

    double direction = (p_a > p_b) ? 1.0 : -1.0;
    double ti_a = (p_a > p_b) ? t_a : t_b;
    double pi_a = (p_a > p_b) ? p_a : p_b;
    double p_t = (p_a > p_b) ? p_b : p_a;

    double flow_rate;
    double p_ratio = p_t / pi_a;
    if (p_ratio <= choked_limit) flow_rate = choked_cache / sqrt(gas_system_r * ti_a);
    else {
        double s = pow(p_ratio, 1.0 / hcr);

        flow_rate = (2.0 * hcr) / (hcr - 1);
        flow_rate *= s * (s - p_ratio);
        flow_rate = sqrt(MAX(flow_rate, 0.0) / (gas_system_r * ti_a));
    }

    return flow_rate * (direction * pi_a);
}

/*ELC_INLINE*/ double flowConstant(double target, double p, double drop, double t, double hcr) {
    double p_t = p - drop;

    double choked_limit = pow(2.0 / (hcr + 1.0), hcr / (hcr - 1.0));
    double p_r = p_t / p;

    double flow_rate;
    if (p_r <= choked_limit) flow_rate = sqrt(hcr) * pow(2.0 / (hcr + 1.0), (hcr + 1.0) / (2.0 * (hcr - 1.0)));
    else {
        flow_rate = ((2.0 * hcr) / (hcr - 1.0)) * (1.0 - pow(p_r, (hcr - 1.0) / hcr));
        flow_rate = sqrt(flow_rate) * pow(p_r, 1.0 / hcr);
    }

    flow_rate *= p / sqrt(gas_system_r * t);

    return target / flow_rate;
}

void dissipateExcessVelocity(GasSystem* gas) {
    double v = pow2(velocityX(*gas)) + pow2(velocityY(*gas));
    double c = pow2(gasSystemC(*gas));

    if (c >= v || v == 0.0) return;

    double k = sqrt(c / v);

    gas->momentum[0] *= k;
    gas->momentum[1] *= k;

    gas->total_speed += 0.5 * mass(*gas) * (v - c);

    if (gas->total_speed < 0.0) gas->total_speed = 0.0;
}

void updateVelocity(GasSystem* gas, u32 freedom, double dt, double beta) {
    if (gas->particle_count == 0.0) return;

    double depth = gas->volume / (gas->width * gas->height);

    double p_a = dynamicPressure(*gas, gas->dx, gas->dy) * gas->height * depth;
    double p_b = dynamicPressure(*gas, -gas->dx, -gas->dy) * gas->height * depth;
    double p_c = dynamicPressure(*gas, gas->dy, gas->dx) * gas->width * depth;
    double p_d = dynamicPressure(*gas, -gas->dy, -gas->dx) * gas->width * depth;

    double dm_x = (((p_a * gas->dx) - (p_b * gas->dx)) + (p_c * gas->dx)) - (p_d * gas->dx);
    double dm_y = (((p_a * gas->dy) - (p_b * gas->dy)) + (p_c * gas->dy)) - (p_d * gas->dy);

    double inv_mass = 1.0 / mass(*gas);

    double va_x = gas->momentum[0] * inv_mass;
    double va_y = gas->momentum[1] * inv_mass;

    gas->momentum[0] -= dm_x * dt * beta;
    gas->momentum[1] -= dm_y * dt * beta;

    double vb_x = gas->momentum[0] * inv_mass;
    double vb_y = gas->momentum[1] * inv_mass;

    gas->total_speed -= 0.5 * mass(*gas) * (pow2(vb_x) - pow2(va_x));
    gas->total_speed -= 0.5 * mass(*gas) * (pow2(vb_y) - pow2(va_y));

    if (gas->total_speed < 0.0) gas->total_speed = 0.0;
}

void dissipateVelocity(GasSystem* gas, double dt, double tc) {
    if (gas->particle_count == 0.0) return;

    double inv_mass = 1.0 / mass(*gas);

    double v_x = gas->momentum[0] * inv_mass;
    double v_y = gas->momentum[1] * inv_mass;
    double v = pow2(v_x) + pow2(v_y);

    double s = dt / (dt + tc);
    gas->momentum[0] *= 1.0 - s;
    gas->momentum[1] *= 1.0 - s;

    double nv_x = gas->momentum[0] * inv_mass;
    double nv_y = gas->momentum[1] * inv_mass;
    double nv = pow2(nv_x) + pow2(nv_y);

    gas->total_speed += 0.5 * mass(*gas) * (v - nv);
}

double gasSystemGainN(GasSystem* gas, double dn, double epm) {
    gas->total_speed += dn * epm;
    gas->particle_count += dn;

    return -dn;
}

double gasSystemLoseN(GasSystem* gas, double dn, double epm) {
    gas->total_speed -= dn * epm;
    gas->particle_count = MAX(0.0, gas->particle_count - dn);

    return dn;
}

/*ELC_INLINE*/ double pressureEquilibriumMaxFlow(GasSystem gas_a, GasSystem gas_b) {
    if (pressure(gas_a) > pressure(gas_b))
        return MAX(0.0, MIN((gas_b.volume * gas_a.total_speed - gas_a.volume * gas_b.total_speed) / (gas_b.volume * kineticEnergy(gas_a, 1.0) + gas_a.volume * kineticEnergy(gas_a, 1.0)), gas_a.particle_count));
    else
        return MIN(0.0, MAX((gas_b.volume * gas_a.total_speed - gas_a.volume * gas_b.total_speed) / (gas_b.volume * kineticEnergy(gas_b, 1.0) + gas_a.volume * kineticEnergy(gas_b, 1.0)), -gas_b.particle_count));
}

/*ELC_INLINE*/ void updateFractionMomentum(GasSystem* gas, dvec2 dir, double area, double fract_mass, double fract_vol, double c, double dt) {
    double fract_vel = CLAMP((fract_vol / area) / dt, 0.0, c);

    gas->momentum[0] += fract_vel * dir[0] * fract_mass;
    gas->momentum[1] += fract_vel * dir[1] * fract_mass;
}

/*ELC_INLINE*/ void conserveEnergy(GasSystem* gas, dvec2 initial_momentum, double m, double inv_mass) {
    gas->total_speed -= 0.5 * m * (pow2(gas->momentum[0] * inv_mass) - pow2(initial_momentum[0] * inv_mass));
    gas->total_speed -= 0.5 * m * (pow2(gas->momentum[1] * inv_mass) - pow2(initial_momentum[1] * inv_mass));
}

/*ELC_INLINE*/ double flowSourceToSink(GasSystem* src, GasSystem* dst, dvec2 dir, double flow_dir, double area_src, double area_dst, double p_src, double p_dst, double k_flow, double dt) {
    printTotal("start", src, dst);
    double flow = dt * flowRate(k_flow, p_src, p_dst, temperature(*src), temperature(*dst), heatCapacityRatio(src->freedom), src->choked_limit, src->choked_cache);

    double max_flow = pressureEquilibriumMaxFlow(*src, *dst);
    flow = CLAMP(flow, 0.0, 0.9 * src->particle_count);

    double fraction = flow / src->particle_count;
    double fract_vol = fraction * src->volume;
    double fract_mass = fraction * mass(*src);
    double rem_mass = (1.0 - fraction) * mass(*src);

    if (flow != 0.0) {
        double bk_src_a = bulkKineticEnergy(*src);
        double bk_dst_a = bulkKineticEnergy(*dst);

        double s_a = totalEnergy(*src) + totalEnergy(*dst);

        double epm = kineticEnergy(*src, 1.0);
        gasSystemGainN(dst, flow, epm);
        gasSystemLoseN(src, flow, epm);
        printTotal("gain and lose", src, dst);

        double s_b = totalEnergy(*src) + totalEnergy(*dst);

        dst->momentum[0] += src->momentum[0] * fraction;
        dst->momentum[1] += src->momentum[1] * fraction;

        src->momentum[0] -= src->momentum[0] * fraction;
        src->momentum[1] -= src->momentum[1] * fraction;

        printTotal("momentum update", src, dst);

        double bk_src_b = bulkKineticEnergy(*src);
        double bk_dst_b = bulkKineticEnergy(*dst);

        dst->total_speed -= (bk_src_b + bk_dst_b) - (bk_src_a + bk_dst_a);

        printTotal("kinetic energy update", src, dst);
    }

    double mass_src = mass(*src);
    double im_src = 1.0 / mass_src;
    double mass_dst = mass(*src);
    double im_dst = 1.0 / mass_dst;

    double c_src = gasSystemC(*src);
    double c_dst = gasSystemC(*dst);

    dvec2 initial_momentum_src, initial_momentum_dst;
    elc_dvec2_copy(src->momentum, initial_momentum_src);
    elc_dvec2_copy(dst->momentum, initial_momentum_dst);

    if (area_dst != 0.0) updateFractionMomentum(dst, dir, area_dst, fract_mass, fract_vol, c_dst, dt);
    if (area_src != 0.0 && mass_src != 0.0) updateFractionMomentum(src, dir, area_src, fract_mass, fract_vol, c_src, dt);

    if (mass_src != 0.0) conserveEnergy(src, initial_momentum_src, mass_src, im_src);
    if (mass_dst > 0.0) conserveEnergy(dst, initial_momentum_dst, mass_dst, im_dst);

    if (src->total_speed < 0.0) src->total_speed = 0.0;
    if (dst->total_speed < 0.0) dst->total_speed = 0.0;

    return flow * flow_dir;
}

double gasSystemFlow(GasSystem* gas_a, GasSystem* gas_b, dvec2 dir, double area_a, double area_b, double k_flow, double dt) {
    double p_a = pressure(*gas_a) + dynamicPressure(*gas_a, dir[0], dir[1]);
    double p_b = pressure(*gas_b) + dynamicPressure(*gas_b, -dir[0], -dir[1]);

    if (p_a > p_b) return flowSourceToSink(gas_a, gas_b, dir, 1.0, area_a, area_b, p_a, p_b, k_flow, dt);
    else return flowSourceToSink(gas_b, gas_a, dir, -1.0, area_b, area_a, p_b, p_a, k_flow, dt);
}

GasSystem createGasSystem(double p, double v, double t, double width, double height, double dx, double dy, u32 freedom) {
    GasSystem gas = {.freedom = freedom, .volume = v, .width = width, .height = height, .dx = dx, .dy = dy};
    gas.particle_count = p * gas.volume / (gas_system_r * t);
    gas.total_speed = t * (0.5 * gas.freedom * gas.particle_count * gas_system_r);
    gas.choked_limit = chokedFlowLimit(gas.freedom);
    gas.choked_cache = chokedFlowRate(gas.freedom);
    return gas;
}
