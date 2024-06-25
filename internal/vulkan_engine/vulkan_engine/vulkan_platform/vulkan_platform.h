#ifndef VULKAN_PLATFORM_VULKAN_PLATFORM_H
#define VULKAN_PLATFORM_VULKAN_PLATFORM_H

#include "constants.h"
#include "vulkan_instance_properties.h"
#include "physical_device_properties.h"
#include "physical_device_requirements_builder.h"
#include "physical_device_requirements.h"
#include "vulkan_instance_requirements_builder.h"
#include "vulkan_instance_requirements.h"


namespace VulkanEngine {

namespace VulkanPlatform {


using MissingPlatformRequirements = VulkanInstanceRequirements;
using MissingPhysicalDeviceRequirements = PhysicalDeviceRequirements;


class PlatformInfoProvider {
public:
    enum class Platform {
        Apple,
        Linux,
        Windows,
        Unknown
    };


    explicit PlatformInfoProvider() = default;
    ~PlatformInfoProvider() = default;

    VulkanInstanceProperties getVulkanInstanceInfo() const;

    VulkanInstanceRequirements getWindowSystemInstanceRequirements() const;

    MissingPhysicalDeviceRequirements detectMissingRequiredDeviceExtensions(
        const PhysicalDeviceProperties& physicalDeviceProperties,
        const PhysicalDeviceRequirements& physicalDeviceRequirements
    ) const;

    Platform detectOperatingSystem() const {
        #if defined(__APPLE__) || defined(__MACH__)
        return Platform::Apple;
        #elif defined(__LINUX__)
        return Platform::Linux;
        #elif defined(_WIN32)
        return Platform::Windows;
        #else
        return Platform::Unknown;
        #endif
    }

    std::vector<VkExtensionProperties> getAvailableVulkanInstanceExtensions() const;

    std::vector<VkLayerProperties> getAvailableVulkanInstanceLayers()const ;

    MissingPlatformRequirements detectMissingInstanceRequirements(
        const VulkanInstanceProperties& instanceInfo,
        const VulkanInstanceRequirements& platformRequirements
    ) const;

    PhysicalDeviceProperties getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice) const;

    bool areValidationLayersSupported() const;
};


}

}

#endif // VULKAN_PLATFORM_VULKAN_PLATFORM_H
