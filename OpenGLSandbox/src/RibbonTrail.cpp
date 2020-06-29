//
// Created by jeffcreswell on 6/26/20.
//

#include "RibbonTrail.h"

RibbonTrail::RibbonTrail(size_t numSegments): mNumSegments(numSegments)
{
    // init indices member based on desired number of segments
    //  A given ribbon segment is a quadrilateral between 4 vertices in the trail.
    //  Given that plus the fact that we're using tri-strip primitive mode means
    //  we basically have 2*numSegments triangles, and 4 + 2*(numSegments-1) vertices because
    //  we need at least 4 unique verts to draw the first segment and then only 2 additional
    //  ones for each new segment after that.
    //  The indices progression needs to account for tri-strip as well, basically following
    //  a pattern of 0, 1, 3, 2, 4, 5, 7, 6... where every other vertex pair's natural traversal order
    //  is reversed; this is because tri-strip's algorithm draws every three adjacent indices
    //  as a triangle and for contiguous quadrilaterals each comprised of two contiguous triangles
    //  this works out to needing an index progression like that given above.
    // todo: hmm, pretty sure OpenGL won't be happy if we build an EBO with indices that
    //  point to data that doesn't exist yet... likely will need to build this stuff in
    //  addVertexPair() along with vertex buffer data
    for(size_t segmentIdx = 0; segmentIdx < numSegments; segmentIdx++)
    {
        // initial 4 for first segment only
        if(segmentIdx == 0)
        {
            mIndices.push_back(0);
            mIndices.push_back(1);
            mIndices.push_back(3);
            mIndices.push_back(2);
        }
        else
        {
            /*
             derive indices from segment idx:
                segment 1 gets 4, 5
                segment 2 gets 7, 6
                segment 3 gets 8, 9...
                number of verts total should match number of indices and
                is given by 4 + 2 * (numSegments-1).  Since we
                drop 4 verts for the first segment idx above as a special case,
                the current indices we need to add are given
                by 4 + 2 * (segmentIdx - 1) for the lower idx
                and 4 + 2 * (segmentIdx - 1) + 1 for the higher idx
                ( - 1 to account for 0 indexing)
             */
            // since we know that segment 1 will be back to natural progression and that
            // every other pair uses reversed progression, we can just check to see if
            // segment idx is even to determine if progression should be flipped
            size_t lowerIdx = 4 + 2 * (segmentIdx - 1);
            if(segmentIdx % 2 == 0)
            {
                // reverse
                mIndices.push_back(lowerIdx + 1);
                mIndices.push_back(lowerIdx);

            }
            else
            {
                // natural progression
                mIndices.push_back(lowerIdx);
                mIndices.push_back(lowerIdx + 1);
            }
        }
    }
}

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

    // todo: insert indices following the every other reverse natural order pattern
    //  We basically only want to populate mIndices completely once, so the population
    //  op here needs to be conditional on whether or not we already have enough indices to
    //  direct drawing of our complete vertex element buffer (where complete vert set is defined
    //  as enough verts to form the input number of quadrilateral segments)

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

    // todo: consider building the whole mIndices vector up front in the ctor
    //  and then only pull as many indices as we have verts into a temp indices
    //  array here that we pass over to OpenGL
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