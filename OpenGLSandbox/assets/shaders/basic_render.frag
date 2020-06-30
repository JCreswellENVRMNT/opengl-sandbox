#version 460 core
layout(location = 0) out vec4 FragColor;
/**
 * Simply assigns a color to gl_FragColor
 */
void main()
{
    FragColor = vec4(1.0, 0.5, 0.2, 0.5);
}
