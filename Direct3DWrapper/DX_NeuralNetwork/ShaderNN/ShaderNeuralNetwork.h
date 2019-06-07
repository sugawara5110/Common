///////////////////////////////////////////////////////////////////////////////////////////////////////////
//                                        ShaderNeuralNetwork.hlsl                                       //
///////////////////////////////////////////////////////////////////////////////////////////////////////////

char *ShaderNeuralNetwork =
"#define MAX_DEPTH_NUM 5\n"
"#define MAX_OUTPUT_NUM 10\n"

"RWStructuredBuffer<float> gInNode : register(u0);\n"
"RWStructuredBuffer<float> gOutNode : register(u1);\n"
"RWStructuredBuffer<float> gWeight : register(u2);\n"
"RWStructuredBuffer<float> gInError : register(u3);\n"
"RWStructuredBuffer<float> gOutError : register(u4);\n"
"RWStructuredBuffer<float> gDropOutF : register(u5);\n"

"cbuffer global  : register(b0)\n"
"{\n"
"    float4 gLear_Depth_inputS;\n"//�w�K��:x, �������[��:y, MaxDepth:z, inputSet��:w
"    float4 gNumNode[MAX_DEPTH_NUM];\n"//�e�w��Node��:x, gNode,gError�e�w�J�n�C���f�b�N�X:y
"    float4 gNumWeight[MAX_DEPTH_NUM - 1];\n"//gWeight�e�w�J�n�C���f�b�N�X:x
"    float4 gTarget[MAX_OUTPUT_NUM];\n"//target�l:x
"};\n"

//Dispatch(APP��)(X1, Y1, Z1)numthreads(CS��)(X, Y, Z)
//x,y,z,x1,y1,z1 �́`�Ԗ�
//X,Y,Z,X1,Y1,Z1 ��Max�l
//SV_GroupThreadID:    x, y, z
//SV_GroupID:          x1,y1,z1
//SV_DispatchThreadID: x1*X+x, y1*Y+y, z1*Z+z
//SV_GroupIndex      : z*X*Y+y*X+x
//SV_GroupIndex uint ���̑�uint3

"uint NNFPCSsub(int3 outid)\n"
"{\n"
"   float WeightStartInd = gNumWeight[gLear_Depth_inputS.y].x;\n"
"   float InNodeNum = gNumNode[gLear_Depth_inputS.y].x;\n"
"   float OutNodeNum = gNumNode[gLear_Depth_inputS.y + 1].x;\n"
"   int inputsetInd = outid.z;\n"

"   float tmp = 0.0f;\n"
"   int x = outid.x;\n"
"   for(int i = 0; i < InNodeNum; i++)\n"
"   {\n"
"      tmp += gInNode[InNodeNum * inputsetInd + i] * gWeight[WeightStartInd + InNodeNum * x + i] * \n"
"             gDropOutF[i];\n"
"   }\n"
"   gOutNode[OutNodeNum * inputsetInd + x] = tmp;\n"
"   return OutNodeNum * inputsetInd + x;\n"
"}\n"

"#define FP_X ?**\n"
//���`�dsigmoid
//�o�͑�����񏈗�,���͑������[�v gLear_Depth_inputS.y = 0�`Depth-2�܂�
"[numthreads(FP_X, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNFPCS(int3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = NNFPCSsub(outid);\n"
"   float tmp = gOutNode[index];\n"
"   float sig = 1.0f / (1.0f + pow(2.71828182846, -tmp));\n"
"   gOutNode[index] = sig;\n"
"}\n"

//���`�dReLU
//�o�͑�����񏈗�,���͑������[�v gLear_Depth_inputS.y = 0�`Depth-2�܂�
"[numthreads(FP_X, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNFPReLUCS(int3 outid : SV_DispatchThreadID)\n"
"{\n"
"   uint index = NNFPCSsub(outid);\n"
"   float tmp = gOutNode[index];\n"
"   float sigre = 0.0f;"
"   if(gLear_Depth_inputS.y + 1 == gLear_Depth_inputS.z)sigre = 1.0f / (1.0f + pow(2.71828182846, -tmp));\n"//�ŉ��w�̂�sigmoid
"   else sigre = max(0, tmp);\n"
"   gOutNode[index] = sigre;\n"
"}\n"

