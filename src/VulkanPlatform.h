#ifndef VULKAN_PLATFORM_H
#define VULKAN_PLATFORM_H

#include "VulkanInstanceProperties.h"
#include "PhysicalDeviceProperties.h"
#include "PhysicalDeviceRequirements.h"
#include "VulkanInstanceRequirements.h"


#include <numbers>

using MissingPhysicalDeviceRequirements = PhysicalDeviceRequirements;
using MissingPlatformRequirements = VulkanInstanceRequirements;

namespace VulkanPlatform {
    VulkanInstanceProperties getVulkanInstanceInfo();
    PhysicalDeviceProperties getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice);
    
    std::vector<VkLayerProperties> getAvailableVulkanInstanceLayers();
    std::vector<VkExtensionProperties> getAvailableVulkanInstanceExtensions();

    MissingPlatformRequirements detectMissingInstanceRequirements(
        const VulkanInstanceProperties& platformInfo,
        const VulkanInstanceRequirements& platformRequirements
    );

    MissingPhysicalDeviceRequirements detectMissingRequiredDeviceExtensions(
        const PhysicalDeviceProperties& physicalDeviceProperties,
        const PhysicalDeviceRequirements& physicalDeviceRequirements
    );

    const std::string VK_LAYER_KHRONOS_validation = std::string { "VK_LAYER_KHRONOS_validation" };
    const std::string VK_KHR_portability_subset = std::string { "VK_KHR_portability_subset" };
}

#endif /* VULKAN_PLATFORM_H */
