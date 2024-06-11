#ifndef PHYSICAL_DEVICE_REQUIREMENTS_H
#define PHYSICAL_DEVICE_REQUIREMENTS_H

#include <vulkan/vulkan.h>
#include <vector>
#include <string>
#include <string_view>
#include <fmt/core.h>


class PhysicalDeviceRequirements final {
private:
    std::vector<std::string> m_deviceExtensions;
public:
    explicit PhysicalDeviceRequirements(std::vector<std::string> deviceExtensions);

    const std::vector<std::string>& getExtensions() const;

    bool isEmpty() const;
};

template <> struct fmt::formatter<PhysicalDeviceRequirements>: fmt::formatter<string_view> {
    auto format(const PhysicalDeviceRequirements& deviceRequirements, format_context& ctx) const -> format_context::iterator;
};

class PhysicalDeviceRequirementsBuilder final {
private:
    std::vector<std::string> m_deviceExtensions;
public:
    explicit PhysicalDeviceRequirementsBuilder();

    PhysicalDeviceRequirementsBuilder& requireExtension(const std::string& extensionName);

    PhysicalDeviceRequirements build() const;
};

#endif
