#ifndef VULKAN_INSTANCE_PROPERTIES_H
#define VULKAN_INSTANCE_PROPERTIES_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>


namespace VulkanEngine {

namespace VulkanPlatform {


class VulkanInstanceProperties final {
public:
    explicit VulkanInstanceProperties(std::vector<VkLayerProperties> availableLayers, std::vector<VkExtensionProperties> availableExtensions);
    
    bool isExtensionAvailable(const char* layerName) const;

    bool isLayerAvailable(const char* extensionName) const;

    const std::vector<VkLayerProperties>& getAvailableLayers() const;

    const std::vector<VkExtensionProperties>& getAvailableExtensions() const;

    bool areValidationLayersAvailable() const;
    
    bool areDebugUtilsAvailable() const;
private:
    std::vector<VkLayerProperties> m_availableLayers;
    std::vector<VkExtensionProperties> m_availableExtensions;
    bool m_validationLayersAvailable = false;
	bool m_debugUtilsAvailable = false;
};


}

}

#endif /* VULKAN_INSTANCE_PROPERTIES_H */
