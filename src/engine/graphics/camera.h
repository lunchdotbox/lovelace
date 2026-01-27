#ifndef ENGINE_GRAPHICS_CAMERA_H
#define ENGINE_GRAPHICS_CAMERA_H

#include <cglm/cglm.h>
#include <stdalign.h>
#include "window.h"

typedef struct CameraPushConstant { // TODO: move this struct somewhere else and rename it cause its not just the camera data
    mat4 model_matrix;
    u32 uniform_id;
    u32 texture_id;
} CameraPushConstant;

typedef struct Camera {
    double last_mouse_x, last_mouse_y;
    vec3 position, rotation;
    mat4 projection;
} Camera;

Camera createCamera();
void getCameraMatrix(Camera camera, mat4 dest);
void updateCamera(Camera* camera, Window window);

#endif
