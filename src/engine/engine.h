#pragma once
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <fstream>
#include <iostream>
#include <optional>
#include <string>

using namespace std;

const vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices {
  optional<uint32_t> graphicsFamily;
  optional<uint32_t> presentFamily;

  bool isComplete() { return graphicsFamily.has_value() && presentFamily.has_value(); }
};

struct SwapChainSupportDetails {
  VkSurfaceCapabilitiesKHR capabilities;
  std::vector<VkSurfaceFormatKHR> formats;
  std::vector<VkPresentModeKHR> presentModes;
};

struct QueueFamilies {
  VkQueue presentQueue;
  VkQueue graphicsQueue;
};

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);

class VulkanEngine {
private:
  bool enableValidationLayers;

  string windowName;
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
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  vector<VkFramebuffer> swapChainFramebuffers;

  VkCommandPool commandPool;

  vector<VkCommandBuffer> commandBuffers;
  vector<VkSemaphore> imageAvailableSemaphores;
  vector<VkSemaphore> renderFinishedSemaphores;
  vector<VkFence> inFlightFences;

  uint32_t currentFrame = 0;

  int max_inflight_frames;
  bool frameBufferResized = false;

  // physical device management
  QueueFamilyIndices findSuitableQueueFamiles(VkPhysicalDevice device);
  vector<VkDeviceQueueCreateInfo> buildQueueCreateInfos(QueueFamilyIndices);
  tuple<VkDevice, VkPhysicalDevice, QueueFamilies>
  pickPhysicalDevice(optional<std::vector<const char *>> validationLayers);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice *device);
  vector<tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures>> getAvailableDevices();
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    cerr << "validation layer: " << pCallbackData->pMessage << endl;

    return VK_FALSE;
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

  void initWindow();
  void createSurface();

  void createInstance();
  void setupDebugMessenger();
  void createSwapChain();
  void createImageViews();
  void createRenderPass();
  void createGraphicsPipeline();
  void createFramebuffers();
  void createCommandPool();
  void createCommandBuffer();
  void createSyncObjects();

  void initVulkan() {
    createInstance();
    setupDebugMessenger();
    createSwapChain();
    createImageViews();
    createRenderPass();
    createGraphicsPipeline();
    createFramebuffers();
    createCommandPool();
    createCommandBuffer();
    createSyncObjects();
  }

  void drawFrame();
  void recordCommandBuffer(VkCommandBuffer commandBuffer, uint32_t imageIndex);

  void mainLoop() {
    while (!glfwWindowShouldClose(window)) {
      glfwPollEvents();
      drawFrame();
    }

    vkDeviceWaitIdle(device);
  }

  void cleanup();
  void cleanupSwapChain();

public:
  VulkanEngine(int width, int height, int max_inflight_frames, bool enableValidationLayers, string windowName)
      : width(width), height(height), max_inflight_frames(max_inflight_frames),
        enableValidationLayers(enableValidationLayers), windowName(windowName) {}
  VkShaderModule createShaderModule(const std::vector<char> &code);
  bool checkValidationLayerSupport();
  vector<const char *> getRequiredExtensions();

  ~VulkanEngine() { this->cleanup(); }

  void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
  }

  void run() {
    initWindow();
    initVulkan();
    mainLoop();
    cleanup();
  }
};

std::vector<char> readFile(const std::string &filename);
VkShaderModule createShaderModule(const std::vector<char> &code, VkDevice *device);
