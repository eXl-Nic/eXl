
char const* linePS = 
#ifdef __ANDROID__
"precision mediump float;\n"
#endif
"uniform vec4 color;\n"
"\n"
"void main()\n"
"{\n"
"  gl_FragColor = color;\n"
"}\n"
;
