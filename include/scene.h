// include/scene.h
#pragma once
#include "math_types.h"
#include <embree3/rtcore.h>
#include <cstdint>
#include <optional>
#include <vector>
#include <memory>

struct Material {
    Vec3f emission{0, 0, 0};
    Vec3f albedo{0, 0, 0};
    bool checkerboard = false;
    bool isEmissive() const { return dot(emission, emission) > 0.0f; }
};

struct AreaLightSample {
    Vec3f position;
    Vec3f normal;
    Vec3f emission;
    float pdfArea = 0.0f;
};

class Scene {
public:
    Scene();
    ~Scene();
    
    void addTriangleMesh(const std::vector<Vec3f>& vertices, const std::vector<Index3>& indices, const Material& mat);
    void commit();
    Intersection intersect(const Ray& ray) const;
    const Material& getMaterial(int geomID) const;
    std::optional<AreaLightSample> sampleLight(float select, float u, float v) const;
    float lightPdfSolidAngle(const Vec3f& referencePoint, const Intersection& lightHit) const;

private:
    struct LightTriangle {
        Vec3f a;
        Vec3f b;
        Vec3f c;
        Vec3f normal;
        Vec3f emission;
        float cumulativeArea;
        unsigned int geomID;
        std::uint32_t primID;
    };

    RTCDevice device;
    RTCScene scene;
    std::vector<Material> materials;
    std::vector<LightTriangle> lights;
    float totalLightArea = 0.0f;
};