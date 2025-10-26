#pragma once

#include <string>
#include <vector>
#include <vulkan/vulkan_core.h>
#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

std::vector<char> readFile(const std::string &filename);

VkShaderModule createShaderModule(const std::vector<char> &code, VkDevice *device);
