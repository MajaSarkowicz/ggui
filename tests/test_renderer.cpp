#include "pathtracer.h"

#include <cmath>
#include <cstdint>
#include <iostream>
#include <stdexcept>
#include <string>
#include <vector>

namespace {

void require(bool condition, const std::string& message) {
    if (!condition) {
        throw std::runtime_error(message);
    }
}

void addPlane(Scene& scene, float z, float halfExtent, const Material& material) {
    const std::vector<Vec3f> vertices = {
        Vec3f(-halfExtent, -halfExtent, z),
        Vec3f(halfExtent, -halfExtent, z),
        Vec3f(halfExtent, halfExtent, z),
        Vec3f(-halfExtent, halfExtent, z)
    };
    const std::vector<Index3> indices = {
        Index3(0, 2, 1),
        Index3(0, 3, 2)
    };
    scene.addTriangleMesh(vertices, indices, material);
}

void testEmissiveReferenceImage() {
    constexpr int width = 8;
    constexpr int height = 8;
    const Vec3f expectedEmission(0.25f, 0.5f, 1.0f);

    Scene scene;
    Material emitter;
    emitter.emission = expectedEmission;
    addPlane(scene, 4.0f, 10.0f, emitter);
    scene.commit();

    PathTracer tracer(width, height);
    const Camera camera(1.0f, Vec3f(0.0f));
    for (int sample = 0; sample < 4; ++sample) {
        tracer.renderPass(scene, camera, 1);
    }

    require(tracer.getSampleCount() == 4, "Reference render sample count is wrong");
    const std::uint8_t expectedDisplay[] = {135, 186, 255, 255};
    const std::uint8_t* image = tracer.getDisplayData();
    for (int pixel = 0; pixel < width * height; ++pixel) {
        const Vec3f linear = tracer.getLinearPixel(pixel % width, pixel / width);
        require(std::abs(linear.x - expectedEmission.x) < 1e-6f
                    && std::abs(linear.y - expectedEmission.y) < 1e-6f
                    && std::abs(linear.z - expectedEmission.z) < 1e-6f,
                "Reference image linear pixel differs from emitter radiance");
        for (int channel = 0; channel < 4; ++channel) {
            require(image[pixel * 4 + channel] == expectedDisplay[channel],
                    "Reference image display pixel differs from expected output");
        }
    }
}

void testLambertianConvergence() {
    constexpr int width = 8;
    constexpr int height = 8;
    constexpr int earlySampleCount = 64;
    constexpr int finalSampleCount = 1024;
    const Vec3f albedo(0.5f, 0.25f, 0.75f);
    const Vec3f skyMean(0.6f, 0.725f, 0.925f);
    const Vec3f expected = albedo * skyMean;

    Scene scene;
    Material diffuse;
    diffuse.albedo = albedo;
    addPlane(scene, 2.0f, 100.0f, diffuse);
    scene.commit();

    PathTracer tracer(width, height);
    const Camera camera(1.0f, Vec3f(0.0f));
    float earlyRmse = 0.0f;
    float finalRmse = 0.0f;

    for (int sample = 1; sample <= finalSampleCount; ++sample) {
        tracer.renderPass(scene, camera, 2);
        if (sample != earlySampleCount && sample != finalSampleCount) {
            continue;
        }

        float squaredError = 0.0f;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Vec3f estimate = tracer.getLinearPixel(x, y);
                squaredError += std::pow(estimate.x - expected.x, 2.0f);
                squaredError += std::pow(estimate.y - expected.y, 2.0f);
                squaredError += std::pow(estimate.z - expected.z, 2.0f);
            }
        }

        const float rmse = std::sqrt(
            squaredError / static_cast<float>(width * height * 3)
        );
        if (sample == earlySampleCount) {
            earlyRmse = rmse;
        } else {
            finalRmse = rmse;
        }
    }

    require(finalRmse < 0.01f, "Lambertian render did not converge to the analytic mean");
    require(finalRmse < earlyRmse, "Lambertian render error did not decrease with more samples");
}

void testAreaLightIlluminatesDiffuseSurface() {
    constexpr int width = 8;
    constexpr int height = 8;
    constexpr int sampleCount = 128;

    const Camera camera(1.0f, Vec3f(0.0f));
    const auto renderMeanLuminance = [&](const Scene& scene) {
        PathTracer tracer(width, height);
        for (int sample = 0; sample < sampleCount; ++sample) {
            tracer.renderPass(scene, camera, 1);
        }

        float meanLuminance = 0.0f;
        for (int y = 0; y < height; ++y) {
            for (int x = 0; x < width; ++x) {
                const Vec3f pixel = tracer.getLinearPixel(x, y);
                meanLuminance += pixel.x + pixel.y + pixel.z;
            }
        }
        return meanLuminance / static_cast<float>(width * height * 3);
    };

    Material diffuse;
    diffuse.albedo = Vec3f(0.8f);
    Material emitter;
    emitter.emission = Vec3f(4.0f);
    const std::vector<Vec3f> lightVertices = {
        Vec3f(2.0f, -1.0f, 0.8f),
        Vec3f(2.0f, -1.0f, 1.2f),
        Vec3f(2.0f, 1.0f, 1.2f),
        Vec3f(2.0f, 1.0f, 0.8f)
    };
    const std::vector<Index3> lightIndices = {
        Index3(0, 1, 2),
        Index3(0, 2, 3)
    };

    Scene litScene;
    addPlane(litScene, 2.0f, 100.0f, diffuse);
    litScene.addTriangleMesh(lightVertices, lightIndices, emitter);
    litScene.commit();
    const float litMean = renderMeanLuminance(litScene);
    require(litMean > 0.01f, "Area light did not illuminate the diffuse surface");

    Scene shadowedScene;
    addPlane(shadowedScene, 2.0f, 100.0f, diffuse);
    shadowedScene.addTriangleMesh(lightVertices, lightIndices, emitter);
    const std::vector<Vec3f> blockerVertices = {
        Vec3f(1.0f, -10.0f, 0.0f),
        Vec3f(1.0f, 10.0f, 0.0f),
        Vec3f(1.0f, 10.0f, 2.0f),
        Vec3f(1.0f, -10.0f, 2.0f)
    };
    const std::vector<Index3> blockerIndices = {
        Index3(0, 1, 2),
        Index3(0, 2, 3)
    };
    shadowedScene.addTriangleMesh(blockerVertices, blockerIndices, diffuse);
    shadowedScene.commit();
    const float shadowedMean = renderMeanLuminance(shadowedScene);
    require(shadowedMean < litMean * 0.1f, "Area light shadow did not block direct illumination");
}

} // namespace

int main() {
    testEmissiveReferenceImage();
    testLambertianConvergence();
    testAreaLightIlluminatesDiffuseSurface();
    std::cout << "Renderer reference and convergence tests passed\n";
}
