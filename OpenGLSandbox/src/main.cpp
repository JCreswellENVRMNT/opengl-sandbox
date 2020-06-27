#include <iostream>
#include "glad/glad.h"
#include "RibbonTrail.h"
#include <GLFW/glfw3.h>
#include <sstream>
#include <fstream>
#include <cassert>
#include <functional>
#include <thread>
#include <glm/glm.hpp>

enum ShaderType
{
    vertex,
    fragment
};

/**
 * The maximum supported number of draw elements, after which we should reset to init
 */
GLsizei  g_maxDrawElements = 0;
/**
 * The initial supported number of draw elements
 */
GLsizei  g_initDrawElements = 0;
/**
 * The number of elements by which the g_numDrawElements should increase each
 * loop of the animation thread
 */
GLsizei  g_stepDrawElements = 0;
/**
 * The number of elements we want to draw from our active EBO
 */
GLsizei g_numDrawElements = 0;
/**
 * This var controls the while loop in the animation thread; if the thread
 * has been started it'll loop until this var is false
 */
bool g_shouldRunAnimationThread = false;

/**
 * Starts the thread managing progression of elements we'll draw from the active EBO,
 * updating the number of elements every interval ms
 * @param periodicFunc the function that be run periodically
 * @param interval the milliseconds that should elapse between executions of periodicFunc
 */
void start_animation(const std::function<void(void)>& periodicFunc, unsigned int interval)
{
    g_shouldRunAnimationThread = true;
    std::thread([periodicFunc, interval]() {
        while (g_shouldRunAnimationThread)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(interval));
            periodicFunc();
        }
    }).detach();
}

/**
 * Stops the thread managing progression of elements we'll draw from the active EBO
 */
void stop_animation()
{
    g_shouldRunAnimationThread = false;
}

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
 * load basic_render.vert as vertex shader and basic_render.frag as fragment shader.
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

/**
 * Performs the OpenGL Dance necessary to summon a vertex array object
 * describing usage of a vertex buffer object which in turn holds vertex data
 * for a simple tightly packed single triangle.
 * @return the ID of the vertex array object that can be bound at a later time for rendering use
 */
unsigned int generateBasicTriangleVAO()
{
    // Config Step 1: create vertex array object to track our config
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Config Step 2: define and buffer data
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

    // Config Step 3: configure vertex attribute pointers to tell OpenGL how to interpret buffered data
    // 0 is the location we specified for our aPos attribute in basic_render.vert
    /*
     * Note: Here's how the bridge between our VBO and our vertex attribute aPos works
     * "
     * Each vertex attribute takes its data from memory
     * managed by a VBO and which VBO it takes its data from
     * (you can have multiple VBOs) is determined by the VBO
     * currently bound to GL_ARRAY_BUFFER when calling
     * glVertexAttribPointer. Since the previously defined
     * VBO is still bound before calling glVertexAttribPointer
     * vertex attribute 0 is now associated with its vertex data.
     * "
     */
    glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            (void*)nullptr
    );
    glEnableVertexAttribArray(0);
    /*
     // note that this is allowed, the call to glVertexAttribPointer registered VBO as the vertex attribute's bound vertex buffer object so afterwards we can safely unbind
    glBindBuffer(GL_ARRAY_BUFFER, 0);

    // You can unbind the VAO afterwards so other VAO calls won't accidentally modify this VAO, but this rarely happens. Modifying other
    // VAOs requires a call to glBindVertexArray anyways so we generally don't unbind VAOs (nor VBOs) when it's not directly necessary.
    // If we unbound here, we would need to rebind in render loop before trying to use this VAO.
    glBindVertexArray(0);


    // uncomment this call to draw in wireframe polygons.
    //glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
     */
    return VAO;
}

/**
 * Performs the OpenGL Dance necessary to summon a vertex array object
 * describing usage of an element buffer object which in turn holds vertex data
 * for two simple tightly packed triangles.
 * @return the ID of the vertex array object that can be bound at a later time for rendering use
 */
