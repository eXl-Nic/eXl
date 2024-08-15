char const* meshNormalPS = 
#if EXL_PLAY_PLATFORM
"precision mediump float;\n"
#else
"#version 140\n"
#endif
R"(
in vec3 worldNormalU;

out vec4 fragColor;

void main()
{
  fragColor = vec4( (normalize(worldNormalU) + vec3(1.0)) / 2.0, 1);

})"
;


