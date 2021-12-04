
#ifdef __ANDROID__
//precision mediump float;
#endif

char const* defaultVS =
R"(
#version 140
attribute vec4 iPosition;
attribute vec2 iTexCoord;
uniform mat4 worldMatrix;

layout(std140) uniform Camera
{
  mat4 viewMatrix;
  mat4 viewInverseMatrix;
  mat4 projMatrix;
};

uniform vec2 tcOffset;
uniform vec2 tcScaling;

varying vec2 texCoord;

void main()
{
  texCoord = iTexCoord * tcScaling + tcOffset;
  gl_Position = projMatrix * viewMatrix * worldMatrix * iPosition;
}
)";