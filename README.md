# 3D Renderer

A real-time 3D renderer built with C++ and OpenGL 4.1.

## Features

- Deferred rendering with PBR (Cook-Torrance BRDF) and Blinn-Phong shading
- Shadow mapping with PCF filtering
- HDR pipeline with bloom and tone mapping (Reinhard / ACES Filmic)
- Procedural skybox
- Frustum culling
- 3D model loading via Assimp
- Dear ImGui debug panel

## Dependencies

- GLFW 3
- GLM
- Assimp
- OpenGL 4.1 (via GLAD)
- Dear ImGui
- stb_image

## Build

```bash
# macOS
brew install glfw glm assimp cmake

cmake -B build -DCMAKE_BUILD_TYPE=Release
cmake --build build -j$(nproc)
./build/3d-render
```

## Controls

| Input | Action |
|---|---|
| W A S D | Move |
| Space / Shift | Up / Down |
| Mouse | Look around |
| F1 | Toggle deferred / forward |
| F2 | Toggle PBR / Blinn-Phong |
| F3 | Toggle bloom |
| Tab | Debug panel |
| Esc | Quit |
