#include <vulkan/vulkan.h>

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
#include <chrono>
#include <random>

#include <fmt/core.h>
#include <fmt/ostream.h>

#include <vulkan_engine/vulkan_platform.h>
#include <vulkan_engine/vulkan_platform_impl_fmt.h>

#include <unordered_set>

#ifndef GLFW_INCLUDE_VULKAN
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#endif // GLFW_INCLUDE_VULKAN

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEFAULT_ALIGNED_GENTYPES
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/hash.hpp>


#include <stb/stb_image.h>
#include <tiny_obj_loader/tiny_obj_loader.h>


const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

const uint32_t PARTICLE_COUNT = 8192;

const std::string MODEL_PATH = std::string { "assets/viking_room/viking_room.obj" };
const std::string TEXTURE_PATH = std::string { "assets/viking_room/viking_room.png" };

const std::vector<std::string> validationLayers = std::vector<std::string> { 
    VulkanEngine::Constants::VK_LAYER_KHRONOS_validation
};

const std::vector<const char*> deviceExtensions = std::vector<const char*> {
    VK_KHR_SWAPCHAIN_EXTENSION_NAME
};

#ifdef NDEBUG
const bool enableValidationLayers = false;
const bool enableDebuggingExtensions = false;
#else
const bool enableValidationLayers = true;
const bool enableDebuggingExtensions = true;
#endif

const int MAX_FRAMES_IN_FLIGHT = 2;




struct Vertex {
    glm::vec3 pos;
    glm::vec3 color;
    glm::vec2 texCoord;

    static VkVertexInputBindingDescription getBindingDescription() {
        auto bindingDescription = VkVertexInputBindingDescription {};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Vertex);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 3> getAttributeDescriptions() {
        auto attributeDescriptions = std::array<VkVertexInputAttributeDescription, 3> {};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Vertex, pos);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Vertex, color);

        attributeDescriptions[2].binding = 0;
        attributeDescriptions[2].location = 2;
        attributeDescriptions[2].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[2].offset = offsetof(Vertex, texCoord);

        return attributeDescriptions;
    }

    bool operator==(const Vertex& other) const {
        return pos == other.pos && color == other.color && texCoord == other.texCoord;
    }
};

namespace std {
    template<> struct hash<Vertex> {
        size_t operator()(Vertex const& vertex) const {
            return ((hash<glm::vec3>()(vertex.pos) ^ (hash<glm::vec3>()(vertex.color) << 1)) >> 1) ^ (hash<glm::vec2>()(vertex.texCoord) << 1);
        }
    };
}


/// @brief The uniform buffer object for distpaching camera data to the GPU.
///
/// @note Vulkan expects data to be aligned in a specific way. For example,
/// let `T` be a data type.
///
/// @li If `T` is a scalar, `align(T) == sizeof(T)`
/// @li If `T` is a scalar, `align(vec2<T>) == 2 * sizeof(T)`
/// @li If `T` is a scalar, `align(vec3<T>) == 4 * sizeof(T)`
/// @li If `T` is a scalar, `align(vec4<T>) == 4 * sizeof(T)`
/// @li If `T` is a scalar, `align(mat4<T>) == 4 * sizeof(T)`
/// @li If `T` is a structure type, `align(T) == max(align(members(T)))`
///
/// In particular, each data type is a nice multiple of the alignment of the largest
/// scalar type constituting that data type. See the specification
/// https://registry.khronos.org/vulkan/specs/1.3-extensions/html/chap15.html#interfaces-resources-layout
/// for more details.
struct UniformBufferObject {
    glm::mat4x4 model;
    glm::mat4x4 view;
    glm::mat4x4 proj;
};




using VulkanInstanceProperties = VulkanEngine::VulkanPlatform::VulkanInstanceProperties;
using PhysicalDeviceProperties = VulkanEngine::VulkanPlatform::PhysicalDeviceProperties;
using Platform = VulkanEngine::VulkanPlatform::PlatformInfoProvider::Platform;
using PlatformInfoProvider = VulkanEngine::VulkanPlatform::PlatformInfoProvider;


struct QueueFamilyIndices final {
    std::optional<uint32_t> graphicsAndComputeFamily;
    std::optional<uint32_t> presentFamily;

    bool isComplete() {
        return graphicsAndComputeFamily.has_value() && presentFamily.has_value();
    }
};

struct SwapChainSupportDetails final {
    VkSurfaceCapabilitiesKHR capabilities;
    std::vector<VkSurfaceFormatKHR> formats;
    std::vector<VkPresentModeKHR> presentModes;
};

class VulkanInstanceSpec final {
public:
    explicit VulkanInstanceSpec() = default;
    explicit VulkanInstanceSpec(
        std::vector<std::string> instanceExtensions,
        std::vector<std::string> instanceLayers,
        VkInstanceCreateFlags instanceCreateFlags,
        std::string applicationName,
        std::string engineName
    ) 
        : m_instanceExtensions { instanceExtensions }
        , m_instanceLayers { instanceLayers }
        , m_instanceCreateFlags { instanceCreateFlags }
        , m_applicationName { applicationName }
        , m_engineName { engineName }
    {
    }
    ~VulkanInstanceSpec() = default;

    const std::vector<std::string>& instanceExtensions() const {
        return m_instanceExtensions;
    }

    const std::vector<std::string>& instanceLayers() const {
        return m_instanceLayers;
    }

    VkInstanceCreateFlags instanceCreateFlags() const {
        return m_instanceCreateFlags;
    }

    const std::string& applicationName() const {
        return m_applicationName;
    }

    const std::string& engineName() const {
        return m_engineName;
    }

    bool areValidationLayersEnabled() const {
        auto found = std::find(
            m_instanceLayers.begin(), 
            m_instanceLayers.end(), 
            VulkanEngine::Constants::VK_LAYER_KHRONOS_validation
        );
        
        return found != m_instanceLayers.end();
    }
private:
    std::vector<std::string> m_instanceExtensions;
    std::vector<std::string> m_instanceLayers;
    VkInstanceCreateFlags m_instanceCreateFlags;
    std::string m_applicationName;
    std::string m_engineName;
};

class InstanceSpecProvider final {
public:
    explicit InstanceSpecProvider() = default;
    explicit InstanceSpecProvider(bool enableValidationLayers, bool enableDebuggingExtensions)
        : m_enableValidationLayers { enableValidationLayers }
        , m_enableDebuggingExtensions { enableDebuggingExtensions }
    {
    }

    ~InstanceSpecProvider() {
        m_enableValidationLayers = false;
        m_enableDebuggingExtensions = false;
    }

    VulkanInstanceSpec createInstanceSpec() const {
        auto instanceExtensions = this->getInstanceExtensions();
        auto instanceLayers = this->getInstanceLayers();
        auto instanceCreateFlags = this->minInstanceCreateFlags();
        auto applicationName = std::string { "" };
        auto engineName = std::string { "" };

        return VulkanInstanceSpec {
            instanceExtensions,
            instanceLayers,
            instanceCreateFlags,
            applicationName,
            engineName
        };
    }
private:
    bool m_enableValidationLayers;
    bool m_enableDebuggingExtensions;

    enum class Platform {
        Apple,
        Linux,
        Windows,
        Unknown
    };

    Platform getOperatingSystem() const {
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

    VkInstanceCreateFlags minInstanceCreateFlags() const {
        auto instanceCreateFlags = 0;
        if (this->getOperatingSystem() == Platform::Apple) {
            instanceCreateFlags = VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;
        }

        return instanceCreateFlags;
    }

    std::vector<std::string> getWindowSystemInstanceRequirements() const {        
        uint32_t requiredExtensionCount = 0;
        const char** requiredExtensionNames = glfwGetRequiredInstanceExtensions(&requiredExtensionCount);
        auto requiredExtensions = std::vector<std::string> {};
        for (int i = 0; i < requiredExtensionCount; i++) {
            requiredExtensions.emplace_back(std::string(requiredExtensionNames[i]));
        }

        return requiredExtensions;
    }

    std::vector<std::string> getInstanceExtensions() const {
        auto instanceExtensions = this->getWindowSystemInstanceRequirements();
        instanceExtensions.push_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);
        instanceExtensions.push_back(VK_KHR_GET_PHYSICAL_DEVICE_PROPERTIES_2_EXTENSION_NAME);
        if (m_enableDebuggingExtensions) {
            instanceExtensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return instanceExtensions;
    }

    std::vector<std::string> getInstanceLayers() const {
        auto instanceLayers = std::vector<std::string> {};
        if (m_enableValidationLayers) {
            instanceLayers.push_back(VulkanEngine::Constants::VK_LAYER_KHRONOS_validation);
        }

        return instanceLayers;
    }
};

class SystemFactory final {
public:
    explicit SystemFactory() = default;

    VkInstance create(const VulkanInstanceSpec& instanceSpec) {
        if (instanceSpec.areValidationLayersEnabled() && !m_infoProvider->areValidationLayersSupported()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        auto instanceInfo = m_infoProvider->getVulkanInstanceInfo();
        auto instanceExtensions = instanceSpec.instanceExtensions();
        auto instanceLayers = instanceSpec.instanceLayers();
        auto missingExtensions = m_infoProvider->detectMissingInstanceExtensions(
            instanceInfo,
            instanceExtensions
        );
        auto missingLayers = m_infoProvider->detectMissingInstanceLayers(
            instanceInfo,
            instanceLayers
        );
        if (missingExtensions.empty() && !missingLayers.empty()) {
            auto errorMessage = std::string { "Vulkan does not have the required extensions on this system:\n" };
            for (const auto& extensionName : missingExtensions) {
                errorMessage.append(extensionName);
                errorMessage.append("\n");
            }

            throw std::runtime_error { errorMessage };
        }

        if (!missingExtensions.empty() && missingLayers.empty()) {
            auto errorMessage = std::string { "Vulkan does not have the required layers on this system:\n" };
            for (const auto& layerName : missingLayers) {
                errorMessage.append(layerName);
                errorMessage.append("\n");
            }

            throw std::runtime_error { errorMessage };
        }

        if (!missingExtensions.empty() && !missingLayers.empty()) {
            auto errorMessage = std::string { "Vulkan does not have the required extensions on this system:\n" };
            for (const auto& extensionName : missingExtensions) {
                errorMessage.append(extensionName);
                errorMessage.append("\n");
            }

            errorMessage.append(std::string { "Vulkan does not have the required layers on this system:\n" });
            for (const auto& layerName : missingLayers) {
                errorMessage.append(layerName);
                errorMessage.append("\n");
            }

            throw std::runtime_error { errorMessage };
        }

        auto instanceCreateFlags = instanceSpec.instanceCreateFlags();
        auto enabledLayerNames = SystemFactory::convertToCStrings(instanceLayers);
        auto enabledExtensionNames = SystemFactory::convertToCStrings(instanceExtensions);

        auto appInfo = VkApplicationInfo {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = instanceSpec.applicationName().data();
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = instanceSpec.engineName().data();
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_3;

        auto createInfo = VkInstanceCreateInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;
        createInfo.flags |= instanceCreateFlags;
        createInfo.enabledLayerCount = enabledLayerNames.size();
        createInfo.ppEnabledLayerNames = enabledLayerNames.data();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(enabledExtensionNames.size());
        createInfo.ppEnabledExtensionNames = enabledExtensionNames.data();

        VkInstance instance = VK_NULL_HANDLE;
        auto result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error(fmt::format("Failed to create Vulkan instance."));
        }

        return instance;
    }
private:
    PlatformInfoProvider* m_infoProvider;

    static std::vector<const char*> convertToCStrings(const std::vector<std::string>& strings) {
        auto cStrings = std::vector<const char*> {};
        cStrings.reserve(strings.size());
        std::transform(
            strings.begin(), 
            strings.end(), 
            std::back_inserter(cStrings),
            [](const std::string& string) { return string.c_str(); }
        );
    
        return cStrings;
    }
};

class PhysicalDeviceSpec final {
public:
    explicit PhysicalDeviceSpec() = default;
    explicit PhysicalDeviceSpec(const std::vector<std::string>& requiredExtensions, bool hasGraphicsFamily, bool hasPresentFamily)
        : m_requiredExtensions { requiredExtensions }
        , m_hasGraphicsFamily { hasGraphicsFamily }
        , m_hasPresentFamily { hasPresentFamily }
    {
    }
    ~PhysicalDeviceSpec() = default;

    const std::vector<std::string>& requiredExtensions() const {
        return m_requiredExtensions;
    }

    bool hasGraphicsFamily() const {
        return m_hasGraphicsFamily;
    }

    bool hasPresentFamily() const {
        return m_hasPresentFamily;
    }
private:
    std::vector<std::string> m_requiredExtensions;
    bool m_hasGraphicsFamily;
    bool m_hasPresentFamily;
};

class PhysicalDeviceSpecProvider final {
public:
    explicit PhysicalDeviceSpecProvider() = default;

    PhysicalDeviceSpec createPhysicalDeviceSpec() const {
        auto requiredExtensions = this->getPhysicalDeviceRequirements();

        return PhysicalDeviceSpec { requiredExtensions, true, true };
    }
private:
    enum class Platform {
        Apple,
        Linux,
        Windows,
        Unknown
    };

    Platform getOperatingSystem() const {
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

    std::vector<std::string> getPhysicalDeviceRequirements() const {
        auto physicalDeviceExtensions = std::vector<std::string> {};
        if (this->getOperatingSystem() == Platform::Apple) {
            physicalDeviceExtensions.push_back(VulkanEngine::Constants::VK_KHR_portability_subset);
        }

        physicalDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        return physicalDeviceExtensions;
    }
};

class PhysicalDeviceSelector final {
public:
    explicit PhysicalDeviceSelector(VkInstance instance, PlatformInfoProvider* infoProvider)
        : m_instance { instance }
        , m_infoProvider { infoProvider }
    {
    }

    ~PhysicalDeviceSelector() {
        m_instance = VK_NULL_HANDLE;
        m_infoProvider = nullptr;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
        auto indices = QueueFamilyIndices {};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        auto queueFamilies = std::vector<VkQueueFamilyProperties> { queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.graphicsAndComputeFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

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

    bool checkDeviceExtensionSupport(VkPhysicalDevice physicalDevice, const std::vector<std::string>& requiredExtensions) const {
        uint32_t extensionCount;
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, nullptr);

        auto availableExtensions = std::vector<VkExtensionProperties> { extensionCount };
        vkEnumerateDeviceExtensionProperties(physicalDevice, nullptr, &extensionCount, availableExtensions.data());

        auto remainingExtensions = std::set<std::string> { requiredExtensions.begin(), requiredExtensions.end() };
        for (const auto& extension : availableExtensions) {
            for (const auto& requiredExtension : requiredExtensions) {
                remainingExtensions.erase(extension.extensionName);
            }
        }

        return remainingExtensions.empty();
    }

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
        auto details = SwapChainSupportDetails {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    bool isPhysicalDeviceCompatible(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, const PhysicalDeviceSpec& physicalDeviceSpec) const {
        QueueFamilyIndices indices = this->findQueueFamilies(physicalDevice, surface);

        bool areRequiredExtensionsSupported = this->checkDeviceExtensionSupport(
            physicalDevice,
            physicalDeviceSpec.requiredExtensions()
        );

        bool swapChainCompatible = false;
        if (areRequiredExtensionsSupported) {
            SwapChainSupportDetails swapChainSupport = this->querySwapChainSupport(physicalDevice, surface);
            swapChainCompatible = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();
        }

        auto supportedFeatures = VkPhysicalDeviceFeatures {};
        vkGetPhysicalDeviceFeatures(physicalDevice, &supportedFeatures);

        return indices.isComplete() && areRequiredExtensionsSupported && swapChainCompatible && supportedFeatures.samplerAnisotropy;
    }

    std::vector<VkPhysicalDevice> findAllPhysicalDevices() const {
        uint32_t physicalDeviceCount = 0;
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, nullptr);

        auto physicalDevices = std::vector<VkPhysicalDevice> { physicalDeviceCount };
        vkEnumeratePhysicalDevices(m_instance, &physicalDeviceCount, physicalDevices.data());

        return physicalDevices;
    }

    std::vector<VkPhysicalDevice> findCompatiblePhysicalDevices(VkSurfaceKHR surface, const PhysicalDeviceSpec& physicalDeviceSpec) const {
        auto physicalDevices = this->findAllPhysicalDevices();
        if (physicalDevices.empty()) {
            throw std::runtime_error("failed to find GPUs with Vulkan support!");
        }

        auto compatiblePhysicalDevices = std::vector<VkPhysicalDevice> {};
        for (const auto& physicalDevice : physicalDevices) {
            if (this->isPhysicalDeviceCompatible(physicalDevice, surface, physicalDeviceSpec)) {
                compatiblePhysicalDevices.emplace_back(physicalDevice);
            }
        }

        return compatiblePhysicalDevices;
    }

    VkPhysicalDevice selectPhysicalDeviceForSurface(VkSurfaceKHR surface, const PhysicalDeviceSpec& physicalDeviceSpec) const {
        auto physicalDevices = this->findCompatiblePhysicalDevices(surface, physicalDeviceSpec);
        if (physicalDevices.empty()) {
            throw std::runtime_error("failed to find a suitable GPU!");
        }

        VkPhysicalDevice selectedPhysicalDevice = physicalDevices[0];

        return selectedPhysicalDevice;
    }
private:
    VkInstance m_instance;
    PlatformInfoProvider* m_infoProvider;
};

class LogicalDeviceSpec final {
public:
    explicit LogicalDeviceSpec() = default;
    explicit LogicalDeviceSpec(const std::vector<std::string>& requiredExtensions, bool requireSamplerAnisotropy)
        : m_requiredExtensions { requiredExtensions }
        , m_requireSamplerAnisotropy { requireSamplerAnisotropy }
    {
    }
    ~LogicalDeviceSpec() = default;

