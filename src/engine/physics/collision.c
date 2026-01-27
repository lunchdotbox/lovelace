#include "collision.h"

typedef struct SupportPoint {
    vec3 original_a;
    vec3 original_b;
    vec3 minkowski;
} SupportPoint;

typedef struct Simplex {
    SupportPoint points[4];
    uint64_t n_points;
} Simplex;

typedef struct Face {
    uint64_t a, b, c;
} Face;

typedef struct Edge {
    uint64_t a, b;
} Edge;

ELC_INLINE void bodySpaceToWorld(Particle* body, vec3 point, vec3 result) {
    glm_quat_rotatev(body->rotation, point, result);
    glm_vec3_add(result, body->position, result);
}

ELC_INLINE void findFurthestPoint(Particle* body, CollisionMesh mesh, vec3 direction, vec3 result) {
    float max_distance = -FLT_MAX; // sets the starting max distance to the lowest possible float so every distance is bigger

    for (uint64_t i = 0; i < mesh.n_points; i++) {
        vec3 point;
        bodySpaceToWorld(body, point, point);
        float distance = glm_dot(point, direction); // gets essentially the amount that the vectors line up times the distance
        if (distance > max_distance) {
            max_distance = distance;
            glm_vec3_copy(point, result);
        }
    }
}

ELC_INLINE SupportPoint calculateSupportPoint(Particle* body_a, CollisionMesh mesh_a, Particle* body_b, CollisionMesh mesh_b, vec3 direction) { // TODO: update to use center of mass
    SupportPoint result;
    vec3 internal_direction;
    glm_vec3_copy(direction, internal_direction);
    findFurthestPoint(body_a, mesh_a, internal_direction, result.original_a);
    glm_vec3_negate(internal_direction);
    findFurthestPoint(body_b, mesh_b, internal_direction, result.original_b);
    glm_vec3_sub(result.original_a, result.original_b, result.minkowski);
    return result;
}

ELC_INLINE bool checkLineWithOrigin(Simplex* points, vec3 direction) {
    vec3 ab, ao;
    glm_vec3_sub(points->points[1].minkowski, points->points[0].minkowski, ab);
    glm_vec3_negate_to(points->points[0].minkowski, ao);

    if (glm_dot(ab, ao) > 0.0f) {
        glm_cross(ab, ao, direction);
        glm_cross(direction, ab, direction);
    } else {
        points->n_points = 0;
        glm_vec3_copy(ao, direction);
    }

    return false;
}

ELC_INLINE bool checkTriangleWithOrigin(Simplex* points, vec3 direction) {
    vec3 ab, ac, ao, abc, abc_cross_ac, ab_cross_abc;
    glm_vec3_sub(points->points[1].minkowski, points->points[0].minkowski, ab);
    glm_vec3_sub(points->points[2].minkowski, points->points[0].minkowski, ac);
    glm_vec3_negate_to(points->points[0].minkowski, ao);
    glm_cross(ab, ac, abc);
    glm_cross(abc, ac, abc_cross_ac);
    glm_cross(ab, abc, ab_cross_abc);

    if (glm_dot(abc_cross_ac, ao) > 0.0f) {
        if (glm_dot(ac, ao) > 0.0f) {
            *points = (Simplex){.points = {points->points[0], points->points[2]}, .n_points = 1};
            glm_cross(ac, ao, direction);
            glm_cross(direction, ac, direction);
        } else {
            points->n_points = 1;
            return checkLineWithOrigin(points, direction);
        }
    } else if (glm_dot(ab_cross_abc, ao) > 0.0f) {
        points->n_points = 1;
        return checkLineWithOrigin(points, direction);
    } else if (glm_dot(abc, ao) > 0.0f)
        glm_vec3_copy(abc, direction);
    else {
        *points = (Simplex){.points = {points->points[0], points->points[2], points->points[1]}, .n_points = 2};
        glm_vec3_negate_to(abc, direction);
    }

    return false;
}

