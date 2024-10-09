#include "BoundingBoxRenderer.h"
#include <globjects/base/File.h>
#include <iostream>
#include "Viewer.h"
#include "Scene.h"
#include "Protein.h"

#include <glm/glm.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtc/matrix_transform.hpp>


#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/string_cast.hpp>
#include <glm/gtx/transform.hpp>

using namespace dynamol;
using namespace gl;
using namespace glm;
using namespace globjects;

BoundingBoxRenderer::BoundingBoxRenderer(Viewer* viewer) : Renderer(viewer)
{
	static std::array<vec3, 8> vertices{ {
			// front
			{-1.0, -1.0, 1.0 },
			{ 1.0, -1.0, 1.0 },
			{ 1.0, 1.0, 1.0 },
			{ -1.0, 1.0, 1.0 },
			// back
			{ -1.0, -1.0, -1.0 },
			{ 1.0, -1.0, -1.0 },
			{ 1.0, 1.0, -1.0 },
			{ -1.0, 1.0, -1.0 }
		} };


	static std::array< std::array<GLushort, 4>, 6> indices{ {
			// front
			{0,1,2,3},
			// top
			{1,5,6,2},
			// back
			{7,6,5,4},
			// bottom
			{4,0,3,7},
			// left
			{4,5,1,0},
			// right
			{3,2,6,7}
		} };

	m_indices->setData(indices, GL_STATIC_DRAW);
	m_vertices->setData(vertices, GL_STATIC_DRAW);

	m_size = static_cast<GLsizei>(indices.size() * indices.front().size());
	m_vao->bindElementBuffer(m_indices.get());

	auto vertexBinding = m_vao->binding(0);
	vertexBinding->setAttribute(0);
	vertexBinding->setBuffer(m_vertices.get(), 0, sizeof(vec3));
	vertexBinding->setFormat(3, GL_FLOAT);
	m_vao->enable(0);

	m_vao->unbind();

	createShaderProgram("boundingbox", {
		{ GL_VERTEX_SHADER,"./res/boundingbox/boundingbox-vs.glsl" },
		{ GL_TESS_CONTROL_SHADER, "./res/boundingbox/boundingbox-tcs.glsl" },
		{ GL_TESS_EVALUATION_SHADER, "./res/boundingbox/boundingbox-tes.glsl"},
		{ GL_GEOMETRY_SHADER,"./res/boundingbox/boundingbox-gs.glsl" },
		{ GL_FRAGMENT_SHADER,"./res/boundingbox/boundingbox-fs.glsl" },
		});
}

void BoundingBoxRenderer::display()
{
	auto currentState = State::currentState();
	glViewport(viewer()->viewportOrigin().x, viewer()->viewportOrigin().y, viewer()->viewportSize().x, viewer()->viewportSize().y);

	glEnable(GL_DEPTH_TEST);
	glDepthFunc(GL_LESS);

	glEnable(GL_SAMPLE_ALPHA_TO_COVERAGE);
	glEnable(GL_BLEND);
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	mat4 boundingBoxTransform;
	boundingBoxTransform = scale(0.5f * (viewer()->scene()->protein()->maximumBounds() - viewer()->scene()->protein()->minimumBounds()));
	boundingBoxTransform = translate(0.5f * (viewer()->scene()->protein()->maximumBounds() + viewer()->scene()->protein()->minimumBounds())) * boundingBoxTransform;

	mat4 modelViewTransform = viewer()->modelViewTransform() * boundingBoxTransform;

	static vec3 lineColor = vec3(0.5f, 0.5f, 0.5f);

	if (ImGui::BeginMenu("Bounding Box"))
	{
		ImGui::ColorEdit3("Line Color", (float*)&lineColor);
		ImGui::EndMenu();
	}

	auto program = shaderProgram("boundingbox");
	program->setUniform("projection", viewer()->projectionTransform());
	program->setUniform("modelView", modelViewTransform);
	program->setUniform("lineColor", lineColor);

	m_vao->bind();
	glPatchParameteri(GL_PATCH_VERTICES, 4);

	program->use();
	m_vao->drawElements(GL_PATCHES, m_size, GL_UNSIGNED_SHORT, nullptr);
	program->release();

	m_vao->unbind();

	glDisable(GL_BLEND);
	glDisable(GL_SAMPLE_ALPHA_TO_COVERAGE);

	currentState->apply();
}