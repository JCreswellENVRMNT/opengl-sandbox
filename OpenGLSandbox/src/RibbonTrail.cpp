//
// Created by jeffcreswell on 6/26/20.
//

#include "RibbonTrail.h"

RibbonTrail::RibbonTrail(size_t numSegments): mNumSegments(numSegments){}

// todo: presumably we'll want the ribbon trail to disappear down to nothing when the
//  attached object stops moving, so some removeOldestVertexPair() function that
//  gets called at a time interval possibly conditionally on whether addVertexPair() has been
//  called within that interval (i.e. addVertexPair() resets the clock, such that we can
//  enforce our segment cap and also support clearing the trail after idle time?)

void RibbonTrail::addVertexPair(glm::vec3 firstVertex, glm::vec3 secondVertex)
{
    // figure out if we're at cap, where vertex cap is defined
    //  as our indices count
    size_t vertCap = 4 + 2*(mNumSegments-1);
    if(mVertices.size() >= vertCap)
    {
        // discard the oldest vert pair
        mVertices.pop_front();
        mVertices.pop_front();
    }
    mVertices.push_back(firstVertex);
    mVertices.push_back(secondVertex);

    // check if we need to build up indices
    if(mIndices.size() <= vertCap - 2)
    {
        // check what order the indices should be added in;
        // this derives from the following:
        // 1. the first pair uses natural progression
        // and every-other pair uses reversed natural progression to accommodate
        // tri-strips
        // 2. since we're adding verts only in pairs, dividing current vert count by
        // 2 gives us the number of pairs we've added
        // 3. since we're using number of pairs, we're counting from 1 and therefore
        // we can use check for an even number of pairs to reveal if we need to reverse
        // natural progression order
        size_t vertCount = mVertices.size();
        if((vertCount / 2) % 2)
        {
            // natural progression; lower idx is vertCount because of 0-based array index
            mIndices.push_back(vertCount);
            mIndices.push_back(vertCount + 1);
        }
        else
        {
            // reverse
            mIndices.push_back(vertCount + 1);
            mIndices.push_back(vertCount);
        }
    }
}

size_t RibbonTrail::getNumIndices()
{
    return mIndices.size();
}

unsigned int RibbonTrail::generateRibbonTrailVAO()
{
    // Config Step 1: create vertex array object to track our config
    unsigned int VAO;
    glGenVertexArrays(1, &VAO);
    glBindVertexArray(VAO);

    // Config Step 2: extract vertex data and buffer it
    size_t vertFloatCount = mVertices.size() * 3;
    float vertices[vertFloatCount];
    for(size_t vertIdx = 0; vertIdx < mVertices.size(); vertIdx++)
    {
        vertices[vertIdx * 3] = mVertices[vertIdx].x;
        vertices[vertIdx * 3 + 1] = mVertices[vertIdx].y;
        vertices[vertIdx * 3 + 2] = mVertices[vertIdx].z;
    }
    unsigned int* indices = mIndices.data();

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