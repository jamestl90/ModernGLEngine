#include "Graphics.h"
#include "Mesh.h"
#include "TextureReader.h"
#include "FileHelpers.h"
#include "RenderTarget.h"
#include "InstanceBuffer.h"
#include "Batch.h"

#include <set>
#include <array>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>

namespace JLEngine
{
	Graphics::Graphics(Window* window) 
		: m_drawAABB(false), m_shaderInfo(""), m_versionInfo(""), m_activeTextureUnit(0), m_activeShaderProgram(0),
		m_vendorInfo(""), m_extensionInfo(""), m_rendererInfo(""), m_window(window), m_usingMSAA(false)
	{
		float fov = 45.0f;
		float nearDist = 0.1f;
		float farDist = 200.0f;

		m_viewFrustum = new ViewFrustum(fov, (float)window->GetWidth() / (float)window->GetHeight(), nearDist, farDist);

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

		GLint maxUnits;
		glGetIntegerv(GL_MAX_COMBINED_TEXTURE_IMAGE_UNITS, &maxUnits);
		m_boundTextures = std::vector<uint32_t>(maxUnits);
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
		glViewport(0, 0, m_window->GetWidth(), m_window->GetHeight());
		glClearColor(m_clearColour.x, m_clearColour.y, m_clearColour.z, m_clearColour.w); 
		glEnable(GL_DEPTH_TEST);
		glDepthFunc(GL_LESS);

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
		ShaderCompileErrorCheck(vertId);
		ShaderCompileErrorCheck(fragId);

		m_defaultShader = glCreateProgram();

		glAttachShader(m_defaultShader, vertId);
		glAttachShader(m_defaultShader, fragId);
		glLinkProgram(m_defaultShader);
		ShaderProgramLinkErrorCheck(m_defaultShader);

		GeneratePrimitives();
	}

	void Graphics::ClearColour( float x, float y, float z, float w )
	{
		glClearColor(x, y, z, w);
		m_clearColour = glm::vec4(x, y, z, w);
	}

