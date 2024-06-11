#ifndef PLATFORM_REQUIREMENTS_H
#define PLATFORM_REQUIREMENTS_H

#include <vector>
#include <string>
#include <fmt/core.h>


class PlatformRequirements final {
private:
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;
public:
    explicit PlatformRequirements() = default;
    explicit PlatformRequirements(const std::vector<std::string>& extensions, const std::vector<std::string>& layers);

    const std::vector<std::string>& getExtensions() const;
 
    const std::vector<std::string>& getLayers() const;

    bool isEmpty() const;
};

class PlatformRequirementsBuilder final {
private:
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;
public:
    explicit PlatformRequirementsBuilder();

    PlatformRequirementsBuilder& requireExtension(const std::string& extensionName);

    PlatformRequirementsBuilder& requireLayer(const std::string& layerName);

    PlatformRequirementsBuilder& requireDebuggingExtensions();

    PlatformRequirementsBuilder& requireValidationLayers();

    PlatformRequirementsBuilder& includeFrom(const PlatformRequirements& other);

    PlatformRequirements build() const;
};

template <> struct fmt::formatter<PlatformRequirements>: fmt::formatter<string_view> {
    auto format(const PlatformRequirements& requirements, format_context& ctx) const -> format_context::iterator;
};

#endif /* PLATFORM_REQUIREMENTS_H */
