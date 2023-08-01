#include <algorithm>

#include "entity_chunk_system.h"

void EntityChunkSystem::Update(Entity &entity, ChunkManager &chunks) {
  entity.velocity += entity.acceleration;
  if (glm::length(entity.velocity) > .5) {
    entity.velocity = glm::normalize(entity.velocity) * glm::vec3(0.5);
  }
  if (glm::length(entity.acceleration) == 0) {
    entity.velocity = glm::vec3(0);
  }
  entity.acceleration = glm::vec3(0);
  entity.position += entity.velocity;
}
