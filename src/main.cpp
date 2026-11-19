#include "render_worker.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <vector>
#include <tbb/global_control.h>
#include <GLFW/glfw3.h>

namespace {

struct RenderControls {
    Camera* camera;
    RenderWorker* renderer;
    bool dragging = false;
    bool hasLastCursorPosition = false;
    double lastX = 0.0;
    double lastY = 0.0;
};

RenderControls* getControls(GLFWwindow* window) {
    return static_cast<RenderControls*>(glfwGetWindowUserPointer(window));
}

void onCursorMove(GLFWwindow* window, double x, double y) {
    RenderControls* controls = getControls(window);
    if (!controls) {
        return;
    }
    if (!controls->hasLastCursorPosition) {
        controls->lastX = x;
        controls->lastY = y;
        controls->hasLastCursorPosition = true;
        return;
    }

    if (controls->dragging) {
        controls->camera->look(
            static_cast<float>(x - controls->lastX) * 0.0025f,
            static_cast<float>(controls->lastY - y) * 0.0025f
        );
        controls->renderer->request(*controls->camera);
    }
    controls->lastX = x;
    controls->lastY = y;
}

void onMouseButton(GLFWwindow* window, int button, int action, int) {
    if (button != GLFW_MOUSE_BUTTON_LEFT) {
        return;
    }
    RenderControls* controls = getControls(window);
    if (!controls) {
        return;
    }

    controls->dragging = action == GLFW_PRESS;
    glfwGetCursorPos(window, &controls->lastX, &controls->lastY);
    controls->hasLastCursorPosition = true;
}

void generateSphere(
    float radius,
    const Vec3f& center,
    int rings,
    int sectors,
    std::vector<Vec3f>& vertices,
    std::vector<Index3>& indices
) {
    constexpr float pi = 3.14159265359f;

    for (int ring = 0; ring <= rings; ++ring) {
        const float phi = static_cast<float>(ring) / rings * pi;
        for (int sector = 0; sector <= sectors; ++sector) {
            const float theta = static_cast<float>(sector) / sectors * 2.0f * pi;
            vertices.emplace_back(
                center.x + radius * std::cos(theta) * std::sin(phi),
                center.y + radius * std::cos(phi),
                center.z + radius * std::sin(theta) * std::sin(phi)
            );
        }
    }

    for (int ring = 0; ring < rings; ++ring) {
        for (int sector = 0; sector < sectors; ++sector) {
            const auto current = static_cast<unsigned int>(ring * (sectors + 1) + sector);
            const auto next = current + static_cast<unsigned int>(sectors + 1);
            indices.emplace_back(current, next, current + 1);
            indices.emplace_back(current + 1, next, next + 1);
        }
    }
}

} // namespace

