#include "Im3dManager.h"
#include "FlyCamera.h"
#include "ResourceLoader.h"
#include "ShaderProgram.h"
#include "UniformBuffer.h"
#include "im3d/im3d_math.h"

#include <GLFW/glfw3.h>

void JLEngine::IM3DManager::Initialise(Window* window, ResourceLoader* resLoader, const std::string& assetFolder)
{
	m_resourceLoader = resLoader;
	m_window = window;
	SetupBuffers();
	LoadShader(assetFolder);
	Im3d::GetAppData().drawCallback = &IM3DManager::DrawCallback;
}

void JLEngine::IM3DManager::BeginFrame(Input* input, FlyCamera* flyCam, const glm::mat4& proj, float fov, float deltaTime)
{
	Im3d::GetAppData().m_appData = this;
	Im3d::AppData& appData = Im3d::GetAppData();

	auto camPos = flyCam->GetPosition();
	auto camFwd = flyCam->GetForward();

	appData.m_viewOrigin = Im3d::Vec3(camPos.x, camPos.y, camPos.z);
	appData.m_viewDirection = Im3d::Normalize(Im3d::Vec3(camFwd.x, camFwd.y, camFwd.z));
	appData.m_projOrtho = false;
	appData.m_projScaleY = tanf(fov * 0.5f);
	appData.m_viewportSize = Im3d::Vec2((float)m_window->GetWidth(), (float)m_window->GetHeight());
	appData.m_deltaTime = deltaTime;
	appData.m_worldUp = Im3d::Vec3(0.0f, 1.0f, 0.0f);

	glm::vec2 ndc = input->GetNDC();
	glm::vec3 rayDir = flyCam->ScreenToWorldRay(ndc, proj);
	appData.m_cursorRayOrigin = appData.m_viewOrigin;
	appData.m_cursorRayDirection = Im3d::Normalize(Im3d::Vec3(rayDir.x, rayDir.y, rayDir.z));

	std::fill(std::begin(appData.m_keyDown), std::end(appData.m_keyDown), false);
	appData.m_keyDown[Im3d::Mouse_Left] = input->IsMouseDown(GLFW_MOUSE_BUTTON_LEFT) == GLFW_PRESS;

	//glm::mat4 viewProj = proj * flyCam->GetViewMatrix();
	//appData.setCullFrustum(reinterpret_cast<const Im3d::Mat4&>(viewProj), true);

	Im3d::NewFrame();
}

void JLEngine::IM3DManager::EndFrameAndRender(const glm::mat4& mvp)
{
	m_mvp = mvp;
	Im3d::Draw();
}

void JLEngine::IM3DManager::DrawCallback(const Im3d::DrawList& drawList)
{
	if (drawList.m_vertexCount == 0)
		return;

	auto* self = static_cast<IM3DManager*>(Im3d::GetAppData().m_appData);
	if (!self || !self->m_shader)
		return;

	if (drawList.m_primType == Im3d::DrawPrimitiveType::DrawPrimitive_Points)
	{
		glEnable(GL_PROGRAM_POINT_SIZE);
	}
	else
	{
		glDisable(GL_PROGRAM_POINT_SIZE);
	}
	if (drawList.m_primType == Im3d::DrawPrimitiveType::DrawPrimitive_Lines)
	{
		// hack until I implement geometry shader for line drawing
		glLineWidth(2.0f); // or whatever default you want
	}
	GLenum glPrim = GL_POINTS; // default

	switch (drawList.m_primType)
	{
		case Im3d::DrawPrimitiveType::DrawPrimitive_Points:    glPrim = GL_POINTS;    break;
		case Im3d::DrawPrimitiveType::DrawPrimitive_Lines:     glPrim = GL_LINES;     break;
		case Im3d::DrawPrimitiveType::DrawPrimitive_Triangles: glPrim = GL_TRIANGLES; break;
		default: break;
	}

	glDisable(GL_DEPTH_TEST);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);
	glViewport(0, 0, (GLsizei)self->m_window->GetWidth(), (GLsizei)self->m_window->GetHeight());
	glUseProgram(self->m_shader->GetProgramId());

	self->m_shader->SetUniform("u_ViewProjMatrix", self->m_mvp);
	int shaderMode = 0; // Default = POINTS
	if (drawList.m_primType == Im3d::DrawPrimitiveType::DrawPrimitive_Lines)
		shaderMode = 1;
	else if (drawList.m_primType == Im3d::DrawPrimitiveType::DrawPrimitive_Triangles)
		shaderMode = 2;

	self->m_shader->SetUniformi("u_Mode", shaderMode);
	glBindVertexArray(self->m_vao);
	glNamedBufferSubData(self->m_vbo, 0, drawList.m_vertexCount * sizeof(Im3d::VertexData), drawList.m_vertexData);
	glDrawArrays(glPrim, 0, drawList.m_vertexCount);
}

