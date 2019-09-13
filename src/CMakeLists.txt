file(GLOB_RECURSE dynamol_sources *.cpp *.h)
file(GLOB imgui_sources ${CMAKE_SOURCE_DIR}/lib/imgui/*.cpp ${CMAKE_SOURCE_DIR}/lib/imgui/*.h)
file(GLOB tinyfd_sources ${CMAKE_SOURCE_DIR}/lib/tinyfd/tinyfiledialogs.c ${CMAKE_SOURCE_DIR}/lib/tinyfd/tinyfiledialogs.h)
file(GLOB stb_sources ${CMAKE_SOURCE_DIR}/lib/stb/*.c ${CMAKE_SOURCE_DIR}/lib/stb/*.h)

add_executable(dynamol ${dynamol_sources} ${imgui_sources} ${tinyfd_sources} ${stb_sources})

list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glm)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glbinding)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/globjects)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/glfw)
list(APPEND CMAKE_PREFIX_PATH ${CMAKE_SOURCE_DIR}/lib/tinyfd)

find_package(glm REQUIRED)
find_package(glbinding REQUIRED)
find_package(globjects REQUIRED)
find_package(glfw3 REQUIRED)

include_directories(${CMAKE_SOURCE_DIR}/lib/imgui/)
include_directories(${CMAKE_SOURCE_DIR}/lib/tinyfd/)
include_directories(${CMAKE_SOURCE_DIR}/lib/stb/)

target_link_libraries(dynamol PUBLIC glfw)
target_link_libraries(dynamol PUBLIC glbinding::glbinding )
target_link_libraries(dynamol PUBLIC glbinding::glbinding-aux )
target_link_libraries(dynamol PUBLIC globjects::globjects)

set_target_properties(dynamol PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY ${CMAKE_SOURCE_DIR})
