//
// Created by jeffcreswell on 6/26/20.
//

#ifndef OPENGLSANDBOX_RIBBONTRAIL_H
#define OPENGLSANDBOX_RIBBONTRAIL_H

#include <deque>
#include <vector>
#include <glm/glm.hpp>
#include <glad/glad.h>

/**
 * A sequence of vertex pairs forming the structure of a arbitrarily oriented ribbon trail
 * for a configurable distance back in the history of the structure; every four vertices
 * form a quadrilateral which OpenGL draws from two triangles using GL_TRIANGLE_STRIP
 * primitive mode, and as we add new vert pairs we effectively add a new segment to the ribbon.
 * After a configurable number of segments have rendered, we can start discarding the oldest to
 * create the illusion of e.g. a rocket trail fading in the wind.
 */
class RibbonTrail
{
private:
    /**
     * The complete set of vertices comprising our current ribbon structure, to be uploaded to VBO
     */
    std::deque<glm::vec3> mVertices;
    /**
     * The indices into VBO to be uploaded to the EBO
     */
    std::vector<unsigned int> mIndices;
    /**
     * The number of ribbon segments (complete quadrilaterals) we want to build up to and then
     * maintain, adding new segments at the head of the ribbon and removing the oldest from the tail
     */
    size_t mNumSegments;
    /**
     * Flag indicating that underlying data has been changed and that the render loop
     * should regenerate the buffers via generateRibbonTrailVAO()
     */
    bool mInvalidBuffers = false;
public:
    /**
     * Construct a new RibbonTrail which will build up to the given number of ribbon segments
     * and then maintain that number
     * @param numSegments the maximum number of ribbon segments we want to render at a given time
     */
    explicit RibbonTrail(size_t numSegments);
    /**
     * Adds a vertex pair to the vertex buffer, dropping the oldest pair if we're already at capacity
     * based on the desired mNumSegments
     * @param firstVertex vertex we draw from
     * @param secondVertex vertex we draw to
     */
    void addVertexPair(glm::vec3 firstVertex, glm::vec3 secondVertex);
    /**
     * Generates a VAO, VBO, and EBO to render our set of vertices as a ribbon using GL_TRIANGLE_STRIP
     * @return the ID of the vertex array object that can be bound at a later time for rendering use
     */
    unsigned int generateRibbonTrailVAO();
    /**
     * @return the total number of vertices we'll need to render the desired segment count
     *         using tri-strips
     */
    size_t calculateMaxVertexCount() const;
    /**
     * @return the size of the mVertices deque, which indicates the number of vertices
     *         that currently comprise this ribbon trail
     */
    size_t getVertexCount();
    /**
     * Resets mVertices and mIndices containers, emptying the ribbon's structure
     */
    void resetRibbon();
    /**
     * Raises the mInvalidBuffers flag
     */
    void invalidateBuffers();
    /**
     * @return true if the VBO and EBO are no longer valid with respect to
     *         underlying data and need to be updated via a fresh call
     *         to generateRibbonTrailVAO()
     */
    bool areBuffersInvalid() const;
};


#endif //OPENGLSANDBOX_RIBBONTRAIL_H