    const std::vector<std::string>& requiredExtensions() const {
        return m_requiredExtensions;
    }

    bool requireSamplerAnisotropy() const {
        return m_requireSamplerAnisotropy;
    }
private:
    std::vector<std::string> m_requiredExtensions;
    bool m_requireSamplerAnisotropy;
};

class LogicalDeviceSpecProvider final {
public:
    explicit LogicalDeviceSpecProvider() = default;
    explicit LogicalDeviceSpecProvider(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) 
        : m_physicalDevice { physicalDevice }
        , m_surface { surface }
    {
    }

    ~LogicalDeviceSpecProvider() {
        m_physicalDevice = VK_NULL_HANDLE;
        m_surface = VK_NULL_HANDLE;
    }

    LogicalDeviceSpec createLogicalDeviceSpec() const {
        auto requiredExtensions = this->getLogicalDeviceRequirements();
        bool requireSamplerAnisotropy = true;

        return LogicalDeviceSpec { requiredExtensions, requireSamplerAnisotropy };
    }
private:
    VkPhysicalDevice m_physicalDevice;
    VkSurfaceKHR m_surface;

    enum class Platform {
        Apple,
        Linux,
        Windows,
        Unknown
    };

    Platform getOperatingSystem() const {
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

    std::vector<std::string> getLogicalDeviceRequirements() const {
        auto logicalDeviceExtensions = std::vector<std::string> {};
        if (this->getOperatingSystem() == Platform::Apple) {
            logicalDeviceExtensions.push_back(VulkanEngine::Constants::VK_KHR_portability_subset);
        }

        logicalDeviceExtensions.push_back(VK_KHR_SWAPCHAIN_EXTENSION_NAME);

        return logicalDeviceExtensions;
    }
};

class LogicalDeviceFactory final {
public:
    explicit LogicalDeviceFactory(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface, PlatformInfoProvider* infoProvider)
        : m_physicalDevice { physicalDevice }
        , m_surface { surface }
        , m_infoProvider { infoProvider }
    {
    }

    ~LogicalDeviceFactory() {
        m_physicalDevice = VK_NULL_HANDLE;
        m_surface = VK_NULL_HANDLE;
        m_infoProvider = nullptr;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
        auto indices = QueueFamilyIndices {};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        auto queueFamilies = std::vector<VkQueueFamilyProperties> { queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.graphicsAndComputeFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

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

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
        auto details = SwapChainSupportDetails {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    std::tuple<VkDevice, VkQueue, VkQueue, VkQueue> createLogicalDevice(const LogicalDeviceSpec& logicalDeviceSpec) {
        QueueFamilyIndices indices = this->findQueueFamilies(m_physicalDevice, m_surface);

        auto uniqueQueueFamilies = std::set<uint32_t> {
            indices.graphicsAndComputeFamily.value(), 
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

        VkBool32 requireSamplerAnisotropy = [&logicalDeviceSpec]() { 
            if (logicalDeviceSpec.requireSamplerAnisotropy()) {    
                return VK_TRUE;
            } else {
                return VK_FALSE;
            }
        }();
        auto deviceFeatures = VkPhysicalDeviceFeatures {};
        deviceFeatures.samplerAnisotropy = requireSamplerAnisotropy;

        auto createInfo = VkDeviceCreateInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO;
        createInfo.queueCreateInfoCount = static_cast<uint32_t>(queueCreateInfos.size());
        createInfo.pQueueCreateInfos = queueCreateInfos.data();
        
        createInfo.pEnabledFeatures = &deviceFeatures;

        auto deviceExtensionProperties = m_infoProvider->getAvailableVulkanDeviceExtensions(m_physicalDevice);
        auto missingExtensions = m_infoProvider->detectMissingRequiredDeviceExtensions(
            deviceExtensionProperties, 
            logicalDeviceSpec.requiredExtensions()
        );
        if (!missingExtensions.empty()) {
            auto errorMessage = std::string { "Vulkan does not have the required extensions on this system: " };
            for (const auto& extension : missingExtensions) {
                errorMessage.append(extension);
                errorMessage.append("\n");
            }

            throw std::runtime_error(errorMessage);
        }
        auto enabledExtensions = LogicalDeviceFactory::convertToCStrings(logicalDeviceSpec.requiredExtensions());
        createInfo.enabledExtensionCount = enabledExtensions.size();
        createInfo.ppEnabledExtensionNames = enabledExtensions.data();
        
        auto validationLayersCStrings = LogicalDeviceFactory::convertToCStrings(validationLayers);

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayersCStrings.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        VkDevice device = VK_NULL_HANDLE;
        auto result = vkCreateDevice(m_physicalDevice, &createInfo, nullptr, &device);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }


        VkQueue graphicsQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);

        VkQueue computeQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &computeQueue);
        
        VkQueue presentQueue = VK_NULL_HANDLE;
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);

        return std::make_tuple(device, graphicsQueue, computeQueue, presentQueue);
    }
private:
    VkPhysicalDevice m_physicalDevice;
    VkSurfaceKHR m_surface;
    PlatformInfoProvider* m_infoProvider;

    static std::vector<const char*> convertToCStrings(const std::vector<std::string>& strings) {
        auto cStrings = std::vector<const char*> {};
        cStrings.reserve(strings.size());
        std::transform(
            strings.begin(), 
            strings.end(), 
            std::back_inserter(cStrings),
            [](const std::string& string) { return string.c_str(); }
        );
    
        return cStrings;
    }
};

class VulkanDebugMessenger final {
public:
    explicit VulkanDebugMessenger()
        : m_instance { VK_NULL_HANDLE }
        , m_debugMessenger { VK_NULL_HANDLE }
    {
    }

    ~VulkanDebugMessenger() {
        this->cleanup();
    }

    static VulkanDebugMessenger* create(VkInstance instance) {
        if (instance == VK_NULL_HANDLE) {
            throw std::invalid_argument { "Got an empty `VkInstance` handle" };
        }

        uint32_t physicalDeviceCount = 0;
        auto result = vkEnumeratePhysicalDevices(instance, &physicalDeviceCount, nullptr);
        if (result != VK_SUCCESS) {
            throw std::invalid_argument { "Got an invalid `VkInstance` handle" };
        }

        auto createInfo = VkDebugUtilsMessengerCreateInfoEXT {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_INFO_BIT_EXT |
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = 
            VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | 
            VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;

        auto debugMessenger = static_cast<VkDebugUtilsMessengerEXT>(nullptr);
        result = VulkanDebugMessenger::CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger);
        if (result != VK_SUCCESS) {
            throw std::runtime_error { "failed to set up debug messenger!" };
        }

        auto vulkanDebugMessenger = new VulkanDebugMessenger {};
        vulkanDebugMessenger->m_instance = instance;
        vulkanDebugMessenger->m_debugMessenger = debugMessenger;

        return vulkanDebugMessenger;
    }

    static VkResult CreateDebugUtilsMessengerEXT(
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

    static void DestroyDebugUtilsMessengerEXT(
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

    static const std::string& messageSeverityToString(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity) {
        static const std::string MESSAGE_SEVERITY_INFO  = std::string { "INFO " };
        static const std::string MESSAGE_SEVERITY_WARN  = std::string { "WARN " };
        static const std::string MESSAGE_SEVERITY_ERROR = std::string { "ERROR" };

        if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT) {
            return MESSAGE_SEVERITY_ERROR;
        } else if (messageSeverity & VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT) {
            return MESSAGE_SEVERITY_WARN;
        } else {
            return MESSAGE_SEVERITY_INFO;
        }
    }

    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
        VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity,
        VkDebugUtilsMessageTypeFlagsEXT messageType,
        const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData,
        void* pUserData
    ) {
        auto messageSeverityString = VulkanDebugMessenger::messageSeverityToString(messageSeverity);
        fmt::println(std::cerr, "[{}] {}", messageSeverityString, pCallbackData->pMessage);

        return VK_FALSE;
    }

    void cleanup() {
        if (m_instance == VK_NULL_HANDLE) {
            return;
        }

        if (m_debugMessenger != VK_NULL_HANDLE) {
            VulkanDebugMessenger::DestroyDebugUtilsMessengerEXT(m_instance, m_debugMessenger, nullptr);
        }

        m_debugMessenger = VK_NULL_HANDLE;
        m_instance = VK_NULL_HANDLE;
    }
private:
    VkInstance m_instance;
    VkDebugUtilsMessengerEXT m_debugMessenger;
};

class SurfaceProvider final {
public:
    SurfaceProvider() = delete;
    SurfaceProvider(VkInstance instance, GLFWwindow* window)
        : m_instance { instance }
        , m_window { window }
    {
    }

    ~SurfaceProvider() {
        m_instance = VK_NULL_HANDLE;
        m_window = nullptr;
    }

    VkSurfaceKHR createSurface() {
        auto surface = VkSurfaceKHR {};
        auto result = glfwCreateWindowSurface(m_instance, m_window, nullptr, &surface);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }

        return surface;
    }
private:
    VkInstance m_instance;
    GLFWwindow* m_window;
};

class WindowSystem final {
public:
    explicit WindowSystem() = default;
    explicit WindowSystem(VkInstance instance) : m_instance { instance } {}
    ~WindowSystem() {
        m_framebufferResized = false;
        m_windowExtent = VkExtent2D { 0, 0 };
        m_surface = VK_NULL_HANDLE;

        glfwDestroyWindow(m_window);

        m_instance = VK_NULL_HANDLE;
    }

    static WindowSystem* create(VkInstance instance) {
        return new WindowSystem { instance };
    }

    void createWindow(uint32_t width, uint32_t height, const std::string& title) {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_TRUE);

        auto window = glfwCreateWindow(width, height, title.data(), nullptr, nullptr);
        glfwSetWindowUserPointer(window, this);
        glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);

        m_window = window;
        m_windowExtent = VkExtent2D { width, height };
    }

    GLFWwindow* getWindow() const {
        return m_window;
    }

    SurfaceProvider createSurfaceProvider() {
        return SurfaceProvider { m_instance, m_window};
    }

    bool hasFramebufferResized() const {
        return m_framebufferResized;
    }

    void setFramebufferResized(bool framebufferResized) {
        m_framebufferResized = framebufferResized;
    }

    void setWindowTitle(const std::string& title) {
        glfwSetWindowTitle(m_window, title.data());
    }
private:
    VkInstance m_instance;
    GLFWwindow* m_window;
    VkSurfaceKHR m_surface;
    VkExtent2D m_windowExtent;
    bool m_framebufferResized;

    static void framebufferResizeCallback(GLFWwindow* window, int width, int height) {
        auto windowSystem = reinterpret_cast<WindowSystem*>(glfwGetWindowUserPointer(window));
        windowSystem->m_framebufferResized = true;
        windowSystem->m_windowExtent = VkExtent2D { 
            static_cast<uint32_t>(width),
            static_cast<uint32_t>(height)
        };
    }
};

class GpuDevice final {
public:
    explicit GpuDevice() = delete;
    explicit GpuDevice(
        VkInstance instance,
        VkPhysicalDevice physicalDevice, 
        VkDevice device, 
        VkQueue graphicsQueue,
        VkQueue computeQueue,
        VkQueue presentQueue,
        VkCommandPool commandPool
    )   : m_instance { instance }
        , m_physicalDevice { physicalDevice }
        , m_device { device }
        , m_graphicsQueue { graphicsQueue }
        , m_computeQueue { computeQueue }
        , m_presentQueue { presentQueue }
        , m_commandPool { commandPool }
        , m_shaderModules { std::unordered_set<VkShaderModule> {} }
    {
        m_msaaSamples = GpuDevice::getMaxUsableSampleCount(physicalDevice);
    }

    ~GpuDevice() {
        for (const auto& shaderModule : m_shaderModules) {
            vkDestroyShaderModule(m_device, shaderModule, nullptr);
        }

        vkDestroySurfaceKHR(m_instance, m_surface, nullptr);
        vkDestroyCommandPool(m_device, m_commandPool, nullptr);
        vkDestroyDevice(m_device, nullptr);

        m_surface = VK_NULL_HANDLE;
        m_commandPool = VK_NULL_HANDLE;
        m_presentQueue = VK_NULL_HANDLE;
        m_graphicsQueue = VK_NULL_HANDLE;
        m_device = VK_NULL_HANDLE;
        m_physicalDevice = VK_NULL_HANDLE;
        m_instance = VK_NULL_HANDLE;
    }

    VkPhysicalDevice getPhysicalDevice() const {
        return m_physicalDevice;
    }

    VkDevice getLogicalDevice() const {
        return m_device;
    }

    VkQueue getGraphicsQueue() const {
        return m_graphicsQueue;
    }

    VkQueue getComputeQueue() const {
        return m_computeQueue;
    }

    VkQueue getPresentQueue() const {
        return m_presentQueue;
    }

    VkCommandPool getCommandPool() const {
        return m_commandPool;
    }

    VkSampleCountFlagBits getMsaaSamples() const {
        return m_msaaSamples;
    }

    static VkSampleCountFlagBits getMaxUsableSampleCount(VkPhysicalDevice physicalDevice) {
        VkPhysicalDeviceProperties physicalDeviceProperties;
        vkGetPhysicalDeviceProperties(physicalDevice, &physicalDeviceProperties);

        VkSampleCountFlags counts = physicalDeviceProperties.limits.framebufferColorSampleCounts & physicalDeviceProperties.limits.framebufferDepthSampleCounts;
        if (counts & VK_SAMPLE_COUNT_64_BIT) { return VK_SAMPLE_COUNT_64_BIT; }
        if (counts & VK_SAMPLE_COUNT_32_BIT) { return VK_SAMPLE_COUNT_32_BIT; }
        if (counts & VK_SAMPLE_COUNT_16_BIT) { return VK_SAMPLE_COUNT_16_BIT; }
        if (counts & VK_SAMPLE_COUNT_8_BIT) { return VK_SAMPLE_COUNT_8_BIT; }
        if (counts & VK_SAMPLE_COUNT_4_BIT) { return VK_SAMPLE_COUNT_4_BIT; }
        if (counts & VK_SAMPLE_COUNT_2_BIT) { return VK_SAMPLE_COUNT_2_BIT; }

        return VK_SAMPLE_COUNT_1_BIT;
    }

