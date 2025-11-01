#include "engine.h"
#include <vulkan/vulkan_core.h>
using namespace std;

void VulkanEngine::initVulkan() {
  cout << "C++ VERSION " << __cplusplus << endl;
  createInstance();
  setupDebugMessenger();
  createSwapChain();
  createImageViews();
  createRenderPass();
  createGraphicsPipeline();
  createFramebuffers();
  createCommandPool();
  initializeTransferBuffer();
  createGeometries();

  createCommandBuffers();
  createSyncObjects();
}

void VulkanEngine::initWindow() {
  glfwInit();
  glfwWindowHint(GLFW_CLIENT_API, GLFW_NO_API);
  glfwWindowHint(GLFW_RESIZABLE, GLFW_FALSE);

  window = glfwCreateWindow(width, height, windowName, nullptr, nullptr);
  glfwSetWindowUserPointer(window, this); // pass arbistrary pointer
  glfwSetFramebufferSizeCallback(window, framebufferResizeCallback);
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

vector<const char *> VulkanEngine::getRequiredExtensions() {
  uint32_t glfwExtensionCount = 0;
  const char **glfwExtensions;
  glfwExtensions = glfwGetRequiredInstanceExtensions(&glfwExtensionCount);

  std::vector<const char *> extensions(glfwExtensions, glfwExtensions + glfwExtensionCount);

  if (enableValidationLayers) {
    extensions.push_back(VK_EXT_DEBUG_UTILS_EXTENSION_NAME);
  }

  return extensions;
}

void VulkanEngine::createSurface() {
  if (glfwCreateWindowSurface(instance, window, nullptr, &surface) != VK_SUCCESS) {
    throw runtime_error("failed to create window surface!");
  }
}

void VulkanEngine::createInstance() {

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
    deviceSetup = pickPhysicalDevice(validationLayers);
  } else {
    deviceSetup = pickPhysicalDevice({});
  }

  device = get<0>(deviceSetup);
  physicalDevice = get<1>(deviceSetup);
  presentQueue = get<2>(deviceSetup).presentQueue;
  graphicsQueue = get<2>(deviceSetup).graphicsQueue;
}

void VulkanEngine::setupDebugMessenger() {
  if (!enableValidationLayers)
    return;

  VkDebugUtilsMessengerCreateInfoEXT createInfo;
  populateDebugMessengerCreateInfo(createInfo);

  if (CreateDebugUtilsMessengerEXT(instance, &createInfo, nullptr, &debugMessenger) != VK_SUCCESS) {
    throw std::runtime_error("failed to set up debug messenger!");
  }
}

void VulkanEngine::createSwapChain() {
  SwapChainSupportDetails swapChainSupport = querySwapChainSupport(&physicalDevice);

  VkSurfaceFormatKHR surfaceFormat = chooseSwapSurfaceFormat(swapChainSupport.formats);
  VkPresentModeKHR presentMode = chooseSwapPresentMode(swapChainSupport.presentModes);
  VkExtent2D extent = chooseSwapExtent(swapChainSupport.capabilities);

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
                                               .preTransform = swapChainSupport.capabilities.currentTransform,
                                               .compositeAlpha = VK_COMPOSITE_ALPHA_OPAQUE_BIT_KHR,
                                               .presentMode = presentMode,
                                               .clipped = VK_TRUE,
                                               .oldSwapchain = VK_NULL_HANDLE};

  // VK_SHARING_MODE_EXCLUSIVE is faster, but if the queues differ than we have to handle
  // ownership (TODO)

  QueueFamilyIndices indices = findSuitableQueueFamiles(this->physicalDevice);

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

void VulkanEngine::createImageViews() {
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
      throw runtime_error("failed to create image views!");
    }
  }
}

void VulkanEngine::createFramebuffers() {
  swapChainFramebuffers.resize(swapChainImageViews.size());

  for (size_t i = 0; i < swapChainImageViews.size(); i++) {
    VkImageView attachments[] = {swapChainImageViews[i]};

    VkFramebufferCreateInfo framebufferInfo{
        .sType = VK_STRUCTURE_TYPE_FRAMEBUFFER_CREATE_INFO,
        .renderPass = renderPass,
        .attachmentCount = 1,
        .pAttachments = attachments,
        .width = swapChainExtent.width,
        .height = swapChainExtent.height,
        .layers = 1,
    };

    if (vkCreateFramebuffer(device, &framebufferInfo, nullptr, &swapChainFramebuffers[i]) != VK_SUCCESS) {
      throw runtime_error("failed to create framebuffer!");
    }
  }
}

