#pragma once
#include <cmath>
#include <cstdint>
#include <glm/detail/qualifier.hpp>
#include <vector>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <array>
#include <cstring>
#include <fstream>
#include <glm/glm.hpp>
#include <iostream>
#include <math.h>
#include <optional>
#include <string>

const float PI = 3.141592653;
struct RigidBody;

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer &buffer, VkDeviceMemory &bufferMemory, VkDeviceSize offset, VkDevice *device,
                  VkPhysicalDevice *physDevice);

uint32_t findMemoryType(VkPhysicalDevice *device, uint32_t typeFilter, VkMemoryPropertyFlags properties);

// shader vertex inputs
struct Vertex {
  glm::vec2 pos;
  glm::vec3 color;

  static VkVertexInputBindingDescription getBindingDescription() {
    VkVertexInputBindingDescription bindingDescription{
        .binding = 0, .stride = sizeof(Vertex), .inputRate = VK_VERTEX_INPUT_RATE_VERTEX};

    return bindingDescription;
  }

  static std::array<VkVertexInputAttributeDescription, 2> getAttributeDescriptions() {
    std::array<VkVertexInputAttributeDescription, 2> attributeDescriptions{};

    attributeDescriptions[0] = {
        .location = 0, .binding = 0, .format = VK_FORMAT_R32G32_SFLOAT, .offset = offsetof(Vertex, pos)};
    attributeDescriptions[1] = {
        .location = 1,
        .binding = 0,
        .format = VK_FORMAT_R32G32B32_SFLOAT,
        .offset = offsetof(Vertex, color),
    };
    return attributeDescriptions;
  }
};

const std::vector<const char *> validationLayers = {"VK_LAYER_KHRONOS_validation"};
const std::vector<const char *> deviceExtensions = {VK_KHR_SWAPCHAIN_EXTENSION_NAME};

struct QueueFamilyIndices {
  std::optional<uint32_t> graphicsFamily;
  std::optional<uint32_t> presentFamily;

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

  const char *windowName;
  int width, height;
  GLFWwindow *window;
  VkInstance instance;
  VkDebugUtilsMessengerEXT debugMessenger;

  VkQueue graphicsQueue;

  VkSurfaceKHR surface;
  VkQueue presentQueue;

  VkSwapchainKHR swapChain;
  std::vector<VkImage> swapChainImages;
  VkFormat swapChainImageFormat;
  VkExtent2D swapChainExtent;

  std::vector<VkImageView> swapChainImageViews;
  VkRenderPass renderPass;
  VkPipelineLayout pipelineLayout;
  VkPipeline graphicsPipeline;

  std::vector<VkFramebuffer> swapChainFramebuffers;

  VkCommandPool commandPool;

  std::vector<VkCommandBuffer> commandBuffers;
  std::vector<VkSemaphore> imageAvailableSemaphores;
  std::vector<VkSemaphore> renderFinishedSemaphores;
  std::vector<VkFence> inFlightFences;

  uint32_t currentFrame = 0;

  int max_inflight_frames;

  // transfer command buffer bulk queueing
  std::vector<RigidBody> geometries;
  VkCommandBuffer transferCommandBuffer;

