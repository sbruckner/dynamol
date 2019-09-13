#include "SphereRenderer.h"
#include <globjects/base/File.h>
#include <globjects/State.h>
#include <iostream>
#include <filesystem>
#include <imgui.h>
#include "Viewer.h"
#include "Scene.h"
#include "Protein.h"
#include <sstream>

#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>

#define STB_IMAGE_IMPLEMENTATION
#include <stb_image.h>

using namespace dynamol;
using namespace gl;
using namespace glm;
using namespace globjects;

std::unique_ptr<Texture> loadTexture(const std::string & filename)
{
	int width, height, channels;

	stbi_set_flip_vertically_on_load(true);
	unsigned char *data = stbi_load(filename.c_str(), &width, &height, &channels, 0);

	if (data)
	{
		std::cout << "Loaded " << filename << std::endl;

		auto texture = Texture::create(GL_TEXTURE_2D);
		texture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR_MIPMAP_LINEAR);
		texture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		texture->setParameter(GL_TEXTURE_WRAP_S, GL_REPEAT);
		texture->setParameter(GL_TEXTURE_WRAP_T, GL_REPEAT);

		GLenum format = GL_RGBA;

		switch (channels)
		{
		case 1:
			format = GL_RED;
			break;

		case 2:
			format = GL_RG;
			break;

		case 3:
			format = GL_RGB;
			break;

		case 4:
			format = GL_RGBA;
			break;
		}

		texture->image2D(0, format, ivec2(width, height), 0, format, GL_UNSIGNED_BYTE, data);
		texture->generateMipmap();

		stbi_image_free(data);

		return texture;
	}

	return std::unique_ptr<Texture>();
}

