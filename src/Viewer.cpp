#include "Viewer.h"

#include <glbinding/gl/gl.h>
#include <iostream>

#ifdef _WIN32
#define GLFW_EXPOSE_NATIVE_WIN32
#include <glfw/glfw3native.h>
#endif

#include <glm/gtc/matrix_transform.hpp>

#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/matrix_decompose.hpp>

#include "CameraInteractor.h"
#include "BoundingBoxRenderer.h"
#include "SphereRenderer.h"
#include "Scene.h"
#include "Protein.h"
#include <fstream>
#include <sstream>
#include <list>

#define STB_IMAGE_WRITE_IMPLEMENTATION
#include <stb_image_write.h>

using namespace dynamol;
using namespace gl;
using namespace glm;
using namespace globjects;

#define IMGUI_IMPL_OPENGL_LOADER_CUSTOM <glbinding/gl/gl.h>
#include <examples/imgui_impl_opengl3.cpp>
#include <examples/imgui_impl_glfw.cpp>


Viewer::Viewer(GLFWwindow *window, Scene *scene) : m_window(window), m_scene(scene)
{
	ImGui::CreateContext();
	ImGuiIO& io = ImGui::GetIO();

	glfwSetWindowUserPointer(window, static_cast<void*>(this));
	glfwSetFramebufferSizeCallback(window, &Viewer::framebufferSizeCallback);
	glfwSetKeyCallback(window, &Viewer::keyCallback);
	glfwSetMouseButtonCallback(window, &Viewer::mouseButtonCallback);
	glfwSetCursorPosCallback(window, &Viewer::cursorPosCallback);
	glfwSetScrollCallback(window, &Viewer::scrollCallback);

	ImGui_ImplGlfw_InitForOpenGL(window, true);
	ImGui_ImplOpenGL3_Init();
	io.Fonts->AddFontFromFileTTF("./res/ui/Lato-Semibold.ttf", 18);

	m_interactors.emplace_back(std::make_unique<CameraInteractor>(this));
	m_renderers.emplace_back(std::make_unique<SphereRenderer>(this));
	m_renderers.emplace_back(std::make_unique<BoundingBoxRenderer>(this));

	int i = 1;

	globjects::debug() << "Available renderers (use the number keys to toggle):";

	for (auto& r : m_renderers)
	{
		globjects::debug() << "  " << i << " - " << typeid(*r.get()).name();
		++i;
	}
}

void Viewer::display()
{
	beginFrame();
	mainMenu();

	glClearColor(m_backgroundColor.r, m_backgroundColor.g, m_backgroundColor.b, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glViewport(0, 0, viewportSize().x, viewportSize().y);


	for (auto& r : m_renderers)
	{
		if (r->isEnabled())
		{		
			r->display();
		}
	}
	
	for (auto& i : m_interactors)
	{
		i->display();
	}

	endFrame();
}

GLFWwindow * Viewer::window()
{
	return m_window;
}

Scene* Viewer::scene()
{
	return m_scene;
}

ivec2 Viewer::viewportSize() const
{
	int width, height;
	glfwGetFramebufferSize(m_window, &width, &height);
	return ivec2(width,height);
}

glm::vec3 Viewer::backgroundColor() const
{
	return m_backgroundColor;
}

mat4 Viewer::modelTransform() const
{
	return m_modelTransform;
}

mat4 Viewer::viewTransform() const
{
	return m_viewTransform;
}

void Viewer::setModelTransform(const glm::mat4& m)
{
	m_modelTransform = m;
}

void dynamol::Viewer::setBackgroundColor(const glm::vec3 & c)
{
	m_backgroundColor = c;
}

void Viewer::setViewTransform(const glm::mat4& m)
{
	m_viewTransform = m;
}

void Viewer::setProjectionTransform(const glm::mat4& m)
{
	m_projectionTransform = m;
}

void Viewer::setLightTransform(const glm::mat4& m)
{
	m_lightTransform = m;
}

mat4 Viewer::projectionTransform() const
{
	return m_projectionTransform;
}

mat4 Viewer::lightTransform() const
{
	return m_lightTransform;
}

mat4 Viewer::modelViewTransform() const
{
	return viewTransform()*modelTransform();
}

mat4 Viewer::modelViewProjectionTransform() const
{
	return projectionTransform()*modelViewTransform();
}

mat4 Viewer::modelLightTransform() const
{
	return lightTransform()*modelTransform();
}

mat4 Viewer::modelLightProjectionTransform() const
{
	return projectionTransform()*modelLightTransform();
}

void Viewer::saveImage(const std::string & filename)
{
	uvec2 size = viewportSize();
	std::vector<unsigned char> image(size.x*size.y * 4);

	glReadPixels(0, 0, size.x, size.y, GL_RGBA, GL_UNSIGNED_BYTE, (void*)&image.front());

	stbi_flip_vertically_on_write(true);
	stbi_write_png(filename.c_str(), size.x, size.y, 4, &image.front(), size.x * 4);
}

void Viewer::framebufferSizeCallback(GLFWwindow* window, int width, int height)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		for (auto& i : viewer->m_interactors)
		{
			i->framebufferSizeEvent(width, height);
		}
	}
}

