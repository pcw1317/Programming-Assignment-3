#version 330 core

in vec3 fs_ViewNormal;
in vec3 fs_ViewPosition;
in vec3 fs_ViewLightPos;

out vec4 outColor;

uniform mat4 u_ViewMat;
uniform sampler2D u_ShadowMap;
uniform int u_numLights;
uniform vec3 u_DiffuseColor;
uniform vec3 u_AmbientColor;
uniform vec3 u_vplIntensity;
uniform vec3 u_vplDirection;

void main ()
{
	vec3 outColor3 = vec3 (0.0, 0.0, 0.0);
	vec3 lightVec = (fs_ViewLightPos + u_vplDirection) - fs_ViewPosition;

	vec3 L = normalize(lightVec);
	float decay = clamp (pow(length (lightVec), -2.0), 0.0, 1.0);
	float clampedDiffuseFactor = clamp(dot(fs_ViewNormal, L), 0.0, 1.0);
	outColor3 += (u_vplIntensity * u_DiffuseColor * clampedDiffuseFactor * decay);
    outColor3 = max(u_AmbientColor, outColor3);

	outColor = vec4 (outColor3, 1.0 / u_numLights);
}