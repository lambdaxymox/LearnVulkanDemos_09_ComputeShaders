#ifndef PLATFORM_CAPABILITY_SCANNER_H
#define PLATFORM_CAPABILITY_SCANNER_H

#include "PlatformInfo.h"
#include "PhysicalDeviceProperties.h"
#include "PhysicalDeviceRequirements.h"
#include "PlatformRequirements.h"


using MissingPhysicalDeviceRequirements = PhysicalDeviceRequirements;
using MissingPlatformRequirements = PlatformRequirements;

class PlatformCapabilities final {
public:
    static PlatformInfo getPlatformInfo();
    static PhysicalDeviceProperties getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice);

    static MissingPlatformRequirements detectMissingInstanceRequirements(
        const PlatformInfo& platformInfo,
        const PlatformRequirements& platformRequirements
    );

    static MissingPhysicalDeviceRequirements detectMissingRequiredDeviceExtensions(
        const PhysicalDeviceProperties& physicalDeviceProperties,
        const PhysicalDeviceRequirements& physicalDeviceRequirements
    );
private:
    static std::vector<VkLayerProperties> getAvailableVulkanInstanceLayers();

    static std::vector<VkExtensionProperties> getAvailableVulkanInstanceExtensions();
};

#endif /* PLATFORM_CAPABILITY_SCANNER_H */