void Viewer::keyCallback(GLFWwindow* window, int key, int scancode, int action, int mods)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureKeyboard)
				return;
		}

		if (key == GLFW_KEY_SPACE && action == GLFW_RELEASE)
		{
			viewer->m_showUi = !viewer->m_showUi;
		}

		if (key == GLFW_KEY_F5 && action == GLFW_RELEASE)
		{
			for (auto& r : viewer->m_renderers)
			{
				std::cout << "Reloading shaders for instance of " << typeid(*r.get()).name() << " ... " << std::endl;

				for (auto& s : r->shaderFiles())
				{
					std::cout << "  " << s->shortInfo() << std::endl;
					s->reload();
				}
				std::cout << r->shaderFiles().size() << " shaders reloaded." << std::endl << std::endl;
			}
		}
		else if (key == GLFW_KEY_F2 && action == GLFW_RELEASE)
		{
			viewer->m_saveScreenshot = true;
		}
		else if (key >= GLFW_KEY_1 && key <= GLFW_KEY_9 && action == GLFW_RELEASE)		
		{
			int index = key - GLFW_KEY_1;

			if (index < viewer->m_renderers.size())
			{
				bool enabled = viewer->m_renderers[index]->isEnabled();

				if (enabled)
					std::cout << "Renderer " << index + 1 << " of type " << typeid(*viewer->m_renderers[index].get()).name() << " is now disabled." << std::endl << std::endl;
				else
					std::cout << "Renderer " << index + 1 << " of type " << typeid(*viewer->m_renderers[index].get()).name() << " is now enabled." << std::endl << std::endl;

				viewer->m_renderers[index]->setEnabled(!enabled);
			}
		}

		for (auto& i : viewer->m_interactors)
		{
			i->keyEvent(key, scancode, action, mods);
		}
	}
}

void Viewer::mouseButtonCallback(GLFWwindow* window, int button, int action, int mods)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureMouse)
				return;
		}

		for (auto& i : viewer->m_interactors)
		{
			i->mouseButtonEvent(button, action, mods);
		}
	}
}

void Viewer::cursorPosCallback(GLFWwindow* window, double xpos, double ypos)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureMouse)
				return;
		}

		for (auto& i : viewer->m_interactors)
		{
			i->cursorPosEvent(xpos, ypos);
		}
	}
}

void Viewer::scrollCallback(GLFWwindow* window, double xoffset, double yoffset)
{
	Viewer* viewer = static_cast<Viewer*>(glfwGetWindowUserPointer(window));

	if (viewer)
	{
		if (viewer->m_showUi)
		{
			ImGuiIO& io = ImGui::GetIO();

			if (io.WantCaptureMouse)
				return;
		}

		for (auto& i : viewer->m_interactors)
		{
			i->scrollEvent(xoffset, yoffset);
		}
	}
}

