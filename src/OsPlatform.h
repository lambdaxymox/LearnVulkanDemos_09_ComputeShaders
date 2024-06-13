#ifndef OS_PLATFORM_H
#define OS_PLATFORM_H

namespace Os {

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

#endif /* OS_PLATFORM_H */