SphereRenderer::SphereRenderer(Viewer* viewer) : Renderer(viewer)
{
	Shader::hintIncludeImplementation(Shader::IncludeImplementation::Fallback);

	for (auto i : viewer->scene()->protein()->atoms())
	{
		m_vertices.push_back(Buffer::create());
		m_vertices.back()->setStorage(i, gl::GL_NONE_BIT);
	}
	
	m_elementColorsRadii->setStorage(viewer->scene()->protein()->activeElementColorsRadiiPacked(), gl::GL_NONE_BIT);
	m_residueColors->setStorage(viewer->scene()->protein()->activeResidueColorsPacked(), gl::GL_NONE_BIT);
	m_chainColors->setStorage(viewer->scene()->protein()->activeChainColorsPacked(), gl::GL_NONE_BIT);
	
	m_intersectionBuffer->setStorage(sizeof(vec3) * 1024*1024*128 + sizeof(uint), nullptr, gl::GL_NONE_BIT);

	m_verticesQuad->setStorage(std::array<vec3, 1>({ vec3(0.0f, 0.0f, 0.0f) }), gl::GL_NONE_BIT);
	auto vertexBindingQuad = m_vaoQuad->binding(0);
	vertexBindingQuad->setBuffer(m_verticesQuad.get(), 0, sizeof(vec3));
	vertexBindingQuad->setFormat(3, GL_FLOAT);
	m_vaoQuad->enable(0);
	m_vaoQuad->unbind();

	m_shaderSourceDefines = StaticStringSource::create("");
	m_shaderDefines = NamedString::create("/defines.glsl", m_shaderSourceDefines.get());

	m_shaderSourceGlobals = File::create("./res/sphere/globals.glsl");
	m_shaderGlobals = NamedString::create("/globals.glsl", m_shaderSourceGlobals.get());

	m_vertexShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-vs.glsl");
	m_geometryShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-gs.glsl");
	m_fragmentShaderSourceSphere = Shader::sourceFromFile("./res/sphere/sphere-fs.glsl");
	m_fragmentShaderSourceSpawn = Shader::sourceFromFile("./res/sphere/spawn-fs.glsl");
	m_vertexShaderSourceImage = Shader::sourceFromFile("./res/sphere/image-vs.glsl");
	m_geometryShaderSourceImage = Shader::sourceFromFile("./res/sphere/image-gs.glsl");
	m_fragmentShaderSourceSurface = Shader::sourceFromFile("./res/sphere/surface-fs.glsl");
	m_fragmentShaderSourceAOSample = Shader::sourceFromFile("./res/sphere/aosample-fs.glsl");
	m_fragmentShaderSourceAOBlur = Shader::sourceFromFile("./res/sphere/aoblur-fs.glsl");
	m_fragmentShaderSourceShade = Shader::sourceFromFile("./res/sphere/shade-fs.glsl");
	m_fragmentShaderSourceDOFBlur = Shader::sourceFromFile("./res/sphere/dofblur-fs.glsl");
	m_fragmentShaderSourceDOFBlend = Shader::sourceFromFile("./res/sphere/dofblend-fs.glsl");
	
	m_vertexShaderTemplateSphere = Shader::applyGlobalReplacements(m_vertexShaderSourceSphere.get());
	m_geometryShaderTemplateSphere = Shader::applyGlobalReplacements(m_geometryShaderSourceSphere.get());
	m_fragmentShaderTemplateSphere = Shader::applyGlobalReplacements(m_fragmentShaderSourceSphere.get());
	m_fragmentShaderTemplateSpawn = Shader::applyGlobalReplacements(m_fragmentShaderSourceSpawn.get());
	m_vertexShaderTemplateImage = Shader::applyGlobalReplacements(m_vertexShaderSourceImage.get());
	m_geometryShaderTemplateImage = Shader::applyGlobalReplacements(m_geometryShaderSourceImage.get());
	m_fragmentShaderTemplateSurface = Shader::applyGlobalReplacements(m_fragmentShaderSourceSurface.get());
	m_fragmentShaderTemplateAOSample = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOSample.get());
	m_fragmentShaderTemplateAOBlur = Shader::applyGlobalReplacements(m_fragmentShaderSourceAOBlur.get());
	m_fragmentShaderTemplateShade = Shader::applyGlobalReplacements(m_fragmentShaderSourceShade.get());
	m_fragmentShaderTemplateDOFBlur = Shader::applyGlobalReplacements(m_fragmentShaderSourceDOFBlur.get());
	m_fragmentShaderTemplateDOFBlend = Shader::applyGlobalReplacements(m_fragmentShaderSourceDOFBlend.get());

	m_vertexShaderSphere = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateSphere.get());
	m_geometryShaderSphere = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateSphere.get());
	m_fragmentShaderSphere = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSphere.get());
	m_fragmentShaderSpawn = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSpawn.get());
	m_vertexShaderImage = Shader::create(GL_VERTEX_SHADER, m_vertexShaderTemplateImage.get());
	m_geometryShaderImage = Shader::create(GL_GEOMETRY_SHADER, m_geometryShaderTemplateImage.get());
	m_fragmentShaderSurface = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateSurface.get());
	m_fragmentShaderAOSample = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOSample.get());
	m_fragmentShaderAOBlur = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateAOBlur.get());
	m_fragmentShaderShade = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateShade.get());
	m_fragmentShaderDOFBlur = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateDOFBlur.get());
	m_fragmentShaderDOFBlend = Shader::create(GL_FRAGMENT_SHADER, m_fragmentShaderTemplateDOFBlend.get());

	m_programSphere->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSphere.get());
	m_programSpawn->attach(m_vertexShaderSphere.get(), m_geometryShaderSphere.get(), m_fragmentShaderSpawn.get());
	m_programSurface->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderSurface.get());
	m_programAOSample->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOSample.get());
	m_programAOBlur->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderAOBlur.get());
	m_programShade->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderShade.get());
	m_programDOFBlur->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderDOFBlur.get());
	m_programDOFBlend->attach(m_vertexShaderImage.get(), m_geometryShaderImage.get(), m_fragmentShaderDOFBlend.get());

	m_framebufferSize = viewer->viewportSize();

	m_depthTexture = Texture::create(GL_TEXTURE_2D);
	m_depthTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_depthTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_depthTexture->image2D(0, GL_DEPTH_COMPONENT, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);

	m_spherePositionTexture = Texture::create(GL_TEXTURE_2D);
	m_spherePositionTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_spherePositionTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_spherePositionTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_spherePositionTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_spherePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_sphereNormalTexture = Texture::create(GL_TEXTURE_2D);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_sphereNormalTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_sphereNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_surfacePositionTexture = Texture::create(GL_TEXTURE_2D);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_surfacePositionTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_surfacePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_surfaceNormalTexture = Texture::create(GL_TEXTURE_2D);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_surfaceNormalTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_surfaceNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_sphereDiffuseTexture = Texture::create(GL_TEXTURE_2D);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_sphereDiffuseTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_sphereDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_surfaceDiffuseTexture = Texture::create(GL_TEXTURE_2D);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_surfaceDiffuseTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_surfaceDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_ambientTexture = Texture::create(GL_TEXTURE_2D);
	m_ambientTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_ambientTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_ambientTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_ambientTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_ambientTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_blurTexture = Texture::create(GL_TEXTURE_2D);
	m_blurTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_blurTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_blurTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_blurTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_blurTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_colorTexture = Texture::create(GL_TEXTURE_2D);
	m_colorTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_colorTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);

	m_offsetTexture = Texture::create(GL_TEXTURE_2D);
	m_offsetTexture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	m_offsetTexture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	m_offsetTexture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	m_offsetTexture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	m_offsetTexture->image2D(0, GL_R32UI, m_framebufferSize, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);

	stbi_set_flip_vertically_on_load(true);

	for (auto& d : std::filesystem::directory_iterator("./dat/environments"))
	{
		std::filesystem::path environmentPath(d);

		std::unique_ptr<Texture> texture = loadTexture(environmentPath.string());

		if (texture)
			m_environmentTextures.push_back(std::move(texture));

	}

	for (auto& d : std::filesystem::directory_iterator("./dat/materials"))
	{
		std::filesystem::path materialPath(d);

		std::unique_ptr<Texture> texture = loadTexture(materialPath.string());

		if (texture)
		{
			texture->setParameter(GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			texture->setParameter(GL_TEXTURE_MAG_FILTER, GL_LINEAR);
			texture->setParameter(GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
			texture->setParameter(GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);

			m_materialTextures.push_back(std::move(texture));
		}
	}
	
	for (auto& d : std::filesystem::directory_iterator("./dat/bumps"))
	{
		std::filesystem::path bumpPath(d);

		std::unique_ptr<Texture> texture = loadTexture(bumpPath.string());

		if (texture)
			m_bumpTextures.push_back(std::move(texture));
	}

	m_sphereFramebuffer = Framebuffer::create();
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_spherePositionTexture.get());
	m_sphereFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_sphereNormalTexture.get());
	m_sphereFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_sphereFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_surfaceFramebuffer = Framebuffer::create();
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_surfacePositionTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_surfaceNormalTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT2, m_surfaceDiffuseTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_COLOR_ATTACHMENT3, m_sphereDiffuseTexture.get());
	m_surfaceFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());
	m_surfaceFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2, GL_COLOR_ATTACHMENT3 });

	m_shadeFramebuffer = Framebuffer::create();
	m_shadeFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_colorTexture.get());
	m_shadeFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });
	m_shadeFramebuffer->attachTexture(GL_DEPTH_ATTACHMENT, m_depthTexture.get());

	m_aoBlurFramebuffer = Framebuffer::create();
	m_aoBlurFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_blurTexture.get());
	m_aoBlurFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_aoFramebuffer = Framebuffer::create();
	m_aoFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_ambientTexture.get());
	m_aoFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0 });

	m_dofBlurFramebuffer = Framebuffer::create();
	m_dofBlurFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_sphereNormalTexture.get());
	m_dofBlurFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_surfaceNormalTexture.get());
	m_dofBlurFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });

	m_dofFramebuffer = Framebuffer::create();
	m_dofFramebuffer->attachTexture(GL_COLOR_ATTACHMENT0, m_sphereDiffuseTexture.get());
	m_dofFramebuffer->attachTexture(GL_COLOR_ATTACHMENT1, m_surfaceDiffuseTexture.get());
	m_dofFramebuffer->setDrawBuffers({ GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1 });
}

