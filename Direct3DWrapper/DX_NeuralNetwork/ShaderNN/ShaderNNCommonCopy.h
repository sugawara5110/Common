///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderNNCommonCopy.hlsl                                        //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char* ShaderNNCommonCopy =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWStructuredBuffer<float> gOutput : register(u1); \n"

"cbuffer global  : register(b0)\n"
"{\n"
"   uint gNumNode;\n"
"};\n"

//リソース拡張, src側並列
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void ResourceExpansionCS(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint srcInputSetSt = gNumNode * inid.z;\n"
"   uint srcNodeIndex = srcInputSetSt + inid.x;\n"
"   uint destInputSetSt = gNumNode * 2.0f * inid.z;\n"
"   uint destNodeIndex1 = destInputSetSt + inid.x;\n"
"   uint destNodeIndex2 = destNodeIndex1 + gNumNode;\n"
"   gOutput[destNodeIndex1] = gInput[srcNodeIndex];\n"
"   gOutput[destNodeIndex2] = gInput[srcNodeIndex];\n"
"}\n"

//リソース縮小, dest側並列
"[numthreads(1, 1, 1)]\n"//最大X * Y * Z = 1024
"void ResourceReductionCS(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint destInputSetSt = gNumNode * inid.z;\n"
"   uint destNodeIndex = destInputSetSt + inid.x;\n"
"   uint srcInputSetSt = gNumNode * 2.0f * inid.z;\n"
"   uint srcNodeIndex1 = srcInputSetSt + inid.x;\n"
"   uint srcNodeIndex2 = srcNodeIndex1 + gNumNode;\n"
"   gOutput[destNodeIndex] = (gInput[srcNodeIndex1] + gInput[srcNodeIndex2]) * 0.5f;\n"
"}\n";

