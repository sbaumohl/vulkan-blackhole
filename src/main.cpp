#include "engine/engine.h"
#include <cstdlib>
#include <vulkan/vulkan.h>

using namespace std;

int main() {
  VulkanEngine engine(800, 800, 2, true, "Vulkan Engine working!");
  engine.checkValidationLayerSupport();

  try {
    engine.run();
  } catch (const exception &e) {
    cerr << e.what() << endl;
    return EXIT_FAILURE;
  }

  return EXIT_SUCCESS;
}
