#include <vulkan/vulkan.h>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>
#include <optional>
#include <set>
#include <cstdint>
#include <limits>
#include <algorithm>
#include <fstream>

#include <fmt/core.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const char* VK_LAYER_KHRONOS_validation = "VK_LAYER_KHRONOS_validation";
const char* VK_KHR_portability_subset = "VK_KHR_portability_subset";

const std::vector<std::string> validationLayers = { 
    VK_LAYER_KHRONOS_validation
};

const std::vector<const char*> deviceExtensions = {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
#else
const bool enableValidationLayers = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;


namespace vk_platform {
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

VkResult CreateDebugUtilsMessengerEXT(
    VkInstance instance, 
    const VkDebugUtilsMessengerCreateInfoEXT* pCreateInfo, 
    const VkAllocationCallbacks* pAllocator, 
    VkDebugUtilsMessengerEXT* pDebugMessenger
) {
    auto func = (PFN_vkCreateDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
    if (func != nullptr) {
        return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
    } else {
        return VK_ERROR_EXTENSION_NOT_PRESENT;
    }
}

void DestroyDebugUtilsMessengerEXT(
    VkInstance instance, 
    VkDebugUtilsMessengerEXT debugMessenger, 
    const VkAllocationCallbacks* pAllocator
) {
    auto func = (PFN_vkDestroyDebugUtilsMessengerEXT) vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT");
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

std::vector<const char*> convertToCStrings(const std::vector<std::string>& strings) {
    std::vector<const char*> cStrings;
    cStrings.reserve(strings.size());
    std::transform(
        strings.begin(), 
        strings.end(), 
        std::back_inserter(cStrings),
        [](const std::string& str) { return str.c_str(); }
    );
    
    return cStrings;
}

class PlatformInfo {
private:
    std::vector<VkLayerProperties> _availableLayers;
    std::vector<VkExtensionProperties> _availableExtensions;
    bool _validationLayersAvailable = false;
	bool _debugUtilsAvailable = false;
public:
    explicit PlatformInfo(std::vector<VkLayerProperties> availableLayers, std::vector<VkExtensionProperties> availableExtensions) 
        : _availableLayers { availableLayers }, _availableExtensions { availableExtensions }
    {
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerProperties.layerName, VK_LAYER_KHRONOS_validation) == 0) {
                _validationLayersAvailable = true;
            }
        }

        for (const auto& extensionProperties : availableExtensions) {
            if (strcmp(extensionProperties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                _debugUtilsAvailable = true;
            }
        }
    }
    
    bool isExtensionAvailable(const char* layerName) const {
        for (const auto& layerProperties : _availableLayers) {
            if (strcmp(layerProperties.layerName, layerName) == 0) {
                return true;
            }
        }

        return false;
    }

    bool isLayerAvailable(const char* extensionName) const {
        for (const auto& extensionProperties : _availableExtensions) {
            if (strcmp(extensionProperties.extensionName, extensionName) == 0) {
                return true;
            }
        }

        return false;
    }

    const std::vector<VkLayerProperties>& getAvailableLayers() const {
        return _availableLayers;
    }

    const std::vector<VkExtensionProperties>& getAvailableExtensions() const {
        return _availableExtensions;
    }

    bool areValidationLayersAvailable() const {
        return _validationLayersAvailable;
    }
    
    bool areDebugUtilsAvailable() const {
        return _debugUtilsAvailable;
    }
};

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

template <> struct fmt::formatter<PlatformInfo>: fmt::formatter<string_view> {
    auto format(const PlatformInfo& platformInfo, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(
            ctx.out(),
            "PlatformInfo {{ availableLayers: {}, availableExtensions: {} }}",
            platformInfo.getAvailableLayers(), 
            platformInfo.getAvailableExtensions()
        );
    }
};

class PlatformRequirements {
private:
    std::vector<std::string> instanceExtensions;
    std::vector<std::string> instanceLayers;

public:
    explicit PlatformRequirements(const std::vector<std::string>& extensions, const std::vector<std::string>& layers)
        : instanceExtensions { extensions }, instanceLayers{ layers }
    {
    }

    const std::vector<std::string>& getExtensions() const {
        return instanceExtensions;
    }
 
    const std::vector<std::string>& getLayers() const {
        return instanceLayers;
    }

    bool isEmpty() {
        return instanceExtensions.empty() && instanceLayers.empty();
    }
};

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

template <> struct fmt::formatter<PlatformRequirements>: fmt::formatter<string_view> {
    auto format(const PlatformRequirements& requirements, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(
            ctx.out(),
            "PlatformRequirements {{ instanceExtensions: {}, instanceLayers: {} }}",
            requirements.getExtensions(), 
            requirements.getLayers()
        );
    }
};

class PlatformRequirementsBuilder {
private:
    std::vector<std::string> instanceExtensions;
    std::vector<std::string> instanceLayers;
public:
    explicit PlatformRequirementsBuilder() {
        if (vk_platform::detectOperatingSystem() == vk_platform::Platform::Apple) {
            instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
    }

    PlatformRequirementsBuilder& requireExtension(const std::string& extensionName) {
        instanceExtensions.push_back(extensionName);
        
        return *this;
    }

    PlatformRequirementsBuilder& requireLayer(const std::string& layerName) {
        instanceLayers.push_back(layerName);
        
        return *this;
    }

    PlatformRequirementsBuilder& requireDebuggingExtensions() {
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return *this;
    }

    PlatformRequirementsBuilder& requireValidationLayers() {
        instanceLayers.push_back(VK_LAYER_KHRONOS_validation);
        instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return *this;
    }

    PlatformRequirementsBuilder& includeFrom(const PlatformRequirements& other) {
        instanceExtensions.insert(
            instanceExtensions.end(), 
            other.getExtensions().begin(), 
            other.getExtensions().end()
        );
        instanceLayers.insert(
            instanceLayers.end(), 
            other.getLayers().begin(), 
            other.getLayers().end()
        );
        
        return *this;
    }

    PlatformRequirements build() const {
        // TODO: Check for move semantics.
        return PlatformRequirements(instanceExtensions, instanceLayers);
    }
};

using MissingPlatformRequirements = PlatformRequirements;

class PlatformInfoOps {
public:
    static PlatformInfo getPlatformInfo() {
        const auto availableLayers = getAvailableVulkanInstanceLayers();
        const auto availableExtensions = getAvailableVulkanInstanceExtensions();
        
        PlatformInfo platformInfo { availableLayers, availableExtensions };

        return platformInfo;
    }

    static MissingPlatformRequirements detectMissingInstanceRequirements(
        const PlatformInfo& platformInfo,
        const PlatformRequirements& platformRequirements
    ) {
        std::vector<std::string> missingExtensionNames {};
        auto availableExtensions = platformInfo.getAvailableExtensions();
        for (const auto& extensionName : platformRequirements.getExtensions()) {
            auto found = std::find_if(
                std::begin(availableExtensions),
                std::end(availableExtensions),
                [extensionName](const auto& extension) {
                    return strcmp(extensionName.data(), extension.extensionName) == 0;
                }
            );
            auto extensionNotFound = (found == std::end(availableExtensions));
            if (extensionNotFound) {
                missingExtensionNames.emplace_back(extensionName);
            }
        }

        std::vector<std::string> missingLayerNames {};
        auto availableLayers = platformInfo.getAvailableLayers();
        for (const auto& layerName : platformRequirements.getLayers()) {
            auto found = std::find_if(
                std::begin(availableLayers),
                std::end(availableLayers),
                [layerName](const auto& layer) {
                    return strcmp(layerName.data(), layer.layerName) == 0;
                }
            );
            auto layerNotFound = (found == std::end(availableLayers));
            if (layerNotFound) {
                missingLayerNames.emplace_back(layerName);
            }
        }

        return MissingPlatformRequirements(missingExtensionNames, missingLayerNames);
    }
private:
    static std::vector<VkLayerProperties> getAvailableVulkanInstanceLayers() {
        std::vector<VkLayerProperties> instanceLayerProperties {};
        uint32_t numInstanceExtensions = 0;
        vkEnumerateInstanceLayerProperties(&numInstanceExtensions, nullptr);
        if (numInstanceExtensions > 0) {
            instanceLayerProperties.resize(numInstanceExtensions);
            vkEnumerateInstanceLayerProperties(
                &numInstanceExtensions, 
                instanceLayerProperties.data()
            );
        }

        return instanceLayerProperties;
    }

    static std::vector<VkExtensionProperties> getAvailableVulkanInstanceExtensions() {
        std::vector<VkExtensionProperties> instanceExtensionProperties {};
        uint32_t numInstanceExtensions = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &numInstanceExtensions, nullptr);
        if (numInstanceExtensions > 0) {
            instanceExtensionProperties.resize(numInstanceExtensions);
            vkEnumerateInstanceExtensionProperties(
                nullptr, 
                &numInstanceExtensions, 
                instanceExtensionProperties.data()
            );
        }

        return instanceExtensionProperties;
    }
};

class PhysicalDeviceProperties {
private:
    std::vector<VkExtensionProperties> deviceExtensions;
public:
    explicit PhysicalDeviceProperties(std::vector<VkExtensionProperties> extensions) 
        : deviceExtensions { extensions } 
    {
    }

    const std::vector<VkExtensionProperties>& getExtensions() const {
        return deviceExtensions;
    }
};

template <> struct fmt::formatter<PhysicalDeviceProperties>: fmt::formatter<string_view> {
    auto format(const PhysicalDeviceProperties& deviceProperties, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(
            ctx.out(),
            "PhysicalDeviceProperties {{ deviceExtensions: {} }}",
            deviceProperties.getExtensions()
        );
    }
};

class PhysicalDeviceRequirements {
private:
    std::vector<std::string> deviceExtensions;
public:
    explicit PhysicalDeviceRequirements(std::vector<std::string> extensions) : deviceExtensions { extensions } {
    }

    const std::vector<std::string>& getExtensions() const {
        return deviceExtensions;
    }

    bool isEmpty() const {
        return deviceExtensions.empty();
    }
};

template <> struct fmt::formatter<PhysicalDeviceRequirements>: fmt::formatter<string_view> {
    auto format(const PhysicalDeviceRequirements& deviceRequirements, format_context& ctx) const -> format_context::iterator {
        return fmt::format_to(
            ctx.out(),
            "PhysicalDeviceRequirements {{ deviceExtensions: {} }}",
            deviceRequirements.getExtensions()
        );
    }
};

class PhysicalDeviceRequirementsBuilder {
private:
    std::vector<std::string> deviceExtensions;
public:
    explicit PhysicalDeviceRequirementsBuilder() {
        // https://stackoverflow.com/questions/66659907/vulkan-validation-warning-catch-22-about-vk-khr-portability-subset-on-moltenvk
        if (vk_platform::detectOperatingSystem() == vk_platform::Platform::Apple) {
            deviceExtensions.emplace_back(VK_KHR_portability_subset);
        }
    }

    PhysicalDeviceRequirementsBuilder& requireExtension(const std::string& extensionName) {
        deviceExtensions.emplace_back(extensionName);
        
        return *this;
    }

    PhysicalDeviceRequirements build() const {
        // TODO: Check move semantics.
        return PhysicalDeviceRequirements(deviceExtensions);
    }
};

using MissingPhysicalDeviceRequirements = PhysicalDeviceRequirements;

class PhysicalDeviceInfoOps {
public:
    static PhysicalDeviceProperties getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice) {
        std::vector<VkExtensionProperties> deviceExtensionProperties {};
        uint32_t numInstanceExtensions = 0;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &numInstanceExtensions, nullptr);
        if (numInstanceExtensions > 0) {
            deviceExtensionProperties.resize(numInstanceExtensions);
            vkEnumerateDeviceExtensionProperties(
                physicalDevice,
                nullptr, 
                &numInstanceExtensions, 
                deviceExtensionProperties.data()
            );
        }

        return PhysicalDeviceProperties(deviceExtensionProperties);
    }

    static MissingPhysicalDeviceRequirements detectMissingRequiredDeviceExtensions(
        const PhysicalDeviceProperties& physicalDeviceProperties,
        const PhysicalDeviceRequirements& physicalDeviceRequirements
    ) {
        std::vector<std::string> missingExtensionNames {};

        auto requiredExtensions = physicalDeviceRequirements.getExtensions();
        for (const auto& requiredExtension : requiredExtensions) {
            auto found = std::find_if(
                std::begin(requiredExtensions),
                std::end(requiredExtensions),
                [requiredExtension](const auto& extension) {
                    return strcmp(requiredExtension.data(), extension.data()) == 0;
                }
            );
            auto extensionNotFound = (found == std::end(requiredExtensions));
            if (extensionNotFound) {
                missingExtensionNames.emplace_back(requiredExtension);
            }
        }

        return PhysicalDeviceRequirements(missingExtensionNames);
    }
};

