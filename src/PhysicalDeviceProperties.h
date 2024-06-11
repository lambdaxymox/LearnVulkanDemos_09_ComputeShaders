#ifndef PHYSICAL_DEVICE_PROPERTIES_H
#define PHYSICAL_DEVICE_PROPERTIES_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <fmt/core.h>


class PhysicalDeviceProperties final {
private:
    std::vector<VkExtensionProperties> m_deviceExtensions;
public:
    explicit PhysicalDeviceProperties(std::vector<VkExtensionProperties> deviceExtensions);

    const std::vector<VkExtensionProperties>& getExtensions() const;
};

template <> struct fmt::formatter<PhysicalDeviceProperties>: fmt::formatter<string_view> {
    auto format(const PhysicalDeviceProperties& deviceProperties, format_context& ctx) const -> format_context::iterator;
};

#endif /* PHYSICAL_DEVICE_PROPERTIES_H */
