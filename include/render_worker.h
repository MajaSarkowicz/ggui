#pragma once

#include "camera.h"
#include "pathtracer.h"

#include <condition_variable>
#include <cstdint>
#include <mutex>
#include <thread>
#include <vector>

class RenderWorker {
public:
    RenderWorker(const Scene& scene, const Camera& camera, int width, int height);
    ~RenderWorker();

    RenderWorker(const RenderWorker&) = delete;
    RenderWorker& operator=(const RenderWorker&) = delete;

    void request(const Camera& camera);
    bool copyLatestImage(std::vector<std::uint8_t>& image, std::uint64_t& version);

private:
    static constexpr int maxSamples = 128;
    static constexpr double renderInterval = 1.0 / 12.0;

    const Scene& scene;
    PathTracer pathTracer;
    Camera requestedCamera;
    std::vector<std::uint8_t> publishedImage;
    std::mutex mutex;
    std::condition_variable condition;
    std::uint64_t requestedGeneration = 1;
    std::uint64_t publishedVersion = 0;
    bool stopping = false;
    std::thread thread;

    void run();
};
