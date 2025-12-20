#include "wavefront.h"

#include "device.h"
#include "model.h"
#include <cglm/io.h>
#include <cglm/vec2-ext.h>
#include <cglm/vec3-ext.h>
#include <elc/core.h>
#include <stdint.h>
#include <stdlib.h>
#include <string.h>

ELC_INLINE bool isWhiteSpace(char c) {
    return c == '#' || c == '\n' || c == ' ' || c == '\t';
}

ELC_INLINE void skipWhiteSpace(char** data, size_t* n_data) {
    while (isWhiteSpace(**data) && *n_data > 0) {
        if (**data == '#') while (**data != '\n') (*data)++, (*n_data)--;
        (*data)++, (*n_data)--;
    }
}

ELC_INLINE float parseFloatingPoint(char** data, size_t* n_data) {
    size_t old_data = (size_t)*data;
    float result = strtof(*data, data);
    (*n_data) -= (size_t)(*data) - old_data;
    return result;
}

ELC_INLINE u64 parseUnsignedLong(char** data, size_t* n_data) {
    size_t old_data = (size_t)*data;
    u64 result = strtoull(*data, data, 10);
    (*n_data) -= (size_t)(*data) - old_data;
    return result;
}

ELC_INLINE bool expectCharacter(char** data, size_t* n_data, char c) {
    bool result = **data != c;
    (*data)++, (*n_data)--;
    return result;
}

ELC_INLINE bool expectWhiteSpace(char** data, size_t* n_data) {
    bool result = !isWhiteSpace(**data);
    (*data)++, (*n_data)--;
    return result;
}

ELC_INLINE bool isNumber(char c) {
    return c == '0' || c == '1' || c == '2' || c == '3' || c == '4' || c == '5' || c == '6' || c == '7' || c == '8' || c == '9';
}

ELC_INLINE char* nextNonWhitespace(char* data) {
    while (isWhiteSpace(*data)) data++;
    return data;
}

void parseWavefrontFaces(char* data, size_t n_data, vec3s* positions, u32* n_positions, vec3s* normals, u32* n_normals, vec2s* uvs, u32* n_uvs, Face* faces, u32* n_faces) {
    *n_positions = 0, *n_normals = 0, *n_uvs = 0, *n_faces = 0;
    while (n_data > 0) {
        skipWhiteSpace(&data, &n_data);
        if (n_data == 0) break;
        switch (*data) {
            case 'v':
                if (expectCharacter(&data, &n_data, 'v')) return;
                switch (*data) {
                    case ' ':
                        vec3s vertex;
                        for (u8 i = 0; i < 3 && n_data > 0; i++) {
                            if (expectWhiteSpace(&data, &n_data)) return;
                            skipWhiteSpace(&data, &n_data);
                            vertex.raw[i] = parseFloatingPoint(&data, &n_data);
                        }
                        if (positions != NULL) positions[*n_positions] = vertex;
                        (*n_positions)++;
                        break;
                    case 't':
                        if (expectCharacter(&data, &n_data, 't')) return;
                        vec2s texture;
                        for (u8 i = 0; i < 2 && n_data > 0; i++) {
                            if (expectWhiteSpace(&data, &n_data)) return;
                            skipWhiteSpace(&data, &n_data);
                            texture.raw[i] = parseFloatingPoint(&data, &n_data);
                        }
                        if (uvs != NULL) uvs[*n_uvs] = texture;
                        (*n_uvs)++;
                        break;
                    case 'n':
                        if (expectCharacter(&data, &n_data, 'n')) return;
                        vec3s normal;
                        for (u8 i = 0; i < 3 && n_data > 0; i++) {
                            if (expectWhiteSpace(&data, &n_data)) return;
                            skipWhiteSpace(&data, &n_data);
                            normal.raw[i] = parseFloatingPoint(&data, &n_data);
                        }
                        if (normals != NULL) normals[*n_normals] = normal;
                        (*n_normals)++;
                        break;
                }
                break;
            case 'f':
                if (expectCharacter(&data, &n_data, 'f')) return;
                Face face = {0};
                for (u8 i = 0; i < 4; i++) {
                    if (i == 3 && !(isWhiteSpace(*data) && isNumber(*nextNonWhitespace(data)))) break;
                    if (expectWhiteSpace(&data, &n_data)) return;
                    skipWhiteSpace(&data, &n_data);
                    face.indices[i].position = parseUnsignedLong(&data, &n_data);
                    if (!isWhiteSpace(*data)) {
                        if (expectCharacter(&data, &n_data, '/')) return;
                        if (*data == '/') face.indices[i].texture = 0;
                        else face.indices[i].texture = parseUnsignedLong(&data, &n_data);
                        if (expectCharacter(&data, &n_data, '/')) return;
                        face.indices[i].normal = parseUnsignedLong(&data, &n_data);
                    }
                }
                if (faces != NULL) faces[*n_faces] = face;
                (*n_faces)++;
                break;
            default: break;
        }
        data++, n_data--;
    }
}

ELC_INLINE bool isVertexEqual(VulkanVertex a, VulkanVertex b) {
    return glm_vec3_eqv_eps(a.position, b.position) && glm_vec3_eqv_eps(a.normal, b.normal) && glm_vec2_eqv_eps(a.uv, b.uv);
}