  // physical device management
  QueueFamilyIndices findSuitableQueueFamiles(VkPhysicalDevice device);
  std::vector<VkDeviceQueueCreateInfo> buildQueueCreateInfos(QueueFamilyIndices);
  std::tuple<VkDevice, VkPhysicalDevice, QueueFamilies>
  pickPhysicalDevice(std::optional<std::vector<const char *>> validationLayers);
  bool checkDeviceExtensionSupport(VkPhysicalDevice device);
  SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice *device);
  std::vector<std::tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures>>
  getAvailableDevices();
  VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities);

  static VKAPI_ATTR VkBool32 VKAPI_CALL debugCallback(
      VkDebugUtilsMessageSeverityFlagBitsEXT messageSeverity, VkDebugUtilsMessageTypeFlagsEXT messageType,
      const VkDebugUtilsMessengerCallbackDataEXT *pCallbackData, void *pUserData) {

    std::cerr << "validation layer: " << pCallbackData->pMessage << std::endl;

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

  void createGeometries();

  void createCommandBuffers();
  void createSyncObjects();

  void initVulkan();

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
  bool frameBufferResized = false;
  VkDevice device;
  VkPhysicalDevice physicalDevice;

  VulkanEngine(int width, int height, int max_inflight_frames, bool enableValidationLayers,
               const char *windowName)
      : width(width), height(height), max_inflight_frames(max_inflight_frames),
        enableValidationLayers(enableValidationLayers), windowName(windowName) {}
  VkShaderModule createShaderModule(const std::vector<char> &code);
  bool checkValidationLayerSupport();
  std::vector<const char *> getRequiredExtensions();

  void initializeTransferBuffer();
  void copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size);
  void endTransfers();
  void waitForTransfers();
  void beginTransfers();

  ~VulkanEngine() { this->cleanup(); }

  void recreateSwapChain() {
    vkDeviceWaitIdle(device);

    cleanupSwapChain();

    createSwapChain();
    createImageViews();
    createFramebuffers();
  }

  static void framebufferResizeCallback(GLFWwindow *window, int width, int height) {
    auto app = reinterpret_cast<VulkanEngine *>(glfwGetWindowUserPointer(window));
    app->frameBufferResized = true;
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

struct RigidBody {
  std::vector<Vertex> vertices;
  std::vector<uint32_t> indices;
  VulkanEngine *engine;

  VkBuffer vertexBuffer, indexBuffer;
  VkDeviceMemory vertexMemory, indexMemory;

  void loadVertices() {
    VkDeviceSize bufferSize = sizeof(Vertex) * vertices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                 stagingBufferMemory, 0, &engine->device, &engine->physicalDevice);

    void *data;
    vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, vertices.data(), (size_t)bufferSize);
    vkUnmapMemory(engine->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexMemory, 0, &engine->device,
                 &engine->physicalDevice);

    engine->beginTransfers();
    engine->copyBuffer(stagingBuffer, vertexBuffer, bufferSize);
    engine->endTransfers();
    engine->waitForTransfers();

    vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
    vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
  }

  void loadIndices() {
    VkDeviceSize bufferSize = sizeof(uint32_t) * indices.size();

    VkBuffer stagingBuffer;
    VkDeviceMemory stagingBufferMemory;
    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
                 VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT, stagingBuffer,
                 stagingBufferMemory, 0, &engine->device, &engine->physicalDevice);

    void *data;
    vkMapMemory(engine->device, stagingBufferMemory, 0, bufferSize, 0, &data);
    memcpy(data, indices.data(), (size_t)bufferSize);
    vkUnmapMemory(engine->device, stagingBufferMemory);

    createBuffer(bufferSize, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
                 VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexMemory, 0, &engine->device,
                 &engine->physicalDevice);

    engine->beginTransfers();
    engine->copyBuffer(stagingBuffer, indexBuffer, bufferSize);
    engine->endTransfers();
    engine->waitForTransfers();

    vkDestroyBuffer(engine->device, stagingBuffer, nullptr);
    vkFreeMemory(engine->device, stagingBufferMemory, nullptr);
  }

  void loadToGpu() {
    loadVertices();
    loadIndices();
  }

  void cleanup() {
    vkDestroyBuffer(engine->device, vertexBuffer, nullptr);
    vkDestroyBuffer(engine->device, indexBuffer, nullptr);
    vkFreeMemory(engine->device, vertexMemory, nullptr);
    vkFreeMemory(engine->device, indexMemory, nullptr);
  }

  static RigidBody createSphere(float x, float y, float r, glm::vec3 color, VulkanEngine *engine) {
    const int increments = 10000;
    RigidBody sphere{{}, {}};
    std::vector<Vertex> verts;
    std::vector<uint32_t> indices;
    verts.push_back({{x, y}, color});
    for (int i = 0; i < increments; i++) {
      float theta = (2 * PI * i) / increments;
      verts.push_back({{r * cosf(theta) + x, r * sinf(theta) + y}, color});
    }
    for (int i = 0; i < increments; i++)
      indices.insert(indices.end(), {0, (uint32_t)i + 1, (uint32_t)(i % increments) + 1});

    return {.vertices = verts, .indices = indices, .engine = engine};
  }

  static RigidBody createSquare(float x, float y, float half_length, glm::vec3 color, VulkanEngine *engine) {
    return {.vertices =
                {
                    {{x - half_length, y - half_length}, color},
                    {{x + half_length, y - half_length}, color},
                    {{x + half_length, y + half_length}, color},
                    {{x - half_length, y + half_length}, color},
                },
            .indices = {0, 1, 2, 2, 3, 0},
            .engine = engine};
  }
};
