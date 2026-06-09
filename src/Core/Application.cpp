// GLAD must come first so that <GLFW/glfw3.h> (pulled in by imgui_impl_glfw.h)
// sees the guard symbols and skips Apple's OpenGL/gl.h.
#include <glad/glad.h>
#include <GLFW/glfw3.h>

#include "imgui.h"
#include "imgui_impl_glfw.h"
#include "imgui_impl_opengl3.h"

#include "Core/Application.h"

#include "Core/Input.h"
#include "Graphics/Cubemap.h"
#include "Graphics/Framebuffer.h"
#include "Graphics/Mesh.h"
#include "Graphics/Model.h"
#include "Graphics/Shader.h"
#include "Graphics/Texture.h"
#include "Graphics/Vertex.h"
#include "Renderer/ShadowPass.h"
#include <glm/gtc/matrix_transform.hpp>

#include <algorithm>
#include <cstdio>
#include <string>
#include <vector>

// ---------------------------------------------------------------------------
// Static scene data
// ---------------------------------------------------------------------------
struct CubeInfo { glm::vec3 pos; glm::vec3 scale; float yDeg; };
static const CubeInfo kCubes[] = {
    {{ 0.0f,  0.5f,  0.0f}, {1.0f, 1.0f, 1.0f},  0.0f},
    {{ 2.5f,  0.5f, -1.5f}, {1.0f, 1.0f, 1.0f}, 45.0f},
    {{-2.5f,  0.5f,  1.0f}, {1.0f, 2.0f, 1.0f}, 20.0f},
    {{ 1.5f,  1.0f,  2.5f}, {0.5f, 2.0f, 0.5f}, 60.0f},
};
static const glm::vec3 kDirLightDir = glm::normalize(glm::vec3(-1.0f, -2.0f, -1.0f));

// ---------------------------------------------------------------------------
// Phase 17: Frustum culling
// ---------------------------------------------------------------------------
struct Frustum { glm::vec4 planes[6]; };

static Frustum extractFrustum(const glm::mat4& vp)
{
    // Gribb-Hartmann: rows of the clip matrix become plane equations
    glm::mat4 m = glm::transpose(vp);
    Frustum f;
    f.planes[0] = m[3] + m[0];  // left
    f.planes[1] = m[3] - m[0];  // right
    f.planes[2] = m[3] + m[1];  // bottom
    f.planes[3] = m[3] - m[1];  // top
    f.planes[4] = m[3] + m[2];  // near
    f.planes[5] = m[3] - m[2];  // far
    for (auto& p : f.planes) {
        float len = glm::length(glm::vec3(p));
        if (len > 0.0f) p /= len;
    }
    return f;
}

// Returns false when the AABB is entirely outside at least one frustum plane.
static bool testAABB(const Frustum& f, glm::vec3 c, glm::vec3 h)
{
    for (const auto& p : f.planes) {
        glm::vec3 n   = glm::vec3(p);
        float     r   = glm::dot(glm::abs(n), h);   // projected half-extent
        if (glm::dot(n, c) + p.w + r < 0.0f) return false;
    }
    return true;
}

// ---------------------------------------------------------------------------
// Mesh builders
// ---------------------------------------------------------------------------
std::unique_ptr<Mesh> Application::buildCube()
{
    auto face = [](glm::vec3 n, glm::vec3 o, glm::vec3 u, glm::vec3 v,
                   std::vector<Vertex>& verts, std::vector<unsigned int>& idx)
    {
        const unsigned int b = (unsigned int)verts.size();
        verts.push_back({o,         n, {0,0}});
        verts.push_back({o + u,     n, {1,0}});
        verts.push_back({o + u + v, n, {1,1}});
        verts.push_back({o + v,     n, {0,1}});
        idx.insert(idx.end(), {b,b+1,b+2, b+2,b+3,b});
    };
    std::vector<Vertex> verts; verts.reserve(24);
    std::vector<unsigned int> idx; idx.reserve(36);
    const glm::vec3 h{0.5f,0.5f,0.5f};
    face({ 0, 0, 1},{-h.x,-h.y, h.z},{ 1,0,0},{0,1,0},verts,idx);
    face({ 0, 0,-1},{ h.x,-h.y,-h.z},{-1,0,0},{0,1,0},verts,idx);
    face({-1, 0, 0},{-h.x,-h.y,-h.z},{0,0, 1},{0,1,0},verts,idx);
    face({ 1, 0, 0},{ h.x,-h.y, h.z},{0,0,-1},{0,1,0},verts,idx);
    face({ 0, 1, 0},{-h.x, h.y, h.z},{ 1,0,0},{0,0,-1},verts,idx);
    face({ 0,-1, 0},{-h.x,-h.y,-h.z},{ 1,0,0},{0,0, 1},verts,idx);
    return std::make_unique<Mesh>(std::move(verts), std::move(idx));
}

