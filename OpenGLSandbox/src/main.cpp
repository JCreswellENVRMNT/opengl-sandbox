#include <iostream>
#include "glad/glad.h"
#include <GLFW/glfw3.h>
#include <sstream>
#include <fstream>
#include <assert.h>

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

/**
 * Loads the shader source from the given filename and compiles it
 * @param shaderName the base of the shader filename, e.g. basic_triangle; the .vert and .frag
 * etc. extension will derive from the given shader type
 * @param shaderType the type of shader e.g. ShaderType::vertex
 * @return the generated shaderId (gt 0) iff compilation succeeded, else 0
 */
unsigned int loadShader(const std::string& shaderName, ShaderType shaderType)
{
    std::string shaderPath = "../assets/shaders/"+shaderName;
    std::string shaderSource;
    unsigned int shaderId = 0;
    if(readFile(shaderPath, shaderSource))
    {
        // use shader source to compile and bind shader
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
            return 0;
        }
    }
    else
    {
        std::cerr << "failed loading shader source file: " << shaderPath << std::endl;
        return 0;
    }
    return shaderId;
}

/**
 * Creates a new shader program and adds a vertex and fragment shader for the named program into it, e.g. basic_triangle
 * load basic_triangle.vert as vertex shader and basic_triangle.frag as fragment shader.
 * @param programName the name of the full effect we want to generate with combined vertex and fragment shaders
 * @return non-zero shader program ID if both vertex and fragment shaders loaded/compiled successfully
 * and the program linked successfully, else 0
 */
unsigned int loadShaders(const std::string& programName)
{
    bool successStatus = true;
    unsigned int shaderProgramId;
    shaderProgramId = glCreateProgram();
    // todo: I'd really like to see a more automated approach to this, iterating through
    //  an input array of shader types we're interested in or something, and storing the result
    //  shader IDs in an array of the same size that we can then glDeleteShader over during cleanup
    unsigned int vertexShaderId;
    unsigned int fragmentShaderId;
    // compile and attach shaders
    vertexShaderId = loadShader(programName+".vert", ShaderType::vertex);
    if(!vertexShaderId)
    {
        std::cerr << "error occurred compiling " << programName << ".vert and we cannot proceed" << std::endl;
        return 0;
    }
    glAttachShader(shaderProgramId, vertexShaderId);
    fragmentShaderId = loadShader(programName+".frag", ShaderType::fragment);
    if(!fragmentShaderId)
    {
        std::cerr << "error occurred compiling " << programName << ".frag and we cannot proceed" << std::endl;
        return 0;
    }
    glAttachShader(shaderProgramId, fragmentShaderId);

    // link the assembled program
    glLinkProgram(shaderProgramId);

    // cleanup resources
    glDeleteShader(vertexShaderId);
    glDeleteShader(fragmentShaderId);

    // check link success status
    int linkSuccessStatus;
    char infoLog[512];
    glGetProgramiv(shaderProgramId, GL_LINK_STATUS, &linkSuccessStatus);
    if(!linkSuccessStatus) {
        glGetProgramInfoLog(shaderProgramId, 512, nullptr, infoLog);
        std::cerr << "error linking " << programName << ":\n" << infoLog << std::endl;
        return 0;
    }

    return shaderProgramId;
}

int main()
{
    // todo: add Google Test unit test support; it would be great if we
    //  just called runTests() or something from within the renderloop and then whatever unit tests
    //  were registered would run.  Tough to meaningfully automate validation, but something's better than nothing.
    //  Can also use this to make sure new shaders load up, compile, and link properly.

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

    // create our program object and load vertex and fragment shaders into it
    std::string shaderProgramName = "basic_triangle";
    unsigned int shaderProgramId = loadShaders(shaderProgramName);
    assert(shaderProgramId > 0);

    // render loop
    while(!glfwWindowShouldClose(window))
    {
        // handle any user input this frame
        processInput(window);

        // check and call events and swap the buffers
        glfwPollEvents();

        // rendering code
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);

        // todo: not all of this needs to be/should be in the render loop
        // Step 0: create vertex array object to track our config
        unsigned int VAO;
        glGenVertexArrays(1, &VAO);
        glBindVertexArray(VAO);

        // Step 1: define and buffer data
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

        // Step 2: configure vertex attribute pointers to tell OpenGL how to interpret buffered data
        // 0 is the location we specified for our aPos attribute in basic_triangle.vert
        glVertexAttribPointer(
                0,
                3,
                GL_FLOAT,
                GL_FALSE,
                3 * sizeof(float),
                (void*)nullptr
        );
        glEnableVertexAttribArray(0);

        // Step 3: select shader program to use
        glUseProgram(shaderProgramId);

        // Step 4: bind the configured VAO
        glBindVertexArray(VAO);

        // Step 5: draw calls
        // specify primitve type triangles and that we want to render
        // vertices starting at index 0 and rendering a total count of 3 vertices
        glDrawArrays(GL_TRIANGLES, 0, 3);

        // render the back buffer to the window
        glfwSwapBuffers(window);
    }

    // free GLFW resources
    glfwTerminate();
    return 0;
}
