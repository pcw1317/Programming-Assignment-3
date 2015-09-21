#version 330 core

uniform sampler2D u_Tex;

in vec2 fs_Texcoord;

out vec4 outColor;

void main() 
{
    outColor = texture2D(u_Tex, fs_Texcoord);
    //outColor = vec4(1.0, 0.5, 0.5, 1.0);
}

