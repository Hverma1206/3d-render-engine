#pragma once

#include <glm/glm.hpp>

// Canonical per-vertex layout used by every mesh in this project.
// Attribute locations are fixed here and must match every vertex shader:
//   location 0 -> position  (vec3)
//   location 1 -> normal    (vec3)
//   location 2 -> texCoords (vec2)
//
// Keeping this as a plain struct (no virtual, no padding between members) lets
// us use offsetof() in glVertexAttribPointer safely and matches the layout
// Assimp produces (Phase 7 just copies fields in directly).
struct Vertex
{
    glm::vec3 position;
    glm::vec3 normal;
    glm::vec2 texCoords;
};
