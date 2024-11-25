#include "Graphics.h"
#include "Mesh.h"
#include "TextureReader.h"
#include "FileHelpers.h"

#include <set>

namespace JLEngine
{
	Graphics::Graphics(Window* window) 
		: m_drawAABB(false), m_shaderInfo(""), m_versionInfo(""), m_vendorInfo(""), m_extensionInfo(""), m_rendererInfo(""), m_window(window), m_usingMSAA(false)
	{
		float fov = 40.0f;
		float nearDist = 0.1f;
		float farDist = 10000000.0f;

		m_viewFrustum = new ViewFrustum(m_window, fov, nearDist, farDist);

		m_clearColour = glm::vec4(1.0f);

		Initialise();

		// get GLSL version
		const GLubyte* str = glGetString(GL_SHADING_LANGUAGE_VERSION);
		m_shaderInfo = "GLSL Version: " + string((char*)str);

		// get GL version
		str = glGetString(GL_VERSION);
		m_versionInfo = "GL Version: " + string((char*)str);

		// get vendor (AMD etc)
		str = glGetString(GL_VENDOR);
		m_vendorInfo = "Vendor: " + string((char*)str);

		// get renderer (graphics card?)
		str = glGetString(GL_RENDERER);
		m_rendererInfo = "Renderer: " + string((char*)str);

		glGetIntegerv(GL_SAMPLE_BUFFERS, &m_MSAABuffers);
		glGetIntegerv(GL_SAMPLES, &m_MSAASamples);

		// get list of extensions
		int numOfExtensions;
		glGetIntegerv(GL_NUM_EXTENSIONS, &numOfExtensions);
		for (int i = 0; i < numOfExtensions; i++)
		{
			str = glGetStringi(GL_EXTENSIONS, i);
			m_extensionInfo += string((char*)str) + "\n";
		}
	}

	Graphics::~Graphics()
	{
		delete m_viewFrustum;
		glDeleteShader(m_defaultShader);

		glDeleteVertexArrays(1, &m_octahedronGeom[0]);
		glDeleteVertexArrays(1, &m_coneGeom[0]);
	}

	void Graphics::Initialise()
	{
		glViewport(0, 0, m_window->getWidth(), m_window->getHeight());
		glClearColor(m_clearColour.x, m_clearColour.y, m_clearColour.z, m_clearColour.w); 
		glEnable(GL_DEPTH_TEST);

		std::string defaultShaderV = 
			"#version 400\n" 
			"layout (location = 0) in vec3 in_Position;\n" 
			"uniform mat4 u_MVP;\n"
			//"out vec3 Colour;\n" 
			"void main()\n" 
			"{\n" 
			"gl_Position = u_MVP * vec4(in_Position, 1.0);\n" 
			//"Colour = in_Colour;\n"
			"}";
		std::string defaultShaderF = 
			"#version 400\n" 
			//"in vec3 Colour;\n" 
			"out vec4 FragColour;\n"
			"void main()\n"
			"{\n" 
			"FragColour = vec4(1.0, 0.0, 0.0, 1.0);\n" 
			"}";

		GLuint vertId = glCreateShader(GL_VERTEX_SHADER);
		GLuint fragId = glCreateShader(GL_FRAGMENT_SHADER);

		const char* cStrVert = defaultShaderV.c_str();
		const char* cStrFrag = defaultShaderF.c_str();
		glShaderSource(vertId, 1, &cStrVert, NULL);
		glCompileShader(vertId);
		glShaderSource(fragId, 1, &cStrFrag, NULL);
		glCompileShader(fragId);
		ShaderCompileErrorCheck(vertId, false);
		ShaderCompileErrorCheck(fragId, false);

		m_defaultShader = glCreateProgram();

		glAttachShader(m_defaultShader, vertId);
		glAttachShader(m_defaultShader, fragId);
		glLinkProgram(m_defaultShader);
		ShaderCompileErrorCheck(m_defaultShader, false);

		GeneratePrimitives();
	}

	void Graphics::ClearColour( float x, float y, float z, float w )
	{
		glClearColor(x, y, z, w);
		m_clearColour = glm::vec4(x, y, z, w);
	}

