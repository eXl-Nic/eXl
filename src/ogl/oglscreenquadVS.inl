char const* screenQuadVS = 
"#version 140\n"
"\n"
"varying vec2 texCoord;\n"
"void main()\n"
"{\n"
"  vec4 quadPos[4] = vec4[](vec4(-1.0,1.0,0.0,1.0), vec4(-1.0,-1.0,0.0,1.0), vec4(1.0,1.0,0.0,1.0), vec4(1.0,-1.0,0.0,1.0));"
"  texCoord = quadPos[gl_VertexID].xy;\n"
"  gl_Position = quadPos[gl_VertexID];\n"
"}\n"
;