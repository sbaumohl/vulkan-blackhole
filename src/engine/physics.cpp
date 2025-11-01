#include "engine.h"
#include <cstdlib>
#include <utility>
#include <vulkan/vulkan_core.h>

RigidBody RigidBody::createSphere(float x, float y, float r, glm::vec3 color) {
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
  return {.vertices = verts, .indices = indices};
}

RigidBody RigidBody::createSquare(float x, float y, float half_length, glm::vec3 color) {
  return {
      .vertices =
          {
              {{x - half_length, y - half_length}, color},
              {{x + half_length, y - half_length}, color},
              {{x + half_length, y + half_length}, color},
              {{x - half_length, y + half_length}, color},
          },
      .indices = {0, 1, 2, 2, 3, 0},
  };
}

RigidBodyManager::RigidBodyManager(VulkanEngine *engine) : engine(engine) {}

RigidBodyManager::~RigidBodyManager() {
  vkDestroyBuffer(engine->device, this->vertexBuffer, nullptr);
  vkDestroyBuffer(engine->device, this->indexBuffer, nullptr);
  vkFreeMemory(engine->device, this->vertexMemory, nullptr);
  vkFreeMemory(engine->device, this->indexMemory, nullptr);

  this->geometries.clear();
}

void RigidBodyManager::calculateOffsets() {

  VkDeviceSize idxOffset, vertexOffset;
  idxOffset = 0;
  vertexOffset = 0;

  for (auto &[name, rb] : this->geometries) {
    rb.vertexOffset = vertexOffset;
    rb.indexOffset = idxOffset;

    vertexOffset += rb.getVertSize();
    idxOffset += rb.getIndexSize();
  }
}

std::pair<VkDeviceSize, VkDeviceSize> RigidBodyManager::getSizes() {
  VkDeviceSize idxSz = 0, vertSz = 0;
  for (auto &[name, rb] : this->geometries) {
    vertSz += rb.getVertSize();
    idxSz += rb.getIndexSize();
  }
  return std::make_pair(idxSz, vertSz);
}

void RigidBodyManager::loadToGpu() {
  this->calculateOffsets();
  auto sizes = this->getSizes();
  VkDeviceSize vertexBufferSz = std::get<1>(sizes);
  VkDeviceSize indicesBufferSz = std::get<0>(sizes);

  std::cout << "sizes: " << vertexBufferSz << " " << indicesBufferSz << std::endl;

  // copy from our structs into staging buffer
  VkBuffer vertexStagingBuffer, indicesStagingBuffer;
  VkDeviceMemory vertexStagingBufferMemory, indicesStagingBufferMemory;

  createBuffer(vertexBufferSz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               vertexStagingBuffer, vertexStagingBufferMemory, 0, &engine->device, &engine->physicalDevice);

  createBuffer(indicesBufferSz, VK_BUFFER_USAGE_TRANSFER_SRC_BIT,
               VK_MEMORY_PROPERTY_HOST_VISIBLE_BIT | VK_MEMORY_PROPERTY_HOST_COHERENT_BIT,
               indicesStagingBuffer, indicesStagingBufferMemory, 0, &engine->device, &engine->physicalDevice);

  void *vertexData = nullptr, *indexData = nullptr;

  vkMapMemory(engine->device, vertexStagingBufferMemory, 0, vertexBufferSz, 0, &vertexData);
  vkMapMemory(engine->device, indicesStagingBufferMemory, 0, indicesBufferSz, 0, &indexData);

  for (const auto &[name, rb] : this->geometries) {
    void *v_ptr = static_cast<void *>(((char *)vertexData) + rb.vertexOffset);
    void *i_ptr = static_cast<void *>(((char *)indexData) + rb.indexOffset);
    std::memcpy(v_ptr, rb.vertices.data(), (size_t)rb.vertices.size() * sizeof(rb.vertices[0]));
    std::memcpy(i_ptr, rb.indices.data(), (size_t)rb.indices.size() * sizeof(rb.indices[0]));
  }

  vkUnmapMemory(engine->device, vertexStagingBufferMemory);
  vkUnmapMemory(engine->device, indicesStagingBufferMemory);
  vertexData = nullptr;
  indexData = nullptr;

  // create our GPU-only buffers, begin transfer
  createBuffer(vertexBufferSz, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_VERTEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, vertexBuffer, vertexMemory, 0, &engine->device,
               &engine->physicalDevice);

  createBuffer(indicesBufferSz, VK_BUFFER_USAGE_TRANSFER_DST_BIT | VK_BUFFER_USAGE_INDEX_BUFFER_BIT,
               VK_MEMORY_PROPERTY_DEVICE_LOCAL_BIT, indexBuffer, indexMemory, 0, &engine->device,
               &engine->physicalDevice);

  engine->beginTransfers();
  engine->copyBuffer(vertexStagingBuffer, vertexBuffer, vertexBufferSz);
  engine->copyBuffer(indicesStagingBuffer, indexBuffer, indicesBufferSz);
  engine->endTransfers();
  engine->waitForTransfers();

  // clean staging buffers
  vkDestroyBuffer(engine->device, vertexStagingBuffer, nullptr);
  vkDestroyBuffer(engine->device, indicesStagingBuffer, nullptr);
  vkFreeMemory(engine->device, vertexStagingBufferMemory, nullptr);
  vkFreeMemory(engine->device, indicesStagingBufferMemory, nullptr);
}
