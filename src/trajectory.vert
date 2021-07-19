#version 330 core

layout(location = 0) in vec4 position;
layout(location = 1) in vec4 color;

uniform mat4 uMVP;
flat out vec4 vColor;

void main()
{
   gl_Position = uMVP * position;
   vColor = color;
}