	void Graphics::ClearColour()
	{
		glClearColor(m_clearColour.x, m_clearColour.y, m_clearColour.z, m_clearColour.w);
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

	bool Graphics::ShaderCompileErrorCheck(uint32_t id)
	{
		GLint compileStatus;
		glGetShaderiv(id, GL_COMPILE_STATUS, &compileStatus);

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

	bool Graphics::ShaderProgramLinkErrorCheck(uint32_t programId)
	{
		GLint linkStatus;
		glGetProgramiv(programId, GL_LINK_STATUS, &linkStatus);

		if (linkStatus == GL_FALSE)
		{
			int maxLength;
			glGetProgramiv(programId, GL_INFO_LOG_LENGTH, &maxLength);
			GLchar* infoLog = new char[maxLength];
			glGetShaderInfoLog(programId, maxLength, &maxLength, infoLog);
			std::cout << "ShaderInfoLog: " << infoLog << std::endl;
			delete[] infoLog;
			return false;
		}
		return true;
	}

	std::vector<std::tuple<std::string, int>> Graphics::GetActiveUniforms(uint32_t programId)
	{
		std::vector<std::tuple<std::string, int>> activeUniforms;
		GLint nUniforms, maxLen;
		glGetProgramiv(programId, GL_ACTIVE_UNIFORM_MAX_LENGTH, &maxLen);
		glGetProgramiv(programId, GL_ACTIVE_UNIFORMS, &nUniforms);
		GLchar* name = new GLchar[maxLen];
		GLint size, location;
		GLsizei written;
		GLenum type;
		for (int i = 0; i < nUniforms; i++)
		{
			glGetActiveUniform(programId, i, maxLen, &written, &size, &type, name);
			location = glGetUniformLocation(programId, name);
			activeUniforms.push_back({ name, location });
		}
		return activeUniforms;
	}

	void Graphics::PrintActiveUniforms( uint32_t programId )
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

	void Graphics::CreateShaderFromFile(ShaderProgram* program)
	{
		auto shaders = program->GetShaders();
		for (uint32_t i = 0; i < shaders.size(); i++)
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

			ShaderCompileErrorCheck(shaderId);
		}

		GLuint programID = glCreateProgram();
		program->SetProgramId(programID);

		for (uint32_t i = 0; i < shaders.size(); i++)
		{
			glAttachShader(programID, shaders.at(i).GetShaderId());
		}

		glLinkProgram(programID);

		if (!ShaderProgramLinkErrorCheck(programID))
		{
			DisposeShader(program);
		}
		else
		{
			// another loop to delete :( not sure if i can delete the shader before linking, probably not
			for (uint32_t i = 0; i < shaders.size(); i++)
			{
				glDeleteShader(shaders.at(i).GetShaderId());
			}
		}		
	}

	void Graphics::CreateShaderFromText(ShaderProgram* program, std::vector<std::string> shaderTexts)
	{
		auto shaders = program->GetShaders();

		if (shaderTexts.size() < shaders.size())
		{
			std::cout << "Not enough shader text provided" << std::endl;
			return;
		}

		for (uint32_t i = 0; i < shaders.size(); i++)
		{
			Shader& s = shaders.at(i);

			GLuint shaderId = glCreateShader(s.GetType());
			s.SetShaderId(shaderId);

			auto& shaderFile = shaderTexts[i];

			const char* cStr = shaderFile.c_str();
			glShaderSource(shaderId, 1, &cStr, NULL);
			glCompileShader(shaderId);

			ShaderCompileErrorCheck(shaderId);
		}

		GLuint programID = glCreateProgram();
		program->SetProgramId(programID);

		for (uint32_t i = 0; i < shaders.size(); i++)
		{
			glAttachShader(programID, shaders.at(i).GetShaderId());
		}

		glLinkProgram(programID);

		if (!ShaderProgramLinkErrorCheck(programID))
		{
			DisposeShader(program);
		}
		else
		{
			// another loop to delete :( not sure if i can delete the shader before linking, probably not
			for (uint32_t i = 0; i < shaders.size(); i++)
			{
				glDeleteShader(shaders.at(i).GetShaderId());
			}
		}
	}

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

	void Graphics::SetUniform(uint32_t location, uint32_t x)
	{
		glUniform1i(location, x);
	}

	void Graphics::SetUniform(uint32_t location, uint32_t x, uint32_t y)
	{
		glUniform2i(location, x, y);
	}

	void Graphics::SetUniform(uint32_t location, uint32_t x, uint32_t y, uint32_t z)
	{
		glUniform3i(location, x, y, z);
	}

	void Graphics::SetUniform(uint32_t location, uint32_t x, uint32_t y, uint32_t z, uint32_t w)
	{
		glUniform4i(location, x, y, z, w);
	}

	void Graphics::SetUniform(uint32_t location, float x)
	{
		glUniform1f(location, x);
	}

	void Graphics::SetUniform(uint32_t location, float x, float y)
	{
		glUniform2f(location, x, y);
	}

	void Graphics::SetUniform(uint32_t location, float x, float y, float z)
	{
		glUniform3f(location, x, y, z);
	}

	void Graphics::SetUniform(uint32_t location, float x, float y, float z, float w)
	{
		glUniform4f(location, x, y, z, w);
	}

	void Graphics::SetUniform(uint32_t location, const glm::vec2& vec)
	{
		glUniform2fv(location, 1, &vec[0]);
	}

	void Graphics::SetUniform(uint32_t location, const glm::vec3& vec)
	{
		glUniform3fv(location, 1, &vec[0]);
	}

	void Graphics::SetUniform(uint32_t location, const glm::vec4& vec)
	{
		glUniform4fv(location, 1, &vec[0]);
	}

	void Graphics::SetUniform(uint32_t location, int count, bool transpose, const glm::mat2& mat)
	{
		glUniformMatrix2fv(location, count, transpose ? GL_TRUE : GL_FALSE, &mat[0][0]);
	}

	void Graphics::SetUniform(uint32_t location, int count, bool transpose, const glm::mat3& mat)
	{
		glUniformMatrix3fv(location, count, transpose ? GL_TRUE : GL_FALSE, &mat[0][0]);
	}

	void Graphics::SetUniform(uint32_t location, int count, bool transpose, const glm::mat4& mat)
	{
		glUniformMatrix4fv(location, count, transpose ? GL_TRUE : GL_FALSE, glm::value_ptr(mat));
	}

	uint32_t Graphics::CreateVertexArray()
	{
		GLuint id;
		glGenVertexArrays(1, &id);
		glBindVertexArray(id);
		return id;
	}

	void Graphics::BindFrameBuffer( uint32_t id )
	{
		glBindFramebuffer(GL_FRAMEBUFFER, id);
	}

	void Graphics::CreateFrameBuffer(uint32_t count, uint32_t& id )
	{
		glGenFramebuffers(count, &id);
	}

	void Graphics::BindVertexArray( uint32_t vaoID )
	{
		glBindVertexArray(vaoID);
	}

	void Graphics::CreateBuffer(uint32_t count, uint32_t& id)
	{
		glGenBuffers(count, &id);
	}

	void Graphics::SetBufferData( uint32_t buffType, ptrdiff_t size, void* data, uint32_t usage )
	{
		glBufferData(buffType, size, data, usage);
	}

	void Graphics::SetBufferSubData( uint32_t target, ptrdiff_t offset, int32_t size, void* data)
	{
		glBufferSubData(target, offset, size, data);
	}

	void Graphics::SetAttributePointer( uint32_t index, int32_t count, uint32_t datatype, bool normalise, int32_t size, void* offset )
	{
		glVertexAttribPointer(index, count, datatype, normalise ? GL_TRUE : GL_FALSE, size, offset);
	}

	void Graphics::BindBuffer( uint32_t buffType, uint32_t vboID )
	{
		glBindBuffer(buffType, vboID);
	}

	void* Graphics::MapBufferData( uint32_t target, uint32_t access )
	{
		return glMapBuffer(target, access);
	}

	bool Graphics::UnmapBufferData( uint32_t target )
	{
		return glUnmapBuffer(target) ? true : false;
	}

	void Graphics::DrawArrayBuffer( uint32_t drawMode, uint32_t first, uint32_t count )
	{
		glDrawArrays(drawMode, first, count);
	}

	void Graphics::DrawElementBuffer( uint32_t drawMode, int32_t count, uint32_t dataType, void* offset )
	{
		glDrawElements(drawMode, count, dataType, offset);
	}

	void Graphics::EnableVertexAttribArray(uint32_t pos)
	{
		glEnableVertexAttribArray(pos);
	}

	void Graphics::DisableVertexAttribArray(uint32_t pos)
	{
		glDisableVertexAttribArray(pos);
	}

	void Graphics::BindShader(uint32_t programId)
	{
		if (m_activeShaderProgram != programId)
		{
			glUseProgram(programId);
			m_activeShaderProgram = programId;
		}
	}

	void Graphics::UnbindShader()
	{
		glUseProgram(0);
	}

	void Graphics::CreateTextures( uint32_t count, uint32_t& id )
	{
		glGenTextures(count, &id);
	}

	void Graphics::BindTexture( uint32_t target, uint32_t id )
	{
		glBindTexture(target, id);
	}

	void Graphics::BindTexture(ShaderProgram* shader, const std::string& uniformName, const std::string& flagName, Texture* texture, int textureUnit) 
	{
		if (texture && texture->GetGPUID() != 0)
		{
			shader->SetUniformi(flagName, GL_TRUE);
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(GL_TEXTURE_2D, texture->GetGPUID());
			shader->SetUniformi(uniformName, textureUnit);
		}
		else 
		{
			shader->SetUniformi(flagName, GL_FALSE);
			glActiveTexture(GL_TEXTURE0 + textureUnit);
			glBindTexture(GL_TEXTURE_2D, 0);
		}
	}

	void Graphics::SetActiveTexture( uint32_t texunit )
	{
		glActiveTexture(GL_TEXTURE0 + texunit);
	}

	uint32_t Graphics::GetInternalFormat(uint32_t texId, uint32_t texType, uint32_t texTarget)
	{
		glBindTexture(texType, texId);

		GLint internalFormat = 0;
		glGetTexLevelParameteriv(texTarget, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

		GLenum error = glGetError();
		if (error != GL_NO_ERROR)
		{
			std::cerr << "OpenGL Error: " << error << " in GetInternalFormat" << std::endl;
		}

		glBindTexture(texType, 0);
		return internalFormat;
	}

	inline std::string Graphics::InternalFormatToString(GLint internalFormat)
	{
		switch (internalFormat)
		{
			case GL_RGB8: return "GL_RGB8";
			case GL_RGBA8: return "GL_RGBA8";
			case GL_RGB16F: return "GL_RGB16F";
			case GL_RGBA16F: return "GL_RGBA16F";
			case GL_RGB32F: return "GL_RGB32F";
			case GL_RGBA32F: return "GL_RGBA32F";
			case GL_DEPTH_COMPONENT: return "GL_DEPTH_COMPONENT";
			default: return "Unknown Format";
		}
	}

	void Graphics::CreateInstanceBuffer(InstanceBuffer& instancedBO, const std::vector<glm::mat4>& instanceTransforms)
	{
		if (instancedBO.Uploaded()) return;

		GLuint bufferID;
		glGenBuffers(1, &bufferID);
		glBindBuffer(GL_ARRAY_BUFFER, bufferID);
		glBufferData(GL_ARRAY_BUFFER, instanceTransforms.size() * sizeof(glm::mat4), instanceTransforms.data(), 
			instancedBO.IsStatic() ? GL_STATIC_DRAW : GL_DYNAMIC_DRAW);
		auto instanceCount = instanceTransforms.size();

		instancedBO.SetGPUID(bufferID);
		instancedBO.SetInstanceCount((uint32_t)instanceCount);

		// Configure vertex attributes for the mat4 (split into 4 vec4 attributes)
		for (int i = 0; i < 4; ++i) 
		{
			GLuint attribIndex = InstanceBuffer::INSTANCE_MATRIX_LOCATION + i; // Starting location for instance matrix attributes
			glEnableVertexAttribArray(attribIndex);
			glVertexAttribPointer(attribIndex, 4, GL_FLOAT, GL_FALSE, sizeof(glm::mat4), (void*)(sizeof(glm::vec4) * i));
			glVertexAttribDivisor(attribIndex, 1); // Divisor 1 means per-instance data
		}
		glBindBuffer(GL_ARRAY_BUFFER, 0);
	}

	void Graphics::DisposeInstanceBuffer(InstanceBuffer& instancedBO)
	{
		GLuint id = instancedBO.GetGPUID();
		glDeleteBuffers(1, &id);
	}

	void Graphics::CreateBatch(Batch& batch)
	{
		auto vbo = batch.GetVertexBuffer();
		// Create and bind the VAO
		auto vaoId = CreateVertexArray();
		vbo->SetVAO(vaoId);
		BindVertexArray(vaoId);

		// Bind the vertex buffer
		GLuint vboId;
		CreateBuffer(1, vboId);
		vbo->SetId(vboId);
		BindBuffer(GL_ARRAY_BUFFER, vbo->GetId());

		const auto& vertexData = batch.GetVertexBuffer()->GetBuffer(); // Retrieve vertex data
		glBufferData(GL_ARRAY_BUFFER, vertexData.size() * sizeof(float), vertexData.data(), GL_STATIC_DRAW);

		// Parse the attributesKey and bind attributes based on format
		auto vertexAttribKey = batch.attributesKey; // Determines the layout
		uint32_t offset = 0;
		uint32_t index = 0;
		uint32_t stride = CalculateStride(vertexAttribKey);

		for (uint32_t i = 0; i < static_cast<uint32_t>(AttributeType::COUNT); ++i)
		{
			if (vertexAttribKey & (1 << i)) 
			{
				GLenum type = GL_FLOAT; 
				GLsizei size = 0;

				switch (static_cast<AttributeType>(1 << i))
				{
				case AttributeType::POSITION:
					size = 3; 
					break;
				case AttributeType::NORMAL:
					size = 3; 
					break;
				case AttributeType::TEX_COORD_0:
					size = 2; 
					break;
				case AttributeType::TEX_COORD_1:
					size = 2;
					break;
				//case AttributeType::COLOUR:
				//	size = 4;
				//	break;
				case AttributeType::TANGENT:
					size = 4; 
					break;
				default:
					std::cerr << "Unsupported attribute type!" << std::endl;
					continue;
				}

				glEnableVertexAttribArray(index);
				glVertexAttribPointer(
					index,               
					size,                
					type,                
					GL_FALSE,            
					stride,              
					BUFFER_OFFSET(offset)
				);

				offset += size * sizeof(float);
				++index; // Increment attribute index
			}
		}

		// Bind the index buffer
		auto& ibo = batch.GetIndexBuffer();
		if (ibo != nullptr)
		{
			CreateIndexBuffer(*ibo);
			BindBuffer(GL_ELEMENT_ARRAY_BUFFER, ibo->GetId());
		}

		// Unbind the VAO to prevent accidental modifications
		BindVertexArray(0);
	}

	void Graphics::DisposeBatch(Batch& batch)
	{
		DisposeVertexBuffer(*batch.GetVertexBuffer().get());
		DisposeIndexBuffer(*batch.GetIndexBuffer());
	}

	void Graphics::CreateVertexBuffer( VertexBuffer& vbo )
	{
		if (vbo.Uploaded()) return;

		uint32_t id;
		glGenBuffers(1, &id);
		glBindBuffer(vbo.Type(), id);
		glBufferData(vbo.Type(), vbo.SizeInBytes(), vbo.Array(), vbo.DrawType());
		vbo.SetId(id);

		std::set<VertexAttribute> attribs = vbo.GetAttributes();

		uint32_t i = 0;

		for (auto it = vbo.GetAttributes().begin(); it != vbo.GetAttributes().end(); ++it)
		{
			VertexAttribute attrib = *it;

			glEnableVertexAttribArray(i);
			glVertexAttribPointer(i, attrib.m_count, vbo.DataType(), GL_FALSE, vbo.GetStride() * size_f, BUFFER_OFFSET(attrib.m_offset));
			i++;
		}
	}

	void Graphics::CreateIndexBuffer( IndexBuffer& ibo )
	{
		if (ibo.Uploaded()) return;

		uint32_t id;
		glGenBuffers(1, &id);
		glBindBuffer(ibo.Type(), id);
		glBufferData(ibo.Type(), ibo.Size() * sizeof(uint32_t), ibo.Array(), ibo.DrawType());
		ibo.SetId(id);
	}

	void Graphics::CreateTexture( Texture* texture )
	{
		if (!texture)
		{
			throw std::runtime_error("Invalid texture!");
		}

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
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}

		// texture wont be updated so we can use glTexStorage
		if (texture->IsImmutable())
		{
			int mipLevels = 1 + static_cast<int>(std::floor(std::log2(std::max(texture->GetWidth(), texture->GetHeight()))));
			glTexStorage2D(GL_TEXTURE_2D, mipLevels, texture->GetInternalFormat(), texture->GetWidth(), texture->GetHeight());
			glTexSubImage2D(
				GL_TEXTURE_2D, // Target
				0,             // Mipmap level
				0, 0,          // Offset (x, y)
				texture->GetWidth(), texture->GetHeight(), // Dimensions
				texture->GetFormat(),                      // Format (e.g., GL_RGBA)
				GL_UNSIGNED_BYTE,                          // Data type
				texture->GetData().data()                         // Pointer to texture data
			);
		}
		else
		{
			glTexImage2D(GL_TEXTURE_2D, 0, texture->GetInternalFormat(), texture->GetWidth(), texture->GetHeight(), 0, texture->GetFormat(), texture->GetDataType(), texture->GetData().data());
		}
		texture->SetGPUID(image);

		if (texture->GenerateMipmaps())
		{
			glGenerateMipmap(GL_TEXTURE_2D);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
		}
	}

	void Graphics::ReadTexture2D(uint32_t texId, ImageData& imgData, bool useFramebuffer)
	{
		GLenum format = (imgData.channels == 3) ? GL_RGB : GL_RGBA;
		GLenum type = imgData.isHDR ? GL_FLOAT : GL_UNSIGNED_BYTE;

		if (useFramebuffer)
		{
			// Use a framebuffer to read the texture
			GLuint fbo;
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, texId, 0);

			if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
			{
				glDeleteFramebuffers(1, &fbo);
				throw std::runtime_error("Framebuffer is incomplete for Texture2D.");
			}

			// Allocate storage for pixel data
			size_t dataSize = imgData.width * imgData.height * imgData.channels;
			if (imgData.isHDR)
			{
				imgData.hdrData.resize(dataSize);
				glReadPixels(0, 0, imgData.width, imgData.height, format, type, imgData.hdrData.data());
			}
			else
			{
				imgData.data.resize(dataSize);
				glReadPixels(0, 0, imgData.width, imgData.height, format, type, imgData.data.data());
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &fbo);
		}
		else
		{
			// Use glGetTexImage to read the texture
			glBindTexture(GL_TEXTURE_2D, texId);

			GLint internalFormat;
			glGetTexLevelParameteriv(GL_TEXTURE_2D, 0, GL_TEXTURE_INTERNAL_FORMAT, &internalFormat);

			size_t dataSize = imgData.width * imgData.height * imgData.channels;
			if (imgData.isHDR)
			{
				imgData.hdrData.resize(dataSize);
				glGetTexImage(GL_TEXTURE_2D, 0, format, type, imgData.hdrData.data());
			}
			else
			{
				imgData.data.resize(dataSize);
				glGetTexImage(GL_TEXTURE_2D, 0, format, type, imgData.data.data());
			}
		}

		GLenum errorCode = glGetError();
		if (errorCode != GL_NO_ERROR)
		{
			throw std::runtime_error("Failed to read Texture2D. GL Error: " + std::to_string(errorCode));
		}

		std::cout << "Successfully read Texture2D with " << (imgData.isHDR ? "HDR" : "LDR") << " data." << std::endl;
	}

