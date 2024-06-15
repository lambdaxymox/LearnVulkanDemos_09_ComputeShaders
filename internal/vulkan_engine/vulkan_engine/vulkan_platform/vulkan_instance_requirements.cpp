#include "vulkan_instance_requirements.h"
#include "operating_system.h"
#include "constants.h"

#include <vulkan/vulkan.h>
#include <fmt/core.h>
#include <fmt/ostream.h>


using VulkanInstanceRequirements = VulkanEngine::VulkanPlatform::VulkanInstanceRequirements;
using VulkanInstanceRequirementsBuilder = VulkanEngine::VulkanPlatform::VulkanInstanceRequirementsBuilder;

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





VulkanInstanceRequirementsBuilder::VulkanInstanceRequirementsBuilder() {
    if (VulkanEngine::VulkanPlatform::detectOperatingSystem() == VulkanEngine::VulkanPlatform::Platform::Apple) {
        m_instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        m_instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireExtension(const std::string& extensionName) {
    m_instanceExtensions.push_back(extensionName);
        
    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireLayer(const std::string& layerName) {
    m_instanceLayers.push_back(layerName);
        
    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireDebuggingExtensions() {
    m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireValidationLayers() {
    m_instanceLayers.push_back(VulkanEngine::VulkanPlatform::VK_LAYER_KHRONOS_validation);
    m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::includeFrom(const VulkanInstanceRequirements& other) {
    m_instanceExtensions.insert(
        m_instanceExtensions.end(), 
        other.getExtensions().begin(), 
        other.getExtensions().end()
    );
    m_instanceLayers.insert(
        m_instanceLayers.end(), 
        other.getLayers().begin(), 
        other.getLayers().end()
    );
        
    return *this;
}

VulkanInstanceRequirements VulkanInstanceRequirementsBuilder::build() const {
    // TODO: Check for move semantics.
    return VulkanInstanceRequirements(m_instanceExtensions, m_instanceLayers);
}