	glm::mat4& Graphics::CalculateMVP(glm::mat4& modelMat, glm::mat4& projection, glm::mat4& view)
	{
		m_mvp = projection * view * modelMat;
		return m_mvp;
	}

	void Graphics::SetMSAA( bool flag )
	{
		m_usingMSAA = flag;
		if (flag)
		{
			glEnable(GL_MULTISAMPLE); 
		}
		else
		{
			glDisable(GL_MULTISAMPLE);
		}
	}

	bool Graphics::UsingMSAA() const
	{
		return m_usingMSAA;
	}

	bool Graphics::ShaderCompileErrorCheck(uint32 id, bool compileCheck)
	{
		GLint compileStatus;

		glGetShaderiv(id, compileCheck ? GL_LINK_STATUS : GL_COMPILE_STATUS, &compileStatus);

		if (compileStatus == GL_FALSE)
		{
			int maxLength;

			glGetShaderiv(id, GL_INFO_LOG_LENGTH, &maxLength);

			GLchar* infoLog = new char[maxLength];

			glGetShaderInfoLog(id, maxLength, &maxLength, infoLog);

			std::cout << "ShaderInfoLog: " << infoLog << std::endl;

			delete[] infoLog;

			return false;
		}
		return true;
	}

	void Graphics::PrintActiveUniforms( uint32 programId )
	{
		GLint nUniforms, maxLen;
		glGetProgramiv(programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
		glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &nUniforms);
		GLchar* name = new GLchar[maxLen];
		GLint size, location;
		GLsizei written;
		GLenum type;
		std::cout << "Active uniforms" << std::endl;
		for (int i = 0; i < nUniforms; i++)
		{
			glGetActiveUniform(programId, i, maxLen, &written, &size, &type, name);
			location = glGetUniformLocation(programId, name);
			std::cout << location << " " << name << std::endl;
		}
	}

	void Graphics::CreateShader(ShaderProgram* program)
	{
		auto shaders = program->GetShaders();
		for (uint32 i = 0; i < shaders.size(); i++)
		{
			Shader& s = shaders.at(i);

			GLuint shaderId = glCreateShader(s.GetType());
			s.SetShaderId(shaderId);

			std::string shaderFile;
			if (!ReadTextFile(program->GetFilePath() + s.GetName(), shaderFile))
			{
				throw "Could not find file: " + program->GetFilePath() + s.GetName(), "Graphics";
				//throw JLStd::FileNotFound("Could not find file: " + program->GetFilePath() + s->GetName(), "Graphics");
			}

			const char* cStr = shaderFile.c_str();
			glShaderSource(shaderId, 1, &cStr, NULL);
			glCompileShader(shaderId);

			ShaderCompileErrorCheck(shaderId, false);
		}

		GLuint programID = glCreateProgram();
		program->SetProgramId(programID);

		for (uint32 i = 0; i < shaders.size(); i++)
		{
			glAttachShader(programID, shaders.at(i).GetShaderId());
		}

		glLinkProgram(programID);

		if (!ShaderCompileErrorCheck(programID, true))
		{
			DisposeShader(program);
		}
		else
		{
			// another loop to delete :( not sure if i can delete the shader before linking, probably not
			for (uint32 i = 0; i < shaders.size(); i++)
			{
				glDeleteShader(shaders.at(i).GetShaderId());
			}
		}		
	}

	//void Graphics::CreateShader(ShaderProgram* program)
	//{
	//	FileSystem* fs = FileSystem::getInstance();

	//	Shader* vertShader = program->getShader(VERTEX_SHADER);
	//	Shader* fragShader = program->getShader(FRAGMENT_SHADER);

	//	GLuint vertId = glCreateShader(GL_VERTEX_SHADER);
	//	GLuint fragId = glCreateShader(GL_FRAGMENT_SHADER);

	//	int vertFileId = fs->loadFile(program->getFilePath() + vertShader->getName(), false);
	//	int fragFileId = fs->loadFile(program->getFilePath() + fragShader->getName(), false);

	//	vertShader->setShaderId(vertId);
	//	fragShader->setShaderId(fragId);

	//	string vertFileString("");
	//	string fragFileString("");

	//	fs->getWholeFileString(vertFileId, vertFileString);
	//	fs->getWholeFileString(fragFileId, fragFileString);