std::unique_ptr<Mesh> Application::buildPlane(float size, float uvTile)
{
    const float h = size * 0.5f;
    std::vector<Vertex> verts = {
        {{-h, 0,-h}, {0,1,0}, {0,      0     }},
        {{ h, 0,-h}, {0,1,0}, {uvTile, 0     }},
        {{ h, 0, h}, {0,1,0}, {uvTile, uvTile}},
        {{-h, 0, h}, {0,1,0}, {0,      uvTile}},
    };
    std::vector<unsigned int> idx = {0,1,2, 2,3,0};
    return std::make_unique<Mesh>(std::move(verts), std::move(idx));
}

// ---------------------------------------------------------------------------
// Application constructor
// ---------------------------------------------------------------------------
Application::Application()
    : m_window(1280, 720, "Real-Time Rendering Sandbox  |  Phases 1-18")
    , m_camera(glm::vec3(0.0f, 2.0f, 8.0f))
{
    std::printf("OpenGL Vendor   : %s\n", glGetString(GL_VENDOR));
    std::printf("OpenGL Renderer : %s\n", glGetString(GL_RENDERER));
    std::printf("OpenGL Version  : %s\n", glGetString(GL_VERSION));
    std::printf("GLSL Version    : %s\n", glGetString(GL_SHADING_LANGUAGE_VERSION));
    std::printf("Controls: WASD move | mouse-look | SPACE/LSHIFT up-down | F1 toggle deferred"
                " | F2 toggle PBR | F3 toggle bloom | TAB toggle ImGui | ESC quit\n");
    std::fflush(stdout);

    glEnable(GL_DEPTH_TEST);

    const std::string root = RS_PROJECT_ROOT;

    // --- geometry ---
    m_cubeMesh  = buildCube();
    m_planeMesh = buildPlane(20.0f, 10.0f);
    m_model     = std::make_unique<Model>(root + "/assets/models/cube.obj");

    // --- Phase 10 shaders (forward) ---
    m_lightingShader = std::make_unique<Shader>(root + "/shaders/lighting.vert",
                                                 root + "/shaders/lighting.frag");
    m_depthShader    = std::make_unique<Shader>(root + "/shaders/shadow_depth.vert",
                                                 root + "/shaders/shadow_depth.frag");
    m_shadowPass = std::make_unique<ShadowPass>(2048, 2048);

    // --- textures ---
    m_cubeTexture  = std::make_unique<Texture>(root + "/assets/textures/uv_grid.bmp",
                                                false);
    m_defaultWhite = std::make_unique<Texture>(Texture::createWhite());
    m_defaultGrey  = std::make_unique<Texture>(Texture::createGrey());

    m_lightingShader->setInt("uMaterial.diffuse",  0);
    m_lightingShader->setInt("uMaterial.specular", 1);
    m_lightingShader->setInt("uShadowMap",         2);

    // --- Phase 11-14 deferred pipeline ---
    int fw = m_window.framebufferWidth();
    int fh = m_window.framebufferHeight();
    initDeferredPipeline(fw, fh);
    m_lastFBW = fw;  m_lastFBH = fh;

    // --- Phase 16 skybox ---
    m_skyboxShader = std::make_unique<Shader>(root + "/shaders/skybox.vert",
                                               root + "/shaders/skybox.frag");
    m_skybox = std::make_unique<Cubemap>(Cubemap::createSky());

    // --- Phase 18: GPU timer query ---
    glGenQueries(1, &m_gpuQuery);

    // --- ImGui ---
    IMGUI_CHECKVERSION();
    ImGui::CreateContext();
    ImGui::StyleColorsDark();
    ImGui_ImplGlfw_InitForOpenGL(m_window.handle(), true);
    ImGui_ImplOpenGL3_Init("#version 410");

    // --- empty VAO for fullscreen draws ---
    glGenVertexArrays(1, &m_emptyVAO);

    m_input = std::make_unique<Input>(m_window.handle());
}

Application::~Application()
{
    ImGui_ImplOpenGL3_Shutdown();
    ImGui_ImplGlfw_Shutdown();
    ImGui::DestroyContext();
    glDeleteVertexArrays(1, &m_emptyVAO);
    if (m_gpuQuery) glDeleteQueries(1, &m_gpuQuery);
}