    VkSurfaceKHR createRenderSurface(SurfaceProvider& surfaceProvider) {
        auto surface = surfaceProvider.createSurface();

        m_surface = surface;

        return surface;
    }

    VkShaderModule createShaderModuleFromFile(const std::string& fileName) {
        auto shaderCode = this->loadShaderFromFile(fileName);

        return this->createShaderModule(shaderCode);
    }

    VkShaderModule createShaderModule(std::istream& stream) {
        auto shaderCode = this->loadShader(stream);

        return this->createShaderModule(shaderCode);
    }

    VkShaderModule createShaderModule(const std::vector<char>& code) {
        auto createInfo = VkShaderModuleCreateInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO;
        createInfo.codeSize = code.size();
        createInfo.pCode = reinterpret_cast<const uint32_t*>(code.data());
        createInfo.pNext = nullptr;
        createInfo.flags = 0;

        auto shaderModule = VkShaderModule {};
        auto result = vkCreateShaderModule(m_device, &createInfo, nullptr, &shaderModule);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create shader module!");
        }

        m_shaderModules.insert(shaderModule);

        return shaderModule;
    }
private:
    VkInstance m_instance;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_computeQueue;
    VkQueue m_presentQueue;
    VkCommandPool m_commandPool;
    VkSurfaceKHR m_surface;
    VkSampleCountFlagBits m_msaaSamples = VK_SAMPLE_COUNT_1_BIT;

    std::unordered_set<VkShaderModule> m_shaderModules;

    std::vector<char> loadShader(std::istream& stream) {
        size_t shaderSize = static_cast<size_t>(stream.tellg());
        auto buffer = std::vector<char>(shaderSize);

        stream.seekg(0);
        stream.read(buffer.data(), shaderSize);

        return buffer;
    }

    std::ifstream openShaderFile(const std::string& fileName) {
        auto file = std::ifstream { fileName, std::ios::ate | std::ios::binary };

        if (!file.is_open()) {
            throw std::runtime_error("failed to open file!");
        }

        return file;
    }

    std::vector<char> loadShaderFromFile(const std::string& fileName) {
        auto stream = this->openShaderFile(fileName);
        auto shader = this->loadShader(stream);
        stream.close();

        return shader;
    }
};

class GpuDeviceInitializer final {
public:
    explicit GpuDeviceInitializer(VkInstance instance)
        : m_instance { instance }
    {
        m_infoProvider = new PlatformInfoProvider {};
    }

    ~GpuDeviceInitializer() {
        vkDestroySurfaceKHR(m_instance, m_dummySurface, nullptr);

        m_instance = VK_NULL_HANDLE;
    }

    GpuDevice* createGpuDevice() {
        this->createDummySurface();
        this->selectPhysicalDevice();
        this->createLogicalDevice();
        this->createCommandPool();

        auto gpuDevice = new GpuDevice { m_instance, m_physicalDevice, m_device, m_graphicsQueue, m_computeQueue, m_presentQueue, m_commandPool };

        return gpuDevice;
    }
private:
    VkInstance m_instance;
    PlatformInfoProvider* m_infoProvider;
    VkSurfaceKHR m_dummySurface;
    VkPhysicalDevice m_physicalDevice;
    VkDevice m_device;
    VkQueue m_graphicsQueue;
    VkQueue m_computeQueue;
    VkQueue m_presentQueue;
    VkCommandPool m_commandPool;

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
        auto indices = QueueFamilyIndices {};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        auto queueFamilies = std::vector<VkQueueFamilyProperties> { queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.graphicsAndComputeFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

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

    void createDummySurface() {
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_VISIBLE, GLFW_FALSE);

        auto dummyWindow = glfwCreateWindow(1, 1, "DUMMY WINDOW", nullptr, nullptr);
        auto dummySurface = VkSurfaceKHR {};
        auto result = glfwCreateWindowSurface(m_instance, dummyWindow, nullptr, &dummySurface);
        if (result != VK_SUCCESS) {
            glfwDestroyWindow(dummyWindow);
            dummyWindow = nullptr;

            throw std::runtime_error("failed to create window surface!");
        }

        glfwDestroyWindow(dummyWindow);
        dummyWindow = nullptr;

        m_dummySurface = dummySurface;
    }

    void selectPhysicalDevice() {
        auto physicalDeviceSpecProvider = PhysicalDeviceSpecProvider {};
        auto physicalDeviceSpec = physicalDeviceSpecProvider.createPhysicalDeviceSpec();
        auto physicalDeviceSelector = PhysicalDeviceSelector { m_instance, m_infoProvider };
        auto selectedPhysicalDevice = physicalDeviceSelector.selectPhysicalDeviceForSurface(
            m_dummySurface,
            physicalDeviceSpec
        );

        m_physicalDevice = selectedPhysicalDevice;
    }

    void createLogicalDevice() {
        auto logicalDeviceSpecProvider = LogicalDeviceSpecProvider { m_physicalDevice, m_dummySurface };
        auto logicalDeviceSpec = logicalDeviceSpecProvider.createLogicalDeviceSpec();
        auto factory = LogicalDeviceFactory { m_physicalDevice, m_dummySurface, m_infoProvider };
        auto [device, graphicsQueue, computeQueue, presentQueue] = factory.createLogicalDevice(logicalDeviceSpec);

        m_device = device;
        m_graphicsQueue = graphicsQueue;
        m_computeQueue = computeQueue;
        m_presentQueue = presentQueue;
    }

    void createCommandPool() {
        auto queueFamilyIndices = this->findQueueFamilies(m_physicalDevice, m_dummySurface);

        auto poolInfo = VkCommandPoolCreateInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

        VkCommandPool commandPool = VK_NULL_HANDLE;
        auto result = vkCreateCommandPool(m_device, &poolInfo, nullptr, &commandPool);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create command pool!");
        }

        m_commandPool = commandPool;
    }
};

class Engine final {
public:
    explicit Engine() = default;
    ~Engine() {
        delete m_windowSystem;
        delete m_gpuDevice;
        delete m_debugMessenger;

        vkDestroyInstance(m_instance, nullptr);

        delete m_systemFactory;
        delete m_infoProvider;

        glfwTerminate();
    }

    static Engine* createDebugMode() {
        return Engine::create(true);
    }

    static Engine* createReleaseMode() {
        return Engine::create(false);
    }

    VkInstance getInstance() const {
        return m_instance;
    }

    VkPhysicalDevice getPhysicalDevice() const {
        return m_gpuDevice->getPhysicalDevice();
    }

    VkDevice getLogicalDevice() const {
        return m_gpuDevice->getLogicalDevice();
    }

    VkQueue getGraphicsQueue() const {
        return m_gpuDevice->getGraphicsQueue();
    }

    VkQueue getComputeQueue() const {
        return m_gpuDevice->getComputeQueue();
    }

    VkQueue getPresentQueue() const {
        return m_gpuDevice->getPresentQueue();
    }

    VkCommandPool getCommandPool() const {
        return m_gpuDevice->getCommandPool();
    }

    VkSurfaceKHR getSurface() const {
        return m_surface;
    }

    VkSampleCountFlagBits getMsaaSamples() const {
        return m_gpuDevice->getMsaaSamples();
    }

    GLFWwindow* getWindow() const {
        return m_windowSystem->getWindow();
    }

    bool hasFramebufferResized() const {
        return m_windowSystem->hasFramebufferResized();
    }

    void setFramebufferResized(bool framebufferResized) {
        m_windowSystem->setFramebufferResized(framebufferResized);
    }

    bool isInitialized() const {
        return m_instance != VK_NULL_HANDLE;
    }

    void createGLFWLibrary() {
        auto result = glfwInit();
        if (!result) {
            glfwTerminate();

            auto errorMessage = std::string { "Failed to initialize GLFW" };

            throw std::runtime_error { errorMessage };
        }
    }

    void createInfoProvider() {
        auto infoProvider = new PlatformInfoProvider {};

        m_infoProvider = infoProvider;
    }

    void createSystemFactory() {
        auto systemFactory = new SystemFactory {};

        m_systemFactory = systemFactory;
    }

    void createInstance() {
        auto instanceSpecProvider = InstanceSpecProvider { m_enableValidationLayers, m_enableDebuggingExtensions };
        auto instanceSpec = instanceSpecProvider.createInstanceSpec();
        auto instance = m_systemFactory->create(instanceSpec);
        
        m_instance = instance;
    }

    void createWindowSystem() {
        auto windowSystem = WindowSystem::create(m_instance);

        m_windowSystem = windowSystem;
    }

    void createDebugMessenger() {
        if (!m_enableValidationLayers) {
            return;
        }

        auto debugMessenger = VulkanDebugMessenger::create(m_instance);

        m_debugMessenger = debugMessenger;
    }

    void createWindow(uint32_t width, uint32_t height, const std::string& title) {
        m_windowSystem->createWindow(width, height, title);
       
        this->createRenderSurface();
    }

    void createGpuDevice() {
        auto gpuDeviceInitializer = GpuDeviceInitializer { m_instance };
        auto gpuDevice = gpuDeviceInitializer.createGpuDevice();

        m_gpuDevice = gpuDevice;
    }

    void createRenderSurface() {
        auto surfaceProvider = m_windowSystem->createSurfaceProvider();
        auto surface = m_gpuDevice->createRenderSurface(surfaceProvider);

        m_surface = surface;
    }

    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
        auto indices = QueueFamilyIndices {};

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, nullptr);

        auto queueFamilies = std::vector<VkQueueFamilyProperties> { queueFamilyCount };
        vkGetPhysicalDeviceQueueFamilyProperties(physicalDevice, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.graphicsAndComputeFamily = i;
            }

            VkBool32 presentSupport = false;
            vkGetPhysicalDeviceSurfaceSupportKHR(physicalDevice, i, surface, &presentSupport);

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

    SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice physicalDevice, VkSurfaceKHR surface) const {
        auto details = SwapChainSupportDetails {};
        vkGetPhysicalDeviceSurfaceCapabilitiesKHR(physicalDevice, surface, &details.capabilities);

        uint32_t formatCount = 0;
        vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, nullptr);

        if (formatCount != 0) {
            details.formats.resize(formatCount);
            vkGetPhysicalDeviceSurfaceFormatsKHR(physicalDevice, surface, &formatCount, details.formats.data());
        }

        uint32_t presentModeCount = 0;
        vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, nullptr);

        if (presentModeCount != 0) {
            details.presentModes.resize(presentModeCount);
            vkGetPhysicalDeviceSurfacePresentModesKHR(physicalDevice, surface, &presentModeCount, details.presentModes.data());
        }

        return details;
    }

    VkShaderModule createShaderModuleFromFile(const std::string& fileName) {
        return m_gpuDevice->createShaderModuleFromFile(fileName);
    }

    VkShaderModule createShaderModule(std::istream& stream) {
        return m_gpuDevice->createShaderModule(stream);
    }

    VkShaderModule createShaderModule(std::vector<char>& code) {
        return m_gpuDevice->createShaderModule(code);
    }
private:
    PlatformInfoProvider* m_infoProvider;
    SystemFactory* m_systemFactory;
    VkInstance m_instance;
    VulkanDebugMessenger* m_debugMessenger;
    WindowSystem* m_windowSystem;
    VkSurfaceKHR m_surface;

    GpuDevice* m_gpuDevice;

    bool m_enableValidationLayers; 
    bool m_enableDebuggingExtensions;

    static Engine* create(bool enableDebugging) {
        auto newEngine = new Engine {};

        if (enableDebugging) {
            newEngine->m_enableValidationLayers = true;
            newEngine->m_enableDebuggingExtensions = true;
        } else {
            newEngine->m_enableValidationLayers = false;
            newEngine->m_enableDebuggingExtensions = false;
        }

        newEngine->createGLFWLibrary();
        newEngine->createInfoProvider();
        newEngine->createSystemFactory();
        newEngine->createInstance();
        newEngine->createDebugMessenger();
        newEngine->createGpuDevice();
        newEngine->createWindowSystem();

        return newEngine;
    }
};



class App {
public:
    void run() {
        this->initEngine();
        this->mainLoop();
        this->cleanup();
    }
private:
    Engine* m_engine;

    VkImage m_colorImage;
    VkDeviceMemory m_colorImageMemory;
    VkImageView m_colorImageView;

    VkImage m_depthImage;
    VkDeviceMemory m_depthImageMemory;
    VkImageView m_depthImageView;

    uint32_t m_mipLevels;
    VkImage m_textureImage;
    VkDeviceMemory m_textureImageMemory;
    VkImageView m_textureImageView;
    VkSampler m_textureSampler;

    std::vector<Vertex> m_vertices;
    std::vector<uint32_t> m_indices;
    VkBuffer m_vertexBuffer;
    VkDeviceMemory m_vertexBufferMemory;
    VkBuffer m_indexBuffer;
    VkDeviceMemory m_indexBufferMemory;

    std::vector<VkBuffer> m_uniformBuffers;
    std::vector<VkDeviceMemory> m_uniformBuffersMemory;
    std::vector<void*> m_uniformBuffersMapped;

    VkDescriptorPool m_descriptorPool;
    std::vector<VkDescriptorSet> m_descriptorSets;
    VkDescriptorSetLayout m_descriptorSetLayout;

    std::vector<VkCommandBuffer> m_commandBuffers;

    VkRenderPass m_renderPass;
    VkPipelineLayout m_pipelineLayout;
    VkPipeline m_graphicsPipeline;

    std::vector<VkSemaphore> m_imageAvailableSemaphores;
    std::vector<VkSemaphore> m_renderFinishedSemaphores;
    std::vector<VkFence> m_inFlightFences;

    VkSwapchainKHR m_swapChain;
    std::vector<VkImage> m_swapChainImages;
    VkFormat m_swapChainImageFormat;
    VkExtent2D m_swapChainExtent;
    std::vector<VkImageView> m_swapChainImageViews;
    std::vector<VkFramebuffer> m_swapChainFramebuffers;
    
    uint32_t m_currentFrame = 0;

    bool m_enableValidationLayers { false };
    bool m_enableDebuggingExtensions { false };

