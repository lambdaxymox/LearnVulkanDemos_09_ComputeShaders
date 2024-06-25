#include "vulkan_platform.h"

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // GLFW_INCLUDE_VULKAN

using VulkanInstanceProperties = VulkanEngine::VulkanPlatform::VulkanInstanceProperties;
using PhysicalDeviceProperties = VulkanEngine::VulkanPlatform::PhysicalDeviceProperties;
using MissingPhysicalDeviceRequirements = VulkanEngine::VulkanPlatform::MissingPhysicalDeviceRequirements;


VulkanInstanceProperties VulkanEngine::VulkanPlatform::PlatformInfoProvider::getVulkanInstanceInfo() const {
    auto availableLayers = this->getAvailableVulkanInstanceLayers();
    auto availableExtensions = this->getAvailableVulkanInstanceExtensions();    
    auto instanceInfo = VulkanInstanceProperties { availableLayers, availableExtensions };

    return instanceInfo;
}

std::vector<std::string> VulkanEngine::VulkanPlatform::PlatformInfoProvider::getWindowSystemInstanceExtensions() const {
    uint32_t requiredExtensionCount = 0;
    const char** requiredExtensionNames = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
    auto requiredExtensions = std::vector<std::string> {};
    for (int i = 0; i < requiredExtensionCount; i++) {
        requiredExtensions.emplace_back(std::string(requiredExtensionNames[i]));
    }

    return requiredExtensions;
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

std::vector<std::string> VulkanEngine::VulkanPlatform::PlatformInfoProvider::detectMissingInstanceExtensions(
    const VulkanInstanceProperties& instanceInfo,
    const std::vector<std::string>& instanceExtensions
) const {
    auto missingInstanceExtensions = std::vector<std::string> {};
    auto availableInstanceExtensions = instanceInfo.getAvailableExtensions();
    for (const auto& extensionName : instanceExtensions) {
        auto found = std::find_if(
            std::begin(availableInstanceExtensions),
            std::end(availableInstanceExtensions),
            [extensionName](const auto& extension) {
                return strcmp(extensionName.data(), extension.extensionName) == 0;
            }
        );
        auto extensionNotFound = (found == std::end(availableInstanceExtensions));
        if (extensionNotFound) {
            missingInstanceExtensions.emplace_back(extensionName);
        }
    }

    return missingInstanceExtensions;
}

std::vector<std::string> VulkanEngine::VulkanPlatform::PlatformInfoProvider::detectMissingInstanceLayers(
    const VulkanInstanceProperties& instanceInfo,
    const std::vector<std::string>& instanceLayers
) const {
    auto missingInstanceLayers = std::vector<std::string> {};
    auto availableInstanceLayers = instanceInfo.getAvailableLayers();
    for (const auto& layerName : instanceLayers) {
        auto found = std::find_if(
            std::begin(availableInstanceLayers),
            std::end(availableInstanceLayers),
            [layerName](const auto& layer) {
                return strcmp(layerName.data(), layer.layerName) == 0;
            }
        );
        auto layerNotFound = (found == std::end(availableInstanceLayers));
        if (layerNotFound) {
            missingInstanceLayers.emplace_back(layerName);
        }
    }

    return missingInstanceLayers;
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