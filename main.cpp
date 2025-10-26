#include <cstdint>
#include <tuple>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "devices.h"
#include "shaders.h"
#include <bits/stdc++.h>
#include <cstdlib>
#include <cstring>
#include <stdexcept>
#include <vulkan/vulkan.h>

using namespace std;

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const bool enableValidationLayers = true;

std::vector<const char *> getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

VkResult CreateDebugUtilsMessengerEXT(VkInstance instance,
                                      const VkDebugUtilsMessengerCreateInfoEXT *pCreateInfo,
                                      const VkAllocationCallbacks *pAllocator,
                                      VkDebugUtilsMessengerEXT *pDebugMessenger) {
  auto func =
      (PFN_vkCreateDebugUtilsMessengerEXT)vkGetInstanceProcAddr(instance, "vkCreateDebugUtilsMessengerEXT");
  if (func != nullptr) {
    return func(instance, pCreateInfo, pAllocator, pDebugMessenger);
  } else {
    return VK_ERROR_EXTENSION_NOT_PRESENT;
  }
}

// https://vulkan-tutorial.com/en/Drawing_a_triangle/Setup/Validation_layers
class VulkanApplication {
public:
  VulkanApplication(int width, int height) : width(width), height(height) {}
  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }

  ~VulkanApplication() { this->cleanup(); }

  bool checkValidationLayerSupport() {
    uint32_t layerCount;
    vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

    vector<VkLayerProperties> availLayers(layerCount);
    vkEnumerateInstanceLayerProperties(&layerCount, availLayers.data());

    for (const char *layerName : validationLayers) {
      bool found = false;
      for (const auto &availLayer : availLayers) {
        if (strcmp(availLayer.layerName, layerName) == 0) {
          found = true;
          break;
        }
      }

      if (!found)
        return false;
    }

    return true;
  }
  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

    return VK_FALSE;
  }

