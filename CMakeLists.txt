cmake_minimum_required(VERSION 3.20)
project(PhosphorPathTracer VERSION 0.1.0 LANGUAGES CXX C)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)
set(embree_DIR "$ENV{HOME}/embree-3.13.5.x86_64.linux/lib/cmake/embree-3.13.5")

find_package(embree 3.0 REQUIRED)
find_package(TBB REQUIRED)
find_package(OpenGL REQUIRED)
find_package(glfw3 REQUIRED)

include(FetchContent)

# Pobieranie GLM (nagłówkowa)
FetchContent_Declare(glm GIT_REPOSITORY https://github.com/g-truc/glm.git GIT_TAG master)
FetchContent_MakeAvailable(glm)

# Pobieranie ImGui
FetchContent_Declare(imgui GIT_REPOSITORY https://github.com/ocornut/imgui.git GIT_TAG docking)
FetchContent_MakeAvailable(imgui)
set(IMGUI_DIR ${imgui_SOURCE_DIR})

# Dear ImGui does not define a CMake target, so build its core and backends here.
add_library(imgui STATIC
    ${IMGUI_DIR}/imgui.cpp
    ${IMGUI_DIR}/imgui_draw.cpp
    ${IMGUI_DIR}/imgui_tables.cpp
    ${IMGUI_DIR}/imgui_widgets.cpp
    ${IMGUI_DIR}/backends/imgui_impl_glfw.cpp
    ${IMGUI_DIR}/backends/imgui_impl_opengl3.cpp
)

target_include_directories(imgui PUBLIC
    ${IMGUI_DIR}
    ${IMGUI_DIR}/backends
)

target_link_libraries(imgui PUBLIC
    glfw
    OpenGL::GL
)

# Pobieranie tinyobjloader
FetchContent_Declare(
    tinyobjloader
    GIT_REPOSITORY https://github.com/tinyobjloader/tinyobjloader.git
    GIT_TAG release
)
FetchContent_MakeAvailable(tinyobjloader)

# Główna aplikacja w jednym targecie
add_executable(renderer_app 
    src/main.cpp
    src/scene.cpp
    src/camera.cpp
    src/glad.c # <--- DODANO GLAD
)

target_include_directories(renderer_app PUBLIC 
    include
)

target_link_libraries(renderer_app PRIVATE 
    embree 
    TBB::tbb 
    OpenGL::GL 
    glfw 
    glm::glm
    imgui
    tinyobjloader
)