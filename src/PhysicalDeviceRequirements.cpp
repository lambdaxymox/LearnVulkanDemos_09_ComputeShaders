#include "PhysicalDeviceRequirements.h"

#include "platform.h"


template <> struct fmt::formatter<std::vector<std::string>> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.end();
    };

    auto format(const std::vector<std::string>& vec, format_context& ctx) const -> format_context::iterator {
        auto appender = fmt::format_to(ctx.out(), "{}", "[");
        if (!vec.empty()) {
            auto lastItem = vec.end() - 1;
            for (auto item = vec.begin(); item != lastItem; item++) {
                fmt::format_to(appender, "\"{}\", ", *item);
            }
            fmt::format_to(appender, "\"{}\"", *lastItem);
        }
        fmt::format_to(appender, "{}", "]");

        return appender;
    }
};

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

auto fmt::formatter<PhysicalDeviceRequirements>::format(const PhysicalDeviceRequirements& deviceRequirements, format_context& ctx) const -> format_context::iterator {
    return fmt::format_to(
        ctx.out(),
        "PhysicalDeviceRequirements {{ deviceExtensions: {} }}",
        deviceRequirements.getExtensions()
    );
}

PhysicalDeviceRequirements::PhysicalDeviceRequirements(std::vector<std::string> deviceExtensions) 
    : m_deviceExtensions { deviceExtensions }
{
}

const std::vector<std::string>& PhysicalDeviceRequirements::getExtensions() const {
    return m_deviceExtensions;
}

bool PhysicalDeviceRequirements::isEmpty() const {
    return m_deviceExtensions.empty();
}



PhysicalDeviceRequirementsBuilder::PhysicalDeviceRequirementsBuilder() {
    // https://stackoverflow.com/questions/66659907/vulkan-validation-warning-catch-22-about-vk-khr-portability-subset-on-moltenvk
    if (platform::detectOperatingSystem() == platform::Platform::Apple) {
        m_deviceExtensions.emplace_back(platform::VK_KHR_portability_subset);
    }
}

PhysicalDeviceRequirementsBuilder& PhysicalDeviceRequirementsBuilder::requireExtension(const std::string& extensionName) {
    m_deviceExtensions.emplace_back(extensionName);
        
    return *this;
}

PhysicalDeviceRequirements PhysicalDeviceRequirementsBuilder::build() const {
    // TODO: Check move semantics.
    return PhysicalDeviceRequirements(m_deviceExtensions);
}
