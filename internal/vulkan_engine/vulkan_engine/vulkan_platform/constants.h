#ifndef CONSTANTS_H
#define CONSTANTS_H

#include <string>

namespace VulkanEngine {

namespace Constants {


const std::string VK_LAYER_KHRONOS_validation = std::string { "VK_LAYER_KHRONOS_validation" };
const std::string VK_KHR_portability_subset = std::string { "VK_KHR_portability_subset" };


}

class PhysicalDeviceProperties final {
    public:
        explicit PhysicalDeviceProperties(std::vector<VkExtensionProperties> deviceExtensions);

        const std::vector<VkExtensionProperties>& getExtensions() const;
    private:
        std::vector<VkExtensionProperties> m_deviceExtensions;
};





}

#endif // CONSTANTS_H
