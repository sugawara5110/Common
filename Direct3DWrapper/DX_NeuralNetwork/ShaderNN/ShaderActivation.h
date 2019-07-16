///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                    ShaderActivation.hlsl                                              //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderActivation =
"RWStructuredBuffer<float> gNode : register(u0);\n"
"RWStructuredBuffer<float> gErr : register(u1);\n"

"#define MAX_OUTPUT_NUM 255\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gTarget[MAX_OUTPUT_NUM];\n"//.xÇÃÇ›
"    float gActivationAlpha;\n"
"    uint gNumNode;\n"
"};\n"

"uint IndexCS(uint3 outid)\n"
"{\n"
"   uint indexInputSet = gNumNode * outid.z;\n"
"   return indexInputSet + outid.x;\n"
"}\n"

"#define FBP_X ?**\n"
//èáì`îdsigmoid
"[numthreads(FBP_X, 1, 1)]\n"
"void FPsigmoidCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   float tmp = gNode[index];\n"
"   gNode[index] = 1.0f / (1.0f + exp(-tmp));\n"
"}\n"

//ãtì`îdSigmoidCrossEntropy
"[numthreads(FBP_X, 1, 1)]\n"
"void BPSigmoidCECS(int3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   uint tarInd = outid.x;\n"
"   gErr[index] = gNode[index] - gTarget[tarInd].x;\n"
"}\n"

//ãtì`îdSigmoid
"[numthreads(FBP_X, 1, 1)]\n"
"void BPSigmoidCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   float sig = gNode[index] * (1.0f - gNode[index]);\n"
"   gErr[index] = gErr[index] * sig;\n"
"}\n"

//èáì`îdReLU
"[numthreads(FBP_X, 1, 1)]\n"
"void FPReLUCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   float tmp = gNode[index];\n"
"   gNode[index] = max(tmp * gActivationAlpha, tmp);\n"
"}\n"

//ãtì`îdReLU
"[numthreads(FBP_X, 1, 1)]\n"
"void BPReLUCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   if(gNode[index] <= 0.0f)gErr[index] = gErr[index] * gActivationAlpha;\n"
"}\n"

//èáì`îdELU
"[numthreads(FBP_X, 1, 1)]\n"
"void FPELUCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   if(gNode[index] <= 0.0f)\n"
"   {\n"
"      gNode[index] = (exp(gNode[index]) - 1.0f) * gActivationAlpha;\n"
"   }\n"
"}\n"

//ãtì`îdELU
"[numthreads(FBP_X, 1, 1)]\n"
"void BPELUCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   if(gNode[index] <= 0.0f)\n"
"   {\n"
"      gErr[index] = gErr[index] * (gNode[index] + gActivationAlpha);\n"
"   }\n"
"}\n"

//èáì`îdtanh
"[numthreads(FBP_X, 1, 1)]\n"
"void FPtanhCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   float tmp = gNode[index];\n"
"   float ex2 = exp(-2.0f * tmp);\n"
"   gNode[index] = (1.0f - ex2) / (1.0f + ex2);\n"
"}\n"

//ãtì`îdtanh
"[numthreads(FBP_X, 1, 1)]\n"
"void BPtanhCS(uint3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = IndexCS(outid);\n"
"   float tan = 1.0f - gNode[index] * gNode[index];\n"
"   gErr[index] = gErr[index] * tan;\n"
"}\n";
