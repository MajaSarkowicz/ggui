#pragma once
#include "math_types.h"

#include <algorithm>
#include <cmath>

class Camera {
public:
    Camera(float aspect_ratio, const Vec3f& start_position)
        : aspect(aspect_ratio), position(start_position) {}

    void look(float yaw_delta, float pitch_delta) {
        yaw += yaw_delta;
        pitch = std::clamp(pitch + pitch_delta, -1.55f, 1.55f);
    }

    void moveLocal(const Vec3f& displacement) {
        const Vec3f forward(std::sin(yaw), 0.0f, std::cos(yaw));
        const Vec3f right(std::cos(yaw), 0.0f, -std::sin(yaw));
        position += right * displacement.x
            + Vec3f(0.0f, displacement.y, 0.0f)
            + forward * displacement.z;
    }

    Ray generateRay(float u, float v) const {
        const float cos_pitch = std::cos(pitch);
        const Vec3f forward(
            std::sin(yaw) * cos_pitch,
            std::sin(pitch),
            std::cos(yaw) * cos_pitch
        );
        const Vec3f right = normalize(cross(Vec3f(0.0f, 1.0f, 0.0f), forward));
        const Vec3f up = cross(forward, right);

        const float screen_x = (2.0f * u - 1.0f) * aspect;
        const float screen_y = 1.0f - 2.0f * v;
        const Vec3f direction = normalize(
            forward * 2.0f + right * screen_x + up * screen_y
        );

        return Ray{position, 0.001f, direction, 1000.0f};
    }

private:
    float aspect;
    Vec3f position;
    float yaw = 0.0f;
    float pitch = 0.0f;
};
