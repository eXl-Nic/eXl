
char const* defaultPS = 
#ifdef __ANDROID__
"#version 320 es\n"
"precision mediump float;\n"
#else
"#version 140\n"
#endif
"uniform vec4 tint;\n"
"in vec2 texCoord;\n"
"uniform vec2 tcOffset;\n"
"uniform vec2 texSize;\n"
"out vec4 fragColor;\n"
"uniform sampler2D iDiffuseTexture;\n"
"uniform float     alphaMult;\n"
"\n"
"void main()\n"
"{\n"
"  vec2 finalTc = fract((texCoord - tcOffset) / texSize) * texSize + tcOffset;\n"
#if !defined(__ANDROID__)
"  vec2 tcDx = dFdx(texCoord);\n"
"  vec2 tcDy = dFdy(texCoord);\n"
"  vec4 tempFragColor = texture2DGrad(iDiffuseTexture, finalTc, tcDx, tcDy);\n"
#else
"  vec4 tempFragColor = texture2D(iDiffuseTexture,finalTc);\n"
//"  vec4 tempFragColor = vec4(1.0,1.0,1.0,1.0);\n"
#endif
"  if(tempFragColor.w < 0.5)\n"
"    discard;\n"
"  tempFragColor.w = (tempFragColor.w - 0.5) * 2.0 *alphaMult;\n"
"  fragColor = tempFragColor * tint;\n"
"}\n"
//#endif
//#endif
;

char const* defaultUPS = 
#ifdef __ANDROID__
"#version 320 es\n"
"precision mediump float;\n"
#else
"#version 140\n"
#endif
"uniform vec4 tint;\n"
"in vec2 texCoord;\n"
"uniform vec2 tcOffset;\n"
"uniform vec2 texSize;\n"
"uniform sampler2D iUnfilteredTexture;\n"
"uniform float     alphaMult;\n"
"out vec4 fragColor;\n"
"\n"
"void main()\n"
"{\n"
"  vec2 finalTc = fract((texCoord - tcOffset) / texSize) * texSize + tcOffset;\n"
"  vec4 tempFragColor = texture2D(iUnfilteredTexture,finalTc);\n"
//"  vec4 tempFragColor = vec4(1.0, 1.0 ,1.0 ,1.0);\n"
"  if(tempFragColor.w < 0.5)\n"
"    discard;\n"
"  tempFragColor.w = (tempFragColor.w - 0.5) * 2.0 *alphaMult;\n"
"  fragColor = tempFragColor * tint;\n"
"}\n"
;

char const* fontPS = 

#ifdef __ANDROID__
"#version 320 es\n"
"precision mediump float;\n"
#else
"#version 140\n"
#endif
"in vec2 texCoord;\n"
"out vec4 fragColor;\n"

"uniform vec4      tint;\n"
"uniform float     alphaMult;\n"
"uniform sampler2D iUnfilteredTexture;\n"
"\n"
"void main()\n"
"{\n"
"  float alpha = texture2D(iUnfilteredTexture,texCoord,0.0).x;\n"
//Do not discard on mobile, set alpha to zero instead ?
//"  if(alpha <= 0.01)\n"
//"    discard;\n"
//"  vec4 fragColor = vec4(iColor.x,iColor.y,iColor.z,iColor.w*alpha*iAlphaMult);\n"
"  fragColor = vec4(tint.x,tint.y,tint.z,alpha*alphaMult);\n"

"}\n"
//#endif
//#endif
;
