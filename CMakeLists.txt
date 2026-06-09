cmake_minimum_required(VERSION 3.20)
project(RenderingSandbox LANGUAGES C CXX)

# ---------------------------------------------------------------------------
# Language standard: C++17 (modern C++ requirement of the role)
# ---------------------------------------------------------------------------
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

if(NOT CMAKE_BUILD_TYPE)
    set(CMAKE_BUILD_TYPE Release CACHE STRING "Build type" FORCE)
endif()

# Emit compile_commands.json so clangd / IntelliSense resolve Homebrew include
# paths (glm, GLFW, ...) correctly instead of showing false include errors.
set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

# ---------------------------------------------------------------------------
# Dependencies (Homebrew, resolved via find_package)
#   glfw3  -> imported target `glfw`     (pulls Cocoa/IOKit/CoreVideo on macOS)
#   glm    -> imported target `glm::glm` (header-only)
#   OpenGL -> imported target `OpenGL::GL` (links OpenGL.framework on macOS)
# ---------------------------------------------------------------------------
find_package(glfw3 REQUIRED)
find_package(glm REQUIRED)
find_package(OpenGL REQUIRED)

# ---------------------------------------------------------------------------
# GLAD: vendored OpenGL 4.1 Core loader (generated into third_party/glad/)
# Compiled as its own static lib so it builds once with C, not C++.
# ---------------------------------------------------------------------------
add_library(glad STATIC third_party/glad/src/glad.c)
target_include_directories(glad PUBLIC third_party/glad/include)

# ---------------------------------------------------------------------------
# Application target
# ---------------------------------------------------------------------------
file(GLOB_RECURSE SANDBOX_SOURCES CONFIGURE_DEPENDS src/*.cpp)
add_executable(RenderingSandbox ${SANDBOX_SOURCES})

target_include_directories(RenderingSandbox PRIVATE src third_party/stb)
target_link_libraries(RenderingSandbox PRIVATE
    glad
    glfw
    OpenGL::GL
    glm::glm
)

# macOS: OpenGL is deprecated in favor of Metal; silence the deprecation spam.
target_compile_definitions(RenderingSandbox PRIVATE GL_SILENCE_DEPRECATION)

# Absolute path to the project root so the app can locate shaders/ and assets/
# regardless of the working directory it is launched from.
target_compile_definitions(RenderingSandbox PRIVATE RS_PROJECT_ROOT="${CMAKE_SOURCE_DIR}")
