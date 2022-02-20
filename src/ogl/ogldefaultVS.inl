
char const* defaultVS =
#ifdef __ANDROID__
"#version 320 es\n"
"precision mediump float;\n"
#else
"#version 140\n"
#endif
R"(in vec4 iPosition;
in vec2 iTexCoord;
uniform mat4 worldMatrix;

layout(std140) uniform Camera
{
  mat4 viewMatrix;
  mat4 viewInverseMatrix;
  mat4 projMatrix;
};

uniform vec2 tcOffset;
uniform vec2 tcScaling;

out vec2 texCoord;

void main()
{
  texCoord = iTexCoord * tcScaling + tcOffset;
  gl_Position = projMatrix * viewMatrix * worldMatrix * iPosition;
}
)";