    void cleanup() {
        if (m_engine->isInitialized()) {
            this->cleanupSwapChain();

            vkDestroyPipeline(m_engine->getLogicalDevice(), m_graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->getLogicalDevice(), m_pipelineLayout, nullptr);
            vkDestroyRenderPass(m_engine->getLogicalDevice(), m_renderPass, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(m_engine->getLogicalDevice(), m_renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(m_engine->getLogicalDevice(), m_imageAvailableSemaphores[i], nullptr);
                vkDestroyFence(m_engine->getLogicalDevice(), m_inFlightFences[i], nullptr);
            }

            vkDestroyDescriptorPool(m_engine->getLogicalDevice(), m_descriptorPool, nullptr);

            vkDestroySampler(m_engine->getLogicalDevice(), m_textureSampler, nullptr);
            vkDestroyImageView(m_engine->getLogicalDevice(), m_textureImageView, nullptr);

            vkDestroyImage(m_engine->getLogicalDevice(), m_textureImage, nullptr);
            vkFreeMemory(m_engine->getLogicalDevice(), m_textureImageMemory, nullptr);

            vkDestroyDescriptorSetLayout(m_engine->getLogicalDevice(), m_descriptorSetLayout, nullptr);

            vkDestroyBuffer(m_engine->getLogicalDevice(), m_indexBuffer, nullptr);
            vkFreeMemory(m_engine->getLogicalDevice(), m_indexBufferMemory, nullptr);

            vkDestroyBuffer(m_engine->getLogicalDevice(), m_vertexBuffer, nullptr);
            vkFreeMemory(m_engine->getLogicalDevice(), m_vertexBufferMemory, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroyBuffer(m_engine->getLogicalDevice(), m_uniformBuffers[i], nullptr);
                vkFreeMemory(m_engine->getLogicalDevice(), m_uniformBuffersMemory[i], nullptr);
            }

            delete m_engine;
        }
    }

    void createEngine() {
        auto engine = Engine::createDebugMode();
        engine->createWindow(WIDTH, HEIGHT, "Hello, Triangle!");

        m_engine = engine;
    }

    void initEngine() {
        this->createEngine();

        this->createTextureImage();
        this->createTextureImageView();
        this->createTextureSampler();
        this->loadModel();
        this->createVertexBuffer();
        this->createIndexBuffer();
        this->createDescriptorSetLayout();
        this->createUniformBuffers();
        this->createDescriptorPool();
        this->createDescriptorSets();
        this->createCommandBuffers();
        this->createSwapChain();
        this->createImageViews();
        this->createRenderPass();
        this->createGraphicsPipeline();
        this->createColorResources();
        this->createDepthResources();
        this->createFramebuffers();
        this->createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(m_engine->getWindow())) {
            glfwPollEvents();
            this->draw();
        }

        vkDeviceWaitIdle(m_engine->getLogicalDevice());
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_engine->getPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        auto bufferInfo = VkBufferCreateInfo {};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        auto result = vkCreateBuffer(m_engine->getLogicalDevice(), &bufferInfo, nullptr, &buffer);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        auto memRequirements = VkMemoryRequirements {};
        vkGetBufferMemoryRequirements(m_engine->getLogicalDevice(), buffer, &memRequirements);

        auto allocInfo = VkMemoryAllocateInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        result = vkAllocateMemory(m_engine->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(m_engine->getLogicalDevice(), buffer, bufferMemory, 0);
    }

    void createImage(uint32_t width, uint32_t height, uint32_t mipLevels, VkSampleCountFlagBits numSamples, VkFormat format, VkImageTiling tiling, VkImageUsageFlags usage, VkMemoryPropertyFlags properties, VkImage& image, VkDeviceMemory& imageMemory) {
        auto imageInfo = VkImageCreateInfo {};
        imageInfo.sType = VK_STRUCTURE_TYPE_IMAGE_CREATE_INFO;
        imageInfo.imageType = VK_IMAGE_TYPE_2D;
        imageInfo.extent.width = width;
        imageInfo.extent.height = height;
        imageInfo.extent.depth = 1;
        imageInfo.mipLevels = mipLevels;
        imageInfo.arrayLayers = 1;
        imageInfo.format = format;
        imageInfo.tiling = tiling;
        imageInfo.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        imageInfo.usage = usage;
        imageInfo.samples = numSamples;
        imageInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateImage(m_engine->getLogicalDevice(), &imageInfo, nullptr, &image) != VK_SUCCESS) {
            throw std::runtime_error("failed to create image!");
        }

        VkMemoryRequirements memRequirements;
        vkGetImageMemoryRequirements(m_engine->getLogicalDevice(), image, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_engine->getLogicalDevice(), &allocInfo, nullptr, &imageMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate image memory!");
        }

        vkBindImageMemory(m_engine->getLogicalDevice(), image, imageMemory, 0);
    }

    VkCommandBuffer beginSingleTimeCommands() {
        auto allocInfo = VkCommandBufferAllocateInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_engine->getCommandPool();
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_engine->getLogicalDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        return commandBuffer;
    }

    void endSingleTimeCommands(VkCommandBuffer commandBuffer) {
        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_engine->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_engine->getGraphicsQueue());

