#include <vulkan/vulkan.h>

#include <iostream>
#include <stdexcept>
#include <cstdlib>
#include <vector>

#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <fmt/core.h>

const uint32_t WIDTH = 800;
const uint32_t HEIGHT = 600;

class App {
public:
    void run() {
        initWindow();
        initVulkan();
        mainLoop();
        cleanup();
    }

    void enumerateExtensions() {
        uint32_t extensionCount = 0;
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, nullptr);

        auto extensions = std::vector<VkExtensionProperties> { extensionCount };
        vkEnumerateInstanceExtensionProperties(nullptr, &extensionCount, extensions.data());

        for (const auto& extension: extensions) {
            fmt::println("NAME: {} ; VERSION: {}", extension.extensionName, extension.specVersion);
        }
    }
private:
    void initWindow() {
        glfwInit();
        glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
        glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

        m_window = glfwCreateWindow(WIDTH, HEIGHT, "Vulkan Engine", nullptr, nullptr);
    }

    void createInstance() {
        auto appInfo = VkApplicationInfo {};
        appInfo.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO;
        appInfo.pApplicationName = "Hello Window";
        appInfo.applicationVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.pEngineName = "No Engine";
        appInfo.engineVersion = VK_MAKE_VERSION(1, 0, 0);
        appInfo.apiVersion = VK_API_VERSION_1_0;

        auto createInfo = VkInstanceCreateInfo {};
        createInfo.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO;
        createInfo.pApplicationInfo = &appInfo;

        uint32_t glfwExtensionCount = 0;
        const char** glfwExtensionNames;
        glfwExtensionNames = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

        createInfo.enabledExtensionCount = glfwExtensionCount;
        createInfo.ppEnabledExtensionNames = glfwExtensionNames;
        createInfo.enabledLayerCount = 0;

        std::vector<const char*> requiredExtensions;
        for(uint32_t i = 0; i < glfwExtensionCount; i++) {
            requiredExtensions.emplace_back(glfwExtensionNames[i]);
        }

        requiredExtensions.emplace_back(VK_KHR_PORTABILITY_ENUMERATION_EXTENSION_NAME);

        createInfo.flags |= VK_INSTANCE_CREATE_ENUMERATE_PORTABILITY_BIT_KHR;

        createInfo.enabledExtensionCount = (uint32_t) requiredExtensions.size();
        createInfo.ppEnabledExtensionNames = requiredExtensions.data();

        VkResult result = vkCreateInstance(&createInfo, nullptr, &instance);
        if (result != VK_SUCCESS) {
            throw std::runtime_error("failed to create instance!");
        }
    }

    void initVulkan() {
        this->createInstance();
    }

    void mainLoop() {
        while (!glfwWindowShouldClose(m_window)) {
            glfwPollEvents();
        }
    }

    void cleanup() {
        vkDestroyInstance(instance, nullptr);
        glfwDestroyWindow(m_window);
        glfwTerminate();
    }

    GLFWwindow* m_window;
    VkInstance instance;
};

int main() {
    App app;

    try {
        app.run();
    } catch (const std::exception& e) {
        std::cerr << e.what() << std::endl;
        return EXIT_FAILURE;
    }

    return EXIT_SUCCESS;
}
