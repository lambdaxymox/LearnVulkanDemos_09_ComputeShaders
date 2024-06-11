#ifndef PLATFORM_INFO_OPS_H
#define PLATFORM_INFO_OPS_H

#include "PlatformInfo.h"
#include "PlatformRequirements.h"

#include <vector>
#include <string>


using MissingPlatformRequirements = PlatformRequirements;


class PlatformInfoOps {
public:
    static PlatformInfo getPlatformInfo();

    static MissingPlatformRequirements detectMissingInstanceRequirements(
        const PlatformInfo& platformInfo,
        const PlatformRequirements& platformRequirements
    );
private:
    static std::vector<VkLayerProperties> getAvailableVulkanInstanceLayers();

    static std::vector<VkExtensionProperties> getAvailableVulkanInstanceExtensions();
};

#endif /* PLATFORM_INFO_OPS_H */
