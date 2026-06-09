#pragma once

#include "Core/Window.h"
#include "Scene/Camera.h"

#include <glad/glad.h>
#include <glm/glm.hpp>
#include <memory>
#include <array>

class Shader;
class Texture;
class Mesh;
class Model;
class Input;
class ShadowPass;
class Framebuffer;
class Cubemap;

class Application
{
public:
    Application();
    ~Application();
    int run();

private:
    // --- geometry builders ---
    static std::unique_ptr<Mesh> buildCube();
    static std::unique_ptr<Mesh> buildPlane(float size = 10.0f, float uvTile = 5.0f);

    // --- per-frame helpers ---
    void processInput(float dt);
    void setLightUniforms(Shader& s, float time);
    void drawSceneGeometry(Shader& s, bool frustumCull = false);
    void drawSceneShaded();

    // --- phase pipeline helpers ---
    void initDeferredPipeline(int w, int h);
    void resizeFBOs(int w, int h);
    void renderDeferred(float time, float aspect, int fbw, int fbh);
    void renderFullscreenQuad();
    void renderSkybox(const glm::mat4& view, const glm::mat4& proj);
    void renderDebugUI(float dt);

    // === CORE ===
    Window                   m_window;
    Camera                   m_camera;
    std::unique_ptr<Input>   m_input;

    // === GEOMETRY ===
    std::unique_ptr<Mesh>    m_cubeMesh;
    std::unique_ptr<Mesh>    m_planeMesh;
    std::unique_ptr<Model>   m_model;

    // === PHASE 10: Forward + Shadow ===
    std::unique_ptr<Shader>     m_lightingShader;
    std::unique_ptr<Shader>     m_depthShader;
    std::unique_ptr<ShadowPass> m_shadowPass;
    std::unique_ptr<Texture>    m_cubeTexture;
    std::unique_ptr<Texture>    m_defaultWhite;
    std::unique_ptr<Texture>    m_defaultGrey;

    // === PHASE 11/12: Framebuffer + Deferred ===
    GLuint m_emptyVAO = 0;
    std::unique_ptr<Framebuffer> m_gBuffer;       // 4 MRT: pos/normal/albedo+spec/material
    std::unique_ptr<Framebuffer> m_hdrFBO;        // 1 RGBA16F colour + depth
    std::unique_ptr<Shader>      m_gbufferShader;
    std::unique_ptr<Shader>      m_deferredLightShader;

    // === PHASE 13/14: HDR + Bloom ===
    std::unique_ptr<Framebuffer> m_bloomFBO[2];   // ping-pong at half res
    std::unique_ptr<Shader>      m_tonemapShader;
    std::unique_ptr<Shader>      m_brightPassShader;
    std::unique_ptr<Shader>      m_blurShader;

    // === PHASE 16: Skybox ===
    std::unique_ptr<Shader>  m_skyboxShader;
    std::unique_ptr<Cubemap> m_skybox;

    // === PHASE 17: Frustum culling ===
    bool m_frustumCulling = true;
    int  m_lastDrawCalls  = 0;

    // === PHASE 18: ImGui + profiling ===
    bool  m_useDeferred     = true;
    bool  m_usePBR          = false;
    bool  m_bloomEnabled    = true;
    float m_bloomThreshold  = 1.0f;
    int   m_bloomIterations = 5;
    float m_exposure        = 1.0f;
    int   m_tonemapMode     = 1;   // 0=Reinhard 1=ACES
    int   m_debugView       = 0;   // 0=final 1=pos 2=normal 3=albedo 4=material 5=shadow
    GLuint m_gpuQuery       = 0;
    bool   m_queryActive    = false;
    double m_gpuMs          = 0.0;
    double m_cpuMs          = 0.0;
    int    m_drawCallCount  = 0;
    bool   m_showUI         = true;

    float m_lastFrame = 0.0f;
    int   m_lastFBW   = 0;
    int   m_lastFBH   = 0;
};
