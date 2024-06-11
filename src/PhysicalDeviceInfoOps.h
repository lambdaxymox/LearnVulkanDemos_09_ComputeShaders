#ifndef PHYSICAL_DEVICE_INFO_OPS_H
#define PHYSICAL_DEVICE_INFO_OPS_H

#include "PhysicalDeviceProperties.h"
#include "PhysicalDeviceRequirements.h"


using MissingPhysicalDeviceRequirements = PhysicalDeviceRequirements;


class PhysicalDeviceInfoOps {
public:
    static PhysicalDeviceProperties getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice);

    static MissingPhysicalDeviceRequirements detectMissingRequiredDeviceExtensions(
        const PhysicalDeviceProperties& physicalDeviceProperties,
        const PhysicalDeviceRequirements& physicalDeviceRequirements
    );
};

#endif /* PHYSICAL_DEVICE_INFO_OPS_H */
