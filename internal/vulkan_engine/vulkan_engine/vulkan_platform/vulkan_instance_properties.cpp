#include "vulkan_instance_properties.h"
#include "constants.h"


using VulkanInstanceProperties = VulkanEngine::VulkanPlatform::VulkanInstanceProperties;

VulkanInstanceProperties::VulkanInstanceProperties(std::vector<VkLayerProperties> availableLayers, std::vector<VkExtensionProperties> availableExtensions) 
    : m_availableLayers { availableLayers }
    , m_availableExtensions { availableExtensions }
{
    for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerProperties.layerName, VulkanPlatform::VK_LAYER_KHRONOS_validation.data()) == 0) {
            m_validationLayersAvailable = true;
        }
    }

    for (const auto& extensionProperties : availableExtensions) {
        if (strcmp(extensionProperties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            m_debugUtilsAvailable = true;
        }
    }
}

bool VulkanInstanceProperties::isExtensionAvailable(const char* layerName) const {
    for (const auto& layerProperties : m_availableLayers) {
        if (strcmp(layerProperties.layerName, layerName) == 0) {
            return true;
        }
    }

    return false;
}

bool VulkanInstanceProperties::isLayerAvailable(const char* extensionName) const {
    for (const auto& extensionProperties : m_availableExtensions) {
        if (strcmp(extensionProperties.extensionName, extensionName) == 0) {
            return true;
        }
    }

    return false;
}

const std::vector<VkLayerProperties>& VulkanInstanceProperties::getAvailableLayers() const {
    return m_availableLayers;
}

const std::vector<VkExtensionProperties>& VulkanInstanceProperties::getAvailableExtensions() const {
    return m_availableExtensions;
}

bool VulkanInstanceProperties::areValidationLayersAvailable() const {
    return m_validationLayersAvailable;
}
    
bool VulkanInstanceProperties::areDebugUtilsAvailable() const {
    return m_debugUtilsAvailable;
}
