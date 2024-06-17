#include "constants.h"
#include "operating_system.h"
#include "physical_device_requirements_builder.h"
#include "physical_device_requirements.h"


using PhysicalDeviceRequirements = VulkanEngine::VulkanPlatform::PhysicalDeviceRequirements;
using PhysicalDeviceRequirementsBuilder = VulkanEngine::VulkanPlatform::PhysicalDeviceRequirementsBuilder;


PhysicalDeviceRequirementsBuilder::PhysicalDeviceRequirementsBuilder() {}

PhysicalDeviceRequirementsBuilder& PhysicalDeviceRequirementsBuilder::requireExtension(const std::string& extensionName) {
    m_deviceExtensions.emplace_back(extensionName);
        
    return *this;
}

PhysicalDeviceRequirements PhysicalDeviceRequirementsBuilder::build() const {
    // TODO: Check move semantics.
    return PhysicalDeviceRequirements(m_deviceExtensions);
}
