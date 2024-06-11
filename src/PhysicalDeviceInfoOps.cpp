#include "PhysicalDeviceInfoOps.h"


PhysicalDeviceProperties PhysicalDeviceInfoOps::getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice) {
    auto deviceExtensionProperties =  std::vector<VkExtensionProperties> {};
    uint32_t numInstanceExtensions = 0;
    vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numInstanceExtensions, nullptr);
    if (numInstanceExtensions > 0) {
        deviceExtensionProperties.resize(numInstanceExtensions);
        vkEnumerateDeviceExtensionProperties(
            physicalDevice,
            nullptr, 
            &numInstanceExtensions, 
            deviceExtensionProperties.data()
        );
    }

    return PhysicalDeviceProperties(deviceExtensionProperties);
}

MissingPhysicalDeviceRequirements PhysicalDeviceInfoOps::detectMissingRequiredDeviceExtensions(
    const PhysicalDeviceProperties& physicalDeviceProperties,
    const PhysicalDeviceRequirements& physicalDeviceRequirements
) {
    auto missingExtensionNames = std::vector<std::string> {};
    auto requiredExtensions = physicalDeviceRequirements.getExtensions();
    for (const auto& requiredExtension : requiredExtensions) {
        auto found = std::find_if(
            std::begin(requiredExtensions),
            std::end(requiredExtensions),
            [requiredExtension](const auto& extension) {
            return strcmp(requiredExtension.data(), extension.data()) == 0;
            }
        );
        auto extensionNotFound = (found == std::end(requiredExtensions));
        if (extensionNotFound) {
            missingExtensionNames.emplace_back(requiredExtension);
        }
    }

    return PhysicalDeviceRequirements(missingExtensionNames);
}
