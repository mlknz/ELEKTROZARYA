cmake_minimum_required(VERSION 3.12.0)
project(vulkan-ride)

set(CXX_STANDARD 17)
SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++17")

include_directories(external)
include_directories(external/glm)
include_directories(external/gli)
include_directories(external/assimp)

find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(ASSIMP REQUIRED)
find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

include_directories(
    ${PROJECT_SOURCE_DIR}
    ${FREETYPE_INCLUDE_DIRS}
    ${SDL2_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
)

SET(SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/source/main.cpp
    ${PROJECT_SOURCE_DIR}/source/Render/RenderSystem.cpp
    ${PROJECT_SOURCE_DIR}/source/Render/RenderSystem.hpp
    ${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanInstance.cpp
    ${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanInstance.hpp
    ${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanDevice.cpp
    ${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanDevice.hpp
    ${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanSwapchain.cpp
    ${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanSwapchain.hpp
	${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanBuffer.cpp
	${PROJECT_SOURCE_DIR}/source/Render/Vulkan/VulkanBuffer.hpp
	${PROJECT_SOURCE_DIR}/source/Render/Vulkan/GraphicsPipeline.cpp
	${PROJECT_SOURCE_DIR}/source/Render/Vulkan/GraphicsPipeline.hpp
    ${PROJECT_SOURCE_DIR}/source/Render/Vulkan/Utils.hpp
	${PROJECT_SOURCE_DIR}/source/Render/Vulkan/Text/Text.hpp

	${PROJECT_SOURCE_DIR}/source/FileUtils.cpp
	${PROJECT_SOURCE_DIR}/source/FileUtils.hpp

    ${PROJECT_SOURCE_DIR}/source/Scene/Mesh.hpp
	${PROJECT_SOURCE_DIR}/source/Scene/Scene.hpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/")
add_executable(vulkan-ride ${SOURCE_FILES} )
target_compile_definitions(vulkan-ride PRIVATE VK_USE_PLATFORM_XCB_KHR=1 GLM_FORCE_RADIANS=1)

TARGET_LINK_LIBRARIES(vulkan-ride -L$ENV{VK_SDK_PATH}/x86_64/lib ${FREETYPE_LIBRARIES} ${ASSIMP_LIBRARIES} -lSDL2main -lSDL2 -lSDL2_image -lvulkan)
