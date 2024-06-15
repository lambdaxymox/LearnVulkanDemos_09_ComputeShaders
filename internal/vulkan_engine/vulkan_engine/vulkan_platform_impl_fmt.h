#ifndef VULKAN_PLATFORM_IMPL_FMT_H
#define VULKAN_PLATFORM_IMPL_FMT_H

#include "vulkan_platform/physical_device_properties.h"
#include "vulkan_platform/physical_device_requirements.h"
#include "vulkan_platform/vulkan_instance_properties.h"
#include "vulkan_platform/vulkan_instance_requirements.h"

#include <fmt/core.h>


template <> struct fmt::formatter<VulkanEngine::VulkanPlatform::PhysicalDeviceProperties>: fmt::formatter<string_view> {
    auto format(const VulkanEngine::VulkanPlatform::PhysicalDeviceProperties& deviceProperties, format_context& ctx) const -> format_context::iterator;
};

template <> struct fmt::formatter<VulkanEngine::VulkanPlatform::PhysicalDeviceRequirements>: fmt::formatter<string_view> {
    auto format(const VulkanEngine::VulkanPlatform::PhysicalDeviceRequirements& deviceRequirements, format_context& ctx) const -> format_context::iterator;
};

template <> struct fmt::formatter<VulkanEngine::VulkanPlatform::VulkanInstanceProperties>: fmt::formatter<string_view> {
    auto format(const VulkanEngine::VulkanPlatform::VulkanInstanceProperties& platformInfo, format_context& ctx) const -> format_context::iterator;
};

template <> struct fmt::formatter<VulkanEngine::VulkanPlatform::VulkanInstanceRequirements>: fmt::formatter<string_view> {
    auto format(const VulkanEngine::VulkanPlatform::VulkanInstanceRequirements& requirements, format_context& ctx) const -> format_context::iterator;
};

#endif /* VULKAN_PLATFORM_IMPL_FMT_H */
