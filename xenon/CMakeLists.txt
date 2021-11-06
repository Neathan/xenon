cmake_minimum_required(VERSION 3.10)

project(xenon CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

set(KTX_FEATURE_STATIC_LIBRARY ON)

add_subdirectory(libs/glad/)
add_subdirectory(libs/glfw/)
add_subdirectory(libs/spdlog/)
add_subdirectory(libs/glm/)
add_subdirectory(libs/tinygltf/)
add_subdirectory(libs/entt)
add_subdirectory(libs/ktx)
add_subdirectory(libs/mono)

add_library(xenon STATIC
	"src/xenon/core/application.cpp"
	"src/xenon/core/assert.h"
	"src/xenon/core/debug.cpp"
	"src/xenon/core/debug.h"
	"src/xenon/core/filesystem.cpp"
	"src/xenon/core/filesystem.h"
	"src/xenon/core/input.cpp"
	"src/xenon/core/input.h"
	"src/xenon/core/log.h"
	"src/xenon/core/time.h"
	"src/xenon/core/uuid.cpp"
	"src/xenon/core/uuid.h"
	"src/xenon/core/asset.h"
	"src/xenon/core/asset_manager.h"
	"src/xenon/core/asset.cpp"
	"src/xenon/core/asset_manager.cpp"
	"src/xenon/graphics/camera.h"
	"src/xenon/graphics/framebuffer.cpp"
	"src/xenon/graphics/framebuffer.h"
	"src/xenon/graphics/model.cpp"
	"src/xenon/graphics/model.h"
	"src/xenon/graphics/model_loader.cpp"
	"src/xenon/graphics/model_loader.h"
	"src/xenon/graphics/primitive.cpp"
	"src/xenon/graphics/primitive.h"
	"src/xenon/graphics/primitives.cpp"
	"src/xenon/graphics/primitives.h"
	"src/xenon/graphics/animation.h"
	"src/xenon/graphics/renderer.cpp"
	"src/xenon/graphics/renderer.h"
	"src/xenon/graphics/shader.cpp"
	"src/xenon/graphics/shader.h"
	"src/xenon/graphics/material.h"
	"src/xenon/graphics/texture.h"
	"src/xenon/graphics/texture.cpp"
	"src/xenon/graphics/light.h"
	"src/xenon/graphics/environment.h"
	"src/xenon/graphics/brdf.h"
	"src/xenon/graphics/brdf.cpp"
	"src/xenon/scene/scene.h"
	"src/xenon/scene/scene.cpp"
	"src/xenon/scripting/script.h"
	"src/xenon/scripting/script.cpp"
	 "src/xenon/graphics/animation.cpp" "src/xenon/graphics/skin.h")


target_compile_definitions(xenon PUBLIC GLFW_INCLUDE_NONE)

target_include_directories(xenon PUBLIC src/)
target_link_libraries(xenon PUBLIC glad)
target_link_libraries(xenon PUBLIC glfw)
target_link_libraries(xenon PUBLIC spdlog::spdlog)
target_link_libraries(xenon PUBLIC glm)
target_link_libraries(xenon PUBLIC tinygltf)
target_link_libraries(xenon PUBLIC EnTT::EnTT)
target_link_libraries(xenon PUBLIC ktx)
target_link_libraries(xenon PUBLIC mono)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_CURRENT_LIST_DIR}/assets $<TARGET_FILE_DIR:${PROJECT_NAME}>/assets)

add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_CURRENT_LIST_DIR}/libs/mono/bin $<TARGET_FILE_DIR:${PROJECT_NAME}>/mono)

					add_custom_command(TARGET ${PROJECT_NAME} POST_BUILD
                    COMMAND ${CMAKE_COMMAND} -E copy_directory
                    ${CMAKE_CURRENT_LIST_DIR}/libs/mono/compiler $<TARGET_FILE_DIR:${PROJECT_NAME}>/mono/mono/lib)