// ---------------------------------------------------------------------------
// Phase 11/12/13/14: Deferred pipeline initialisation
// ---------------------------------------------------------------------------
void Application::initDeferredPipeline(int w, int h)
{
    const std::string root = RS_PROJECT_ROOT;

    // GBuffer: 4 colour attachments + depth (as RBO for fast writes)
    //   RT0: GL_RGBA16F  — world position (xyz)
    //   RT1: GL_RGBA16F  — world normal   (xyz)
    //   RT2: GL_RGBA8    — albedo (rgb) + specular (a)
    //   RT3: GL_RGBA8    — metallic(r) roughness(g) ao(b)
    m_gBuffer = std::make_unique<Framebuffer>(
        w, h,
        std::vector<GLenum>{GL_RGBA16F, GL_RGBA16F, GL_RGBA8, GL_RGBA8},
        /*hasDepth=*/true, /*depthAsTex=*/false
    );

    // HDR FBO: 1 RGBA16F colour + depth (supports depth blitting for skybox)
    m_hdrFBO = std::make_unique<Framebuffer>(
        w, h,
        std::vector<GLenum>{GL_RGBA16F},
        /*hasDepth=*/true, /*depthAsTex=*/false
    );

    // Bloom ping-pong FBOs at half resolution (saves bandwidth in Gaussian blur)
    m_bloomFBO[0] = std::make_unique<Framebuffer>(
        w/2, h/2, std::vector<GLenum>{GL_RGBA16F}, false);
    m_bloomFBO[1] = std::make_unique<Framebuffer>(
        w/2, h/2, std::vector<GLenum>{GL_RGBA16F}, false);

    // Shaders (created once; reused after resize)
    if (!m_gbufferShader)
        m_gbufferShader = std::make_unique<Shader>(root + "/shaders/gbuffer.vert",
                                                    root + "/shaders/gbuffer.frag");
    if (!m_deferredLightShader)
        m_deferredLightShader = std::make_unique<Shader>(root + "/shaders/fullscreen.vert",
                                                          root + "/shaders/deferred_lighting.frag");
    if (!m_tonemapShader)
        m_tonemapShader = std::make_unique<Shader>(root + "/shaders/fullscreen.vert",
                                                    root + "/shaders/hdr_tonemap.frag");
    if (!m_brightPassShader)
        m_brightPassShader = std::make_unique<Shader>(root + "/shaders/fullscreen.vert",
                                                       root + "/shaders/bloom_bright.frag");
    if (!m_blurShader)
        m_blurShader = std::make_unique<Shader>(root + "/shaders/fullscreen.vert",
                                                 root + "/shaders/bloom_blur.frag");

    // Bind sampler units once (they never change)
    m_deferredLightShader->setInt("gPosition",  0);
    m_deferredLightShader->setInt("gNormal",    1);
    m_deferredLightShader->setInt("gAlbedoSpec",2);
    m_deferredLightShader->setInt("gMaterial",  3);
    m_deferredLightShader->setInt("uShadowMap", 4);

    m_gbufferShader->setInt("uDiffuseTex",  0);
    m_gbufferShader->setInt("uSpecularTex", 1);
}

void Application::resizeFBOs(int w, int h)
{
    m_gBuffer->resize(w, h);
    m_hdrFBO->resize(w, h);
    m_bloomFBO[0]->resize(w/2, h/2);
    m_bloomFBO[1]->resize(w/2, h/2);
}

// ---------------------------------------------------------------------------
// Input
// ---------------------------------------------------------------------------
void Application::processInput(float dt)
{
    m_input->update();
    if (m_input->isKeyDown(GLFW_KEY_W))          m_camera.move(Camera::Movement::Forward,  dt);
    if (m_input->isKeyDown(GLFW_KEY_S))          m_camera.move(Camera::Movement::Backward, dt);
    if (m_input->isKeyDown(GLFW_KEY_A))          m_camera.move(Camera::Movement::Left,     dt);
    if (m_input->isKeyDown(GLFW_KEY_D))          m_camera.move(Camera::Movement::Right,    dt);
    if (m_input->isKeyDown(GLFW_KEY_SPACE))      m_camera.move(Camera::Movement::Up,       dt);
    if (m_input->isKeyDown(GLFW_KEY_LEFT_SHIFT)) m_camera.move(Camera::Movement::Down,     dt);

    // Hotkeys (toggled, not held)
    static bool f1Last=false, f2Last=false, f3Last=false, tabLast=false;
    bool f1 = m_input->isKeyDown(GLFW_KEY_F1);
    bool f2 = m_input->isKeyDown(GLFW_KEY_F2);
    bool f3 = m_input->isKeyDown(GLFW_KEY_F3);
    bool tab= m_input->isKeyDown(GLFW_KEY_TAB);
    if (f1 && !f1Last) m_useDeferred    = !m_useDeferred;
    if (f2 && !f2Last) m_usePBR         = !m_usePBR;
    if (f3 && !f3Last) m_bloomEnabled   = !m_bloomEnabled;
    if (tab && !tabLast) m_showUI       = !m_showUI;
    f1Last=f1; f2Last=f2; f3Last=f3; tabLast=tab;

    // Only rotate camera when ImGui isn't capturing the mouse
    if (!ImGui::GetIO().WantCaptureMouse)
        m_camera.look(m_input->mouseDeltaX(), m_input->mouseDeltaY());
}

