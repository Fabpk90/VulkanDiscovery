#define GLFW_INCLUDE_VULKAN
#include <GLFW/glfw3.h>

#include "Application.h"

constexpr int32_t height = 600;
constexpr int32_t width = 800;


int main() {
    Application app(height, width, "Testing Vulkan");

    app.run();

    return 0;
}
