///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderNNCommon.hlsl                                            //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderNeuralNetworkTexture =
"RWStructuredBuffer<float> gInput : register(u0);\n"
"RWTexture2D<float4> gOutput : register(u1); \n"

"cbuffer global2  : register(b0)\n"
"{\n"
"    float4 gWid_Hei;\n"
"};\n"

//�e�N�X�`���R�s�[
"[numthreads(1, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNTexCopyCS(int2 inid : SV_DispatchThreadID)\n"
"{\n"
"   float tmp = gInput[gWid_Hei.x * inid.y + inid.x];\n"
"   float4 tmp4;\n"
"   tmp4.x = tmp;\n"
"   tmp4.y = tmp;\n"
"   tmp4.z = tmp;\n"
"   tmp4.w = 1.0f;\n"
"   gOutput[inid] = tmp4;\n"
"}\n";