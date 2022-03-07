char const* skyBoxVS = 
R"(#version 140
attribute vec4 iPosition;
attribute vec4 iNormal;

layout(std140) uniform Camera
{
  mat4 viewMatrix;
  mat4 viewInverseMatrix;
  mat4 projMatrix;
};

varying vec3 texCoord;

void main()
{
  vec4 pos = viewMatrix * vec4(iPosition.xyz, 0);
  pos = projMatrix * vec4(pos.xyz, 1.0);
  texCoord = iPosition.xyz;
  gl_Position = pos;
})"
;