	//	fs->closeFile(vertId);
	//	fs->closeFile(fragId);

	//	const char* vertFile = vertFileString.c_str();
	//	const char* fragFile = fragFileString.c_str();

	//	glShaderSource(vertId, 1, &vertFile, NULL);
	//	glCompileShader(vertFileId);
	//	if (!ShaderCompileErrorCheck(vertFileId, program->getFilePath() + vertShader->getName())) return;

	//	glShaderSource(fragId, 1, &fragFile, NULL);
	//	glCompileShader(fragFileId);
	//	if (!ShaderCompileErrorCheck(fragFileId, program->getFilePath() + fragShader->getName())) return;

	//	GLuint id = glCreateProgram();

	//	program->setProgramId(id);

	//	for (vector<Shader*>::iterator it = program->getShaders().begin(); it != program->getShaders().end(); it++)
	//	{
	//		glAttachShader(id, (*it)->getShaderId());
	//	}

	//	glLinkProgram(id);
	//	if (!ShaderCompileErrorCheck(id, program->getFileName())) DisposeShader(program);

	//	glDeleteShader(vertId);
	//	glDeleteShader(fragId);
	//}

	void Graphics::DisposeShader(ShaderProgram* program)
	{
		auto shaders = program->GetShaders();

		for (auto it = shaders.begin(); it != shaders.end(); it++)
		{
			glDetachShader(program->GetProgramId(), (*it).GetShaderId());
			glDeleteShader((*it).GetShaderId());
		}

		glDeleteProgram(program->GetProgramId());
	}

