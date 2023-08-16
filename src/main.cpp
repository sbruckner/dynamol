#include <iostream>

#include <glbinding/Version.h>
#include <glbinding/Binding.h>
#include <glbinding/FunctionCall.h>
#include <glbinding/CallbackMask.h>

#include <glbinding-aux/ContextInfo.h>
#include <glbinding-aux/Meta.h>
#include <glbinding-aux/types_to_string.h>
#include <glbinding-aux/ValidVersions.h>

#define GLFW_INCLUDE_NONE
#include <GLFW/glfw3.h>

#include <glm/glm.hpp>
#define GLM_ENABLE_EXPERIMENTAL
#include <glm/gtx/transform.hpp>

#include <globjects/globjects.h>
#include <globjects/logging.h>
#include <tinyfiledialogs.h>

#include "Scene.h"
#include "Protein.h"
#include "Viewer.h"
#include "Interactor.h"
#include "Renderer.h"

using namespace gl;
using namespace glm;
using namespace globjects;
using namespace dynamol;

void error_callback(int errnum, const char * errmsg)
{
	globjects::critical() << errnum << ": " << errmsg << std::endl;
}

int main(int argc, char *argv[])
{
	// Initialize GLFW
	if (!glfwInit())
		return 1;

	glfwSetErrorCallback(error_callback);

	glfwDefaultWindowHints();
	glfwWindowHint(GLFW_SCALE_TO_MONITOR, GLFW_TRUE);
	glfwWindowHint(GLFW_COCOA_RETINA_FRAMEBUFFER, GLFW_FALSE);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
	glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 5);
	glfwWindowHint(GLFW_OPENGL_FORWARD_COMPAT, true);
	glfwWindowHint(GLFW_DOUBLEBUFFER, true);
	glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_COMPAT_PROFILE);
	glfwWindowHint(GLFW_SAMPLES, 8);

	// Create a context and, if valid, make it current
	GLFWwindow * window = glfwCreateWindow(1280, 720, "dynamol", NULL, NULL);

	if (window == nullptr)
	{
		globjects::critical() << "Context creation failed - terminating execution.";

		glfwTerminate();
		return 1;
	}

	// Make context current
	glfwMakeContextCurrent(window);

	// Initialize globjects (internally initializes glbinding, and registers the current context)
	globjects::init([](const char * name) {
		return glfwGetProcAddress(name);
	});

	// Enable debug logging
	globjects::DebugMessage::enable();
	
	globjects::debug()
		<< "OpenGL Version:  " << glbinding::aux::ContextInfo::version() << std::endl
		<< "OpenGL Vendor:   " << glbinding::aux::ContextInfo::vendor() << std::endl
		<< "OpenGL Renderer: " << glbinding::aux::ContextInfo::renderer() << std::endl;

	std::string fileName = "./dat/6b0x.pdb";

	if (argc > 1)
		fileName = std::string(argv[1]);
	else
	{
		const char *filterExtensions[] = { "*.pdb" };
		const char *openfileName = tinyfd_openFileDialog("Open File", "./", 1, filterExtensions, "Protein Data Bank Files (*.pdb)", 0);

		if (openfileName)
			fileName = std::string(openfileName);
	}
	
	auto scene = std::make_unique<Scene>();
	scene->protein()->load(fileName);
	auto viewer = std::make_unique<Viewer>(window, scene.get());

	// Scaling the model's bounding box to the canonical view volume
	vec3 boundingBoxSize = scene->protein()->maximumBounds() - scene->protein()->minimumBounds();
	float maximumSize = std::max( boundingBoxSize.x, std::max(boundingBoxSize.y, boundingBoxSize.z) );
	mat4 modelTransform =  scale(vec3(2.0f) / vec3(maximumSize)); 
	modelTransform = modelTransform * translate(-0.5f*(scene->protein()->minimumBounds() + scene->protein()->maximumBounds()));
	viewer->setModelTransform(modelTransform);


	glfwSwapInterval(0);

	// Main loop
	while (!glfwWindowShouldClose(window))
	{
		glfwPollEvents();
		viewer->display();
		//glFinish();
		glfwSwapBuffers(window);
	}

	// Destroy window
	glfwDestroyWindow(window);

	// Properly shutdown GLFW
	glfwTerminate();

	return 0;
}
