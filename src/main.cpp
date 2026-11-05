#include <iostream>
#include <vector>
#include <GLFW/glfw3.h>
#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "scene.h"
#include "pathtracer.h"
#include "camera.h"
#include "obj_loader.h"

int main() {
    if (!glfwInit()) return -1;
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 3);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 3);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    GLFWwindow* window = glfwCreateWindow(1280, 720, "Phosphor Path Tracer", nullptr, nullptr);
    if (!window) { glfwTerminate(); return -1; }
    glfwMakeContextCurrent(window);
    glfwSwapInterval(1); // VSync na UI

    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui_ImplGlfw_InitForOpenGL(window, true);
    ImGui_ImplOpenGL3_Init("#version 330");

    // Inicjalizacja silnika
    Scene scene;

    // 1. Materiały
    Material floorMat;
    floorMat.albedo = Vec3f(0.7f, 0.7f, 0.7f); // Jasnoszara podłoga
    floorMat.emission = Vec3f(0.0f);

    Material modelMat;
    modelMat.albedo = Vec3f(0.2f, 0.5f, 0.9f); // Niebieski materiał Lamberta
    modelMat.emission = Vec3f(0.0f);

    Material lightMat;
    lightMat.albedo = Vec3f(0.0f);
    lightMat.emission = Vec3f(15.0f, 15.0f, 15.0f); // Silne białe światło

    // 2. Geometria: Podłoga (Dwa trójkąty tworzące kwadrat)
    std::vector<Vec3f> floorVerts = {
        Vec3f(-10.0f, -2.0f, -10.0f), Vec3f(10.0f, -2.0f, -10.0f),
        Vec3f(10.0f, -2.0f,  10.0f), Vec3f(-10.0f, -2.0f,  10.0f)
    };
    std::vector<glm::uvec3> floorInds = { glm::uvec3(0, 1, 2), glm::uvec3(0, 2, 3) };
    scene.addTriangleMesh(floorVerts, floorInds, floorMat);

    // 3. Geometria: Sufitowe źródło światła
    std::vector<Vec3f> lightVerts = {
        Vec3f(-2.0f, 5.0f, 2.0f), Vec3f(2.0f, 5.0f, 2.0f),
        Vec3f(2.0f, 5.0f, 6.0f), Vec3f(-2.0f, 5.0f, 6.0f)
    };
    std::vector<glm::uvec3> lightInds = { glm::uvec3(0, 1, 2), glm::uvec3(0, 2, 3) };
    scene.addTriangleMesh(lightVerts, lightInds, lightMat);

    // 4. Geometria: Wczytanie modelu z pliku (umieszczony w Z=4.0 na wprost kamery)
    std::vector<Vec3f> objVerts;
    std::vector<glm::uvec3> objInds;
    // Pamiętaj o wrzuceniu pliku np. sphere.obj do katalogu uruchomieniowego!
    if (loadObj("sphere.obj", objVerts, objInds, Vec3f(0.0f, -0.5f, 4.0f), 1.5f)) {
        scene.addTriangleMesh(objVerts, objInds, modelMat);
    }

    scene.commit();
    return 0;
}