	void Graphics::SetUniform(uint32 programId, string name, uint32 x)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform1i(id, x);
	}

	void Graphics::SetUniform(uint32 programId, string name, uint32 x, uint32 y)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform2i(id, x, y);
	}

	void Graphics::SetUniform(uint32 programId, string name, uint32 x, uint32 y, uint32 z)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform3i(id, x, y, z);
	}

	void Graphics::SetUniform(uint32 programId, string name, uint32 x, uint32 y, uint32 z, uint32 w)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform4i(id, x, y, z, w);
	}

	void Graphics::SetUniform(uint32 programId, string name, float x)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform1f(id, x);
	}

	void Graphics::SetUniform(uint32 programId, string name, float x, float y)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform2f(id, x, y);
	}

	void Graphics::SetUniform(uint32 programId, string name, float x, float y, float z)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform3f(id, x, y, z);
	}

	void Graphics::SetUniform(uint32 programId, string name, float x, float y, float z, float w)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform4f(id, x, y, z, w);
	}

	void Graphics::SetUniform(uint32 programId, string name, glm::vec2& vec)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform2fv(id, 1, &vec[0]);
	}

	void Graphics::SetUniform(uint32 programId, string name, glm::vec3& vec)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform3fv(id, 1, &vec[0]);
	}

	void Graphics::SetUniform(uint32 programId, string name, glm::vec4& vec)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniform4fv(id, 1, &vec[0]);
	}

	void Graphics::SetUniform(uint32 programId, string name, int count, bool transpose, glm::mat2& mat)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniformMatrix2fv(id, count, transpose, &mat[0][0]);
	}

	void Graphics::SetUniform(uint32 programId, string name, int count, bool transpose, glm::mat3& mat)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniformMatrix3fv(id, count, transpose, &mat[0][0]);
	}

	void Graphics::SetUniform(uint32 programId, string name, int count, bool transpose, glm::mat4& mat)
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniformMatrix4fv(id, count, transpose, &mat[0][0]);
	}

	void Graphics::SetUniform( uint32 programId, string name, int count, bool transpose, const glm::mat4& mat )
	{
		GLuint id = glGetUniformLocation(programId, name.c_str());
		glUniformMatrix4fv(id, count, transpose, &mat[0][0]);
	}

	uint32 Graphics::CreateVertexArray()
	{
		GLuint id;
		glGenVertexArrays(1, &id);
		glBindVertexArray(id);
		return id;
	}

	void Graphics::BindFrameBuffer( uint32 id )
	{
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	void Graphics::CreateFrameBuffer(uint32 count, uint32& id )
	{
		glGenFramebuffers(count, &id);
	}

	void Graphics::CreateVertexArray(uint32 count, uint32& id)
	{
		glGenVertexArrays(count, &id);
	}

	void Graphics::BindVertexArray( uint32 vaoID )
	{
		glBindVertexArray(vaoID);
	}

	void Graphics::CreateBuffer(uint32 count, uint32& id)
	{
		glGenBuffers(count, &id);
	}

	void Graphics::SetBufferData( uint32 buffType, ptrdiff_t size, void* data, uint32 usage )
	{
		glBufferData(buffType, size, data, usage);
	}

	void Graphics::SetBufferSubData( uint32 target, ptrdiff_t offset, int32 size, void* data)
	{
		glBufferSubData(target, offset, size, data);
	}

	void Graphics::SetAttributePointer( uint32 index, int32 count, uint32 datatype, bool normalise, int32 size, void* offset )
	{
		glVertexAttribPointer(index, count, datatype, normalise ? GL_TRUE : GL_FALSE, size, offset);
	}

	void Graphics::BindBuffer( uint32 buffType, uint32 vboID )
	{
		glBindBuffer(buffType, vboID);
	}

	void* Graphics::MapBufferData( uint32 target, uint32 access )
	{
		return glMapBuffer(target, access);
	}

	bool Graphics::UnmapBufferData( uint32 target )
	{
		return glUnmapBuffer(target) ? true : false;
	}

	void Graphics::DrawArrayBuffer( uint32 drawMode, uint32 first, uint32 count )
	{
		glDrawArrays(drawMode, first, count);
	}

	void Graphics::DrawElementBuffer( uint32 drawMode, int32 count, uint32 dataType, void* offset )
	{
		glDrawElements(drawMode, count, dataType, offset);
	}

	void Graphics::EnableVertexAttribArray(uint32 pos)
	{
		glEnableVertexAttribArray(pos);
	}

	void Graphics::DisableVertexAttribArray(uint32 pos)
	{
		glDisableVertexAttribArray(pos);
	}

	void Graphics::BindShader(uint32 programId)
	{
		glUseProgram(programId);
	}

	void Graphics::UnbindShader()
	{
		glUseProgram(0);
	}

	void Graphics::CreateTextures( uint32 count, uint32& id )
	{
		glGenTextures(count, &id);
	}

	void Graphics::BindTexture( uint32 type, uint32 id )
	{
		glBindTexture(type, id);
	}

	void Graphics::SetActiveTexture( uint32 type )
	{
		glActiveTexture(type);
	}

	void Graphics::CreateVertexBuffer( VertexBuffer& vbo )
	{
		uint32 id;
		glGenBuffers(1, &id);
		glBindBuffer(vbo.Type(), id);
		glBufferData(vbo.Type(), vbo.SizeInBytes(), vbo.Array(), vbo.DrawType());
		vbo.SetId(id);

		std::set<VertexAttribute> attribs = vbo.GetAttributes();

		uint32 i = 0;

		for (auto it = vbo.GetAttributes().begin(); it != vbo.GetAttributes().end(); ++it)
		{
			VertexAttribute attrib = *it;

			glEnableVertexAttribArray(attrib.m_type);
			glVertexAttribPointer(attrib.m_type, attrib.m_count, vbo.DataType(), GL_FALSE, vbo.GetStride() * size_f, BUFFER_OFFSET(attrib.m_offset));
			i++;
		}
	}

	void Graphics::CreateIndexBuffer( IndexBuffer& ibo )
	{
		uint32 id;
		glGenBuffers(1, &id);
		glBindBuffer(ibo.Type(), id);
		glBufferData(ibo.Type(), ibo.Size() * sizeof(uint32), ibo.Array(), ibo.DrawType());
		ibo.SetId(id);
	}

	void Graphics::CreateMesh( Mesh* mesh )
	{
		mesh->SetVao(CreateVertexArray());
		CreateVertexBuffer(mesh->GetVertexBuffer());
		if (mesh->HasIndices())
		{
			CreateIndexBuffer(mesh->GetIndexBuffer());
		}
		glBindVertexArray(0);
	}

	void Graphics::DisposeMesh( Mesh* mesh )
	{
		GLuint vaoID = mesh->GetVaoId();
		GLuint vboID = mesh->GetVertexBuffer().GetId();

		glDeleteBuffers(1, &vboID);
		if (mesh->HasIndices())
		{
			GLuint iboID = mesh->GetIndexBuffer().GetId();
			glDeleteBuffers(1, &iboID);
		}
		glDeleteBuffers(1, &vaoID);
	}

	void Graphics::CreateMaterial( Material* mat )
	{

	}

	void Graphics::DisposeMaterial( Material* mat )
	{

	}

	void Graphics::CreateTexture( Texture* texture )
	{
		GLuint image;
		glGenTextures(1, &image); 
		glBindTexture(GL_TEXTURE_2D, image);
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
		if (texture->IsClamped())
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}
		glTexImage2D(GL_TEXTURE_2D, 0, texture->GetInternalFormat(), texture->GetWidth(), texture->GetHeight(), 0, texture->GetFormat(), texture->GetDataType(), texture->GetData().data());
		texture->SetGPUID(image);
	}

	void Graphics::DisposeTexture(Texture* texture)
	{
		GLuint id = texture->GetGPUID();
	
		glDeleteTextures(1, &id);
	}

	void Graphics::DisposeTexture( uint32 count, uint32* textures )
	{
		glDeleteTextures(count, textures);
	}

	//void Graphics::UpdateViewport()
	//{
	//	// did the user resize the IWindow?
	//	if (m_window->NeedResize())
	//	{
	//		glViewport(0, 0, m_window->GetWidth(), m_window->GetHeight());	// set the new view port
	//
	//		m_window->SetResized(false);	// the IWindow doesn't need a resize anymore
	//	}
	//}

	void Graphics::Clear(uint32 flags)
	{
		glClear(flags);
	}

	void Graphics::SwapBuffers()
	{
		m_window->swapBuffers();
	}

	void Graphics::RenderMesh(Mesh* mesh)
	{
		if (mesh == nullptr) return;

		if (m_drawAABB)
		{
			AABB meshAABB = mesh->GetAABB();

			DrawAABB(meshAABB);
		}

		GLuint vaoId = mesh->GetVaoId();

		glBindVertexArray(vaoId);

		if (mesh->HasIndices())
		{
			glDrawElements(GL_TRIANGLES, (GLsizei)mesh->GetIndexBuffer().Size(), GL_UNSIGNED_INT, BUFFER_OFFSET(0));
		}
		else
		{
			glDrawArrays(GL_TRIANGLES, 0, (GLsizei)mesh->GetVertexBuffer().Size() / mesh->GetVertexBuffer().GetStride());
		}

		glBindVertexArray(0);
	}

	// immediate mode :(
	void Graphics::DrawAABB( AABB& aabb )
	{
		glm::vec3 max = aabb.max;
		glm::vec3 min = aabb.min;

		glLineWidth(3.0f);

		glBegin(GL_LINES);
		glVertex3f(min.x, min.y, min.z);
		glVertex3f(min.x, min.y, max.z);

		glVertex3f(max.x, min.y, min.z);
		glVertex3f(max.x, min.y, max.z);

		glVertex3f(min.x, max.y, min.z);
		glVertex3f(min.x, max.y, max.z);

		glVertex3f(max.x, max.y, min.z);
		glVertex3f(max.x, max.y, max.z);

		glVertex3f(min.x, min.y, min.z);
		glVertex3f(max.x, min.y, min.z);

		glVertex3f(min.x, min.y, max.z);
		glVertex3f(max.x, min.y, max.z);

		glVertex3f(min.x, max.y, min.z);
		glVertex3f(max.x, max.y, min.z);

		glVertex3f(min.x, max.y, max.z);
		glVertex3f(max.x, max.y, max.z);

		glVertex3f(min.x, min.y, min.z);
		glVertex3f(min.x, max.y, min.z);

		glVertex3f(min.x, min.y, max.z);
		glVertex3f(min.x, max.y, max.z);

		glVertex3f(max.x, min.y, min.z);
		glVertex3f(max.x, max.y, min.z);

		glVertex3f(max.x, min.y, max.z);
		glVertex3f(max.x, max.y, max.z);
		glEnd();

		glLineWidth(1.0f);
	}

	void Graphics::CreateRenderBuffer( uint32 count, uint32& id )
	{
		glGenRenderbuffers(count, &id);
	}

	void Graphics::BindRenderBuffer(uint32 id )
	{
		glBindRenderbuffer(GL_RENDERBUFFER, id);
	}

	void Graphics::BindFrameBufferToTexture( uint32 type, uint32 attachment, uint32 target, uint32 id, int32 level )
	{
		glFramebufferTexture2D(type, attachment, target, id, level);
	}

	void Graphics::BindFrameBufferToRenderbuffer( uint32 type, uint32 attachment, uint32 target, uint32 id)
	{
		glFramebufferRenderbuffer(type, attachment, target, id);
	}

	void Graphics::RenderBufferStorage( uint32 type, uint32 internalFormat, uint32 width, uint32 height )
	{
		glRenderbufferStorage(type, internalFormat, width, height);
	}

	void Graphics::DrawBuffers( uint32 count, uint32* targets)
	{
		glDrawBuffers(count, targets);
	}

	void Graphics::DisposeBuffer( uint32 count, uint32* id )
	{
		glDeleteBuffers(count, id);
	}

	uint32 Graphics::CheckFrameBuffer()
	{
		return glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}

	void Graphics::SetViewport( uint32 x, uint32 y, uint32 width, uint32 height )
	{
		glViewport(x, y, width, height);
	}

	void Graphics::DisposeVertexBuffer( VertexBuffer& vbo )
	{
		GLuint id = vbo.GetId();
		glDeleteBuffers(1, &id);
	}

	void Graphics::DisposeIndexBuffer( IndexBuffer& ibo )
	{
		GLuint id = ibo.GetId();
		glDeleteBuffers(1, &id);
	}

	bool Graphics::CreateRenderTarget( RenderTarget* target )
	{
		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		target->SetFrameBufferId(fbo);

		for (uint32 i = 0; i < target->GetNumSources(); i++)
		{
			GLuint tex;
			glActiveTexture(GL_TEXTURE0 + i);
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_RGB32F, target->GetWidth(), target->GetHeight(), 0, GL_RGB, GL_UNSIGNED_BYTE, 0);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, tex, 0);
			target->SetSourceId(i, tex);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR); 
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		}

		if (target->UseDepth())
		{
			GLuint depth;
			glGenRenderbuffers(1, &depth);
			glBindRenderbuffer(GL_RENDERBUFFER, depth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, target->GetWidth(), target->GetHeight());
			target->SetDepthBufferId(depth);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
		}

		glDrawBuffers(target->GetNumSources(), target->GetDrawBuffers());

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			std::cout << "Graphics: Frame buffer has errors" << std::endl;
			return false;
		}
		return true;
	}

	void Graphics::DisposeRenderTarget( RenderTarget* target )
	{
		GLuint fboId = target->GetFrameBufferId();
		
		glDeleteTextures(target->GetNumSources(), target->GetSources());
		glDeleteFramebuffers(1, &fboId);

		if (target->UseDepth())
		{
			GLuint dboId = target->GetDepthBufferId();
			glDeleteRenderbuffers(1, &dboId);
		}
	}

	void Graphics::DisposeFrameBuffer( uint32 count, uint32* fbo )
	{
		glDeleteFramebuffers(count, fbo);
	}

	void Graphics::GeneratePrimitives()
	{
		float coneVerts[13 * 3] = {  // x, y, z
			0.f, 0.f, 0.f,
			0.f, 1.f, -1.f,   -0.5f, 0.866f, -1.f,   -0.866f, 0.5f, -1.f,
			-1.f, 0.f, -1.f,   -0.866f, -0.5f, -1.f,   -0.5f, -0.866f, -1.f,
			0.f, -1.f, -1.f,   0.5f, -0.866f, -1.f,   0.866f, -0.5f, -1.f,
			1.f, 0.f, -1.f,   0.866f, 0.5f, -1.f,   0.5f, 0.866f, -1.f,
		};
		uint32 coneInds[22 * 3] = {
			0, 1, 2,   0, 2, 3,   0, 3, 4,   0, 4, 5,   0, 5, 6,   0, 6, 7,
			0, 7, 8,   0, 8, 9,   0, 9, 10,   0, 10, 11,   0, 11, 12,   0, 12, 1,
			10, 6, 2,   10, 8, 6,   10, 9, 8,   8, 7, 6,   6, 4, 2,   6, 5, 4,   4, 3, 2,
			2, 12, 10,   2, 1, 12,   12, 11, 10
		};
	
		float octahedronVerts[18] = 
		{
			0.2843f, 10.1878f, -0.3454f,
			0.2843f, -0.0000f, -10.5332f,
			-9.9035f, -0.0000f, -0.3454f,
			0.2843f, -0.0000f, 9.8424f,
			10.4721f, -0.0000f, -0.3454f,
			0.2843f, -10.1878f, -0.3454f
		};

		uint32 octahedronInds[24] = 
		{
			0, 1, 2, 
			0, 2, 3,
			0, 3, 4, 
			0, 4, 1, 
			5, 2, 1, 
			5, 3, 2, 
			5, 4, 3, 
			5, 1, 4
		};

		CreatePrimitiveBuffers(octahedronVerts, sizeof(octahedronVerts), octahedronInds, sizeof(octahedronInds), m_octahedronGeom);
		CreatePrimitiveBuffers(coneVerts, sizeof(coneVerts), coneInds, sizeof(coneInds), m_coneGeom);
	}

	void Graphics::CreatePrimitiveBuffers( float vertices[], uint32 vertSize, uint32 indices[], uint32 indSize, uint32 ids[] )
	{
		glGenVertexArrays(1, &ids[0]);
		glBindVertexArray(ids[0]);
		glGenBuffers(1, &ids[1]);
		glBindBuffer(GL_ARRAY_BUFFER, ids[1]);
		glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, false, 12, BUFFER_OFFSET(0));
		glGenBuffers(1, &ids[2]);
		glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, ids[2]);
		glBufferData(GL_ELEMENT_ARRAY_BUFFER, indSize, indices, GL_STATIC_DRAW);
		ids[3] = indSize / sizeof(uint32);
		glBindVertexArray(0);
	}

	void Graphics::SetBlendFunc( uint32 first, uint32 second )
	{
		glBlendFunc(first, second);
	}

	void Graphics::Enable( uint32 val )
	{
		glEnable(val);
	}

	void Graphics::Disable( uint32 val )
	{
		glDisable(val);
	}

	void Graphics::SetDepthFunc( uint32 val )
	{
		glDepthFunc(val);
	}

	void Graphics::SetDepthMask( uint32 val )
	{
		glDepthMask(val);
	}

	void Graphics::BeginPrimitiveDraw()
	{
		glUseProgram(m_defaultShader);
	}

	void Graphics::RenderPrimitive(glm::mat4& mvp, uint32 type, uint32 shaderId )
	{
		if (shaderId == -1)
		{
			SetUniform(m_defaultShader, "u_MVP", 1, false, mvp);
		}
		else
		{
			SetUniform(shaderId, "u_MVP", 1, false, mvp);
		}

		switch (type)
		{
		case PrimitiveType::Cone: 
				glBindVertexArray(m_coneGeom[0]);
				glDrawElements(GL_TRIANGLES, m_coneGeom[3], GL_UNSIGNED_INT, BUFFER_OFFSET(0));
				glBindVertexArray(0);
			break;
		case PrimitiveType::Octahedron: 
				glBindVertexArray(m_octahedronGeom[0]);
				glDrawElements(GL_TRIANGLES, m_octahedronGeom[3], GL_UNSIGNED_INT, BUFFER_OFFSET(0));
				glBindVertexArray(0);
			break;
		case PrimitiveType::Sphere: break;
		default: break;
		}
	}

	void Graphics::EndPrimitiveDraw()
	{
		glUseProgram(0);
	}

	void Graphics::SetCullFace( uint32 face )
	{
		glCullFace(face);
	}

}

