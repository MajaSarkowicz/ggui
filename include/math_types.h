#pragma once
#include <glm/glm.hpp>
#include <glm/gtc/constants.hpp>
#include <random>

using Vec3f = glm::vec3;
using Vec2f = glm::vec2;

struct Ray {
    Vec3f origin;
    float tnear = 0.001f;
    Vec3f direction;
    float tfar = 1e8f;
};

struct Intersection {
    bool hit = false;
    float t;
    Vec3f p;
    Vec3f n;
    int geomID = -1;
    int primID = -1;
};

inline Vec3f sampleCosineHemisphere(const Vec3f& normal, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    float r1 = dist(rng);
    float r2 = dist(rng);
    float z = std::sqrt(1.0f - r2);
    float phi = 2.0f * glm::pi<float>() * r1;
    float x = std::cos(phi) * std::sqrt(r2);
    float y = std::sin(phi) * std::sqrt(r2);
    
    Vec3f w = normal;
    Vec3f a = (std::abs(w.x) > 0.9f) ? Vec3f(0, 1, 0) : Vec3f(1, 0, 0);
    Vec3f v = glm::normalize(glm::cross(w, a));
    Vec3f u = glm::cross(w, v);
    
    return glm::normalize(u * x + v * y + w * z);
}