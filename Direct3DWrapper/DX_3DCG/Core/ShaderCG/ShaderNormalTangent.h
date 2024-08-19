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

"void getTangentBinormal(in float3 N, out float3 T, out float3 B)\n"
"{\n"
"   float3 a = abs(N);\n"
"   uint xm = ((a.x - a.y) < 0 && (a.x - a.z) < 0) ? 1 : 0;\n"
"   uint ym = (a.y - a.z) < 0 ? (1 ^ xm) : 0;\n"
"   uint zm = 1 ^ (xm | ym);\n"
"   B = cross(N, float3(xm, ym, zm));\n"
"   T = cross(N, B);\n"
"}\n"

"float3 worldToLocal(float3 N, float3 localVec)\n"
"{\n"
"   float3 T;\n"
"   float3 B;\n"
"   getTangentBinormal(N, T, B);\n"
"   return normalize(float3(dot(T, localVec), dot(B, localVec), dot(N, localVec)));\n"
"}\n"

"float3 localToWorld(float3 N, float3 localVec)\n"
"{\n"
"   float3 T;\n"
"   float3 B;\n"
"   getTangentBinormal(N, T, B);\n"
"   return normalize(T * localVec.x + B * localVec.y + N * localVec.z);\n"
"}\n"

"float3 normalTexConvert(float3 norT, float3 normal, float3 tangent)\n"
"{\n"
"   float3 N = normal;\n"
"   float3 T = tangent;\n"
"   float3 B = cross(T, N);\n"
"   return normalize(T * norT.x + B * norT.y + N * norT.z);\n"
"}\n"

"float3 GetNormal(float3 norTex, float3 normal, float3 tangent)\n"
"{\n"
"   float3 norT = norTex * 2.0f - 1.0f;\n"
"   return normalTexConvert(norT, normal, tangent);\n"
"}\n";
