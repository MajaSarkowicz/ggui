#pragma once

#include <algorithm>
#include <cmath>
#include <cstdint>
#include <random>

struct Vec2f {
    float x = 0.0f;
    float y = 0.0f;

    constexpr Vec2f() = default;
    constexpr Vec2f(float x_value, float y_value) : x(x_value), y(y_value) {}
};

struct Vec3f {
    float x = 0.0f;
    float y = 0.0f;
    float z = 0.0f;

    constexpr Vec3f() = default;
    constexpr Vec3f(float value) : x(value), y(value), z(value) {}
    constexpr Vec3f(float x_value, float y_value, float z_value)
        : x(x_value), y(y_value), z(z_value) {}

    constexpr Vec3f& operator+=(const Vec3f& other) {
        x += other.x;
        y += other.y;
        z += other.z;
        return *this;
    }

    constexpr Vec3f& operator-=(const Vec3f& other) {
        x -= other.x;
        y -= other.y;
        z -= other.z;
        return *this;
    }

    constexpr Vec3f& operator*=(float scalar) {
        x *= scalar;
        y *= scalar;
        z *= scalar;
        return *this;
    }

    constexpr Vec3f& operator/=(float scalar) {
        x /= scalar;
        y /= scalar;
        z /= scalar;
        return *this;
    }
};

struct Index3 {
    std::uint32_t x;
    std::uint32_t y;
    std::uint32_t z;

    constexpr Index3(std::uint32_t x_value, std::uint32_t y_value, std::uint32_t z_value)
        : x(x_value), y(y_value), z(z_value) {}
};

constexpr Vec3f operator+(Vec3f lhs, const Vec3f& rhs) {
    return lhs += rhs;
}

constexpr Vec3f operator-(Vec3f lhs, const Vec3f& rhs) {
    return lhs -= rhs;
}

constexpr Vec3f operator-(const Vec3f& value) {
    return Vec3f(-value.x, -value.y, -value.z);
}

constexpr Vec3f operator*(const Vec3f& value, float scalar) {
    return Vec3f(value.x * scalar, value.y * scalar, value.z * scalar);
}

constexpr Vec3f operator*(float scalar, const Vec3f& value) {
    return value * scalar;
}

constexpr Vec3f operator*(const Vec3f& lhs, const Vec3f& rhs) {
    return Vec3f(lhs.x * rhs.x, lhs.y * rhs.y, lhs.z * rhs.z);
}

constexpr Vec3f operator/(const Vec3f& value, float scalar) {
    return Vec3f(value.x / scalar, value.y / scalar, value.z / scalar);
}

constexpr float dot(const Vec3f& lhs, const Vec3f& rhs) {
    return lhs.x * rhs.x + lhs.y * rhs.y + lhs.z * rhs.z;
}

constexpr Vec3f cross(const Vec3f& lhs, const Vec3f& rhs) {
    return Vec3f(
        lhs.y * rhs.z - lhs.z * rhs.y,
        lhs.z * rhs.x - lhs.x * rhs.z,
        lhs.x * rhs.y - lhs.y * rhs.x
    );
}

inline float length(const Vec3f& value) {
    return std::sqrt(dot(value, value));
}

inline Vec3f normalize(const Vec3f& value) {
    const float magnitude = length(value);
    return magnitude > 0.0f ? value / magnitude : Vec3f(0.0f);
}

inline Vec3f clamp(const Vec3f& value, float minimum, float maximum) {
    return Vec3f(
        std::clamp(value.x, minimum, maximum),
        std::clamp(value.y, minimum, maximum),
        std::clamp(value.z, minimum, maximum)
    );
}

constexpr Vec3f mix(const Vec3f& from, const Vec3f& to, float amount) {
    return from * (1.0f - amount) + to * amount;
}

struct Ray {
    Vec3f origin;
    float tnear = 0.001f;
    Vec3f direction;
    float tfar = 1e8f;
};

struct Intersection {
    bool hit = false;
    float t = 0.0f;
    Vec3f p;
    Vec3f n;
    int geomID = -1;
    int primID = -1;
};

inline Vec3f sampleCosineHemisphere(const Vec3f& normal, std::mt19937& rng) {
    std::uniform_real_distribution<float> dist(0.0f, 1.0f);
    const float r1 = dist(rng);
    const float r2 = dist(rng);
    const float z = std::sqrt(1.0f - r2);
    constexpr float pi = 3.14159265358979323846f;
    const float phi = 2.0f * pi * r1;
    const float x = std::cos(phi) * std::sqrt(r2);
    const float y = std::sin(phi) * std::sqrt(r2);

    const Vec3f helper = std::abs(normal.x) > 0.9f
        ? Vec3f(0.0f, 1.0f, 0.0f)
        : Vec3f(1.0f, 0.0f, 0.0f);
    const Vec3f tangent = normalize(cross(normal, helper));
    const Vec3f bitangent = cross(normal, tangent);
    return normalize(tangent * x + bitangent * y + normal * z);
}
