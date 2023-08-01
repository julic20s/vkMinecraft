#pragma once
#ifndef VKMC_PHYSICAL_ENTITY_H_
#define VKMC_PHYSICAL_ENTITY_H_

#include <glm/vec3.hpp>

struct Entity {
  glm::vec3 aabb;
  glm::vec3 position;
  glm::vec3 velocity;
  glm::vec3 acceleration;
};

#endif // VKMC_PHYSICAL_ENTITY_H_
