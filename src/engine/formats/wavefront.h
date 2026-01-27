#ifndef ENGINE_FORMATS_WAVEFRONT_H
#define ENGINE_FORMATS_WAVEFRONT_H

#include "../graphics/device.h"
#include "../graphics/model.h"

typedef struct Index {
    u32 position;
    u32 texture;
    u32 normal;
} Index;

typedef struct Face {
    Index indices[4];
} Face;

typedef struct HostMesh { // TODO: move this to a seperate mesh utilities file
    VulkanVertex* vertices;
    u32* indices;
    u32 n_vertices, n_indices;
} HostMesh;

void parseWavefrontFaces(char* data, size_t n_data, vec3s* positions, u32* n_positions, vec3s* normals, u32* n_normals, vec2s* uvs, u32* n_uvs, Face* faces, u32* n_faces);
void buildWavefrontMesh(vec3s* positions, u32 n_positions, vec3s* normals, u32 n_normals, vec2s* uvs, u32 n_uvs, Face* faces, u32 n_faces, HostMesh* mesh);
HostMesh parseWavefront(char* data, size_t n_data);
void destroyHostMesh(HostMesh mesh);
Model hostMeshToModel(Device device, HostMesh mesh);
HostMesh loadWavefront(const char* path);
Model loadWavefrontModel(Device device, const char* path);

#endif
