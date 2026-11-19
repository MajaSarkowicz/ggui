// src/scene.cpp
#include "scene.h"
#include <algorithm>
#include <cmath>
#include <cstdint>

Scene::Scene() {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
}

Scene::~Scene() {
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
}

void Scene::addTriangleMesh(const std::vector<Vec3f>& vertices, const std::vector<Index3>& indices, const Material& mat) {
    RTCGeometry geom = rtcNewGeometry(device, RTC_GEOMETRY_TYPE_TRIANGLE);
    
    Vec3f* vertexBuffer = (Vec3f*)rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_VERTEX, 0, RTC_FORMAT_FLOAT3, sizeof(Vec3f), vertices.size());
    std::copy(vertices.begin(), vertices.end(), vertexBuffer);
    
    auto* indexBuffer = static_cast<std::uint32_t*>(
        rtcSetNewGeometryBuffer(geom, RTC_BUFFER_TYPE_INDEX, 0, RTC_FORMAT_UINT3, 3 * sizeof(std::uint32_t), indices.size()));
    for (std::size_t i = 0; i < indices.size(); ++i) {
        indexBuffer[3 * i] = indices[i].x;
        indexBuffer[3 * i + 1] = indices[i].y;
        indexBuffer[3 * i + 2] = indices[i].z;
    }
    
    rtcCommitGeometry(geom);
    unsigned int geomID = rtcAttachGeometry(scene, geom);
    rtcReleaseGeometry(geom);
    
    if (geomID >= materials.size()) materials.resize(geomID + 1);
    materials[geomID] = mat;

    if (mat.isEmissive()) {
        for (std::size_t i = 0; i < indices.size(); ++i) {
            const Index3& index = indices[i];
            const Vec3f& a = vertices[index.x];
            const Vec3f& b = vertices[index.y];
            const Vec3f& c = vertices[index.z];
            const Vec3f crossProduct = cross(b - a, c - a);
            const float twiceArea = length(crossProduct);
            if (twiceArea <= 0.0f) {
                continue;
            }

            totalLightArea += 0.5f * twiceArea;
            lights.push_back({
                a,
                b,
                c,
                crossProduct / twiceArea,
                mat.emission,
                totalLightArea,
                geomID,
                static_cast<std::uint32_t>(i)
            });
        }
    }
}

void Scene::commit() {
    rtcCommitScene(scene);
}

Intersection Scene::intersect(const Ray& ray) const {
    RTCIntersectContext context;
    rtcInitIntersectContext(&context);
    
    RTCRayHit rayhit{};
    rayhit.ray.org_x = ray.origin.x; rayhit.ray.org_y = ray.origin.y; rayhit.ray.org_z = ray.origin.z;
    rayhit.ray.dir_x = ray.direction.x; rayhit.ray.dir_y = ray.direction.y; rayhit.ray.dir_z = ray.direction.z;
    rayhit.ray.tnear = ray.tnear;
    rayhit.ray.tfar = ray.tfar;
    rayhit.ray.time = 0; rayhit.ray.mask = -1;
    rayhit.ray.id = 0; rayhit.ray.flags = 0;
    rayhit.hit.geomID = RTC_INVALID_GEOMETRY_ID;
    
    rtcIntersect1(scene, &context, &rayhit);
    
    Intersection isect;
    if (rayhit.hit.geomID != RTC_INVALID_GEOMETRY_ID) {
        isect.hit = true;
        isect.t = rayhit.ray.tfar;
        isect.p = ray.origin + ray.direction * isect.t;
        isect.n = normalize(Vec3f(
            rayhit.hit.Ng_x,
            rayhit.hit.Ng_y,
            rayhit.hit.Ng_z
        ));
        if (dot(isect.n, ray.direction) > 0.0f) {
            isect.n = -isect.n;
        }
        isect.geomID = rayhit.hit.geomID;
        isect.primID = rayhit.hit.primID;
    }
    return isect;
}

const Material& Scene::getMaterial(int geomID) const {
    return materials[geomID];
}

std::optional<AreaLightSample> Scene::sampleLight(float select, float u, float v) const {
    if (lights.empty() || totalLightArea <= 0.0f) {
        return std::nullopt;
    }

    const float targetArea = std::clamp(select, 0.0f, std::nextafter(1.0f, 0.0f))
        * totalLightArea;
    const auto light = std::lower_bound(
        lights.begin(),
        lights.end(),
        targetArea,
        [](const LightTriangle& triangle, float area) {
            return triangle.cumulativeArea < area;
        }
    );
    const LightTriangle& triangle = light == lights.end() ? lights.back() : *light;

    const float rootU = std::sqrt(u);
    const float weightA = 1.0f - rootU;
    const float weightB = rootU * (1.0f - v);
    const float weightC = rootU * v;
    return AreaLightSample{
        triangle.a * weightA + triangle.b * weightB + triangle.c * weightC,
        triangle.normal,
        triangle.emission,
        1.0f / totalLightArea
    };
}

float Scene::lightPdfSolidAngle(
    const Vec3f& referencePoint,
    const Intersection& lightHit
) const {
    const auto light = std::find_if(
        lights.begin(),
        lights.end(),
        [&lightHit](const LightTriangle& triangle) {
            return triangle.geomID == static_cast<unsigned int>(lightHit.geomID)
                && triangle.primID == static_cast<std::uint32_t>(lightHit.primID);
        }
    );
    if (light == lights.end() || totalLightArea <= 0.0f) {
        return 0.0f;
    }

    const Vec3f toReference = referencePoint - lightHit.p;
    const float distanceSquared = dot(toReference, toReference);
    if (distanceSquared <= 0.0f) {
        return 0.0f;
    }

    const float cosineOnLight = dot(light->normal, toReference / std::sqrt(distanceSquared));
    if (cosineOnLight <= 0.0f) {
        return 0.0f;
    }
    return distanceSquared / (totalLightArea * cosineOnLight);
}