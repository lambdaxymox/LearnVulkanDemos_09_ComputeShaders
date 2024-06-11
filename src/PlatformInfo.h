#ifndef PLATFORM_INFO_H
#define PLATFORM_INFO_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <fmt/core.h>


class PlatformInfo final {
private:
    std::vector<VkLayerProperties> m_availableLayers;
    std::vector<VkExtensionProperties> m_availableExtensions;
    bool m_validationLayersAvailable = false;
	bool m_debugUtilsAvailable = false;
public:
    explicit PlatformInfo(std::vector<VkLayerProperties> availableLayers, std::vector<VkExtensionProperties> availableExtensions);
    
    bool isExtensionAvailable(const char* layerName) const;

    bool isLayerAvailable(const char* extensionName) const;

    const std::vector<VkLayerProperties>& getAvailableLayers() const;

    const std::vector<VkExtensionProperties>& getAvailableExtensions() const;

    bool areValidationLayersAvailable() const;
    
    bool areDebugUtilsAvailable() const;
};

template <> struct fmt::formatter<PlatformInfo>: fmt::formatter<string_view> {
    auto format(const PlatformInfo& platformInfo, format_context& ctx) const -> format_context::iterator;
};

#endif
