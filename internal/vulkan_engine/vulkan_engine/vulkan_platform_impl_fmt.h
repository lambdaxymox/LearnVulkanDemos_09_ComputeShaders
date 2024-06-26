#ifndef VULKAN_PLATFORM_IMPL_FMT_H
#define VULKAN_PLATFORM_IMPL_FMT_H

#include "vulkan_platform.h"

#include <fmt/core.h>


template <> struct fmt::formatter<VulkanEngine::VulkanPlatform::PhysicalDeviceProperties>: fmt::formatter<string_view> {
    auto format(const VulkanEngine::VulkanPlatform::PhysicalDeviceProperties& deviceProperties, format_context& ctx) const -> format_context::iterator;
};

template <> struct fmt::formatter<VulkanEngine::VulkanPlatform::VulkanInstanceProperties>: fmt::formatter<string_view> {
    auto format(const VulkanEngine::VulkanPlatform::VulkanInstanceProperties& platformInfo, format_context& ctx) const -> format_context::iterator;
};

#endif // VULKAN_PLATFORM_IMPL_FMT_H
