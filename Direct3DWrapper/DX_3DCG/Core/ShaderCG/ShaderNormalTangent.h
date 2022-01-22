///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  ShaderNormalTangent.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderNormalTangent =

"struct NormalTangent\n"
"{\n"
"    float3 normal  : NORMAL;\n"
"    float3 tangent : TANGENT;\n"
"};\n"

"NormalTangent GetTangent(float3 normal, float3x3 world, float3 tangent)\n"
"{\n"
"	NormalTangent Out = (NormalTangent)0;\n"

"   Out.normal = mul(normal, world);\n"
"   Out.normal = normalize(Out.normal);\n"
"   Out.tangent = mul(tangent, world);\n"
"   Out.tangent = normalize(Out.tangent);\n"

"   return Out;\n"
"}\n"

"float3 normalTexConvert(float3 norT, float3 normal, float3 tangent)\n"
"{\n"
"   float3 N = normal;\n"
"   float3 T = normalize(tangent - dot(tangent, N) * N);\n"
"   float3 B = cross(N, T);\n"
"   float3x3 TBN = float3x3(T, B, N);\n"

"   return mul(norT, TBN);\n"
"}\n"

"float3 GetNormal(float3 norTex, float3 normal, float3 tangent)\n"
"{\n"
"   float3 norT = norTex * 2.0f - 1.0f;\n"
"   return normalTexConvert(norT, normal, tangent);\n"
"}\n";
