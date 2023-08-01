#include <stdexcept>
#include <string>
#ifdef VKMC_NDEBUG
#include <iostream>
#endif

#include <GLFW/glfw3.h>

#include <application.h>
#include <assets/json.h>
#include <event.h>

#include "internal/assets.h"
#include "internal/input.h"
#include "internal/vulkan.h"
#include "internal/window.h"

EventDispatcher events::global;

static GLFWwindow *InitializeWindow() {
  if (glfwInit() != GLFW_TRUE) {
    throw std::runtime_error("Failed to initialize GLFW!");
  }
  auto &config = assets::LoadJson("config.json");
  auto &window_config = config["window"];
  std::string name = config["name"];
  auto window = window::internal::CreateWindow(
      window_config["width"],
      window_config["height"],
      name.c_str()
  );
  vulkan::internal::Initialize(window, name);
  input::internal::Bind(window);
  assets::Unload("config.json");
  glfwSetInputMode(window, GLFW_CURSOR, GLFW_CURSOR_DISABLED);
  return window;
}

static void UnInitializeWindow() {
  vulkan::internal::Uninitialize();
  glfwTerminate();
}

void GameMain() {
  assets::internal::LoadAssetsFile("default.assets");
  auto window = InitializeWindow();

  app::Initialize();
  while (!glfwWindowShouldClose(window)) {
    glfwPollEvents();
    app::Update();
  }
  app::Uninitialize();

  UnInitializeWindow();
  assets::internal::UnloadAssetsFile();
}

int main(void) {
#ifdef VKMC_NDEBUG
  try
#endif
  {
    GameMain();
  }
#ifdef VKMC_NDEBUG
  catch (std::exception e) {
    std::cerr << e.what() << '\n';
    return 1;
  }
#endif
  return 0;
}