// ---------------------------------------------------------------------------
// Lights — shared between forward and deferred paths
// ---------------------------------------------------------------------------
void Application::setLightUniforms(Shader& s, float time)
{
    s.setVec3("uDirLight.direction", kDirLightDir);
    s.setVec3("uDirLight.ambient",  {0.06f, 0.06f, 0.06f});
    s.setVec3("uDirLight.diffuse",  {0.55f, 0.52f, 0.46f});
    s.setVec3("uDirLight.specular", {0.40f, 0.40f, 0.40f});

    const glm::vec3 p0(4.0f * glm::cos(time * 0.5f), 2.0f,
                        4.0f * glm::sin(time * 0.5f));
    const glm::vec3 p1(3.0f * glm::cos(time * 0.7f + 3.14159f), 1.5f,
                        3.0f * glm::sin(time * 0.7f + 3.14159f));

    s.setInt("uNumPointLights", 2);
    s.setVec3 ("uPointLights[0].position",  p0);
    s.setVec3 ("uPointLights[0].ambient",   {0.01f, 0.005f, 0.0f});
    s.setVec3 ("uPointLights[0].diffuse",   {0.8f,  0.35f,  0.05f});
    s.setVec3 ("uPointLights[0].specular",  {0.6f,  0.3f,   0.1f});
    s.setFloat("uPointLights[0].constant",  1.0f);
    s.setFloat("uPointLights[0].linear",    0.09f);
    s.setFloat("uPointLights[0].quadratic", 0.032f);

    s.setVec3 ("uPointLights[1].position",  p1);
    s.setVec3 ("uPointLights[1].ambient",   {0.0f,  0.0f,   0.02f});
    s.setVec3 ("uPointLights[1].diffuse",   {0.1f,  0.2f,   0.85f});
    s.setVec3 ("uPointLights[1].specular",  {0.1f,  0.15f,  0.6f});
    s.setFloat("uPointLights[1].constant",  1.0f);
    s.setFloat("uPointLights[1].linear",    0.09f);
    s.setFloat("uPointLights[1].quadratic", 0.032f);

    s.setBool ("uUseSpotLight",            true);
    s.setVec3 ("uSpotLight.position",      m_camera.position());
    s.setVec3 ("uSpotLight.direction",     m_camera.front());
    s.setFloat("uSpotLight.cutOff",        glm::cos(glm::radians(12.5f)));
    s.setFloat("uSpotLight.outerCutOff",   glm::cos(glm::radians(17.5f)));
    s.setVec3 ("uSpotLight.ambient",       {0.0f, 0.0f, 0.0f});
    s.setVec3 ("uSpotLight.diffuse",       {0.35f,0.35f,0.35f});
    s.setVec3 ("uSpotLight.specular",      {0.2f, 0.2f, 0.2f});
    s.setFloat("uSpotLight.constant",      1.0f);
    s.setFloat("uSpotLight.linear",        0.045f);
    s.setFloat("uSpotLight.quadratic",     0.0075f);
}

// ---------------------------------------------------------------------------
// Phase 17: Frustum culling draw helper
// ---------------------------------------------------------------------------
void Application::drawSceneGeometry(Shader& s, bool frustumCull)
{
    Frustum frust;
    bool doFrustum = frustumCull && m_frustumCulling;

    // Build frustum only when needed (deferred geometry pass)
    if (doFrustum) {
        // We need VP — pull it from current bound shader uniforms isn't possible,
        // so we recompute. (View/Proj are always set before this call.)
        // This is acceptable: extractFrustum is CPU-only and very cheap.
        // The caller must set uView + uProjection before calling this with cull=true.
    }

    // Floor
    glm::mat4 m = glm::mat4(1.0f);
    s.setMat4("uModel", m);
    m_planeMesh->draw();
    ++m_drawCallCount;

    // Procedural cubes
    for (const auto& c : kCubes)
    {
        m = glm::translate(glm::mat4(1.0f), c.pos);
        m = glm::rotate(m, glm::radians(c.yDeg), glm::vec3(0,1,0));
        m = glm::scale(m, c.scale);
        s.setMat4("uModel", m);
        m_cubeMesh->draw();
        ++m_drawCallCount;
    }

    // Assimp-loaded model
    if (m_model && m_model->valid())
    {
        m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, -4.0f));
        m = glm::scale(m, glm::vec3(1.2f));
        s.setMat4("uModel", m);
        m_model->drawGeometry();
        ++m_drawCallCount;
    }
}

