#version 330 core

//from https://github.com/cforfang/opengl-shadowmapping/blob/master/src/vsmcube/shadowVertexShader.glsl

layout(location = 0) in vec3 position;

out vec4 v_position;

uniform mat4 u_model;
uniform mat4 u_cameraToShadowView;
uniform mat4 u_cameraToShadowProjector;

void main() {
	gl_Position = u_cameraToShadowProjector * u_model * vec4(position, 1.0);
	v_position = u_cameraToShadowView * u_model * vec4(position, 1.0);
};