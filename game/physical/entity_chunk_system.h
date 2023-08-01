#pragma once
#ifndef VKMC_PHYSICAL_ENTITY_CHUNK_SYSTEM_H_
#define VKMC_PHYSICAL_ENTITY_CHUNK_SYSTEM_H_

#include "entity.h"
#include "../chunk/manager.h"

class EntityChunkSystem {

public:
    void Update(Entity &, ChunkManager &);
};

#endif // VKMC_PHYSICAL_ENTITY_CHUNK_SYSTEM_H_
