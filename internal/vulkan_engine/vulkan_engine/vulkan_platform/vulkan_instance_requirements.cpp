#include "vulkan_instance_requirements.h"

#include <vulkan/vulkan.h>
#include <fmt/core.h>
#include <fmt/ostream.h>


using VulkanInstanceRequirements = VulkanEngine::VulkanPlatform::VulkanInstanceRequirements;

VulkanInstanceRequirements::VulkanInstanceRequirements(const std::vector<std::string>& extensions, const std::vector<std::string>& layers)
    : m_instanceExtensions { extensions }
    , m_instanceLayers { layers }
{
}

const std::vector<std::string>& VulkanInstanceRequirements::getExtensions() const {
        return m_instanceExtensions;
}
 
const std::vector<std::string>& VulkanInstanceRequirements::getLayers() const {
    return m_instanceLayers;
}

bool VulkanInstanceRequirements::isEmpty() const {
    return m_instanceExtensions.empty() && m_instanceLayers.empty();
}