//void Graphics::loadMeshAdv(Mesh* mesh)
//{
//	BufferObjectAdv vbo = mesh->getBufferObjectAdv();

//	GLuint vaoID;
//	GLuint vboID;
//	GLuint iboID;

//	glGenVertexArrays(1, &vaoID);
//	glBindVertexArray(vaoID);

//	mesh->setVao(vaoID);

//	glGenBuffers(1, &vboID);
//	glBindBuffer(GL_ARRAY_BUFFER, vboID);
//	glBufferData(GL_ARRAY_BUFFER, vbo.getVertexArray().size() * sizeof(VertexAdv), &vbo.getVertexArray()[0], GL_STATIC_DRAW);

//	// position
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAdv), BUFFER_OFFSET(0));

//	// normal
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAdv), BUFFER_OFFSET(12));

//	// tex-coords
//	glEnableVertexAttribArray(2);
//	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAdv), BUFFER_OFFSET(24));

//	// tangent
//	glEnableVertexAttribArray(3);
//	glVertexAttribPointer(3, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAdv), BUFFER_OFFSET(36));

//	// bitangent
//	glEnableVertexAttribArray(4);
//	glVertexAttribPointer(4, 3, GL_FLOAT, GL_FALSE, sizeof(VertexAdv), BUFFER_OFFSET(48));

