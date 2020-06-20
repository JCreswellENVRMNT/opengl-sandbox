#include <iostream>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <sstream>
#include <fstream>

enum ShaderType
{
    vertex,
    fragment
};

/**
 * Callback function for window resize events; we'll tell OpenGL that we need a
 * new framebuffer to accommodate the new width x height
 * @param window the GLFW window object that has been resized
 * @param width new width dimen value
 * @param height new height dimen value
 */
void framebuffer_size_callback(GLFWwindow* window, int width, int height)
{
    glViewport(0, 0, width, height);
}

/**
 * Callback handler for user input
 * @param window GLFW window receiving input
 */
void processInput(GLFWwindow *window)
{
    if(glfwGetKey(window, GLFW_KEY_ESCAPE) == GLFW_PRESS)
    {
        glfwSetWindowShouldClose(window, true);
    }
}

/**
 * Reads the named file in as a string, which is copied to outputString
 * @param fileName file to read
 * @param outputString string in which to store the file contents
 * @return true if reading file succeeded, false if exception is thrown
 */
bool readFile(const std::string& fileName, std::string& outputString)
{
    try
    {
        std::ifstream inputStream(fileName);
        std::stringstream buffer;
        buffer << inputStream.rdbuf();
        outputString = buffer.str();
        return true;
    }
    catch (std::exception& exception)
    {
        std::cerr << "exception occurred reading in " << fileName << ": " << exception.what() << std::endl;
    }
    return false;
}

bool loadShader(const std::string& shaderName, ShaderType shaderType)
{
    std::string shaderPath = "../assets/shaders/"+shaderName;
    std::string shaderSource;
    if(readFile(shaderPath, shaderSource))
    {
        // todo: use shader source to compile and bind shader
        unsigned int shaderId = 0;
        if (shaderType == ShaderType::vertex)
        {
            shaderId = glCreateShader(GL_VERTEX_SHADER);
        }
        else if (shaderType == ShaderType::fragment)
        {
            shaderId = glCreateShader(GL_FRAGMENT_SHADER);
        }
        const char *shaderSourceCString = shaderSource.c_str();
        glShaderSource(shaderId, 1, &shaderSourceCString, nullptr);
        glCompileShader(shaderId);
        int compileSuccessStatus;
        char infoLog[512];
        glGetShaderiv(shaderId, GL_COMPILE_STATUS, &compileSuccessStatus);
        if (!compileSuccessStatus)
        {
            glGetShaderInfoLog(shaderId, 512, nullptr, infoLog);
            std::cerr << "shader " << shaderName << " compilation failed:\n" << infoLog << std::endl;
            return false;
        }
    }
    else
    {
        std::cerr << "failed loading shader source file: " << shaderPath << std::endl;
        return false;
    }
    return true;
}

int main()
{
    // config GLFW
    glfwInit();
    glfwWindowHint(GLFW_CONTEXT_VERSION_MAJOR, 4);
    glfwWindowHint(GLFW_CONTEXT_VERSION_MINOR, 6);
    glfwWindowHint(GLFW_OPENGL_PROFILE, GLFW_OPENGL_CORE_PROFILE);

    // create GLFW window and make it the current GL context
    GLFWwindow* window = glfwCreateWindow(800, 600, "OpenGL Sandbox", nullptr, nullptr);
    if (window == nullptr)
    {
        std::cout << "Failed to create GLFW window" << std::endl;
        glfwTerminate();
        return -1;
    }
    else
    {
        std::cout << "Successfully created GLFW Window" << std::endl;
    }
    glfwMakeContextCurrent(window);

    // load in GL function addresses
    if (!gladLoadGLLoader((GLADloadproc)glfwGetProcAddress))
    {
        std::cout << "Failed to initialize GLAD" << std::endl;
        return -1;
    }

    // tell OpenGL where to place data for the window and what size its dimensions will be
    glViewport(0, 0, 800, 600);

    // set GLFW callback for window resize events
    glfwSetFramebufferSizeCallback(window, framebuffer_size_callback);

    /*
    // todo: replace with proper moogle test unit test; it would be great if we
    //  just called runTests() or something from within the renderloop and then whatever unit tests
    //  were registered would run.  Tough to meaningfully automate validation, but eh.
    std::string shaderSource;
    readFile("../assets/shaders/basic_triangle.vert", shaderSource);
    std::cout << "the shaderSource says:\n" << shaderSource << std::endl;
    */

    loadShader("basic_triangle.vert", ShaderType::vertex);

    // render loop
    while(!glfwWindowShouldClose(window))
    {
        // handle any user input this frame
        processInput(window);

        // check and call events and swap the buffers
        glfwPollEvents();

        // todo: rendering code
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // raw tri data, using device coords directly
        float vertices[] = {
                -0.5f, -0.5f, 0.0f,
                0.5f, -0.5f, 0.0f,
                0.0f,  0.5f, 0.0f
        };

        // generate a vertex buffer object to manage our vertices in GPU memory
        unsigned int VBO;
        glGenBuffers(1, &VBO);

        // bind our manager VBO to the appropriate type of GPU buffer,
        // which for vertex buffer is GL_ARRAY_BUFFER
        glBindBuffer(GL_ARRAY_BUFFER, VBO);

        // upload vertex data to the GPU memory buffer we're working with,
        // specifying its size in bytes, the data itself as float array, and
        // finally a constant indicating how often we expect drawable data to change;
        // since we're rendering a static triangle for now, static is fine.
        glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

        // render the back buffer to the window
        glfwSwapBuffers(window);
    }

    // free GLFW resources
    glfwTerminate();
    return 0;
}
