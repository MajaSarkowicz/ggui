#include "render_worker.h"

#include <chrono>

RenderWorker::RenderWorker(const Scene& scene, const Camera& camera, int width, int height)
    : scene(scene),
      pathTracer(width, height),
      requestedCamera(camera),
      publishedImage(static_cast<std::size_t>(width * height * 4), 0),
      thread(&RenderWorker::run, this) {}

RenderWorker::~RenderWorker() {
    {
        std::lock_guard lock(mutex);
        stopping = true;
    }
    condition.notify_one();
    thread.join();
}

void RenderWorker::request(const Camera& camera) {
    {
        std::lock_guard lock(mutex);
        requestedCamera = camera;
        ++requestedGeneration;
    }
    condition.notify_one();
}

bool RenderWorker::copyLatestImage(std::vector<std::uint8_t>& image, std::uint64_t& version) {
    std::lock_guard lock(mutex);
    if (version == publishedVersion) {
        return false;
    }

    image = publishedImage;
    version = publishedVersion;
    return true;
}

void RenderWorker::run() {
    std::uint64_t generation = 0;
    auto nextRenderTime = std::chrono::steady_clock::now();
    for (;;) {
        Camera renderCamera(1.0f, Vec3f(0.0f));
        {
            std::unique_lock lock(mutex);
            if (generation != 0 && pathTracer.getSampleCount() >= maxSamples) {
                condition.wait(lock, [&] {
                    return stopping || generation != requestedGeneration;
                });
            }
            if (stopping) {
                return;
            }

            if (generation != 0) {
                condition.wait_until(lock, nextRenderTime, [&] {
                    return stopping;
                });
                if (stopping) {
                    return;
                }
            }

            if (generation != requestedGeneration) {
                generation = requestedGeneration;
                pathTracer.reset();
            }
            renderCamera = requestedCamera;
        }

        pathTracer.renderPass(scene, renderCamera, 3);

        {
            std::lock_guard lock(mutex);
            const auto* begin = pathTracer.getDisplayData();
            publishedImage.assign(begin, begin + publishedImage.size());
            ++publishedVersion;
        }
        nextRenderTime = std::chrono::steady_clock::now()
            + std::chrono::duration_cast<std::chrono::steady_clock::duration>(
                std::chrono::duration<double>(renderInterval)
            );
    }
}
