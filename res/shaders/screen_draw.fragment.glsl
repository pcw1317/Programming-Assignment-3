#version 330 core

in vec3 fs_ViewNormal;
in vec3 fs_ViewPosition;
in vec3 fs_ViewLightPos;

out vec4 outColor;

uniform mat4 u_viewMat;
uniform mat4 u_perspMat;
uniform vec3 u_vplIntensity;
uniform vec3 u_vplDirection;
uniform int u_numLights;
uniform vec3 u_ambientColor;
uniform vec3 u_diffuseColor;
uniform samplerCube u_shadowTex;

const float kBias = 0.05f;

void main()
{
    vec3 fragToLight = fs_ViewLightPos - fs_ViewPosition;
    vec3 fragToLightDir = normalize( fragToLight );
    vec4 shadowCubeDir = inverse( u_viewMat ) * vec4( fragToLightDir, 0 );

    if( length( fragToLight ) - kBias > texture( u_shadowTex, -shadowCubeDir.xyz ).r )
    {
        outColor = vec4( 0.f, 0.f, 0.f, 1.f );
        return;
    }
    float decay = clamp( pow( length( fragToLight ), -2.f ), 0.f, 1.f );
    float clampedDiffuseFactor = abs( dot( fs_ViewNormal, fragToLightDir ) );
    float clampedVplCosineFactor = abs( dot( u_vplDirection, fragToLightDir ) );
    vec3 outColor3 = u_vplIntensity * u_diffuseColor * clampedDiffuseFactor * clampedVplCosineFactor * decay;
    outColor3 = clamp( outColor3, 0.f, 1.f / u_numLights ) //clamping total sample contribution for reducing artifacts ("popping")
                + u_ambientColor / u_numLights; //add light source value for handling emittance as well

    //uncomment following code to see where VPLs are located.
    //
    //if (length(fragToLight) < 8.f) {
    //	outColor3.x *= 0.5f;
    //	outColor3.y *= 0.5f;
    //	outColor3.z += 0.5f;
    //}
    //

    outColor = vec4( outColor3, 1.f );
}