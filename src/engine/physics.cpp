#include "engine.h"

void RigidBody::loadVertices() {
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

void RigidBody::loadIndices() {
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

void RigidBody::loadToGpu() {
  loadVertices();
  loadIndices();
}

void RigidBody::cleanup() {
  vkDestroyBuffer(engine->device, vertexBuffer, nullptr);
  vkDestroyBuffer(engine->device, indexBuffer, nullptr);
  vkFreeMemory(engine->device, vertexMemory, nullptr);
  vkFreeMemory(engine->device, indexMemory, nullptr);
}

RigidBody RigidBody::createSphere(float x, float y, float r, glm::vec3 color, VulkanEngine *engine) {
  const int increments = 10000;
  RigidBody sphere{{}, {}};
  std::vector<Vertex> verts;
  std::vector<uint32_t> indices;
  verts.push_back({{x, y}, color});
  for (int i = 0; i < increments; i++) {
    float theta = (2 * PI * i) / increments;
    verts.push_back({{r * cosf(theta) + x, r * sinf(theta) + y}, color});
  }
  for (int i = 0; i < increments; i++) {
    uint32_t last_index = i + 2;
    if (i == increments - 1) {
      last_index = 1;
    }
    indices.insert(indices.end(), {0, (uint32_t)i + 1, last_index});
  }
  return {.vertices = verts, .indices = indices, .engine = engine};
}

RigidBody RigidBody::createSquare(float x, float y, float half_length, glm::vec3 color,
                                  VulkanEngine *engine) {
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
