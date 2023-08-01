cmake_minimum_required(VERSION 3.21)

project(vkMinecraft VERSION 1.0.0)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED TRUE)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release)
endif()

message(STATUS CMAKE_BUILD_TYPE=${CMAKE_BUILD_TYPE})

# Vulkan
find_package(Vulkan)
if(NOT Vulkan_FOUND)
    message(FATAL_ERROR "Can not found Vulkan! Please install Vulkan SDK!")
endif()
include_directories(${Vulkan_INCLUDE_DIRS})
link_libraries(${Vulkan_LIBRARIES})

# GLFW
set(GLFW_BUILD_EXAMPLES OFF)
set(GLFW_BUILD_TESTS OFF)
set(GLFW_BUILD_DOCS OFF)
set(GLFW_INSTALL OFF)
add_subdirectory(third/glfw)

# GLM
add_subdirectory(third/glm)

# nlohmann/json
add_subdirectory(third/nlohmann)

# stb
add_subdirectory(third/stb)

# Tools
add_subdirectory(tools/respack)
add_subdirectory(tools/respack_builder)

# Game executable
add_executable(vkMinecraft)

# Dependencies
target_link_libraries(vkMinecraft PRIVATE respack glfw glm nlohmann_json stb_image)

# Sources
target_include_directories(vkMinecraft PRIVATE base/include)
file(GLOB_RECURSE VKMC_BASE_SOURCES FALSE base/sources/** ".cpp")
file(GLOB_RECURSE VKMC_GAME_SOURCES FALSE game/** ".cpp")
target_sources(vkMinecraft PRIVATE ${VKMC_BASE_SOURCES} ${VKMC_GAME_SOURCES})

# Assets
set(VKMC_ASSETS_DIR ${CMAKE_CURRENT_SOURCE_DIR}/assets)
file(GLOB_RECURSE VKMC_ASSETS_FILES FALSE ${VKMC_ASSETS_DIR}/** ".cpp")
set(VKMC_DEFAULT_ASSETS_PATH ${CMAKE_BINARY_DIR}/default.assets)
message(STATUS VKMC_ASSETS_DIR=${VKMC_ASSETS_DIR})
message(STATUS VKMC_DEFAULT_ASSETS_PATH=${VKMC_DEFAULT_ASSETS_PATH})
message(STATUS RESPACK_BUILDER_BINARY=${RESPACK_BUILDER_BINARY})
add_custom_target(
    default_assets ALL
    DEPENDS ${VKMC_ASSETS_FILES} respack_builder
    BYPRODUCTS ${VKMC_DEFAULT_ASSETS_PATH}
    COMMAND ${RESPACK_BUILDER_BINARY} ${VKMC_ASSETS_DIR} ${VKMC_DEFAULT_ASSETS_PATH}
)

target_compile_definitions(
    vkMinecraft PRIVATE
    -DGLFW_INCLUDE_VULKAN
    -DVULKAN_HPP_NO_CONSTRUCTORS
    -DGLM_FORCE_DEPTH_ZERO_TO_ONE
)

if(CMAKE_BUILD_TYPE STREQUAL Release)
    target_compile_definitions(vkMinecraft PRIVATE -DVKMC_NDEBUG)

    if(MSVC)
        target_link_options(vkMinecraft PRIVATE /SUBSYSTEM:WINDOWS /ENTRY:mainCRTStartup)
    else()
        target_link_options(vkMinecraft PRIVATE -mwindows)
    endif()
endif()

install(
    TARGETS vkMinecraft
    RUNTIME DESTINATION ./
    LIBRARY DESTINATION ./
    ARCHIVE DESTINATION ./
)
install(
    FILES ${VKMC_DEFAULT_ASSETS_PATH}
    DESTINATION .
)