ELC_INLINE u32 findDuplicateVertex(HostMesh mesh, VulkanVertex vertex) {
    if (mesh.n_vertices == 0) return UINT32_MAX;
    for (u32 i = mesh.n_vertices - 1; i != 0; i--) if (isVertexEqual(mesh.vertices[i], vertex)) return i;
    return UINT32_MAX;
}

ELC_INLINE void addMeshVertex(HostMesh* mesh, VulkanVertex vertex) {
    vertex.position[1] *= -1.0f;
    if (mesh->vertices != NULL) {
        u32 duplicate = findDuplicateVertex(*mesh, vertex);
        if (duplicate == UINT32_MAX) {
            mesh->vertices[mesh->n_vertices] = vertex;
            mesh->indices[mesh->n_indices] = mesh->n_vertices++;
        } else mesh->indices[mesh->n_indices] = duplicate;
    } else mesh->n_vertices++;
    mesh->n_indices++;
}

ELC_INLINE void addMeshTriangle(HostMesh* mesh, VulkanVertex v_a, VulkanVertex v_b, VulkanVertex v_c){
    addMeshVertex(mesh, v_a), addMeshVertex(mesh, v_b), addMeshVertex(mesh, v_c);
}

ELC_INLINE void addMeshQuad(HostMesh* mesh, VulkanVertex v_a, VulkanVertex v_b, VulkanVertex v_c, VulkanVertex v_d) {
    addMeshTriangle(mesh, v_a, v_b, v_c), addMeshTriangle(mesh, v_c, v_d, v_a);
}

ELC_INLINE VulkanVertex indexToVertex(vec3s* positions, u32 n_positions, vec3s* normals, u32 n_normals, vec2s* uvs, u32 n_uvs, Index index) {
    return (VulkanVertex){
        .position = VEC3_USE((index.position == 0) ? (vec3){NAN, NAN, NAN} : positions[index.position - 1].raw),
        .normal = VEC3_USE((index.normal == 0) ? (vec3){NAN, NAN, NAN} : normals[index.normal - 1].raw),
        .uv = VEC2_USE((index.texture == 0) ? (vec2){NAN, NAN} : uvs[index.texture - 1].raw),
    };
}

ELC_INLINE bool isFaceQuad(Face face) {
    return face.indices[3].normal != 0 || face.indices[3].position != 0 || face.indices[3].texture != 0;
}

void buildWavefrontMesh(vec3s* positions, u32 n_positions, vec3s* normals, u32 n_normals, vec2s* uvs, u32 n_uvs, Face* faces, u32 n_faces, HostMesh* mesh) {
    mesh->n_vertices = 0, mesh->n_indices = 0;
    for (u32 i = 0; i < n_faces; i++) {
        if (isFaceQuad(faces[i])) {
            VulkanVertex v[4];
            for (u8 j = 0; j < 4; j++) v[j] = indexToVertex(positions, n_positions, normals, n_normals, uvs, n_uvs, faces[i].indices[j]);
            addMeshQuad(mesh, v[0], v[1], v[2], v[3]);
        } else {
            VulkanVertex v[3];
            for (u8 j = 0; j < 3; j++) v[j] = indexToVertex(positions, n_positions, normals, n_normals, uvs, n_uvs, faces[i].indices[j]);
            addMeshTriangle(mesh, v[0], v[1], v[2]);
        }
    }
}

HostMesh parseWavefront(char* data, size_t n_data) {
    u32 n_positions, n_normals, n_uvs, n_faces;
    parseWavefrontFaces(data, n_data, NULL, &n_positions, NULL, &n_normals, NULL, &n_uvs, NULL, &n_faces);
    vec3s* positions = malloc(n_positions * sizeof(vec3s)), *normals = malloc(n_normals * sizeof(vec3s));
    vec2s* uvs = malloc(n_uvs * sizeof(vec2s));
    Face* faces = malloc(n_faces * sizeof(Face));
    parseWavefrontFaces(data, n_data, positions, &n_positions, normals, &n_normals, uvs, &n_uvs, faces, &n_faces);

    HostMesh mesh = {0};
    buildWavefrontMesh(positions, n_positions, normals, n_normals, uvs, n_uvs, faces, n_faces, &mesh);
    mesh.vertices = malloc(mesh.n_vertices * sizeof(VulkanVertex)), mesh.indices = malloc(mesh.n_indices * sizeof(u32));
    buildWavefrontMesh(positions, n_positions, normals, n_normals, uvs, n_uvs, faces, n_faces, &mesh);

    free(positions), free(normals), free(uvs), free(faces);
    return mesh;
}

Model modelFromWavefront(Device device, char *data, size_t n_data) {
    HostMesh mesh = parseWavefront(data, n_data);
    void* vertex_mapped, *index_mapped;
    Model model = createModel(device, sizeof(VulkanVertex), mesh.n_vertices, mesh.n_indices, &vertex_mapped, &index_mapped);
    memcpy(vertex_mapped, mesh.vertices, sizeof(VulkanVertex) * mesh.n_vertices);
    memcpy(index_mapped, mesh.indices, sizeof(u32) * mesh.n_indices);
    finishModelUpload(device, &model, sizeof(VulkanVertex));
    free(mesh.indices), free(mesh.vertices);
    return model;
}

Model loadWavefront(Device device, const char* path) {
    MappedFile mapped = memoryMapFile(path);
    Model model = modelFromWavefront(device, mapped.memory, mapped.size);
    memoryUnmapFile(mapped);
    return model;
}
