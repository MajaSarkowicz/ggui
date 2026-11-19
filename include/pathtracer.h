
#pragma once

#include "scene.h"
#include "camera.h"

#include <tbb/parallel_for.h>
#include <vector>
#include <random>
#include <algorithm>
#include <cstdint>
#include <cmath>
#include <optional>
#include <stdexcept>

class PathTracer {
public:

    PathTracer(int w, int h)
        : width(w), height(h)
    {
        accumulationBuffer.resize(
            width * height,
            Vec3f(0.0f)
        );

        displayBuffer.resize(
            width * height * 4,
            0
        );
    }

    void renderPass(
        const Scene& scene,
        const Camera& camera,
        int maxDepth
    ) {
        const int currentSample = ++sampleCount;

        tbb::parallel_for(0, height, [&](int y) {

            for (int x = 0; x < width; ++x) {

                // Osobny generator dla każdego piksela
                unsigned int seed =
                    static_cast<unsigned int>(
                        (y * width + x) * 73856093u
                        + currentSample * 19349663u
                    );

                std::mt19937 rng(seed);

                float u = (x + generateFloat(rng))
                    / static_cast<float>(width);

                float v = (y + generateFloat(rng))
                    / static_cast<float>(height);

                Ray ray = camera.generateRay(u, v);

                Vec3f color = tracePath(
                    ray,
                    scene,
                    rng,
                    0,
                    maxDepth,
                    0.0f,
                    Vec3f(0.0f)
                );

                int idx = y * width + x;

                accumulationBuffer[idx] += color;

                Vec3f avg =
                    accumulationBuffer[idx]
                    / static_cast<float>(currentSample);

                // Gamma correction
                avg.x = std::pow(std::max(0.0f, avg.x), 1.0f / 2.2f);
                avg.y = std::pow(std::max(0.0f, avg.y), 1.0f / 2.2f);
                avg.z = std::pow(std::max(0.0f, avg.z), 1.0f / 2.2f);

                avg = clamp(avg, 0.0f, 1.0f);

                displayBuffer[idx * 4 + 0] =
                    static_cast<uint8_t>(avg.x * 255.0f);

                displayBuffer[idx * 4 + 1] =
                    static_cast<uint8_t>(avg.y * 255.0f);

                displayBuffer[idx * 4 + 2] =
                    static_cast<uint8_t>(avg.z * 255.0f);

                displayBuffer[idx * 4 + 3] = 255;
            }
        });
    }

    void reset() {

        std::fill(
            accumulationBuffer.begin(),
            accumulationBuffer.end(),
            Vec3f(0.0f)
        );

        std::fill(
            displayBuffer.begin(),
            displayBuffer.end(),
            0
        );

        sampleCount = 0;
    }

    const uint8_t* getDisplayData() const {
        return displayBuffer.data();
    }

    int getSampleCount() const {
        return sampleCount;
    }

    Vec3f getLinearPixel(int x, int y) const {
        if (x < 0 || x >= width || y < 0 || y >= height) {
            throw std::out_of_range("Pixel coordinates are outside the image");
        }
        if (sampleCount == 0) {
            throw std::logic_error("No samples have been rendered");
        }

        const int index = y * width + x;
        return accumulationBuffer.at(static_cast<std::size_t>(index))
            / static_cast<float>(sampleCount);
    }

private:

    int width;
    int height;
    int sampleCount = 0;

    std::vector<Vec3f> accumulationBuffer;
    std::vector<uint8_t> displayBuffer;

    float generateFloat(std::mt19937& rng) {

        return std::uniform_real_distribution<float>(
            0.0f, 1.0f
        )(rng);
    }

    static float powerHeuristic(float firstPdf, float secondPdf) {
        if (secondPdf <= 0.0f) {
            return 1.0f;
        }
        if (firstPdf <= 0.0f) {
            return 0.0f;
        }
        const float firstSquared = firstPdf * firstPdf;
        const float secondSquared = secondPdf * secondPdf;
        return firstSquared / (firstSquared + secondSquared);
    }

