#ifndef ENGINE_PHYSICS_MESH_INERTIA_H
#define ENGINE_PHYSICS_MESH_INERTIA_H

#include "../formats/wavefront.h"
#include "particle.h"
#include <cglm/mat3.h>
#include <cglm/util.h>
#include <cglm/vec3.h>

void computeMeshInertia(HostMesh mesh, float density, mat3 inertia, vec3 com, float* m);
void shiftMeshBackwards(HostMesh mesh, vec3 com); // TODO: move this to a seperate mesh utilities file

#endif
