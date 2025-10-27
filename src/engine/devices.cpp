#include "engine.h"
#include <cstdint>
#include <set>
#include <vulkan/vulkan.h>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>
#include <algorithm>
#include <cstdint>
#include <limits>
#include <map>
#include <utility>

using namespace std;

vector<tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures>>
VulkanEngine::getAvailableDevices() {
  uint32_t deviceCount = 0;
  vkEnumeratePhysicalDevices(instance, &deviceCount, nullptr);

  if (deviceCount == 0) {
    throw runtime_error("failed to find GPUs with Vulkan support!");
  }

  vector<VkPhysicalDevice> physDevices(deviceCount);
  vkEnumeratePhysicalDevices(instance, &deviceCount, physDevices.data());

  vector<tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures>> availableDevices;
  for (VkPhysicalDevice &device : physDevices) {
    VkPhysicalDeviceProperties properties;
    VkPhysicalDeviceFeatures features;
    vkGetPhysicalDeviceProperties(device, &properties);
    vkGetPhysicalDeviceFeatures(device, &features);

    availableDevices.push_back(tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures>(
        device, properties, features));
  }

  return availableDevices;
}

bool VulkanEngine::checkDeviceExtensionSupport(VkPhysicalDevice device) {
  uint32_t extensionCount;
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, nullptr);

  std::vector<VkExtensionProperties> availableExtensions(extensionCount);
  vkEnumerateDeviceExtensionProperties(device, nullptr, &extensionCount, availableExtensions.data());

  set<std::string> requiredExtensions(deviceExtensions.begin(), deviceExtensions.end());

  for (const auto &extension : availableExtensions) {
    requiredExtensions.erase(extension.extensionName);
  }

  return requiredExtensions.empty();
}

QueueFamilyIndices VulkanEngine::findSuitableQueueFamiles(VkPhysicalDevice device) {
  QueueFamilyIndices indices;

  uint32_t queueFamilyCount = 0;
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, nullptr);

  vector<VkQueueFamilyProperties> queueFamilies(queueFamilyCount);
  vkGetPhysicalDeviceQueueFamilyProperties(device, &queueFamilyCount, queueFamilies.data());

  for (int i = 0; i < queueFamilies.size(); i++) {

    // check if queue can process graphics
    if (queueFamilies[i].queueFlags & VK_QUEUE_GRAPHICS_BIT && !indices.graphicsFamily.has_value())
      indices.graphicsFamily = i;

    VkBool32 presentationSupport = false;
    vkGetPhysicalDeviceSurfaceSupportKHR(device, i, surface, &presentationSupport);

    // check if queue can process presentation commands
    if (presentationSupport && !indices.presentFamily.has_value())
      indices.presentFamily = i;

    if (indices.isComplete())
      break;
  }
  return indices;
}

vector<VkDeviceQueueCreateInfo> VulkanEngine::buildQueueCreateInfos(QueueFamilyIndices indices) {

  float *queuePriority = new float(1.0f);
  VkDeviceQueueCreateInfo graphicsInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                       .queueFamilyIndex = indices.graphicsFamily.value(),
                                       .queueCount = 1,
                                       .pQueuePriorities = queuePriority};

  vector<VkDeviceQueueCreateInfo> infos = {graphicsInfo};

  if (indices.graphicsFamily.value() == indices.presentFamily.value()) {
    return infos;
  }

  VkDeviceQueueCreateInfo presentInfo{.sType = VK_STRUCTURE_TYPE_DEVICE_QUEUE_CREATE_INFO,
                                      .queueFamilyIndex = indices.presentFamily.value(),
                                      .queueCount = 1,
                                      .pQueuePriorities = queuePriority};
  infos.push_back(presentInfo);

  return infos;
}

SwapChainSupportDetails VulkanEngine::querySwapChainSupport(VkPhysicalDevice *device) {
  SwapChainSupportDetails details;

  vkGetPhysicalDeviceSurfaceCapabilitiesKHR(*device, surface, &details.capabilities);

  uint32_t formatCount;
  vkGetPhysicalDeviceSurfaceFormatsKHR(*device, surface, &formatCount, nullptr);

  if (formatCount != 0) {
    details.formats.resize(formatCount);
    vkGetPhysicalDeviceSurfaceFormatsKHR(*device, surface, &formatCount, details.formats.data());
  }

  uint32_t presentModeCount;
  vkGetPhysicalDeviceSurfacePresentModesKHR(*device, surface, &presentModeCount, nullptr);

  if (presentModeCount != 0) {
    details.presentModes.resize(presentModeCount);
    vkGetPhysicalDeviceSurfacePresentModesKHR(*device, surface, &presentModeCount,
                                              details.presentModes.data());
  }
  return details;
}

