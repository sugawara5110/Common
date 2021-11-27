///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderOptimizer.hlsl                                           //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderOptimizer =
"RWStructuredBuffer<float> gGradient : register(u0);\n"
"RWStructuredBuffer<float> gGradientMovAve1 : register(u1);\n"
"RWStructuredBuffer<float> gGradientMovAve2 : register(u2);\n"
"RWStructuredBuffer<float> gWeight : register(u3);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gLear_b1_b2_eps;\n"//x:äwèKó¶, y:å∏êäó¶b1. z:å∏êäó¶b2, w:î≠éUñhé~ÉpÉâeps
"};\n"

"#define FBP_X ?**\n"
//ämó¶ìIå˘îzç~â∫ñ@SGD(Stochastic Gradient Descent)
"[numthreads(FBP_X, 1, 1)]\n"//ç≈ëÂX * Y * Z = 1024
"void SGDCS(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   gWeight[inid.x] -= gGradient[inid.x] * gLear_b1_b2_eps.x;\n"
"}\n"

//Adam
"[numthreads(FBP_X, 1, 1)]\n"//ç≈ëÂX * Y * Z = 1024
"void AdamCS(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   gGradientMovAve1[inid.x] += (1.0f - gLear_b1_b2_eps.y) * \n"
"                               (gGradient[inid.x] - gGradientMovAve1[inid.x]);\n"
"   gGradientMovAve2[inid.x] += (1.0f - gLear_b1_b2_eps.z) * \n"
"                               (gGradient[inid.x] * gGradient[inid.x] - gGradientMovAve2[inid.x]);\n"
"   gWeight[inid.x] -= gLear_b1_b2_eps.x * gGradientMovAve1[inid.x] / \n"
"                     (sqrt(gGradientMovAve2[inid.x]) + gLear_b1_b2_eps.w);\n"
"}\n";


