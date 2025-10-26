#pragma once
#include <cstdint>
#include <stdlib.h>
#include <tuple>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include <cstdlib>
#include <cstring>
#include <iostream>
#include <map>
#include <optional>
#include <set>
#include <stdexcept>
#include <vector>
#include <vulkan/vulkan.h>

using namespace std;

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

class PhysicalDeviceManager {
public:
  const static std::vector<const char *> deviceExtensions;

  static QueueFamilyIndices findSuitableQueueFamiles(VkPhysicalDevice device, VkSurfaceKHR *surface);

  static vector<VkDeviceQueueCreateInfo> buildQueueCreateInfos(QueueFamilyIndices);

  static tuple<VkDevice, VkPhysicalDevice, QueueFamilies>
  pickPhysicalDevice(VkInstance *instance, VkSurfaceKHR *surface,
                     optional<std::vector<const char *>> validationLayers);
  static bool checkDeviceExtensionSupport(VkPhysicalDevice device);

  static vector<tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures>>
  getAvailableDevices(VkInstance *instance);

  static SwapChainSupportDetails querySwapChainSupport(VkPhysicalDevice *device, VkSurfaceKHR *surface);
};

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats);
VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes);
VkExtent2D chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities, GLFWwindow *window);