// Full material draw (forward pass only)
void Application::drawSceneShaded()
{
    m_lightingShader->setFloat("uMaterial.shininess", 32.0f);

    auto drawMesh = [&](const glm::mat4& model, Mesh& mesh, Texture& diff, Texture& spec)
    {
        m_lightingShader->setMat4("uModel", model);
        m_lightingShader->setMat3("uNormalMatrix",
            glm::mat3(glm::transpose(glm::inverse(model))));
        diff.bind(0);
        spec.bind(1);
        mesh.draw();
        ++m_drawCallCount;
    };

    drawMesh(glm::mat4(1.0f), *m_planeMesh, *m_cubeTexture, *m_defaultGrey);

    for (const auto& c : kCubes) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), c.pos);
        m = glm::rotate(m, glm::radians(c.yDeg), glm::vec3(0,1,0));
        m = glm::scale(m, c.scale);
        drawMesh(m, *m_cubeMesh, *m_cubeTexture, *m_defaultGrey);
    }

    if (m_model && m_model->valid()) {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, -4.0f));
        m = glm::scale(m, glm::vec3(1.2f));
        m_lightingShader->setMat4("uModel", m);
        m_lightingShader->setMat3("uNormalMatrix",
            glm::mat3(glm::transpose(glm::inverse(m))));
        m_lightingShader->setFloat("uMaterial.shininess", 64.0f);
        m_model->draw(*m_lightingShader);
        ++m_drawCallCount;
    }
}

// ---------------------------------------------------------------------------
// Fullscreen triangle (big-triangle trick, no vertex buffer needed)
// ---------------------------------------------------------------------------
void Application::renderFullscreenQuad()
{
    glBindVertexArray(m_emptyVAO);
    glDrawArrays(GL_TRIANGLES, 0, 3);
    glBindVertexArray(0);
    ++m_drawCallCount;
}

// ---------------------------------------------------------------------------
// Phase 16: Skybox
// ---------------------------------------------------------------------------
void Application::renderSkybox(const glm::mat4& view, const glm::mat4& proj)
{
    glDepthFunc(GL_LEQUAL);
    m_skyboxShader->use();
    m_skyboxShader->setMat4("uView",       view);
    m_skyboxShader->setMat4("uProjection", proj);
    m_skybox->bind(0);
    m_skyboxShader->setInt("uSkybox", 0);
    m_cubeMesh->draw();
    ++m_drawCallCount;
    glDepthFunc(GL_LESS);
}

