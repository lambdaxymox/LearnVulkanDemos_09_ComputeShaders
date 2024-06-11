#include "PlatformInfo.h"
#include "platform.h"


template <> struct fmt::formatter<VkLayerProperties> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.end();
    };
    
    auto format(const VkLayerProperties& layerProperties, format_context& ctx) const -> format_context::iterator {
        auto specVariant = VK_API_VERSION_VARIANT(layerProperties.specVersion);
        auto specMajor = VK_API_VERSION_MAJOR(layerProperties.specVersion);
        auto specMinor = VK_API_VERSION_MINOR(layerProperties.specVersion);
        auto specPatch = VK_API_VERSION_PATCH(layerProperties.specVersion);
        auto implVariant = VK_API_VERSION_VARIANT(layerProperties.implementationVersion);
        auto implMajor = VK_API_VERSION_MAJOR(layerProperties.implementationVersion);
        auto implMinor = VK_API_VERSION_MINOR(layerProperties.implementationVersion);
        auto implPatch = VK_API_VERSION_PATCH(layerProperties.implementationVersion);
        std::string_view layerName { layerProperties.layerName, sizeof(layerProperties.layerName) };
        std::string_view description { layerProperties.description, sizeof(layerProperties.description) };

        auto appender = fmt::format_to(ctx.out(), "{}", "VkLayerProperties {{ ");
        fmt::format_to(appender, "layerName: \"{}\", ", layerName);
        fmt::format_to(appender, "specVersion: {}.{}.{}.{}, ", specVariant, specMajor, specMinor, specPatch);
        fmt::format_to(appender, "implementationVersion: {}.{}.{}.{}, ", implVariant, implMajor, implMinor, implPatch);
        fmt::format_to(appender, "description: \"{}\"", description);
        fmt::format_to(ctx.out(), "{}", " }}");

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

template <> struct fmt::formatter<std::vector<VkLayerProperties>> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.end();
    };

    auto format(const std::vector<VkLayerProperties>& vec, format_context& ctx) const -> format_context::iterator {
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

template <> struct fmt::formatter<std::vector<VkExtensionProperties>> {
    constexpr auto parse(format_parse_context& ctx) -> format_parse_context::iterator {
        return ctx.end();
    };

    auto format(const std::vector<VkExtensionProperties>& vec, format_context& ctx) const -> format_context::iterator {
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


PlatformInfo::PlatformInfo(std::vector<VkLayerProperties> availableLayers, std::vector<VkExtensionProperties> availableExtensions) 
    : m_availableLayers { availableLayers }
    , m_availableExtensions { availableExtensions }
{
    for (const auto& layerProperties : availableLayers) {
        if (strcmp(layerProperties.layerName, platform::VK_LAYER_KHRONOS_validation) == 0) {
            m_validationLayersAvailable = true;
        }
    }

    for (const auto& extensionProperties : availableExtensions) {
        if (strcmp(extensionProperties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
            m_debugUtilsAvailable = true;
        }
    }
}
    
bool PlatformInfo::isExtensionAvailable(const char* layerName) const {
    for (const auto& layerProperties : m_availableLayers) {
        if (strcmp(layerProperties.layerName, layerName) == 0) {
            return true;
        }
    }

    return false;
}

bool PlatformInfo::isLayerAvailable(const char* extensionName) const {
    for (const auto& extensionProperties : m_availableExtensions) {
        if (strcmp(extensionProperties.extensionName, extensionName) == 0) {
            return true;
        }
    }

    return false;
}

const std::vector<VkLayerProperties>& PlatformInfo::getAvailableLayers() const {
    return m_availableLayers;
}

const std::vector<VkExtensionProperties>& PlatformInfo::getAvailableExtensions() const {
    return m_availableExtensions;
}

bool PlatformInfo::areValidationLayersAvailable() const {
    return m_validationLayersAvailable;
}
    
bool PlatformInfo::areDebugUtilsAvailable() const {
    return m_debugUtilsAvailable;
}

auto fmt::formatter<PlatformInfo>::format(const PlatformInfo& platformInfo, format_context& ctx) const -> format_context::iterator {
    return fmt::format_to(
        ctx.out(),
        "PlatformInfo {{ availableLayers: {}, availableExtensions: {} }}",
        platformInfo.getAvailableLayers(), 
        platformInfo.getAvailableExtensions()
    );
}
