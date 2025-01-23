cmake_minimum_required(VERSION 3.13.0...3.19.3)
project(tnm061-lab1 VERSION 1.0.0 DESCRIPTION "TNM061 Lab 1 GL-Stack" LANGUAGES CXX)
set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_EXTENSIONS OFF)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

function(enable_warnings target)
target_compile_options(${target} PUBLIC 
$<$<CXX_COMPILER_ID:MSVC>:/W4 /permissive->
$<$<CXX_COMPILER_ID:AppleClang>:-Wall -Wextra -Wconversion>
$<$<CXX_COMPILER_ID:Clang>:-Wall -Wextra -Wconversion>
$<$<CXX_COMPILER_ID:GNU>:-Wall -Wextra -Wconversion>
)
endfunction()

find_package(OpenGL REQUIRED)

set(GLFW_BUILD_DOCS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_TESTS OFF CACHE BOOL "" FORCE)
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "" FORCE)

add_subdirectory(glfw-3.3.2)

set(HEADER_FILES
	util/TriangleSoup.hpp
	util/MatrixStack.hpp
	util/Rotator.hpp
	util/Shader.hpp
	util/Texture.hpp
	util/tnm061.hpp
)

set(SOURCE_FILES
	glstack.cpp
	util/MatrixStack.cpp
	util/Rotator.cpp
	util/Shader.cpp
	util/Texture.cpp
	util/tnm061.cpp
	util/TriangleSoup.cpp
)

add_executable(tnm061-lab1 ${SOURCE_FILES} ${HEADER_FILES})
enable_warnings(tnm061-lab1)

if(MSVC AND TARGET tnm061-lab1)
	set_property(DIRECTORY PROPERTY VS_STARTUP_PROJECT tnm061-lab1)
	set_property(TARGET tnm061-lab1 PROPERTY VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}")
elseif(CMAKE_GENERATOR MATCHES "Xcode"
	AND ${CMAKE_VERSION} VERSION_GREATER_EQUAL "3.15.0"
	AND TARGET tnm061-lab1)
	set_target_properties(tnm061-lab1 PROPERTIES
		XCODE_GENERATE_SCHEME TRUE
		XCODE_SCHEME_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}"
	)
endif()

target_compile_definitions(tnm061-lab1 PRIVATE $<$<CXX_COMPILER_ID:MSVC>:_CRT_SECURE_NO_WARNINGS>)

target_link_libraries(tnm061-lab1 PRIVATE OpenGL::GL glfw)

option(TNM061_USE_EXTERNAL_GLEW "GLEW is provided externaly" OFF)
# Set CMake to prefere Vendor gl libraries rather than legacy, fixes warning on some unix systems
if(NOT TNM061_USE_EXTERNAL_GLEW)
	set(OpenGL_GL_PREFERENCE GLVND) 
	add_subdirectory(glew)
	target_link_libraries(tnm061-lab1 PUBLIC tnm061::GLEW)
else()
	find_package(GLEW REQUIRED)
	target_link_libraries(tnm061-lab1 PUBLIC GLEW::GLEW)
endif()
