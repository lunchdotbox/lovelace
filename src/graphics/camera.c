#include "camera.h"

#include "window.h"

Camera createCamera() {
    Camera camera = {0};
    glm_perspective_rh_no(GLM_PI / 4.0f, 800.0f / 600.0f, 0.01f, 1000.0f, camera.projection);
    return camera;
}

void getCameraMatrix(Camera camera, mat4 dest) {
    glm_euler(camera.rotation, dest);
    glm_translate(dest, camera.position);
    glm_mat4_mul(camera.projection, dest, dest);
}

void updateCamera(Camera* camera, Window window) {
    double mouse_x, mouse_y;
    glfwGetCursorPos(window.window, &mouse_x, &mouse_y);

    camera->rotation[0] -= (mouse_y - camera->last_mouse_y) * 0.002;
    camera->rotation[1] += (mouse_x - camera->last_mouse_x) * 0.002;

    camera->rotation[0] = CLAMP(camera->rotation[0], -((GLM_PI / 2.0) - 0.01), (GLM_PI / 2.0) - 0.01);

    vec3 camera_movement = {};

    if (glfwGetKey(window.window, GLFW_KEY_S)) camera_movement[2] -= 1.0f;
    if (glfwGetKey(window.window, GLFW_KEY_W)) camera_movement[2] += 1.0f;
    if (glfwGetKey(window.window, GLFW_KEY_D)) camera_movement[0] -= 1.0f;
    if (glfwGetKey(window.window, GLFW_KEY_A)) camera_movement[0] += 1.0f;

    glm_normalize(camera_movement);
    glm_vec3_rotate(camera_movement, -camera->rotation[1], (vec3){0.0f, 1.0f, 0.0f});
    glm_vec3_scale(camera_movement, 0.001f, camera_movement);
    glm_vec3_add(camera->position, camera_movement, camera->position);

    if (glfwGetKey(window.window, GLFW_KEY_LEFT_SHIFT)) camera->position[1] -= 0.001f;
    if (glfwGetKey(window.window, GLFW_KEY_SPACE)) camera->position[1] += 0.001f;

    camera->last_mouse_x = mouse_x;
    camera->last_mouse_y = mouse_y;
}
