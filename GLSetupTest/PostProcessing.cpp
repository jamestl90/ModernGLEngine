#include "PostProcessing.h"
#include "BloomEffect.h"
#include "ImageHelpers.h"
#include "ResourceLoader.h"
#include "ShaderProgram.h"
#include "IMGuiManager.h"

namespace JLEngine
{
	PostProcessing::PostProcessing(ResourceLoader* loader, const std::string& assetPath)
		: m_loader(loader), m_assetPath(assetPath)
	{
		m_bloom = new BloomEffect();		
	}

	PostProcessing::~PostProcessing()
	{
		delete m_bloom;
	}

	void PostProcessing::Initialise(int width, int height)
	{
		auto shaderPath = m_assetPath + "Core/Shaders/";
		m_combineShader = m_loader->CreateShaderFromFile("PostProcessCombine", 
			"screenspacetriangle.glsl", 
			"PostProcessing/postprocesscombine_frag.glsl",
			shaderPath).get();

		if (!m_combineShader) 
		{
			std::cerr << "PostProcessing Error: Failed to load BloomCombine shader." << std::endl;
		}

		m_bloom->Initialise(m_loader, m_assetPath, width, height);
	}

	void PostProcessing::Render(RenderTarget* lightOutput, RenderTarget* finalOutputTarget, int width, int height, glm::vec3& eyePos, glm::mat4& viewMat, glm::mat4& projMat)
	{
		RenderTarget* bloomResultTexture = nullptr;
		if (m_bloom->enabled)
		{
			bloomResultTexture = m_bloom->Render(lightOutput, m_iterations);
		}

		Graphics::API()->BindFrameBuffer(finalOutputTarget->GetGPUID());
		Graphics::API()->SetViewport(0, 0, width, height);
		Graphics::API()->Clear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

		Graphics::API()->BindShader(m_combineShader->GetProgramId());

		m_combineShader->SetUniformi("u_NeedsGammaCorrection",
			Graphics::API()->GetWindow()->SRGBCapable() ? 0 : 1);
		m_combineShader->SetUniformf("u_Exposure", tonemapExposure);
		m_combineShader->SetUniformf("u_BloomIntensity", m_bloom->intensity);

		GLuint textures[] =
		{
			lightOutput->GetTexId(0),
			m_bloom->enabled ? bloomResultTexture->GetTexId(0) : m_loader->DefaultBlackTexture()->GetGPUID()
		};
		Graphics::API()->BindTextures(0, 2, textures);

		ImageHelpers::RenderFullscreenTriangle();
	}

	void PostProcessing::OnResize(int newWidth, int newHeight)
	{
		m_bloom->OnResize(newWidth, newHeight);
	}

	void PostProcessing::DrawDebugUI()
	{
		ImGui::Begin("Post Processing");
		ImGui::Checkbox("Enable Bloom", &m_bloom->enabled);
		ImGui::SliderFloat("Intensity", &m_bloom->intensity, 0.001f, 1.0f);
		ImGui::SliderFloat("Threshold", &m_bloom->threshold, 0.0001f, 1.0f);
		ImGui::SliderFloat("Knee", &m_bloom->knee, 0.001f, 1.0f);
		ImGui::SliderFloat("Scatter", &m_bloom->scatter, 0.001f, 1.0f);
		ImGui::SliderInt("Iterations", &m_iterations, 1, 6);
		ImGui::End();
	}
}