std::list<globjects::File*> SphereRenderer::shaderFiles() const
{
	return std::list<globjects::File*>({ 
		m_shaderSourceGlobals.get(),
		m_vertexShaderSourceSphere.get(),
		m_geometryShaderSourceSphere.get(),
		m_fragmentShaderSourceSphere.get(),
		m_vertexShaderSourceImage.get(),
		m_geometryShaderSourceImage.get(),
		m_fragmentShaderSourceSpawn.get(),
		m_fragmentShaderSourceSurface.get(),
		m_fragmentShaderSourceAOSample.get(),
		m_fragmentShaderSourceAOBlur.get(),
		m_fragmentShaderSourceShade.get(),
		m_fragmentShaderSourceDOFBlur.get(),
		m_fragmentShaderSourceDOFBlend.get()
		});
}

void SphereRenderer::display()
{
	if (viewer()->scene()->protein()->atoms().size() == 0)
		return;

	// SaveOpenGL state
	auto currentState = State::currentState();

	const ivec2 viewportSize = viewer()->viewportSize();

	// Resize all FBOs if the viewport size has changed
	if (viewportSize != m_framebufferSize)
	{
		m_framebufferSize = viewportSize;
		m_offsetTexture->image2D(0, GL_R32UI, m_framebufferSize, 0, GL_RED_INTEGER, GL_UNSIGNED_BYTE, nullptr);
		m_depthTexture->image2D(0, GL_DEPTH_COMPONENT, m_framebufferSize, 0, GL_DEPTH_COMPONENT, GL_UNSIGNED_BYTE, nullptr);
		m_spherePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_sphereNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_surfacePositionTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_surfaceNormalTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_surfaceDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_sphereDiffuseTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_ambientTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_blurTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
		m_colorTexture->image2D(0, GL_RGBA32F, m_framebufferSize, 0, GL_RGBA, GL_UNSIGNED_BYTE, nullptr);
	}
	
	// get cursor position for magic lens
	double mouseX, mouseY;
	glfwGetCursorPos(viewer()->window(), &mouseX, &mouseY);
	const vec2 focusPosition = vec2(2.0f*float(mouseX) / float(viewportSize.x) - 1.0f, -2.0f*float(mouseY) / float(viewportSize.y) + 1.0f);

	// retrieve/compute all necessary matrices and related properties
	const mat4 viewMatrix = viewer()->viewTransform();
	const mat4 inverseViewMatrix = inverse(viewMatrix);
	const mat4 modelViewMatrix = viewer()->modelViewTransform();
	const mat4 inverseModelViewMatrix = inverse(modelViewMatrix);
	const mat4 modelLightMatrix = viewer()->modelLightTransform();
	const mat4 inverseModelLightMatrix = inverse(modelLightMatrix);
	const mat4 modelViewProjectionMatrix = viewer()->modelViewProjectionTransform();
	const mat4 inverseModelViewProjectionMatrix = inverse(modelViewProjectionMatrix);
	const mat4 projectionMatrix = viewer()->projectionTransform();
	const mat4 inverseProjectionMatrix = inverse(projectionMatrix);
	const mat3 normalMatrix = mat3(transpose(inverseModelViewMatrix));
	const mat3 inverseNormalMatrix = inverse(normalMatrix);

	const vec4 projectionInfo(float(-2.0 / (viewportSize.x * projectionMatrix[0][0])),
		float(-2.0 / (viewportSize.y * projectionMatrix[1][1])),
		float((1.0 - (double)projectionMatrix[0][2]) / projectionMatrix[0][0]),
		float((1.0 + (double)projectionMatrix[1][2]) / projectionMatrix[1][1]));

	const float projectionScale = float(viewportSize.y) / fabs(2.0f / projectionMatrix[1][1]);
	const float fieldOfView = 2.0f * atan(1.0f / projectionMatrix[1][1]);

	vec4 nearPlane = inverseProjectionMatrix * vec4(0.0, 0.0, -1.0, 1.0);
	nearPlane /= nearPlane.w;

	vec4 worldLightPosition = inverseModelLightMatrix * vec4(0.0f, 0.0f, 0.0f, 1.0f);
	vec4 viewLightPosition = modelViewMatrix * worldLightPosition;

	// all input parameters and their default values
	static vec3 ambientMaterial(0.3f, 0.3f, 0.3f);
	static vec3 diffuseMaterial(0.6f, 0.6f, 0.6f);
	static vec3 specularMaterial(0.3f, 0.3f, 0.3f);
	static float shininess = 20.0f;
	static float sharpness = 1.0f;

	static float distanceBlending = 0.0f;
	static float distanceScale = 1.0;

	static bool ambientOcclusion = false;
	static bool environmentMapping = false;
	static bool environmentLighting = false;
	static bool normalMapping = false;
	static bool materialMapping = false;
	static bool depthOfField = false;

	static int coloring = 0;
	static bool animate = false;
	static float animationAmplitude = 1.0f;
	static float animationFrequency = 1.0f;
	static bool lens = false;

	static float focalDistance = 2.0f*sqrt(3.0f);
	static float maximumCoCRadius = 9.0f;
	static float farRadiusRescale = 1.0f;
	static float focalLength = 1.0f;
	static float aparture = 1.0f;
	static float fStop = 1.0;
	static int fStop_current = 12;
	const char* fStops[] = { "0.7", "0.8", "1.0", "1.2", "1.4", "1.7", "2.0", "2.4", "2.8", "3.3", "4.0", "4.8", "5.6", "6.7", "8.0", "9.5", "11.0", "16.0", "22.0", "32.0" };


	// user interface for manipulating rendering parameters
	ImGui::Begin("Surface Renderer");

	if (ImGui::CollapsingHeader("Lighting"))
	{
		ImGui::ColorEdit3("Ambient", (float*)&ambientMaterial);
		ImGui::ColorEdit3("Diffuse", (float*)&diffuseMaterial);
		ImGui::ColorEdit3("Specular", (float*)&specularMaterial);
		ImGui::SliderFloat("Shininess", &shininess, 1.0f, 256.0f);
		ImGui::Checkbox("Ambient Occlusion Enabled", &ambientOcclusion);
		ImGui::Checkbox("Material Mapping Enabled", &materialMapping);
		ImGui::Checkbox("Normal Mapping Enabled", &normalMapping);
		ImGui::Checkbox("Environment Mapping Enabled", &environmentMapping);
		ImGui::Checkbox("Depth of Field Enabled", &depthOfField);
	}

	if (ImGui::CollapsingHeader("Surface"))
	{		
		ImGui::SliderFloat("Sharpness", &sharpness, 0.5f, 16.0f);
		ImGui::SliderFloat("Dist. Blending", &distanceBlending, 0.0f, 1.0f);
		ImGui::SliderFloat("Dist. Scale", &distanceScale, 0.0f, 16.0f);
		ImGui::Combo("Coloring", &coloring, "None\0Element\0Residue\0Chain\0");
		ImGui::Checkbox("Magic Lens", &lens);
	}

	static uint environmentTextureIndex = 0;

	if (environmentMapping)
	{
		if (ImGui::CollapsingHeader("Environment Mapping"))
		{
			if (ImGui::ListBoxHeader("Environment Map"))
			{
				for (uint i = 0; i < m_environmentTextures.size(); i++)
				{
					auto& texture = m_environmentTextures[i];
					bool selected = (i == environmentTextureIndex);
					ImGui::BeginGroup();
					ImGui::PushID(i);

					if (ImGui::Selectable("", &selected, 0, ImVec2(0.0f, 32.0f)))
						environmentTextureIndex = i;

					ImGui::SameLine();
					ImGui::Image((ImTextureID)texture->id(), ImVec2(32.0f, 32.0f));
					ImGui::PopID();
					ImGui::EndGroup();
				}

				ImGui::ListBoxFooter();
			}

			ImGui::Checkbox("Use for Illumination", &environmentLighting);
		}
	}

	static uint materialTextureIndex = 0;

	if (materialMapping)
	{
		if (ImGui::CollapsingHeader("Material Mapping"))
		{
			if (ImGui::ListBoxHeader("Material Map"))
			{
				for (uint i = 0; i < m_materialTextures.size(); i++)
				{
					auto& texture = m_materialTextures[i];
					bool selected = (i == materialTextureIndex);
					ImGui::BeginGroup();
					ImGui::PushID(i);

					if (ImGui::Selectable("", &selected, 0, ImVec2(0.0f, 32.0f)))
						materialTextureIndex = i;

					ImGui::SameLine();
					ImGui::Image((ImTextureID)texture->id(), ImVec2(32.0f, 32.0f));
					ImGui::PopID();
					ImGui::EndGroup();
				}

				ImGui::ListBoxFooter();
			}
		}
	}
		
	static uint bumpTextureIndex = 0;

	if (normalMapping)
	{
		if (ImGui::CollapsingHeader("Normal Mapping"))
		{
			if (ImGui::ListBoxHeader("Normal Map"))
			{
				for (uint i = 0; i < m_bumpTextures.size(); i++)
				{
					auto& texture = m_bumpTextures[i];
					bool selected = (i == bumpTextureIndex);
					ImGui::BeginGroup();
					ImGui::PushID(i);

					if (ImGui::Selectable("", &selected, 0, ImVec2(0.0f, 32.0f)))
						bumpTextureIndex = i;

					ImGui::SameLine();
					ImGui::Image((ImTextureID)texture->id(), ImVec2(32.0f, 32.0f));
					ImGui::PopID();
					ImGui::EndGroup();
				}

				ImGui::ListBoxFooter();
			}
		}
	}
	
	if (depthOfField)
	{
		if (ImGui::CollapsingHeader("Depth of Field"))
		{
			ImGui::SliderFloat("Focal Distance", &focalDistance, 0.1f, 35.0f);
			ImGui::Combo("F-stop", &fStop_current, fStops, IM_ARRAYSIZE(fStops));

			ImGui::SliderFloat("Max. CoC Radius", &maximumCoCRadius, 1.0f, 20.0f);
			ImGui::SliderFloat("Far Radius Scale", &farRadiusRescale, 0.1f, 5.0f);

		}
	}

	fStop = std::stof(fStops[fStop_current]);
	focalLength = 1.0f / (tan(fieldOfView * 0.5f) * 2.0f);
	aparture = focalLength / fStop;

	if (ImGui::CollapsingHeader("Animation"))
	{
		ImGui::Checkbox("Prodecural Animation", &animate);
		ImGui::SliderFloat("Frequency", &animationFrequency, 1.0f, 256.0f);
		ImGui::SliderFloat("Amplitude", &animationAmplitude, 1.0f, 32.0f);
	}

	ImGui::End();

	// Scaling for sphere of influence radius based on estimated density
	const float contributingAtoms = 32.0f;
	const float radiusScale = sqrtf(log(contributingAtoms*exp(sharpness)) / sharpness);

	// Properties for animation
	const uint timestepCount = (uint)viewer()->scene()->protein()->atoms().size();
	const float animationTime = animate ? float(glfwGetTime()) : -1.0f;
	const float currentTime = glfwGetTime()*animationFrequency;
	const uint currentTimestep = uint(currentTime) % timestepCount;
	const uint nextTimestep = (currentTimestep + 1) % timestepCount;
	const float animationDelta = currentTime - floor(currentTime);
	const int vertexCount = int(viewer()->scene()->protein()->atoms()[currentTimestep].size());
	
	// Defines for enabling/disabling shader feature based on parameter setting
	std::string defines = "";

	if (animate)
		defines += "#define ANIMATION\n";

	if (lens)
		defines += "#define LENSING\n";

	if (coloring > 0)
		defines += "#define COLORING\n";

	if (ambientOcclusion)
		defines += "#define AMBIENT\n";

	if (environmentMapping)
		defines += "#define ENVIRONMENT\n";

	if (environmentMapping && environmentLighting)
		defines += "#define ENVIRONMENTLIGHTING\n";

	if (normalMapping)
		defines += "#define NORMAL\n";

	if (materialMapping)
		defines += "#define MATERIAL\n";

	if (depthOfField)
		defines += "#define DEPTHOFFIELD\n";

	// Reload shaders if settings have changed
	if (defines != m_shaderSourceDefines->string())
	{
		m_shaderSourceDefines->setString(defines);

		for (auto& s : shaderFiles())
			s->reload();
	}

	// Vertex binding setup
	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices[currentTimestep].get(), 0, sizeof(vec4));
	vertexBinding->setFormat(4, GL_FLOAT);
	m_vao->enable(0);
	
	if (timestepCount > 0)
	{
		auto nextVertexBinding = m_vao->binding(1);
		nextVertexBinding->setAttribute(1);
		nextVertexBinding->setBuffer(m_vertices[nextTimestep].get(), 0, sizeof(vec4));
		nextVertexBinding->setFormat(4, GL_FLOAT);
		m_vao->enable(1);
	}

	//////////////////////////////////////////////////////////////////////////
	// Sphere rendering pass
	//////////////////////////////////////////////////////////////////////////
	m_sphereFramebuffer->bind();
	glClearDepth(1.0f);
	glClearColor(0.0, 0.0, 0.0, 65535.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	m_programSphere->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSphere->setUniform("projectionMatrix", projectionMatrix);
	m_programSphere->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSphere->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSphere->setUniform("radiusScale", 1.0f);
	m_programSphere->setUniform("clipRadiusScale", radiusScale);
	m_programSphere->setUniform("nearPlaneZ", nearPlane.z);
	m_programSphere->setUniform("animationDelta", animationDelta);
	m_programSphere->setUniform("animationTime", animationTime);
	m_programSphere->setUniform("animationAmplitude", animationAmplitude);
	m_programSphere->setUniform("animationFrequency", animationFrequency);

	m_vao->bind();
	m_programSphere->use();
	m_vao->drawArrays(GL_POINTS, 0, vertexCount);
	m_programSphere->release();
	m_vao->unbind();


	//////////////////////////////////////////////////////////////////////////
	// List generation pass
	//////////////////////////////////////////////////////////////////////////
	const uint intersectionClearValue = 1;
	m_intersectionBuffer->clearSubData(GL_R32UI, 0, sizeof(uint), GL_RED_INTEGER, GL_UNSIGNED_INT, &intersectionClearValue);

	const uint offsetClearValue = 0;
	m_offsetTexture->clearImage(0, GL_RED_INTEGER, GL_UNSIGNED_INT, &offsetClearValue);

	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	glDepthFunc(GL_ALWAYS);
	glColorMask(GL_FALSE, GL_FALSE, GL_FALSE, GL_FALSE);
	glDepthMask(GL_FALSE);

	m_spherePositionTexture->bindActive(0);
	m_offsetTexture->bindImageTexture(0, 0, false, 0, GL_READ_WRITE, GL_R32UI);
	m_elementColorsRadii->bindBase(GL_UNIFORM_BUFFER, 0);
	m_residueColors->bindBase(GL_UNIFORM_BUFFER, 1);
	m_chainColors->bindBase(GL_UNIFORM_BUFFER, 2);
	
	m_programSpawn->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSpawn->setUniform("projectionMatrix", projectionMatrix);
	m_programSpawn->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSpawn->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSpawn->setUniform("radiusScale", radiusScale);
	m_programSpawn->setUniform("clipRadiusScale", radiusScale);
	m_programSpawn->setUniform("nearPlaneZ", nearPlane.z);
	m_programSpawn->setUniform("animationDelta", animationDelta);
	m_programSpawn->setUniform("animationTime", animationTime);
	m_programSpawn->setUniform("animationAmplitude", animationAmplitude);
	m_programSpawn->setUniform("animationFrequency", animationFrequency);

	m_vao->bind();
	m_programSpawn->use();
	m_vao->drawArrays(GL_POINTS, 0, vertexCount);
	m_programSpawn->release();
	m_vao->unbind();


	m_spherePositionTexture->unbindActive(0);
	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);
	m_offsetTexture->unbindImageTexture(0);

	m_sphereFramebuffer->unbind();
	glMemoryBarrier(GL_ALL_BARRIER_BITS);

	//////////////////////////////////////////////////////////////////////////
	// Surface intersection pass
	//////////////////////////////////////////////////////////////////////////
	m_surfaceFramebuffer->bind();
	glColorMask(GL_TRUE, GL_TRUE, GL_TRUE, GL_TRUE);
	glDepthMask(GL_TRUE);

	glClearDepth(1.0f);
	glClearColor(0.0f, 0.0f, 0.0f, 65535.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_spherePositionTexture->bindActive(0);
	m_sphereNormalTexture->bindActive(1);
	m_offsetTexture->bindActive(3);
	m_environmentTextures[environmentTextureIndex]->bindActive(4);
	m_bumpTextures[bumpTextureIndex]->bindActive(5);
	m_materialTextures[materialTextureIndex]->bindActive(6);
	m_intersectionBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 1);
	m_statisticsBuffer->bindBase(GL_SHADER_STORAGE_BUFFER, 2);

	m_programSurface->setUniform("modelViewMatrix", modelViewMatrix);
	m_programSurface->setUniform("projectionMatrix", projectionMatrix);
	m_programSurface->setUniform("modelViewProjectionMatrix", modelViewProjectionMatrix);
	m_programSurface->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programSurface->setUniform("normalMatrix", normalMatrix);
	m_programSurface->setUniform("lightPosition", vec3(worldLightPosition));
	m_programSurface->setUniform("ambientMaterial", ambientMaterial);
	m_programSurface->setUniform("diffuseMaterial", diffuseMaterial);
	m_programSurface->setUniform("specularMaterial", specularMaterial);
	m_programSurface->setUniform("shininess", shininess);
	m_programSurface->setUniform("focusPosition", focusPosition);
	m_programSurface->setUniform("positionTexture", 0);
	m_programSurface->setUniform("normalTexture", 1);
	m_programSurface->setUniform("offsetTexture", 3);
	m_programSurface->setUniform("environmentTexture", 4);
	m_programSurface->setUniform("bumpTexture", 5);
	m_programSurface->setUniform("materialTexture", 6);
	m_programSurface->setUniform("sharpness", sharpness);
	m_programSurface->setUniform("coloring", uint(coloring));
	m_programSurface->setUniform("environment", environmentMapping);
	m_programSurface->setUniform("lens", lens);
	
	m_vaoQuad->bind();
	m_programSurface->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_programSurface->release();
	m_vaoQuad->unbind();

	m_intersectionBuffer->unbind(GL_SHADER_STORAGE_BUFFER);

	m_materialTextures[materialTextureIndex]->unbindActive(6);
	m_bumpTextures[bumpTextureIndex]->unbindActive(5);
	m_environmentTextures[environmentTextureIndex]->unbindActive(4);
	m_offsetTexture->unbindActive(3);
	m_sphereNormalTexture->unbindActive(1);
	m_spherePositionTexture->unbindActive(0);

	m_chainColors->unbind(GL_UNIFORM_BUFFER);
	m_residueColors->unbind(GL_UNIFORM_BUFFER);
	m_elementColorsRadii->unbind(GL_UNIFORM_BUFFER);

	m_surfaceFramebuffer->unbind();


	//////////////////////////////////////////////////////////////////////////
	// Ambient occlusion (optional)
	//////////////////////////////////////////////////////////////////////////
	if (ambientOcclusion)
	{
		//////////////////////////////////////////////////////////////////////////
		// Ambient occlusion sampling
		//////////////////////////////////////////////////////////////////////////
		m_aoFramebuffer->bind();

		m_programAOSample->setUniform("projectionInfo", projectionInfo);
		m_programAOSample->setUniform("projectionScale", projectionScale);
		m_programAOSample->setUniform("viewLightPosition", viewLightPosition);
		m_programAOSample->setUniform("surfaceNormalTexture", 0);

		m_surfaceNormalTexture->bindActive(0);

		m_vaoQuad->bind();
		m_programAOSample->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programAOSample->release();
		m_vaoQuad->unbind();

		m_aoFramebuffer->unbind();


		//////////////////////////////////////////////////////////////////////////
		// Ambient occlusion blurring -- horizontal
		//////////////////////////////////////////////////////////////////////////
		m_aoBlurFramebuffer->bind();
		m_programAOBlur->setUniform("normalTexture", 0);
		m_programAOBlur->setUniform("ambientTexture", 1);
		m_programAOBlur->setUniform("offset", vec2(1.0f/float(viewportSize.x),0.0f));

		m_ambientTexture->bindActive(1);

		m_vaoQuad->bind();
		m_programAOBlur->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programAOBlur->release();
		m_vaoQuad->unbind();

		m_ambientTexture->unbindActive(1);
		m_aoBlurFramebuffer->unbind();


		//////////////////////////////////////////////////////////////////////////
		// Ambient occlusion blurring -- vertical
		//////////////////////////////////////////////////////////////////////////
		m_aoFramebuffer->bind();
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_programAOBlur->setUniform("offset", vec2(0.0f,1.0f / float(viewportSize.y)));

		m_blurTexture->bindActive(1);

		m_vaoQuad->bind();
		m_programAOBlur->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programAOBlur->release();
		m_vaoQuad->unbind();

		m_blurTexture->unbindActive(1);
		m_surfaceNormalTexture->unbindActive(0);

		m_aoFramebuffer->unbind();
	}

	//////////////////////////////////////////////////////////////////////////
	// Shading
	//////////////////////////////////////////////////////////////////////////
	m_shadeFramebuffer->bind();
	glDepthMask(GL_FALSE);

	m_spherePositionTexture->bindActive(0);
	m_sphereNormalTexture->bindActive(1);
	m_sphereDiffuseTexture->bindActive(2);
	m_surfacePositionTexture->bindActive(3);
	m_surfaceNormalTexture->bindActive(4);
	m_surfaceDiffuseTexture->bindActive(5);
	m_depthTexture->bindActive(6);
	m_ambientTexture->bindActive(7);
	m_materialTextures[materialTextureIndex]->bindActive(8);
	m_environmentTextures[environmentTextureIndex]->bindActive(9);

	m_programShade->setUniform("modelViewMatrix", modelViewMatrix);
	m_programShade->setUniform("projectionMatrix", projectionMatrix);
	m_programShade->setUniform("modelViewProjection", modelViewProjectionMatrix);
	m_programShade->setUniform("inverseModelViewProjectionMatrix", inverseModelViewProjectionMatrix);
	m_programShade->setUniform("normalMatrix", normalMatrix);
	m_programShade->setUniform("inverseNormalMatrix", inverseNormalMatrix);
	m_programShade->setUniform("lightPosition", vec3(worldLightPosition));
	m_programShade->setUniform("ambientMaterial", ambientMaterial);
	m_programShade->setUniform("diffuseMaterial", diffuseMaterial);
	m_programShade->setUniform("specularMaterial", specularMaterial);
	m_programShade->setUniform("distanceBlending", distanceBlending);
	m_programShade->setUniform("distanceScale", distanceScale);
	m_programShade->setUniform("shininess", shininess);
	m_programShade->setUniform("focusPosition", focusPosition);

	m_programShade->setUniform("spherePositionTexture", 0);
	m_programShade->setUniform("sphereNormalTexture", 1);
	m_programShade->setUniform("sphereDiffuseTexture", 2);

	m_programShade->setUniform("surfacePositionTexture", 3);
	m_programShade->setUniform("surfaceNormalTexture", 4);
	m_programShade->setUniform("surfaceDiffuseTexture", 5);

	m_programShade->setUniform("depthTexture", 6);
	m_programShade->setUniform("ambientTexture", 7);
	m_programShade->setUniform("materialTexture", 8);
	m_programShade->setUniform("environmentTexture", 9);

	m_programShade->setUniform("environment", environmentMapping);
	m_programShade->setUniform("maximumCoCRadius", maximumCoCRadius);
	m_programShade->setUniform("aparture", aparture);
	m_programShade->setUniform("focalDistance", focalDistance);
	m_programShade->setUniform("focalLength", focalLength);
	m_programShade->setUniform("backgroundColor", viewer()->backgroundColor());


	m_vaoQuad->bind();
	m_programShade->use();
	m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
	m_programShade->release();
	m_vaoQuad->unbind();

	m_environmentTextures[environmentTextureIndex]->unbindActive(9);
	m_materialTextures[materialTextureIndex]->unbindActive(8);
	m_ambientTexture->unbindActive(7);
	m_depthTexture->unbindActive(6);
	m_surfaceDiffuseTexture->unbindActive(5);
	m_surfaceNormalTexture->unbindActive(4);
	m_surfacePositionTexture->unbindActive(3);
	m_sphereDiffuseTexture->unbindActive(2);
	m_sphereNormalTexture->unbindActive(1);
	m_spherePositionTexture->unbindActive(0);

	m_shadeFramebuffer->unbind();


	//////////////////////////////////////////////////////////////////////////
	// Depth of field (optional)
	//////////////////////////////////////////////////////////////////////////
	if (depthOfField)
	{
		//////////////////////////////////////////////////////////////////////////
		// Depth of field blurring -- horizontal
		//////////////////////////////////////////////////////////////////////////
		m_dofBlurFramebuffer->bind();

		m_colorTexture->bindActive(0);
		m_colorTexture->bindActive(1);
		
		m_programDOFBlur->setUniform("maximumCoCRadius", maximumCoCRadius);
		m_programDOFBlur->setUniform("aparture", aparture);
		m_programDOFBlur->setUniform("focalDistance", focalDistance);
		m_programDOFBlur->setUniform("focalLength", focalLength);

		m_programDOFBlur->setUniform("uMaxCoCRadiusPixels", (int)round(maximumCoCRadius));
		m_programDOFBlur->setUniform("uNearBlurRadiusPixels", (int)round(maximumCoCRadius));
		m_programDOFBlur->setUniform("uInvNearBlurRadiusPixels", 1.0f / maximumCoCRadius);
		m_programDOFBlur->setUniform("horizontal", true);
		m_programDOFBlur->setUniform("nearTexture", 0);
		m_programDOFBlur->setUniform("blurTexture", 1);

		m_vaoQuad->bind();
		m_programDOFBlur->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programDOFBlur->release();
		m_vaoQuad->unbind();

		m_colorTexture->unbindActive(1);
		m_colorTexture->unbindActive(0);
		m_dofBlurFramebuffer->unbind();


		//////////////////////////////////////////////////////////////////////////
		// Depth of field blurring -- vertical
		//////////////////////////////////////////////////////////////////////////
		m_dofFramebuffer->bind();

		m_sphereNormalTexture->bindActive(0);
		m_surfaceNormalTexture->bindActive(1);
		m_programDOFBlur->setUniform("horizontal", false);
		m_programDOFBlur->setUniform("nearTexture", 0);
		m_programDOFBlur->setUniform("blurTexture", 1);

		m_vaoQuad->bind();
		m_programDOFBlur->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programDOFBlur->release();
		m_vaoQuad->unbind();

		m_surfaceNormalTexture->unbindActive(1);
		m_sphereNormalTexture->unbindActive(0);

		m_dofFramebuffer->unbind();


		//////////////////////////////////////////////////////////////////////////
		// Depth of field blending
		//////////////////////////////////////////////////////////////////////////
		m_shadeFramebuffer->bind();

		m_colorTexture->bindActive(0);
		m_sphereDiffuseTexture->bindActive(1);
		m_surfaceDiffuseTexture->bindActive(2);

		m_programDOFBlend->setUniform("maximumCoCRadius", maximumCoCRadius);
		m_programDOFBlend->setUniform("aparture", aparture);
		m_programDOFBlend->setUniform("focalDistance", focalDistance);
		m_programDOFBlend->setUniform("focalLength", focalLength);

		m_programDOFBlend->setUniform("colorTexture", 0);
		m_programDOFBlend->setUniform("nearTexture", 1);
		m_programDOFBlend->setUniform("blurTexture", 2);

		m_vaoQuad->bind();
		m_programDOFBlend->use();
		m_vaoQuad->drawArrays(GL_POINTS, 0, 1);
		m_programDOFBlend->release();
		m_vaoQuad->unbind();

		m_surfaceDiffuseTexture->unbindActive(1);
		m_sphereDiffuseTexture->unbindActive(0);
		m_shadeFramebuffer->unbind();
	}
	
	// Blit final image into visible framebuffer
	m_shadeFramebuffer->blit(GL_COLOR_ATTACHMENT0, {0,0,viewer()->viewportSize().x, viewer()->viewportSize().y}, Framebuffer::defaultFBO().get(), GL_BACK, { 0,0,viewer()->viewportSize().x, viewer()->viewportSize().y }, GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT, GL_NEAREST);

	// Restore OpenGL state
	currentState->apply();
}