struct QueueFamilyIndices {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class HelloTriangleApplication {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }
private:
    VkInstance instance;
    VkDebugUtilsMessengerEXT debugMessenger;
    VkSurfaceKHR surface;
    VkDevice device;
    VkQueue presentQueue;
    VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkQueue graphicsQueue;

    VkCommandPool commandPool;
    std::vector<VkCommandBuffer> commandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkFence> inFlightFences;

    bool framebufferResized = false;

    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkExtent2D windowExtent { WIDTH, HEIGHT };
    GLFWwindow* m_window { nullptr };

    uint32_t currentFrame = 0;

    bool _isInitialized { false };
    bool _enableValidationLayers { false };

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<HelloTriangleApplication*>(glfwGetWindowUserPointer(window));
        app->framebufferResized = true;
    }

    void initVulkan() {
        createInstance();
        setupDebugMessenger();
        createSurface();
        choosePhysicalDevice();
        createLogicalDevice();
        createSwapChain();
        createImageViews();
        createRenderPass();
        createGraphicsPipeline();
        createFramebuffers();
        createCommandPool();
        createCommandBuffers();
        createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            drawFrame();
        }

        vkDeviceWaitIdle(device);
    }

    void cleanup() {
        if (_isInitialized) {
            cleanupSwapChain();

            vkDestroyPipeline(device, graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
            
            vkDestroyRenderPass(device, renderPass, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(device, inFlightFences[i], nullptr);
            }
            
            vkDestroyCommandPool(device, commandPool, nullptr);

            vkDestroyDevice(device, nullptr);

            if (enableValidationLayers) {
                DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
            }

            vkDestroySurfaceKHR(instance, surface, nullptr);
            vkDestroyInstance(instance, nullptr);

            glfwDestroyWindow(m_window);
            glfwTerminate();
        }
    }

    void initWindow() {
        glfwInit();

        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Hello Triangle", nullptr, nullptr);
        glfwSetWindowUserPointer(m_window, this);
        glfwSetFramebufferSizeCallback(m_window, framebufferResizeCallback);
    }

    PlatformRequirements getVulkanInstanceExtensionsRequiredBySDL() const {
        uint32_t requiredExtensionCount = 0;
        const char** requiredExtensionNames = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
        auto requiredExtensions = std::vector<std::string> {};
        for (int i = 0; i < requiredExtensionCount; i++) {
            requiredExtensions.emplace_back(std::string(requiredExtensionNames[i]));
        }

        auto builder = PlatformRequirementsBuilder();
        for (const auto& extensionName : requiredExtensions) {
            builder.requireExtension(extensionName);
        }

        return builder.build();
    }

    VkInstanceCreateFlags defaultInstanceCreateFlags() const {
        auto flags = 0;
        if (vk_platform::detectOperatingSystem() == vk_platform::Platform::Apple) {
            flags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }

        return flags;
    }

    PlatformRequirements getInstanceRequirements() const {
        auto vulkanExtensionsRequiredBySDL = getVulkanInstanceExtensionsRequiredBySDL();
        auto platformRequirements = PlatformRequirementsBuilder()
            .requireValidationLayers()
            .requireDebuggingExtensions()
            .includeFrom(vulkanExtensionsRequiredBySDL)
            .build();
        auto platformInfo = PlatformInfoOps::getPlatformInfo();

       return platformRequirements;
    }

    bool checkValidationLayerSupport() const {
        auto platformInfo = PlatformInfoOps::getPlatformInfo();

        return platformInfo.areValidationLayersAvailable();
    }

    PhysicalDeviceRequirements getDeviceRequirements(VkPhysicalDevice physicalDevice) {
        return PhysicalDeviceRequirementsBuilder()
            .requireExtension(VK_KHR_SWAPCHAIN_EXTENSION_NAME)
            .build();
    }

    std::vector<std::string> getEnabledLayerNames() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        auto enabledLayerCount = 0;
        std::vector<std::string> enabledLayerNames {};
        if (enableValidationLayers) {
            enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            enabledLayerNames = validationLayers;
        } else {
            enabledLayerCount = 0;
        }

        return enabledLayerNames;
    }

    void createInstance() {
        auto platformInfo = PlatformInfoOps::getPlatformInfo();    
        auto instanceRequirements = getInstanceRequirements();
        auto missingRequirements = PlatformInfoOps::detectMissingInstanceRequirements(
            platformInfo,
            instanceRequirements
        );
        if (!missingRequirements.isEmpty()) {
            std::string errorMessage("Vulkan does not have the required extension on this system: ");
            for (const auto& extensionName : missingRequirements.getExtensions()) {
                errorMessage.append(extensionName);
                errorMessage.append("\n");
            }

            throw std::runtime_error(errorMessage);
        }

        auto enabledLayerNames = getEnabledLayerNames();
        auto instanceCreateFlags = defaultInstanceCreateFlags();
        auto enabledLayerNamesCStrings = convertToCStrings(enabledLayerNames);
        auto requiredExtensionsCStrings = convertToCStrings(instanceRequirements.getExtensions());

        VkApplicationInfo appInfo {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello, Triangle!";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        VkInstanceCreateInfo createInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.flags |= instanceCreateFlags;
        createInfo.enabledLayerCount = enabledLayerNamesCStrings.size();
        createInfo.ppEnabledLayerNames = enabledLayerNamesCStrings.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensionsCStrings.size());
        createInfo.ppEnabledExtensionNames = requiredExtensionsCStrings.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("Failed to create Vulkan instance.");
        }

        _isInitialized = true;
    }

    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }

    void setupDebugMessenger() {
        if (!enableValidationLayers) {
            return;
        }

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        VkResult result = glfwCreateWindowSurface(instance, m_window, nullptr, &surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentSupport);

            if (presentSupport) {
                indices.presentFamily = i;
            }

            if (indices.isComplete()) {
                break;
            }

            i++;
        }

        return indices;
    }

    bool checkDeviceExtensionSupport(VkPhysicalDevice device) {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

        std::vector<VkExtensionProperties> availableExtensions(extensionCount);
        vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

        std::set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

        for (const auto& extension : availableExtensions) {
            requiredExtensions.erase(extension.extensionName);
        }

        return requiredExtensions.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice device) {
        SwapChainSupportDetails details;

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        // We would probably want to use `VK_PRESENT_MODE_FIFO_KHR` on mobile devices.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetWindowSize(m_window, &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(
                actualExtent.width, 
                capabilities.minImageExtent.width, 
                capabilities.maxImageExtent.width
            );
            actualExtent.height = std::clamp(
                actualExtent.height, 
                capabilities.minImageExtent.height, 
                capabilities.maxImageExtent.height
            );

            return actualExtent;
        }
    }

    bool isDeviceSuitable(VkPhysicalDevice device) {
        QueueFamilyIndices indices = findQueueFamilies(device);

        bool extensionsSupported = checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    void choosePhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        std::vector<VkPhysicalDevice> devices(deviceCount);
        vkEnumeratePhysicalDevices(instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (isDeviceSuitable(device)) {
                physicalDevice = device;
                break;
            }
        }

        if (physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::set<uint32_t> uniqueQueueFamilies = {
            indices.graphicsFamily.value(), 
            indices.presentFamily.value()
        };
        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos{};
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        VkPhysicalDeviceFeatures deviceFeatures{};

        VkDeviceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        
        createInfo.pEnabledFeatures = &deviceFeatures;

        auto deviceExtensionProperties = PhysicalDeviceInfoOps::getAvailableVulkanDeviceExtensions(physicalDevice);
        auto requiredDeviceExtensions = getDeviceRequirements(physicalDevice);
        auto missingRequirements = PhysicalDeviceInfoOps::detectMissingRequiredDeviceExtensions(
            deviceExtensionProperties, 
            requiredDeviceExtensions
        );
        if (!missingRequirements.isEmpty()) {
            std::string errorMessage("Vulkan does not have the required extension on this system: ");
            for (const auto& extension : missingRequirements.getExtensions()) {
                errorMessage.append(extension);
                errorMessage.append("\n");
            }

            throw std::runtime_error(errorMessage);
        }
        auto enabledExtensions = convertToCStrings(requiredDeviceExtensions.getExtensions());
        createInfo.enabledExtensionCount = enabledExtensions.size();
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();
        
        auto validationLayersCStrings = convertToCStrings(validationLayers);

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayersCStrings.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = querySwapChainSupport(physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);
        uint32_t queueFamilyIndices[] = {indices.graphicsFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices;
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(device, &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(device, &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
        std::ifstream file(filename, std::ios::ate | std::ios::binary);

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = (size_t) file.tellg();
        std::vector<char> buffer(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        VkShaderModuleCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        VkShaderModule shaderModule;
        if (vkCreateShaderModule(device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void createRenderPass() {
        VkAttachmentDescription colorAttachment{};
        colorAttachment.format = swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        VkAttachmentReference colorAttachmentRef{};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        VkSubpassDescription subpass{};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(device, &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline() {
        auto vertexShaderCode = readFile("shaders/shader.vert.hlsl.spv");
        auto fragmentShaderCode = readFile("shaders/shader.frag.hlsl.spv");
        auto vertexShaderModule = createShaderModule(vertexShaderCode);
        auto fragmentShaderModule = createShaderModule(fragmentShaderCode);

        VkPipelineShaderStageCreateInfo vertexShaderStageInfo{};
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragmentShaderStageInfo{};
        fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertexShaderStageInfo, fragmentShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_TRIANGLE_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

        // Without dynamic state, the viewport and scissor rectangle need to be set 
        // in the pipeline using the `VkPipelineViewportStateCreateInfo` struct. This
        // makes the viewport and scissor rectangle for this pipeline immutable.
        // Any changes to these values would require a new pipeline to be created with
        // the new values.
        // VkPipelineViewportStateCreateInfo viewportState{};
        // viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        // viewportState.viewportCount = 1;
        // viewportState.pViewports = &viewport;
        // viewportState.scissorCount = 1;
        // viewportState.pScissors = &scissor;
        VkPipelineViewportStateCreateInfo viewportState{};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        VkPipelineRasterizationStateCreateInfo rasterizer{};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.pSampleMask = nullptr;            // Optional.
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional.
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional.

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_FALSE;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;             // Optional
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;  // Optional
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO; // Optional
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;             // Optional
        // // Alpha blending:
        // // finalColor.rgb = newAlpha * newColor + (1 - newAlpha) * oldColor;
        // // finalColor.a = newAlpha.a;
        // colorBlendAttachment.blendEnable = VK_TRUE;
        // colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        // colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        // colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        // colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE;
        // colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;
        // colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;

        VkPipelineColorBlendStateCreateInfo colorBlending{};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        std::vector<VkDynamicState> dynamicStates = {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        VkPipelineDynamicStateCreateInfo dynamicState{};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(device, &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;       // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        auto result = vkCreateGraphicsPipelines(
            device, 
            VK_NULL_HANDLE, 
            1, 
            &pipelineInfo, 
            nullptr, 
            &graphicsPipeline
        );
        
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    }

    void createFramebuffers() {
        swapChainFramebuffers.resize(swapChainImageViews.size());

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            VkImageView attachments[] = {
                swapChainImageViews[i]
            };

            VkFramebufferCreateInfo framebufferInfo{};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments;
            framebufferInfo.width = swapChainExtent.width;
            framebufferInfo.height = swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(commandBuffers.size());

        if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional.
        beginInfo.pInheritanceInfo = nullptr; // Optional.

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        VkRenderPassBeginInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = renderPass;
        renderPassInfo.framebuffer = swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = swapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, graphicsPipeline);
        
        VkViewport viewport{};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(swapChainExtent.width);
        viewport.height = static_cast<float>(swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        VkRect2D scissor{};
        scissor.offset = {0, 0};
        scissor.extent = swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS
            ) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void drawFrame() {
        vkWaitForFences(device, 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            device, 
            swapChain, 
            UINT64_MAX, 
            imageAvailableSemaphores[currentFrame], 
            VK_NULL_HANDLE, 
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(device, 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /* VkCommandBufferResetFlagBits */ 0);
        recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        VkSemaphore waitSemaphores[] = {imageAvailableSemaphores[currentFrame]};
        VkPipelineStageFlags waitStages[] = {VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT};
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];

        VkSemaphore signalSemaphores[] = {renderFinishedSemaphores[currentFrame]};
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores;

        if (vkQueueSubmit(graphicsQueue, 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores;

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanupSwapChain() {
        for (size_t i = 0; i < swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(device, swapChainFramebuffers[i], nullptr);
        }

        for (size_t i = 0; i < swapChainImageViews.size(); i++) {
            vkDestroyImageView(device, swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(device, swapChain, nullptr);
    }

    void recreateSwapChain() {
        int width = 0, height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(device);

        cleanupSwapChain();

        createSwapChain();
        createImageViews();
        createFramebuffers();
    }
};

int main() {
    HelloTriangleApplication app;

    try {
        app.run();
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}