	void Graphics::ReadCubemap(uint32_t texId, int width, int height, int channels, bool hdr, std::array<ImageData, 6>& imgData, bool useFramebuffer)
	{
		GLenum format = channels == 3 ? GL_RGB : GL_RGBA;
		GLenum type = hdr ? GL_FLOAT : GL_UNSIGNED_BYTE;
		size_t faceSize = width * height * channels;

		GLint originalViewport[4];
		glGetIntegerv(GL_VIEWPORT, originalViewport);

		if (useFramebuffer)
		{
			// Use a framebuffer to read each cubemap face
			GLuint fbo;
			glGenFramebuffers(1, &fbo);
			glBindFramebuffer(GL_FRAMEBUFFER, fbo);

			for (int i = 0; i < 6; ++i)
			{
				imgData.at(i).width = width;
				imgData.at(i).height = height;
				imgData.at(i).channels = channels;
				imgData.at(i).isHDR = hdr;
				
				glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, texId, 0);
				glViewport(0, 0, width, height);
				if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
				{
					glDeleteFramebuffers(1, &fbo);
					throw std::runtime_error("Framebuffer is incomplete for cubemap face: " + std::to_string(i));
				}
				glPixelStorei(GL_PACK_ALIGNMENT, 1);
				if (imgData.at(i).isHDR)
				{
					std::vector<float> faceData(faceSize);
					glReadPixels(0, 0, width, height, format, type, faceData.data());
					imgData.at(i).hdrData = std::move(faceData);
				}
				else
				{
					std::vector<unsigned char> faceData(faceSize);
					glReadPixels(0, 0, width, height, format, type, faceData.data());
					imgData.at(i).data = std::move(faceData);
				}
				glPixelStorei(GL_PACK_ALIGNMENT, 4);

				GL_CHECK_ERROR();
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			glDeleteFramebuffers(1, &fbo);
		}
		else
		{
			for (int i = 0; i < 6; ++i)
			{
				auto internalFormat = GetInternalFormat(texId, GL_TEXTURE_CUBE_MAP, GL_TEXTURE_CUBE_MAP_POSITIVE_X + i);
				if (internalFormat == GL_RGB16F || internalFormat == GL_RGBA16F ||
					internalFormat == GL_RGB32F || internalFormat == GL_RGBA32F)
				{
					if (type != GL_FLOAT)
						throw std::runtime_error("Type mismatch for HDR cubemap face " + std::to_string(i));
				}

				imgData.at(i).width = width;
				imgData.at(i).height = height;
				imgData.at(i).channels = channels;
				imgData.at(i).isHDR = hdr;

				glBindTexture(GL_TEXTURE_CUBE_MAP, texId);
				if (hdr)
				{
					std::vector<float> faceData(width * height * channels);
					glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, type, faceData.data());
					imgData.at(i).hdrData = std::move(faceData);
				}
				else
				{
					std::vector<unsigned char> faceData(faceSize);
					glGetTexImage(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, format, type, faceData.data());
					imgData.at(i).data = std::move(faceData);
				}

				GL_CHECK_ERROR();
			}
		}