//�t�`�d�OTarget�l���� gLear_Depth_inputS.y = Depth-1�̂�
"[numthreads(?**, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void InTargetCS(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   float OutNodeNum = gNumNode[gLear_Depth_inputS.y].x;\n"
"   int x = inid.x;\n"
"   int inputsetInd = inid.z;\n"
"   int outInd = OutNodeNum * inputsetInd + x;\n"
"   gOutError[outInd] = gOutNode[outInd] - gTarget[x].x;\n"
"}\n"

"uint NNBPCS0sub(int3 inid)\n"
"{\n"
"   float WeightStartInd = gNumWeight[gLear_Depth_inputS.y].x;\n"
"   float InNodeNum = gNumNode[gLear_Depth_inputS.y].x;\n"
"   float OutNodeNum = gNumNode[gLear_Depth_inputS.y + 1].x;\n"
"   int inputsetInd = inid.z;\n"

"   float tmp = 0.0f;\n"
"   int x = inid.x;\n"
"   int inInd = InNodeNum * inputsetInd + x;\n"
"   for(int i = 0; i < OutNodeNum; i++)\n"
"   {\n"
"      tmp += gOutError[OutNodeNum * inputsetInd + i] * gWeight[WeightStartInd + InNodeNum * i + x];\n"
"   }\n"
"   gInError[inInd] = tmp;\n"
"   return inInd;\n"
"}\n"

"#define BP_X ?**\n"
//�t�`�dSigmoid
//���͑�����񏈗�,�o�͑������[�v gLear_Depth_inputS.y = Depth-2�`0�܂�
"[numthreads(BP_X, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNBPCS0(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint inInd = NNBPCS0sub(inid);\n"
"   int x = inid.x;\n"
"   float tmp = gInError[inInd];\n"
"   float difSig = 1.0f;\n"
"   if(gLear_Depth_inputS.y != 0.0f)difSig = gInNode[inInd] * (1.0f - gInNode[inInd]);\n"
"   gInError[inInd] = difSig * tmp * gDropOutF[x];\n"
"}\n"

//�t�`�dReLU
//���͑�����񏈗�,�o�͑������[�v gLear_Depth_inputS.y = Depth-2�`0�܂�
"[numthreads(BP_X, 1, 1)]\n"//�ő�X * Y * Z = 1024
"void NNBPReLUCS0(int3 inid : SV_DispatchThreadID)\n"
"{\n"
"   uint inInd = NNBPCS0sub(inid);\n"
"   int x = inid.x;\n"
"   float tmp = gInError[inInd];\n"
"   float relu = 0.0f;\n"
"   if(gInNode[inInd] > 0.0f || gLear_Depth_inputS.y == 0.0f)relu = tmp;\n"
"   gInError[inInd] = relu * gDropOutF[x];\n"
"}\n"

//weight�l�X�V
//gWeight����X�V gLear_Depth_inputS.y = Depth-2�`0�܂�
"[numthreads(?**, ?**, 1)]\n"//�ő�X * Y * Z = 1024
"void NNBPCS1(int2 inXoutY : SV_DispatchThreadID)\n"
"{\n"
"   float WeightStartInd = gNumWeight[gLear_Depth_inputS.y].x;\n"

"   int x = inXoutY.x;\n"//In��
"   int y = inXoutY.y;\n"//Out��
"   float InNodeNum = gNumNode[gLear_Depth_inputS.y].x;\n"
"   float OutNodeNum = gNumNode[gLear_Depth_inputS.y + 1].x;\n"

"   float tmpSum = 0.0f;\n"
"   for(int i = 0; i < gLear_Depth_inputS.w; i++)\n"
"   {\n"
"      int oind = OutNodeNum * i + y;\n"
"      int iind = InNodeNum * i + x;\n"
"      tmpSum += gOutError[oind] * gInNode[iind] * gLear_Depth_inputS.x;\n"
"   }\n"
"   float tmpAve = tmpSum / gLear_Depth_inputS.w;\n"

"   int w = gNumNode[gLear_Depth_inputS.y].x * y + x;\n"
"   gWeight[WeightStartInd + w] -= tmpAve;\n"
"}\n";

