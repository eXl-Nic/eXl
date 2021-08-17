char const* skyBoxPS = 
#ifdef __ANDROID__
"precision mediump float;\n"
#endif
"varying vec3 texCoord;\n"
"uniform samplerCube iSkyBox;\n"
"\n"
"void main()\n"
"{\n"
"  gl_FragColor = vec4(texture(iSkyBox, texCoord).xyz, 1.0);\n"
//"  fragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);\n"
"}\n"
;

//"  vec4 tempFragColor = texture2D(iDiffuseTexture,texCoord);\n"
//"  vec4 tempFragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);\n"
//"out vec4 fragColor;\n"