ELC_INLINE bool checkTetrahedronWithOrigin(Simplex* points, vec3 direction) {
    vec3 ab, ac, ad, ao, abc, acd, adb;
    glm_vec3_sub(points->points[1].minkowski, points->points[0].minkowski, ab);
    glm_vec3_sub(points->points[2].minkowski, points->points[0].minkowski, ac);
    glm_vec3_sub(points->points[3].minkowski, points->points[0].minkowski, ad);
    glm_vec3_negate_to(points->points[0].minkowski, ao);
    glm_cross(ab, ac, abc);
    glm_cross(ac, ad, acd);
    glm_cross(ad, ab, adb);

    if (glm_dot(abc, ao) > 0.0f) {
        points->n_points = 2;
        return checkTriangleWithOrigin(points, direction);
    }

    if (glm_dot(acd, ao) > 0.0f) {
        *points = (Simplex){.points = {points->points[0], points->points[2], points->points[3]}, .n_points = 2};
        return checkTriangleWithOrigin(points, direction);
    }

    if (glm_dot(adb, ao) > 0.0f) {
        *points = (Simplex){.points = {points->points[0], points->points[3], points->points[1]}, .n_points = 2};
        return checkTriangleWithOrigin(points, direction);
    }

    return true;
}

ELC_INLINE bool nextSimplex(Simplex* points, vec3 direction) {
    switch (points->n_points) {
        case 1: return checkLineWithOrigin(points, direction);
        case 2: return checkTriangleWithOrigin(points, direction);
        case 3: return checkTetrahedronWithOrigin(points, direction);
    }

    return false;
}

ELC_INLINE void getFaceNormal(vec3 point_a, vec3 point_b, vec3 point_c, vec4 result) {
    vec3 ab, ac, normalized_result;
    glm_vec3_sub(point_b, point_a, ab);
    glm_vec3_sub(point_c, point_a, ac);

    glm_cross(ab, ac, result);
    glm_normalize_to(result, normalized_result);
    result[3] = glm_dot(normalized_result, point_a);

    if (result[3] < 0.0f) {
        glm_vec3_negate(result);
        result[3] = -result[3];
    }
}

ELC_INLINE uint64_t getFaceNormals(SupportPoint* polytope, const Face* faces, uint64_t n_faces, vec4s* normals) {
    uint64_t min_triangle = 0;
    float min_distance = FLT_MAX;

    for (uint64_t i = 0; i < n_faces; i++) {
        getFaceNormal(polytope[faces[i].a].minkowski, polytope[faces[i].b].minkowski, polytope[faces[i].c].minkowski, normals[i].raw);

        if (fabsf(glm_vec3_norm2(normals[i].raw)) > 0.0f && normals[i].raw[3] < min_distance) {
            min_distance = normals[i].raw[3];
            min_triangle = i;
        }

        glm_normalize(normals[i].raw);
    }

    return min_triangle;
}

ELC_INLINE void addIfUniqueEdge(Edge* edges, uint64_t* n_edges, uint64_t face_a, uint64_t face_b) {
    for (uint64_t i = 0; i < *n_edges; i++) {
        if (edges[i].a == face_b && edges[i].b == face_a) {
            for (uint64_t j = i; j < *n_edges - 1; j++) edges[j] = edges[j + 1];
            (*n_edges)--;

            return;
        }
    }

    edges[*n_edges] = (Edge){face_a, face_b};
    (*n_edges)++;
}

ELC_INLINE void triangleBarycentricOfOrigin(vec3 vertex_a, vec3 vertex_b, vec3 vertex_c, vec3 result) {
    vec3 a, b, c, normal, a_cross_c, c_cross_b;
    glm_vec3_sub(vertex_b, vertex_a, a);
    glm_vec3_sub(vertex_c, vertex_a, b);
    glm_vec3_negate_to(vertex_a, c);
    glm_vec3_adds(c, 0.001f, c);
    glm_cross(a, b, normal);
    glm_cross(a, c, a_cross_c);
    glm_cross(c, b, c_cross_b);

    float normal_length_squared = glm_vec3_norm2(normal);
    result[2] = glm_dot(a_cross_c, normal) / normal_length_squared;
    result[1] = glm_dot(c_cross_b, normal) / normal_length_squared;
    result[0] = 1.0f - result[2] - result[1];
}

ELC_INLINE void lineBarycentricOfOrigin(vec3 vertex_a, vec3 vertex_b, vec2 result) {
    vec3 ab;
    glm_vec3_sub(vertex_b, vertex_a, ab);
    result[1] = -glm_dot(vertex_a, ab) / fabsf(glm_vec3_norm2(ab));
    result[0] = 1.0f - result[1];
}

ELC_INLINE void interpolatePositionOnTriangle(vec3 vertex_a, vec3 vertex_b, vec3 vertex_c, vec3 uvw, vec3 result) {
    glm_vec3_scale(vertex_a, uvw[0], result);
    glm_vec3_muladds(vertex_b, uvw[1], result);
    glm_vec3_muladds(vertex_c, uvw[2], result);
}

