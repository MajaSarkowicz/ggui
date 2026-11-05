#pragma once
#include "math_types.h"
#include <tiny_obj_loader.h>
#include <vector>
#include <string>
#include <iostream>

inline bool loadObj(const std::string& filename, 
                    std::vector<Vec3f>& out_vertices, 
                    std::vector<glm::uvec3>& out_indices,
                    Vec3f offset = Vec3f(0.0f), 
                    float scale = 1.0f) 
{
    tinyobj::ObjReader reader;
    if (!reader.ParseFromFile(filename)) {
        std::cerr << "Błąd ładownia OBJ: " << reader.Error() << "\n";
        return false;
    }

    auto& attrib = reader.GetAttrib();
    // Konwersja i transformacja wierzchołków
    for (size_t v = 0; v < attrib.vertices.size() / 3; v++) {
        Vec3f vertex(
            attrib.vertices[3 * v + 0],
            attrib.vertices[3 * v + 1],
            attrib.vertices[3 * v + 2]
        );
        out_vertices.push_back((vertex * scale) + offset);
    }

    // Wyciąganie indeksów trójkątów z poszczególnych pod-siatek
    for (const auto& shape : reader.GetShapes()) {
        size_t index_offset = 0;
        for (size_t f = 0; f < shape.mesh.num_face_vertices.size(); f++) {
            if (shape.mesh.num_face_vertices[f] == 3) {
                out_indices.emplace_back(
                    shape.mesh.indices[index_offset + 0].vertex_index,
                    shape.mesh.indices[index_offset + 1].vertex_index,
                    shape.mesh.indices[index_offset + 2].vertex_index
                );
            }
            index_offset += 3;
        }
    }
    return true;
}