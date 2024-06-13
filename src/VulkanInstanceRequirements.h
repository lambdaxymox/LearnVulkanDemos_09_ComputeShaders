#ifndef VULKAN_INSTANCE_REQUIREMENTS_H
#define VULKAN_INSTANCE_REQUIREMENTS_H

#include <vector>
#include <string>
#include <fmt/core.h>


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

class VulkanInstanceRequirementsBuilder final {
public:
    explicit VulkanInstanceRequirementsBuilder();

    VulkanInstanceRequirementsBuilder& requireExtension(const std::string& extensionName);

    VulkanInstanceRequirementsBuilder& requireLayer(const std::string& layerName);

    VulkanInstanceRequirementsBuilder& requireDebuggingExtensions();

    VulkanInstanceRequirementsBuilder& requireValidationLayers();

    VulkanInstanceRequirementsBuilder& includeFrom(const VulkanInstanceRequirements& other);

    VulkanInstanceRequirements build() const;
private:
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;
};

template <> struct fmt::formatter<VulkanInstanceRequirements>: fmt::formatter<string_view> {
    auto format(const VulkanInstanceRequirements& requirements, format_context& ctx) const -> format_context::iterator;
};

#endif /* VULKAN_INSTANCE_REQUIREMENTS_H */
