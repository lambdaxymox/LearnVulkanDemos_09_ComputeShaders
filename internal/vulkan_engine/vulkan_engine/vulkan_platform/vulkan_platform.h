#ifndef VULKAN_PLATFORM_VULKAN_PLATFORM_H
#define VULKAN_PLATFORM_VULKAN_PLATFORM_H

#include "constants.h"
#include "vulkan_instance_properties.h"
#include "physical_device_properties.h"


namespace VulkanEngine {

namespace VulkanPlatform {


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

    std::vector<std::string> getWindowSystemInstanceExtensions() const;

    std::vector<std::string> detectMissingRequiredDeviceExtensions(
        const PhysicalDeviceProperties& physicalDeviceProperties,
        const std::vector<std::string>& requiredExtensions
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

    std::vector<VkLayerProperties> getAvailableVulkanInstanceLayers() const;

    std::vector<std::string> detectMissingInstanceExtensions(
        const VulkanInstanceProperties& instanceInfo,
        const std::vector<std::string>& instanceExtensions
    ) const;

    std::vector<std::string> detectMissingInstanceLayers(
        const VulkanInstanceProperties& instanceInfo,
        const std::vector<std::string>& instanceExtensions
    ) const;

    PhysicalDeviceProperties getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice) const;

    bool areValidationLayersSupported() const;
};


}

}

#endif // VULKAN_PLATFORM_VULKAN_PLATFORM_H