void JLEngine::IM3DManager::SetupBuffers()
{
	glCreateVertexArrays(1, &m_vao);
	glCreateBuffers(1, &m_vbo);

	glNamedBufferStorage(m_vbo, 2048 * 2048, nullptr, GL_DYNAMIC_STORAGE_BIT);
	glVertexArrayVertexBuffer(m_vao, 0, m_vbo, 0, sizeof(Im3d::VertexData));

	// Position: vec3 from Vec4
	glEnableVertexArrayAttrib(m_vao, 0);
	glVertexArrayAttribFormat(m_vao, 0, 4, GL_FLOAT, GL_FALSE, (GLuint)offsetof(Im3d::VertexData, m_positionSize));
	glVertexArrayAttribBinding(m_vao, 0, 0);

	// Color: uvec4 from packed uint32
	glEnableVertexArrayAttrib(m_vao, 1);
	glVertexArrayAttribFormat(m_vao, 1, 4, GL_UNSIGNED_BYTE, GL_TRUE, (GLuint)offsetof(Im3d::VertexData, m_color));
	glVertexArrayAttribBinding(m_vao, 1, 0);
}

void JLEngine::IM3DManager::LoadShader(const std::string& assetFolder)
{
	std::string finalAssetPath = assetFolder + "Core/Shaders/Debug/";
	std::string vertSource = "debug_draw_vert.glsl";
	std::string fragSource = "debug_draw_frag.glsl";

	m_shader = m_resourceLoader->CreateShaderFromFile("Im3DShader", vertSource, fragSource, finalAssetPath).get();
}

void JLEngine::IM3DManager::Shutdown()
{
	glDeleteBuffers(1, &m_vbo);
	glDeleteVertexArrays(1, &m_vao);
	glDeleteProgram(m_shader->GetProgramId());
}

void JLEngine::IM3DManager::DrawBox(const glm::vec3& center, const glm::vec3& halfExtents, const Im3d::Color& color)
{
	Im3d::BeginLines();

	Im3d::Vec3 min = Im3d::Vec3(center.x - halfExtents.x, center.y - halfExtents.y, center.z - halfExtents.z);
	Im3d::Vec3 max = Im3d::Vec3(center.x + halfExtents.x, center.y + halfExtents.y, center.z + halfExtents.z);

	auto v = [](float x, float y, float z) { return Im3d::Vec3(x, y, z); };

	Im3d::Vertex(v(min.x, min.y, min.z)); Im3d::Vertex(v(max.x, min.y, min.z));
	Im3d::Vertex(v(max.x, min.y, min.z)); Im3d::Vertex(v(max.x, min.y, max.z));
	Im3d::Vertex(v(max.x, min.y, max.z)); Im3d::Vertex(v(min.x, min.y, max.z));
	Im3d::Vertex(v(min.x, min.y, max.z)); Im3d::Vertex(v(min.x, min.y, min.z));

	Im3d::Vertex(v(min.x, max.y, min.z)); Im3d::Vertex(v(max.x, max.y, min.z));
	Im3d::Vertex(v(max.x, max.y, min.z)); Im3d::Vertex(v(max.x, max.y, max.z));
	Im3d::Vertex(v(max.x, max.y, max.z)); Im3d::Vertex(v(min.x, max.y, max.z));
	Im3d::Vertex(v(min.x, max.y, max.z)); Im3d::Vertex(v(min.x, max.y, min.z));

	Im3d::Vertex(v(min.x, min.y, min.z)); Im3d::Vertex(v(min.x, max.y, min.z));
	Im3d::Vertex(v(max.x, min.y, min.z)); Im3d::Vertex(v(max.x, max.y, min.z));
	Im3d::Vertex(v(max.x, min.y, max.z)); Im3d::Vertex(v(max.x, max.y, max.z));
	Im3d::Vertex(v(min.x, min.y, max.z)); Im3d::Vertex(v(min.x, max.y, max.z));

	Im3d::End();
}