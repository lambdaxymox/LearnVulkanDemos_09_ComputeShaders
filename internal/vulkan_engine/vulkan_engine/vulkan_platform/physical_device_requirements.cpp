#include "physical_device_requirements.h"
#include "operating_system.h"
#include "constants.h"


using PhysicalDeviceRequirements = VulkanEngine::VulkanPlatform::PhysicalDeviceRequirements;
using PhysicalDeviceRequirementsBuilder = VulkanEngine::VulkanPlatform::PhysicalDeviceRequirementsBuilder;


PhysicalDeviceRequirements::PhysicalDeviceRequirements(std::vector<std::string> deviceExtensions) 
    : m_deviceExtensions { deviceExtensions }
{
}

const std::vector<std::string>& PhysicalDeviceRequirements::getExtensions() const {
    return m_deviceExtensions;
}

bool PhysicalDeviceRequirements::isEmpty() const {
    return m_deviceExtensions.empty();
}



PhysicalDeviceRequirementsBuilder::PhysicalDeviceRequirementsBuilder() {
    // https://stackoverflow.com/questions/66659907/vulkan-validation-warning-catch-22-about-vk-khr-portability-subset-on-moltenvk
    if (VulkanPlatform::detectOperatingSystem() == VulkanPlatform::Platform::Apple) {
        m_deviceExtensions.emplace_back(VulkanPlatform::VK_KHR_portability_subset);
    }
}

PhysicalDeviceRequirementsBuilder& PhysicalDeviceRequirementsBuilder::requireExtension(const std::string& extensionName) {
    m_deviceExtensions.emplace_back(extensionName);
        
    return *this;
}

PhysicalDeviceRequirements PhysicalDeviceRequirementsBuilder::build() const {
    // TODO: Check move semantics.
    return PhysicalDeviceRequirements(m_deviceExtensions);
}