        vkFreeCommandBuffers(m_engine->getLogicalDevice(), m_engine->getCommandPool(), 1, &commandBuffer);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        this->endSingleTimeCommands(commandBuffer);
    }

    void transitionImageLayout(VkImage image, VkFormat format, VkImageLayout oldLayout, VkImageLayout newLayout, uint32_t mipLevels) {
        VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();

        VkImageMemoryBarrier barrier{};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.oldLayout = oldLayout;
        barrier.newLayout = newLayout;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.image = image;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseMipLevel = 0;
        barrier.subresourceRange.levelCount = mipLevels;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;

        VkPipelineStageFlags sourceStage;
        VkPipelineStageFlags destinationStage;

        if (oldLayout == VK_IMAGE_LAYOUT_UNDEFINED && newLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL) {
            barrier.srcAccessMask = 0;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;

            sourceStage = VK_PIPELINE_STAGE_TOP_OF_PIPE_BIT;
            destinationStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
        } else if (oldLayout == VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL && newLayout == VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL) {
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            sourceStage = VK_PIPELINE_STAGE_TRANSFER_BIT;
            destinationStage = VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT;
        } else {
            throw std::invalid_argument("unsupported layout transition!");
        }

        vkCmdPipelineBarrier(
            commandBuffer,
            sourceStage, destinationStage,
            0,
            0, nullptr,
            0, nullptr,
            1, &barrier
        );

        this->endSingleTimeCommands(commandBuffer);
    }

    void copyBufferToImage(VkBuffer buffer, VkImage image, uint32_t width, uint32_t height) {
        VkCommandBuffer commandBuffer = beginSingleTimeCommands();

        VkBufferImageCopy region{};
        region.bufferOffset = 0;
        region.bufferRowLength = 0;
        region.bufferImageHeight = 0;
        region.imageSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        region.imageSubresource.mipLevel = 0;
        region.imageSubresource.baseArrayLayer = 0;
        region.imageSubresource.layerCount = 1;
        region.imageOffset = {0, 0, 0};
        region.imageExtent = {
            width,
            height,
            1
        };

        vkCmdCopyBufferToImage(commandBuffer, buffer, image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL, 1, &region);

        endSingleTimeCommands(commandBuffer);
    }

    VkImageView createImageView(VkImage image, VkFormat format, VkImageAspectFlags aspectFlags, uint32_t mipLevels) {
        auto viewInfo = VkImageViewCreateInfo {};
        viewInfo.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO;
        viewInfo.image = image;
        viewInfo.viewType = VK_IMAGE_VIEW_TYPE_2D;
        viewInfo.format = format;
        viewInfo.components = VkComponentMapping { VK_COMPONENT_SWIZZLE_IDENTITY }; // Optional
        viewInfo.subresourceRange.aspectMask = aspectFlags;
        viewInfo.subresourceRange.baseMipLevel = 0;
        viewInfo.subresourceRange.levelCount = mipLevels;
        viewInfo.subresourceRange.baseArrayLayer = 0;
        viewInfo.subresourceRange.layerCount = 1;

        auto imageView = VkImageView {};
        auto result = vkCreateImageView(m_engine->getLogicalDevice(), &viewInfo, nullptr, &imageView);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture image view!");
        }

        return imageView;
    }

    VkFormat findSupportedFormat(const std::vector<VkFormat>& candidates, VkImageTiling tiling, VkFormatFeatureFlags features) {
        for (VkFormat format : candidates) {
            auto props = VkFormatProperties {};
            vkGetPhysicalDeviceFormatProperties(m_engine->getPhysicalDevice(), format, &props);

            if (tiling == VK_IMAGE_TILING_LINEAR && (props.linearTilingFeatures & features) == features) {
                return format;
            } else if (tiling == VK_IMAGE_TILING_OPTIMAL && (props.optimalTilingFeatures & features) == features) {
                return format;
            }
        }

        throw std::runtime_error("failed to find supported format!");
    }

    VkFormat findDepthFormat() {
        auto candidates = std::vector<VkFormat> { 
            VK_FORMAT_D32_SFLOAT,
            VK_FORMAT_D32_SFLOAT_S8_UINT,
            VK_FORMAT_D24_UNORM_S8_UINT
        };
        auto tiling = VK_IMAGE_TILING_OPTIMAL;
        auto features = VK_FORMAT_FEATURE_DEPTH_STENCIL_ATTACHMENT_BIT;
        
        return this->findSupportedFormat(candidates, tiling, features);
    }

    bool hasStencilComponent(VkFormat format) {
        return format == VK_FORMAT_D32_SFLOAT_S8_UINT || format == VK_FORMAT_D24_UNORM_S8_UINT;
    }

    void createColorResources() {
        VkFormat colorFormat = m_swapChainImageFormat;

        VkImage colorImage;
        VkDeviceMemory colorImageMemory;
        this->createImage(
            m_swapChainExtent.width,
            m_swapChainExtent.height,
            1,
            m_engine->getMsaaSamples(), 
            colorFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSIENT_ATTACHMENT_BIT | VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            colorImage,
            colorImageMemory
        );
        auto colorImageView = this->createImageView(colorImage, colorFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);

        m_colorImage = colorImage;
        m_colorImageMemory = colorImageMemory;
        m_colorImageView = colorImageView;
    }

    void createDepthResources() {
        VkFormat depthFormat = this->findDepthFormat();

        auto depthImage = VkImage {};
        auto depthImageMemory = VkDeviceMemory {};
        this->createImage(
            m_swapChainExtent.width,
            m_swapChainExtent.height,
            1,
            m_engine->getMsaaSamples(),
            depthFormat,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_DEPTH_STENCIL_ATTACHMENT_BIT, 
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, 
            depthImage,
            depthImageMemory
        );
        auto depthImageView = this->createImageView(depthImage, depthFormat, VK_IMAGE_ASPECT_DEPTH_BIT, 1);

        m_depthImage = depthImage;
        m_depthImageMemory = depthImageMemory;
        m_depthImageView = depthImageView;
    }

    void generateMipmaps(VkImage image, VkFormat imageFormat, int32_t texWidth, int32_t texHeight, uint32_t mipLevels) {
        // Check if image format supports linear blitting.
        auto formatProperties = VkFormatProperties {};
        vkGetPhysicalDeviceFormatProperties(m_engine->getPhysicalDevice(), imageFormat, &formatProperties);

        if (!(formatProperties.optimalTilingFeatures & VK_FORMAT_FEATURE_SAMPLED_IMAGE_FILTER_LINEAR_BIT)) {
            throw std::runtime_error("texture image format does not support linear blitting!");
        }

        VkCommandBuffer commandBuffer = this->beginSingleTimeCommands();

        auto barrier = VkImageMemoryBarrier {};
        barrier.sType = VK_STRUCTURE_TYPE_IMAGE_MEMORY_BARRIER;
        barrier.image = image;
        barrier.srcQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.dstQueueFamilyIndex = VK_QUEUE_FAMILY_IGNORED;
        barrier.subresourceRange.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
        barrier.subresourceRange.baseArrayLayer = 0;
        barrier.subresourceRange.layerCount = 1;
        barrier.subresourceRange.levelCount = 1;

        int32_t mipWidth = texWidth;
        int32_t mipHeight = texHeight;

        for (uint32_t i = 1; i < mipLevels; i++) {
            barrier.subresourceRange.baseMipLevel = i - 1;
            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
            barrier.dstAccessMask = VK_ACCESS_TRANSFER_READ_BIT;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_TRANSFER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            auto blit = VkImageBlit {};
            blit.srcOffsets[0] = { 0, 0, 0 };
            blit.srcOffsets[1] = { mipWidth, mipHeight, 1 };
            blit.srcSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.srcSubresource.mipLevel = i - 1;
            blit.srcSubresource.baseArrayLayer = 0;
            blit.srcSubresource.layerCount = 1;
            blit.dstOffsets[0] = { 0, 0, 0 };
            blit.dstOffsets[1] = { mipWidth > 1 ? mipWidth / 2 : 1, mipHeight > 1 ? mipHeight / 2 : 1, 1 };
            blit.dstSubresource.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT;
            blit.dstSubresource.mipLevel = i;
            blit.dstSubresource.baseArrayLayer = 0;
            blit.dstSubresource.layerCount = 1;

            vkCmdBlitImage(
                commandBuffer,
                image, VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL,
                image, VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
                1, &blit,
                VK_FILTER_LINEAR
            );

            barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_SRC_OPTIMAL;
            barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            barrier.srcAccessMask = VK_ACCESS_TRANSFER_READ_BIT;
            barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

            vkCmdPipelineBarrier(
                commandBuffer,
                VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
                0, nullptr,
                0, nullptr,
                1, &barrier
            );

            if (mipWidth > 1) mipWidth /= 2;
            if (mipHeight > 1) mipHeight /= 2;
        }

        barrier.subresourceRange.baseMipLevel = mipLevels - 1;
        barrier.oldLayout = VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL;
        barrier.newLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
        barrier.srcAccessMask = VK_ACCESS_TRANSFER_WRITE_BIT;
        barrier.dstAccessMask = VK_ACCESS_SHADER_READ_BIT;

        vkCmdPipelineBarrier(
            commandBuffer,
            VK_PIPELINE_STAGE_TRANSFER_BIT, VK_PIPELINE_STAGE_FRAGMENT_SHADER_BIT, 0,
            0, nullptr,
            0, nullptr,
            1, &barrier);

        this->endSingleTimeCommands(commandBuffer);
    }

    void createTextureImage() {
        int textureWidth = 0;
        int textureHeight = 0;
        int textureChannels = 0;
        stbi_uc* pixels = stbi_load(TEXTURE_PATH.c_str(), &textureWidth, &textureHeight, &textureChannels, STBI_rgb_alpha);
        VkDeviceSize imageSize = textureWidth * textureHeight * 4;

        auto mipLevels = static_cast<uint32_t>(std::floor(std::log2(std::max(textureWidth, textureHeight)))) + 1;

        if (!pixels) {
            throw std::runtime_error("failed to load texture image!");
        }

        auto stagingBuffer = VkBuffer {};
        auto stagingBufferMemory = VkDeviceMemory {};
        this->createBuffer(
            imageSize,
            VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
            VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
            stagingBuffer,
            stagingBufferMemory
        );

        void* data;
        vkMapMemory(m_engine->getLogicalDevice(), stagingBufferMemory, 0, imageSize, 0, &data);
        memcpy(data, pixels, static_cast<size_t>(imageSize));
        vkUnmapMemory(m_engine->getLogicalDevice(), stagingBufferMemory);

        stbi_image_free(pixels);

        VkImage textureImage;
        VkDeviceMemory textureImageMemory;
        this->createImage(
            textureWidth,
            textureHeight,
            mipLevels,
            VK_SAMPLE_COUNT_1_BIT,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_TILING_OPTIMAL,
            VK_IMAGE_USAGE_TRANSFER_SRC_BIT | VK_IMAGE_USAGE_TRANSFER_DST_BIT | VK_IMAGE_USAGE_SAMPLED_BIT,
            VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT,
            textureImage,
            textureImageMemory
        );

        this->transitionImageLayout(
            textureImage,
            VK_FORMAT_R8G8B8A8_SRGB,
            VK_IMAGE_LAYOUT_UNDEFINED,
            VK_IMAGE_LAYOUT_TRANSFER_DST_OPTIMAL,
            mipLevels
        );
        this->copyBufferToImage(
            stagingBuffer,
            textureImage,
            static_cast<uint32_t>(textureWidth),
            static_cast<uint32_t>(textureHeight)
        );
        // Transitioned to `VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL` while generating mipmaps.

        vkDestroyBuffer(m_engine->getLogicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_engine->getLogicalDevice(), stagingBufferMemory, nullptr);

        this->generateMipmaps(textureImage, VK_FORMAT_R8G8B8A8_SRGB, textureWidth, textureHeight, mipLevels);

        m_textureImage = textureImage;
        m_textureImageMemory = textureImageMemory;
        m_mipLevels = mipLevels;
    }

    void createTextureImageView() {
        auto textureImageView = createImageView(m_textureImage, VK_FORMAT_R8G8B8A8_SRGB, VK_IMAGE_ASPECT_COLOR_BIT, m_mipLevels);

        m_textureImageView = textureImageView;
    }

    void createTextureSampler() {
        auto properties = VkPhysicalDeviceProperties {};
        vkGetPhysicalDeviceProperties(m_engine->getPhysicalDevice(), &properties);

        auto samplerInfo = VkSamplerCreateInfo {};
        samplerInfo.sType = VK_STRUCTURE_TYPE_SAMPLER_CREATE_INFO;
        samplerInfo.magFilter = VK_FILTER_LINEAR;
        samplerInfo.minFilter = VK_FILTER_LINEAR;
        samplerInfo.addressModeU = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeV = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.addressModeW = VK_SAMPLER_ADDRESS_MODE_REPEAT;
        samplerInfo.anisotropyEnable = VK_TRUE;
        samplerInfo.maxAnisotropy = properties.limits.maxSamplerAnisotropy;
        samplerInfo.borderColor = VK_BORDER_COLOR_INT_OPAQUE_BLACK;
        samplerInfo.unnormalizedCoordinates = VK_FALSE;
        samplerInfo.compareEnable = VK_FALSE;
        samplerInfo.compareOp = VK_COMPARE_OP_ALWAYS;
        samplerInfo.mipmapMode = VK_SAMPLER_MIPMAP_MODE_LINEAR;
        samplerInfo.minLod = 0.0f;
        samplerInfo.maxLod = static_cast<float>(m_mipLevels);
        samplerInfo.mipLodBias = 0.0f;
        /*
        // Use there parameters to disable anisotropic filtering.
        samplerInfo.anisotropyEnable = VK_FALSE;
        samplerInfo.maxAnisotropy = 1.0f;
        */

        auto textureSampler = VkSampler {};
        auto result = vkCreateSampler(m_engine->getLogicalDevice(), &samplerInfo, nullptr, &textureSampler);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create texture sampler!");
        }

        m_textureSampler = textureSampler;
    }

    void loadModel() {
        auto attrib = tinyobj::attrib_t {};
        auto shapes = std::vector<tinyobj::shape_t> {};
        auto materials = std::vector<tinyobj::material_t> {};
        auto warn = std::string {}; 
        auto err = std::string {};

        if (!tinyobj::LoadObj(&attrib, &shapes, &materials, &warn, &err, MODEL_PATH.c_str())) {
            throw std::runtime_error(warn + err);
        }

        auto uniqueVertices = std::unordered_map<Vertex, uint32_t> {};
        auto vertices = std::vector<Vertex> {};
        auto indices = std::vector<uint32_t> {};
        for (const auto& shape : shapes) {
            for (const auto& index : shape.mesh.indices) {
                auto vertex = Vertex {};

                vertex.pos = glm::vec3 {
                    attrib.vertices[3 * index.vertex_index + 0],
                    attrib.vertices[3 * index.vertex_index + 1],
                    attrib.vertices[3 * index.vertex_index + 2]
                };

                vertex.texCoord = glm::vec2 {
                    attrib.texcoords[2 * index.texcoord_index + 0],
                    1.0f - attrib.texcoords[2 * index.texcoord_index + 1]
                };

                vertex.color = glm::vec3 { 1.0f, 1.0f, 1.0f };

                if (uniqueVertices.count(vertex) == 0) {
                    uniqueVertices[vertex] = static_cast<uint32_t>(vertices.size());
                    vertices.push_back(vertex);
                }

                indices.push_back(uniqueVertices[vertex]);
            }
        }

        m_vertices = vertices;
        m_indices = indices;
    }

    void createVertexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_vertices[0]) * m_vertices.size();

        auto stagingBuffer = VkBuffer {};
        auto stagingBufferMemory = VkDeviceMemory {};
        VkBufferUsageFlags stagingBufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags stagingBufferPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        this->createBuffer(bufferSize, stagingBufferUsageFlags, stagingBufferPropertyFlags, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(m_engine->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        
        memcpy(data, m_vertices.data(), (size_t) bufferSize);
        
        vkUnmapMemory(m_engine->getLogicalDevice(), stagingBufferMemory);

        VkBufferUsageFlags vertexBufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | 
            VK_BUFFER_USAGE_VERTEX_BUFFER_BIT;
        VkMemoryPropertyFlags vertexBufferPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        this->createBuffer(bufferSize, vertexBufferUsageFlags, vertexBufferPropertyFlags, m_vertexBuffer, m_vertexBufferMemory);

        this->copyBuffer(stagingBuffer, m_vertexBuffer, bufferSize);

        vkDestroyBuffer(m_engine->getLogicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_engine->getLogicalDevice(), stagingBufferMemory, nullptr);
    }

    void createIndexBuffer() {
        VkDeviceSize bufferSize = sizeof(m_indices[0]) * m_indices.size();

        auto stagingBuffer = VkBuffer {};
        auto stagingBufferMemory = VkDeviceMemory {};
        VkBufferUsageFlags stagingBufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_SRC_BIT;
        VkMemoryPropertyFlags stagingBufferPropertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | 
            VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
        this->createBuffer(bufferSize, stagingBufferUsageFlags, stagingBufferPropertyFlags, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(m_engine->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        
        memcpy(data, m_indices.data(), static_cast<size_t>(bufferSize));
        
        vkUnmapMemory(m_engine->getLogicalDevice(), stagingBufferMemory);

        VkBufferUsageFlags vertexBufferUsageFlags = VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT;
        VkMemoryPropertyFlags vertexBufferPropertyFlags = VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT;
        this->createBuffer(bufferSize, vertexBufferUsageFlags, vertexBufferPropertyFlags, m_indexBuffer, m_indexBufferMemory);

        this->copyBuffer(stagingBuffer, m_indexBuffer, bufferSize);

        vkDestroyBuffer(m_engine->getLogicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_engine->getLogicalDevice(), stagingBufferMemory, nullptr);
    }

    void createDescriptorSetLayout() {
        auto uboLayoutBinding = VkDescriptorSetLayoutBinding {};
        uboLayoutBinding.binding = 0;
        uboLayoutBinding.descriptorCount = 1;
        uboLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        uboLayoutBinding.pImmutableSamplers = nullptr;
        uboLayoutBinding.stageFlags = VK_SHADER_STAGE_VERTEX_BIT;

        auto samplerLayoutBinding = VkDescriptorSetLayoutBinding {};
        samplerLayoutBinding.binding = 1;
        samplerLayoutBinding.descriptorCount = 1;
        samplerLayoutBinding.descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        samplerLayoutBinding.pImmutableSamplers = nullptr;
        samplerLayoutBinding.stageFlags = VK_SHADER_STAGE_FRAGMENT_BIT;

        auto bindings = std::array<VkDescriptorSetLayoutBinding, 2> { uboLayoutBinding, samplerLayoutBinding };
        auto layoutInfo = VkDescriptorSetLayoutCreateInfo {};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = static_cast<uint32_t>(bindings.size());
        layoutInfo.pBindings = bindings.data();

        auto descriptorSetLayout = VkDescriptorSetLayout {};
        auto result = vkCreateDescriptorSetLayout(m_engine->getLogicalDevice(), &layoutInfo, nullptr, &descriptorSetLayout);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor set layout!");
        }

        m_descriptorSetLayout = descriptorSetLayout;
    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(UniformBufferObject);

        m_uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        m_uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkBufferUsageFlags usageFlags = VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT;
            VkMemoryPropertyFlags propertyFlags = VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT;
            this->createBuffer(bufferSize, usageFlags, propertyFlags, m_uniformBuffers[i], m_uniformBuffersMemory[i]);
            
            vkMapMemory(m_engine->getLogicalDevice(), m_uniformBuffersMemory[i], 0, bufferSize, 0, &m_uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool() {
        auto poolSizes = std::array<VkDescriptorPoolSize, 2> {};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        auto poolInfo = VkDescriptorPoolCreateInfo {};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = static_cast<uint32_t>(poolSizes.size());
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        auto descriptorPool = VkDescriptorPool {};
        auto result = vkCreateDescriptorPool(m_engine->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }

        m_descriptorPool = descriptorPool;
    }

    void createDescriptorSets() {
        auto layouts = std::vector<VkDescriptorSetLayout> { MAX_FRAMES_IN_FLIGHT, m_descriptorSetLayout };
        auto allocInfo = VkDescriptorSetAllocateInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = m_descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        m_descriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        auto result = vkAllocateDescriptorSets(m_engine->getLogicalDevice(), &allocInfo, m_descriptorSets.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            auto bufferInfo = VkDescriptorBufferInfo {};
            bufferInfo.buffer = m_uniformBuffers[i];
            bufferInfo.offset = 0;
            bufferInfo.range = sizeof(UniformBufferObject);

            auto imageInfo = VkDescriptorImageInfo {};
            imageInfo.imageLayout = VK_IMAGE_LAYOUT_SHADER_READ_ONLY_OPTIMAL;
            imageInfo.imageView = m_textureImageView;
            imageInfo.sampler = m_textureSampler;

            auto descriptorWrites = std::array<VkWriteDescriptorSet, 2> {};

            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = m_descriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &bufferInfo;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = m_descriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_COMBINED_IMAGE_SAMPLER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pImageInfo = &imageInfo;

            vkUpdateDescriptorSets(
                m_engine->getLogicalDevice(),
                static_cast<uint32_t>(descriptorWrites.size()),
                descriptorWrites.data(),
                0,
                nullptr
            );
        }
    }

    void createCommandBuffers() {
        m_commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        auto allocInfo = VkCommandBufferAllocateInfo {};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_engine->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = static_cast<uint32_t>(m_commandBuffers.size());

        auto result = vkAllocateCommandBuffers(m_engine->getLogicalDevice(), &allocInfo, m_commandBuffers.data());
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    VkSurfaceFormatKHR selectSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR>& availableFormats) {
        for (const auto& availableFormat : availableFormats) {
            if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB && availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
                return availableFormat;
            }
        }

        return availableFormats[0];
    }

    VkPresentModeKHR selectSwapPresentMode(const std::vector<VkPresentModeKHR>& availablePresentModes) {
        for (const auto& availablePresentMode : availablePresentModes) {
            if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
                return availablePresentMode;
            }
        }

        // We would probably want to use `VK_PRESENT_MODE_FIFO_KHR` on mobile devices.
        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D selectSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width, height;
            glfwGetWindowSize(m_engine->getWindow(), &width, &height);

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

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = m_engine->querySwapChainSupport(m_engine->getPhysicalDevice(), m_engine->getSurface());
        VkSurfaceFormatKHR surfaceFormat = this->selectSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = this->selectSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = this->selectSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        auto createInfo = VkSwapchainCreateInfoKHR {};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_engine->getSurface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = m_engine->findQueueFamilies(m_engine->getPhysicalDevice(), m_engine->getSurface());
        auto queueFamilyIndices = std::array<uint32_t, 2> { 
            indices.graphicsAndComputeFamily.value(),
            indices.presentFamily.value()
        };

        if (indices.graphicsAndComputeFamily != indices.presentFamily) {
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

        if (vkCreateSwapchainKHR(m_engine->getLogicalDevice(), &createInfo, nullptr, &m_swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_engine->getLogicalDevice(), m_swapChain, &imageCount, nullptr);
        m_swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_engine->getLogicalDevice(), m_swapChain, &imageCount, m_swapChainImages.data());

        m_swapChainImageFormat = surfaceFormat.format;
        m_swapChainExtent = extent;
    }

    void createImageViews() {
        m_swapChainImageViews.resize(m_swapChainImages.size());

        for (size_t i = 0; i < m_swapChainImages.size(); i++) {
            auto swapChainImageView = this->createImageView(m_swapChainImages[i], m_swapChainImageFormat, VK_IMAGE_ASPECT_COLOR_BIT, 1);
            m_swapChainImageViews[i] = swapChainImageView;
        }
    }

    void createRenderPass() {
        auto colorAttachment = VkAttachmentDescription {};
        colorAttachment.format = m_swapChainImageFormat;
        colorAttachment.samples = m_engine->getMsaaSamples();
        colorAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        colorAttachment.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;
        /*
        colorAttachment.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;
        */

        auto depthAttachment = VkAttachmentDescription {};
        depthAttachment.format = this->findDepthFormat();
        depthAttachment.samples = m_engine->getMsaaSamples();
        depthAttachment.loadOp = VK_ATTACHMENT_LOAD_OP_CLEAR;
        depthAttachment.storeOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        depthAttachment.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        depthAttachment.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        depthAttachment.finalLayout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        auto colorAttachmentResolve = VkAttachmentDescription {};
        colorAttachmentResolve.format = m_swapChainImageFormat;
        colorAttachmentResolve.samples = VK_SAMPLE_COUNT_1_BIT;
        colorAttachmentResolve.loadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.storeOp = VK_ATTACHMENT_STORE_OP_STORE;
        colorAttachmentResolve.stencilLoadOp = VK_ATTACHMENT_LOAD_OP_DONT_CARE;
        colorAttachmentResolve.stencilStoreOp = VK_ATTACHMENT_STORE_OP_DONT_CARE;
        colorAttachmentResolve.initialLayout = VK_IMAGE_LAYOUT_UNDEFINED;
        colorAttachmentResolve.finalLayout = VK_IMAGE_LAYOUT_PRESENT_SRC_KHR;

        auto colorAttachmentRef = VkAttachmentReference {};
        colorAttachmentRef.attachment = 0;
        colorAttachmentRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        auto depthAttachmentRef = VkAttachmentReference {};
        depthAttachmentRef.attachment = 1;
        depthAttachmentRef.layout = VK_IMAGE_LAYOUT_DEPTH_STENCIL_ATTACHMENT_OPTIMAL;

        auto colorAttachmentResolveRef = VkAttachmentReference {};
        colorAttachmentResolveRef.attachment = 2;
        colorAttachmentResolveRef.layout = VK_IMAGE_LAYOUT_COLOR_ATTACHMENT_OPTIMAL;

        auto subpass = VkSubpassDescription {};
        subpass.pipelineBindPoint = VK_PIPELINE_BIND_POINT_GRAPHICS;
        subpass.colorAttachmentCount = 1;
        subpass.pColorAttachments = &colorAttachmentRef;
        subpass.pDepthStencilAttachment = &depthAttachmentRef;
        subpass.pResolveAttachments = &colorAttachmentResolveRef;

        auto dependency = VkSubpassDependency {};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT | VK_PIPELINE_STAGE_EARLY_FRAGMENT_TESTS_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT | VK_ACCESS_DEPTH_STENCIL_ATTACHMENT_WRITE_BIT;

        auto attachments = std::array<VkAttachmentDescription, 3> { colorAttachment, depthAttachment, colorAttachmentResolve };
        auto renderPassInfo = VkRenderPassCreateInfo {};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
        renderPassInfo.pAttachments = attachments.data();
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        auto result = vkCreateRenderPass(m_engine->getLogicalDevice(), &renderPassInfo, nullptr, &m_renderPass);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createGraphicsPipeline() {
        auto vertexShaderModule = m_engine->createShaderModuleFromFile("shaders/shader.vert.hlsl.spv");
        auto fragmentShaderModule = m_engine->createShaderModuleFromFile("shaders/shader.frag.hlsl.spv");

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

        auto bindingDescription = Vertex::getBindingDescription();
        auto attributeDescriptions = Vertex::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

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
        /*
        rasterizer.frontFace = VK_FRONT_FACE_CLOCKWISE;
        */
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        auto multisampling = VkPipelineMultisampleStateCreateInfo {};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = m_engine->getMsaaSamples();
        multisampling.pSampleMask = nullptr;            // Optional.
        multisampling.alphaToCoverageEnable = VK_FALSE; // Optional.
        multisampling.alphaToOneEnable = VK_FALSE;      // Optional.

        auto depthStencil = VkPipelineDepthStencilStateCreateInfo {};
        depthStencil.sType = VK_STRUCTURE_TYPE_PIPELINE_DEPTH_STENCIL_STATE_CREATE_INFO;
        depthStencil.depthTestEnable = VK_TRUE;
        depthStencil.depthWriteEnable = VK_TRUE;
        depthStencil.depthCompareOp = VK_COMPARE_OP_LESS;
        depthStencil.depthBoundsTestEnable = VK_FALSE;
        depthStencil.stencilTestEnable = VK_FALSE;

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
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &m_descriptorSetLayout;
        pipelineLayoutInfo.pushConstantRangeCount = 0;    // Optional
        pipelineLayoutInfo.pPushConstantRanges = nullptr; // Optional

        auto result = vkCreatePipelineLayout(m_engine->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &m_pipelineLayout);
        if (result != VK_SUCCESS) {
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
        pipelineInfo.pDepthStencilState = &depthStencil;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = m_pipelineLayout;
        pipelineInfo.renderPass = m_renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE; // Optional
        pipelineInfo.basePipelineIndex = -1;              // Optional

        result = vkCreateGraphicsPipelines(
            m_engine->getLogicalDevice(), 
            VK_NULL_HANDLE, 
            1, 
            &pipelineInfo, 
            nullptr, 
            &m_graphicsPipeline
        );
        
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        /*
        vkDestroyShaderModule(m_engine->getLogicalDevice(), fragmentShaderModule, nullptr);
        vkDestroyShaderModule(m_engine->getLogicalDevice(), vertexShaderModule, nullptr);
        */
    }

    void createFramebuffers() {
        m_swapChainFramebuffers.resize(m_swapChainImageViews.size());

        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            auto attachments = std::array<VkImageView, 3> {
                m_colorImageView,
                m_depthImageView,
                m_swapChainImageViews[i],
            };

            auto framebufferInfo = VkFramebufferCreateInfo {};
            framebufferInfo.sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO;
            framebufferInfo.renderPass = m_renderPass;
            framebufferInfo.attachmentCount = static_cast<uint32_t>(attachments.size());
            framebufferInfo.pAttachments = attachments.data();
            framebufferInfo.width = m_swapChainExtent.width;
            framebufferInfo.height = m_swapChainExtent.height;
            framebufferInfo.layers = 1;

            auto result = vkCreateFramebuffer(
                m_engine->getLogicalDevice(),
                &framebufferInfo,
                nullptr,
                &m_swapChainFramebuffers[i]
            );

            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
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
        renderPassInfo.renderArea.offset = VkOffset2D { 0, 0 };
        renderPassInfo.renderArea.extent = m_swapChainExtent;

        auto clearValues = std::array<VkClearValue, 2> {};
        clearValues[0].color = VkClearColorValue { { 0.0f, 0.0f, 0.0f, 1.0f } };
        clearValues[1].depthStencil = VkClearDepthStencilValue { 1.0f, 0 };

        // NOTE: The order of `clearValues` should be identical to the order of the attachments
        // in the render pass.
        renderPassInfo.clearValueCount = static_cast<uint32_t>(clearValues.size());
        renderPassInfo.pClearValues = clearValues.data();

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

        auto vertexBuffers = std::array<VkBuffer, 1> { m_vertexBuffer };
        auto offsets = std::array<VkDeviceSize, 1> { 0 };
        vkCmdBindVertexBuffers(commandBuffer, 0, 1, vertexBuffers.data(), offsets.data());

        vkCmdBindIndexBuffer(commandBuffer, m_indexBuffer, 0, VK_INDEX_TYPE_UINT32);
        vkCmdBindDescriptorSets(
            commandBuffer,
            VK_PIPELINE_BIND_POINT_GRAPHICS,
            m_pipelineLayout,
            0,
            1,
            &m_descriptorSets[m_currentFrame],
            0,
            nullptr
        );
        
        vkCmdDrawIndexed(commandBuffer, static_cast<uint32_t>(m_indices.size()), 1, 0, 0, 0);

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
            auto result = vkCreateSemaphore(m_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &m_imageAvailableSemaphores[i]);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create in-flight semaphore synchronization object");
            }

            result = vkCreateSemaphore(m_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &m_renderFinishedSemaphores[i]);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create render finished synchronization object");
            }

            result = vkCreateFence(m_engine->getLogicalDevice(), &fenceInfo, nullptr, &m_inFlightFences[i]);
            if (result != VK_SUCCESS) {
                throw std::runtime_error("failed to create in-flight fence synchronization object");
            }
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        static auto startTime = std::chrono::high_resolution_clock::now();

        auto currentTime = std::chrono::high_resolution_clock::now();
        float time = std::chrono::duration<float, std::chrono::seconds::period>(currentTime - startTime).count();

        auto ubo = UniformBufferObject {};
        ubo.model = glm::rotate(glm::mat4(1.0f), time * glm::radians(90.0f), glm::vec3(0.0f, 0.0f, 1.0f));
        ubo.view = glm::lookAt(
            glm::vec3(2.0f, 2.0f, 2.0f),
            glm::vec3(0.0f, 0.0f, 0.0f),
            glm::vec3(0.0f, 0.0f, 1.0f)
        );
        ubo.proj = glm::perspective(
            glm::radians(45.0f),
            m_swapChainExtent.width / (float) m_swapChainExtent.height,
            0.1f,
            10.0f
        );
        ubo.proj[1][1] *= -1;

        memcpy(m_uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void draw() {
        vkWaitForFences(m_engine->getLogicalDevice(), 1, &m_inFlightFences[m_currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        auto result = vkAcquireNextImageKHR(
            m_engine->getLogicalDevice(), 
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

        this->updateUniformBuffer(m_currentFrame);

        vkResetFences(m_engine->getLogicalDevice(), 1, &m_inFlightFences[m_currentFrame]);

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

        result = vkQueueSubmit(m_engine->getGraphicsQueue(), 1, &submitInfo, m_inFlightFences[m_currentFrame]);
        if (result != VK_SUCCESS) {
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

        result = vkQueuePresentKHR(m_engine->getPresentQueue(), &presentInfo);
        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || m_engine->hasFramebufferResized()) {
            m_engine->setFramebufferResized(false);
            this->recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        m_currentFrame = (m_currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    void cleanupSwapChain() {
        vkDestroyImageView(m_engine->getLogicalDevice(), m_depthImageView, nullptr);
        vkDestroyImage(m_engine->getLogicalDevice(), m_depthImage, nullptr);
        vkFreeMemory(m_engine->getLogicalDevice(), m_depthImageMemory, nullptr);

        vkDestroyImageView(m_engine->getLogicalDevice(), m_colorImageView, nullptr);
        vkDestroyImage(m_engine->getLogicalDevice(), m_colorImage, nullptr);
        vkFreeMemory(m_engine->getLogicalDevice(), m_colorImageMemory, nullptr);

        for (size_t i = 0; i < m_swapChainFramebuffers.size(); i++) {
            vkDestroyFramebuffer(m_engine->getLogicalDevice(), m_swapChainFramebuffers[i], nullptr);
        }

        for (size_t i = 0; i < m_swapChainImageViews.size(); i++) {
            vkDestroyImageView(m_engine->getLogicalDevice(), m_swapChainImageViews[i], nullptr);
        }

        vkDestroySwapchainKHR(m_engine->getLogicalDevice(), m_swapChain, nullptr);
    }

    void recreateSwapChain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_engine->getWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_engine->getWindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_engine->getLogicalDevice());

        this->cleanupSwapChain();
        this->createSwapChain();
        this->createImageViews();
        this->createColorResources();
        this->createDepthResources();
        this->createFramebuffers();
    }
};
















struct ComputeShaderUniformBufferObject {
    float deltaTime = 1.0f;
};

struct Particle {
    glm::vec2 position;
    glm::vec2 velocity;
    glm::vec4 color;

    static VkVertexInputBindingDescription getBindingDescription() {
        VkVertexInputBindingDescription bindingDescription{};
        bindingDescription.binding = 0;
        bindingDescription.stride = sizeof(Particle);
        bindingDescription.inputRate = VK_VERTEX_INPUT_RATE_VERTEX;

        return bindingDescription;
    }

    static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
        std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

        attributeDescriptions[0].binding = 0;
        attributeDescriptions[0].location = 0;
        attributeDescriptions[0].format = VK_FORMAT_R32G32_SFLOAT;
        attributeDescriptions[0].offset = offsetof(Particle, position);

        attributeDescriptions[1].binding = 0;
        attributeDescriptions[1].location = 1;
        attributeDescriptions[1].format = VK_FORMAT_R32G32B32A32_SFLOAT;
        attributeDescriptions[1].offset = offsetof(Particle, color);

        return attributeDescriptions;
    }
};

class ComputeShaderApp {
public:
    void run() {
        this->initEngine();
        this->mainLoop();
        this->cleanup();
    }

private:
    Engine* m_engine;

    // GLFWwindow* window;
    //
    // VkInstance instance;
    // VkDebugUtilsMessengerEXT debugMessenger;
    // VkSurfaceKHR surface;
    //
    // VkPhysicalDevice physicalDevice = VK_NULL_HANDLE;
    // VkDevice device;
    //
    // VkQueue graphicsQueue;
    // VkQueue computeQueue;
    // VkQueue presentQueue;
    //
    VkSwapchainKHR swapChain;
    std::vector<VkImage> swapChainImages;
    VkFormat swapChainImageFormat;
    VkExtent2D swapChainExtent;
    std::vector<VkImageView> swapChainImageViews;
    std::vector<VkFramebuffer> swapChainFramebuffers;

    VkRenderPass renderPass;
    VkPipelineLayout pipelineLayout;
    VkPipeline graphicsPipeline;

    VkDescriptorSetLayout computeDescriptorSetLayout;
    VkPipelineLayout computePipelineLayout;
    VkPipeline computePipeline;
    //
    // VkCommandPool commandPool;
    //
    std::vector<VkBuffer> shaderStorageBuffers;
    std::vector<VkDeviceMemory> shaderStorageBuffersMemory;

    std::vector<VkBuffer> uniformBuffers;
    std::vector<VkDeviceMemory> uniformBuffersMemory;
    std::vector<void*> uniformBuffersMapped;

    VkDescriptorPool descriptorPool;
    std::vector<VkDescriptorSet> computeDescriptorSets;

    std::vector<VkCommandBuffer> commandBuffers;
    std::vector<VkCommandBuffer> computeCommandBuffers;

    std::vector<VkSemaphore> imageAvailableSemaphores;
    std::vector<VkSemaphore> renderFinishedSemaphores;
    std::vector<VkSemaphore> computeFinishedSemaphores;
    std::vector<VkFence> inFlightFences;
    std::vector<VkFence> computeInFlightFences;
    uint32_t currentFrame = 0;

    bool framebufferResized = false;

    float lastFrameTime = 0.0f;
    double lastTime = 0.0f;

    bool m_enableValidationLayers { false };
    bool m_enableDebuggingExtensions { false };


    void createEngine() {
        auto engine = Engine::createDebugMode();
        engine->createWindow(WIDTH, HEIGHT, "Hello, Triangle!");

        m_engine = engine;
    }

    void initEngine() {
        this->createEngine();
        // createInstance();
        // setupDebugMessenger();
        // createSurface();
        // pickPhysicalDevice();
        // createLogicalDevice();
        
        
        this->createSwapChain();
        this->createImageViews();
        this->createRenderPass();
        this->createComputeDescriptorSetLayout();
        this->createGraphicsPipeline();
        this->createComputePipeline();
        this->createFramebuffers();
        // createCommandPool();
        this->createShaderStorageBuffers();
        this->createUniformBuffers();
        this->createDescriptorPool();
        this->createComputeDescriptorSets();
        this->createCommandBuffers();
        this->createComputeCommandBuffers();
        this->createSyncObjects();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(m_engine->getWindow())) {
            glfwPollEvents();
            this->draw();
            // We want to animate the particle system using the last frames time to get smooth, frame-rate 
            // independent animation.
            double currentTime = glfwGetTime();
            lastFrameTime = (currentTime - lastTime) * 1000.0;
            lastTime = currentTime;
        }

        vkDeviceWaitIdle(m_engine->getLogicalDevice());
    }

    void cleanupSwapChain() {
        for (auto framebuffer : swapChainFramebuffers) {
            vkDestroyFramebuffer(m_engine->getLogicalDevice(), framebuffer, nullptr);
        }

        for (auto imageView : swapChainImageViews) {
            vkDestroyImageView(m_engine->getLogicalDevice(), imageView, nullptr);
        }

        vkDestroySwapchainKHR(m_engine->getLogicalDevice(), swapChain, nullptr);
    }

    void cleanup() {
        if (m_engine->isInitialized()) {
            this->cleanupSwapChain();

            vkDestroyPipeline(m_engine->getLogicalDevice(), graphicsPipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->getLogicalDevice(), pipelineLayout, nullptr);

            vkDestroyPipeline(m_engine->getLogicalDevice(), computePipeline, nullptr);
            vkDestroyPipelineLayout(m_engine->getLogicalDevice(), computePipelineLayout, nullptr);

            vkDestroyRenderPass(m_engine->getLogicalDevice(), renderPass, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroyBuffer(m_engine->getLogicalDevice(), uniformBuffers[i], nullptr);
                vkFreeMemory(m_engine->getLogicalDevice(), uniformBuffersMemory[i], nullptr);
            }

            vkDestroyDescriptorPool(m_engine->getLogicalDevice(), descriptorPool, nullptr);
      
            vkDestroyDescriptorSetLayout(m_engine->getLogicalDevice(), computeDescriptorSetLayout, nullptr);

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroyBuffer(m_engine->getLogicalDevice(), shaderStorageBuffers[i], nullptr);
                vkFreeMemory(m_engine->getLogicalDevice(), shaderStorageBuffersMemory[i], nullptr);
            }

            for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
                vkDestroySemaphore(m_engine->getLogicalDevice(), renderFinishedSemaphores[i], nullptr);
                vkDestroySemaphore(m_engine->getLogicalDevice(), imageAvailableSemaphores[i], nullptr);
                vkDestroySemaphore(m_engine->getLogicalDevice(), computeFinishedSemaphores[i], nullptr);
                vkDestroyFence(m_engine->getLogicalDevice(), inFlightFences[i], nullptr);
                vkDestroyFence(m_engine->getLogicalDevice(), computeInFlightFences[i], nullptr);
            }

            delete m_engine;
        }
    }

    void recreateSwapChain() {
        int width = 0;
        int height = 0;
        glfwGetFramebufferSize(m_engine->getWindow(), &width, &height);
        while (width == 0 || height == 0) {
            glfwGetFramebufferSize(m_engine->getWindow(), &width, &height);
            glfwWaitEvents();
        }

        vkDeviceWaitIdle(m_engine->getLogicalDevice());

        this->cleanupSwapChain();

        this->createSwapChain();
        this->createImageViews();
        this->createFramebuffers();
    }

    /*
    void createInstance() {
        if (enableValidationLayers && !checkValidationLayerSupport()) {
            throw std::runtime_error("validation layers requested, but not available!");
        }

        VkApplicationInfo appInfo{};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Triangle";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        VkInstanceCreateInfo createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        auto extensions = getRequiredExtensions();
        createInfo.enabledExtensionCount = static_cast<uint32_t>(extensions.size());
        createInfo.ppEnabledExtensionNames = extensions.data();

        VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();

            populateDebugMessengerCreateInfo(debugCreateInfo);
            createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT*) &debugCreateInfo;
        } else {
            createInfo.enabledLayerCount = 0;

            createInfo.pNext = nullptr;
        }

        if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }
    */
    /*
    void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT& createInfo) {
        createInfo = {};
        createInfo.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT;
        createInfo.messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT;
        createInfo.messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT | VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT;
        createInfo.pfnUserCallback = debugCallback;
    }
    */
    /*
    void setupDebugMessenger() {
        if (!enableValidationLayers) return;

        VkDebugUtilsMessengerCreateInfoEXT createInfo;
        populateDebugMessengerCreateInfo(createInfo);

        if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
            throw std::runtime_error("failed to set up debug messenger!");
        }
    }
    */
    /*
    void createSurface() {
        if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
            throw std::runtime_error("failed to create window surface!");
        }
    }
    */
    /*
    void pickPhysicalDevice() {
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
    */
    /*
    void createLogicalDevice() {
        QueueFamilyIndices indices = findQueueFamilies(physicalDevice);

        std::vector<VkDeviceQueueCreateInfo> queueCreateInfos;
        std::set<uint32_t> uniqueQueueFamilies = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

        float queuePriority = 1.0f;
        for (uint32_t queueFamily : uniqueQueueFamilies) {
            VkDeviceQueueCreateInfo queueCreateInfo{};
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

        createInfo.enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size());
        createInfo.ppEnabledExtensionNames = deviceExtensions.data();

        if (enableValidationLayers) {
            createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
            createInfo.ppEnabledLayerNames = validationLayers.data();
        } else {
            createInfo.enabledLayerCount = 0;
        }

        if (vkCreateDevice(physicalDevice, &createInfo, nullptr, &device) != VK_SUCCESS) {
            throw std::runtime_error("failed to create logical device!");
        }

        vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &graphicsQueue);
        vkGetDeviceQueue(device, indices.graphicsAndComputeFamily.value(), 0, &computeQueue);
        vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &presentQueue);
    }
    */

    void createSwapChain() {
        SwapChainSupportDetails swapChainSupport = m_engine->querySwapChainSupport(
            m_engine->getPhysicalDevice(),
            m_engine->getSurface()
        );
        VkSurfaceFormatKHR surfaceFormat = this->chooseSwapSurfaceFormat(swapChainSupport.formats);
        VkPresentModeKHR presentMode = this->chooseSwapPresentMode(swapChainSupport.presentModes);
        VkExtent2D extent = this->chooseSwapExtent(swapChainSupport.capabilities);

        uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
        if (swapChainSupport.capabilities.maxImageCount > 0 && imageCount > swapChainSupport.capabilities.maxImageCount) {
            imageCount = swapChainSupport.capabilities.maxImageCount;
        }

        VkSwapchainCreateInfoKHR createInfo{};
        createInfo.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR;
        createInfo.surface = m_engine->getSurface();

        createInfo.minImageCount = imageCount;
        createInfo.imageFormat = surfaceFormat.format;
        createInfo.imageColorSpace = surfaceFormat.colorSpace;
        createInfo.imageExtent = extent;
        createInfo.imageArrayLayers = 1;
        createInfo.imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT;

        QueueFamilyIndices indices = m_engine->findQueueFamilies(
            m_engine->getPhysicalDevice(),
            m_engine->getSurface()
        );
        uint32_t queueFamilyIndices[] = {indices.graphicsAndComputeFamily.value(), indices.presentFamily.value()};

        if (indices.graphicsAndComputeFamily != indices.presentFamily) {
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

        if (vkCreateSwapchainKHR(m_engine->getLogicalDevice(), &createInfo, nullptr, &swapChain) != VK_SUCCESS) {
            throw std::runtime_error("failed to create swap chain!");
        }

        vkGetSwapchainImagesKHR(m_engine->getLogicalDevice(), swapChain, &imageCount, nullptr);
        swapChainImages.resize(imageCount);
        vkGetSwapchainImagesKHR(m_engine->getLogicalDevice(), swapChain, &imageCount, swapChainImages.data());

        swapChainImageFormat = surfaceFormat.format;
        swapChainExtent = extent;
    }

    void createImageViews() {
        swapChainImageViews.resize(swapChainImages.size());

        for (size_t i = 0; i < swapChainImages.size(); i++) {
            VkImageViewCreateInfo createInfo{};
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

            if (vkCreateImageView(m_engine->getLogicalDevice(), &createInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create image views!");
            }
        }
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

        VkSubpassDependency dependency{};
        dependency.srcSubpass = VK_SUBPASS_EXTERNAL;
        dependency.dstSubpass = 0;
        dependency.srcStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.srcAccessMask = 0;
        dependency.dstStageMask = VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT;
        dependency.dstAccessMask = VK_ACCESS_COLOR_ATTACHMENT_WRITE_BIT;

        VkRenderPassCreateInfo renderPassInfo{};
        renderPassInfo.sType = VK_STRUCTURE_TYPE_RENDER_PASS_CREATE_INFO;
        renderPassInfo.attachmentCount = 1;
        renderPassInfo.pAttachments = &colorAttachment;
        renderPassInfo.subpassCount = 1;
        renderPassInfo.pSubpasses = &subpass;
        renderPassInfo.dependencyCount = 1;
        renderPassInfo.pDependencies = &dependency;

        if (vkCreateRenderPass(m_engine->getLogicalDevice(), &renderPassInfo, nullptr, &renderPass) != VK_SUCCESS) {
            throw std::runtime_error("failed to create render pass!");
        }
    }

    void createComputeDescriptorSetLayout() {
        std::array<VkDescriptorSetLayoutBinding, 3> layoutBindings{};
        layoutBindings[0].binding = 0;
        layoutBindings[0].descriptorCount = 1;
        layoutBindings[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        layoutBindings[0].pImmutableSamplers = nullptr;
        layoutBindings[0].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        layoutBindings[1].binding = 1;
        layoutBindings[1].descriptorCount = 1;
        layoutBindings[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBindings[1].pImmutableSamplers = nullptr;
        layoutBindings[1].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        layoutBindings[2].binding = 2;
        layoutBindings[2].descriptorCount = 1;
        layoutBindings[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        layoutBindings[2].pImmutableSamplers = nullptr;
        layoutBindings[2].stageFlags = VK_SHADER_STAGE_COMPUTE_BIT;

        VkDescriptorSetLayoutCreateInfo layoutInfo{};
        layoutInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_LAYOUT_CREATE_INFO;
        layoutInfo.bindingCount = 3;
        layoutInfo.pBindings = layoutBindings.data();

        if (vkCreateDescriptorSetLayout(m_engine->getLogicalDevice(), &layoutInfo, nullptr, &computeDescriptorSetLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute descriptor set layout!");
        }
    }


    void createGraphicsPipeline() {
        auto vertexShaderModule = m_engine->createShaderModuleFromFile("shaders/shader_compute.vert.glsl.spv");
        auto fragmentShaderModule = m_engine->createShaderModuleFromFile("shaders/shader_compute.frag.glsl.spv");

        VkPipelineShaderStageCreateInfo vertShaderStageInfo{};
        vertShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        vertShaderStageInfo.stage = VK_SHADER_STAGE_VERTEX_BIT;
        vertShaderStageInfo.module = vertexShaderModule;
        vertShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo fragShaderStageInfo{};
        fragShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        fragShaderStageInfo.stage = VK_SHADER_STAGE_FRAGMENT_BIT;
        fragShaderStageInfo.module = fragmentShaderModule;
        fragShaderStageInfo.pName = "main";

        VkPipelineShaderStageCreateInfo shaderStages[] = {vertShaderStageInfo, fragShaderStageInfo};

        VkPipelineVertexInputStateCreateInfo vertexInputInfo{};
        vertexInputInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_VERTEX_INPUT_STATE_CREATE_INFO;

        auto bindingDescription = Particle::getBindingDescription();
        auto attributeDescriptions = Particle::getAttributeDescriptions();

        vertexInputInfo.vertexBindingDescriptionCount = 1;
        vertexInputInfo.vertexAttributeDescriptionCount = static_cast<uint32_t>(attributeDescriptions.size());
        vertexInputInfo.pVertexBindingDescriptions = &bindingDescription;
        vertexInputInfo.pVertexAttributeDescriptions = attributeDescriptions.data();

        VkPipelineInputAssemblyStateCreateInfo inputAssembly{};
        inputAssembly.sType = VK_STRUCTURE_TYPE_PIPELINE_INPUT_ASSEMBLY_STATE_CREATE_INFO;
        inputAssembly.topology = VK_PRIMITIVE_TOPOLOGY_POINT_LIST;
        inputAssembly.primitiveRestartEnable = VK_FALSE;

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
        rasterizer.frontFace = VK_FRONT_FACE_COUNTER_CLOCKWISE;
        rasterizer.depthBiasEnable = VK_FALSE;

        VkPipelineMultisampleStateCreateInfo multisampling{};
        multisampling.sType = VK_STRUCTURE_TYPE_PIPELINE_MULTISAMPLE_STATE_CREATE_INFO;
        multisampling.sampleShadingEnable = VK_FALSE;
        multisampling.rasterizationSamples = VK_SAMPLE_COUNT_1_BIT;

        VkPipelineColorBlendAttachmentState colorBlendAttachment{};
        colorBlendAttachment.colorWriteMask = VK_COLOR_COMPONENT_R_BIT | VK_COLOR_COMPONENT_G_BIT | VK_COLOR_COMPONENT_B_BIT | VK_COLOR_COMPONENT_A_BIT;
        colorBlendAttachment.blendEnable = VK_TRUE;
        colorBlendAttachment.colorBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcColorBlendFactor = VK_BLEND_FACTOR_SRC_ALPHA;
        colorBlendAttachment.dstColorBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.alphaBlendOp = VK_BLEND_OP_ADD;
        colorBlendAttachment.srcAlphaBlendFactor = VK_BLEND_FACTOR_ONE_MINUS_SRC_ALPHA;
        colorBlendAttachment.dstAlphaBlendFactor = VK_BLEND_FACTOR_ZERO;

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
        pipelineLayoutInfo.pSetLayouts = nullptr;

        if (vkCreatePipelineLayout(m_engine->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &pipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create pipeline layout!");
        }

        VkGraphicsPipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_GRAPHICS_PIPELINE_CREATE_INFO;
        pipelineInfo.stageCount = 2;
        pipelineInfo.pStages = shaderStages;
        pipelineInfo.pVertexInputState = &vertexInputInfo;
        pipelineInfo.pInputAssemblyState = &inputAssembly;
        pipelineInfo.pViewportState = &viewportState;
        pipelineInfo.pRasterizationState = &rasterizer;
        pipelineInfo.pMultisampleState = &multisampling;
        pipelineInfo.pColorBlendState = &colorBlending;
        pipelineInfo.pDynamicState = &dynamicState;
        pipelineInfo.layout = pipelineLayout;
        pipelineInfo.renderPass = renderPass;
        pipelineInfo.subpass = 0;
        pipelineInfo.basePipelineHandle = VK_NULL_HANDLE;

        if (vkCreateGraphicsPipelines(m_engine->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &graphicsPipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics pipeline!");
        }

        // vkDestroyShaderModule(device, fragmentShaderModule, nullptr);
        // vkDestroyShaderModule(device, vertexShaderModule, nullptr);
    }

    void createComputePipeline() {
        auto computeShaderModule = m_engine->createShaderModuleFromFile("shaders/shader_compute.comp.glsl.spv");

        VkPipelineShaderStageCreateInfo computeShaderStageInfo{};
        computeShaderStageInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_SHADER_STAGE_CREATE_INFO;
        computeShaderStageInfo.stage = VK_SHADER_STAGE_COMPUTE_BIT;
        computeShaderStageInfo.module = computeShaderModule;
        computeShaderStageInfo.pName = "main";

        VkPipelineLayoutCreateInfo pipelineLayoutInfo{};
        pipelineLayoutInfo.sType = VK_STRUCTURE_TYPE_PIPELINE_LAYOUT_CREATE_INFO;
        pipelineLayoutInfo.setLayoutCount = 1;
        pipelineLayoutInfo.pSetLayouts = &computeDescriptorSetLayout;

        if (vkCreatePipelineLayout(m_engine->getLogicalDevice(), &pipelineLayoutInfo, nullptr, &computePipelineLayout) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline layout!");
        }

        VkComputePipelineCreateInfo pipelineInfo{};
        pipelineInfo.sType = VK_STRUCTURE_TYPE_COMPUTE_PIPELINE_CREATE_INFO;
        pipelineInfo.layout = computePipelineLayout;
        pipelineInfo.stage = computeShaderStageInfo;

        if (vkCreateComputePipelines(m_engine->getLogicalDevice(), VK_NULL_HANDLE, 1, &pipelineInfo, nullptr, &computePipeline) != VK_SUCCESS) {
            throw std::runtime_error("failed to create compute pipeline!");
        }

        // vkDestroyShaderModule(m_engine->getLogicalDevice(), computeShaderModule, nullptr);
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

            if (vkCreateFramebuffer(m_engine->getLogicalDevice(), &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create framebuffer!");
            }
        }
    }

    /*
    void createCommandPool() {
        QueueFamilyIndices queueFamilyIndices = findQueueFamilies(physicalDevice);

        VkCommandPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO;
        poolInfo.flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT;
        poolInfo.queueFamilyIndex = queueFamilyIndices.graphicsAndComputeFamily.value();

        if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create graphics command pool!");
        }
    }
    */

    void createShaderStorageBuffers() {
        // Initialize particles.
        std::default_random_engine rndEngine((unsigned)time(nullptr));
        std::uniform_real_distribution<float> rndDist(0.0f, 1.0f);

        // Initial particle positions on a circle
        std::vector<Particle> particles(PARTICLE_COUNT);
        for (auto& particle : particles) {
            float r = 0.25f * sqrt(rndDist(rndEngine));
            float theta = rndDist(rndEngine) * 2.0f * 3.14159265358979323846f;
            float x = r * cos(theta) * HEIGHT / WIDTH;
            float y = r * sin(theta);
            particle.position = glm::vec2(x, y);
            particle.velocity = glm::normalize(glm::vec2(x,y)) * 0.00025f;
            particle.color = glm::vec4(rndDist(rndEngine), rndDist(rndEngine), rndDist(rndEngine), 1.0f);
        }

        VkDeviceSize bufferSize = sizeof(Particle) * PARTICLE_COUNT;

        // Create a staging buffer used to upload data to the gpu
        VkBuffer stagingBuffer;
        VkDeviceMemory stagingBufferMemory;
        createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer, stagingBufferMemory);

        void* data;
        vkMapMemory(m_engine->getLogicalDevice(), stagingBufferMemory, 0, bufferSize, 0, &data);
        memcpy(data, particles.data(), (size_t)bufferSize);
        vkUnmapMemory(m_engine->getLogicalDevice(), stagingBufferMemory);

        shaderStorageBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        shaderStorageBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);

        // Copy initial particle data to all storage buffers
        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_STORAGE_BUFFER_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT | VK_BUFFER_USAGE_TRANSFER_DST_BIT, VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, shaderStorageBuffers[i], shaderStorageBuffersMemory[i]);
            copyBuffer(stagingBuffer, shaderStorageBuffers[i], bufferSize);
        }

        vkDestroyBuffer(m_engine->getLogicalDevice(), stagingBuffer, nullptr);
        vkFreeMemory(m_engine->getLogicalDevice(), stagingBufferMemory, nullptr);

    }

    void createUniformBuffers() {
        VkDeviceSize bufferSize = sizeof(ComputeShaderUniformBufferObject);

        uniformBuffers.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMemory.resize(MAX_FRAMES_IN_FLIGHT);
        uniformBuffersMapped.resize(MAX_FRAMES_IN_FLIGHT);

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            createBuffer(bufferSize, VK_BUFFER_USAGE_UNIFORM_BUFFER_BIT, VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, uniformBuffers[i], uniformBuffersMemory[i]);

            vkMapMemory(m_engine->getLogicalDevice(), uniformBuffersMemory[i], 0, bufferSize, 0, &uniformBuffersMapped[i]);
        }
    }

    void createDescriptorPool() {
        std::array<VkDescriptorPoolSize, 2> poolSizes{};
        poolSizes[0].type = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
        poolSizes[0].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        
        poolSizes[1].type = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
        poolSizes[1].descriptorCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT) * 2;

        VkDescriptorPoolCreateInfo poolInfo{};
        poolInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_POOL_CREATE_INFO;
        poolInfo.poolSizeCount = 2;
        poolInfo.pPoolSizes = poolSizes.data();
        poolInfo.maxSets = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);

        if (vkCreateDescriptorPool(m_engine->getLogicalDevice(), &poolInfo, nullptr, &descriptorPool) != VK_SUCCESS) {
            throw std::runtime_error("failed to create descriptor pool!");
        }
    }

    void createComputeDescriptorSets() {
        std::vector<VkDescriptorSetLayout> layouts(MAX_FRAMES_IN_FLIGHT, computeDescriptorSetLayout);
        VkDescriptorSetAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_DESCRIPTOR_SET_ALLOCATE_INFO;
        allocInfo.descriptorPool = descriptorPool;
        allocInfo.descriptorSetCount = static_cast<uint32_t>(MAX_FRAMES_IN_FLIGHT);
        allocInfo.pSetLayouts = layouts.data();

        computeDescriptorSets.resize(MAX_FRAMES_IN_FLIGHT);
        if (vkAllocateDescriptorSets(m_engine->getLogicalDevice(), &allocInfo, computeDescriptorSets.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate descriptor sets!");
        }

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            VkDescriptorBufferInfo uniformBufferInfo{};
            uniformBufferInfo.buffer = uniformBuffers[i];
            uniformBufferInfo.offset = 0;
            uniformBufferInfo.range = sizeof(ComputeShaderUniformBufferObject);

            std::array<VkWriteDescriptorSet, 3> descriptorWrites{};
            descriptorWrites[0].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[0].dstSet = computeDescriptorSets[i];
            descriptorWrites[0].dstBinding = 0;
            descriptorWrites[0].dstArrayElement = 0;
            descriptorWrites[0].descriptorType = VK_DESCRIPTOR_TYPE_UNIFORM_BUFFER;
            descriptorWrites[0].descriptorCount = 1;
            descriptorWrites[0].pBufferInfo = &uniformBufferInfo;

            VkDescriptorBufferInfo storageBufferInfoLastFrame{};
            storageBufferInfoLastFrame.buffer = shaderStorageBuffers[(i - 1) % MAX_FRAMES_IN_FLIGHT];
            storageBufferInfoLastFrame.offset = 0;
            storageBufferInfoLastFrame.range = sizeof(Particle) * PARTICLE_COUNT;

            descriptorWrites[1].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[1].dstSet = computeDescriptorSets[i];
            descriptorWrites[1].dstBinding = 1;
            descriptorWrites[1].dstArrayElement = 0;
            descriptorWrites[1].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[1].descriptorCount = 1;
            descriptorWrites[1].pBufferInfo = &storageBufferInfoLastFrame;

            VkDescriptorBufferInfo storageBufferInfoCurrentFrame{};
            storageBufferInfoCurrentFrame.buffer = shaderStorageBuffers[i];
            storageBufferInfoCurrentFrame.offset = 0;
            storageBufferInfoCurrentFrame.range = sizeof(Particle) * PARTICLE_COUNT;

            descriptorWrites[2].sType = VK_STRUCTURE_TYPE_WRITE_DESCRIPTOR_SET;
            descriptorWrites[2].dstSet = computeDescriptorSets[i];
            descriptorWrites[2].dstBinding = 2;
            descriptorWrites[2].dstArrayElement = 0;
            descriptorWrites[2].descriptorType = VK_DESCRIPTOR_TYPE_STORAGE_BUFFER;
            descriptorWrites[2].descriptorCount = 1;
            descriptorWrites[2].pBufferInfo = &storageBufferInfoCurrentFrame;

            vkUpdateDescriptorSets(m_engine->getLogicalDevice(), 3, descriptorWrites.data(), 0, nullptr);
        }
    }


    void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties, VkBuffer& buffer, VkDeviceMemory& bufferMemory) {
        VkBufferCreateInfo bufferInfo{};
        bufferInfo.sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO;
        bufferInfo.size = size;
        bufferInfo.usage = usage;
        bufferInfo.sharingMode = VK_SHARING_MODE_EXCLUSIVE;

        if (vkCreateBuffer(m_engine->getLogicalDevice(), &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to create buffer!");
        }

        VkMemoryRequirements memRequirements;
        vkGetBufferMemoryRequirements(m_engine->getLogicalDevice(), buffer, &memRequirements);

        VkMemoryAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO;
        allocInfo.allocationSize = memRequirements.size;
        allocInfo.memoryTypeIndex = findMemoryType(memRequirements.memoryTypeBits, properties);

        if (vkAllocateMemory(m_engine->getLogicalDevice(), &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate buffer memory!");
        }

        vkBindBufferMemory(m_engine->getLogicalDevice(), buffer, bufferMemory, 0);
    }

    void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandPool = m_engine->getCommandPool();
        allocInfo.commandBufferCount = 1;

        VkCommandBuffer commandBuffer;
        vkAllocateCommandBuffers(m_engine->getLogicalDevice(), &allocInfo, &commandBuffer);

        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;
        beginInfo.flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT;

        vkBeginCommandBuffer(commandBuffer, &beginInfo);

        VkBufferCopy copyRegion{};
        copyRegion.size = size;
        vkCmdCopyBuffer(commandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);

        vkEndCommandBuffer(commandBuffer);

        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffer;

        vkQueueSubmit(m_engine->getGraphicsQueue(), 1, &submitInfo, VK_NULL_HANDLE);
        vkQueueWaitIdle(m_engine->getGraphicsQueue());

        vkFreeCommandBuffers(m_engine->getLogicalDevice(), m_engine->getCommandPool(), 1, &commandBuffer);
    }

    uint32_t findMemoryType(uint32_t typeFilter, VkMemoryPropertyFlags properties) {
        VkPhysicalDeviceMemoryProperties memProperties;
        vkGetPhysicalDeviceMemoryProperties(m_engine->getPhysicalDevice(), &memProperties);

        for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
            if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
                return i;
            }
        }

        throw std::runtime_error("failed to find suitable memory type!");
    }

    void createCommandBuffers() {
        commandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_engine->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t) commandBuffers.size();

        if (vkAllocateCommandBuffers(m_engine->getLogicalDevice(), &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate command buffers!");
        }
    }

    void createComputeCommandBuffers() {
        computeCommandBuffers.resize(MAX_FRAMES_IN_FLIGHT);

        VkCommandBufferAllocateInfo allocInfo{};
        allocInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO;
        allocInfo.commandPool = m_engine->getCommandPool();
        allocInfo.level = VK_COMMAND_BUFFER_LEVEL_PRIMARY;
        allocInfo.commandBufferCount = (uint32_t)computeCommandBuffers.size();

        if (vkAllocateCommandBuffers(m_engine->getLogicalDevice(), &allocInfo, computeCommandBuffers.data()) != VK_SUCCESS) {
            throw std::runtime_error("failed to allocate compute command buffers!");
        }
    }

    void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

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
            viewport.width = (float) swapChainExtent.width;
            viewport.height = (float) swapChainExtent.height;
            viewport.minDepth = 0.0f;
            viewport.maxDepth = 1.0f;
            vkCmdSetViewport(commandBuffer, 0, 1, &viewport);

            VkRect2D scissor{};
            scissor.offset = {0, 0};
            scissor.extent = swapChainExtent;
            vkCmdSetScissor(commandBuffer, 0, 1, &scissor);            

            VkDeviceSize offsets[] = {0};
            vkCmdBindVertexBuffers(commandBuffer, 0, 1, &shaderStorageBuffers[currentFrame], offsets);

            vkCmdDraw(commandBuffer, PARTICLE_COUNT, 1, 0, 0);

        vkCmdEndRenderPass(commandBuffer);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record command buffer!");
        }
    }

    void recordComputeCommandBuffer(VkCommandBuffer commandBuffer) {
        VkCommandBufferBeginInfo beginInfo{};
        beginInfo.sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO;

        if (vkBeginCommandBuffer(commandBuffer, &beginInfo) != VK_SUCCESS) {
            throw std::runtime_error("failed to begin recording compute command buffer!");
        }

        vkCmdBindPipeline(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipeline);

        vkCmdBindDescriptorSets(commandBuffer, VK_PIPELINE_BIND_POINT_COMPUTE, computePipelineLayout, 0, 1, &computeDescriptorSets[currentFrame], 0, nullptr);

        vkCmdDispatch(commandBuffer, PARTICLE_COUNT / 256, 1, 1);

        if (vkEndCommandBuffer(commandBuffer) != VK_SUCCESS) {
            throw std::runtime_error("failed to record compute command buffer!");
        }

    }

    void createSyncObjects() {
        imageAvailableSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        renderFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        computeFinishedSemaphores.resize(MAX_FRAMES_IN_FLIGHT);
        inFlightFences.resize(MAX_FRAMES_IN_FLIGHT);
        computeInFlightFences.resize(MAX_FRAMES_IN_FLIGHT);

        VkSemaphoreCreateInfo semaphoreInfo{};
        semaphoreInfo.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO;

        VkFenceCreateInfo fenceInfo{};
        fenceInfo.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO;
        fenceInfo.flags = VK_FENCE_CREATE_SIGNALED_BIT;

        for (size_t i = 0; i < MAX_FRAMES_IN_FLIGHT; i++) {
            if (vkCreateSemaphore(m_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
                vkCreateSemaphore(m_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_engine->getLogicalDevice(), &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create graphics synchronization objects for a frame!");
            }
            if (vkCreateSemaphore(m_engine->getLogicalDevice(), &semaphoreInfo, nullptr, &computeFinishedSemaphores[i]) != VK_SUCCESS ||
                vkCreateFence(m_engine->getLogicalDevice(), &fenceInfo, nullptr, &computeInFlightFences[i]) != VK_SUCCESS) {
                throw std::runtime_error("failed to create compute synchronization objects for a frame!");
            }
        }
    }

    void updateUniformBuffer(uint32_t currentImage) {
        ComputeShaderUniformBufferObject ubo{};
        ubo.deltaTime = lastFrameTime * 2.0f;

        memcpy(uniformBuffersMapped[currentImage], &ubo, sizeof(ubo));
    }

    void draw() {
        VkSubmitInfo submitInfo{};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        // Compute submission        
        vkWaitForFences(m_engine->getLogicalDevice(), 1, &computeInFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        this->updateUniformBuffer(currentFrame);

        vkResetFences(m_engine->getLogicalDevice(), 1, &computeInFlightFences[currentFrame]);

        vkResetCommandBuffer(computeCommandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        this->recordComputeCommandBuffer(computeCommandBuffers[currentFrame]);

        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &computeCommandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &computeFinishedSemaphores[currentFrame];

        if (vkQueueSubmit(m_engine->getComputeQueue(), 1, &submitInfo, computeInFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit compute command buffer!");
        };

        // Graphics submission
        vkWaitForFences(m_engine->getLogicalDevice(), 1, &inFlightFences[currentFrame], VK_TRUE, UINT64_MAX);

        uint32_t imageIndex;
        VkResult result = vkAcquireNextImageKHR(m_engine->getLogicalDevice(), swapChain, UINT64_MAX, imageAvailableSemaphores[currentFrame], VK_NULL_HANDLE, &imageIndex);

        if (result == VK_ERROR_OUT_OF_DATE_KHR) {
            this->recreateSwapChain();
            return;
        } else if (result != VK_SUCCESS && result != VK_SUBOPTIMAL_KHR) {
            throw std::runtime_error("failed to acquire swap chain image!");
        }

        vkResetFences(m_engine->getLogicalDevice(), 1, &inFlightFences[currentFrame]);

        vkResetCommandBuffer(commandBuffers[currentFrame], /*VkCommandBufferResetFlagBits*/ 0);
        this->recordCommandBuffer(commandBuffers[currentFrame], imageIndex);

        VkSemaphore waitSemaphores[] = { computeFinishedSemaphores[currentFrame], imageAvailableSemaphores[currentFrame] };
        VkPipelineStageFlags waitStages[] = { VK_PIPELINE_STAGE_VERTEX_INPUT_BIT, VK_PIPELINE_STAGE_COLOR_ATTACHMENT_OUTPUT_BIT };
        submitInfo = {};
        submitInfo.sType = VK_STRUCTURE_TYPE_SUBMIT_INFO;

        submitInfo.waitSemaphoreCount = 2;
        submitInfo.pWaitSemaphores = waitSemaphores;
        submitInfo.pWaitDstStageMask = waitStages;
        submitInfo.commandBufferCount = 1;
        submitInfo.pCommandBuffers = &commandBuffers[currentFrame];
        submitInfo.signalSemaphoreCount = 1;
        submitInfo.pSignalSemaphores = &renderFinishedSemaphores[currentFrame];

        if (vkQueueSubmit(m_engine->getGraphicsQueue(), 1, &submitInfo, inFlightFences[currentFrame]) != VK_SUCCESS) {
            throw std::runtime_error("failed to submit draw command buffer!");
        }

        VkPresentInfoKHR presentInfo{};
        presentInfo.sType = VK_STRUCTURE_TYPE_PRESENT_INFO_KHR;

        presentInfo.waitSemaphoreCount = 1;
        presentInfo.pWaitSemaphores = &renderFinishedSemaphores[currentFrame];

        VkSwapchainKHR swapChains[] = {swapChain};
        presentInfo.swapchainCount = 1;
        presentInfo.pSwapchains = swapChains;

        presentInfo.pImageIndices = &imageIndex;

        result = vkQueuePresentKHR(m_engine->getPresentQueue(), &presentInfo);

        if (result == VK_ERROR_OUT_OF_DATE_KHR || result == VK_SUBOPTIMAL_KHR || framebufferResized) {
            framebufferResized = false;
            this->recreateSwapChain();
        } else if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to present swap chain image!");
        }

        currentFrame = (currentFrame + 1) % MAX_FRAMES_IN_FLIGHT;
    }

    /*
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
    */

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

        return VK_PRESENT_MODE_FIFO_KHR;
    }

    VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR& capabilities) {
        if (capabilities.currentExtent.width != std::numeric_limits<uint32_t>::max()) {
            return capabilities.currentExtent;
        } else {
            int width = 0;
            int height = 0;
            glfwGetFramebufferSize(m_engine->getWindow(), &width, &height);

            VkExtent2D actualExtent = {
                static_cast<uint32_t>(width),
                static_cast<uint32_t>(height)
            };

            actualExtent.width = std::clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
            actualExtent.height = std::clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

            return actualExtent;
        }
    }
    /*
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
    */
    /*
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
    */
    /*
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
    */
    /*
    QueueFamilyIndices findQueueFamilies(VkPhysicalDevice device) {
        QueueFamilyIndices indices;

        uint32_t queueFamilyCount = 0;
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

        std::vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
        vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

        int i = 0;
        for (const auto& queueFamily : queueFamilies) {
            if ((queueFamily.queueFlags & VK_QUEUE_GRAPHICS_BIT) && (queueFamily.queueFlags & VK_QUEUE_COMPUTE_BIT)) {
                indices.graphicsAndComputeFamily = i;
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
    */
    /*
    std::vector<const char*> getRequiredExtensions() {
        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensions;
        glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        std::vector<const char*> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

        if (enableValidationLayers) {
            extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
        }

        return extensions;
    }
    */
    /*
    bool checkValidationLayerSupport() {
        uint32_t layerCount;
        vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

        std::vector<VkLayerProperties> availableLayers(layerCount);
        vkEnumerateInstanceLayerProperties(&layerCount, availableLayers.data());

        for (const char* layerName : validationLayers) {
            bool layerFound = false;

            for (const auto& layerProperties : availableLayers) {
                if (strcmp(layerName, layerProperties.layerName) == 0) {
                    layerFound = true;
                    break;
                }
            }

            if (!layerFound) {
                return false;
            }
        }

        return true;
    }
    */
    /*
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
    */
    /*
    static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType, const VkDebugUtilsMessengerCallbackDataEXT* pCallbackData, void* pUserData) {
        std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

        return VK_FALSE;
    }
    */
};

int main() {
    ComputeShaderApp app;

    try {
        app.run();
    } catch (const std::exception& exception) {
        fmt::println(std::cerr, "{}", exception.what());
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
