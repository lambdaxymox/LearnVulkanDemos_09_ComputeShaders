#include "vulkan_instance_requirements_builder.h"
#include "constants.h"

#include <vulkan/vulkan.h>


using VulkanInstanceRequirements = VulkanEngine::VulkanPlatform::VulkanInstanceRequirements;
using VulkanInstanceRequirementsBuilder = VulkanEngine::VulkanPlatform::VulkanInstanceRequirementsBuilder;


VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireExtension(const std::string& extensionName) {
    m_instanceExtensions.push_back(extensionName);
        
    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireLayer(const std::string& layerName) {
    m_instanceLayers.push_back(layerName);
        
    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requirePortabilityExtensions() {
    m_instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
    m_instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);

    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireDebuggingExtensions() {
    m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return *this;
}

VulkanInstanceRequirementsBuilder& VulkanInstanceRequirementsBuilder::requireValidationLayers() {
    m_instanceLayers.push_back(VulkanEngine::Constants::VK_LAYER_KHRONOS_validation);

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
