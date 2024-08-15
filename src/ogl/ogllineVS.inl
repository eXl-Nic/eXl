
char const* lineVS =
#if EXL_PLAY_PLATFORM
"#version 300 es\n"
"precision mediump float;\n"
#else
"#version 140\n"
#endif
R"(in vec4 iPosition;
uniform mat4 worldMatrix;

layout(std140) uniform Camera
{
  mat4 viewMatrix;
  mat4 viewInverseMatrix;
  mat4 projMatrix;
};

void main()
{
  gl_Position = projMatrix * viewMatrix * worldMatrix * iPosition;
}
)";