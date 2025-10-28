#include "engine.h"
#include <fstream>
#include <iostream>
#include <string>
#include <vector>

#include <glm/glm.hpp>

using namespace std;

vector<char> readFile(const std::string &filename) {
  ifstream file(filename, ios::ate | ios::binary);

  if (!file.is_open()) {
    throw runtime_error("failed to open file!");
  }

  size_t fileSize = (size_t)file.tellg();
  vector<char> buffer(fileSize);

  file.seekg(0);
  file.read(buffer.data(), fileSize);

  file.close();

  return buffer;
}

VkShaderModule VulkanEngine::createShaderModule(const std::vector<char> &code) {
  VkShaderModuleCreateInfo shaderCreateInfo{.sType = VK_STRUCTURE_TYPE_SHADER_MODULE_CREATE_INFO,
                                            .codeSize = code.size(),
                                            .pCode = reinterpret_cast<const uint32_t *>(code.data())};

  VkShaderModule shaderModule;
  if (vkCreateShaderModule(device, &shaderCreateInfo, nullptr, &shaderModule) != VK_SUCCESS) {
    throw std::runtime_error("failed to create shader module!");
  }

  return shaderModule;
}
