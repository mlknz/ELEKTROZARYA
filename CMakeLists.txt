cmake_minimum_required(VERSION 3.16.0)
project(vulkan-ride)

set(SDL2_DIR C:/libs/SDL2) #requires sdl2-config.cmake in your SDL2 directory
find_package(SDL2 REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(Vulkan REQUIRED)

set(SOURCES ${PROJECT_SOURCE_DIR}/source)

include_directories(
    ${SOURCES}
    ${SOURCES}/Render
    ${SDL2_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
    external
    external/glm
    external/gli
)

set(SOURCE_FILES
    ${SOURCES}/main.cpp
    ${SOURCES}/Render/RenderSystem.cpp
    ${SOURCES}/Render/RenderSystem.hpp
    ${SOURCES}/Render/Vulkan/VulkanInstance.cpp
    ${SOURCES}/Render/Vulkan/VulkanInstance.hpp
    ${SOURCES}/Render/Vulkan/VulkanDevice.cpp
    ${SOURCES}/Render/Vulkan/VulkanDevice.hpp
    ${SOURCES}/Render/Vulkan/VulkanSwapchain.cpp
    ${SOURCES}/Render/Vulkan/VulkanSwapchain.hpp
    ${SOURCES}/Render/Vulkan/VulkanRenderPass.cpp
    ${SOURCES}/Render/Vulkan/VulkanRenderPass.hpp
    ${SOURCES}/Render/Vulkan/VulkanDeviceMemoryManager.cpp
    ${SOURCES}/Render/Vulkan/VulkanDeviceMemoryManager.hpp
    ${SOURCES}/Render/Vulkan/VulkanBuffer.cpp
    ${SOURCES}/Render/Vulkan/VulkanBuffer.hpp
    ${SOURCES}/Render/Vulkan/GraphicsPipeline.cpp
    ${SOURCES}/Render/Vulkan/GraphicsPipeline.hpp
    ${SOURCES}/Render/Vulkan/Utils.hpp
    ${SOURCES}/Render/Vulkan.hpp
    ${SOURCES}/Render/GraphicsResult.hpp
    ${SOURCES}/Render/Vulkan/Text/Text.hpp

    ${SOURCES}/FileUtils.cpp
    ${SOURCES}/FileUtils.hpp

    ${SOURCES}/Scene/Mesh.hpp
    ${SOURCES}/Scene/Scene.hpp
)

set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/")
add_executable(vulkan-ride ${SOURCE_FILES})
target_compile_definitions(vulkan-ride PRIVATE VK_USE_PLATFORM_WIN32_KHR=1 GLM_FORCE_RADIANS=1)
target_link_libraries(vulkan-ride
    # Vulkan_LIBRARIES returns .dll on windows which is incompatible with llvm linker
    ${Vulkan_INCLUDE_DIRS}/../Lib/vulkan-1.lib
    ${SDL2_LIBRARIES}
)

set_property(TARGET vulkan-ride PROPERTY CXX_STANDARD 17)