unsigned int generateUniqueVertsRectangleVAO()
{
    // Config Step 1: create vertex array object to track our config
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Config Step 2: define and buffer data
    // raw rect data, using device coords directly;
    // these are only the unique vertices of the two triangles!
    float vertices[] = {
        0.5f,  0.5f, 0.0f,  // top right
        0.5f, -0.5f, 0.0f,  // bottom right
        -0.5f, -0.5f, 0.0f,  // bottom left
        -0.5f,  0.5f, 0.0f   // top left
    };
    unsigned int indices[] = {
        0, 1, 3, // first triangle, upper-right half
        1, 2, 3  // second triangle, lower-left half
    };

    /// EBO, deals with indices above ///
    // generate an element buffer object to manage our unique vertices in GPU memory
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    // bind our manager EBO to the appropriate type of GPU buffer,
    // which for element buffer is GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // upload vertex data to the GPU memory buffer we're working with,
    // specifying its size in bytes, the data itself as float array, and
    // finally a constant indicating how often we expect drawable data to change;
    // since we're rendering a static triangle for now, static is fine.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    /// VBO, deals with vertices defined above ///
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

    // Config Step 3: configure vertex attribute pointers to tell OpenGL how to interpret buffered data
    // 0 is the location we specified for our aPos attribute in basic_render.vert
    glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            (void*)nullptr
    );
    glEnableVertexAttribArray(0);

    return VAO;
}

/**
 * Performs the OpenGL Dance necessary to summon a vertex array object
 * describing usage of an element buffer object which in turn holds vertex data
 * for a triforce in GL_TRIANGLES primitive mode and a triforce-ish thing
 * in GL_TRIANGLE_STRIP primitive mode.
 * @return the ID of the vertex array object that can be bound at a later time for rendering use
 */
unsigned int generateTriStripForceVAO()
{
    // Config Step 1: create vertex array object to track our config
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Config Step 2: define and buffer data
    // raw rect data, using device coords directly;
    // these are only the unique vertices of the two triangles!
    float vertices[] = {
            0, -1, 1,   // P0: bottom right of first tri and bottom left of third tri
            -0.5, 0, 1, // P1: top of first tri and bottom left of second tri
            -1, -1, 1,  // P2: bottom left of first tri
            0.5, 0, 1,  // P3: bottom right of second tri and top of third tri
            0, 1, 1,    // P4: top of second tri
            1, -1, 1    // P5: bottom right of third tri
    };
    unsigned int indices[] = {
            0, 1, 2,
            3, 4, 1,
            5, 3, 0
    };

    /// EBO, deals with indices above ///
    // generate an element buffer object to manage our unique vertices in GPU memory
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    // bind our manager EBO to the appropriate type of GPU buffer,
    // which for element buffer is GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // upload vertex data to the GPU memory buffer we're working with,
    // specifying its size in bytes, the data itself as float array, and
    // finally a constant indicating how often we expect drawable data to change;
    // since we're rendering a static tri-strip for now, static is fine.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_DYNAMIC_DRAW);

    /// VBO, deals with vertices defined above ///
    // generate a vertex buffer object to manage our vertices in GPU memory
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    // bind our manager VBO to the appropriate type of GPU buffer,
    // which for vertex buffer is GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // upload vertex data to the GPU memory buffer we're working with,
    // specifying its size in bytes, the data itself as float array, and
    // finally a constant indicating how often we expect drawable data to change;
    // since we're rendering a static tri-mesh for now, static is fine.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_DYNAMIC_DRAW);//GL_STATIC_DRAW);

    // Config Step 3: configure vertex attribute pointers to tell OpenGL how to interpret buffered data
    // 0 is the location we specified for our aPos attribute in animated_render.vert
    glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            (void*)nullptr
    );
    glEnableVertexAttribArray(0);

    return VAO;
}

/**
 * Performs the OpenGL Dance necessary to summon a vertex array object
 * describing usage of an element buffer object which in turn holds vertex data
 * for GL_TRIANGLE_STRIP primitive mode to form a ribbon trail effect.
 * @return the ID of the vertex array object that can be bound at a later time for rendering use
 */