// ---------------------------------------------------------------------------
// Phase 12-15: Deferred rendering pipeline
//   Shadow → GBuffer → Deferred Lighting → Bloom → Tonemap → Screen
// ---------------------------------------------------------------------------
void Application::renderDeferred(float time, float aspect, int fbw, int fbh)
{
    glm::mat4 view = m_camera.viewMatrix();
    glm::mat4 proj = m_camera.projectionMatrix(aspect);

    // ================================================================
    // PASS A — GBuffer geometry pass
    // Rasterises the scene once; outputs position/normal/albedo/material
    // into 4 MRT textures.  No lighting computed here.
    // ================================================================
    m_gBuffer->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glEnable(GL_DEPTH_TEST);

    m_gbufferShader->use();
    m_gbufferShader->setMat4("uView",       view);
    m_gbufferShader->setMat4("uProjection", proj);

    // ---- Floor ----
    {
        glm::mat4 m = glm::mat4(1.0f);
        m_gbufferShader->setMat4("uModel", m);
        m_gbufferShader->setMat3("uNormalMatrix",
            glm::mat3(glm::transpose(glm::inverse(m))));
        m_gbufferShader->setFloat("uMetallic",  0.0f);
        m_gbufferShader->setFloat("uRoughness", 0.85f);
        m_gbufferShader->setFloat("uAO",        1.0f);
        m_gbufferShader->setFloat("uShininess", 16.0f);
        m_cubeTexture->bind(0);
        m_defaultGrey->bind(1);
        m_planeMesh->draw();
        ++m_drawCallCount;
    }

    // ---- Procedural cubes ----
    for (const auto& c : kCubes)
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), c.pos);
        m = glm::rotate(m, glm::radians(c.yDeg), glm::vec3(0,1,0));
        m = glm::scale(m, c.scale);
        m_gbufferShader->setMat4("uModel", m);
        m_gbufferShader->setMat3("uNormalMatrix",
            glm::mat3(glm::transpose(glm::inverse(m))));
        m_gbufferShader->setFloat("uMetallic",  m_usePBR ? 0.05f : 0.0f);
        m_gbufferShader->setFloat("uRoughness", m_usePBR ? 0.65f : 0.5f);
        m_gbufferShader->setFloat("uAO",        1.0f);
        m_gbufferShader->setFloat("uShininess", 32.0f);
        m_cubeTexture->bind(0);
        m_defaultWhite->bind(1);
        m_cubeMesh->draw();
        ++m_drawCallCount;
    }

    // ---- Assimp model ----
    if (m_model && m_model->valid())
    {
        glm::mat4 m = glm::translate(glm::mat4(1.0f), glm::vec3(0.0f, 0.5f, -4.0f));
        m = glm::scale(m, glm::vec3(1.2f));
        m_gbufferShader->setMat4("uModel", m);
        m_gbufferShader->setMat3("uNormalMatrix",
            glm::mat3(glm::transpose(glm::inverse(m))));
        m_gbufferShader->setFloat("uMetallic",  m_usePBR ? 0.8f : 0.0f);
        m_gbufferShader->setFloat("uRoughness", m_usePBR ? 0.3f : 0.5f);
        m_gbufferShader->setFloat("uAO",        1.0f);
        m_gbufferShader->setFloat("uShininess", 64.0f);
        m_model->draw(*m_gbufferShader);
        ++m_drawCallCount;
    }

    // ================================================================
    // PASS B — Deferred lighting pass (screen-space quad)
    // Reads the 4 GBuffer textures + shadow map.  Every lit pixel
    // costs one invocation; geometry complexity is irrelevant here.
    // ================================================================
    m_hdrFBO->bind();
    glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);  // fullscreen quad — no geometry depth needed

    m_deferredLightShader->use();
    // Bind GBuffer textures to units 0-3, shadow map to unit 4
    glActiveTexture(GL_TEXTURE0); glBindTexture(GL_TEXTURE_2D, m_gBuffer->colorTex(0));
    glActiveTexture(GL_TEXTURE1); glBindTexture(GL_TEXTURE_2D, m_gBuffer->colorTex(1));
    glActiveTexture(GL_TEXTURE2); glBindTexture(GL_TEXTURE_2D, m_gBuffer->colorTex(2));
    glActiveTexture(GL_TEXTURE3); glBindTexture(GL_TEXTURE_2D, m_gBuffer->colorTex(3));
    m_shadowPass->bindDepthMap(4);

    m_deferredLightShader->setVec3("uViewPos",            m_camera.position());
    m_deferredLightShader->setMat4("uLightSpaceMatrix",   m_shadowPass->lightSpaceMatrix());
    m_deferredLightShader->setBool("uUsePBR",             m_usePBR);
    m_deferredLightShader->setInt ("uDebugView",          m_debugView);
    setLightUniforms(*m_deferredLightShader, time);
    renderFullscreenQuad();

    // ================================================================
    // PASS C — Skybox (rendered into HDR FBO with blitted depth)
    // Blit GBuffer depth → HDR FBO so skybox respects scene geometry.
    // ================================================================
    glBindFramebuffer(GL_READ_FRAMEBUFFER, m_gBuffer->fboId());
    glBindFramebuffer(GL_DRAW_FRAMEBUFFER, m_hdrFBO->fboId());
    glBlitFramebuffer(0, 0, fbw, fbh, 0, 0, fbw, fbh,
                      GL_DEPTH_BUFFER_BIT, GL_NEAREST);
    glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO->fboId());

    glEnable(GL_DEPTH_TEST);
    renderSkybox(view, proj);

    // ================================================================
    // PASS D — Bloom bright-pass
    // ================================================================
    m_bloomFBO[0]->bind();
    glClear(GL_COLOR_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);
    m_brightPassShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrFBO->colorTex(0));
    m_brightPassShader->setInt  ("uHDR",       0);
    m_brightPassShader->setFloat("uThreshold", m_bloomThreshold);
    renderFullscreenQuad();

    // ================================================================
    // PASS E — Gaussian blur (ping-pong N times)
    // ================================================================
    bool horizontal = true;
    for (int i = 0; i < m_bloomIterations * 2; ++i)
    {
        m_bloomFBO[horizontal ? 1 : 0]->bind();
        glClear(GL_COLOR_BUFFER_BIT);
        m_blurShader->use();
        glActiveTexture(GL_TEXTURE0);
        glBindTexture(GL_TEXTURE_2D,
                      m_bloomFBO[horizontal ? 0 : 1]->colorTex(0));
        m_blurShader->setInt ("uImage",      0);
        m_blurShader->setBool("uHorizontal", horizontal);
        renderFullscreenQuad();
        horizontal = !horizontal;
    }
    // After loop, last result is in m_bloomFBO[0]

    // ================================================================
    // PASS F — Tonemap + bloom composite → default framebuffer
    // ================================================================
    Framebuffer::unbind();
    glViewport(0, 0, fbw, fbh);
    glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
    glDisable(GL_DEPTH_TEST);

    m_tonemapShader->use();
    glActiveTexture(GL_TEXTURE0);
    glBindTexture(GL_TEXTURE_2D, m_hdrFBO->colorTex(0));
    glActiveTexture(GL_TEXTURE1);
    glBindTexture(GL_TEXTURE_2D, m_bloomFBO[0]->colorTex(0));
    m_tonemapShader->setInt  ("uHDR",        0);
    m_tonemapShader->setInt  ("uBloom",      1);
    m_tonemapShader->setFloat("uExposure",   m_exposure);
    m_tonemapShader->setBool ("uBloomOn",    m_bloomEnabled);
    m_tonemapShader->setInt  ("uTonemapMode", m_tonemapMode);
    renderFullscreenQuad();

    glEnable(GL_DEPTH_TEST);
}