void Viewer::beginFrame()
{
	ImGui_ImplOpenGL3_NewFrame();
	ImGui_ImplGlfw_NewFrame();

	// Start the frame. This call will update the io.WantCaptureMouse, io.WantCaptureKeyboard flag that you can use to dispatch inputs (or not) to your application.
	ImGui::NewFrame();

	ImGui::BeginMainMenuBar();
}

void Viewer::endFrame()
{
	static std::list<float> frameratesList;
	frameratesList.push_back(ImGui::GetIO().Framerate);

	while (frameratesList.size() > 64)
		frameratesList.pop_front();

	static float framerates[64];
	int i = 0;
	for (auto v : frameratesList)
		framerates[i++] = v;

	std::stringstream stream;
	stream << std::fixed << std::setprecision(2) << ImGui::GetIO().Framerate << " fps";
	std::string s = stream.str();

	//		ImGui::Begin("Information");
	ImGui::SameLine(ImGui::GetWindowWidth() - 220.0f);
	ImGui::PlotLines(s.c_str(), framerates, int(frameratesList.size()), 0, 0, 0.0f, 200.0f,ImVec2(128.0f,0.0f));
	//		ImGui::End();

	ImGui::EndMainMenuBar();

	if (m_saveScreenshot)
	{
		std::string basename = scene()->protein()->filename();
		size_t pos = basename.rfind('.', basename.length());

		if (pos != std::string::npos)
			basename = basename.substr(0,pos);

		uint i = 0;
		std::string filename;

		for (uint i = 0; i <= 9999; i++)
		{
			std::stringstream ss;
			ss << basename << "-";
			ss << std::setw(4) << std::setfill('0') << i;
			ss << ".png";

			filename = ss.str();

			std::ifstream f(filename.c_str());
			
			if (!f.good())
				break;
		}

		std::cout << "Saving screenshot to " << filename << " ..." << std::endl;

		saveImage(filename);
		m_saveScreenshot = false;
	}

	if (m_showUi)
		renderUi();
}

void Viewer::renderUi()
{
	ImGui::Render();

	ImGuiIO& io = ImGui::GetIO();
	ImDrawData* draw_data = ImGui::GetDrawData();
	ImGui_ImplOpenGL3_RenderDrawData(draw_data);
}

void Viewer::mainMenu()
{
	if (ImGui::BeginMenu("File"))
	{
		if (ImGui::MenuItem("Screenshot", "F2"))
			m_saveScreenshot = true;

		if (ImGui::MenuItem("Exit", "Alt+F4"))
			glfwSetWindowShouldClose(m_window, GLFW_TRUE);

		ImGui::EndMenu();
	}

	if (ImGui::BeginMenu("Settings"))
	{
		ImGui::ColorEdit3("Background", (float*)&m_backgroundColor);

		if (ImGui::BeginMenu("Viewport"))
		{
			if (ImGui::MenuItem("512 x 512"))
				glfwSetWindowSize(m_window, 512, 512);

			if (ImGui::MenuItem("768 x 768"))
				glfwSetWindowSize(m_window, 768, 768);

			if (ImGui::MenuItem("1024 x 1024"))
				glfwSetWindowSize(m_window, 1024, 1024);

			if (ImGui::MenuItem("1280 x 1280"))
				glfwSetWindowSize(m_window, 1280, 1280);

			if (ImGui::MenuItem("1280 x 720"))
				glfwSetWindowSize(m_window, 1280, 720);

			if (ImGui::MenuItem("1920 x 1080"))
				glfwSetWindowSize(m_window, 1920, 1080);

			ImGui::EndMenu();
		}

		ImGui::EndMenu();
	}
}