unsigned int generateRibbonTrailVAO()
{
    // Config Step 1: create vertex array object to track our config
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Config Step 2: define and buffer data
    // raw rect data, using device coords directly;
    // these are only the unique vertices of the 6 triangles
    // making up our three quadrilateral ribbon
    float vertices[] = {
        0.75, -0.75, 1.0,
        0.65, 0.25, 1.0,
        0.35, 0.65, 1.0,
        0.45, -0.35, 1.0,
        -0.25, 0.0, 1.0,
        -0.35, 1.0, 1.0,
        -0.95, 0.75, 1.0,
        -0.85, -0.25, 1.0
    };
    unsigned int indices[] = {
        0, 1,
        3, 2,
        4, 5,
        7, 6
    };

    // configure animation via draw element count progression
    g_maxDrawElements = sizeof(indices)/sizeof(indices[0]);
    g_initDrawElements = 2;
    g_stepDrawElements = 2;
    g_numDrawElements = g_initDrawElements;

    // todo: factor out the boilerplate EBO/VBO and common aPos vertex attribute config here into a single function,
    //  which should take float** and unsigned int** for our verts and indices arrays
    //  as well as an enum for the desired usage pattern of buffered data (static, dynamic...)

    /// EBO, deals with indices above ///
    // generate an element buffer object to manage our unique vertices in GPU memory
    unsigned int EBO;
    glGenBuffers(1, &EBO);

    // bind our manager EBO to the appropriate type of GPU buffer,
    // which for element buffer is GL_ELEMENT_ARRAY_BUFFER
    glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, EBO);

    // upload vertex data to the GPU memory buffer we're working with,
    // specifying its size in bytes, the data itself as float array, and
    // finally a constant indicating how often we expect drawable data to change;
    // since we're rendering a static tri-strip for now, static is fine.
    glBufferData(GL_ELEMENT_ARRAY_BUFFER, sizeof(indices), indices, GL_STATIC_DRAW);

    /// VBO, deals with vertices defined above ///
    // generate a vertex buffer object to manage our vertices in GPU memory
    unsigned int VBO;
    glGenBuffers(1, &VBO);

    // bind our manager VBO to the appropriate type of GPU buffer,
    // which for vertex buffer is GL_ARRAY_BUFFER
    glBindBuffer(GL_ARRAY_BUFFER, VBO);

    // upload vertex data to the GPU memory buffer we're working with,
    // specifying its size in bytes, the data itself as float array, and
    // finally a constant indicating how often we expect drawable data to change;
    // since we're rendering a static tri-mesh for now, static is fine.
    glBufferData(GL_ARRAY_BUFFER, sizeof(vertices), vertices, GL_STATIC_DRAW);

    // Config Step 3: configure vertex attribute pointers to tell OpenGL how to interpret buffered data
    // 0 is the location we specified for our aPos attribute in basic_render.vert
    glVertexAttribPointer(
            0,
            3,
            GL_FLOAT,
            GL_FALSE,
            3 * sizeof(float),
            (void*)nullptr
    );
    glEnableVertexAttribArray(0);

    return VAO;
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
    std::string shaderProgramName = "basic_render";
    unsigned int shaderProgramId = loadShaders(shaderProgramName);
    assert(shaderProgramId > 0);

    // generate/configure our VAO
    /*
    unsigned int basicTriangleVAO = generateBasicTriangleVAO();
    */
    /*
    unsigned int unqiueVertsRectangleVAO = generateUniqueVertsRectangleVAO();
    */
    /*
    unsigned int tristripforceVAO = generateTriStripForceVAO();
    */

    unsigned int ribbonTrailVAO = generateRibbonTrailVAO();


    // start off with init anchor verts towards the right of the screen and stretching
    // from somewhere below the Y axis to somewhere above using device coordinates
    float initRibbonVerts[] = {
            0.75, -0.5, 1.0,
            0.65, 0.5, 1.0
    };

    // set up RibbonTrail
    RibbonTrail ribbonTrail(3);
    //unsigned int dynamicRibbonTrailVAO = ribbonTrail.generateRibbonTrailVAO();

    // configure animation timer vars
    g_maxDrawElements = ribbonTrail.getNumIndices();
    g_initDrawElements = 2;
    g_stepDrawElements = 2;
    g_numDrawElements = g_initDrawElements;

    /*
    // animated_render shader modifies vert pos and frag color by trig functions over given time
    int timeSpace = glGetUniformLocation(shaderProgramId, "time");
    if(timeSpace < 0)
    {
        std::cerr << "failed to lookup location of uniform \"time\" in shader program "
        << shaderProgramName << ". GL error code: " << glGetError() << std::endl;
        return -1;
    }
    */

    // todo: figure out how to effectively 'erase' historical ribbon frames after
    //  a certain amount of frames have rendered to give an aging trail effect.
    //  1. Full data and offsets: Lay out all ribbon frame data in the beginning.  Increase num draw elements until some
    //     desirable amount (e.g. 4) is reached and then keep it steady.  Then, start calling
    //     glVertexAttribPointer() afresh in our animation progression functor with an offset that increases
    //     by g_stepDrawElements each animation interval
    //  2. Frame data only: buffer only the data we're going to be drawing, rebuffering with an updated
    //     subset of data each animation interval.  The advantage here is it illustrates a more realistic
    //     scenario where we don't know all the places the ribbon trail will go ahead of time.

    // todo: play with modifying ribbon verts in the shader -- they're basically temporal control points,
    //  giving you the ability to modify the effect at various points in the ribbon's history

    // advance the number of elements to draw by g_stepDrawElements (starting at g_initDrawElements)
    //  until g_maxDrawElements is reached, then reset to g_initDrawElements so we get an
    //  animated ribbon trail effect
    start_animation(
            []{
                if(g_numDrawElements >= g_maxDrawElements)
                {
                    // reset
                    g_numDrawElements = g_initDrawElements;
                }
                else
                {
                    // step
                    g_numDrawElements += g_stepDrawElements;
                }
            },
            5000
    );

    // render loop
    while(!glfwWindowShouldClose(window))
    {
        // handle any user input this frame
        processInput(window);

        // check and call events and swap the buffers
        glfwPollEvents();

        // rendering code
        // Render Step 1: clear screen
        glClearColor(0.2f, 0.3f, 0.3f, 1.0f);
        glClear(GL_COLOR_BUFFER_BIT);
        // Render Step 2: select shader program to use
        glUseProgram(shaderProgramId);
        /*
        // set shader program variables
        glUniform1f(timeSpace, glfwGetTime());
        */
        // Render Step 3: bind the configured VAO
        glBindVertexArray(ribbonTrailVAO);
        // Render Step 4: draw calls
        // specify primitive type triangles
        /* this is for a basic vertex data config, where every vertex is given in the needed order
           as opposed to just the unique vertices

         // and that we want to render vertices starting at index 0 and rendering a total count of 3 vertices
         glDrawArrays(GL_TRIANGLES, 0, 3);
        */
        /* unique vert rectangle
        // and that we want to render 6 vertices in total (not just unique), given by the 6 indices
        // in our EBO which are of type unsigned int
        glDrawElements(GL_TRIANGLES, 6, GL_UNSIGNED_INT, nullptr);
        */
        /*
        // and that we want to render 8 elements (vert indices)
        glDrawElements(GL_TRIANGLE_STRIP, 8, GL_UNSIGNED_INT, nullptr);
        */
        glDrawElements(GL_TRIANGLE_STRIP, g_numDrawElements, GL_UNSIGNED_INT, nullptr);
#ifdef DEBUG
        glPolygonMode(GL_FRONT_AND_BACK, GL_LINE);
#endif

        // render the back buffer to the window
        glfwSwapBuffers(window);
    }

    // free GLFW resources
    glfwTerminate();
    return 0;
}
