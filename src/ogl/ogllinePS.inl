
char const* linePS =
#ifdef __ANDROID__
"#version 320 es\n"
"precision mediump float;\n"
#else
"#version 140\n"
#endif
"uniform vec4 color;\n"
"out vec4 finalColor;\n"
"\n"
"void main()\n"
"{\n"
"  finalColor = color;\n"
"}\n"
;
