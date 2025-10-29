cmake_minimum_required(VERSION 3.10)
project(VulkanTest)

# Set C++ standard
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

# Set optimization level
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O2")

# Find required packages
find_package(Vulkan REQUIRED)
find_package(glfw3 REQUIRED)

# Compile shaders
set(SHADER_SOURCE_DIR ${CMAKE_SOURCE_DIR}/shaders)
set(SHADER_BINARY_DIR ${CMAKE_BINARY_DIR}/shaders)

# List source files 
set(SOURCE_FILES 
	src/main.cpp
	src/engine/devices.cpp
	src/engine/frames.cpp
	src/engine/render_pipeline.cpp
	src/engine/setup.cpp
	src/engine/shaders.cpp
	src/engine/validation.cpp
	src/engine/vertex.cpp
	src/engine/physics.cpp
)

file(MAKE_DIRECTORY ${SHADER_BINARY_DIR})

add_custom_command(
    OUTPUT ${SHADER_BINARY_DIR}/vert.spv
    COMMAND glslc ${SHADER_SOURCE_DIR}/shader.vert -o ${SHADER_BINARY_DIR}/vert.spv
    DEPENDS ${SHADER_SOURCE_DIR}/shader.vert
    COMMENT "Compiling vertex shader"
)

add_custom_command(
    OUTPUT ${SHADER_BINARY_DIR}/frag.spv
    COMMAND glslc ${SHADER_SOURCE_DIR}/shader.frag -o ${SHADER_BINARY_DIR}/frag.spv
    DEPENDS ${SHADER_SOURCE_DIR}/shader.frag
    COMMENT "Compiling fragment shader"
)

add_custom_target(Shaders
    DEPENDS ${SHADER_BINARY_DIR}/vert.spv ${SHADER_BINARY_DIR}/frag.spv
)

# Create executable
add_executable(VulkanTest ${SOURCE_FILES})
add_dependencies(VulkanTest Shaders)

# Link libraries
target_link_libraries(VulkanTest
    glfw
    Vulkan::Vulkan
    ${CMAKE_DL_LIBS}
    pthread
    X11
    # Xxf86vm
    Xrandr
    Xi
)

# Add test target (equivalent to 'make test')
add_custom_target(test
    COMMAND VulkanTest
    DEPENDS VulkanTest
    WORKING_DIRECTORY ${CMAKE_CURRENT_BINARY_DIR}
    COMMENT "Running VulkanTest"
)
