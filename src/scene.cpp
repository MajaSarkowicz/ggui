// src/scene.cpp
#include "scene.h"
#include <algorithm>
#include <cstdint>

Scene::Scene() {
    device = rtcNewDevice(nullptr);
    scene = rtcNewScene(device);
}

Scene::~Scene() {
    rtcReleaseScene(scene);
    rtcReleaseDevice(device);
}

void Scene::addTriangleMesh(const std::vector<Vec3f>& vertices, const std::vector<glm::uvec3>& indices, const Material& mat) {
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
        isect.n = glm::normalize(Vec3f(
            rayhit.hit.Ng_x,
            rayhit.hit.Ng_y,
            rayhit.hit.Ng_z
        ));
    if (glm::dot(isect.n, ray.direction) > 0.0f) {
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