private:
  int width, height;
  GLFWwindow *window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkQueue graphicsQueue;
  VkDevice device;
  VkPhysicalDevice physicalDevice;

  VkSurfaceKHR surface;
  VkQueue presentQueue;

  VkSwapchainKHR swapChain;
  vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  vector<VkImageView> swapChainImageViews;

  const vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

  void initWindow() {
    glfwInit();
    glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
    glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

    window = glfwCreateWindow(width, height, "Intro to Vulkan", nullptr, nullptr);
  }
  void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSwapChain();
    createImageViews();
    createGraphicsPipeline();
  }

  void createSurface() {
    if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
      throw std::runtime_error("failed to create window surface!");
    }
  }

  void createGraphicsPipeline() {
    auto vertShaderCode = readFile("shaders/vert.spv");
    auto fragShaderCode = readFile("shaders/frag.spv");

    VkShaderModule vertShaderModule = createShaderModule(vertShaderCode, &this->device);
    VkShaderModule fragShaderModule = createShaderModule(fragShaderCode, &this->device);

    vkDestroyShaderModule(device, fragShaderModule, nullptr);
    vkDestroyShaderModule(device, vertShaderModule, nullptr);
  }

  void createImageViews() {
    swapChainImageViews.resize(swapChainImages.size());

    for (size_t i = 0; i < swapChainImages.size(); i++) {
      VkImageViewCreateInfo imageViewCreateInfo{.sType = VK_STRUCTURE_TYPE_IMAGE_VIEW_CREATE_INFO,
                                                .image = swapChainImages[i],
                                                .viewType = VK_IMAGE_VIEW_TYPE_2D,
                                                .format = swapChainImageFormat,
                                                .components = {.r = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                               .g = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                               .b = VK_COMPONENT_SWIZZLE_IDENTITY,
                                                               .a = VK_COMPONENT_SWIZZLE_IDENTITY},
                                                .subresourceRange = {.aspectMask = VK_IMAGE_ASPECT_COLOR_BIT,
                                                                     .baseMipLevel = 0,
                                                                     .levelCount = 1,
                                                                     .baseArrayLayer = 0,
                                                                     .layerCount = 1}};
      if (vkCreateImageView(device, &imageViewCreateInfo, nullptr, &swapChainImageViews[i]) != VK_SUCCESS) {
        throw std::runtime_error("failed to create image views!");
      }
    }
  }

  void createSwapChain() {
    SwapChainSupportDetails swapChainSupport =
        PhysicalDeviceManager::querySwapChainSupport(&this->physicalDevice, &this->surface);

    VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
    VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
    VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities, this->window);

    uint32_t imageCount = swapChainSupport.capabilities.minImageCount + 1;
    if (swapChainSupport.capabilities.maxImageCount > 0)
      imageCount = min(imageCount, swapChainSupport.capabilities.maxImageCount);

    VkSwapchainCreateInfoKHR swapChainCreateInfo{.sType = VK_STRUCTURE_TYPE_SWAPCHAIN_CREATE_INFO_KHR,
                                                 .surface = this->surface,
                                                 .minImageCount = imageCount,
                                                 .imageFormat = surfaceFormat.format,
                                                 .imageColorSpace = surfaceFormat.colorSpace,
                                                 .imageExtent = extent,
                                                 .imageArrayLayers = 1,
                                                 .imageUsage = VK_IMAGE_USAGE_COLOR_ATTACHMENT_BIT,
                                                 .imageSharingMode = VK_SHARING_MODE_EXCLUSIVE,
                                                 .queueFamilyIndexCount = 0,
                                                 .pQueueFamilyIndices = nullptr,
                                                 .preTransform =
                                                     swapChainSupport.capabilities.currentTransform,
                                                 .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                                 .presentMode = presentMode,
                                                 .clipped = VK_TRUE,
                                                 .oldSwapchain = VK_NULL_HANDLE};

    // VK_SHARING_MODE_EXCLUSIVE is faster, but if the queues differ than we have to handle
    // ownership (TODO)

    QueueFamilyIndices indices =
        PhysicalDeviceManager::findSuitableQueueFamiles(this->physicalDevice, &this->surface);

    uint32_t queueFamilyIndices[] = {indices.presentFamily.value(), indices.graphicsFamily.value()};
    if (indices.graphicsFamily != indices.presentFamily) {
      swapChainCreateInfo.imageSharingMode = VK_SHARING_MODE_CONCURRENT;
      swapChainCreateInfo.queueFamilyIndexCount = 2;
      swapChainCreateInfo.pQueueFamilyIndices = queueFamilyIndices;
    }

    if (vkCreateSwapchainKHR(device, &swapChainCreateInfo, nullptr, &swapChain) != VK_SUCCESS) {
      throw std::runtime_error("failed to create swap chain!");
    }

    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, nullptr);
    swapChainImages.resize(imageCount);
    vkGetSwapchainImagesKHR(device, swapChain, &imageCount, swapChainImages.data());

    swapChainImageFormat = surfaceFormat.format;
    swapChainExtent = extent;
  }

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
    }
  }

  void cleanup() {
    for (auto imageView : swapChainImageViews) {
      vkDestroyImageView(device, imageView, nullptr);
    }

    vkDestroySwapchainKHR(device, swapChain, nullptr);

    if (enableValidationLayers) {
      // DestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
    }

    vkDestroySurfaceKHR(instance, surface, nullptr);

    vkDestroyInstance(instance, nullptr);

    glfwDestroyWindow(window);

    glfwTerminate();
  }

  void populateDebugMessengerCreateInfo(VkDebugUtilsMessengerCreateInfoEXT &createInfo) {
    createInfo = {.sType = VK_STRUCTURE_TYPE_DEBUG_UTILS_MESSENGER_CREATE_INFO_EXT,
                  .messageSeverity = VK_DEBUG_UTILS_MESSAGE_SEVERITY_VERBOSE_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_WARNING_BIT_EXT |
                                     VK_DEBUG_UTILS_MESSAGE_SEVERITY_ERROR_BIT_EXT,
                  .messageType = VK_DEBUG_UTILS_MESSAGE_TYPE_GENERAL_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_VALIDATION_BIT_EXT |
                                 VK_DEBUG_UTILS_MESSAGE_TYPE_PERFORMANCE_BIT_EXT,
                  .pfnUserCallback = debugCallback};
  }

  void setupDebugMessenger() {
    if (!enableValidationLayers)
      return;

    VkDebugUtilsMessengerCreateInfoEXT createInfo;
    populateDebugMessengerCreateInfo(createInfo);

    if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
      throw std::runtime_error("failed to set up debug messenger!");
    }
  }

  void createInstance() {
    if (enableValidationLayers && !checkValidationLayerSupport()) {
      throw runtime_error("Validation layers requested!");
    }

    VkApplicationInfo appInfo{.sType = VK_STRUCTURE_TYPE_APPLICATION_INFO,
                              .pApplicationName = "Hello Triangle!",
                              .pEngineName = "No Engine",
                              .engineVersion = VK_MAKE_VERSION(1, 0, 0),
                              .apiVersion = VK_API_VERSION_1_1};

    auto extensions = getRequiredExtensions();
    VkInstanceCreateInfo createInfo{.sType = VK_STRUCTURE_TYPE_INSTANCE_CREATE_INFO,
                                    .pApplicationInfo = &appInfo,
                                    .enabledExtensionCount = static_cast<uint32_t>(extensions.size()),
                                    .ppEnabledExtensionNames = extensions.data()};

    VkDebugUtilsMessengerCreateInfoEXT debugCreateInfo{};
    if (enableValidationLayers) {
      createInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.size());
      createInfo.ppEnabledLayerNames = validationLayers.data();

      populateDebugMessengerCreateInfo(debugCreateInfo);
      createInfo.pNext = (VkDebugUtilsMessengerCreateInfoEXT *)&debugCreateInfo;
    } else {
      createInfo.enabledLayerCount = 0;
      createInfo.pNext = nullptr;
    }

    if (vkCreateInstance(&createInfo, nullptr, &instance) != VK_SUCCESS) {
      throw runtime_error("Failed to create instance!");
    }

    createSurface();

    tuple<VkDevice, VkPhysicalDevice, QueueFamilies> deviceSetup;
    if (enableValidationLayers) {
      deviceSetup = PhysicalDeviceManager::pickPhysicalDevice(&instance, &surface, validationLayers);
    } else {
      deviceSetup = PhysicalDeviceManager::pickPhysicalDevice(&instance, &surface, {});
    }

    device = get<0>(deviceSetup);
    physicalDevice = get<1>(deviceSetup);
    presentQueue = get<2>(deviceSetup).presentQueue;
    graphicsQueue = get<2>(deviceSetup).graphicsQueue;
  }
};

int main() {
  VulkanApplication app(800, 600);
  app.checkValidationLayerSupport();

  try {
    app.run();
  } catch (const std::exception &e) {
    std::cerr << e.what() << std::endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
