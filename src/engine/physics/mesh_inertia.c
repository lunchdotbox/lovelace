#include "mesh_inertia.h"

#include <cglm/mat3.h>
#include <cglm/util.h>
#include <elc/core.h>

ELC_INLINE float computeInertiaMoment(vec3s p[3], u32 i) {
    return (p[0].raw[i] * p[0].raw[i]) + (p[1].raw[i] * p[2].raw[i])
         + (p[1].raw[i] * p[1].raw[i]) + (p[0].raw[i] * p[2].raw[i])
         + (p[2].raw[i] * p[2].raw[i]) + (p[0].raw[i] * p[1].raw[i]);
}

ELC_INLINE float computeInertiaProduct(vec3s p[3], u32 i, u32 j) {
    return (2.0f * p[0].raw[i] * p[0].raw[j]) + (p[1].raw[i] * p[2].raw[j]) + (p[2].raw[i] * p[1].raw[j])
         + (2.0f * p[1].raw[i] * p[1].raw[j]) + (p[0].raw[i] * p[2].raw[j]) + (p[2].raw[i] * p[0].raw[j])
         + (2.0f * p[2].raw[i] * p[2].raw[j]) + (p[0].raw[i] * p[1].raw[j]) + (p[1].raw[i] * p[0].raw[j]);
}

ELC_INLINE float scalarTripleProduct(vec3 a, vec3 b, vec3 c) {
    vec3 temp;
    glm_vec3_cross(b, c, temp);
    return glm_vec3_dot(a, temp);
}

void computeMeshInertia(HostMesh mesh, float density, mat3 inertia, vec3 com, float* m) {
    float ia = 0.0, ib = 0.0, ic = 0.0, iap = 0.0, ibp = 0.0, icp = 0.0;
    vec3 mass_center = {0};
    float mass = 0.0f;

    for (u32 i = 0; i < mesh.n_indices; i += 3) {
        vec3s p[3];
        vec3 tet_center = {0};

        for (u32 j = 0; j < 3; j++) glm_vec3_copy(mesh.vertices[mesh.indices[i + j]].position, p[j].raw);

        float det_j = scalarTripleProduct(p[0].raw, p[1].raw, p[2].raw);
        float tet_volume = det_j / 6.0f;
        float tet_mass = density * tet_volume;

        for (u32 j = 0; j < 3; j++) glm_vec3_add(tet_center, p[j].raw, tet_center);
        glm_vec3_scale(tet_center, 0.25f, tet_center);

        float v100 = computeInertiaMoment(p, 0);
        float v010 = computeInertiaMoment(p, 1);
        float v001 = computeInertiaMoment(p, 2);

        ia += det_j * (v010 + v001);
        ib += det_j * (v100 + v001);
        ic += det_j * (v100 + v010);
        iap += det_j * computeInertiaProduct(p, 1, 2);
        ibp += det_j * computeInertiaProduct(p, 0, 1);
        icp += det_j * computeInertiaProduct(p, 0, 2);

        glm_vec3_muladds(tet_center, tet_mass, mass_center);
        mass += tet_mass;
    }

    glm_vec3_divs(mass_center, mass, mass_center);
    ia = (density * ia / 60.0) - (mass * (glm_pow2(mass_center[1]) + glm_pow2(mass_center[2])));
    ib = (density * ib / 60.0) - (mass * (glm_pow2(mass_center[0]) + glm_pow2(mass_center[2])));
    ic = (density * ic / 60.0) - (mass * (glm_pow2(mass_center[0]) + glm_pow2(mass_center[1])));
    iap = (density * iap / 120.0) - (mass * (mass_center[1] * mass_center[2]));
    ibp = (density * ibp / 120.0) - (mass * (mass_center[0] * mass_center[1]));
    icp = (density * icp / 120.0) - (mass * (mass_center[0] * mass_center[2]));

    mat3 m_i = {
        {ia, -ibp, -icp},
        {-ibp, ib, -iap},
        {-icp, -iap, ic},
    };

    glm_mat3_inv(m_i, inertia);
    glm_vec3_copy(mass_center, com);
    (*m) = 1.0f / ABS(mass);
}

void shiftMeshBackwards(HostMesh mesh, vec3 com) {
    for (u32 i = 0; i < mesh.n_vertices; i++) glm_vec3_mulsubs(com, 1.0f, mesh.vertices[i].position);
}