ELC_INLINE void interpolatePositionOnLine(vec3 vertex_a, vec3 vertex_b, vec2 uv, vec3 result) {
    glm_vec3_scale(vertex_a, uv[0], result);
    glm_vec3_muladds(vertex_b, uv[1], result);
}

ELC_INLINE void completeCollisionResult(Particle* body_a, Particle* body_b, CollisionResult* result) {
    glm_vec3_sub(result->contact_b, result->contact_a, result->normal);
    glm_vec3_normalize(result->normal);

    versor ibra;
    glm_quat_inv(body_a->rotation, ibra);
    glm_vec3_negate_to(result->normal, result->local_normal);
    glm_quat_rotatev(ibra, result->local_normal, result->local_normal);

    worldToBodySpace(body_a, result->contact_a, result->local_a);
    worldToBodySpace(body_b, result->contact_b, result->local_b);

    result->penetration = -glm_vec3_distance(result->contact_a, result->contact_b);

    result->body_a = body_a;
    result->body_b = body_b;
}

ELC_INLINE void collisionResultFromFace(SupportPoint point_a, SupportPoint point_b, SupportPoint point_c, CollisionResult* result) {
    vec3 uvw;
    triangleBarycentricOfOrigin(point_a.minkowski, point_b.minkowski, point_c.minkowski, uvw);
    interpolatePositionOnTriangle(point_a.original_a, point_b.original_a, point_c.original_a, uvw, result->contact_a);
    interpolatePositionOnTriangle(point_a.original_b, point_b.original_b, point_c.original_b, uvw, result->contact_b);
}

ELC_INLINE void collisionResultFromEdge(SupportPoint point_a, SupportPoint point_b, CollisionResult* result) {
    vec2 uv;
    lineBarycentricOfOrigin(point_a.minkowski, point_b.minkowski, uv);
    interpolatePositionOnLine(point_a.original_a, point_b.original_a, uv, result->contact_a);
    interpolatePositionOnLine(point_a.original_b, point_b.original_b, uv, result->contact_b);
}

ELC_INLINE void collisionResultFromSimplex(Simplex* simplex, CollisionResult* result) {
    switch (simplex->n_points) {
        case 0:
            glm_vec3_copy(simplex->points[0].original_a, result->contact_a);
            glm_vec3_copy(simplex->points[0].original_b, result->contact_b);
            break;
        case 1:
            collisionResultFromEdge(simplex->points[0], simplex->points[1], result);
            break;
        case 2:
            collisionResultFromFace(simplex->points[0], simplex->points[1], simplex->points[2], result);
            break;
        case 3:
            break;
        default:
            break;
    }
}

