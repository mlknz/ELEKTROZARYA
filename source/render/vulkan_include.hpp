#pragma once

#define NOMINMAX
#define VULKAN_HPP_NO_EXCEPTIONS

#define VK_USE_PLATFORM_WIN32_KHR 1
#define VULKAN_HPP_DISPATCH_LOADER_DYNAMIC 1

// don't forget about VULKAN_HPP_DEFAULT_DISPATCH_LOADER_DYNAMIC_STORAGE in .cpp
#include <vulkan/vulkan.hpp>

namespace ez
{
}
