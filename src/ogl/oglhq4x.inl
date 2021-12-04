
#ifdef __ANDROID__
//precision mediump float;
#endif

char const* hq4xVS =
R"(
#version 140

attribute vec4 iPosition;
attribute vec2 iTexCoord;

uniform mat4 worldMatrix;

layout(std140) uniform Camera
{
  mat4 viewMatrix;
  mat4 viewInverseMatrix;
  mat4 projMatrix;
};

uniform vec2 tcOffset;
uniform vec2 tcScaling;
uniform vec2 imageSize;

varying vec4 texCoord[7];

void main()
{
  texCoord[0].xy = iTexCoord * tcScaling + tcOffset;

  vec2 dg1 = 0.5 / imageSize;
  vec2 dg2 = vec2(-dg1.x, dg1.y);
  vec2 sd1 = dg1 * 0.5;
	vec2 sd2 = dg2 * 0.5;
  vec2 ddx = vec2(dg1.x, 0.0);
	vec2 ddy = vec2(0.0, dg1.y);

  texCoord[1].xy = texCoord[0].xy - sd1;
  texCoord[2].xy = texCoord[0].xy - sd2;
  texCoord[3].xy = texCoord[0].xy + sd1;
  texCoord[4].xy = texCoord[0].xy + sd2;
  texCoord[5].xy = texCoord[0].xy - dg1;
  texCoord[6].xy = texCoord[0].xy + dg1;
  texCoord[5].zw = texCoord[0].xy - dg2;
  texCoord[6].zw = texCoord[0].xy + dg2;
  texCoord[1].zw = texCoord[0].xy - ddy;
  texCoord[2].zw = texCoord[0].xy + ddx;
  texCoord[3].zw = texCoord[0].xy + ddy;
  texCoord[4].zw = texCoord[0].xy - ddx;

  gl_Position = projMatrix * viewMatrix * worldMatrix * iPosition;
}
)";

char const* hq4xPS =
R"(uniform vec4 tint;
varying vec4 texCoord[7];
uniform vec2 tcOffset;
uniform vec2 texSize;
uniform sampler2D iUnfilteredTexture;
uniform float     alphaMult;

 const float mx = 1.00;      // start smoothing wt.
 const float k = -1.10;      // wt. decrease factor
 const float max_w = 0.75;   // max filter weigth
 const float min_w = 0.03;   // min filter weigth
 const float lum_add = 0.33; // effects smoothing

void main()
{
  vec2 finalTc_c = fract((texCoord[0].xy - tcOffset) / texSize) * texSize + tcOffset;

  vec2 finalTc_i1 = fract((texCoord[1].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_i2 = fract((texCoord[2].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_i3 = fract((texCoord[3].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_i4 = fract((texCoord[4].xy - tcOffset) / texSize) * texSize + tcOffset;

  vec2 finalTc_o1 = fract((texCoord[5].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_o2 = fract((texCoord[6].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_o3 = fract((texCoord[5].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_o4 = fract((texCoord[6].zw - tcOffset) / texSize) * texSize + tcOffset;
  
  vec2 finalTc_s1 = fract((texCoord[1].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_s2 = fract((texCoord[2].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_s3 = fract((texCoord[3].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc_s4 = fract((texCoord[4].zw - tcOffset) / texSize) * texSize + tcOffset;


  vec4 c  = texture2D(iUnfilteredTexture, finalTc_c);
  vec4 i1 = texture2D(iUnfilteredTexture, finalTc_i1);
  vec4 i2 = texture2D(iUnfilteredTexture, finalTc_i2);
  vec4 i3 = texture2D(iUnfilteredTexture, finalTc_i3);
  vec4 i4 = texture2D(iUnfilteredTexture, finalTc_i4);
  vec4 o1 = texture2D(iUnfilteredTexture, finalTc_o1);
  vec4 o3 = texture2D(iUnfilteredTexture, finalTc_o2);
  vec4 o2 = texture2D(iUnfilteredTexture, finalTc_o3);
  vec4 o4 = texture2D(iUnfilteredTexture, finalTc_o4);
  vec4 s1 = texture2D(iUnfilteredTexture, finalTc_s1);
  vec4 s2 = texture2D(iUnfilteredTexture, finalTc_s2);
  vec4 s3 = texture2D(iUnfilteredTexture, finalTc_s3);
  vec4 s4 = texture2D(iUnfilteredTexture, finalTc_s4);
  vec3 dt = vec3(1.0, 1.0, 1.0);

  float ko1=dot(abs(o1.xyz-c.xyz),dt);
  float ko2=dot(abs(o2.xyz-c.xyz),dt);
  float ko3=dot(abs(o3.xyz-c.xyz),dt);
  float ko4=dot(abs(o4.xyz-c.xyz),dt);

  float k1=min(dot(abs(i1.xyz-i3.xyz),dt),max(ko1,ko3));
  float k2=min(dot(abs(i2.xyz-i4.xyz),dt),max(ko2,ko4));

  float w1 = k2; if(ko3<ko1) w1*=ko3/ko1;
  float w2 = k1; if(ko4<ko2) w2*=ko4/ko2;
  float w3 = k2; if(ko1<ko3) w3*=ko1/ko3;
  float w4 = k1; if(ko2<ko4) w4*=ko2/ko4;

  c=(w1*o1+w2*o2+w3*o3+w4*o4+0.001*c)/(w1+w2+w3+w4+0.001);

  w1 = k*dot(abs(i1.xyz-c.xyz)+abs(i3.xyz-c.xyz),dt)/(0.125*dot(i1.xyz+i3.xyz,dt)+lum_add);
  w2 = k*dot(abs(i2.xyz-c.xyz)+abs(i4.xyz-c.xyz),dt)/(0.125*dot(i2.xyz+i4.xyz,dt)+lum_add);
  w3 = k*dot(abs(s1.xyz-c.xyz)+abs(s3.xyz-c.xyz),dt)/(0.125*dot(s1.xyz+s3.xyz,dt)+lum_add);
  w4 = k*dot(abs(s2.xyz-c.xyz)+abs(s4.xyz-c.xyz),dt)/(0.125*dot(s2.xyz+s4.xyz,dt)+lum_add);

  w1 = clamp(w1+mx,min_w,max_w); 
  w2 = clamp(w2+mx,min_w,max_w);
  w3 = clamp(w3+mx,min_w,max_w); 
  w4 = clamp(w4+mx,min_w,max_w);

  c = (w1*(i1+i3)+w2*(i2+i4)+w3*(s1+s3)+w4*(s2+s4)+c)/(2.0*(w1+w2+w3+w4)+1.0);

  if(c.w < 0.5)
    discard;
  c.w = (c.w - 0.5) * 2.0 *alphaMult;
  gl_FragColor = c * tint;
})"
;