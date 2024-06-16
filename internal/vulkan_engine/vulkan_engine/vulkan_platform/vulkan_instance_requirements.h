#ifndef VULKAN_INSTANCE_REQUIREMENTS_H
#define VULKAN_INSTANCE_REQUIREMENTS_H

#include <vector>
#include <string>


namespace VulkanEngine {

namespace VulkanPlatform {


class VulkanInstanceRequirements final {
public:
    explicit VulkanInstanceRequirements() = default;
    explicit VulkanInstanceRequirements(const std::vector<std::string>& extensions, const std::vector<std::string>& layers);

    const std::vector<std::string>& getExtensions() const;
 
    const std::vector<std::string>& getLayers() const;

    bool isEmpty() const;
private:
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;
};


}

}

#endif // VULKAN_INSTANCE_REQUIREMENTS_H
