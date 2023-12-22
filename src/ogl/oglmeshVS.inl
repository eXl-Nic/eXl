char const* meshVS =
#ifdef EXL_PLAY_PLATFORM
"precision mediump float;\n"
#else
"#version 140\n"
#endif
R"(
in vec4 iPosition;
in vec4 iNormal;
in vec2 iTexCoord;

layout(std140) uniform Camera
{
  mat4 viewMatrix;
  mat4 viewInverseMatrix;
  mat4 projMatrix;
};

uniform mat4 worldMatrix;

out vec3 viewPos;
out vec3 worldPos;
out vec3 worldNormalU;
out vec3 viewNormal;
out vec2 texCoord;

void main()
{
  texCoord = iTexCoord;
  mat4 worldInvTrans = transpose(inverse(worldMatrix));
  worldNormalU = (worldMatrix * vec4(iNormal.xyz, 0.0)).xyz;
  mat4 worldViewInvTrans = transpose(inverse(viewMatrix * worldMatrix));
  viewNormal = normalize(worldViewInvTrans * vec4(iNormal.xyz, 0)).xyz;
  vec4 pos = worldMatrix * iPosition;
  worldPos = pos.xyz;
  viewPos =  (viewMatrix * pos).xyz;
  gl_Position = projMatrix * vec4(viewPos.xyz, 1.0);
})"
;