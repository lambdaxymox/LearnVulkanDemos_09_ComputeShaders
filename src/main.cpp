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
    auto func = reinterpret_cast<PFN_vkCreateDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT")
    );
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
    auto func = reinterpret_cast<PFN_vkDestroyDebugUtilsMessengerEXT>(
        vkGetInstanceProcAddr(instance, "vkDestroyDebugUtilsMessengerEXT")
    );
    if (func != nullptr) {
        func(instance, debugMessenger, pAllocator);
    }
}

std::vector<const char*> convertToCStrings(const std::vector<std::string>& strings) {
    auto cStrings = std::vector<const char*> {};
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
    std::vector<VkLayerProperties> m_availableLayers;
    std::vector<VkExtensionProperties> m_availableExtensions;
    bool m_validationLayersAvailable = false;
	bool m_debugUtilsAvailable = false;
public:
    explicit PlatformInfo(std::vector<VkLayerProperties> availableLayers, std::vector<VkExtensionProperties> availableExtensions) 
        : m_availableLayers { availableLayers }
        , m_availableExtensions { availableExtensions }
    {
        for (const auto& layerProperties : availableLayers) {
            if (strcmp(layerProperties.layerName, VK_LAYER_KHRONOS_validation) == 0) {
                m_validationLayersAvailable = true;
            }
        }

        for (const auto& extensionProperties : availableExtensions) {
            if (strcmp(extensionProperties.extensionName, VK_EXT_DEBUG_UTILS_EXTENSION_NAME) == 0) {
                m_debugUtilsAvailable = true;
            }
        }
    }
    
    bool isExtensionAvailable(const char* layerName) const {
        for (const auto& layerProperties : m_availableLayers) {
            if (strcmp(layerProperties.layerName, layerName) == 0) {
                return true;
            }
        }

        return false;
    }

    bool isLayerAvailable(const char* extensionName) const {
        for (const auto& extensionProperties : m_availableExtensions) {
            if (strcmp(extensionProperties.extensionName, extensionName) == 0) {
                return true;
            }
        }

        return false;
    }

    const std::vector<VkLayerProperties>& getAvailableLayers() const {
        return m_availableLayers;
    }

    const std::vector<VkExtensionProperties>& getAvailableExtensions() const {
        return m_availableExtensions;
    }

    bool areValidationLayersAvailable() const {
        return m_validationLayersAvailable;
    }
    
    bool areDebugUtilsAvailable() const {
        return m_debugUtilsAvailable;
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
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;

public:
    explicit PlatformRequirements(const std::vector<std::string>& extensions, const std::vector<std::string>& layers)
        : m_instanceExtensions { extensions }
        , m_instanceLayers{ layers }
    {
    }

    const std::vector<std::string>& getExtensions() const {
        return m_instanceExtensions;
    }
 
    const std::vector<std::string>& getLayers() const {
        return m_instanceLayers;
    }

    bool isEmpty() {
        return m_instanceExtensions.empty() && m_instanceLayers.empty();
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
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;
public:
    explicit PlatformRequirementsBuilder() {
        if (vk_platform::detectOperatingSystem() == vk_platform::Platform::Apple) {
            m_instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
            m_instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        }
    }

    PlatformRequirementsBuilder& requireExtension(const std::string& extensionName) {
        m_instanceExtensions.push_back(extensionName);
        
        return *this;
    }

    PlatformRequirementsBuilder& requireLayer(const std::string& layerName) {
        m_instanceLayers.push_back(layerName);
        
        return *this;
    }

    PlatformRequirementsBuilder& requireDebuggingExtensions() {
        m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return *this;
    }

    PlatformRequirementsBuilder& requireValidationLayers() {
        m_instanceLayers.push_back(VK_LAYER_KHRONOS_validation);
        m_instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);

        return *this;
    }

    PlatformRequirementsBuilder& includeFrom(const PlatformRequirements& other) {
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

    PlatformRequirements build() const {
        // TODO: Check for move semantics.
        return PlatformRequirements(m_instanceExtensions, m_instanceLayers);
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
        auto missingExtensionNames = std::vector<std::string> {};
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

        auto missingLayerNames = std::vector<std::string> {};
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
        auto instanceLayerProperties = std::vector<VkLayerProperties> {};
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
        auto instanceExtensionProperties = std::vector<VkExtensionProperties> {};
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
    std::vector<VkExtensionProperties> m_deviceExtensions;
public:
    explicit PhysicalDeviceProperties(std::vector<VkExtensionProperties> deviceExtensions) 
        : m_deviceExtensions { deviceExtensions } 
    {
    }

    const std::vector<VkExtensionProperties>& getExtensions() const {
        return m_deviceExtensions;
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
    std::vector<std::string> m_deviceExtensions;
public:
    explicit PhysicalDeviceRequirements(std::vector<std::string> deviceExtensions) 
        : m_deviceExtensions { deviceExtensions }
    {
    }

    const std::vector<std::string>& getExtensions() const {
        return m_deviceExtensions;
    }

    bool isEmpty() const {
        return m_deviceExtensions.empty();
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
    std::vector<std::string> m_deviceExtensions;
public:
    explicit PhysicalDeviceRequirementsBuilder() {
        // https://stackoverflow.com/questions/66659907/vulkan-validation-warning-catch-22-about-vk-khr-portability-subset-on-moltenvk
        if (vk_platform::detectOperatingSystem() == vk_platform::Platform::Apple) {
            m_deviceExtensions.emplace_back(VK_KHR_portability_subset);
        }
    }

    PhysicalDeviceRequirementsBuilder& requireExtension(const std::string& extensionName) {
        m_deviceExtensions.emplace_back(extensionName);
        
        return *this;
    }

    PhysicalDeviceRequirements build() const {
        // TODO: Check move semantics.
        return PhysicalDeviceRequirements(m_deviceExtensions);
    }
};

using MissingPhysicalDeviceRequirements = PhysicalDeviceRequirements;

class PhysicalDeviceInfoOps {
public:
    static PhysicalDeviceProperties getAvailableVulkanDeviceExtensions(VkPhysicalDevice physicalDevice) {
        auto deviceExtensionProperties =  std::vector<VkExtensionProperties> {};
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
        auto missingExtensionNames = std::vector<std::string> {};
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

struct QueueFamilyIndices final {
    std::optional<uint32_t> graphicsFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails final {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class App {
public:
    void run() {
        this->initWindow();
        this->initVulkan();
        this->mainLoop();
        this->cleanup();
    }
private:
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
    VkSurfaceKHR m_surface;
    VkDevice m_device;
    VkQueue m_presentQueue;
    VkPhysicalDevice m_physicalDevice = VK_NULL_HANDLE;
    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    VkQueue m_graphicsQueue;

    VkCommandPool m_commandPool;
    std::vector<VkCommandBuffer> m_commandBuffers;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    bool m_framebufferResized = false;

    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;

    VkExtent2D m_windowExtent { WIDTH, HEIGHT };
    GLFWwindow* m_window { nullptr };

    uint32_t m_currentFrame = 0;

    bool _isInitialized { false };
    bool _enableValidationLayers { false };

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto app = reinterpret_cast<App*>(glfwGetWindowUserPointer(window));
        app->m_framebufferResized = true;
    }

    void initVulkan() {
        this->createInstance();
        this->setupDebugMessenger();
        this->createSurface();
        this->choosePhysicalDevice();
        this->createLogicalDevice();
        this->createSwapChain();
        this->createImageViews();
        this->createRenderPass();
        this->createGraphicsPipeline();
        this->createFramebuffers();
        this->createCommandPool();
        this->createCommandBuffers();
        this->createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
            this->drawFrame();
        }

        vkDeviceWaitIdle(m_device);
    }

    void cleanup() {
        if (_isInitialized) {
            this->cleanupSwapChain();

            vkDestroyPipeline(m_device, m_graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(m_device, m_pipelineLayout, nullptr);
            
            vkDestroyRenderPass(m_device, m_renderPass, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(m_device, m_renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(m_device, m_imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(m_device, m_inFlightFences[i], nullptr);
            }
            
            vkDestroyCommandPool(m_device, m_commandPool, nullptr);

            vkDestroyDevice(m_device, nullptr);

            if (enableValidationLayers) {
                DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
            }

            vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
            vkDestroyInstance(m_instance, nullptr);

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

    PlatformRequirements getVulkanInstanceExtensionsRequiredByGLFW() const {
        uint32_t requiredExtensionCount = 0;
        const char** requiredExtensionNames = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
        auto requiredExtensions = std::vector<std::string> {};
        for (int i = 0; i < requiredExtensionCount; i++) {
            requiredExtensions.emplace_back(std::string(requiredExtensionNames[i]));
        }

        auto builder = PlatformRequirementsBuilder {};
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
        auto vulkanExtensionsRequiredByGLFW = this->getVulkanInstanceExtensionsRequiredByGLFW();
        auto platformRequirements = PlatformRequirementsBuilder()
            .requireValidationLayers()
            .requireDebuggingExtensions()
            .includeFrom(vulkanExtensionsRequiredByGLFW)
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
        if (enableValidationLayers && !this->checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        auto enabledLayerCount = 0;
        auto enabledLayerNames = std::vector<std::string> {};
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
        auto instanceRequirements = this->getInstanceRequirements();
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

        auto enabledLayerNames = this->getEnabledLayerNames();
        auto instanceCreateFlags = this->defaultInstanceCreateFlags();
        auto enabledLayerNamesCStrings = convertToCStrings(enabledLayerNames);
        auto requiredExtensionsCStrings = convertToCStrings(instanceRequirements.getExtensions());

        auto appInfo = VkApplicationInfo {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello, Triangle!";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        auto createInfo = VkInstanceCreateInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.flags |= instanceCreateFlags;
        createInfo.enabledLayerCount = enabledLayerNamesCStrings.size();
        createInfo.ppEnabledLayerNames = enabledLayerNamesCStrings.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(requiredExtensionsCStrings.size());
        createInfo.ppEnabledExtensionNames = requiredExtensionsCStrings.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &m_instance);
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

        auto createInfo = VkDebugUtilsMessengerCreateInfoEXT {};
        this->populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(m_instance, &createInfo, nullptr, &m_debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }

    void createSurface() {
        VkResult result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &m_surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        auto indices = QueueFamilyIndices {};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        auto queueFamilies = std::vector<VkQueueFamilyProperties> { queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if (queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) {
                indices.graphicsFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(device, i, m_surface, &presentSupport);

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
        auto details = SwapChainSupportDetails {};

        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(device, m_surface, &details.capabilities);

        uint32_t formatCount;
        vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(device, m_surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount;
        vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(device, m_surface, &presentModeCount, details.presentModes.data());
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

            auto actualExtent = VkExtent2D {
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
        QueueFamilyIndices indices = this->findQueueFamilies(device);

        bool extensionsSupported = this->checkDeviceExtensionSupport(device);

        bool swapChainAdequate = false;
        if (extensionsSupported) {
            SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(device);
            swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        return indices.isComplete() && extensionsSupported && swapChainAdequate;
    }

    void choosePhysicalDevice() {
        uint32_t deviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, nullptr);

        if (deviceCount == 0) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        auto devices = std::vector<VkPhysicalDevice> { deviceCount };
        vkEnumeratePhysicalDevices(m_instance, &deviceCount, devices.data());

        for (const auto& device : devices) {
            if (this->isDeviceSuitable(device)) {
                m_physicalDevice = device;
                break;
            }
        }

        if (m_physicalDevice == VK_NULL_HANDLE) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }
    }

    void createLogicalDevice() {
        QueueFamilyIndices indices = this->findQueueFamilies(m_physicalDevice);

        auto uniqueQueueFamilies = std::set<uint32_t> {
            indices.graphicsFamily.value(), 
            indices.presentFamily.value()
        };
        auto queueCreateInfos = std::vector<VkDeviceQueueCreateInfo> {};
        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            auto queueCreateInfo = VkDeviceQueueCreateInfo {};
            queueCreateInfo.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO;
            queueCreateInfo.queueFamilyIndex = queueFamily;
            queueCreateInfo.queueCount = 1;
            queueCreateInfo.pQueuePriorities = &queuePriority;
            queueCreateInfos.push_back(queueCreateInfo);
        }

        auto deviceFeatures = VkPhysicalDeviceFeatures {};

        auto createInfo = VkDeviceCreateInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        
        createInfo.pEnabledFeatures = &deviceFeatures;

        auto deviceExtensionProperties = PhysicalDeviceInfoOps::getAvailableVulkanDeviceExtensions(m_physicalDevice);
        auto requiredDeviceExtensions = this->getDeviceRequirements(m_physicalDevice);
        auto missingRequirements = PhysicalDeviceInfoOps::detectMissingRequiredDeviceExtensions(
            deviceExtensionProperties, 
            requiredDeviceExtensions
        );
        if (!missingRequirements.isEmpty()) {
            auto errorMessage = std::string { "Vulkan does not have the required extension on this system: " };
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

        if (vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &m_device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(m_device, indices.graphicsFamily.value(), 0, &m_graphicsQueue);
        vkGetDeviceQueue(m_device, indices.presentFamily.value(), 0, &m_presentQueue);
    }

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(m_physicalDevice);

        VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = this->chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        auto createInfo = VkSwapchainCreateInfoKHR {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_surface;

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = this->findQueueFamilies(m_physicalDevice);
        auto queueFamilyIndices = std::array<uint32_t, 2> { 
            indices.graphicsFamily.value(),
            indices.presentFamily.value()
        };

        if (indices.graphicsFamily != indices.presentFamily) {
            createInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
            createInfo.queueFamilyIndexCount = 2;
            createInfo.pQueueFamilyIndices = queueFamilyIndices.data();
        } else {
            createInfo.imageSharingMode = VK_SHARING_MODE_EXCLUSIVE;
        }

        createInfo.preTransform = swapChainSupport.capabilities.currentTransform;
        createInfo.compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR;
        createInfo.presentMode = presentMode;
        createInfo.clipped = VK_TRUE;

        createInfo.oldSwapchain = VK_NULL_HANDLE;

        if (vkCreateSwapchainKHR(m_device, &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_device, m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void createImageViews() {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            auto createInfo = VkImageViewCreateInfo {};
            createInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
            createInfo.image = m_swapChainImages[i];
            createInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
            createInfo.format = m_swapChainImageFormat;
            createInfo.components.r = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.g = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.b = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.components.a = VK_COMPONENT_SWIZZLE_IDENTITY;
            createInfo.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            createInfo.subresourceRange.baseMipLevel = 0;
            createInfo.subresourceRange.levelCount = 1;
            createInfo.subresourceRange.baseArrayLayer = 0;
            createInfo.subresourceRange.layerCount = 1;

            if (vkCreateImageView(m_device, &createInfo, nullptr, &m_swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("Failed to create image views!");
            }
        }
    }

    static std::vector<char> readFile(const std::string& filename) {
        auto file = std::ifstream { filename, std::ios::ate | std::ios::binary };

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        size_t fileSize = static_cast<size_t>(file.tellg());
        auto buffer = std::vector<char>(fileSize);

        file.seekg(0);
        file.read(buffer.data(), fileSize);

        file.close();

        return buffer;
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        auto createInfo = VkShaderModuleCreateInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());

        auto shaderModule = VkShaderModule {};
        if (vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule) != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        return shaderModule;
    }

    void createRenderPass() {
        auto colorAttachment = VkAttachmentDescription {};
        colorAttachment.format = m_swapChainImageFormat;
        colorAttachment.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        auto colorAttachmentRef = VkAttachmentReference {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        auto subpass = VkSubpassDescription {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;

        auto renderPassInfo = VkRenderPassCreateInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;

        if (vkCreateRenderPass(m_device, &renderPassInfo, nullptr, &m_renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline() {
        auto vertexShaderCode = this->readFile("shaders/shader.vert.hlsl.spv");
        auto fragmentShaderCode = this->readFile("shaders/shader.frag.hlsl.spv");
        auto vertexShaderModule = this->createShaderModule(vertexShaderCode);
        auto fragmentShaderModule = this->createShaderModule(fragmentShaderCode);

        auto vertexShaderStageInfo = VkPipelineShaderStageCreateInfo {};
        vertexShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertexShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertexShaderStageInfo.module = vertexShaderModule;
        vertexShaderStageInfo.pName = "main";

        auto fragmentShaderStageInfo = VkPipelineShaderStageCreateInfo {};
        fragmentShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragmentShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragmentShaderStageInfo.module = fragmentShaderModule;
        fragmentShaderStageInfo.pName = "main";

        auto shaderStages = std::array<VkPipelineShaderStageCreateInfo, 2> {
            vertexShaderStageInfo,
            fragmentShaderStageInfo
        };

        auto vertexInputInfo = VkPipelineVertexInputStateCreateInfo {};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;
        vertexInputInfo.vertexBindingDescriptionCount = 0;
        vertexInputInfo.vertexAttributeDescriptionCount = 0;

        auto inputAssembly = VkPipelineInputAssemblyStateCreateInfo {};
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
        auto viewportState = VkPipelineViewportStateCreateInfo {};
        viewportState.sType = VK_STRUCTURE_TYPE_PIPELINE_VIEWPORT_STATE_CREATE_INFO;
        viewportState.viewportCount = 1;
        viewportState.scissorCount = 1;

        auto rasterizer = VkPipelineRasterizationStateCreateInfo {};
        rasterizer.sType = VK_STRUCTURE_TYPE_PIPELINE_RASTERIZATION_STATE_CREATE_INFO;
        rasterizer.depthClampEnable = VK_FALSE;
        rasterizer.rasterizerDiscardEnable = VK_FALSE;
        rasterizer.polygonMode = VK_POLYGON_MODE_FILL;
        rasterizer.lineWidth = 1.0f;
        rasterizer.cullMode = VK_CULL_MODE_BACK_BIT;
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        auto multisampling = VkPipelineMultisampleStateCreateInfo {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;
        multisampling.pSampleMask = nullptr;            // Optional.
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional.
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional.

        auto colorBlendAttachment = VkPipelineColorBlendAttachmentState {};
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

        auto colorBlending = VkPipelineColorBlendStateCreateInfo {};
        colorBlending.sType = VK_STRUCTURE_TYPE_PIPELINE_COLOR_BLEND_STATE_CREATE_INFO;
        colorBlending.logicOpEnable = VK_FALSE;
        colorBlending.logicOp = VK_LOGIC_OP_COPY;
        colorBlending.attachmentCount = 1;
        colorBlending.pAttachments = &colorBlendAttachment;
        colorBlending.blendConstants[0] = 0.0f;
        colorBlending.blendConstants[1] = 0.0f;
        colorBlending.blendConstants[2] = 0.0f;
        colorBlending.blendConstants[3] = 0.0f;

        auto dynamicStates = std::vector<VkDynamicState> {
            VK_DYNAMIC_STATE_VIEWPORT,
            VK_DYNAMIC_STATE_SCISSOR
        };
        auto dynamicState = VkPipelineDynamicStateCreateInfo {};
        dynamicState.sType = VK_STRUCTURE_TYPE_PIPELINE_DYNAMIC_STATE_CREATE_INFO;
        dynamicState.dynamicStateCount = static_cast<uint32_t>(dynamicStates.size());
        dynamicState.pDynamicStates = dynamicStates.data();

        auto pipelineLayoutInfo = VkPipelineLayoutCreateInfo {};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 0;
        pipelineLayoutInfo.pushConstantRangeCount = 0;
        pipelineLayoutInfo.pSetLayouts = nullptr;         // Optional
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        if (vkCreatePipelineLayout(m_device, &pipelineLayoutInfo, nullptr, &m_pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        auto pipelineInfo = VkGraphicsPipelineCreateInfo {};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages.data();
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pDepthStencilState = nullptr;       // Optional
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        auto result = vkCreateGraphicsPipelines(
            m_device, 
            VK_NULL_HANDLE, 
            1, 
            &pipelineInfo, 
            nullptr, 
            &m_graphicsPipeline
        );
        
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        vkDestroyShaderModule(m_device, fragmentShaderModule, nullptr);
        vkDestroyShaderModule(m_device, vertexShaderModule, nullptr);
    }

    void createFramebuffers() {
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            auto attachments = std::array<VkImageView, 1> {
                m_swapChainImageViews[i]
            };

            auto framebufferInfo = VkFramebufferCreateInfo {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = 1;
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_swapChainExtent.width;
            framebufferInfo.height = m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            if (vkCreateFramebuffer(m_device, &framebufferInfo, nullptr, &m_swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = this->findQueueFamilies(m_physicalDevice);

        auto poolInfo = VkCommandPoolCreateInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsFamily.value();

        if (vkCreateCommandPool(m_device, &poolInfo, nullptr, &m_commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }
    }

    void createCommandBuffers() {
        m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        auto allocInfo = VkCommandBufferAllocateInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_commandPool;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

        if (vkAllocateCommandBuffers(m_device, &allocInfo, m_commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        auto beginInfo = VkCommandBufferBeginInfo {};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = 0;                  // Optional.
        beginInfo.pInheritanceInfo = nullptr; // Optional.

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording command buffer!");
        }

        auto renderPassInfo = VkRenderPassBeginInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_BEGIN_INFO;
        renderPassInfo.renderPass = m_renderPass;
        renderPassInfo.framebuffer = m_swapChainFramebuffers[imageIndex];
        renderPassInfo.renderArea.offset = {0, 0};
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        VkClearValue clearColor = {{{0.0f, 0.0f, 0.0f, 1.0f}}};
        renderPassInfo.clearValueCount = 1;
        renderPassInfo.pClearValues = &clearColor;

        vkCmdBeginRenderPass(commandBuffer, &renderPassInfo, VK_SUBPASS_CONTENTS_INLINE);
        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_GRAPHICS, m_graphicsPipeline);
        
        auto viewport = VkViewport {};
        viewport.x = 0.0f;
        viewport.y = 0.0f;
        viewport.width = static_cast<float>(m_swapChainExtent.width);
        viewport.height = static_cast<float>(m_swapChainExtent.height);
        viewport.minDepth = 0.0f;
        viewport.maxDepth = 1.0f;
        vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

        auto scissor = VkRect2D {};
        scissor.offset = VkOffset2D { 0, 0 };
        scissor.extent = m_swapChainExtent;
        vkCmdSetScissor(commandBuffer, 0, 1, &scissor);

        vkCmdDraw(commandBuffer, 3, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void createSyncObjects() {
        m_imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        m_inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        auto semaphoreInfo = VkSemaphoreCreateInfo {};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        auto fenceInfo = VkFenceCreateInfo {};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_device, &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_device, &fenceInfo, nullptr, &m_inFlightFences[i]) != VK_SUCCESS
            ) {
                throw std::runtime_error("failed to create synchronization objects for a frame!");
            }
        }
    }

    void drawFrame() {
        vkWaitForFences(m_device, 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(
            m_device, 
            m_swapChain, 
            UINT64_MAX, 
            m_imageAvailableSemaphores[m_currentFrame], 
            VK_NULL_HANDLE, 
            &imageIndex
        );

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            this->recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(m_device, 1, &m_inFlightFences[m_currentFrame]);

        vkResetCommandBuffer(m_commandBuffers[m_currentFrame], /* VkCommandBufferResetFlagBits */ 0);
        this->recordCommandBuffer(m_commandBuffers[m_currentFrame], imageIndex);

        auto submitInfo = VkSubmitInfo {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        auto waitSemaphores = std::array<VkSemaphore, 1> { m_imageAvailableSemaphores[m_currentFrame] };
        auto waitStages = std::array<VkPipelineStageFlags, 1> { VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo.waitSemaphoreCount = 1;
        submitInfo.pWaitSemaphores = waitSemaphores.data();
        submitInfo.pWaitDstStageMask = waitStages.data();

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &m_commandBuffers[m_currentFrame];

        auto signalSemaphores = std::array<VkSemaphore, 1> { m_renderFinishedSemaphores[m_currentFrame] };
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = signalSemaphores.data();

        if (vkQueueSubmit(m_graphicsQueue, 1, &submitInfo, m_inFlightFences[m_currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        auto presentInfo = VkPresentInfoKHR {};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = signalSemaphores.data();

        auto swapChains = std::array<VkSwapchainKHR, 1> { m_swapChain };
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains.data();

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(m_presentQueue, &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_framebufferResized) {
            m_framebufferResized = false;
            this->recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanupSwapChain() {
        for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(m_device, m_swapChainFramebuffers[i], nullptr);
        }

        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            vkDestroyImageView(m_device, m_swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(m_device, m_swapChain, nullptr);
    }

    void recreateSwapChain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_window, &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_window, &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_device);

        this->cleanupSwapChain();

        this->createSwapChain();
        this->createImageViews();
        this->createFramebuffers();
    }
};

int main() {
    App app;

    try {
        app.run();
    } catch (const std::exception& exception) {
        std::cerr << exception.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
