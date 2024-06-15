#ifndef PHYSICAL_DEVICE_PROPERTIES_H
#define PHYSICAL_DEVICE_PROPERTIES_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>


namespace VulkanEngine {

namespace VulkanPlatform {


class PhysicalDeviceProperties final {
public:
    explicit PhysicalDeviceProperties(std::vector<VkExtensionProperties> deviceExtensions);

    const std::vector<VkExtensionProperties>& getExtensions() const;
private:
    std::vector<VkExtensionProperties> m_deviceExtensions;
};


}

}

#endif /* PHYSICAL_DEVICE_PROPERTIES_H */
