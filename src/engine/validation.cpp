#include "cstring"
#include "engine.h"

bool VulkanEngine::checkValidationLayerSupport() {
  uint32_t layerCount;
  vkEnumerateInstanceLayerProperties(&layerCount, nullptr);

  vector<VkLayerProperties> availLayers(layerCount);
  vkEnumerateInstanceLayerProperties(&layerCount, availLayers.data());

  for (const char *layerName : validationLayers) {
    bool found = false;
    for (const auto &availLayer : availLayers) {
      if (strcmp(availLayer.layerName, layerName) == 0) {
        found = true;
        break;
      }
    }

    if (!found)
      return false;
  }

  return true;
}