ELC_INLINE void expandingPolytopeAlgorithm(Simplex* simplex, Particle* body_a, CollisionMesh mesh_a, Particle* body_b, CollisionMesh mesh_b, CollisionResult* result) {
    #define MAX_VERTICES (8 + 8 + 4)
    #define MAX_FACES (2 * (8 + 8))
    #define MAX_EDGES (MAX_VERTICES + MAX_FACES - 2)

    SupportPoint polytope[MAX_VERTICES];
    uint64_t n_polytope = 4;
    memcpy(polytope, simplex->points, sizeof(SupportPoint) * 4);

    Face faces[MAX_FACES] = {
        {0, 1, 2},
        {0, 3, 1},
        {0, 2, 3},
        {1, 3, 2},
    };
    uint64_t n_faces = 4;

    vec4s normals[MAX_FACES];
    uint64_t n_normals = n_faces;
    uint64_t min_face = getFaceNormals(polytope, faces, n_faces, normals);

    vec3 min_normal;
    float min_distance = FLT_MAX;

    while (min_distance == FLT_MAX) {
        glm_vec3_copy(normals[min_face].raw, min_normal);
        min_distance = normals[min_face].raw[3];

        SupportPoint support = calculateSupportPoint(body_a, mesh_a, body_b, mesh_b, min_normal);
        float s_distance = glm_dot(min_normal, support.minkowski);

        if (fabsf(s_distance - min_distance) > 0.01f) {
            min_distance = FLT_MAX;

            Edge unique_edges[MAX_EDGES];
            uint64_t n_unique_edges = 0;

            for (uint64_t i = 0; i < n_normals; i++) {
                vec3 support_minus_vertex;
                glm_vec3_sub(support.minkowski, polytope[faces[i].a].minkowski, support_minus_vertex);

                if (glm_dot(normals[i].raw, support_minus_vertex) > 0.001f) {
                    addIfUniqueEdge(unique_edges, &n_unique_edges, faces[i].a, faces[i].b);
                    addIfUniqueEdge(unique_edges, &n_unique_edges, faces[i].b, faces[i].c);
                    addIfUniqueEdge(unique_edges, &n_unique_edges, faces[i].c, faces[i].a);

                    faces[i] = faces[n_faces - 1];
                    n_faces--;

                    normals[i] = normals[n_normals - 1];
                    n_normals--;

                    i--;
                }
            }

            for (uint64_t i = 0; i < n_unique_edges; i++)
                faces[n_faces + i] = (Face){unique_edges[i].a, unique_edges[i].b, n_polytope};

            polytope[n_polytope++] = support;
            uint64_t new_min_face = getFaceNormals(polytope, &faces[n_faces], n_unique_edges, &normals[n_faces]);

            float old_min_distance = FLT_MAX;
            for (uint64_t i = 0; i < n_faces; i++) {
                if (normals[i].raw[3] < old_min_distance) {
                    old_min_distance = normals[i].raw[3];
                    min_face = i;
                }
            }

            if (normals[new_min_face + n_faces].raw[3] < old_min_distance) min_face = new_min_face + n_faces;
            n_faces += n_unique_edges;
            n_normals += n_unique_edges;
        }
    }

    vec3 a, b, c, normal, a_cross_c, c_cross_b;
    glm_vec3_sub(polytope[faces[min_face].b].minkowski, polytope[faces[min_face].a].minkowski, a);
    glm_vec3_sub(polytope[faces[min_face].c].minkowski, polytope[faces[min_face].a].minkowski, b);
    glm_vec3_negate_to(polytope[faces[min_face].a].minkowski, c);
    glm_cross(a, b, normal);
    glm_cross(a, c, a_cross_c);
    glm_cross(c, b, c_cross_b);

    float normal_length_squared = glm_vec3_norm2(normal);
    float w = glm_dot(a_cross_c, normal) / normal_length_squared;
    float v = glm_dot(c_cross_b, normal) / normal_length_squared;
    float u = 1.0f - w - v;

    *result = (CollisionResult){};

    glm_vec3_mul(polytope[faces[min_face].a].original_a, (vec3){u, u, u}, result->contact_a);
    glm_vec3_muladd(polytope[faces[min_face].b].original_a, (vec3){v, v, v}, result->contact_a);
    glm_vec3_muladd(polytope[faces[min_face].c].original_a, (vec3){w, w, w}, result->contact_a);

    glm_vec3_mul(polytope[faces[min_face].a].original_b, (vec3){u, u, u}, result->contact_b);
    glm_vec3_muladd(polytope[faces[min_face].b].original_b, (vec3){v, v, v}, result->contact_b);
    glm_vec3_muladd(polytope[faces[min_face].c].original_b, (vec3){w, w, w}, result->contact_b);

    glm_vec3_sub(result->contact_b, result->contact_a, result->normal);
    glm_vec3_normalize(result->normal);

    versor ibra;
    glm_quat_inv(body_a->rotation, ibra);
    glm_vec3_negate_to(result->normal, result->local_normal);
    glm_quat_rotatev(ibra, result->local_normal, result->local_normal);

    worldToBodySpace(body_a, result->contact_a, result->local_a);
    worldToBodySpace(body_b, result->contact_b, result->local_b);

    result->penetration = -glm_vec3_distance(result->contact_a, result->contact_b);

    result->body_a = body_a;
    result->body_b = body_b; // TODO: make sure reporting a collision without checking if penetration or normal is zero isnt causing problems
}

ELC_INLINE bool simplexContainsSupport(Simplex* simplex, SupportPoint* support) {
    for (u8 i = 0; i <= simplex->n_points; i++) if (glm_vec3_eqv_eps(simplex->points[i].original_a, support->original_a) || glm_vec3_eqv_eps(simplex->points[i].original_b, support->original_b)) return true;
    return false;
}

bool gilbertJohnsonKeerthi(Particle* body_a, CollisionMesh mesh_a, Particle* body_b, CollisionMesh mesh_b, CollisionResult* result) {
    vec3 direction = {1.0f, 1.0f, 1.0f};
    glm_normalize(direction);

    Simplex points = {0};
    points.points[0] = calculateSupportPoint(body_a, mesh_a, body_b, mesh_b, direction);

    glm_vec3_negate_to(points.points[0].minkowski, direction);

    bool not_colliding = false;

    while (true) {
        SupportPoint support = calculateSupportPoint(body_a, mesh_a, body_b, mesh_b, direction);

        if (glm_dot(support.minkowski, direction) <= 0.0f) not_colliding = true;
        if (not_colliding && simplexContainsSupport(&points, &support)) break;

        points.n_points++;
        points.points[3] = points.points[2];
        points.points[2] = points.points[1];
        points.points[1] = points.points[0];
        points.points[0] = support;

        if (nextSimplex(&points, direction)) { // check if simplex overlaps origin
            expandingPolytopeAlgorithm(&points, body_a, mesh_a, body_b, mesh_b, result);
            result->penetration = result->penetration;
            return true;
        }
    }

    collisionResultFromSimplex(&points, result); // TODO: make sure the bug this function seemed to have when used with EPA doesnt affect this as well
    completeCollisionResult(body_a, body_b, result);
    result->penetration = -result->penetration;
    return false;
}

