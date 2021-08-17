
char const* defaultPS = 
//#if defined(WIN32)  || defined(__LINUX__) && !defined(__ANDROID__)
//"#version 400\n"
//"in vec2 texCoord;\n"
//"out vec4 fragColor;\n"
//"uniform sampler2D iDiffuseTexture;\n"
//"uniform float     iAlphaMult;\n"
//"\n"
//"void main()\n"
//"{\n"
//"  vec4 tempFragColor = texture(iDiffuseTexture,texCoord,0.0);\n"
////Do not discard on mobile, set alpha to zero instead ?
//"  if(tempFragColor.w < 0.5)\n"
//"    discard;\n"
//"  tempFragColor.w = (tempFragColor.w - 0.5) * 2 *iAlphaMult;\n"
//"  fragColor = tempFragColor;\n"
////"  fragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);\n"
//"}\n"
//#else
#ifdef __ANDROID__
"precision mediump float;\n"
#endif
"uniform vec4 tint;\n"
"varying vec2 texCoord;\n"
"uniform vec2 tcOffset;\n"
"uniform vec2 texSize;\n"
//"out vec4 fragColor;\n"
"uniform sampler2D iDiffuseTexture;\n"
"uniform float     alphaMult;\n"
"\n"
"void main()\n"
"{\n"
"  vec2 finalTc = fract((texCoord - tcOffset) / texSize) * texSize + tcOffset;\n"
#ifndef __ANDROID__
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
"  gl_FragColor = tempFragColor * tint;\n"
//"  fragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);\n"
"}\n"
//#endif
//#endif
;

char const* defaultUPS = 
#ifdef __ANDROID__
"precision mediump float;\n"
#endif
"uniform vec4 tint;\n"
"varying vec2 texCoord;\n"
"uniform vec2 tcOffset;\n"
"uniform vec2 texSize;\n"
"uniform sampler2D iUnfilteredTexture;\n"
"uniform float     alphaMult;\n"
"\n"
"void main()\n"
"{\n"
"  vec2 finalTc = fract((texCoord - tcOffset) / texSize) * texSize + tcOffset;\n"
"  vec4 tempFragColor = texture2D(iUnfilteredTexture,finalTc);\n"
//"  vec4 tempFragColor = vec4(1.0, 1.0 ,1.0 ,1.0);\n"
"  if(tempFragColor.w < 0.5)\n"
"    discard;\n"
"  tempFragColor.w = (tempFragColor.w - 0.5) * 2.0 *alphaMult;\n"
"  gl_FragColor = tempFragColor * tint;\n"
"}\n"
;

char const* fontPS = 
//#if defined(WIN32)  || defined(__LINUX__) && !defined(__ANDROID__)
//"#version 400\n"
//"in vec2 texCoord;\n"
//"out vec4 fragColor;\n"
//
//"uniform vec4      iColor;\n"
//"uniform float     iAlphaMult;\n"
//"uniform sampler2D iDiffuseTexture;\n"
////"uniform vec2      iTexDelta;\n"
//"\n"
//"void main()\n"
//"{\n"
//"  float alpha = texture(iDiffuseTexture,texCoord,0.0).x;\n"
//"  fragColor = vec4(iColor.x,iColor.y,iColor.z,iColor.w)*alpha*iAlphaMult;\n"
////Do not discard on mobile, set alpha to zero instead ?
//"  if(alpha <= 0)\n"
//"  {\n"
//"    //float alpha1 = texture(iDiffuseTexture,texCoord - vec2(iTexDelta.x,0.0) ,0.0).x;\n"
//"    //float alpha2 = texture(iDiffuseTexture,texCoord - vec2(0.0,iTexDelta.y) ,0.0).x;\n"
//"    //if(alpha1 <= 0 && alpha2 <= 0)\n"
//"      discard;\n"
//"    //fragColor = vec4(0.0,0.0,0.0,alpha*iAlphaMult);\n"
//"  }"
////"  fragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);\n"
//"}\n"
//#else
#ifdef __ANDROID__
"precision mediump float;\n"
#endif
"varying vec2 texCoord;\n"
//"out vec4 fragColor;\n"

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
"  vec4 fragColor = vec4(tint.x,tint.y,tint.z,alpha*alphaMult);\n"
//"  fragColor = vec4(texCoord.x,texCoord.y,0.0,1.0);\n"
"  gl_FragColor = fragColor;\n"
"}\n"
//#endif
//#endif
;
