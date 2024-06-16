#ifndef VULKAN_INSTANCE_REQUIREMENTS_BUILDER_H
#define VULKAN_INSTANCE_REQUIREMENTS_BUILDER_H

#include "vulkan_instance_requirements.h"


namespace VulkanEngine {

namespace VulkanPlatform {


class VulkanInstanceRequirementsBuilder final {
public:
    explicit VulkanInstanceRequirementsBuilder();

    VulkanInstanceRequirementsBuilder& requireExtension(const std::string& extensionName);

    VulkanInstanceRequirementsBuilder& requireLayer(const std::string& layerName);

    VulkanInstanceRequirementsBuilder& requireDebuggingExtensions();

    VulkanInstanceRequirementsBuilder& requireValidationLayers();

    VulkanInstanceRequirementsBuilder& includeFrom(const VulkanInstanceRequirements& other);

    VulkanInstanceRequirements build() const;
private:
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;
};


}

}

#endif // VULKAN_INSTANCE_REQUIREMENTS_BUILDER_H