tuple<VkDevice, VkPhysicalDevice, QueueFamilies>
VulkanEngine::pickPhysicalDevice(optional<std::vector<const char *>> validationLayers) {
  // iterate through available devices

  // stack-rank available devices
  multimap<int, int> deviceList;
  int idx = 0;
  auto availableDevices = getAvailableDevices();
  for (tuple<VkPhysicalDevice, VkPhysicalDeviceProperties, VkPhysicalDeviceFeatures> &dev :
       availableDevices) {
    int score{0};

    if (get<1>(dev).deviceType & VK_PHYSICAL_DEVICE_TYPE_DISCRETE_GPU) {
      score += 1000;
    }

    score += get<1>(dev).limits.maxImageDimension2D;

    // check queue family support
    QueueFamilyIndices indices = findSuitableQueueFamiles(get<0>(dev));

    // check swap chain support
    SwapChainSupportDetails swapChainSupport = querySwapChainSupport(&get<0>(dev));
    bool swapChainAdequate = !swapChainSupport.formats.empty() && !swapChainSupport.presentModes.empty();

    if (!get<2>(dev).geometryShader || !indices.isComplete() || !checkDeviceExtensionSupport(get<0>(dev)) ||
        !swapChainAdequate) {
      score = 0;
    }

    deviceList.insert(make_pair(score, idx));

    idx++;
  }

  // pick the best device, initialize the queues
  if (deviceList.rbegin()->first <= 0) {
    throw std::runtime_error("failed to find GPUs with neccessary reuired feature support");
  }

  // construct device, init queues
  int physicalDeviceIdx = deviceList.rbegin()->second;
  VkPhysicalDevice selectedDevice = get<0>(availableDevices[physicalDeviceIdx]);
  VkPhysicalDeviceProperties selectedDeviceProps = get<1>(availableDevices[physicalDeviceIdx]);

  QueueFamilyIndices indices = findSuitableQueueFamiles(selectedDevice);

  auto queueInfos = buildQueueCreateInfos(indices);

  VkPhysicalDeviceFeatures deviceFeatures{};
  VkDeviceCreateInfo deviceCreateInfo{
      .sType = VK_STRUCTURE_TYPE_DEVICE_CREATE_INFO,
      .queueCreateInfoCount = static_cast<uint32_t>(queueInfos.size()),
      .pQueueCreateInfos = queueInfos.data(),
      .enabledLayerCount = 0,
      .enabledExtensionCount = static_cast<uint32_t>(deviceExtensions.size()),
      .ppEnabledExtensionNames = deviceExtensions.data(),
      .pEnabledFeatures = &deviceFeatures,
  };

  if (validationLayers.has_value()) {
    deviceCreateInfo.enabledLayerCount = static_cast<uint32_t>(validationLayers.value().size());
    deviceCreateInfo.ppEnabledLayerNames = validationLayers.value().data();
  }

  VkDevice device;
  if (vkCreateDevice(selectedDevice, &deviceCreateInfo, nullptr, &device) != VK_SUCCESS) {
    throw std::runtime_error("failed to create logical device!");
  }

  QueueFamilies queues{};

  vkGetDeviceQueue(device, indices.graphicsFamily.value(), 0, &queues.graphicsQueue);
  vkGetDeviceQueue(device, indices.presentFamily.value(), 0, &queues.presentQueue);

  // TODO free allocated floats

  cout << "Selected device: " << selectedDeviceProps.deviceName << endl;

  return tuple<VkDevice, VkPhysicalDevice, QueueFamilies>(device, selectedDevice, queues);
}

VkSurfaceFormatKHR chooseSwapSurfaceFormat(const std::vector<VkSurfaceFormatKHR> &availableFormats) {
  for (const auto &availableFormat : availableFormats) {
    if (availableFormat.format == VK_FORMAT_B8G8R8A8_SRGB &&
        availableFormat.colorSpace == VK_COLOR_SPACE_SRGB_NONLINEAR_KHR) {
      return availableFormat;
    }
  }
  return availableFormats[0];
}

VkPresentModeKHR chooseSwapPresentMode(const std::vector<VkPresentModeKHR> &availablePresentModes) {
  for (const auto &availablePresentMode : availablePresentModes) {
    if (availablePresentMode == VK_PRESENT_MODE_MAILBOX_KHR) {
      return availablePresentMode;
    }
  }

  return VK_PRESENT_MODE_FIFO_KHR;
}

VkExtent2D VulkanEngine::chooseSwapExtent(const VkSurfaceCapabilitiesKHR &capabilities) {
  if (capabilities.currentExtent.width != numeric_limits<uint32_t>::max()) {
    return capabilities.currentExtent;
  } else {
    int width, height;
    glfwGetFramebufferSize(window, &width, &height);

    VkExtent2D actualExtent = {static_cast<uint32_t>(width), static_cast<uint32_t>(height)};

    actualExtent.width =
        clamp(actualExtent.width, capabilities.minImageExtent.width, capabilities.maxImageExtent.width);
    actualExtent.height =
        clamp(actualExtent.height, capabilities.minImageExtent.height, capabilities.maxImageExtent.height);

    return actualExtent;
  }
}