//	vbo.setVbo(vboID);

//	glGenBuffers(1, &iboID);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vbo.getIndexArray().size() * sizeof(uint32), &vbo.getIndexArray()[0], GL_STATIC_DRAW);

//	vbo.setIbo(iboID);
//}

//void Graphics::loadMeshBasic(Mesh* mesh)
//{
//	BufferObjectBasic vbo = mesh->getBufferObjectBasic();

//	GLuint vaoID;
//	GLuint vboID;
//	GLuint iboID;

//	glGenVertexArrays(1, &vaoID);
//	glBindVertexArray(vaoID);

//	mesh->setVao(vaoID);

//	glGenBuffers(1, &vboID);
//	glBindBuffer(GL_ARRAY_BUFFER, vboID);
//	glBufferData(GL_ARRAY_BUFFER, vbo.getVertexArray().size() * sizeof(VertexBasic), &vbo.getVertexArray()[0], GL_STATIC_DRAW);

//	// position
//	glEnableVertexAttribArray(0);
//	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(VertexBasic), BUFFER_OFFSET(0));

//	// normal
//	glEnableVertexAttribArray(1);
//	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(VertexBasic), BUFFER_OFFSET(12));

//	// tex-coords
//	glEnableVertexAttribArray(2);
//	glVertexAttribPointer(2, 3, GL_FLOAT, GL_FALSE, sizeof(VertexBasic), BUFFER_OFFSET(24));

