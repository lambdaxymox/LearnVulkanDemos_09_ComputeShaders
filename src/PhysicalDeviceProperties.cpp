#include "PhysicalDeviceProperties.h"


PhysicalDeviceProperties::PhysicalDeviceProperties(std::vector<VkExtensionProperties> deviceExtensions)
    : m_deviceExtensions { deviceExtensions } 
{
}

const std::vector<VkExtensionProperties>& PhysicalDeviceProperties::getExtensions() const {
    return m_deviceExtensions;
}


template <> struct fmt::formatter<VkExtensionProperties> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.end();
    };

    auto format(const VkExtensionProperties& extensionProperties, format_context& ctx) const -> format_context::iterator {
        auto specVariant = VK_API_VERSION_VARIANT(extensionProperties.specVersion);
        auto specMajor = VK_API_VERSION_MAJOR(extensionProperties.specVersion);
        auto specMinor = VK_API_VERSION_MINOR(extensionProperties.specVersion);
        auto specPatch = VK_API_VERSION_PATCH(extensionProperties.specVersion);
        std::string_view extensionName { extensionProperties.extensionName, sizeof(extensionProperties.extensionName) };

        auto appender = fmt::format_to(ctx.out(), "{}", "VkExtensionProperties {{ ");
        fmt::format_to(appender, "extensionName: \"{}\", ", extensionName);
        fmt::format_to(appender, "specVersion: {}.{}.{}.{}", specVariant, specMajor, specMinor, specPatch);
        fmt::format_to(ctx.out(), "{}", " }}");
        
        return appender;
    }
};

auto fmt::formatter<PhysicalDeviceProperties>::format(const PhysicalDeviceProperties& deviceProperties, format_context& ctx) const -> format_context::iterator {
    return fmt::format_to(
        ctx.out(),
        "PhysicalDeviceProperties {{ deviceExtensions: {} }}",
        deviceProperties.getExtensions()
    );
}
