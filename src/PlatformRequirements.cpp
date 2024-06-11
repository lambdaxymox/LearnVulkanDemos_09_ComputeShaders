#include "PlatformRequirements.h"

#include "platform.h"

#include <vulkan/vulkan.h>
#include <fmt/core.h>
#include <fmt/ostream.h>


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


PlatformRequirements::PlatformRequirements(const std::vector<std::string>& extensions, const std::vector<std::string>& layers)
    : m_instanceExtensions { extensions }
    , m_instanceLayers { layers }
{
}

const std::vector<std::string>& PlatformRequirements::getExtensions() const {
        return m_instanceExtensions;
}
 
const std::vector<std::string>& PlatformRequirements::getLayers() const {
    return m_instanceLayers;
}

bool PlatformRequirements::isEmpty() const {
    return m_instanceExtensions.empty() && m_instanceLayers.empty();
}


auto fmt::formatter<PlatformRequirements>::format(const PlatformRequirements& requirements, format_context& ctx) const -> format_context::iterator {
    return fmt::format_to(
        ctx.out(),
        "PlatformRequirements {{ instanceExtensions: {}, instanceLayers: {} }}",
        requirements.getExtensions(), 
        requirements.getLayers()
    );
}


PlatformRequirementsBuilder::PlatformRequirementsBuilder() {
    if (platform::detectOperatingSystem() == platform::Platform::Apple) {
        m_instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        m_instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
    }
}

PlatformRequirementsBuilder& PlatformRequirementsBuilder::requireExtension(const std::string& extensionName) {
    m_instanceExtensions.push_back(extensionName);
        
    return *this;
}

PlatformRequirementsBuilder& PlatformRequirementsBuilder::requireLayer(const std::string& layerName) {
    m_instanceLayers.push_back(layerName);
        
    return *this;
}

PlatformRequirementsBuilder& PlatformRequirementsBuilder::requireDebuggingExtensions() {
    m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return *this;
}

PlatformRequirementsBuilder& PlatformRequirementsBuilder::requireValidationLayers() {
    m_instanceLayers.push_back(platform::VK_LAYER_KHRONOS_validation);
    m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

    return *this;
}

PlatformRequirementsBuilder& PlatformRequirementsBuilder::includeFrom(const PlatformRequirements& other) {
    m_instanceExtensions.insert(
        m_instanceExtensions.end(), 
        other.getExtensions().begin(), 
        other.getExtensions().end()
    );
    m_instanceLayers.insert(
        m_instanceLayers.end(), 
        other.getLayers().begin(), 
        other.getLayers().end()
    );
        
    return *this;
}

PlatformRequirements PlatformRequirementsBuilder::build() const {
    // TODO: Check for move semantics.
    return PlatformRequirements(m_instanceExtensions, m_instanceLayers);
}
