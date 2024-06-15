#include "physical_device_requirements.h"
#include "operating_system.h"
#include "constants.h"


using PhysicalDeviceRequirements = VulkanEngine::VulkanPlatform::PhysicalDeviceRequirements;


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

