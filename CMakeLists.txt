cmake_minimum_required(VERSION 2.8)
project(vulkan-ride)

SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")

include_directories(external)
include_directories(external/glm)
include_directories(external/gli)
include_directories(external/assimp)

find_package(Freetype REQUIRED)
find_package(PkgConfig REQUIRED)
find_package(ASSIMP REQUIRED)
find_package(SDL2 REQUIRED)

#PKG_SEARCH_MODULE(SDL2 REQUIRED sdl2>=2.0.8)
#PKG_SEARCH_MODULE(SDL2IMAGE REQUIRED SDL2_image>=2.0.8)
include_directories(${SDL2_INCLUDE_DIRS})

#TODO: use findVulkan cmake module (3.7.2+ cmake version)
include_directories("${PROJECT_SOURCE_DIR}" $ENV{VK_SDK_PATH}/x86_64/include ${FREETYPE_INCLUDE_DIRS})

SET(SOURCE_FILES
    # ${HEADERS}
    ${PROJECT_SOURCE_DIR}/source/main.cpp
)
set(CMAKE_RUNTIME_OUTPUT_DIRECTORY "${PROJECT_SOURCE_DIR}/bin/")
add_executable(vulkan-ride ${SOURCE_FILES} )

TARGET_LINK_LIBRARIES(vulkan-ride -L$ENV{VK_SDK_PATH}/x86_64/lib ${FREETYPE_LIBRARIES} ${ASSIMP_LIBRARIES} -lSDL2main -lSDL2 -lSDL2_image -lvulkan)


