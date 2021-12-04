
#ifdef __ANDROID__
//precision mediump float;
#endif

char const* hq2xVS =
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

varying vec4 texCoord[5];

void main()
{
  texCoord[0].xy = iTexCoord * tcScaling + tcOffset;

  vec2 dg1 = 0.5 / imageSize;
  vec2 dg2 = vec2(-dg1.x, dg1.y);
  vec2 dx = vec2(dg1.x, 0.0);
  vec2 dy = vec2(0.0, dg1.y);

  texCoord[1].xy = texCoord[0].xy - dg1;
  texCoord[1].zw = texCoord[0].xy - dy;
  texCoord[2].xy = texCoord[0].xy - dg2;
  texCoord[2].zw = texCoord[0].xy + dx;
  texCoord[3].xy = texCoord[0].xy + dg1;
  texCoord[3].zw = texCoord[0].xy + dy;
  texCoord[4].xy = texCoord[0].xy + dg2;
  texCoord[4].zw = texCoord[0].xy - dx;

  gl_Position = projMatrix * viewMatrix * worldMatrix * iPosition;
}
)";

char const* hq2xPS =
R"(uniform vec4 tint;
varying vec4 texCoord[5];
uniform vec2 tcOffset;
uniform vec2 texSize;
uniform sampler2D iUnfilteredTexture;
uniform float     alphaMult;

const float mx = 0.325;      // start smoothing wt.
const float k = -0.250;      // wt. decrease factor
const float max_w = 0.25;    // max filter weigth
const float min_w =-0.05;    // min filter weigth
const float lum_add = 0.25;  // effects smoothing

void main()
{
  vec2 finalTc00 = fract((texCoord[1].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc10 = fract((texCoord[1].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc20 = fract((texCoord[2].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc01 = fract((texCoord[4].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc11 = fract((texCoord[0].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc21 = fract((texCoord[2].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc02 = fract((texCoord[4].xy - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc12 = fract((texCoord[3].zw - tcOffset) / texSize) * texSize + tcOffset;
  vec2 finalTc22 = fract((texCoord[3].xy - tcOffset) / texSize) * texSize + tcOffset;

  vec4 c00 = texture2D(iUnfilteredTexture, finalTc00);
  vec4 c10 = texture2D(iUnfilteredTexture, finalTc10);
  vec4 c20 = texture2D(iUnfilteredTexture, finalTc20);
  vec4 c01 = texture2D(iUnfilteredTexture, finalTc01);
  vec4 c11 = texture2D(iUnfilteredTexture, finalTc11);
  vec4 c21 = texture2D(iUnfilteredTexture, finalTc21);
  vec4 c02 = texture2D(iUnfilteredTexture, finalTc02);
  vec4 c12 = texture2D(iUnfilteredTexture, finalTc12);
  vec4 c22 = texture2D(iUnfilteredTexture, finalTc22);
  vec3 dt = vec3(1.0, 1.0, 1.0);

  float md1 = dot(abs(c00.xyz - c22.xyz), dt);
  float md2 = dot(abs(c02.xyz - c20.xyz), dt);

  float w1 = dot(abs(c22.xyz - c11.xyz), dt) * md2;
  float w2 = dot(abs(c02.xyz - c11.xyz), dt) * md1;
  float w3 = dot(abs(c00.xyz - c11.xyz), dt) * md2;
  float w4 = dot(abs(c20.xyz - c11.xyz), dt) * md1;

  float t1 = w1 + w3;
  float t2 = w2 + w4;
  float ww = max(t1, t2) + 0.001;

  c11 = (w1 * c00 + w2 * c20 + w3 * c22 + w4 * c02 + ww * c11) / (t1 + t2 + ww);

  float lc1 = k / (0.12 * dot(c10.xyz + c12.xyz + c11.xyz, dt) + lum_add);
  float lc2 = k / (0.12 * dot(c01.xyz + c21.xyz + c11.xyz, dt) + lum_add);

  w1 = clamp(lc1 * dot(abs(c11.xyz - c10.xyz), dt) + mx, min_w, max_w);
  w2 = clamp(lc2 * dot(abs(c11.xyz - c21.xyz), dt) + mx, min_w, max_w);
  w3 = clamp(lc1 * dot(abs(c11.xyz - c12.xyz), dt) + mx, min_w, max_w);
  w4 = clamp(lc2 * dot(abs(c11.xyz - c01.xyz), dt) + mx, min_w, max_w);

  c11 = w1 * c10 + w2 * c21 + w3 * c12 + w4 * c01 + (1.0 - w1 - w2 - w3 - w4) * c11;

  if(c11.w < 0.5)
    discard;
  c11.w = (c11.w - 0.5) * 2.0 *alphaMult;
  gl_FragColor = c11 * tint;
})"
;