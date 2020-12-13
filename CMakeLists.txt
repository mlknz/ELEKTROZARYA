cmake_minimum_required(VERSION 3.16.0)
project(ELEKTROZARYA)

set(SDL2_DIR C:/libs/SDL2) #requires sdl2-config.cmake in your SDL2 directory
set(SOURCES ${PROJECT_SOURCE_DIR}/source)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/build/")

find_package(SDL2 REQUIRED)
find_package(Vulkan REQUIRED)

include_directories(
    ${SOURCES}
    ${SDL2_INCLUDE_DIRS}
    ${Vulkan_INCLUDE_DIRS}
    external
    external/glm
    external/gli
)

set(IMGUI_SOURCE_FILES
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui.h
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui_demo.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui_draw.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui_widgets.cpp
    ${PROJECT_SOURCE_DIR}/external/imgui/imgui_internal.h
    ${PROJECT_SOURCE_DIR}/external/imgui/imconfig.h
    ${PROJECT_SOURCE_DIR}/external/imgui/imstb_rectpack.h
    ${PROJECT_SOURCE_DIR}/external/imgui/imstb_textedit.h
    ${PROJECT_SOURCE_DIR}/external/imgui/imstb_truetype.h
)

set(SOURCE_FILES
    ${IMGUI_SOURCE_FILES}
    ${SOURCES}/main.cpp
    ${SOURCES}/render/render_system.cpp
    ${SOURCES}/render/render_system.hpp
    ${SOURCES}/render/graphics_result.hpp
    ${SOURCES}/render/vulkan_include.hpp

    ${SOURCES}/render/vulkan/vulkan_instance.cpp
    ${SOURCES}/render/vulkan/vulkan_instance.hpp
    ${SOURCES}/render/vulkan/vulkan_device.cpp
    ${SOURCES}/render/vulkan/vulkan_device.hpp
    ${SOURCES}/render/vulkan/vulkan_swapchain.cpp
    ${SOURCES}/render/vulkan/vulkan_swapchain.hpp
    ${SOURCES}/render/vulkan/vulkan_render_pass.cpp
    ${SOURCES}/render/vulkan/vulkan_render_pass.hpp
    ${SOURCES}/render/vulkan/vulkan_device_memory_manager.cpp
    ${SOURCES}/render/vulkan/vulkan_device_memory_manager.hpp
    ${SOURCES}/render/vulkan/vulkan_buffer.cpp
    ${SOURCES}/render/vulkan/vulkan_buffer.hpp
    ${SOURCES}/render/vulkan/vulkan_graphics_pipeline.cpp
    ${SOURCES}/render/vulkan/vulkan_graphics_pipeline.hpp
    ${SOURCES}/render/vulkan/utils.hpp

    ${SOURCES}/core/file_utils.cpp
    ${SOURCES}/core/file_utils.hpp
    ${SOURCES}/core/scene/mesh.hpp
    ${SOURCES}/core/scene/scene.cpp
    ${SOURCES}/core/scene/scene.hpp
    ${SOURCES}/core/view.cpp
    ${SOURCES}/core/view.hpp
    ${SOURCES}/core/camera/camera.cpp
    ${SOURCES}/core/camera/camera.hpp
    ${SOURCES}/core/input/input.cpp
    ${SOURCES}/core/input/input.hpp

    ${SOURCES}/gameplay/gameplay.cpp
    ${SOURCES}/gameplay/gameplay.hpp
)

add_executable(ELEKTROZARYA ${SOURCE_FILES})

target_compile_definitions(ELEKTROZARYA PRIVATE VK_USE_PLATFORM_WIN32_KHR=1 GLM_FORCE_RADIANS=1)
target_link_libraries(ELEKTROZARYA
    # Vulkan_LIBRARIES returns .dll on windows which is incompatible with llvm linker
    ${Vulkan_INCLUDE_DIRS}/../Lib/vulkan-1.lib
    ${SDL2_LIBRARIES}
)

set_property(TARGET ELEKTROZARYA PROPERTY CXX_STANDARD 17)