int main() {
    constexpr int imageWidth = 320;
    constexpr int imageHeight = 240;
    tbb::global_control threadLimit(
        tbb::global_control::max_allowed_parallelism,
        2
    );

    Scene scene;

    Material sphereMaterial;
    sphereMaterial.albedo = Vec3f(0.2f, 0.5f, 0.9f);

    std::vector<Vec3f> vertices;
    std::vector<Index3> indices;
    generateSphere(1.5f, Vec3f(0.0f, 0.0f, 4.0f), 30, 30, vertices, indices);

    scene.addTriangleMesh(vertices, indices, sphereMaterial);

    Material groundMaterial;
    groundMaterial.albedo = Vec3f(0.72f, 0.72f, 0.72f);
    groundMaterial.checkerboard = true;
    const std::vector<Vec3f> groundVertices = {
        Vec3f(-100.0f, -1.6f, -100.0f),
        Vec3f(100.0f, -1.6f, -100.0f),
        Vec3f(100.0f, -1.6f, 100.0f),
        Vec3f(-100.0f, -1.6f, 100.0f)
    };
    const std::vector<Index3> groundIndices = {
        Index3(0, 2, 1),
        Index3(0, 3, 2)
    };
    scene.addTriangleMesh(groundVertices, groundIndices, groundMaterial);

    Material lightMaterial;
    lightMaterial.emission = Vec3f(4.0f, 3.8f, 3.5f);
    const std::vector<Vec3f> lightVertices = {
        Vec3f(-1.0f, 2.4f, 3.0f),
        Vec3f(1.0f, 2.4f, 3.0f),
        Vec3f(1.0f, 2.4f, 5.0f),
        Vec3f(-1.0f, 2.4f, 5.0f)
    };
    const std::vector<Index3> lightIndices = {
        Index3(0, 1, 2),
        Index3(0, 2, 3)
    };
    scene.addTriangleMesh(lightVertices, lightIndices, lightMaterial);
    scene.commit();

    if (!glfwInit()) {
        std::cerr << "Failed to initialize GLFW\n";
        return 1;
    }

    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 2);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 1);
    GLFWwindow* window = glfwCreateWindow(
        960,
        720,
        "Sphere Path Tracer | WASD: move | Space/C: up/down | Left-drag: look",
        nullptr,
        nullptr
    );
    if (!window) {
        std::cerr << "Failed to create a window\n";
        glfwTerminate();
        return 1;
    }

    glfwMakeContextCurrent(window);
    glfwSwapInterval(1);
    Camera camera(static_cast<float>(imageWidth) / imageHeight, Vec3f(0.0f, 0.0f, 0.0f));
    RenderWorker renderer(scene, camera, imageWidth, imageHeight);
    RenderControls controls{&camera, &renderer};
    glfwSetWindowUserPointer(window, &controls);
    glfwSetCursorPosCallback(window, onCursorMove);
    glfwSetMouseButtonCallback(window, onMouseButton);

    GLuint texture = 0;
    glGenTextures(1, &texture);
    glBindTexture(GL_TEXTURE_2D, texture);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
    glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
    glTexImage2D(
        GL_TEXTURE_2D,
        0,
        GL_RGBA,
        imageWidth,
        imageHeight,
        0,
        GL_RGBA,
        GL_UNSIGNED_BYTE,
        nullptr
    );

    glDisable(GL_DEPTH_TEST);
    glEnable(GL_TEXTURE_2D);

    double previousFrameTime = glfwGetTime();
    std::uint64_t imageVersion = 0;
    std::vector<std::uint8_t> latestImage;
    while (!glfwWindowShouldClose(window)) {
        glfwPollEvents();
        const double currentFrameTime = glfwGetTime();
        const float deltaTime = static_cast<float>(currentFrameTime - previousFrameTime);
        previousFrameTime = currentFrameTime;

        Vec3f movement(0.0f);
        if (glfwGetKey(window, GLFW_KEY_W) == GLFW_PRESS) movement.z += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_S) == GLFW_PRESS) movement.z -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_D) == GLFW_PRESS) movement.x += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_A) == GLFW_PRESS) movement.x -= 1.0f;
        if (glfwGetKey(window, GLFW_KEY_SPACE) == GLFW_PRESS) movement.y += 1.0f;
        if (glfwGetKey(window, GLFW_KEY_C) == GLFW_PRESS) movement.y -= 1.0f;

        if (length(movement) > 0.0f) {
            movement = normalize(movement) * (3.0f * deltaTime);
            camera.moveLocal(movement);
            renderer.request(camera);
        }

        if (renderer.copyLatestImage(latestImage, imageVersion)) {
            glBindTexture(GL_TEXTURE_2D, texture);
            glTexSubImage2D(
                GL_TEXTURE_2D,
                0,
                0,
                0,
                imageWidth,
                imageHeight,
                GL_RGBA,
                GL_UNSIGNED_BYTE,
                latestImage.data()
            );
        }

        int framebufferWidth;
        int framebufferHeight;
        glfwGetFramebufferSize(window, &framebufferWidth, &framebufferHeight);
        glViewport(0, 0, framebufferWidth, framebufferHeight);
        glClear(GL_COLOR_BUFFER_BIT);

        glBegin(GL_QUADS);
        glTexCoord2f(0.0f, 1.0f);
        glVertex2f(-1.0f, -1.0f);
        glTexCoord2f(1.0f, 1.0f);
        glVertex2f(1.0f, -1.0f);
        glTexCoord2f(1.0f, 0.0f);
        glVertex2f(1.0f, 1.0f);
        glTexCoord2f(0.0f, 0.0f);
        glVertex2f(-1.0f, 1.0f);
        glEnd();

        glfwSwapBuffers(window);
    }

    glDeleteTextures(1, &texture);
    glfwDestroyWindow(window);
    glfwTerminate();
}
