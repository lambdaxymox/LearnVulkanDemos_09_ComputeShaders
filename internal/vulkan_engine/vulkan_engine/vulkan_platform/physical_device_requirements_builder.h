#ifndef PHYSICAL_DEVICE_REQUIREMENTS_BUILDER_H
#define PHYSICAL_DEVICE_REQUIREMENTS_BUILDER_H

#include "physical_device_requirements.h"


namespace VulkanEngine {

namespace VulkanPlatform {


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

#endif /* PHYSICAL_DEVICE_REQUIREMENTS_BUILDER_H */
