#pragma once
#include <list>
#include <utility>
#include <initializer_list>
#include <memory>
#include <unordered_map>
#include <set>

#include <glm/glm.hpp>
#include <glbinding/gl/gl.h>
#include <glbinding/gl/enum.h>
#include <glbinding/gl/functions.h>

#include <globjects/VertexArray.h>
#include <globjects/VertexAttributeBinding.h>
#include <globjects/Buffer.h>
#include <globjects/Program.h>
#include <globjects/Shader.h>
#include <globjects/Framebuffer.h>
#include <globjects/Renderbuffer.h>
#include <globjects/Texture.h>
#include <globjects/base/File.h>
#include <globjects/TextureHandle.h>
#include <globjects/NamedString.h>
#include <globjects/base/StaticStringSource.h>

namespace dynamol
{
	class Viewer;

	class Renderer
	{
		struct ShaderProgram
		{
			std::set< std::unique_ptr< globjects::File> > m_files;
			std::set< std::unique_ptr< globjects::AbstractStringSource> > m_sources;
			std::set< std::unique_ptr< globjects::NamedString> > m_strings;
			std::set< std::unique_ptr< globjects::Shader > > m_shaders;
			std::unique_ptr< globjects::Program > m_program = std::make_unique<globjects::Program>();
		};

	public:
		Renderer(Viewer* viewer);
		Viewer* viewer();
		void setEnabled(bool enabled);
		bool isEnabled() const;

		virtual void reloadShaders();
		virtual void display() = 0;

		bool createShaderProgram(const std::string& name, std::initializer_list< std::pair<gl::GLenum, std::string> > shaders, std::initializer_list < std::string> shaderIncludes = {});
		globjects::Program* shaderProgram(const std::string& name);

	private:
		Viewer* m_viewer;
		bool m_enabled = true;
		std::unordered_map<std::string, ShaderProgram > m_shaderPrograms;

	};

}