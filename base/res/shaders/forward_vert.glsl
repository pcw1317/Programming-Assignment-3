#version 330 core

uniform mat4 u_ModelMat;
uniform mat4 u_ViewMat;
uniform mat4 u_PerspMat;
uniform vec3 u_vplPosition;
uniform vec3 u_vplIntensity;
uniform vec3 u_vplDirection;
uniform vec3 u_DiffuseColor;
uniform vec3 u_AmbientColor;
uniform int u_numLights;

in  vec3 Position;
in  vec3 Normal;

out vec3 fs_ViewNormal;
out vec3 fs_ViewPosition;
out vec3 fs_ViewLightPos;
out vec3 fs_LightIntensity;

void main(void) 
{
    fs_ViewNormal = (u_ViewMat * u_ModelMat * vec4(Normal, 0.0)).xyz;
    fs_ViewPosition = (u_ViewMat * u_ModelMat * vec4(Position, 1.0)).xyz;
    fs_ViewLightPos = (u_ViewMat * u_ModelMat * vec4(u_vplPosition, 1.0)).xyz;

    gl_Position = u_PerspMat * u_ViewMat * u_ModelMat * vec4(Position, 1.0);
}