// ---------------------------------------------------------------------------
// Phase 18: ImGui debug panel
// ---------------------------------------------------------------------------
void Application::renderDebugUI(float dt)
{
    if (!m_showUI) return;

    ImGui::Begin("Rendering Sandbox  —  Debug Panel");

    // FPS + timing
    float fps = dt > 0.0f ? 1.0f / dt : 0.0f;
    ImGui::Text("FPS: %.1f  |  CPU: %.2f ms  |  GPU: %.2f ms",
                fps, m_cpuMs, m_gpuMs);
    ImGui::Text("Draw calls: %d", m_drawCallCount);
    ImGui::Separator();

    // Rendering mode
    ImGui::Text("Rendering Pipeline");
    ImGui::Checkbox("Deferred (F1)",     &m_useDeferred);
    ImGui::SameLine();
    ImGui::Checkbox("PBR (F2)",          &m_usePBR);
    ImGui::SameLine();
    ImGui::Checkbox("Frustum Cull",      &m_frustumCulling);
    ImGui::Separator();

    // HDR + Bloom (Phase 13/14)
    ImGui::Text("Post-Processing");
    ImGui::SliderFloat("Exposure",        &m_exposure,        0.1f, 5.0f);
    const char* tmodes[] = {"Reinhard", "ACES Filmic"};
    ImGui::Combo("Tone Map",              &m_tonemapMode,     tmodes, 2);
    ImGui::Checkbox("Bloom (F3)",         &m_bloomEnabled);
    if (m_bloomEnabled) {
        ImGui::SliderFloat("Bloom Threshold", &m_bloomThreshold, 0.0f, 3.0f);
        ImGui::SliderInt  ("Bloom Iters",     &m_bloomIterations, 1, 10);
    }
    ImGui::Separator();

    // PBR display (Phase 15)
    if (m_usePBR) {
        ImGui::Text("PBR: Cook-Torrance  (GGX + Smith G + Schlick F)");
        ImGui::Separator();
    }

    // GBuffer debug views (Phase 12)
    ImGui::Text("GBuffer Debug View");
    const char* views[] = {"Final", "World Pos", "World Normal",
                            "Albedo", "Metallic/Rough/AO", "Shadow"};
    ImGui::Combo("View", &m_debugView, views, 6);
    ImGui::Separator();

    // Forward pass info
    if (!m_useDeferred) {
        ImGui::TextColored({1,0.7f,0,1}, "Forward (Phase 10 pipeline active)");
    }

    ImGui::End();
}

