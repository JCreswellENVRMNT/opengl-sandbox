#version 460 core

/**
 This attribute gives us triangle position data, and we specify here that it should
 show up at location 0 so we don't have to lookup attribute location at runtime.
 */
layout (location = 0) in vec3 aPos;

void main()
{
    gl_Position = vec4(aPos.x, aPos.y, aPos.z, 1.0);
}