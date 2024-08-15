char const* meshPS = 

#if EXL_PLAY_PLATFORM
"precision mediump float;"
#else
"#version 140"
#endif
R"(
#define M_PI 3.1415926535897932384626433832795

in vec3 viewPos;
in vec3 worldPos;

in vec2 texCoord;

in vec3 worldNormalU;
in vec3 viewNormal;

layout(std140) uniform Camera
{
  mat4 viewMatrix;
  mat4 viewInverseMatrix;
  mat4 projMatrix;
};

uniform sampler2D iDiffuseTexture;
uniform samplerCube iIrradianceMap;
uniform samplerCube iSpecularEnvMap;
uniform sampler2D iEnvBrdfLUT;

uniform vec3     iLightDir;
uniform vec3     iLightColor;
uniform vec3     iDiffuseColor;
uniform vec4     iBRDFParameters;

out vec4 fragColor;

)"
#include "oglGGXutils.inl"
R"(
#define SamplesCount 20u

vec3 GGX_Specular(float roughness, vec3 F0, out vec3 kS )
{
  vec3 viewVectorWorld = -normalize((viewInverseMatrix * vec4(viewPos, 0.0)).xyz);
  vec3 worldNormal = normalize(worldNormalU);
  float minSize = min(textureSize(iSpecularEnvMap, 0).x, textureSize(iSpecularEnvMap, 0).y);
  float lodCoeff = log2(minSize);
//  kS = 1.0;

  float dotUp = abs(dot(worldNormal, vec3(1,0,0)));
  vec3 side = mix(vec3(1,0,0), vec3(0,1,0), step(0.98, dotUp));
  vec3 up = normalize(cross( worldNormal, side));
  side = normalize(cross(up, worldNormal));
  vec3 radiance = vec3(0, 0, 0);
  float  NoV = saturate(dot(worldNormal, viewVectorWorld));
  for(uint i = 0u; i < SamplesCount; ++i)
  {
    // Generate a sample vector in some local space
//    float sinT = 1.0;
    vec3 H = GenerateGGXsampleVector(i, SamplesCount, roughness, side, up, worldNormal);
    vec3 L = reflect(-viewVectorWorld, H);


    float VoH = saturate(dot( H, viewVectorWorld ));
    // Calculate fresnel
    vec3 fresnel = Fresnel_Schlick(VoH , F0 );
    // Geometry term
    float geometry = G_Smith(worldNormal, L, viewVectorWorld, roughness);
    // Calculate the Cook-Torrance denominator
    float denominator = saturate(NoV * dot(H, worldNormal) + 0.05 );
    kS += fresnel;
    // Accumulate the radiance
    vec3 approxSpecular = pow(textureLod(iSpecularEnvMap, L, roughness * lodCoeff).rgb, vec3(1.0/2.2));
    vec2 envBrdf = texture(iEnvBrdfLUT, vec2(roughness, NoV)).xy;
    approxSpecular = approxSpecular* (envBrdf.x + envBrdf.y);
    radiance += approxSpecular * geometry * fresnel * VoH / denominator;
  }

  // Scale back for the samples count
  kS = saturate( kS / (SamplesCount));
  radiance = radiance / (SamplesCount); 

   vec3 L = normalize(-iLightDir);

  float VoH = saturate(dot( worldNormal, viewVectorWorld ));

  // Calculate fresnel
  vec3 fresnel = Fresnel_Schlick( VoH, F0 );
  // Diffusion term
  float diffusion = GGX_Distribution(worldNormal, L, roughness);
  // Geometry term
  float geometry = G_Smith(worldNormal, L, viewVectorWorld, 0.125 * (roughness + 1)* (roughness + 1));
  float denominator = saturate(NoV + 0.05 );

  // Accumulate the radiance
  vec3 addedLight = iLightColor * diffusion * geometry * fresnel * VoH / denominator;
  if(max(addedLight.x , max(addedLight.y, addedLight.z)) > 0.0001)
  {
    radiance += addedLight;
    kS += fresnel;
  }
  kS = saturate(kS);

  return radiance ;
}


void main()
{
  vec3 diffuseSampleDir = normalize(worldNormalU);
  vec3 diffuseColor = texture(iDiffuseTexture, texCoord).xyz;

  float ior = 1.0 + iBRDFParameters.x;
  float roughness = saturate(iBRDFParameters.y - 0.00001) + 0.00001;
  float metallic = iBRDFParameters.z;
  float diffuseCoeff = saturate(iBRDFParameters.w);
//  float specularCoeff = 1.0 - diffuseCoeff;
  
  // Calculate colour at normal incidence
  float F0 = abs ((1.0 - ior) / (1.0 + ior));
  vec3 F0vec = vec3(F0, F0, F0);
  F0vec = F0vec * F0vec;
  F0vec = mix(F0vec, diffuseColor.rgb, metallic);
  
  // Calculate the specular contribution
  vec3 one_v = vec3(1,1,1);
  vec3 ks = vec3(0, 0, 0);
  vec3 specular = GGX_Specular(roughness, F0vec, ks );
  //vec3 specular = vec3(0, 0, 0);

//  specular = saturate(specular) - specular;
  vec3 kd = diffuseCoeff * (one_v - ks) * (1.0 - metallic);
  ks = ks /** specularCoeff*/;
  // Calculate the diffuse contribution
  vec3 irradiance = pow(texture(iIrradianceMap, diffuseSampleDir ).xyz, vec3(1.0/2.2)) + saturate(dot(worldNormalU, -iLightDir)) * iLightColor;
  vec3 diffuse = kd * diffuseColor * irradiance;
  
  fragColor = vec4( pow(specular + diffuse, vec3(2.2)), 1);
})"
;

