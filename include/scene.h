// include/scene.h
#pragma once
#include "math_types.h"
#include <embree3/rtcore.h>
#include <vector>
#include <memory>

struct Material {
    Vec3f emission{0, 0, 0};
    Vec3f albedo{0, 0, 0};
    bool isEmissive() const { return glm::dot(emission, emission) > 0.0f; }
};

class Scene {
public:
    Scene();
    ~Scene();
    
    void addTriangleMesh(const std::vector<Vec3f>& vertices, const std::vector<glm::uvec3>& indices, const Material& mat);
    void commit();
    Intersection intersect(const Ray& ray) const;
    const Material& getMaterial(int geomID) const;

private:
    RTCDevice device;
    RTCScene scene;
    std::vector<Material> materials;
};