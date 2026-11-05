#pragma once
#include "math_types.h"

class Camera {
public:
    Camera(float aspect_ratio) : aspect(aspect_ratio) {}

    Ray generateRay(float u, float v) const { 
        // Mapowanie (u,v) z przestrzeni [0,1] do przestrzeni NDC [-1,1]
        float screen_x = (2.0f * u - 1.0f) * aspect;
        float screen_y = 1.0f - 2.0f * v; // Oś Y ekranu rośnie w dół, więc odwracamy
        
        Vec3f origin(0.0f, 0.0f, 0.0f);
        // Płaszczyzna obrazu odsunięta o Z=2.0
        Vec3f direction = glm::normalize(Vec3f(screen_x, screen_y, 2.0f));
        
        return Ray{origin, 0.001f, direction, 1000.0f}; 
    }
private:
    float aspect;
};