//	vbo.setVbo(vboID);

//	glGenBuffers(1, &iboID);
//	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, iboID);
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vbo.getIndexArray().size() * sizeof(uint32), &vbo.getIndexArray()[0], GL_STATIC_DRAW);

//	vbo.setIbo(iboID);
//}

//void Graphics::unloadMesh( Mesh* mesh )
//{
//	GLuint vao = mesh->getVaoId();
//	GLuint vbo;
//	GLuint ibo;

//	if (mesh->usesAdvancedVertexStructure())
//	{
//		vbo = mesh->getBufferObjectAdv().getVboId();
//		ibo = mesh->getBufferObjectAdv().getIboId();
//	}
//	else
//	{
//		vbo = mesh->getBufferObjectBasic().getVboId();
//		ibo = mesh->getBufferObjectBasic().getIboId();
//	}

//	glDeleteBuffers(1, &vbo);
//	glDeleteBuffers(1, &ibo);
//	glDeleteBuffers(1, &vao);
//}

//void Graphics::unloadModel(Model* model)
//{
//	for (int i = 0; i < (signed)model->getMeshes().size(); i++)
//	{
//		Mesh* mesh = model->getMeshAt(i);

//		GLuint vao = mesh->getVaoId();
//		GLuint vbo;
//		GLuint ibo;

//		if (mesh->usesAdvancedVertexStructure())
//		{
//			vbo = mesh->getBufferObjectAdv().getVboId();
//			ibo = mesh->getBufferObjectAdv().getIboId();
//		}
//		else
//		{
//			vbo = mesh->getBufferObjectBasic().getVboId();
//			ibo = mesh->getBufferObjectBasic().getIboId();
//		}

//		glDeleteBuffers(1, &vbo);
//		glDeleteBuffers(1, &ibo);
//		glDeleteBuffers(1, &vao);
//	}
//}
