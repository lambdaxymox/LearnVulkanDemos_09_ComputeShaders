#include "physical_device_properties.h"


using PhysicalDeviceProperties = VulkanEngine::VulkanPlatform::PhysicalDeviceProperties;

PhysicalDeviceProperties::PhysicalDeviceProperties(std::vector<VkExtensionProperties> deviceExtensions)
    : m_deviceExtensions { deviceExtensions } 
{
}

const std::vector<VkExtensionProperties>& PhysicalDeviceProperties::getExtensions() const {
    return m_deviceExtensions;
}
