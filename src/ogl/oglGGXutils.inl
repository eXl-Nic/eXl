"float saturate(float value)\n"
"{\n"
"  return clamp(value,0.0,1.0);\n"
"}\n"

"vec2 saturate(vec2 value)\n"
"{\n"
"  return clamp(value,0.0,1.0);\n"
"}\n"

"vec3 saturate(vec3 value)\n"
"{\n"
"  return clamp(value,0.0,1.0);\n"
"}\n"

"vec4 saturate(vec4 value)\n"
"{\n"
"  return clamp(value,0.0,1.0);\n"
"}\n"

"float chiGGX(float v)\n"
"{\n"
"  return step(0.0, v)/*v > 0 ? 1 : 0*/;\n"
"}\n"

"float GGX_Distribution(vec3 n, vec3 h, float alpha)\n"
"{\n"
"  float NoH = dot(n,h);\n"
"  float alpha2 = alpha * alpha;\n"
"  float NoH2 = NoH * NoH;\n"
"  float den = NoH2 * alpha2 + (1 - NoH2);\n"
"  return (chiGGX(NoH) * alpha2) / ( M_PI * den * den );\n"
"}\n"

//"  float k = (1 + roughness)\n"
//"  k = k * k / 8\n"

"float G_Smith_Partial(vec3 n, vec3 v, float k)\n"
"{\n"
"  float NoV = dot(n, v);\n"
"  return NoV / (k + (1 - k) * NoV);\n"
"}\n"
"\n"
"float G_Smith(vec3 n, vec3 l, vec3 v, float k)\n"
"{\n"
"  return G_Smith_Partial(n,l,k) * G_Smith_Partial(n,v,k);\n"
"}\n"
"\n"
"vec3 Fresnel_Schlick(float cosT, vec3 F0)\n"
"{\n"
"  return F0 + (1-F0) * pow( 1 - cosT, 5);\n"
"}\n"

"float radicalInverse_VdC(uint bits) {\n"
"  bits = (bits << 16u) | (bits >> 16u);\n"
"  bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);\n"
"  bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);\n"
"  bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);\n"
"  bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);\n"
"  return float(bits) * 2.3283064365386963e-10; // / 0x100000000\n"
"}\n"
"\n"

"vec2 hammersley2d(uint i, uint N) {\n"
"  return vec2(float(i)/float(N), radicalInverse_VdC(i));\n"
"}\n"

"vec3 GenerateGGXsampleVector(uint i, uint N, float roughness, vec3 iX, vec3 iY, vec3 iZ)\n"
"{\n"
"  vec2 quantVec = hammersley2d(i, N);\n"
"  vec3 mainDir = normalize(cos(quantVec.y * 2 * M_PI) * iX + sin(quantVec.y * 2 * M_PI) * iY);\n"
"  float sinTS = roughness * sqrt(quantVec.x) /*/ sqrt(1 - quantVec.x)*/;\n"
"  vec3 offset = mainDir * sinTS;\n"
"  return normalize(iZ + offset);\n"
"}\n"