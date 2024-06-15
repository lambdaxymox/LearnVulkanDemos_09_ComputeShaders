#ifndef VULKAN_PLATFORM_VULKAN_PLATFORM_H
#define VULKAN_PLATFORM_VULKAN_PLATFORM_H

#include "operating_system.h"
#include "vulkan_instance_properties.h"
#include "physical_device_properties.h"
#include "physical_device_requirements_builder.h"
#include "physical_device_requirements.h"
#include "vulkan_instance_requirements_builder.h"
#include "vulkan_instance_requirements.h"


namespace VulkanEngine {

namespace VulkanPlatform {


using MissingPhysicalDeviceRequirements = PhysicalDeviceRequirements;
using MissingPlatformRequirements = VulkanInstanceRequirements;

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


}

}

#endif /* VULKAN_PLATFORM_VULKAN_PLATFORM_H */
