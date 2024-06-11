/*
const char* VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";
const char* VK_KHR_portability_subset = "VK_KHR_portability_subset";

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
*/

#include "platform.h"

const char* platform::VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";
const char* platform::VK_KHR_portability_subset = "VK_KHR_portability_subset";


