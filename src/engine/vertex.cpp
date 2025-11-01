#include "engine.h"
#include <format>
#include <iostream>
#include <string>

#include <vulkan/vulkan_core.h>

using namespace std;

void VulkanEngine::initializeTransferBuffer() {
  VkCommandBufferAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_ALLOCATE_INFO,
      .commandPool = commandPool,
      .level = VK_COMMAND_BUFFER_LEVEL_PRIMARY,
      .commandBufferCount = 1,
  };

  vkAllocateCommandBuffers(device, &allocInfo, &this->transferCommandBuffer);
}

void VulkanEngine::beginTransfers() {
  VkCommandBufferBeginInfo beginInfo{
      .sType = VK_STRUCTURE_TYPE_COMMAND_BUFFER_BEGIN_INFO,
      .flags = VK_COMMAND_BUFFER_USAGE_ONE_TIME_SUBMIT_BIT,
  };

  vkBeginCommandBuffer(this->transferCommandBuffer, &beginInfo);
}

void VulkanEngine::endTransfers() {
  vkEndCommandBuffer(this->transferCommandBuffer);

  VkSubmitInfo submitInfo{
      .sType = VK_STRUCTURE_TYPE_SUBMIT_INFO,
      .commandBufferCount = 1,
      .pCommandBuffers = &this->transferCommandBuffer,
  };

  vkQueueSubmit(graphicsQueue, 1, &submitInfo, VK_NULL_HANDLE);
}

void VulkanEngine::waitForTransfers() {
  // vkWaitForFences(device, 1, &transferFence, VK_TRUE, UINT64_MAX);
  vkQueueWaitIdle(graphicsQueue);
}

void VulkanEngine::copyBuffer(VkBuffer srcBuffer, VkBuffer dstBuffer, VkDeviceSize size) {
  VkBufferCopy copyRegion{
      copyRegion.srcOffset = 0,
      copyRegion.dstOffset = 0,
      copyRegion.size = size,
  };
  vkCmdCopyBuffer(this->transferCommandBuffer, srcBuffer, dstBuffer, 1, &copyRegion);
}

void createBuffer(VkDeviceSize size, VkBufferUsageFlags usage, VkMemoryPropertyFlags properties,
                  VkBuffer &buffer, VkDeviceMemory &bufferMemory, VkDeviceSize offset, VkDevice *device,
                  VkPhysicalDevice *physDevice) {
  VkBufferCreateInfo bufferInfo{
      .sType = VK_STRUCTURE_TYPE_BUFFER_CREATE_INFO,
      .size = size,
      .usage = usage,
      .sharingMode = VK_SHARING_MODE_EXCLUSIVE,
  };

  if (vkCreateBuffer(*device, &bufferInfo, nullptr, &buffer) != VK_SUCCESS) {
    throw runtime_error("failed to create buffer!");
  }

  VkMemoryRequirements memRequirements;
  vkGetBufferMemoryRequirements(*device, buffer, &memRequirements);

  VkMemoryAllocateInfo allocInfo{
      .sType = VK_STRUCTURE_TYPE_MEMORY_ALLOCATE_INFO,
      .allocationSize = memRequirements.size,
      .memoryTypeIndex = findMemoryType(physDevice, memRequirements.memoryTypeBits, properties),
  };

  if (vkAllocateMemory(*device, &allocInfo, nullptr, &bufferMemory) != VK_SUCCESS) {
    throw runtime_error("failed to allocate buffer memory!");
  }

  vkBindBufferMemory(*device, buffer, bufferMemory, offset);
}

void VulkanEngine::createGeometries() {
  for (int i = 0; i < 10; i++)
    for (int j = 0; j < 10; j++)
      this->rigidBodyManager.geometries.insert(
          {std::format("N {} {}", i, j),
           RigidBody::createSphere(0.075 * i - 0.5, 0.075 * j - 0.5, 0.05, {0.0f, 0.0f, 1.0f})});

  this->rigidBodyManager.loadToGpu();
}
