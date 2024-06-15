#ifndef PHYSICAL_DEVICE_REQUIREMENTS_H
#define PHYSICAL_DEVICE_REQUIREMENTS_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>


namespace VulkanEngine {

namespace VulkanPlatform {


class PhysicalDeviceRequirements final {
public:
    explicit PhysicalDeviceRequirements(std::vector<std::string> deviceExtensions);

    const std::vector<std::string>& getExtensions() const;

    bool isEmpty() const;
private:
    std::vector<std::string> m_deviceExtensions;
};

class PhysicalDeviceRequirementsBuilder final {
public:
    explicit PhysicalDeviceRequirementsBuilder();

    PhysicalDeviceRequirementsBuilder& requireExtension(const std::string& extensionName);

    PhysicalDeviceRequirements build() const;
private:
    std::vector<std::string> m_deviceExtensions;
};


}

}

#endif /* PHYSICAL_DEVICE_REQUIREMENTS_H */
