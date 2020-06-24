#version 460 core

/**
 This attribute gives us vertex position data, and we specify here that it should
 show up at location 0 so we don't have to lookup attribute location at runtime.
 */
layout (location = 0) in vec3 aPos;
/**
 * Current time, passed in from CPU code
 */
uniform float time;

/**
 * Assigns the X, Y, and Z components of attribute aPos to gl_Position,
 * with X translated by sin(time)
 */
void main()
{
    gl_Position = vec4(aPos.x + sin(time), aPos.y, aPos.z, 1.0);
}