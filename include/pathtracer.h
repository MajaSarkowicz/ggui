#pragma once
#include "scene.h"
#include "camera.h"
#include <tbb/parallel_for.h>
#include <vector>

class PathTracer {
public:
    PathTracer(int w, int h) : width(w), height(h) {
        accumulationBuffer.resize(w * h, Vec3f(0.0f));
        displayBuffer.resize(w * h * 4, 0); // RGBA8
    }

    void renderPass(const Scene& scene, const Camera& camera, int maxDepth) {
        sampleCount++;
        tbb::parallel_for(0, height, [&](int y) {
            // Unikalny seed dla każdego piksela w oparciu o próbkę
            std::mt19937 rng(y * width + sampleCount * 91238); 
            
            for (int x = 0; x < width; ++x) {
                float u = (x + generateFloat(rng)) / width;
                float v = (y + generateFloat(rng)) / height;
                
                Vec3f color = tracePath(camera.generateRay(u, v), scene, rng, 0, maxDepth);
                
                int idx = y * width + x;
                accumulationBuffer[idx] += color;
                
                // Oblicz średnią i skonwertuj na 8-bit RGBA dla OpenGL (z prostym gamma 2.2)
                Vec3f avg = accumulationBuffer[idx] / (float)sampleCount;
                avg = glm::pow(avg, Vec3f(1.0f / 2.2f)); // Gamma correction
                avg = glm::clamp(avg, 0.0f, 1.0f);
                
                displayBuffer[idx * 4 + 0] = static_cast<uint8_t>(avg.x * 255.0f);
                displayBuffer[idx * 4 + 1] = static_cast<uint8_t>(avg.y * 255.0f);
                displayBuffer[idx * 4 + 2] = static_cast<uint8_t>(avg.z * 255.0f);
                displayBuffer[idx * 4 + 3] = 255;
            }
        });
    }

    void reset() {
        std::fill(accumulationBuffer.begin(), accumulationBuffer.end(), Vec3f(0.0f));
        sampleCount = 0;
    }

    const uint8_t* getDisplayData() const { return displayBuffer.data(); }
    int getSampleCount() const { return sampleCount; }

private:
    int width, height, sampleCount = 0;
    std::vector<Vec3f> accumulationBuffer;
    std::vector<uint8_t> displayBuffer;

    float generateFloat(std::mt19937& rng) {
        return std::uniform_real_distribution<float>(0.0f, 1.0f)(rng);
    }

    Vec3f tracePath(Ray ray, const Scene& scene, std::mt19937& rng, int depth, int maxDepth) {
        if (depth > maxDepth) return Vec3f(0.0f);

        Intersection isect = scene.intersect(ray);
        if (!isect.hit) return Vec3f(0.05f); // Szare tło

        const Material& mat = scene.getMaterial(isect.geomID);
        if (glm::dot(mat.emission, mat.emission) > 0) return mat.emission;

        Ray scattered;
        scattered.origin = isect.p + isect.n * 0.001f;
        scattered.direction = sampleCosineHemisphere(isect.n, rng);
        
        Vec3f incoming = tracePath(scattered, scene, rng, depth + 1, maxDepth);
        return mat.albedo * incoming; // GLM ułatwia mnożenie per-komponent
    }
};