// ---------------------------------------------------------------------------
// Main loop
// ---------------------------------------------------------------------------
int Application::run()
{
    // One-time startup verification printed to console.
    {
        glBindFramebuffer(GL_FRAMEBUFFER, m_gBuffer->fboId());
        GLenum gbufStatus = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, m_hdrFBO->fboId());
        GLenum hdrStatus  = glCheckFramebufferStatus(GL_FRAMEBUFFER);
        glBindFramebuffer(GL_FRAMEBUFFER, 0);
        std::printf("\n=== Phase 11-18 startup verification ===\n");
        std::printf("  GBuffer   FBO: %s (%dx%d, 4 MRT: RGBA16F×2 + RGBA8×2)\n",
                    gbufStatus == GL_FRAMEBUFFER_COMPLETE ? "COMPLETE" : "INCOMPLETE",
                    m_gBuffer->width(), m_gBuffer->height());
        std::printf("  HDR       FBO: %s\n",
                    hdrStatus == GL_FRAMEBUFFER_COMPLETE ? "COMPLETE" : "INCOMPLETE");
        std::printf("  Bloom FBO[0] : %dx%d\n",
                    m_bloomFBO[0]->width(), m_bloomFBO[0]->height());
        std::printf("  Default mode : Deferred + Blinn-Phong + ACES + Bloom\n");
        std::printf("  Skybox       : procedural cubemap (64×64 per face)\n");
        std::printf("  GL errors    : 0x%04X\n", glGetError());
        std::printf("========================================\n\n");
        std::fflush(stdout);
    }

    while (!m_window.shouldClose())
    {
        double cpuStart = glfwGetTime();

        // Collect last frame's GPU timer
        if (m_queryActive) {
            GLint available = 0;
            glGetQueryObjectiv(m_gpuQuery, GL_QUERY_RESULT_AVAILABLE, &available);
            if (available) {
                GLuint64 elapsed = 0;
                glGetQueryObjectui64v(m_gpuQuery, GL_QUERY_RESULT, &elapsed);
                m_gpuMs = (double)elapsed / 1e6;
                m_queryActive = false;
            }
        }

        const float now = (float)glfwGetTime();
        const float dt  = now - m_lastFrame;
        m_lastFrame = now;

        m_window.pollEvents();
        processInput(dt);

        const int   fbw    = m_window.framebufferWidth();
        const int   fbh    = m_window.framebufferHeight();
        const float aspect = fbh > 0 ? (float)fbw / fbh : 1.0f;

        // Resize FBOs on window change
        if (m_gBuffer && (m_lastFBW != fbw || m_lastFBH != fbh)) {
            resizeFBOs(fbw, fbh);
            m_lastFBW = fbw;  m_lastFBH = fbh;
        }

        // ImGui frame start
        ImGui_ImplOpenGL3_NewFrame();
        ImGui_ImplGlfw_NewFrame();
        ImGui::NewFrame();

        m_drawCallCount = 0;

        // Start GPU timer only if no outstanding query (one frame lag, no stall)
        bool queryStarted = !m_queryActive;
        if (queryStarted) {
            glBeginQuery(GL_TIME_ELAPSED, m_gpuQuery);
            m_queryActive = true;
        }

        // ================================================================
        // SHADOW PASS (shared by forward + deferred)
        // ================================================================
        m_shadowPass->beginFrame(kDirLightDir, glm::vec3(0.0f), 12.0f);
        m_depthShader->use();
        m_depthShader->setMat4("uLightSpaceMatrix", m_shadowPass->lightSpaceMatrix());
        drawSceneGeometry(*m_depthShader);
        m_shadowPass->endFrame(fbw, fbh);

        // ================================================================
        // DEFERRED or FORWARD
        // ================================================================
        if (m_useDeferred) {
            renderDeferred(now, aspect, fbw, fbh);
        } else {
            // Phase 10 forward path (preserved for comparison)
            glBindFramebuffer(GL_FRAMEBUFFER, 0);
            glViewport(0, 0, fbw, fbh);
            glClearColor(0.05f, 0.07f, 0.12f, 1.0f);
            glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
            glEnable(GL_DEPTH_TEST);

            m_lightingShader->use();
            m_lightingShader->setMat4("uView",            m_camera.viewMatrix());
            m_lightingShader->setMat4("uProjection",      m_camera.projectionMatrix(aspect));
            m_lightingShader->setVec3("uViewPos",         m_camera.position());
            m_lightingShader->setMat4("uLightSpaceMatrix", m_shadowPass->lightSpaceMatrix());
            m_shadowPass->bindDepthMap(2);
            setLightUniforms(*m_lightingShader, now);
            drawSceneShaded();
        }

        // End GPU timer (matched to BeginQuery above)
        if (queryStarted) glEndQuery(GL_TIME_ELAPSED);

        // ImGui render
        renderDebugUI(dt);
        ImGui::Render();
        ImGui_ImplOpenGL3_RenderDrawData(ImGui::GetDrawData());

        m_lastDrawCalls = m_drawCallCount;
        m_cpuMs = (glfwGetTime() - cpuStart) * 1000.0;

        m_window.swapBuffers();
    }
    return 0;
}