void VulkanEngine::createCommandPool() {
  QueueFamilyIndices queueFamilyIndices = findSuitableQueueFamiles(physicalDevice);

  VkCommandPoolCreateInfo poolInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_POOL_CREATE_INFO,
      .flags = VK_COMMAND_POOL_CREATE_RESET_COMMAND_BUFFER_BIT,
      .queueFamilyIndex = queueFamilyIndices.graphicsFamily.value(),
  };

  if (vkCreateCommandPool(device, &poolInfo, nullptr, &commandPool) != VK_SUCCESS) {
    throw runtime_error("failed to create command pool!");
  }
}

uint32_t findMemoryType(VkPhysicalDevice *physicalDevice, uint32_t typeFilter,
                        VkMemoryPropertyFlags properties) {
  VkPhysicalDeviceMemoryProperties memProperties;
  vkGetPhysicalDeviceMemoryProperties(*physicalDevice, &memProperties);

  for (uint32_t i = 0; i < memProperties.memoryTypeCount; i++) {
    if ((typeFilter & (1 << i)) && (memProperties.memoryTypes[i].propertyFlags & properties) == properties) {
      return i;
    }
  }

  throw runtime_error("failed to find suitable memory type!");
}

void VulkanEngine::createCommandBuffers() {
  commandBuffers.resize(max_inflight_frames);

  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = (uint32_t)commandBuffers.size(),
  };

  if (vkAllocateCommandBuffers(device, &allocInfo, commandBuffers.data()) != VK_SUCCESS) {
    throw runtime_error("failed to allocate command buffers!");
  }
}

void VulkanEngine::createSyncObjects() {
  VkSemaphoreCreateInfo semaphoreInfo{.sType = VK_STRUCTURE_TYPE_SEMAPHORE_CREATE_INFO};
  VkFenceCreateInfo fenceInfo{.sType = VK_STRUCTURE_TYPE_FENCE_CREATE_INFO,
                              .flags = VK_FENCE_CREATE_SIGNALED_BIT};

  imageAvailableSemaphores.resize(max_inflight_frames);
  renderFinishedSemaphores.resize(max_inflight_frames);
  inFlightFences.resize(max_inflight_frames);

  for (size_t i = 0; i < max_inflight_frames; i++) {
    if (vkCreateSemaphore(device, &semaphoreInfo, nullptr, &imageAvailableSemaphores[i]) != VK_SUCCESS ||
        vkCreateSemaphore(device, &semaphoreInfo, nullptr, &renderFinishedSemaphores[i]) != VK_SUCCESS ||
        vkCreateFence(device, &fenceInfo, nullptr, &inFlightFences[i]) != VK_SUCCESS) {
      throw runtime_error("failed to create semaphores!");
    }
  }
}

void VulkanEngine::cleanupSwapChain() {
  for (auto framebuffer : swapChainFramebuffers) {
    vkDestroyFramebuffer(device, framebuffer, nullptr);
  }

  for (auto imageView : swapChainImageViews) {
    vkDestroyImageView(device, imageView, nullptr);
  }

  vkDestroySwapchainKHR(device, swapChain, nullptr);
}

void VulkanEngine::cleanup() {
  for (size_t i = 0; i < max_inflight_frames; i++) {
    vkDestroySemaphore(device, renderFinishedSemaphores[i], nullptr);
    vkDestroySemaphore(device, imageAvailableSemaphores[i], nullptr);
    vkDestroyFence(device, inFlightFences[i], nullptr);
  }

  vkDestroyCommandPool(device, commandPool, nullptr);

  vkDestroyPipeline(device, graphicsPipeline, nullptr);
  vkDestroyPipelineLayout(device, pipelineLayout, nullptr);
  vkDestroyRenderPass(device, renderPass, nullptr);

  cleanupSwapChain();

  if (enableValidationLayers) {
    // vkDestroyDebugUtilsMessengerEXT(instance, debugMessenger, nullptr);
  }

  vkFreeCommandBuffers(device, commandPool, 1, &transferCommandBuffer);

  vkDestroySurfaceKHR(instance, surface, nullptr);

  vkDestroyInstance(instance, nullptr);

  glfwDestroyWindow(window);

  glfwTerminate();
}
