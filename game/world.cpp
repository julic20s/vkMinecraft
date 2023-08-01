#include <application.h>

#include "block/registry.h"
#include "chunk/manager.h"
#include "physical/entity_chunk_system.h"
#include "player.h"
#include "render/camera.h"
#include "renderer.h"

class World {
private:
  ChunkManager chunks_;
  Player player_;
  BlockRegistry block_registry_;
  EntityChunkSystem entity_chunk_system_;
  Renderer renderer_;

public:
  World() : chunks_(114514), player_(chunks_), renderer_(chunks_, block_registry_) {
    player_.GetEntity().position = {0, 20, 0};
    renderer_.BindCamera(player_.GetCamera());
  }

  void Update() {
    player_.Update();
    chunks_.LoadAutomatic(block_registry_, player_.GetEntity().position);
    entity_chunk_system_.Update(player_.GetEntity(), chunks_);
    renderer_.Render(player_.GetEntity().position);
  }
};

static World *world;

void app::Initialize() {
  world = new World;
}

void app::Update() {
  world->Update();
}

void app::Uninitialize() {
  delete world;
}
