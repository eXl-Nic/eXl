
char const* defaultVS = 
//#if defined(WIN32) || defined(__LINUX__) && !defined(__ANDROID__)
//"#version 400\n"
//"in vec4 iPosition;\n"
//"in vec2 iTexCoord;\n"
//""
//"uniform mat4 iViewMatrix;\n"
//"uniform mat4 iProjMatrix;\n"
//"uniform mat4 iWorldMatrix;\n"
//"\n"
//"out vec2 texCoord;\n"
//"\n"
//"void main()\n"
//"{\n"
//"  texCoord = iTexCoord;\n"
//"  gl_Position = iProjMatrix * iViewMatrix * iWorldMatrix * iPosition;\n"
//"}\n"
//#else

#ifdef __ANDROID__
"precision mediump float;\n"
#endif
"attribute vec4 iPosition;\n"
"attribute vec2 iTexCoord;\n"
""
"uniform mat4 viewMatrix;\n"
"uniform mat4 projMatrix;\n"
"uniform mat4 worldMatrix;\n"
"uniform vec2 tcOffset;\n"
"uniform vec2 tcScaling;\n"
"\n"
"varying vec2 texCoord;\n"
"\n"
"void main()\n"
"{\n"
"  texCoord = iTexCoord * tcScaling + tcOffset;\n"
"  gl_Position = projMatrix * viewMatrix * worldMatrix * iPosition;\n"
"}\n"
//#endif
//#endif
  ;