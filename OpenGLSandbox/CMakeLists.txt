cmake_minimum_required(VERSION 3.16)
project(OpenGLSandbox)

set(CMAKE_CXX_STANDARD 14)
find_package(OpenGL REQUIRED)
message(STATUS "opengl lib given as ${OPENGL_LIBRARY}")
if("${GLFW_PATH}" STREQUAL "")
    message(FATAL_ERROR "GLFW path is undefined")
else()
    message(STATUS "GLFW path is ${GLFW_PATH}")
endif()
include_directories("${CMAKE_CURRENT_SOURCE_DIR}/include/")
include_directories(${GLFW_PATH}/include/)
add_executable(
        OpenGLSandbox
        src/main.cpp
        src/glad/glad.c
)
add_library(glfw SHARED IMPORTED)
set_target_properties(glfw PROPERTIES IMPORTED_LOCATION ${GLFW_PATH}/lib/${CMAKE_SYSTEM_PROCESSOR}/libglfw.so)
message(STATUS "the glfw lib location is understood to be ${GLFW_PATH}/lib/${CMAKE_SYSTEM_PROCESSOR}/libglfw.so")
target_link_libraries(
        # target we're linking things to
        OpenGLSandbox
        PRIVATE
        # libs we're linking in
        dl # needed by glad
        OpenGL
        glfw
)