#include "BakeCubemapIrradiance.h"
#include "GlobalVars.h"
#include "FlyCamera.h"

#include <glad/glad.h>
#include <GLFW/glfw3.h>  

int BakeCubemapIrradiance(std::string assetFolder)
{
    JLEngine::JLEngineCore engine(SCREEN_WIDTH, SCREEN_HEIGHT, "JL Engine", 60, 120, assetFolder);
    engine.InitIMGUI();
    auto input = engine.GetInput();
    auto window = engine.GetGraphicsAPI()->GetWindow()->GetGLFWwindow();
    input->SetMouseCursor(GLFW_CURSOR_DISABLED);

    bool writeOutHdri = false;
    bool writeOutIrradiance = false;
    bool writeOutPrefilter = false;
    int irradianceMapSize = 32;
    int prefilteredMapSize = 128;
    int prefilteredSamples = 2048;
    int channels = 3;
    std::string hdriTexture = "metro_noord_4k";    
    std::string irradianceOutput = "irradiance_";
    std::string prefilteredOutput = "prefiltered_";

    std::array<std::string, 6> facingExt =
    {
        "posx", // +X
        "negx", // -X
        "posy", // +Y
        "negy", // -Y
        "posz", // +Z
        "negz"  // -Z
    };

    auto graphics = engine.GetGraphicsAPI();
    auto baker = new JLEngine::CubemapBaker(assetFolder, engine.GetResourceLoader());

    JLEngine::ImageData hdriImage;
    JLEngine::TextureReader::LoadTexture(assetFolder + "HDRI/" + hdriTexture + ".hdr", hdriImage, 0);
    channels = hdriImage.channels;
    int cubemapSize = hdriImage.width / 4;
    
    glEnable(GL_TEXTURE_CUBE_MAP_SEAMLESS);

    uint32_t cmId = baker->HDRtoCubemap(hdriImage, cubemapSize, true);

    if (writeOutHdri)
    {
        std::array<JLEngine::ImageData, 6> faceData;
        graphics->ReadCubemap(cmId, cubemapSize, cubemapSize, hdriImage.channels, true, faceData, false);

        //auto data = JLEngine::TextureReader::StitchSky(faceData, cubemapSize, cubemapSize, 3);
        //JLEngine::TextureWriter::WriteTexture(assetFolder + "HDRI/venice_sunset_4k/_stitched.hdr", data);

        for (size_t i = 0; i < faceData.size(); ++i)
        {
            std::string facePath = assetFolder + "HDRI/" + hdriTexture + "/" + facingExt[i] + ".hdr";
            JLEngine::TextureWriter::WriteTexture(facePath, faceData.at(i));
        }
    }

    uint32_t irradianceId = baker->GenerateIrradianceCubemap(cmId, irradianceMapSize);

    if (writeOutIrradiance)
    {
        std::array<JLEngine::ImageData, 6> faceData;
        graphics->ReadCubemap(irradianceId, irradianceMapSize, irradianceMapSize, hdriImage.channels, true, faceData, false);

        //auto data = JLEngine::TextureReader::StitchSky(faceData, irradianceMapSize, irradianceMapSize, 3);
        //JLEngine::TextureWriter::WriteTexture(assetFolder + "HDRI/venice_sunset_4k/irradiance_stitched.hdr", data);

        for (size_t i = 0; i < faceData.size(); ++i)
        {
            std::string facePath = assetFolder + "HDRI/" + hdriTexture + "/" + irradianceOutput + facingExt[i] + ".hdr";
            JLEngine::TextureWriter::WriteTexture(facePath, faceData.at(i));
        }
    }

    uint32_t prefilteredEnvMap = baker->GeneratePrefilteredEnvMap(cmId, prefilteredMapSize, prefilteredSamples);

    if (writeOutPrefilter)
    {
        //std::array<JLEngine::ImageData, 6> faceData;
        //graphics->ReadCubemap(prefilteredEnvMap, prefilteredMapSize, prefilteredMapSize, hdriImage.channels, true, faceData, false);
        //
        //auto data = JLEngine::TextureReader::StitchSky(faceData, prefilteredMapSize, prefilteredMapSize, 3);
        //JLEngine::TextureWriter::WriteTexture(assetFolder + "HDRI/" + hdriTexture + "/prefiltered_stitched.hdr", data);
    }

    JLEngine::VertexArrayObject vao;
    JLEngine::Geometry::CreateBox(vao);
    JLEngine::Graphics::CreateVertexArray(&vao);
    int vaoId = vao.GetGPUID();

    auto flyCamera = new JLEngine::FlyCamera(glm::vec3(0.0f, 1.0f, 5.0f), glm::vec3(0.0f, 1.0f, 0.0f), -90.0f, 0.0f);
    auto drawSkyboxShader = engine.GetResourceLoader()->CreateShaderFromFile(
        "SkyboxShader", "enviro_cubemap_vert.glsl", "enviro_cubemap_frag.glsl", assetFolder + "core/shaders/"
    );
    graphics->BindShader(drawSkyboxShader->GetProgramId());

    auto projMatrix = glm::perspective(glm::radians(70.0f), (float)SCREEN_WIDTH / (float)SCREEN_HEIGHT, 0.1f, 10.0f);
    auto viewMatrix = glm::lookAt(glm::vec3(0.0f, 0.0f, 10.0f), glm::vec3(0.0f), glm::vec3(0.0f, 1.0f, 0.0f));

    graphics->SetViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);

    bool showIbl = false;
    bool regenerate = false;

    auto renderFunc = [&prefilteredEnvMap, &hdriImage, &baker, &regenerate, &showIbl, &graphics, &vaoId, &cmId, &irradianceId, &drawSkyboxShader, &flyCamera, &projMatrix](JLEngine::GraphicsAPI& g, double interpolationFac)
        {
            drawSkyboxShader->SetUniform("u_Projection", projMatrix);
            drawSkyboxShader->SetUniform("u_View", glm::mat4(glm::mat3(flyCamera->GetViewMatrix())));
            graphics->SetActiveTexture(0);
            graphics->BindTexture(GL_TEXTURE_CUBE_MAP, showIbl ? prefilteredEnvMap : cmId);
            drawSkyboxShader->SetUniformi("u_Skybox", 0);

            graphics->BindVertexArray(vaoId);
            graphics->DrawArrayBuffer(GL_TRIANGLES, 0, 36);
            graphics->BindVertexArray(0);
        }; 

    auto fn = [&input, &flyCamera](double a, double b) 
    {
        double deltaX, deltaY;
        input->GetMouseDelta(deltaX, deltaY);

        flyCamera->ProcessMouseMovement(static_cast<float>(deltaX), static_cast<float>(deltaY));
    };

    auto logicUpdateFn = [&flyCamera, &window](double dt)
    {
        flyCamera->ProcessKeyboard(window, (float)dt);
    };

    auto keyboardCallback = [&regenerate, &showIbl, &window](int a, int b, int c, int d)
    {
        if (glfwGetKey(window, GLFW_KEY_F))
            showIbl = !showIbl;
    };

    input->SetMouseMoveCallback(fn);
    input->SetKeyboardCallback(keyboardCallback);

    /*auto cubemapDebugShader = engine.GetResourceLoader()->CreateShaderFromFile(
        "CubemapDebugShader",
        "pos_uv_vert.glsl",
        "/Debug/cubemap_debug_frag.glsl",
        assetFolder + "core/"
    );
    graphics->BindShader(cubemapDebugShader->GetProgramId());
    graphics->SetActiveTexture(0);
    graphics->BindTexture(GL_TEXTURE_CUBE_MAP, irradianceId);
    cubemapDebugShader->SetUniformi("u_CubeMap", 0);
    cubemapDebugShader->SetUniform("u_Direction", glm::vec3(0.0f, 1.0f, 0.0f));

    auto screenSpaceQuad = JLEngine::Geometry::CreateScreenSpaceQuad(graphics);
    auto& ibo = std::get<1>(screenSpaceQuad);
    auto vaoId = std::get<0>(screenSpaceQuad).GetVAO();

    glDisable(GL_DEPTH_TEST);
    glViewport(0, 0, SCREEN_WIDTH, SCREEN_HEIGHT);
    auto renderFunc = [&ibo, &vaoId, &cmId, &irradianceId, &screenSpaceQuad, &cubemapDebugShader](JLEngine::Graphics& g, double interpolationFac)
        {
            g.SetActiveTexture(0);
            g.BindTexture(GL_TEXTURE_CUBE_MAP, irradianceId);
            cubemapDebugShader->SetUniformi("u_CubeMap", 0);

            g.ClearColour(0.2f, 0.2f, 0.2f, 1.0f);
            g.Clear(GL_COLOR_BUFFER_BIT);

            g.BindVertexArray(vaoId);
            size_t size = ibo.Size();
            g.DrawElementBuffer(GL_TRIANGLES, (int32_t)size, GL_UNSIGNED_INT, nullptr);
            g.BindVertexArray(0);
        };*/

    engine.Run(logicUpdateFn, renderFunc, [](double dt) {});

    return 0;
}