#ifndef VK_PLATFORM_H
#define VK_PLATFORM_H

#include <string>


namespace platform {

extern const std::string VK_LAYER_KHRONOS_validation;
extern const std::string VK_KHR_portability_subset;

enum class Platform {
    Apple,
    Linux,
    Windows,
    Unknown,
};

constexpr Platform detectOperatingSystem() {
#if defined(__APPLE__) || defined(__MACH__)
    return Platform::Apple;
#elif defined(__LINUX__)
    return Platform::Linux;
#elif defined(_WIN32)
    return Platform::Windows;
#else
    return Platform::Unknown;
#endif
}

}

#endif /* VK_PLATFORM_H */
