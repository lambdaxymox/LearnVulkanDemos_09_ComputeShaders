#include "vulkan_platform.h"

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // GLFW_INCLUDE_VULKAN

using VulkanInstanceProperties = VulkanEngine::VulkanPlatform::VulkanInstanceProperties;
using VulkanInstanceRequirements = VulkanEngine::VulkanPlatform::VulkanInstanceRequirements;
using VulkanInstanceRequirementsBuilder = VulkanEngine::VulkanPlatform::VulkanInstanceRequirementsBuilder;
using PhysicalDeviceProperties = VulkanEngine::VulkanPlatform::PhysicalDeviceProperties;
using MissingPlatformRequirements = VulkanEngine::VulkanPlatform::MissingPlatformRequirements;
using MissingPhysicalDeviceRequirements = VulkanEngine::VulkanPlatform::MissingPhysicalDeviceRequirements;


VulkanInstanceProperties VulkanEngine::VulkanPlatform::PlatformInfoProvider::getVulkanInstanceInfo() const {
    auto availableLayers = this->getAvailableVulkanInstanceLayers();
    auto availableExtensions = this->getAvailableVulkanInstanceExtensions();    
    auto instanceInfo = VulkanInstanceProperties { availableLayers, availableExtensions };

    return instanceInfo;
}

VulkanInstanceRequirements VulkanEngine::VulkanPlatform::PlatformInfoProvider::getWindowSystemInstanceRequirements() const {
    uint32_t requiredExtensionCount = 0;
    const char** requiredExtensionNames = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    auto requiredExtensions = std::vector<std::string> {};
    for (int i = 0; i < requiredExtensionCount; i++) {
        requiredExtensions.emplace_back(std::string(requiredExtensionNames[i]));
    }

    auto builder = VulkanInstanceRequirementsBuilder {};
    for (const auto& extensionName : requiredExtensions) {
        builder.requireExtension(extensionName);
    }

    return builder.build();
}

PhysicalDeviceProperties VulkanEngine::VulkanPlatform::PlatformInfoProvider::getAvailableVulkanDeviceExtensions(
    VkPhysicalDevice physicalDevice
) const {
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

std::vector<VkLayerProperties> VulkanEngine::VulkanPlatform::PlatformInfoProvider::getAvailableVulkanInstanceLayers() const {
    auto instanceLayerProperties = std::vector<VkLayerProperties> {};
    uint32_t numInstanceExtensions = 0;
    vkEnumerateInstanceLayerProperties(&numInstanceExtensions, nullptr);
    if (numInstanceExtensions > 0) {
        instanceLayerProperties.resize(numInstanceExtensions);
        vkEnumerateInstanceLayerProperties(
            &numInstanceExtensions, 
            instanceLayerProperties.data()
        );
    }

    return instanceLayerProperties;
}

std::vector<VkExtensionProperties> VulkanEngine::VulkanPlatform::PlatformInfoProvider::getAvailableVulkanInstanceExtensions() const {
    auto instanceExtensionProperties = std::vector<VkExtensionProperties> {};
    uint32_t numInstanceExtensions = 0;
    vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions, nullptr);
    if (numInstanceExtensions > 0) {
        instanceExtensionProperties.resize(numInstanceExtensions);
        vkEnumerateInstanceExtensionProperties(
            nullptr, 
            &numInstanceExtensions, 
            instanceExtensionProperties.data()
        );
    }

    return instanceExtensionProperties;
}

MissingPlatformRequirements VulkanEngine::VulkanPlatform::PlatformInfoProvider::detectMissingInstanceRequirements(
    const VulkanInstanceProperties& instanceInfo,
    const VulkanInstanceRequirements& platformRequirements
) const {
    auto missingExtensionNames = std::vector<std::string> {};
    auto availableExtensions = instanceInfo.getAvailableExtensions();
    for (const auto& extensionName : platformRequirements.getExtensions()) {
        auto found = std::find_if(
            std::begin(availableExtensions),
            std::end(availableExtensions),
            [extensionName](const auto& extension) {
                return strcmp(extensionName.data(), extension.extensionName) == 0;
            }
        );
        auto extensionNotFound = (found == std::end(availableExtensions));
        if (extensionNotFound) {
            missingExtensionNames.emplace_back(extensionName);
        }
    }

    auto missingLayerNames = std::vector<std::string> {};
    auto availableLayers = instanceInfo.getAvailableLayers();
    for (const auto& layerName : platformRequirements.getLayers()) {
        auto found = std::find_if(
            std::begin(availableLayers),
            std::end(availableLayers),
            [layerName](const auto& layer) {
                return strcmp(layerName.data(), layer.layerName) == 0;
            }
        );
        auto layerNotFound = (found == std::end(availableLayers));
        if (layerNotFound) {
            missingLayerNames.emplace_back(layerName);
        }
    }

    return MissingPlatformRequirements(missingExtensionNames, missingLayerNames);
}

MissingPhysicalDeviceRequirements VulkanEngine::VulkanPlatform::PlatformInfoProvider::detectMissingRequiredDeviceExtensions(
    const PhysicalDeviceProperties& physicalDeviceProperties,
    const PhysicalDeviceRequirements& physicalDeviceRequirements
) const {
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

bool VulkanEngine::VulkanPlatform::PlatformInfoProvider::areValidationLayersSupported() const {
    auto instanceInfo = this->getVulkanInstanceInfo();

    return instanceInfo.areValidationLayersAvailable();
}