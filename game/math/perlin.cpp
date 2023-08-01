#include "perlin.h"

const std::array<glm::vec2, 8> kPerlinGrad{
    glm::vec2(1, 0),
    glm::vec2(-1, 0),
    glm::vec2(0, -1),
    glm::vec2(0, 1),
    glm::normalize(glm::vec2(1.f, 1.f)),
    glm::normalize(glm::vec2(1.f, -1.f)),
    glm::normalize(glm::vec2(-1.f, 1.f)),
    glm::normalize(glm::vec2(-1.f, -1.f)),
};
