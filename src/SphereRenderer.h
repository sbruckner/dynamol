#pragma once
#include "Renderer.h"
#include <memory>

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

	class SphereRenderer : public Renderer
	{
	public:
		SphereRenderer(Viewer *viewer);
		virtual void display();
		virtual std::list<globjects::File*> shaderFiles() const;

	private:
		
		std::vector< std::unique_ptr<globjects::Buffer> > m_vertices;
		std::unique_ptr<globjects::VertexArray> m_vao = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_elementColorsRadii = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_residueColors = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_chainColors = std::make_unique<globjects::Buffer>();
		
		std::unique_ptr<globjects::VertexArray> m_vaoQuad = std::make_unique<globjects::VertexArray>();
		std::unique_ptr<globjects::Buffer> m_verticesQuad = std::make_unique<globjects::Buffer>();
		
		std::unique_ptr<globjects::Program> m_programSphere = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programSpawn = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programSurface = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programAOSample = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programAOBlur = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programShade = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programDOFBlur = std::make_unique<globjects::Program>();
		std::unique_ptr<globjects::Program> m_programDOFBlend = std::make_unique<globjects::Program>();

		std::unique_ptr<globjects::StaticStringSource> m_shaderSourceDefines = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderDefines = nullptr;

		std::unique_ptr<globjects::File> m_shaderSourceGlobals = nullptr;
		std::unique_ptr<globjects::NamedString> m_shaderGlobals = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_geometryShaderTemplateSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSphere = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateSphere = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSphere = nullptr;

		std::unique_ptr<globjects::File> m_vertexShaderSourceImage = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_vertexShaderTemplateImage = nullptr;
		std::unique_ptr<globjects::Shader> m_vertexShaderImage = nullptr;

		std::unique_ptr<globjects::File> m_geometryShaderSourceImage = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_geometryShaderTemplateImage = nullptr;
		std::unique_ptr<globjects::Shader> m_geometryShaderImage = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSpawn = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateSpawn = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSpawn = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceSurface = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateSurface = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderSurface = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceAOSample = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateAOSample = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderAOSample = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceAOBlur = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateAOBlur = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderAOBlur = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceShade = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateShade = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderShade = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceDOFBlur = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateDOFBlur = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderDOFBlur = nullptr;

		std::unique_ptr<globjects::File> m_fragmentShaderSourceDOFBlend = nullptr;
		std::unique_ptr<globjects::AbstractStringSource> m_fragmentShaderTemplateDOFBlend = nullptr;
		std::unique_ptr<globjects::Shader> m_fragmentShaderDOFBlend = nullptr;

		std::unique_ptr<globjects::Buffer> m_intersectionBuffer = std::make_unique<globjects::Buffer>();
		std::unique_ptr<globjects::Buffer> m_statisticsBuffer = std::make_unique<globjects::Buffer>();

		std::unique_ptr<globjects::Texture> m_offsetTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_depthTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_spherePositionTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_sphereNormalTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_surfacePositionTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_surfaceNormalTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_sphereDiffuseTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_surfaceDiffuseTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_ambientTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_blurTexture = nullptr;
		std::unique_ptr<globjects::Texture> m_colorTexture = nullptr;

		std::unique_ptr<globjects::Framebuffer> m_sphereFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_surfaceFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_ambientFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_shadeFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_aoFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_aoBlurFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_dofFramebuffer = nullptr;
		std::unique_ptr<globjects::Framebuffer> m_dofBlurFramebuffer = nullptr;


		std::vector< std::unique_ptr<globjects::Texture> > m_environmentTextures;
		std::vector< std::unique_ptr<globjects::Texture> > m_materialTextures;
		std::vector< std::unique_ptr<globjects::Texture> > m_bumpTextures;

		glm::ivec2 m_framebufferSize;
	};

}