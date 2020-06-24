#version 460 core
layout(location = 0) out vec4 FragColor;
/**
 * Current time, passed in from vertex shader
 */
uniform float time;

/**
 * Assigns a color to gl_FragColor based on sin(time)
 */
void main()
{
    FragColor = vec4(sin(time), sin(time), sin(time), 1.0);
}
