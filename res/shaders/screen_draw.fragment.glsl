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
uniform samplerCube u_shadowTex;

//from http://www.opengl-tutorial.org/intermediate-tutorials/tutorial-16-shadow-mapping/
vec2 poissonDisk[4] = vec2[](
	vec2(-0.94201624, -0.39906216),
	vec2(0.94558609, -0.76890725),
	vec2(-0.094184101, -0.92938870),
	vec2(0.34495938, 0.29387760)
	);

const float kBias = 0.05f;

void main ()
{
	vec3 lightVec = (fs_ViewLightPos + u_vplDirection) - fs_ViewPosition;

	vec3 fragToLight = fs_ViewLightPos - fs_ViewPosition;
	vec3 fragToLightDir = normalize(fragToLight);

	vec4 shadowCubeDir = inverse(u_ViewMat) * vec4(fragToLightDir, 0);
	//float visibility = 1.f;
	//for (int i = 0; i<4; i++) {
	//	//visibility -= length(fragToLight) - 0.05f<= texture(u_shadowTex, -shadowCubeDir.xyz + vec3(poissonDisk[i]/1000.f,0.f)).r ? 0f : 0.25f;
	//	visibility -= length(fragToLight) - kBias <= texture(u_shadowTex, -shadowCubeDir.xyz).r ? 0f : 0.25f;
	//}
	float visibility = length(fragToLight) - kBias <= texture(u_shadowTex, -shadowCubeDir.xyz).r ? 1f : 0f;

	vec3 L = normalize(lightVec);
	float decay = clamp (pow(length (lightVec), -2.0), 0.0, 1);
	float clampedDiffuseFactor = clamp(dot(fs_ViewNormal, L), 0.0, 1.0);
	//vec3 outColor3 = (u_vplIntensity * u_DiffuseColor * clampedDiffuseFactor * decay) * visibility;
	vec3 outColor3 = (u_vplIntensity * u_DiffuseColor * clampedDiffuseFactor * decay);
	outColor3 = clamp(outColor3, 0, 1.0/ u_numLights) * visibility; //clamping sample contribution to reduce artifacts
	
	outColor = vec4 (outColor3, 1.0 / u_numLights);
}