		glViewport(originalViewport[0], originalViewport[1], originalViewport[2], originalViewport[3]);
		std::cout << "Successfully read Cubemap with " << (imgData.at(0).isHDR ? "HDR" : "LDR") << " data." << std::endl;
	}

	int Graphics::CreateCubemap(std::array<ImageData, 6>& cubeFaceData)
	{
		auto width = cubeFaceData.at(0).width;
		auto height = cubeFaceData.at(0).height;

		auto format = cubeFaceData.at(0).channels == 3 ? GL_RGB : GL_RGBA;
		auto internalFormat = cubeFaceData.at(0).channels == 3 ? GL_RGB16F : GL_RGBA16F;

		// Generate texture ID
		GLuint hdrTexture;
		glGenTextures(1, &hdrTexture);
		glBindTexture(GL_TEXTURE_CUBE_MAP, hdrTexture);

		// Upload cubemap data (for each face)
		for (unsigned int i = 0; i < 6; ++i)
		{
			auto& imgData = cubeFaceData.at(i);
			if (imgData.isHDR)
			{
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0,
					internalFormat, // Internal format
					width,
					height,
					0,
					format,      // Data format
					GL_FLOAT,
					imgData.hdrData.data()
				);
			}
			else
			{
				glTexImage2D(
					GL_TEXTURE_CUBE_MAP_POSITIVE_X + i,
					0,
					internalFormat, // Internal format
					width,
					height,
					0,
					format,      // Data format
					GL_UNSIGNED_BYTE,
					imgData.data.data()
				);
			}
		}

		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
		glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);

		

		return hdrTexture;
	}

	void Graphics::DisposeTexture(Texture* texture)
	{
		GLuint id = texture->GetGPUID();
	
		glDeleteTextures(1, &id);
	}

	void Graphics::DisposeTexture( uint32_t count, uint32_t* textures )
	{
		glDeleteTextures(count, textures);
	}

	void Graphics::Clear(uint32_t flags)
	{
		glClear(flags);
	}

	void Graphics::SwapBuffers()
	{
		m_window->SwapBuffers();
	}

	void Graphics::CreateRenderBuffer( uint32_t count, uint32_t& id )
	{
		glGenRenderbuffers(count, &id);
	}

	void Graphics::BindRenderBuffer(uint32_t id )
	{
		glBindRenderbuffer(GL_RENDERBUFFER, id);
	}

	void Graphics::BindFrameBufferToTexture( uint32_t type, uint32_t attachment, uint32_t target, uint32_t id, int32_t level )
	{
		glFramebufferTexture2D(type, attachment, target, id, level);
	}

	void Graphics::BindFrameBufferToRenderbuffer( uint32_t type, uint32_t attachment, uint32_t target, uint32_t id)
	{
		glFramebufferRenderbuffer(type, attachment, target, id);
	}

	void Graphics::RenderBufferStorage( uint32_t type, uint32_t internalFormat, uint32_t width, uint32_t height )
	{
		glRenderbufferStorage(type, internalFormat, width, height);
	}

	void Graphics::DrawBuffers( uint32_t count, uint32_t* targets)
	{
		glDrawBuffers(count, targets);
	}

	void Graphics::DisposeBuffer( uint32_t count, uint32_t* id )
	{
		glDeleteBuffers(count, id);
	}

	uint32_t Graphics::CheckFrameBuffer()
	{
		return glCheckFramebufferStatus(GL_FRAMEBUFFER);
	}

	void Graphics::SetViewport( uint32_t x, uint32_t y, uint32_t width, uint32_t height )
	{
		glViewport(x, y, width, height);
	}

	void Graphics::DisposeVertexBuffer( VertexBuffer& vbo )
	{
		if (vbo.GetVAO() > 0)
		{
			GLuint id = vbo.GetVAO();
			glDeleteVertexArrays(1, &id);
		}

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
		if (target->GetWidth() == 0 || target->GetHeight() == 0)
		{
			std::cerr << "Graphics: Invalid render target dimensions!" << std::endl;
			return false;
		}

		GLuint fbo;
		glGenFramebuffers(1, &fbo);
		glBindFramebuffer(GL_FRAMEBUFFER, fbo);
		target->SetFrameBufferId(fbo);

		auto& attributes = target->GetTextureAttributes();

		for (uint32_t i = 0; i < target->GetNumSources(); i++)
		{
			GLuint tex;
			glGenTextures(1, &tex);
			glBindTexture(GL_TEXTURE_2D, tex);

			auto& attrib = attributes[i];

			glTexImage2D(GL_TEXTURE_2D, 0, attrib.internalFormat, target->GetWidth(), target->GetHeight(), 0, attrib.format, attrib.dataType, nullptr);
			//glTexStorage2D(GL_TEXTURE_2D, 1, attrib.internalFormat, target->GetWidth(), target->GetHeight());
			// glTexStorage2DMultisample(GL_TEXTURE_2D_MULTISAMPLE, 4, attrib.internalFormat, target->GetWidth(), target->GetHeight(), GL_TRUE);
			glFramebufferTexture(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0 + i, tex, 0);
			target->SetSourceId(i, tex);

			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, attrib.minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, attrib.magFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, attrib.wrapS);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, attrib.wrapT);
		}

		GLuint depth;
		if (target->DepthType() == DepthType::Renderbuffer)
		{
			glGenRenderbuffers(1, &depth);
			glBindRenderbuffer(GL_RENDERBUFFER, depth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, target->GetWidth(), target->GetHeight());
			// glRenderbufferStorageMultisample(GL_RENDERBUFFER, 4, GL_DEPTH_COMPONENT, target->GetWidth(), target->GetHeight()); // 4x MSAA
			target->SetDepthId(depth);
			glFramebufferRenderbuffer(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_RENDERBUFFER, depth);
		}
		else if (target->DepthType() == DepthType::Texture)
		{
			glGenTextures(1, &depth);
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT32, target->GetWidth(), target->GetHeight(), 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
			target->SetDepthId(depth);
			glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
		}

		glDrawBuffers(target->GetNumSources(), target->GetDrawBuffers().data());

		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			std::cerr << "Framebuffer not complete: ";
			switch (status) 
			{
				case GL_FRAMEBUFFER_UNDEFINED: std::cerr << "GL_FRAMEBUFFER_UNDEFINED"; break;
				case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
				case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
				case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
				case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
				case GL_FRAMEBUFFER_UNSUPPORTED: std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED"; break;
				default: std::cerr << "Unknown Error"; break;
			}
			std::cerr << std::endl;

			glDeleteFramebuffers(1, &fbo);
			for (uint32_t i = 0; i < target->GetNumSources(); i++)
			{
				GLuint tex = target->GetSourceId(i);
				glDeleteTextures(1, &tex);
			}
			if (target->DepthType() == DepthType::Renderbuffer)
			{
				GLuint dboId = target->GetDepthBufferId();
				glDeleteRenderbuffers(1, &dboId);
			}
			else if (target->DepthType() == DepthType::Texture)
			{
				GLuint dboId = target->GetDepthBufferId();
				glDeleteTextures(1, &dboId);
			}

			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		return true;
	}

	bool Graphics::ResizeRenderTarget(RenderTarget* target, int newWidth, int newHeight)
	{
		if (newWidth == 0 || newHeight == 0)
		{
			std::cerr << "Graphics: Invalid render target dimensions for resizing!" << std::endl;
			return false;
		}

		target->SetWidth(newWidth);
		target->SetHeight(newHeight);

		glBindFramebuffer(GL_FRAMEBUFFER, target->GetFrameBufferId());

		// Resize textures for color attachments
		auto& attributes = target->GetTextureAttributes();
		for (uint32_t i = 0; i < target->GetNumSources(); i++)
		{
			GLuint tex = target->GetSourceId(i);
			glBindTexture(GL_TEXTURE_2D, tex);

			auto& attrib = attributes[i];
			glTexImage2D(GL_TEXTURE_2D, 0, attrib.internalFormat, newWidth, newHeight, 0, attrib.format, attrib.dataType, nullptr);
		}

		// Resize depth buffer
		GLuint depth = target->GetDepthBufferId();
		if (target->DepthType() == DepthType::Renderbuffer)
		{
			glBindRenderbuffer(GL_RENDERBUFFER, depth);
			glRenderbufferStorage(GL_RENDERBUFFER, GL_DEPTH_COMPONENT, newWidth, newHeight);
		}
		else if (target->DepthType() == DepthType::Texture)
		{
			glBindTexture(GL_TEXTURE_2D, depth);
			glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT24, newWidth, newHeight, 0, GL_DEPTH_COMPONENT, GL_FLOAT, nullptr);
		}

		// Check framebuffer completeness
		if (glCheckFramebufferStatus(GL_FRAMEBUFFER) != GL_FRAMEBUFFER_COMPLETE)
		{
			GLenum status = glCheckFramebufferStatus(GL_FRAMEBUFFER);
			std::cerr << "Framebuffer resize failed: ";
			switch (status)
			{
			case GL_FRAMEBUFFER_UNDEFINED: std::cerr << "GL_FRAMEBUFFER_UNDEFINED"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_MISSING_ATTACHMENT"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_DRAW_BUFFER"; break;
			case GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER: std::cerr << "GL_FRAMEBUFFER_INCOMPLETE_READ_BUFFER"; break;
			case GL_FRAMEBUFFER_UNSUPPORTED: std::cerr << "GL_FRAMEBUFFER_UNSUPPORTED"; break;
			default: std::cerr << "Unknown Error"; break;
			}
			std::cerr << std::endl;
			glBindFramebuffer(GL_FRAMEBUFFER, 0);
			return false;
		}

		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		glBindTexture(GL_TEXTURE_2D, 0);
		glBindRenderbuffer(GL_RENDERBUFFER, 0);

		std::cout << "RenderTarget resized to " << newWidth << "x" << newHeight << std::endl;
		return true;
	}

	void Graphics::DisposeRenderTarget( RenderTarget* target )
	{
		GLuint fboId = target->GetFrameBufferId();
		
		glDeleteTextures(target->GetNumSources(), target->GetSources().data());
		glDeleteFramebuffers(1, &fboId);

		if (target->DepthType() == DepthType::Renderbuffer)
		{
			GLuint dboId = target->GetDepthBufferId();
			glDeleteRenderbuffers(1, &dboId);
		}
		else if (target->DepthType() == DepthType::Texture)
		{
			GLuint dboId = target->GetDepthBufferId();
			glDeleteTextures(1, &dboId);
		}
	}

	void Graphics::DisposeFrameBuffer( uint32_t count, uint32_t* fbo )
	{
		glDeleteFramebuffers(count, fbo);
	}

	void Graphics::GeneratePrimitives()
	{
		float coneVerts[13 * 3] = 
		{  
			0.f, 0.f, 0.f,
			0.f, 1.f, -1.f,   -0.5f, 0.866f, -1.f,   -0.866f, 0.5f, -1.f,
			-1.f, 0.f, -1.f,   -0.866f, -0.5f, -1.f,   -0.5f, -0.866f, -1.f,
			0.f, -1.f, -1.f,   0.5f, -0.866f, -1.f,   0.866f, -0.5f, -1.f,
			1.f, 0.f, -1.f,   0.866f, 0.5f, -1.f,   0.5f, 0.866f, -1.f,
		};
		uint32_t coneInds[22 * 3] = 
		{
			0, 1, 2,   0, 2, 3,   0, 3, 4,   0, 4, 5,   0, 5, 6,   0, 6, 7,
			0, 7, 8,   0, 8, 9,   0, 9, 10,   0, 10, 11,   0, 11, 12,   0, 12, 1,
			10, 6, 2,   10, 8, 6,   10, 9, 8,   8, 7, 6,   6, 4, 2,   6, 5, 4,   4, 3, 2,
			2, 12, 10,   2, 1, 12,   12, 11, 10
		};
	
		float octahedronVerts[18] = 
		{
			 0.0f,  1.0f,  0.0f,  
			 1.0f,  0.0f,  0.0f,  
			 0.0f,  0.0f,  1.0f,  
			 0.0f,  0.0f, -1.0f,  
			-1.0f,  0.0f,  0.0f,  
			 0.0f, -1.0f,  0.0f   
		};

		uint32_t octahedronInds[24] = 
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

		GLfloat fullScreenQuad[] =
		{
			// position				// texture coordinates
			-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			1.0f, -1.0f, 0.0f,		1.0f, 0.0f,
			1.0f,  1.0f, 0.0f,		1.0f, 1.0f,
			-1.0f, -1.0f, 0.0f,		0.0f, 0.0f,
			1.0f, 1.0f, 0.0f,	    1.0f, 1.0f,
			-1.0f,  1.0f, 0.0f,		0.0f, 1.0f
		};
		VertexBuffer vbo(GL_ARRAY_BUFFER, GL_FLOAT, GL_STATIC_DRAW);
		vbo.InsertAt(0, fullScreenQuad, 36);
		auto posAttri = VertexAttribute(AttributeType::POSITION, 0, 3);
		auto uvAttri = VertexAttribute(AttributeType::TEX_COORD_0, 12, 2);
		vbo.AddAttribute(posAttri);
		vbo.AddAttribute(uvAttri);
		vbo.CalcStride();
		uint32_t vao;
		vao = CreateVertexArray();
		CreateVertexBuffer(vbo);

		CreatePrimitiveBuffers(octahedronVerts, sizeof(octahedronVerts), octahedronInds, sizeof(octahedronInds), m_octahedronGeom);
		CreatePrimitiveBuffers(coneVerts, sizeof(coneVerts), coneInds, sizeof(coneInds), m_coneGeom);
	}

	void Graphics::CreatePrimitiveBuffers( float vertices[], uint32_t vertSize, uint32_t indices[], uint32_t indSize, uint32_t ids[] )
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
		ids[3] = indSize / sizeof(uint32_t);
		glBindVertexArray(0);
	}

	void Graphics::CreatePrimitiveBuffers(float vertices[], uint32_t vertSize, uint32_t ids[])
	{
		glGenVertexArrays(1, &ids[0]);
		glBindVertexArray(ids[0]);
		glGenBuffers(1, &ids[1]);
		glBindBuffer(GL_ARRAY_BUFFER, ids[1]);
		glBufferData(GL_ARRAY_BUFFER, vertSize, vertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
	}

	void Graphics::SetBlendFunc( uint32_t first, uint32_t second )
	{
		glBlendFunc(first, second);
	}

	void Graphics::Enable( uint32_t val )
	{
		glEnable(val);
	}

	void Graphics::Disable( uint32_t val )
	{
		glDisable(val);
	}

	void Graphics::SetDepthFunc( uint32_t val )
	{
		glDepthFunc(val);
	}

	void Graphics::SetDepthMask( uint32_t val )
	{
		glDepthMask(val);
	}

	void Graphics::BeginPrimitiveDraw()
	{
		glUseProgram(m_defaultShader);
	}

	void Graphics::RenderPrimitive(glm::mat4& mvp, uint32_t type, uint32_t shaderId )
	{
		if (shaderId == -1)
		{	
			auto loc = glGetUniformLocation(m_defaultShader, "u_MVP");
			SetUniform(loc, 1, false, mvp);
		}
		else
		{
			auto loc = glGetUniformLocation(shaderId, "u_MVP");
			SetUniform(loc, 1, false, mvp);
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

	void Graphics::SetCullFace( uint32_t face )
	{
		glCullFace(face);
	}

	void Graphics::DumpInfo() const
	{
		std::cout << "****************************************************" << std::endl;
		std::cout << m_shaderInfo << std::endl;
		std::cout << m_versionInfo << std::endl;
		std::cout << m_vendorInfo << std::endl;
		std::cout << m_rendererInfo << std::endl;
		std::cout << "MSAA Buffers: " << m_MSAABuffers << std::endl;
		std::cout << "MSAA Samples: " << m_MSAASamples << std::endl;
		std::cout << "****************************************************" << std::endl;
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
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vbo.getIndexArray().size() * sizeof(uint32_t), &vbo.getIndexArray()[0], GL_STATIC_DRAW);

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
//	glBufferData(GL_ELEMENT_ARRAY_BUFFER, vbo.getIndexArray().size() * sizeof(uint32_t), &vbo.getIndexArray()[0], GL_STATIC_DRAW);

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
