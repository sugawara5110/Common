///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                  ShaderNormalTangent.hlsl                                             //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderNormalTangent =
"struct NormalTangent\n"
"{\n"
"    float3 normal  : NORMAL;\n"
"    float3 tangent : TANGENT;\n"
"};\n"

"NormalTangent GetTangent(float3 normal, matrix World, float4 viewUp)\n"
"{\n"
"	NormalTangent Out = (NormalTangent)0;\n"

"   Out.normal = mul(normal, (float3x3)World);\n"
"   Out.normal = normalize(Out.normal);\n"
"   float3 view = normalize(viewUp.xyz);\n"
"   Out.tangent = cross(Out.normal, view);\n"
"   if(Out.tangent.x == 0.0f && Out.tangent.y == 0.0f && Out.tangent.z == 0.0f){"
"      Out.tangent = cross(Out.normal, view - 0.001f);\n"
"   }\n"

"   return Out;\n"
"}\n"

"float3 GetNormal(float3 norTex, float3 normal, float3 tangent)\n"
"{\n"
"   float3 norT = norTex * 2.0f - 1.0f;\n"

"   float3 N = normal;\n"
"   float3 T = normalize(tangent - dot(tangent, N) * N);\n"
"   float3 B = cross(N, T);\n"
"   float3x3 TBN = float3x3(T, B, N);\n"

"   return mul(norT, TBN);\n"
"}\n";