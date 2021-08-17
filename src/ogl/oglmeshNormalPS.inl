char const* meshNormalPS = 
#ifdef __ANDROID__
"precision mediump float;\n"
#endif

"varying vec3 worldNormalU;\n"

"\n"
"void main()\n"
"{\n"
"  gl_FragColor = vec4( (normalize(worldNormalU) + vec3(1.0)) / 2.0, 1);\n"

"}\n"
;