ELC_INLINE float fastestLinearSpeed(Particle* body, CollisionMesh mesh, vec3 direction) {
    float max_speed = 0.0f;
    for (u64 i = 0; i < mesh.n_points; i++) {
        vec3 point, linear_velocity;
        glm_vec3_cross(body->omega, point, linear_velocity);
        float speed = glm_vec3_dot(direction, linear_velocity);
        if (speed > max_speed) max_speed = speed;
    }
    return max_speed;
}

bool intersectConservativeAdvance(Particle* body_a, CollisionMesh mesh_a, Particle* body_b, CollisionMesh mesh_b, float dt, CollisionResult* result) {
    float toi = 0.0f;
    u8 iterations = 0;

    while (dt > 0.0f) {
        bool colliding = gilbertJohnsonKeerthi(body_a, mesh_a, body_b, mesh_b, result);

        if (colliding) {
            result->time_of_impact = toi;
            applyParticleVelocity(body_a, -toi);
            applyParticleVelocity(body_b, -toi);
            return true;
        }

        iterations++;
        if (iterations > 10) break;

        vec3 ab, relative_velocity;
        glm_vec3_sub(result->contact_b, result->contact_a, ab);
        glm_vec3_normalize(ab);
        glm_vec3_sub(body_a->velocity, body_b->velocity, relative_velocity);
        float ortho_speed = glm_vec3_dot(relative_velocity, ab);
        float angular_speed_a = fastestLinearSpeed(body_a, mesh_a, ab);
        glm_vec3_negate(ab);
        float angular_speed_b = fastestLinearSpeed(body_b, mesh_b, ab);
        ortho_speed += angular_speed_a + angular_speed_b;
        if (ortho_speed <= 0.0f) break;
        float time_to_go = result->penetration / ortho_speed;
        if (time_to_go > dt) break;

        dt -= time_to_go;
        toi += time_to_go;
        applyParticleVelocity(body_a, time_to_go);
        applyParticleVelocity(body_b, time_to_go);
    }

    applyParticleVelocity(body_a, -toi);
    applyParticleVelocity(body_b, -toi);
    return false;
}

void resolveCollision(CollisionResult* collision) {
    mat3 inertia_a, inertia_b;
    inertiaTensorWorld(*collision->body_a, inertia_a);
    inertiaTensorWorld(*collision->body_b, inertia_b);

    float elasitcity = 0.5f, im_a = collision->body_a->mass, im_b = collision->body_b->mass;

    vec3 ai_a, ai_b, afv, v_a, v_b, ab, impulse;
    glm_vec3_cross(collision->body_a->omega, collision->local_a, v_a);
    glm_vec3_add(v_a, collision->body_a->velocity, v_a);
    glm_vec3_cross(collision->body_b->omega, collision->local_b, v_b);
    glm_vec3_add(v_b, collision->body_b->velocity, v_b);
    glm_vec3_cross(collision->local_a, collision->normal, ai_a);
    glm_mat3_mulv(inertia_a, ai_a, ai_a);
    glm_vec3_cross(ai_a, collision->local_a, ai_a);
    glm_vec3_cross(collision->local_b, collision->normal, ai_b);
    glm_mat3_mulv(inertia_b, ai_b, ai_b);
    glm_vec3_cross(ai_b, collision->local_b, ai_b);
    glm_vec3_add(ai_a, ai_b, afv);
    glm_vec3_sub(v_a, v_b, ab);

    float af = glm_vec3_dot(afv, collision->normal);
    float imps = (1.0f + elasitcity) * glm_dot(ab, collision->normal) / (im_a + im_b + af);
    glm_vec3_scale(collision->normal, imps, impulse);

    applyGeneralImpulse(collision->body_b, impulse, collision->local_b);
    glm_vec3_negate(impulse);
    applyGeneralImpulse(collision->body_a, impulse, collision->local_a);
}