    // Losowanie kierunku na półkuli cosinusowej
    Vec3f sampleCosineHemisphere(
        const Vec3f& normal,
        std::mt19937& rng
    ) {

        const float PI = 3.14159265359f;

        float r1 = generateFloat(rng);
        float r2 = generateFloat(rng);

        float phi = 2.0f * PI * r1;

        float r = std::sqrt(r2);

        float x = r * std::cos(phi);
        float y = r * std::sin(phi);
        float z = std::sqrt(1.0f - r2);

        Vec3f helper =
            std::abs(normal.x) > 0.9f
            ? Vec3f(0, 1, 0)
            : Vec3f(1, 0, 0);

        Vec3f tangent = normalize(cross(helper, normal));

        Vec3f bitangent = cross(normal, tangent);

        return normalize(
            tangent * x +
            bitangent * y +
            normal * z
        );
    }

    Vec3f tracePath(
        const Ray& ray,
        const Scene& scene,
        std::mt19937& rng,
        int depth,
        int maxDepth,
        float previousBsdfPdf,
        const Vec3f& previousPoint
    ) {

        if (depth >= maxDepth) {
            return Vec3f(0.0f);
        }

        Intersection isect = scene.intersect(ray);

        if (!isect.hit) {
            const float sky = 0.5f * (ray.direction.y + 1.0f);
            return mix(
                Vec3f(0.85f, 0.9f, 1.0f),
                Vec3f(0.35f, 0.55f, 0.85f),
                sky
            );
        }

        const Material& mat =
            scene.getMaterial(isect.geomID);

        if (mat.isEmissive()) {
            const float lightPdf = depth == 0
                ? 0.0f
                : scene.lightPdfSolidAngle(previousPoint, isect);
            return mat.emission * powerHeuristic(previousBsdfPdf, lightPdf);
        }

        Vec3f albedo = mat.albedo;
        if (mat.checkerboard) {
            const int tileX = static_cast<int>(std::floor(isect.p.x / 2.0f));
            const int tileZ = static_cast<int>(std::floor(isect.p.z / 2.0f));
            if ((tileX + tileZ) % 2 != 0) {
                albedo *= 0.65f;
            }
        }

        constexpr float pi = 3.14159265359f;
        Vec3f directLighting(0.0f);
        const std::optional<AreaLightSample> light = scene.sampleLight(
            generateFloat(rng),
            generateFloat(rng),
            generateFloat(rng)
        );
        if (light) {
            const Vec3f toLight = light->position - isect.p;
            const float distanceSquared = dot(toLight, toLight);
            if (distanceSquared > 0.0f && light->pdfArea > 0.0f) {
                const float distance = std::sqrt(distanceSquared);
                const Vec3f lightDirection = toLight / distance;
                const float surfaceCosine = std::max(dot(isect.n, lightDirection), 0.0f);
                const float lightCosine = std::max(dot(light->normal, -lightDirection), 0.0f);
                if (surfaceCosine > 0.0f && lightCosine > 0.0f) {
                    constexpr float rayOffset = 0.001f;
                    const Vec3f shadowOrigin = isect.p + isect.n * rayOffset;
                    const Vec3f shadowToLight = light->position - shadowOrigin;
                    const float shadowDistance = length(shadowToLight);
                    if (shadowDistance > 2.0f * rayOffset) {
                        const Ray shadowRay{
                            shadowOrigin,
                            rayOffset,
                            shadowToLight / shadowDistance,
                            shadowDistance - rayOffset
                        };
                        if (!scene.intersect(shadowRay).hit) {
                            const float lightPdf = light->pdfArea
                                * distanceSquared / lightCosine;
                            const float bsdfPdf = surfaceCosine / pi;
                            const float weight = powerHeuristic(lightPdf, bsdfPdf);
                            const float scale = surfaceCosine * lightCosine * weight
                                / (pi * distanceSquared * light->pdfArea);
                            directLighting = albedo * light->emission * scale;
                        }
                    }
                }
            }
        }

        Ray scattered;

        scattered.origin =
            isect.p + isect.n * 0.001f;

        scattered.direction =
            sampleCosineHemisphere(isect.n, rng);

        const float bsdfPdf = std::max(dot(isect.n, scattered.direction), 0.0f) / pi;
        Vec3f incoming = tracePath(
            scattered,
            scene,
            rng,
            depth + 1,
            maxDepth,
            bsdfPdf,
            isect.p
        );

        return directLighting + albedo * incoming;
    }
};
