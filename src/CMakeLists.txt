file(GLOB_RECURSE dynamol_sources *.cpp *.h)
add_executable(dynamol ${dynamol_sources})
set_target_properties(dynamol PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})

find_package(glm CONFIG REQUIRED)
target_link_libraries(dynamol PRIVATE glm::glm)

find_package(glfw3 CONFIG REQUIRED)
target_link_libraries(dynamol PRIVATE glfw)

find_package(glbinding CONFIG REQUIRED)
target_link_libraries(dynamol PRIVATE glbinding::glbinding glbinding::glbinding-aux)

find_package(globjects CONFIG REQUIRED)
target_link_libraries(dynamol PRIVATE globjects::globjects)

find_package(imgui CONFIG REQUIRED)
target_link_libraries(dynamol PRIVATE imgui::imgui)

find_package(tinyfiledialogs CONFIG REQUIRED)
target_link_libraries(dynamol PRIVATE tinyfiledialogs::tinyfiledialogs)

find_path(STB_INCLUDE_DIRS "stb.h")
target_include_directories(dynamol PRIVATE ${STB_INCLUDE_DIRS})
